#!/usr/bin/env python3
"""Red-green check for the tests (tests/README.md): breaks the game's code on purpose, one
mutation at a time, and checks that the tests catch each one.

    tests/mutate.py [-j WORKERS] [-k PATTERN] [--list]

The mutations are in tests/mutations/*.txt:

    == strutil: StrHash multiplier          <- name (unique)
    file: src/core/strutil.c                <- the file to change
    expect: StrHash_is_the_31_polynomial    <- optional, repeatable: tests that must fail
    - h = h * 31 + s[i];                    <- the text to replace (one or more "- " lines,
    + h = h * 37 + s[i];                       joined with newlines; must occur exactly once)
                                            <- replacement ("+ " lines; none = delete)
Lines starting with # are comments. A bare "-" or "+" line is an empty line.

Each worker copies src/, include/ and tests/ to a temporary folder and builds there, so the
working tree is never touched. A mutation passes ("killed") when it builds and at least one
test fails (and every `expect` test is among the failures). Exits 1 if any mutation survives,
doesn't apply or doesn't build.
"""
import argparse
import glob
import multiprocessing
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEPS = os.environ.get("WB_DEPS") or os.path.join(ROOT, "build", "deps-test", "prefix")


class Mutation:
    def __init__(self, name, where):
        self.name, self.where = name, where
        self.file = None
        self.old, self.new, self.expect = [], [], []


def parse():
    muts = []
    for path in sorted(glob.glob(os.path.join(ROOT, "tests", "mutations", "*.txt"))):
        cur = None
        with open(path) as f:
            for no, line in enumerate(f, 1):
                line = line.rstrip("\n")
                where = f"{os.path.relpath(path, ROOT)}:{no}"
                if line.startswith("#") or (not line.strip() and cur is None):
                    continue
                if line.startswith("== "):
                    cur = Mutation(line[3:].strip(), where)
                    muts.append(cur)
                elif cur is None:
                    sys.exit(f"{where}: expected '== name'")
                elif line.startswith("file:"):
                    cur.file = line[5:].strip()
                elif line.startswith("expect:"):
                    cur.expect.append(line[7:].strip())
                elif line == "-" or line.startswith("- "):
                    cur.old.append(line[2:])
                elif line == "+" or line.startswith("+ "):
                    cur.new.append(line[2:])
                elif not line.strip():
                    continue
                else:
                    sys.exit(f"{where}: can't parse: {line}")
    names = set()
    for m in muts:
        if m.name in names:
            sys.exit(f"{m.where}: duplicate mutation name {m.name!r}")
        names.add(m.name)
        if not m.file or not m.old:
            sys.exit(f"{m.where}: {m.name!r} needs a file: and - lines")
    return muts


def run(cmd, cwd, timeout=None):
    try:
        p = subprocess.run(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           text=True, errors="replace", timeout=timeout)
        return p.returncode, p.stdout
    except subprocess.TimeoutExpired as e:
        return -1, (e.stdout or "") if isinstance(e.stdout, str) else "timed out"


class Worker:
    def __init__(self, parent=None):
        self.dir = tempfile.mkdtemp(prefix="wbmutate-", dir=parent)
        try:
            for d in ("src", "include", "tests"):
                shutil.copytree(os.path.join(ROOT, d), os.path.join(self.dir, d),
                                ignore=shutil.ignore_patterns("__pycache__"))
            self.build = os.path.join(self.dir, "build")
            cmd = ["cmake", "-S", os.path.join(self.dir, "tests"), "-B", self.build, "-G", "Ninja"]
            if os.path.isdir(DEPS):
                cmd.append(f"-DCMAKE_PREFIX_PATH={DEPS}")
            code, out = run(cmd, self.dir)
            if code:
                raise RuntimeError("configure failed:\n" + out)
        except BaseException:
            shutil.rmtree(self.dir, ignore_errors=True)
            raise

    def test(self, engine, patterns=()):
        code, out = run(["cmake", "--build", self.build], self.dir)
        if code:
            return "build", out
        exe = "wbengine" if engine else "wbtests"
        if not os.path.exists(os.path.join(self.build, exe)):
            return "build", f"{exe} wasn't built (SDL not found?)"
        code, out = run([os.path.join(self.build, exe), "-j", "2", *patterns], self.build, timeout=900)
        if code not in (0, 1):
            return "runner", out or "test runner terminated or timed out"
        return ("fail" if code else "pass"), out

    def mutate(self, m):
        path = os.path.join(self.dir, m.file)
        try:
            with open(path) as f:
                original = f.read()
        except OSError as e:
            return "nomatch", str(e)
        old, new = "\n".join(m.old), "\n".join(m.new)
        count = original.count(old)
        if count != 1:
            return "nomatch", f"the text occurs {count} times in {m.file}"
        with open(path, "w") as f:
            f.write(original.replace(old, new))
        try:
            return self.test(os.path.basename(m.file).startswith("sdl"), m.expect)
        finally:
            with open(path, "w") as f:
                f.write(original)


_worker = None
_worker_error = None


def init_worker(parent):
    global _worker, _worker_error
    try:
        _worker = Worker(parent)
    except Exception as e:
        # Pool repeatedly spawns replacements when an initializer raises. Report the
        # configure failure through each task so the pool can exit and clean up.
        _worker_error = str(e)


def check(m):
    if _worker_error is not None:
        return m, "build", _worker_error
    status, out = _worker.mutate(m)
    failed = re.findall(r"^FAIL (\S+)", out, re.M)
    if status == "fail":
        if not failed:
            return m, "runner", out[-3000:] or "test runner failed without reporting a failed test"
        missing = [e for e in m.expect if not any(e in t for t in failed)]
        if missing:
            return m, "missed", f"expected failures not seen: {missing}; failed: {failed}"
        return m, "killed", ", ".join(failed[:6]) + (" ..." if len(failed) > 6 else "")
    if status == "pass":
        return m, "survived", ""
    return m, status, out[-3000:]


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("-j", type=int, default=max(1, (os.cpu_count() or 2) // 2))
    ap.add_argument("-k", action="append", help="only mutations whose name contains this")
    ap.add_argument("--list", action="store_true")
    args = ap.parse_args()
    muts = parse()
    if args.k:
        muts = [m for m in muts if any(k in m.name for k in args.k)]
    if args.list:
        for m in muts:
            print(f"{m.name}  ({m.where})")
        return 0

    print("baseline: building and running the unmutated tests ...", flush=True)
    with tempfile.TemporaryDirectory(prefix="wbmutate-baseline-") as base_dir:
        base = Worker(base_dir)
        for engine in (False, True):
            status, out = base.test(engine)
            if status == "build" and engine and not os.path.isdir(DEPS):
                print("  (no engine tests: run tests/build-deps.sh for them)")
                continue
            if status != "pass":
                print(out[-5000:])
                sys.exit("baseline isn't green: fix the tests first")

    bad = 0
    with tempfile.TemporaryDirectory(prefix="wbmutate-run-") as run_dir:
        with multiprocessing.Pool(args.j, initializer=init_worker, initargs=(run_dir,)) as pool:
            for m, status, detail in pool.imap_unordered(check, muts):
                ok = status == "killed"
                bad += not ok
                print(f"{'ok  ' if ok else 'BAD '} {status:8} {m.name}" + (f"  [{detail}]" if ok and detail else ""),
                      flush=True)
                if not ok and detail:
                    print("     " + detail.replace("\n", "\n     "))
    print(f"{len(muts)} mutations, {len(muts) - bad} killed, {bad} not")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
