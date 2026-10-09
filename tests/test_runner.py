#!/usr/bin/env python3
"""Small isolated fault-injection checks for runner.c; needs a C compiler on Linux."""

import errno
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parent
PROBE = r'''
#include "test.h"
#include <unistd.h>
TEST(runner_probe_a) { usleep(100000); }
TEST(runner_probe_b) { usleep(100000); }
'''
FAULTS = r'''
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int is(const char *name) {
    const char *fault = getenv("RUNNER_FAULT");
    return fault && strcmp(fault, name) == 0;
}
FILE *tmpfile(void) {
    if (is("tmpfile")) { errno = EMFILE; return NULL; }
    FILE *(*real)(void) = dlsym(RTLD_NEXT, "tmpfile");
    return real();
}
pid_t fork(void) {
    static int calls;
    calls++;
    if (is("fork2") && calls == 2) { errno = EAGAIN; return -1; }
    pid_t (*real)(void) = dlsym(RTLD_NEXT, "fork");
    pid_t pid = real();
    if (pid > 0 && calls == 1) {
        const char *path = getenv("RUNNER_CHILD_PID");
        if (path) {
            int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
            if (fd >= 0) { dprintf(fd, "%ld\n", (long)pid); close(fd); }
        }
    }
    return pid;
}
int dup2(int oldfd, int newfd) {
    if (is("dup2")) { errno = EBADF; return -1; }
    int (*real)(int, int) = dlsym(RTLD_NEXT, "dup2");
    return real(oldfd, newfd);
}
pid_t wait(int *status) {
    static int calls;
    if (is("wait_error")) { errno = ECHILD; return -1; }
    if (is("wait_eintr") && ++calls == 1) { errno = EINTR; return -1; }
    pid_t (*real)(int *) = dlsym(RTLD_NEXT, "wait");
    return real(status);
}
'''


class RunnerInfrastructure(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.scratch = tempfile.TemporaryDirectory(prefix="wb-runner-test-")
        cls.dir = Path(cls.scratch.name)
        try:
            (cls.dir / "probe.c").write_text(PROBE)
            (cls.dir / "faults.c").write_text(FAULTS)
            compiler = os.environ.get("CC", "cc")
            subprocess.run([compiler, "-std=c17", "-D_DEFAULT_SOURCE", "-I", str(ROOT),
                            str(ROOT / "runner.c"), str(cls.dir / "probe.c"), "-o",
                            str(cls.dir / "runner")], check=True)
            subprocess.run([compiler, "-shared", "-fPIC", str(cls.dir / "faults.c"),
                            "-ldl", "-o", str(cls.dir / "faults.so")], check=True)
        except BaseException:
            cls.scratch.cleanup()
            raise

    @classmethod
    def tearDownClass(cls):
        cls.scratch.cleanup()

    def run_fault(self, fault):
        pidfile = self.dir / (fault + ".pid")
        env = os.environ.copy()
        env.update(LD_PRELOAD=str(self.dir / "faults.so"), RUNNER_FAULT=fault,
                   RUNNER_CHILD_PID=str(pidfile))
        result = subprocess.run([str(self.dir / "runner"), "-j", "2"],
                                env=env, capture_output=True, text=True, timeout=5)
        if pidfile.exists():
            pid = int(pidfile.read_text())
            try:
                os.kill(pid, 0)
            except ProcessLookupError:
                pass
            else:
                self.fail(f"runner left child {pid} after {fault}")
        return result

    def test_normal_and_eintr(self):
        for fault in ("none", "wait_eintr"):
            with self.subTest(fault=fault):
                result = self.run_fault(fault)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertIn("2 tests, 0 failed", result.stdout)

    def test_infrastructure_failures(self):
        for fault in ("tmpfile", "fork2", "dup2", "wait_error"):
            with self.subTest(fault=fault):
                result = self.run_fault(fault)
                self.assertEqual(result.returncode, 2, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
