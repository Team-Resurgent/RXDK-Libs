#pragma once

#include "tests.h"

typedef struct XapiSmokeTest {
    const char* name;
    int (*run)(void);
} XapiSmokeTest;

static const XapiSmokeTest kXapiSmokeTests[] = {
    { "error",       test_error },
    { "memory",      test_memory },
    { "files",       test_files },
    { "copyfile",    test_copyfile },
    { "dirs",        test_dirs },
    { "find",        test_find },
    { "path",        test_path },
    { "sync",        test_sync },
    { "sync2",       test_sync2 },
    { "handle",      test_handle },
    { "threads",     test_threads },
    { "tls",         test_tls },
    { "fiber",       test_fiber },
    { "virtual",     test_virtual },
    { "time",        test_time },
    { "widechar",    test_widechar },
    { "interlocked", test_interlocked },
    { "strings",     test_strings },
    { "xbox",        test_xbox },
    { "section",     test_section },
    { "devices",     test_devices },
    { "xinput",      test_xinput },
    { "mu",          test_mu },
    { "savegame",    test_savegame },
    { "content",     test_content },
    { "nickname",    test_nickname },
};

#define kXapiSmokeTestCount (sizeof(kXapiSmokeTests) / sizeof(kXapiSmokeTests[0]))
