// runner.c: Runs the registered tests (test.h), each in a forked child with a time limit.
//
//   wbtests [-l] [-v] [-j N] [PATTERN...]
//
// PATTERNs select tests whose name contains any of them; -l lists the tests, -v shows each
// test's output even when it passes, -j runs N tests at a time. Exits 1 if any test fails.
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "test.h"

enum { MAX_TESTS = 4096, TIME_LIMIT_S = 20 };

typedef struct Test {
    const char *name;
    const char *file;
    TestFn fn;
} Test;

static Test s_tests[MAX_TESTS];

// Runs in each test's process before the test (support.c sets up the fake engine).
__attribute__((weak)) void TestSetUp(void) {}
static int s_count;

void TestRegister(const char *name, const char *file, TestFn fn)
{
    if (s_count == MAX_TESTS) {
        fprintf(stderr, "too many tests\n");
        exit(2);
    }
    s_tests[s_count++] = (Test){name, file, fn};
}

void TestFail(const char *file, int line, const char *fmt, ...)
{
    va_list ap;
    fprintf(stderr, "%s:%d: ", file, line);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    fflush(stderr);
    _exit(1);
}

static int CompareTests(const void *a, const void *b)
{
    const Test *x = a, *y = b;
    int c = strcmp(x->file, y->file);
    return c ? c : strcmp(x->name, y->name);
}

static bool Selected(const Test *t, int argc, char **argv, int first)
{
    if (first >= argc)
        return true;
    for (int i = first; i < argc; i++)
        if (strstr(t->name, argv[i]) || strstr(t->file, argv[i]))
            return true;
    return false;
}

typedef struct Running {
    pid_t pid;
    int index;
    FILE *out;          // the child's stdout+stderr
} Running;

// Starts test `t` in a child whose output goes to a temporary file.
// On failure, leaves no open output file or child behind.
static bool Start(int index, Running *r)
{
    r->out = tmpfile();
    if (!r->out) {
        perror("tmpfile");
        return false;
    }
    r->index = index;
    fflush(stdout);
    fflush(stderr);
    r->pid = fork();
    if (r->pid < 0) {
        perror("fork");
        fclose(r->out);
        r->out = NULL;
        return false;
    }
    if (r->pid == 0) {
        int fd = fileno(r->out);
        if (fd < 0 || dup2(fd, STDOUT_FILENO) < 0 || dup2(fd, STDERR_FILENO) < 0) {
            perror("test output redirect");
            _exit(2);
        }
        setvbuf(stdout, NULL, _IONBF, 0);
        alarm(TIME_LIMIT_S);
        TestSetUp();
        s_tests[index].fn();
        _exit(0);
    }
    return true;
}

// A runner error must not leave tests executing after the runner reports failure.
static void StopRunning(Running *running, int active)
{
    for (int i = 0; i < active; i++)
        kill(running[i].pid, SIGKILL);
    for (int i = 0; i < active; i++) {
        int status;
        while (waitpid(running[i].pid, &status, 0) < 0 && errno == EINTR) {}
        fclose(running[i].out);
    }
}

static void Dump(FILE *f)
{
    char buf[4096];
    size_t n;
    rewind(f);
    while ((n = fread(buf, 1, sizeof buf, f)) > 0)
        fwrite(buf, 1, n, stdout);
}

int main(int argc, char **argv)
{
    bool list = false, verbose = false;
    int jobs = 1, first = 1;
    for (; first < argc && argv[first][0] == '-'; first++) {
        if (!strcmp(argv[first], "-l"))
            list = true;
        else if (!strcmp(argv[first], "-v"))
            verbose = true;
        else if (!strcmp(argv[first], "-j") && first + 1 < argc)
            jobs = atoi(argv[++first]);
        else {
            fprintf(stderr, "usage: %s [-l] [-v] [-j N] [PATTERN...]\n", argv[0]);
            return 2;
        }
    }
    if (jobs < 1)
        jobs = 1;
    qsort(s_tests, s_count, sizeof *s_tests, CompareTests);

    int selected[MAX_TESTS], n = 0;
    for (int i = 0; i < s_count; i++)
        if (Selected(&s_tests[i], argc, argv, first))
            selected[n++] = i;
    if (list) {
        for (int i = 0; i < n; i++)
            printf("%s  (%s)\n", s_tests[selected[i]].name, s_tests[selected[i]].file);
        return 0;
    }

    Running running[64];
    if (jobs > 64)
        jobs = 64;
    int next = 0, active = 0, failed = 0;
    while (next < n || active > 0) {
        while (active < jobs && next < n) {
            if (!Start(selected[next], &running[active])) {
                StopRunning(running, active);
                return 2;
            }
            active++;
            next++;
        }
        int status;
        pid_t pid;
        do { pid = wait(&status); } while (pid < 0 && errno == EINTR);
        if (pid < 0) {
            perror("wait");
            StopRunning(running, active);
            return 2;
        }
        bool matched = false;
        for (int i = 0; i < active; i++) {
            if (running[i].pid != pid)
                continue;
            matched = true;
            if (WIFEXITED(status) && WEXITSTATUS(status) == 2) {
                fprintf(stderr, "test child setup failed: %s\n", s_tests[running[i].index].name);
                Dump(running[i].out);
                fclose(running[i].out);
                running[i] = running[--active];
                StopRunning(running, active);
                return 2;
            }
            const Test *t = &s_tests[running[i].index];
            bool ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
            if (!ok) {
                failed++;
                printf("FAIL %s (%s)", t->name, t->file);
                if (WIFSIGNALED(status))
                    printf(": %s", WTERMSIG(status) == SIGALRM ? "timed out"
                                                               : strsignal(WTERMSIG(status)));
                printf("\n");
                Dump(running[i].out);
            } else if (verbose) {
                printf("ok   %s\n", t->name);
                Dump(running[i].out);
            }
            fclose(running[i].out);
            running[i] = running[--active];
            break;
        }
        if (!matched) {
            fprintf(stderr, "wait returned an unknown child: %ld\n", (long)pid);
            StopRunning(running, active);
            return 2;
        }
    }
    printf("%d tests, %d failed\n", n, failed);
    return failed ? 1 : 0;
}
