/* Batch G: sched_yield, dup3, getentropy/getrandom, confstr, the rest of the
   *at family (AT_FDCWD), and ftw/nftw tree walking. */
#include "rxdk_test.h"
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/random.h>
#include <ftw.h>
#include <string.h>
#include <errno.h>

/* sched_yield is gated in <sched.h> behind _POSIX_THREADS (set once pthreads
   lands); declare it directly here. */
extern int sched_yield(void);

static int g_files;
static int ftw_cb(const char *p, const struct stat *st, int type) {
    (void)p; (void)st;
    if (type == FTW_F) g_files++;
    return 0;
}
static int g_nf, g_nd;
static int nftw_cb(const char *p, const struct stat *st, int type, struct FTW *fw) {
    (void)p; (void)st; (void)fw;
    if (type == FTW_F) g_nf++;
    else if (type == FTW_DP) g_nd++;
    return 0;
}

int main(void) {
    struct stat st;
    int fd;

    /* ---- sched_yield ---- */
    CHECK_EQI(sched_yield(), 0, "sched_yield -> 0");

    /* ---- dup3 ---- */
    fd = open("T:/p4.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    CHECK(fd >= 0, "open for dup3");
    if (fd >= 0) {
        CHECK_EQI(dup3(fd, 10, 0), 10, "dup3 -> target fd 10");
        errno = 0;
        CHECK(dup3(fd, fd, 0) == -1 && errno == EINVAL, "dup3 with equal fds -> EINVAL");
        close(10);
        close(fd);
    }

    /* ---- getentropy / getrandom ---- */
    unsigned char b[16];
    int i, nz = 0;
    memset(b, 0, sizeof b);
    CHECK_EQI(getentropy(b, sizeof b), 0, "getentropy -> 0");
    for (i = 0; i < 16; i++) if (b[i]) nz = 1;
    CHECK(nz, "getentropy produced non-zero bytes");
    errno = 0;
    CHECK(getentropy(b, 257) == -1 && errno == EIO, "getentropy > 256 -> EIO");
    CHECK_EQI((long)getrandom(b, sizeof b, 0), 16, "getrandom returns the count");

    /* ---- confstr ---- */
    char cb[32];
    CHECK(confstr(_CS_PATH, cb, sizeof cb) > 0, "confstr _CS_PATH length");
    CHECK_STR(cb, "D:\\", "confstr _CS_PATH == launch drive");

    /* ---- *at family (AT_FDCWD) ---- */
    rmdir("T:/atdir");                 /* clean slate on the persistent drive */
    CHECK_EQI(mkdirat(AT_FDCWD, "T:/atdir", 0777), 0, "mkdirat AT_FDCWD");
    CHECK_EQI(fstatat(AT_FDCWD, "T:/atdir", &st, 0), 0, "fstatat AT_FDCWD");
    CHECK(S_ISDIR(st.st_mode), "fstatat reports a directory");
    CHECK_EQI(faccessat(AT_FDCWD, "T:/atdir", F_OK, 0), 0, "faccessat existing");
    errno = 0;
    CHECK(mkdirat(5, "x", 0777) == -1 && errno == ENOSYS, "mkdirat non-AT_FDCWD -> ENOSYS");

    /* ---- ftw / nftw ---- */
    mkdir("T:/ftwt", 0777);
    mkdir("T:/ftwt/sub", 0777);
    fd = open("T:/ftwt/a", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd >= 0) { write(fd, "x", 1); close(fd); }
    fd = open("T:/ftwt/sub/b", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd >= 0) { write(fd, "x", 1); close(fd); }

    g_files = 0;
    CHECK_EQI(ftw("T:/ftwt", ftw_cb, 8), 0, "ftw -> 0");
    CHECK_EQI(g_files, 2, "ftw visited 2 files (a, sub/b)");

    g_nf = 0; g_nd = 0;
    CHECK_EQI(nftw("T:/ftwt", nftw_cb, 8, FTW_DEPTH), 0, "nftw FTW_DEPTH -> 0");
    CHECK_EQI(g_nf, 2, "nftw visited 2 files");
    CHECK(g_nd >= 2, "nftw post-order visited the directories");

    /* tidy up the persistent drive */
    unlink("T:/p4.txt");
    rmdir("T:/atdir");

    CHECK_DONE("posix4");
    return 0;
}
