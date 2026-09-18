/* Filename pattern matching: fnmatch shell-glob semantics, glob() expansion
   against a real directory on the writable T: drive, and select() reporting
   the console's always-ready file descriptors. */
#include "rxdk_test.h"
#include <fnmatch.h>
#include <glob.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static void mk(const char *p) {
    int fd = creat(p, 0644);
    if (fd >= 0) {
        write(fd, "x", 1);
        close(fd);
    }
}

int main(void) {
    /* ---- fnmatch ---- */
    CHECK(fnmatch("*.txt", "readme.txt", 0) == 0, "fnmatch: *.txt matches");
    CHECK(fnmatch("*.txt", "readme.dat", 0) == FNM_NOMATCH, "fnmatch: *.txt rejects .dat");
    CHECK(fnmatch("a?c", "abc", 0) == 0, "fnmatch: ? matches one char");
    CHECK(fnmatch("a?c", "ac", 0) == FNM_NOMATCH, "fnmatch: ? needs a char");
    CHECK(fnmatch("[a-c]x", "bx", 0) == 0, "fnmatch: range matches");
    CHECK(fnmatch("[a-c]x", "dx", 0) == FNM_NOMATCH, "fnmatch: range excludes");
    CHECK(fnmatch("[!a-c]x", "dx", 0) == 0, "fnmatch: negated range");
    CHECK(fnmatch("READ*", "readme", FNM_CASEFOLD) == 0, "fnmatch: FNM_CASEFOLD");
    CHECK(fnmatch("a/b", "a/b", FNM_PATHNAME) == 0, "fnmatch: PATHNAME literal slash");
    CHECK(fnmatch("a*b", "a/b", FNM_PATHNAME) == FNM_NOMATCH, "fnmatch: * won't cross slash");
    CHECK(fnmatch("*", ".hidden", FNM_PERIOD) == FNM_NOMATCH, "fnmatch: PERIOD guards leading dot");
    CHECK(fnmatch("\\*x", "*x", 0) == 0, "fnmatch: backslash escapes star");

    /* ---- glob against T: ---- */
    mkdir("T:/globtest", 0777);
    mk("T:/globtest/a.txt");
    mk("T:/globtest/b.txt");
    mk("T:/globtest/c.dat");

    glob_t g;
    CHECK_EQI(glob("T:/globtest/*.txt", 0, NULL, &g), 0, "glob: returns 0");
    CHECK_EQI(g.gl_pathc, 2, "glob: found 2 .txt files");
    CHECK_STR(g.gl_pathv[0], "T:/globtest/a.txt", "glob: sorted result [0]");
    CHECK_STR(g.gl_pathv[1], "T:/globtest/b.txt", "glob: sorted result [1]");
    globfree(&g);

    glob_t g2;
    CHECK_EQI(glob("T:/globtest/*.none", 0, NULL, &g2), GLOB_NOMATCH, "glob: GLOB_NOMATCH");
    globfree(&g2);

    glob_t g3;
    CHECK_EQI(glob("T:/globtest/*.none", GLOB_NOCHECK, NULL, &g3), 0, "glob: NOCHECK returns 0");
    CHECK_EQI(g3.gl_pathc, 1, "glob: NOCHECK yields the pattern");
    CHECK_STR(g3.gl_pathv[0], "T:/globtest/*.none", "glob: NOCHECK path is the pattern");
    globfree(&g3);

    mkdir("T:/globtest/sub", 0777);
    glob_t g4;
    glob("T:/globtest/s*", GLOB_MARK, NULL, &g4);
    CHECK(g4.gl_pathc >= 1 &&
          g4.gl_pathv[0][strlen(g4.gl_pathv[0]) - 1] == '/',
          "glob: GLOB_MARK appends / to a directory");
    globfree(&g4);

    /* ---- select: file fds never block ---- */
    fd_set r;
    FD_ZERO(&r);
    FD_SET(0, &r);
    FD_SET(2, &r);
    struct timeval tv = {0, 0};
    CHECK_EQI(select(3, &r, NULL, NULL, &tv), 2, "select: reports 2 ready");
    CHECK(FD_ISSET(0, &r) && FD_ISSET(2, &r), "select: leaves ready fds set");

    CHECK_DONE("posixglob");
    return 0;
}
