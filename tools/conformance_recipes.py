#!/usr/bin/env python3
"""Runtime conformance test bodies for RXDK kit libc/C23 probes.

Header taxonomy follows vendor/stdtests/template/c_header.txt.
Add a (group, name, body) tuple here; regenerate with generate_conformance_tests.py.
Bodies must not #include headers (generator adds file-scope includes).
"""

from __future__ import annotations

CONFORMANCE_TESTS: list[tuple[str, str, str]] = [
    (
        "string",
        "strlen",
        """
    RXDK_TEST_EQ(strlen(""), 0u);
    RXDK_TEST_EQ(strlen("abc"), 3u);
    return 0;
""",
    ),
    (
        "string",
        "strcmp",
        """
    RXDK_TEST_EQ(strcmp("abc", "abc"), 0);
    RXDK_TEST_TRUE(strcmp("abc", "abd") < 0);
    RXDK_TEST_TRUE(strcmp("abd", "abc") > 0);
    return 0;
""",
    ),
    (
        "string",
        "memcpy",
        """
    char dst[8];
    RXDK_TEST_EQ(memcpy(dst, "xy", 3), dst);
    RXDK_TEST_STR_EQ(dst, "xy");
    return 0;
""",
    ),
    (
        "string",
        "memset",
        """
    unsigned char buf[4];
    memset(buf, 0xA5, sizeof buf);
    for (unsigned i = 0; i < sizeof buf; i++)
        if (buf[i] != 0xA5)
            RXDK_TEST_FAIL();
    return 0;
""",
    ),
    (
        "string",
        "strchr",
        """
    const char *p = strchr("abc", 'b');
    RXDK_TEST_TRUE(p != NULL && p[0] == 'b' && p[1] == 'c');
    RXDK_TEST_EQ(strchr("abc", 'z'), NULL);
    return 0;
""",
    ),
    (
        "stdlib",
        "atoi",
        """
    RXDK_TEST_EQ(atoi("42"), 42);
    RXDK_TEST_EQ(atoi("-7"), -7);
    return 0;
""",
    ),
    (
        "stdlib",
        "strtol",
        """
    char *end = NULL;
    long v = strtol("0x10", &end, 16);
    RXDK_TEST_EQ(v, 16L);
    RXDK_TEST_TRUE(end != NULL && *end == '\\0');
    return 0;
""",
    ),
    (
        "stdlib",
        "malloc_free",
        """
    void *p = malloc(64);
    if (p == NULL)
        RXDK_TEST_FAIL();
    free(p);
    return 0;
""",
    ),
    (
        "ctype",
        "isdigit",
        """
    RXDK_TEST_TRUE(isdigit('0'));
    RXDK_TEST_TRUE(isdigit('9'));
    RXDK_TEST_FALSE(isdigit('a'));
    return 0;
""",
    ),
    (
        "ctype",
        "toupper",
        """
    RXDK_TEST_EQ(toupper('a'), 'A');
    RXDK_TEST_EQ(toupper('Z'), 'Z');
    return 0;
""",
    ),
    (
        "errno",
        "errno_rw",
        """
    errno = 123;
    RXDK_TEST_EQ(errno, 123);
    errno = 0;
    return 0;
""",
    ),
    (
        "stddef",
        "offsetof",
        """
    struct s { char a; int b; };
    RXDK_TEST_TRUE(offsetof(struct s, b) >= 1);
    return 0;
""",
    ),
    (
        "limits",
        "char_bit",
        """
    RXDK_TEST_EQ(CHAR_BIT, 8);
    return 0;
""",
    ),
    (
        "stdint",
        "uint32_width",
        """
    uint32_t x = 0x12345678u;
    RXDK_TEST_EQ((unsigned)(x >> 16), 0x1234u);
    return 0;
""",
    ),
    (
        "stdio",
        "sprintf_int",
        """
    char buf[16];
    RXDK_TEST_EQ(sprintf(buf, "%d", 1234), 4);
    RXDK_TEST_STR_EQ(buf, "1234");
    return 0;
""",
    ),
    (
        "c23",
        "stdbit",
        """
#if __STDC_VERSION__ >= 202311L
    /* leading / trailing zeros (32-bit) */
    RXDK_TEST_EQ(stdc_leading_zeros(0x80000000u), 0u);
    RXDK_TEST_EQ(stdc_leading_zeros(1u), 31u);
    RXDK_TEST_EQ(stdc_leading_zeros(0u), 32u);
    RXDK_TEST_EQ(stdc_trailing_zeros(0x80000000u), 31u);
    RXDK_TEST_EQ(stdc_trailing_zeros(1u), 0u);
    RXDK_TEST_EQ(stdc_trailing_zeros(0u), 32u);
    /* narrow types via _Generic dispatch */
    RXDK_TEST_EQ(stdc_leading_zeros((unsigned char)0x80), 0u);
    RXDK_TEST_EQ(stdc_leading_zeros((unsigned char)1), 7u);
    RXDK_TEST_EQ(stdc_leading_zeros((unsigned short)1), 15u);
    RXDK_TEST_EQ(stdc_count_ones((unsigned char)0xFF), 8u);
    RXDK_TEST_EQ(stdc_count_zeros((unsigned char)0xFF), 0u);
    /* ones */
    RXDK_TEST_EQ(stdc_leading_ones(0xFFFFFFFFu), 32u);
    RXDK_TEST_EQ(stdc_leading_ones(0xF0000000u), 4u);
    RXDK_TEST_EQ(stdc_trailing_ones(0xFFu), 8u);
    RXDK_TEST_EQ(stdc_trailing_ones(7u), 3u);
    /* counts */
    RXDK_TEST_EQ(stdc_count_ones(0xFFu), 8u);
    RXDK_TEST_EQ(stdc_count_zeros(0xFFu), 24u);
    /* first leading / trailing */
    RXDK_TEST_EQ(stdc_first_leading_one(0x80000000u), 1u);
    RXDK_TEST_EQ(stdc_first_leading_one(0u), 0u);
    RXDK_TEST_EQ(stdc_first_leading_zero(0xFFFFFFFFu), 0u);
    RXDK_TEST_EQ(stdc_first_trailing_one(8u), 4u);
    RXDK_TEST_EQ(stdc_first_trailing_one(0u), 0u);
    RXDK_TEST_EQ(stdc_first_trailing_zero(7u), 4u);
    /* single bit / width / floor / ceil */
    RXDK_TEST_TRUE(stdc_has_single_bit(8u));
    RXDK_TEST_TRUE(!stdc_has_single_bit(6u));
    RXDK_TEST_TRUE(!stdc_has_single_bit(0u));
    RXDK_TEST_EQ(stdc_bit_width(0u), 0u);
    RXDK_TEST_EQ(stdc_bit_width(1u), 1u);
    RXDK_TEST_EQ(stdc_bit_width(255u), 8u);
    RXDK_TEST_EQ(stdc_bit_floor(5u), 4u);
    RXDK_TEST_EQ(stdc_bit_floor(8u), 8u);
    RXDK_TEST_EQ(stdc_bit_floor(0u), 0u);
    RXDK_TEST_EQ(stdc_bit_ceil(5u), 8u);
    RXDK_TEST_EQ(stdc_bit_ceil(8u), 8u);
    RXDK_TEST_EQ(stdc_bit_ceil(0u), 1u);
    RXDK_TEST_EQ(stdc_bit_ceil(1u), 1u);
    /* 64-bit (unsigned long long) */
    RXDK_TEST_EQ(stdc_leading_zeros(1ull), 63u);
    RXDK_TEST_EQ(stdc_bit_width(0xFFFFFFFFFFFFFFFFull), 64u);
    RXDK_TEST_EQ(stdc_count_ones(0xFFFFFFFFFFFFFFFFull), 64u);
#endif
    return 0;
""",
    ),
    (
        "filesystem",
        "roundtrip",
        r"""
    /* Exercises the kernel-backed file I/O on the mounted E: drive: create a
       folder, confirm it exists, write a file, stat it, seek + read back, then
       clean up after itself. The harness mounts E: before tests run. */
    const char *dir = "E:\\test";
    const char *file = "E:\\test\\probe.bin";
    const char *msg = "RXDK-FS-CHECK!"; /* 14 bytes */
    struct stat sbuf;
    char rbuf[8];
    FILE *fp;

    /* start from a clean slate (ignore errors if absent) */
    remove(file);
    rmdir(dir);

    /* create the test folder and confirm it is a directory */
    if (mkdir(dir, 0777) != 0) RXDK_TEST_FAIL();
    if (stat(dir, &sbuf) != 0) RXDK_TEST_FAIL();
    RXDK_TEST_TRUE(S_ISDIR(sbuf.st_mode));

    /* write a known payload */
    fp = fopen(file, "wb");
    RXDK_TEST_TRUE(fp != NULL);
    RXDK_TEST_EQ(fwrite(msg, 1, 14, fp), 14u);
    fclose(fp);

    /* size reported via stat */
    if (stat(file, &sbuf) != 0) RXDK_TEST_FAIL();
    RXDK_TEST_EQ((long)sbuf.st_size, 14L);

    /* reopen, seek to offset 5, read 4 bytes ("FS-C") */
    fp = fopen(file, "rb");
    RXDK_TEST_TRUE(fp != NULL);
    if (fseek(fp, 5, SEEK_SET) != 0) RXDK_TEST_FAIL();
    RXDK_TEST_EQ(fread(rbuf, 1, 4, fp), 4u);
    rbuf[4] = '\0';
    RXDK_TEST_STR_EQ(rbuf, "FS-C");
    fclose(fp);

    /* clean up after ourselves */
    RXDK_TEST_EQ(remove(file), 0);
    RXDK_TEST_EQ(rmdir(dir), 0);
    return 0;
""",
    ),
    (
        "time",
        "monotonic",
        """
    struct timespec a, b;
    volatile unsigned long spin = 0;

    RXDK_TEST_EQ(clock_gettime(CLOCK_MONOTONIC, &a), 0);
    RXDK_TEST_TRUE(a.tv_nsec >= 0 && a.tv_nsec < 1000000000L);

    for (unsigned long i = 0; i < 20000000UL; ++i)
        spin += i;
    (void)spin;

    RXDK_TEST_EQ(clock_gettime(CLOCK_MONOTONIC, &b), 0);
    {
        long long da = (long long)a.tv_sec * 1000000000LL + a.tv_nsec;
        long long db = (long long)b.tv_sec * 1000000000LL + b.tv_nsec;
        RXDK_TEST_TRUE(db > da); /* monotonic clock must advance */
    }
    return 0;
""",
    ),
    (
        "time",
        "wallclock",
        """
    struct timeval tv;
    RXDK_TEST_EQ(gettimeofday(&tv, NULL), 0);
    RXDK_TEST_TRUE(tv.tv_usec >= 0 && tv.tv_usec < 1000000L);
    return 0;
""",
    ),
    (
        "time",
        "clock",
        """
    volatile unsigned long spin = 0;
    clock_t c0 = clock();
    clock_t c1;

    RXDK_TEST_NE(c0, (clock_t)-1);
    for (unsigned long i = 0; i < 20000000UL; ++i)
        spin += i;
    (void)spin;
    c1 = clock();
    RXDK_TEST_TRUE(c1 >= c0);
    return 0;
""",
    ),
    (
        "threads",
        "mutex_counter",
        """
    /* Four threads each bump a shared counter 10000x under a mutex; with mutual
       exclusion the total is exact. Exercises thrd_create/join + mtx_*. */
    thrd_t th[4];
    int i, r;

    g_counter = 0;
    RXDK_TEST_EQ(mtx_init(&g_mtx, mtx_plain), thrd_success);

    for (i = 0; i < 4; ++i)
        RXDK_TEST_EQ(thrd_create(&th[i], worker_inc, NULL), thrd_success);

    for (i = 0; i < 4; ++i) {
        RXDK_TEST_EQ(thrd_join(th[i], &r), thrd_success);
        RXDK_TEST_EQ(r, 0);
    }

    mtx_destroy(&g_mtx);
    RXDK_TEST_EQ(g_counter, 4 * 10000);
    return 0;
""",
        """
static mtx_t g_mtx;
static volatile int g_counter;

static int worker_inc(void *arg)
{
    (void)arg;
    for (int i = 0; i < 10000; ++i) {
        mtx_lock(&g_mtx);
        g_counter++;
        mtx_unlock(&g_mtx);
    }
    return 0;
}
""",
    ),
    (
        "math",
        "classify",
        """
    RXDK_TEST_TRUE(isnan(NAN));
    RXDK_TEST_TRUE(isinf(INFINITY));
    RXDK_TEST_TRUE(!isfinite(INFINITY));
    RXDK_TEST_TRUE(isfinite(1.0));
    RXDK_TEST_TRUE(signbit(-1.0));
    RXDK_TEST_TRUE(!signbit(1.0));
    RXDK_TEST_TRUE(fabs(-2.5) == 2.5);
    RXDK_TEST_TRUE(scalbn(1.0, 3) == 8.0);
    return 0;
""",
    ),
    (
        "float",
        "limits",
        """
    RXDK_TEST_EQ(FLT_RADIX, 2);
    RXDK_TEST_EQ(DBL_MANT_DIG, 53);
    RXDK_TEST_TRUE(FLT_MAX > 0.0f);
    RXDK_TEST_TRUE(DBL_EPSILON > 0.0);
    return 0;
""",
    ),
    (
        "setjmp",
        "roundtrip",
        """
    jmp_buf jb;
    volatile int reached = 0;
    int v = setjmp(jb);
    if (v == 0) {
        reached = 1;
        longjmp(jb, 42);
    }
    RXDK_TEST_EQ(v, 42);
    RXDK_TEST_EQ(reached, 1);
    return 0;
""",
    ),
    (
        "inttypes",
        "format",
        """
    char buf[32];
    int32_t v = -123456;
    RXDK_TEST_TRUE(sprintf(buf, "%" PRId32, v) > 0);
    RXDK_TEST_STR_EQ(buf, "-123456");
    RXDK_TEST_EQ((long)imaxabs((intmax_t)-5), 5L);
    return 0;
""",
    ),
    (
        "stdarg",
        "varargs",
        """
    RXDK_TEST_EQ(sum_ints(4, 1, 2, 3, 4), 10);
    RXDK_TEST_EQ(sum_ints(0), 0);
    return 0;
""",
        """
static int sum_ints(int n, ...)
{
    va_list ap;
    int s = 0;
    va_start(ap, n);
    for (int i = 0; i < n; ++i)
        s += va_arg(ap, int);
    va_end(ap);
    return s;
}
""",
    ),
    (
        "wchar",
        "basic",
        """
    wchar_t dst[4];
    RXDK_TEST_EQ(wcslen(L"abc"), 3u);
    RXDK_TEST_EQ(wcscmp(L"abc", L"abc"), 0);
    RXDK_TEST_TRUE(wcscmp(L"abc", L"abd") < 0);
    wmemcpy(dst, L"xy", 3);
    RXDK_TEST_EQ(wcscmp(dst, L"xy"), 0);
    return 0;
""",
    ),
    (
        "iso646",
        "macros",
        """
    RXDK_TEST_TRUE(1 and 1);
    RXDK_TEST_TRUE(1 or 0);
    RXDK_TEST_TRUE(not 0);
    RXDK_TEST_EQ(5 bitand 3, 1);
    RXDK_TEST_EQ(4 bitor 1, 5);
    return 0;
""",
    ),
    (
        "threads",
        "malloc_safety",
        """
    /* Hammer the now thread-safe picolibc allocator from four threads at once;
       each verifies its own buffer survived. Exercises the retargeted locks. */
    thrd_t th[4];
    int i, r;

    for (i = 0; i < 4; ++i)
        RXDK_TEST_EQ(thrd_create(&th[i], worker_malloc, NULL), thrd_success);
    for (i = 0; i < 4; ++i) {
        RXDK_TEST_EQ(thrd_join(th[i], &r), thrd_success);
        RXDK_TEST_EQ(r, 0);
    }
    return 0;
""",
        """
static int worker_malloc(void *arg)
{
    (void)arg;
    for (int i = 0; i < 2000; ++i) {
        unsigned char *p = (unsigned char *)malloc(64);
        if (!p)
            return 1;
        for (int j = 0; j < 64; ++j)
            p[j] = (unsigned char)(j ^ i);
        for (int j = 0; j < 64; ++j)
            if (p[j] != (unsigned char)(j ^ i)) {
                free(p);
                return 2;
            }
        free(p);
    }
    return 0;
}
""",
    ),
    (
        "threads",
        "tss",
        """
    /* Each thread stores a distinct value under one key and reads its own back;
       the main thread's slot must be unaffected. */
    thrd_t th[2];
    int i, r;

    RXDK_TEST_EQ(tss_create(&g_key, NULL), thrd_success);
    RXDK_TEST_EQ(tss_set(g_key, (void *)(intptr_t)100), thrd_success);

    for (i = 0; i < 2; ++i)
        RXDK_TEST_EQ(thrd_create(&th[i], tss_worker, (void *)(intptr_t)(i + 1)),
                     thrd_success);
    for (i = 0; i < 2; ++i) {
        RXDK_TEST_EQ(thrd_join(th[i], &r), thrd_success);
        RXDK_TEST_EQ(r, 0);
    }

    RXDK_TEST_EQ((int)(intptr_t)tss_get(g_key), 100);
    tss_delete(g_key);
    return 0;
""",
        """
static tss_t g_key;

static int tss_worker(void *arg)
{
    int want = (int)(intptr_t)arg;
    if (tss_set(g_key, arg) != thrd_success)
        return 1;
    thrd_yield(); /* interleave with the other worker */
    if ((int)(intptr_t)tss_get(g_key) != want)
        return 2;
    return 0;
}
""",
    ),
    (
        "threads",
        "errno_isolation",
        """
    /* errno is per-thread: each worker's value must survive the others. */
    thrd_t th[3];
    int i, r;

    errno = 4242;
    for (i = 0; i < 3; ++i)
        RXDK_TEST_EQ(thrd_create(&th[i], errno_worker, (void *)(intptr_t)(100 + i)),
                     thrd_success);
    for (i = 0; i < 3; ++i) {
        RXDK_TEST_EQ(thrd_join(th[i], &r), thrd_success);
        RXDK_TEST_EQ(r, 0);
    }
    RXDK_TEST_EQ(errno, 4242); /* main's errno untouched by workers */
    return 0;
""",
        """
static int errno_worker(void *arg)
{
    int want = (int)(intptr_t)arg;
    errno = want;
    thrd_yield();
    thrd_yield();
    if (errno != want) /* another thread's errno must not clobber ours */
        return 1;
    return 0;
}
""",
    ),
    (
        "threads",
        "identity",
        """
    /* Every thread has a distinct id, thrd_current() inside a thread matches
       the handle thrd_create() handed back, and thrd_equal is reflexive. */
    thrd_t th[4];
    int i, j, r;

    for (i = 0; i < 4; ++i)
        g_ids[i] = NULL;
    for (i = 0; i < 4; ++i)
        RXDK_TEST_EQ(thrd_create(&th[i], id_worker, (void *)(intptr_t)i),
                     thrd_success);
    for (i = 0; i < 4; ++i) {
        RXDK_TEST_EQ(thrd_join(th[i], &r), thrd_success);
        RXDK_TEST_EQ(r, 0);
    }

    for (i = 0; i < 4; ++i) {
        RXDK_TEST_TRUE(g_ids[i] != NULL);
        RXDK_TEST_TRUE(thrd_equal(g_ids[i], th[i])); /* self == its handle */
        for (j = i + 1; j < 4; ++j)
            RXDK_TEST_TRUE(!thrd_equal(g_ids[i], g_ids[j])); /* all distinct */
    }
    return 0;
""",
        """
static thrd_t g_ids[4];

static int id_worker(void *arg)
{
    int idx = (int)(intptr_t)arg;
    thrd_t self = thrd_current();
    if (self == NULL)
        return 1;
    if (!thrd_equal(self, thrd_current())) /* reflexive */
        return 2;
    g_ids[idx] = self;
    return 0;
}
""",
    ),
    (
        "posix",
        "regex",
        """
    /* picolibc's POSIX regex (Henry Spencer engine). */
    regex_t re;
    regmatch_t m[2];

    /* anchored numeric pattern: matches all-digits, rejects otherwise */
    RXDK_TEST_EQ(regcomp(&re, "^[0-9]+$", REG_EXTENDED), 0);
    RXDK_TEST_EQ(regexec(&re, "12345", 0, NULL, 0), 0);
    RXDK_TEST_EQ(regexec(&re, "12a45", 0, NULL, 0), REG_NOMATCH);
    regfree(&re);

    /* capture group offsets */
    RXDK_TEST_EQ(regcomp(&re, "([a-z]+)", REG_EXTENDED), 0);
    RXDK_TEST_EQ(regexec(&re, "  abc  ", 2, m, 0), 0);
    RXDK_TEST_EQ((int)m[1].rm_so, 2);
    RXDK_TEST_EQ((int)m[1].rm_eo, 5);
    regfree(&re);
    return 0;
""",
    ),
    (
        "posix",
        "getentropy",
        """
    unsigned char a[32], b[32];
    int i, diff = 0;
    RXDK_TEST_EQ(getentropy(a, sizeof(a)), 0);
    RXDK_TEST_EQ(getentropy(b, sizeof(b)), 0);
    for (i = 0; i < 32; ++i)
        if (a[i] != b[i])
            diff++;
    RXDK_TEST_TRUE(diff > 0);          /* two reads differ */
    RXDK_TEST_EQ(getentropy(a, 257), -1); /* >256 rejected */
    return 0;
""",
    ),
    (
        "posix",
        "times",
        """
    struct tms tb;
    volatile unsigned long spin = 0;
    clock_t t0 = times(&tb);
    clock_t t1;

    RXDK_TEST_NE(t0, (clock_t)-1);
    RXDK_TEST_EQ(tb.tms_stime, (clock_t)0);
    for (unsigned long i = 0; i < 20000000UL; ++i)
        spin += i;
    (void)spin;
    t1 = times(&tb);
    RXDK_TEST_TRUE(t1 >= t0);              /* monotonic */
    RXDK_TEST_TRUE(tb.tms_utime >= t0);    /* user time advanced */
    return 0;
""",
    ),
    (
        "posix",
        "pipe",
        """
    int fd[2];
    char b[8];
    RXDK_TEST_EQ(pipe(fd), 0);
    RXDK_TEST_EQ((int)write(fd[1], "hello", 5), 5);
    RXDK_TEST_EQ((int)read(fd[0], b, 5), 5);
    b[5] = '\\0';
    RXDK_TEST_STR_EQ(b, "hello");
    close(fd[1]);                          /* writer closed */
    RXDK_TEST_EQ((int)read(fd[0], b, 5), 0); /* -> EOF */
    close(fd[0]);
    return 0;
""",
    ),
    (
        "posix",
        "pipe_blocking",
        """
    /* reader blocks until a second thread writes (exercises pipe wakeups) */
    thrd_t t;
    char b[8];
    ssize_t n;
    RXDK_TEST_EQ(pipe(g_pfd), 0);
    RXDK_TEST_EQ(thrd_create(&t, pipe_writer, NULL), thrd_success);
    n = read(g_pfd[0], b, 5);
    RXDK_TEST_EQ((int)n, 5);
    b[5] = '\\0';
    RXDK_TEST_STR_EQ(b, "async");
    thrd_join(t, NULL);
    close(g_pfd[0]);
    close(g_pfd[1]);
    return 0;
""",
        """
static int g_pfd[2];
static int pipe_writer(void *arg)
{
    struct timespec d = { 0, 50000000 }; /* 50ms */
    (void)arg;
    thrd_sleep(&d, NULL);
    write(g_pfd[1], "async", 5);
    return 0;
}
""",
    ),
    (
        "posix",
        "dup2",
        """
    int fd[2];
    char b[8];
    RXDK_TEST_EQ(pipe(fd), 0);
    RXDK_TEST_EQ(dup2(fd[0], 20), 20);     /* alias read end onto fd 20 */
    RXDK_TEST_EQ((int)write(fd[1], "xy", 2), 2);
    RXDK_TEST_EQ((int)read(20, b, 2), 2);
    b[2] = '\\0';
    RXDK_TEST_STR_EQ(b, "xy");
    close(fd[0]);                          /* original closed; dup survives */
    RXDK_TEST_EQ((int)write(fd[1], "z", 1), 1);
    RXDK_TEST_EQ((int)read(20, b, 1), 1);
    RXDK_TEST_EQ(b[0], 'z');
    close(20);
    close(fd[1]);
    RXDK_TEST_EQ(dup2(1, 2), 2);           /* console alias is a no-op */
    return 0;
""",
    ),
    (
        "stdio",
        "printf",
        """
    /* capture printf's stdout via the output hook and check the formatting */
    g_pf_n = 0;
    g_pf_buf[0] = '\\0';
    rxdk_set_output_handler(pf_capture);
    printf("n=%d s=%s x=%x", 42, "hi", 255);
    fflush(stdout);
    rxdk_set_output_handler(NULL);
    RXDK_TEST_STR_EQ(g_pf_buf, "n=42 s=hi x=ff");
    return 0;
""",
        """
static char g_pf_buf[64];
static size_t g_pf_n;

static ssize_t pf_capture(int fd, const void *buf, size_t count)
{
    const char *p = (const char *)buf;
    size_t i;
    (void)fd;
    for (i = 0; i < count && g_pf_n < sizeof(g_pf_buf) - 1; ++i)
        g_pf_buf[g_pf_n++] = p[i];
    g_pf_buf[g_pf_n] = '\\0';
    return (ssize_t)count;
}
""",
    ),
    (
        "posix",
        "signal",
        """
    sigset_t set, old, pend;

    g_sig_count = 0;
    g_sig_last = 0;

    /* register + synchronous raise */
    RXDK_TEST_TRUE(signal(SIGTERM, sig_handler) != SIG_ERR);
    RXDK_TEST_EQ(raise(SIGTERM), 0);
    RXDK_TEST_EQ(g_sig_count, 1);
    RXDK_TEST_EQ(g_sig_last, SIGTERM);

    /* C semantics: disposition resets to SIG_DFL after delivery */
    RXDK_TEST_EQ(raise(SIGTERM), 0);
    RXDK_TEST_EQ(g_sig_count, 1);

    /* SIG_IGN is not delivered */
    signal(SIGINT, SIG_IGN);
    RXDK_TEST_EQ(raise(SIGINT), 0);
    RXDK_TEST_EQ(g_sig_count, 1);

    /* block -> pending -> unblock delivers */
    signal(SIGTERM, sig_handler);
    set = (sigset_t)1 << SIGTERM;
    RXDK_TEST_EQ(sigprocmask(SIG_BLOCK, &set, &old), 0);
    RXDK_TEST_EQ(raise(SIGTERM), 0);
    RXDK_TEST_EQ(g_sig_count, 1);
    sigpending(&pend);
    RXDK_TEST_TRUE((pend & ((sigset_t)1 << SIGTERM)) != 0);
    RXDK_TEST_EQ(sigprocmask(SIG_UNBLOCK, &set, NULL), 0);
    RXDK_TEST_EQ(g_sig_count, 2);

    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    return 0;
""",
        """
static volatile int g_sig_count;
static volatile int g_sig_last;

static void sig_handler(int s)
{
    g_sig_count++;
    g_sig_last = s;
}
""",
    ),
    (
        "rxdk",
        "io_hooks",
        """
    char b[8];
    ssize_t sn, en;
    int er;

    /* stdin handler: read(0) routes to it; unset -> EOF */
    rxdk_set_stdin_handler(hk_stdin);
    sn = read(0, b, 5);
    rxdk_set_stdin_handler(NULL);
    RXDK_TEST_EQ((int)sn, 5);
    b[5] = '\\0';
    RXDK_TEST_STR_EQ(b, "hello");
    en = read(0, b, 5);
    RXDK_TEST_EQ((int)en, 0);

    /* output handler: write(2) routes with the stderr fd flag */
    g_hk_out_fd = -1;
    g_hk_out_n = 0;
    rxdk_set_output_handler(hk_out);
    write(2, "err", 3);
    rxdk_set_output_handler(NULL);
    RXDK_TEST_EQ(g_hk_out_fd, 2);
    RXDK_TEST_EQ((int)g_hk_out_n, 3);
    RXDK_TEST_EQ(g_hk_out_buf[0], 'e');

    /* exec handler: execve routes to it */
    rxdk_set_exec_handler(hk_exec);
    er = execve("d:\\\\x.xbe", NULL, NULL);
    rxdk_set_exec_handler(NULL);
    RXDK_TEST_EQ(er, 4242);
    return 0;
""",
        """
static ssize_t hk_stdin(void *buf, size_t count)
{
    const char *s = "hello";
    size_t n = count < 5 ? count : 5;
    memcpy(buf, s, n);
    return (ssize_t)n;
}

static int g_hk_out_fd;
static size_t g_hk_out_n;
static char g_hk_out_buf[16];

static ssize_t hk_out(int fd, const void *buf, size_t count)
{
    g_hk_out_fd = fd;
    g_hk_out_n = count < sizeof(g_hk_out_buf) ? count : sizeof(g_hk_out_buf);
    memcpy(g_hk_out_buf, buf, g_hk_out_n);
    return (ssize_t)count;
}

static int hk_exec(const char *p, char *const a[], char *const e[])
{
    (void)p;
    (void)a;
    (void)e;
    return 4242; /* sentinel: handler invoked */
}
""",
    ),
    (
        "rxdk",
        "exec_args",
        """
    /* verify path, argv (count + values) and envp reach the exec handler */
    char *args[] = { "prog", "arg1", NULL };
    char *env[] = { "K=V", NULL };
    int r;

    g_xa_argc = -1;
    g_xa_path[0] = '\\0';
    g_xa_a0[0] = '\\0';
    g_xa_a1[0] = '\\0';
    g_xa_env_ok = 0;

    rxdk_set_exec_handler(hk_exec_args);
    r = execve("game.xbe", args, env);
    rxdk_set_exec_handler(NULL);

    RXDK_TEST_EQ(r, 7);
    RXDK_TEST_STR_EQ(g_xa_path, "game.xbe");
    RXDK_TEST_EQ(g_xa_argc, 2);
    RXDK_TEST_STR_EQ(g_xa_a0, "prog");
    RXDK_TEST_STR_EQ(g_xa_a1, "arg1");
    RXDK_TEST_TRUE(g_xa_env_ok);
    return 0;
""",
        """
static int g_xa_argc;
static int g_xa_env_ok;
static char g_xa_path[16];
static char g_xa_a0[16];
static char g_xa_a1[16];

static int hk_exec_args(const char *path, char *const argv[], char *const envp[])
{
    int i = 0;
    if (path) {
        strncpy(g_xa_path, path, sizeof(g_xa_path) - 1);
        g_xa_path[sizeof(g_xa_path) - 1] = '\\0';
    }
    if (argv) {
        while (argv[i])
            i++;
        if (argv[0]) {
            strncpy(g_xa_a0, argv[0], sizeof(g_xa_a0) - 1);
            g_xa_a0[sizeof(g_xa_a0) - 1] = '\\0';
        }
        if (i > 1 && argv[1]) {
            strncpy(g_xa_a1, argv[1], sizeof(g_xa_a1) - 1);
            g_xa_a1[sizeof(g_xa_a1) - 1] = '\\0';
        }
    }
    g_xa_argc = i;
    if (envp && envp[0] && !envp[1] && strcmp(envp[0], "K=V") == 0)
        g_xa_env_ok = 1;
    return 7;
}
""",
    ),
    (
        "c23",
        "stdckdint",
        """
    int r;
    unsigned char ur;
    RXDK_TEST_TRUE(!ckd_add(&r, 2, 3));
    RXDK_TEST_EQ(r, 5);
    RXDK_TEST_TRUE(!ckd_mul(&r, 6, 7));
    RXDK_TEST_EQ(r, 42);
    RXDK_TEST_TRUE(ckd_add(&ur, (unsigned char)200, (unsigned char)100));
    RXDK_TEST_TRUE(ckd_sub(&ur, (unsigned char)1, (unsigned char)2));
    return 0;
""",
    ),
    (
        "c23",
        "stdbool",
        """
    bool b = true;
    RXDK_TEST_TRUE(b);
    RXDK_TEST_TRUE(!false);
    RXDK_TEST_EQ((int)true, 1);
    RXDK_TEST_EQ((int)false, 0);
    RXDK_TEST_EQ(sizeof(bool), 1u);
    return 0;
""",
    ),
    (
        "c23",
        "stdalign",
        """
    struct A16 { _Alignas(16) char x; };
    RXDK_TEST_EQ(alignof(int), 4u);
    RXDK_TEST_EQ(alignof(struct A16), 16u);
    return 0;
""",
    ),
    (
        "c23",
        "assert",
        """
    assert(1);
    assert(2 + 2 == 4);
    static_assert(sizeof(int) == 4, "int is 4 bytes");
    return 0;
""",
    ),
    (
        "c23",
        "stdatomic",
        """
    atomic_int a;
    int expected = 8;
    atomic_flag f = ATOMIC_FLAG_INIT;
    atomic_init(&a, 0);
    atomic_store(&a, 5);
    RXDK_TEST_EQ(atomic_load(&a), 5);
    RXDK_TEST_EQ(atomic_fetch_add(&a, 3), 5);
    RXDK_TEST_EQ(atomic_load(&a), 8);
    RXDK_TEST_TRUE(atomic_compare_exchange_strong(&a, &expected, 10));
    RXDK_TEST_EQ(atomic_load(&a), 10);
    RXDK_TEST_TRUE(!atomic_flag_test_and_set(&f));
    RXDK_TEST_TRUE(atomic_flag_test_and_set(&f));
    return 0;
""",
    ),
    (
        "c23",
        "uchar",
        """
    char16_t c16 = 0;
    char32_t c32 = 0;
    mbstate_t st;
    memset(&st, 0, sizeof(st));
    RXDK_TEST_EQ(mbrtoc16(&c16, "A", 1, &st), 1u);
    RXDK_TEST_EQ(c16, (char16_t)'A');
    memset(&st, 0, sizeof(st));
    RXDK_TEST_EQ(mbrtoc32(&c32, "Z", 1, &st), 1u);
    RXDK_TEST_EQ(c32, (char32_t)'Z');
    return 0;
""",
    ),
    (
        "c23",
        "wctype",
        """
    RXDK_TEST_TRUE(iswalpha(L'A'));
    RXDK_TEST_TRUE(iswdigit(L'7'));
    RXDK_TEST_TRUE(!iswalpha(L'7'));
    RXDK_TEST_TRUE(iswspace(L' '));
    RXDK_TEST_EQ(towupper(L'a'), (wint_t)L'A');
    RXDK_TEST_EQ(towlower(L'A'), (wint_t)L'a');
    return 0;
""",
    ),
]
