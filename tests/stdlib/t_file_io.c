/* C standard file I/O over the kernel backend (T: = writable utility drive). */
#include "rxdk_test.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main(void) {
    const char *path = "T:\\t_file_io.txt";
    const char *msg = "hello rxdk360\n";

    FILE *f = fopen(path, "wb");
    CHECK(f != NULL, "fopen for write");
    if (f) {
        size_t w = fwrite(msg, 1, strlen(msg), f);
        CHECK_EQI(w, (long)strlen(msg), "fwrite byte count");
        CHECK(fprintf(f, "n=%d\n", 42) > 0, "fprintf to file");
        CHECK_EQI(fclose(f), 0, "fclose");
    }

    f = fopen(path, "rb");
    CHECK(f != NULL, "reopen for read");
    if (f) {
        char buf[128];
        memset(buf, 0, sizeof buf);
        size_t r = fread(buf, 1, sizeof buf - 1, f);
        CHECK(r > strlen(msg), "fread returned data");
        CHECK(strncmp(buf, msg, strlen(msg)) == 0, "read-back matches written prefix");

        CHECK_EQI(fseek(f, 0, SEEK_SET), 0, "fseek to start");
        CHECK_EQI(ftell(f), 0, "ftell at start");
        CHECK_EQI(fseek(f, 0, SEEK_END), 0, "fseek to end");
        CHECK(ftell(f) == (long)r, "ftell at end == bytes read");

        int c;
        CHECK_EQI(fseek(f, 0, SEEK_SET), 0, "rewind");
        c = fgetc(f);
        CHECK_EQI(c, 'h', "fgetc first char");
        fclose(f);
    }

    /* stat()/fstat() must report the real byte length (a live handle query on
       xenia, not the stale 0 the cached directory entry can carry). */
    off_t total = (off_t)(strlen(msg) + strlen("n=42\n"));
    struct stat sb;
    CHECK_EQI(stat(path, &sb), 0, "stat succeeds");
    CHECK(sb.st_size == total, "stat st_size == bytes written (non-zero)");
    CHECK(S_ISREG(sb.st_mode), "stat st_mode is a regular file");

    int fd = open(path, O_RDONLY);
    CHECK(fd >= 0, "open for fstat");
    if (fd >= 0) {
        struct stat fb;
        CHECK_EQI(fstat(fd, &fb), 0, "fstat succeeds");
        CHECK(fb.st_size == total, "fstat st_size == bytes written (non-zero)");
        close(fd);
    }

    /* C11 exclusive create: "x" after "w" fails, with EEXIST, if the file exists,
       and must not truncate it. picolibc's mode parser once ignored the "x", so
       "wx" silently truncated an existing file (SDL's testautomation caught it). */
    errno = 0;
    f = fopen(path, "wx");
    CHECK(f == NULL, "fopen \"wx\" fails on an existing file");
    CHECK_EQI(errno, EEXIST, "fopen \"wx\" on an existing file sets EEXIST");
    if (f) fclose(f);
    errno = 0;
    f = fopen(path, "w+x");
    CHECK(f == NULL, "fopen \"w+x\" fails on an existing file");
    CHECK_EQI(errno, EEXIST, "fopen \"w+x\" on an existing file sets EEXIST");
    if (f) fclose(f);
    CHECK_EQI(stat(path, &sb), 0, "stat after refused \"wx\"");
    CHECK(sb.st_size == total, "refused \"wx\" left the file untouched");

    CHECK_EQI(remove(path), 0, "remove file");
    f = fopen(path, "rb");
    CHECK(f == NULL, "removed file no longer opens");
    if (f) fclose(f);

    f = fopen(path, "wbx");
    CHECK(f != NULL, "fopen \"wbx\" creates a file that does not exist");
    if (f) {
        CHECK(fputs("x\n", f) >= 0, "write to the exclusively created file");
        CHECK_EQI(fclose(f), 0, "fclose the exclusively created file");
    }
    CHECK_EQI(remove(path), 0, "remove the exclusively created file");

    /* Why a file call failed reaches errno (it used to be ENOENT, EIO or EEXIST
       whatever the reason). */
    errno = 0;
    CHECK(stat("T:\\t_file_io_missing.txt", &sb) != 0 && errno == ENOENT, "stat of a missing file: ENOENT");
    errno = 0;
    CHECK(unlink("T:\\t_file_io_missing.txt") != 0 && errno == ENOENT, "unlink of a missing file: ENOENT");
    errno = 0;
    CHECK(mkdir("T:\\t_file_io_nodir\\sub", 0777) != 0 && errno == ENOENT, "mkdir under a missing folder: ENOENT");
    CHECK_EQI(mkdir("T:\\t_file_io_dir", 0777), 0, "mkdir");
    errno = 0;
    CHECK(mkdir("T:\\t_file_io_dir", 0777) != 0 && errno == EEXIST, "mkdir of an existing folder: EEXIST");
    f = fopen("T:\\t_file_io_dir\\inner.txt", "w");
    CHECK(f != NULL, "create a file in the folder");
    if (f) fclose(f);
    errno = 0;
    CHECK(rmdir("T:\\t_file_io_dir") != 0 && errno == ENOTEMPTY, "rmdir of a non-empty folder: ENOTEMPTY");
    CHECK_EQI(remove("T:\\t_file_io_dir\\inner.txt"), 0, "remove the inner file");
    CHECK_EQI(rmdir("T:\\t_file_io_dir"), 0, "rmdir the emptied folder");

    /* tmpfile()/tmpnam(): reference them so the suite also guards their link --
       nothing else here did, which once let 52 samples ship with an undefined
       tmpfile. RXDK's libs/libc/xbox/tmpio.c provides both, targeting the Z: scratch
       drive directly (they ignore the cwd, unlike stock picolibc), so no cwd swap is
       needed. (The 360 copy chdirs to cache:/ because its picolibc tmpfile uses cwd.) */
    FILE *tf = tmpfile();
    CHECK(tf != NULL, "tmpfile opens a stream");
    if (tf) {
        CHECK(fputs("tmp\n", tf) >= 0, "write to tmpfile");
        rewind(tf);
        char tb[16]; memset(tb, 0, sizeof tb);
        CHECK(fgets(tb, sizeof tb, tf) != NULL, "read back from tmpfile");
        CHECK(strcmp(tb, "tmp\n") == 0, "tmpfile round-trip matches");
        CHECK_EQI(fclose(tf), 0, "fclose tmpfile (auto-unlinks)");
    }
    char nm[L_tmpnam];
    CHECK(tmpnam(nm) != NULL, "tmpnam returns a candidate name");

    CHECK_DONE("file_io");
    return 0;
}
