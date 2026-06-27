// libcpp-smoke conformance tests: C++23 / libc++ features that work today on
// the Xbox target (exceptions are off; <charconv>/<chrono>/<thread>/<random>/
// <regex>/<filesystem> are not yet enabled). Each test returns 0 or __LINE__.

#include "tests.hpp"

#include <cstdio>

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <chrono>
#include <condition_variable>
#include <expected>
#include <filesystem>
#include <future>
#include <mutex>
#include <functional>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <ranges>
#include <regex>
#include <set>
#include <shared_mutex>
#include <stdexcept>
#include <span>
#include <string>
#include <thread>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

namespace {

int test_string_basic()
{
    std::string s = "abc";
    RXDK_TEST_EQ(s.size(), 3u);
    s += "def";
    RXDK_TEST_EQ(s.size(), 6u);
    RXDK_TEST_TRUE(s == "abcdef");
    RXDK_TEST_TRUE(s.substr(2, 2) == "cd");
    RXDK_TEST_EQ(s.find("def"), 3u);
    RXDK_TEST_EQ(s.find("xyz"), std::string::npos);
    // force a heap allocation past the small-string buffer
    std::string big(128, 'x');
    RXDK_TEST_EQ(big.size(), 128u);
    RXDK_TEST_EQ(big.front(), 'x');
    RXDK_TEST_EQ(big.back(), 'x');
    return 0;
}

int test_vector_basic()
{
    std::vector<int> v;
    for (int i = 0; i < 100; ++i)
        v.push_back(i);
    RXDK_TEST_EQ(v.size(), 100u);
    RXDK_TEST_EQ(v[42], 42);
    RXDK_TEST_EQ(v.front(), 0);
    RXDK_TEST_EQ(v.back(), 99);
    v.erase(v.begin());
    RXDK_TEST_EQ(v.front(), 1);
    RXDK_TEST_EQ(v.size(), 99u);
    return 0;
}

int test_algorithm_sort_find()
{
    std::vector<int> v = {5, 3, 1, 4, 2};
    std::sort(v.begin(), v.end());
    RXDK_TEST_TRUE(std::is_sorted(v.begin(), v.end()));
    RXDK_TEST_EQ(v.front(), 1);
    auto it = std::find(v.begin(), v.end(), 4);
    RXDK_TEST_TRUE(it != v.end());
    RXDK_TEST_EQ(std::count(v.begin(), v.end(), 3), 1);
    RXDK_TEST_EQ(*std::max_element(v.begin(), v.end()), 5);
    return 0;
}

int test_numeric_accumulate()
{
    std::vector<int> v = {1, 2, 3, 4, 5};
    RXDK_TEST_EQ(std::accumulate(v.begin(), v.end(), 0), 15);
    int prod = std::accumulate(v.begin(), v.end(), 1, std::multiplies<int>());
    RXDK_TEST_EQ(prod, 120);
    return 0;
}

int test_optional_basic()
{
    std::optional<int> a;
    RXDK_TEST_FALSE(a.has_value());
    RXDK_TEST_EQ(a.value_or(7), 7);
    a = 42;
    RXDK_TEST_TRUE(a.has_value());
    RXDK_TEST_EQ(*a, 42);
    return 0;
}

int test_optional_monadic()
{
    // C++23 monadic operations
    std::optional<int> a = 10;
    auto b = a.and_then([](int x) -> std::optional<int> { return x * 2; });
    RXDK_TEST_TRUE(b.has_value());
    RXDK_TEST_EQ(*b, 20);
    auto c = a.transform([](int x) { return x + 1; });
    RXDK_TEST_EQ(*c, 11);
    std::optional<int> e;
    auto d = e.or_else([]() -> std::optional<int> { return -1; });
    RXDK_TEST_EQ(*d, -1);
    return 0;
}

int test_expected_basic()
{
    // C++23 std::expected
    std::expected<int, const char *> ok = 42;
    RXDK_TEST_TRUE(ok.has_value());
    RXDK_TEST_EQ(*ok, 42);
    std::expected<int, const char *> err = std::unexpected("boom");
    RXDK_TEST_FALSE(err.has_value());
    RXDK_TEST_EQ(err.value_or(-1), -1);
    auto mapped = ok.transform([](int x) { return x + 1; });
    RXDK_TEST_EQ(*mapped, 43);
    return 0;
}

int test_variant_basic()
{
    std::variant<int, double, std::string> v = 5;
    RXDK_TEST_EQ(v.index(), 0u);
    RXDK_TEST_TRUE(std::holds_alternative<int>(v));
    RXDK_TEST_EQ(std::get<int>(v), 5);
    v = std::string("hello");
    RXDK_TEST_EQ(v.index(), 2u);
    RXDK_TEST_TRUE(std::get<std::string>(v) == "hello");
    return 0;
}

int test_variant_visit()
{
    std::variant<int, double> v = 3.5;
    double r = std::visit([](auto x) { return static_cast<double>(x) * 2.0; }, v);
    RXDK_TEST_TRUE(r > 6.9 && r < 7.1);
    return 0;
}

int test_tuple_basic()
{
    auto t = std::make_tuple(1, 2.5, 'c');
    RXDK_TEST_EQ(std::get<0>(t), 1);
    RXDK_TEST_EQ(std::get<2>(t), 'c');
    auto [a, b, c] = t;
    RXDK_TEST_EQ(a, 1);
    RXDK_TEST_EQ(c, 'c');
    int sum = std::apply([](int x, double y, char) { return x + static_cast<int>(y); }, t);
    RXDK_TEST_EQ(sum, 3);
    return 0;
}

int test_map_ordered()
{
    std::map<int, std::string> m;
    m[3] = "three";
    m[1] = "one";
    m[2] = "two";
    RXDK_TEST_EQ(m.size(), 3u);
    RXDK_TEST_TRUE(m.find(2) != m.end());
    RXDK_TEST_TRUE(m[1] == "one");
    // ordered iteration
    int prev = 0;
    for (auto &kv : m) {
        RXDK_TEST_TRUE(kv.first > prev);
        prev = kv.first;
    }
    return 0;
}

int test_unordered_map_int()
{
    std::unordered_map<int, int> h;
    for (int i = 0; i < 50; ++i)
        h[i] = i * i;
    RXDK_TEST_EQ(h.size(), 50u);
    RXDK_TEST_EQ(h[7], 49);
    RXDK_TEST_TRUE(h.find(49) != h.end());
    RXDK_TEST_TRUE(h.find(50) == h.end());
    return 0;
}

int test_set_basic()
{
    std::set<int> s = {3, 1, 2, 1, 3};
    RXDK_TEST_EQ(s.size(), 3u);
    RXDK_TEST_EQ(s.count(2), 1u);
    RXDK_TEST_EQ(s.count(9), 0u);
    return 0;
}

int test_smart_ptr_unique()
{
    auto p = std::make_unique<int>(99);
    RXDK_TEST_EQ(*p, 99);
    auto q = std::move(p);
    RXDK_TEST_TRUE(p == nullptr);
    RXDK_TEST_EQ(*q, 99);
    q.reset();
    RXDK_TEST_TRUE(q == nullptr);
    return 0;
}

int test_smart_ptr_shared()
{
    auto p = std::make_shared<int>(7);
    RXDK_TEST_EQ(p.use_count(), 1L);
    {
        auto q = p;
        RXDK_TEST_EQ(p.use_count(), 2L);
        RXDK_TEST_EQ(*q, 7);
    }
    RXDK_TEST_EQ(p.use_count(), 1L);
    std::weak_ptr<int> w = p;
    RXDK_TEST_FALSE(w.expired());
    auto locked = w.lock();
    RXDK_TEST_EQ(*locked, 7);
    return 0;
}

int test_functional_function()
{
    int captured = 10;
    std::function<int(int)> f = [captured](int x) { return x + captured; };
    RXDK_TEST_EQ(f(5), 15);
    f = [](int x) { return x * x; };
    RXDK_TEST_EQ(f(4), 16);
    return 0;
}

int test_bit_ops()
{
    RXDK_TEST_EQ(std::popcount(0xFFu), 8);
    RXDK_TEST_EQ(std::bit_width(0xFFu), 8);
    RXDK_TEST_TRUE(std::has_single_bit(64u));
    RXDK_TEST_FALSE(std::has_single_bit(63u));
    RXDK_TEST_EQ(std::rotl(0x01u, 4), 0x10u);
    RXDK_TEST_EQ(std::countl_zero(0x01u), 31);
    return 0;
}

int test_string_view_basic()
{
    std::string_view sv = "hello world";
    RXDK_TEST_EQ(sv.size(), 11u);
    RXDK_TEST_TRUE(sv.substr(0, 5) == "hello");
    RXDK_TEST_EQ(sv.find("world"), 6u);
    sv.remove_prefix(6);
    RXDK_TEST_TRUE(sv == "world");
    return 0;
}

int test_span_basic()
{
    int arr[] = {1, 2, 3, 4, 5};
    std::span<int> sp(arr);
    RXDK_TEST_EQ(sp.size(), 5u);
    RXDK_TEST_EQ(sp.front(), 1);
    RXDK_TEST_EQ(sp.back(), 5);
    int sum = 0;
    for (int x : sp)
        sum += x;
    RXDK_TEST_EQ(sum, 15);
    auto sub = sp.subspan(1, 3);
    RXDK_TEST_EQ(sub.size(), 3u);
    RXDK_TEST_EQ(sub.front(), 2);
    return 0;
}

int test_ranges_algorithms()
{
    std::vector<int> v = {5, 2, 4, 1, 3};
    std::ranges::sort(v);
    RXDK_TEST_TRUE(std::ranges::is_sorted(v));
    auto it = std::ranges::find(v, 3);
    RXDK_TEST_TRUE(it != v.end());
    RXDK_TEST_EQ(std::ranges::count(v, 4), 1);
    return 0;
}

int test_ranges_views()
{
    std::vector<int> v = {1, 2, 3, 4, 5, 6};
    auto even_squared = v | std::views::filter([](int x) { return x % 2 == 0; }) |
                        std::views::transform([](int x) { return x * x; });
    std::vector<int> out;
    for (int x : even_squared)
        out.push_back(x);
    RXDK_TEST_EQ(out.size(), 3u);
    RXDK_TEST_EQ(out[0], 4);
    RXDK_TEST_EQ(out[1], 16);
    RXDK_TEST_EQ(out[2], 36);
    return 0;
}

int test_array_basic()
{
    std::array<int, 4> a = {1, 2, 3, 4};
    RXDK_TEST_EQ(a.size(), 4u);
    RXDK_TEST_EQ(a.at(2), 3);
    a.fill(9);
    RXDK_TEST_EQ(a[0], 9);
    RXDK_TEST_EQ(a.back(), 9);
    return 0;
}

int test_chrono_duration()
{
    using namespace std::chrono;
    seconds s(1);
    milliseconds ms = s; // implicit, lossless
    RXDK_TEST_EQ(ms.count(), 1000);
    auto d = hours(2) + minutes(30);
    RXDK_TEST_EQ(duration_cast<minutes>(d).count(), 150);
    RXDK_TEST_EQ(duration_cast<seconds>(d).count(), 9000);
    return 0;
}

int test_chrono_clock()
{
    using namespace std::chrono;
    // system_clock is wired to real time (libc clock_gettime/gettimeofday):
    // sanity-check the epoch is past 2020-01-01 (1577836800s).
    auto sys = system_clock::now();
    auto secs = duration_cast<seconds>(sys.time_since_epoch()).count();
    RXDK_TEST_TRUE(secs > 1577836800LL);
    // steady_clock is monotonic and non-decreasing.
    RXDK_TEST_TRUE(steady_clock::is_steady);
    auto a = steady_clock::now();
    auto b = steady_clock::now();
    RXDK_TEST_TRUE(b >= a);
    return 0;
}

int test_random_engine()
{
    // mt19937 is header-only and deterministic for a fixed seed.
    std::mt19937 gen(12345u);
    std::uniform_int_distribution<int> dist(1, 6);
    bool seen_low = false, seen_high = false;
    for (int i = 0; i < 1000; ++i) {
        int v = dist(gen);
        RXDK_TEST_TRUE(v >= 1 && v <= 6);
        if (v == 1)
            seen_low = true;
        if (v == 6)
            seen_high = true;
    }
    RXDK_TEST_TRUE(seen_low && seen_high);
    // same seed -> same sequence
    std::mt19937 a(42u), b(42u);
    RXDK_TEST_EQ(a(), b());
    return 0;
}

int test_random_device()
{
    // random_device routes to libc getentropy(); draws should not be constant.
    std::random_device rd;
    unsigned first = rd();
    bool varied = false;
    for (int i = 0; i < 16; ++i) {
        if (rd() != first) {
            varied = true;
            break;
        }
    }
    RXDK_TEST_TRUE(varied);
    // seed an engine from it and draw within range
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 99);
    int v = dist(gen);
    RXDK_TEST_TRUE(v >= 0 && v <= 99);
    return 0;
}

int test_thread_spawn()
{
    // std::thread maps onto C11 threads (libc threads.c -> PsCreateSystemThreadEx).
    int result = 0;
    std::thread t([&result] { result = 42; });
    RXDK_TEST_TRUE(t.joinable());
    t.join();
    RXDK_TEST_EQ(result, 42);
    return 0;
}

int test_mutex_counter()
{
    std::mutex m;
    int counter = 0;
    auto worker = [&] {
        for (int i = 0; i < 1000; ++i) {
            std::lock_guard<std::mutex> lk(m);
            ++counter;
        }
    };
    std::thread t1(worker), t2(worker), t3(worker), t4(worker);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    RXDK_TEST_EQ(counter, 4000);
    return 0;
}

int test_condition_variable()
{
    std::mutex m;
    std::condition_variable cv;
    bool ready = false;
    int data = 0;
    std::thread producer([&] {
        std::unique_lock<std::mutex> lk(m);
        data = 99;
        ready = true;
        cv.notify_one();
    });
    {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [&] { return ready; });
        RXDK_TEST_EQ(data, 99);
    }
    producer.join();
    return 0;
}

int test_shared_mutex()
{
    std::shared_mutex sm;
    int value = 0;
    {
        std::unique_lock lk(sm); // exclusive
        value = 5;
    }
    {
        std::shared_lock lk(sm); // shared
        RXDK_TEST_EQ(value, 5);
    }
    return 0;
}

int test_future_async()
{
    std::future<int> f = std::async(std::launch::async, [] { return 7 * 6; });
    RXDK_TEST_TRUE(f.valid());
    RXDK_TEST_EQ(f.get(), 42);
    return 0;
}

int test_thread_sleep()
{
    auto a = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto b = std::chrono::steady_clock::now();
    RXDK_TEST_TRUE(b >= a);
    std::this_thread::yield();
    return 0;
}

int test_charconv_integer()
{
    // Integer to_chars/from_chars are header-only (no charconv.cpp / ryu).
    char buf[32];
    auto r = std::to_chars(buf, buf + sizeof(buf), 123456);
    RXDK_TEST_TRUE(r.ec == std::errc{});
    *r.ptr = '\0';
    RXDK_TEST_TRUE(std::string_view(buf) == "123456");

    int value = 0;
    auto f = std::from_chars(buf, r.ptr, value);
    RXDK_TEST_TRUE(f.ec == std::errc{});
    RXDK_TEST_EQ(value, 123456);

    // hex base
    auto h = std::to_chars(buf, buf + sizeof(buf), 255, 16);
    *h.ptr = '\0';
    RXDK_TEST_TRUE(std::string_view(buf) == "ff");

    unsigned parsed = 0;
    std::from_chars(buf, h.ptr, parsed, 16);
    RXDK_TEST_EQ(parsed, 255u);

    // invalid input reports an error, not a crash
    const char *bad = "xyz";
    int unused = -1;
    auto e = std::from_chars(bad, bad + 3, unused);
    RXDK_TEST_TRUE(e.ec == std::errc::invalid_argument);
    return 0;
}

int test_regex_match()
{
    // exceptions are off, so use only valid patterns (a bad pattern aborts).
    std::regex re("[a-z]+([0-9]+)");
    std::cmatch m;
    RXDK_TEST_TRUE(std::regex_match("abc123", m, re));
    RXDK_TEST_EQ(m.size(), 2u);
    RXDK_TEST_TRUE(m[1].str() == "123");
    RXDK_TEST_FALSE(std::regex_match("ABC", re));

    // search + replace
    std::regex ws("\\s+");
    std::string out = std::regex_replace(std::string("a  b   c"), ws, "_");
    RXDK_TEST_TRUE(out == "a_b_c");
    return 0;
}

int test_filesystem_ops()
{
    // exceptions are off, so use the error_code overloads throughout.
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path dir = "E:\\fstest";
    fs::path file = "E:\\fstest\\hello.txt";
    fs::path file2 = "E:\\fstest\\renamed.txt";

    // Reliable, re-runnable clean slate: individual remove() uses unlink/rmdir
    // directly (not the openat-relative path), so it works regardless of a
    // prior run's state. (remove_all itself is exercised at the end.)
    fs::remove(file, ec);
    fs::remove(file2, ec);
    fs::remove(dir, ec);

    // create_directory -> libc mkdir
    RXDK_TEST_TRUE(fs::create_directory(dir, ec));
    RXDK_TEST_FALSE((bool)ec);
    RXDK_TEST_TRUE(fs::is_directory(fs::status(dir, ec)));

    // write a payload via C stdio (fstream is not built)
    FILE *fp = std::fopen("E:\\fstest\\hello.txt", "wb");
    RXDK_TEST_TRUE(fp != nullptr);
    RXDK_TEST_EQ(std::fwrite("hello!", 1, 6, fp), 6u);
    std::fclose(fp);

    // exists + file_size -> libc stat
    RXDK_TEST_TRUE(fs::exists(file, ec));
    RXDK_TEST_EQ((long)fs::file_size(file, ec), 6L);

    // directory_iterator -> libc opendir/readdir/closedir
    bool found = false;
    for (auto const &e : fs::directory_iterator(dir, ec)) {
        if (e.path().filename() == "hello.txt") {
            found = true;
            RXDK_TEST_EQ((long)e.file_size(ec), 6L);
        }
    }
    RXDK_TEST_FALSE((bool)ec);
    RXDK_TEST_TRUE(found);

    // rename within the filesystem -> libc rename
    fs::rename(file, file2, ec);
    RXDK_TEST_FALSE((bool)ec);
    RXDK_TEST_TRUE(fs::exists(file2, ec));
    RXDK_TEST_FALSE(fs::exists(file, ec));

    // remove_all -> exercises openat/fdopendir/readdir/unlinkat
    std::uintmax_t n = fs::remove_all(dir, ec);
    RXDK_TEST_FALSE((bool)ec);
    RXDK_TEST_TRUE(n >= 2); /* directory + the renamed file */
    RXDK_TEST_FALSE(fs::exists(dir, ec));
    return 0;
}

namespace {
int g_eh_dtor_ran;
struct UnwindGuard {
    ~UnwindGuard() { g_eh_dtor_ran = 1; }
};
[[noreturn]] void throw_int_via_guard()
{
    UnwindGuard g; // must be destroyed as the exception unwinds the stack
    throw 42;
}
} // namespace

int test_exceptions_basic()
{
    // DWARF/Itanium exceptions (libunwind + cxa_personality). Throw across a
    // frame with a local object and confirm its destructor runs during unwind.
    g_eh_dtor_ran = 0;
    bool caught = false;
    try {
        throw_int_via_guard();
    } catch (int e) {
        caught = true;
        RXDK_TEST_EQ(e, 42);
    }
    RXDK_TEST_TRUE(caught);
    RXDK_TEST_EQ(g_eh_dtor_ran, 1);

    // std::exception hierarchy + what()
    try {
        throw std::runtime_error("boom");
    } catch (const std::exception &ex) {
        RXDK_TEST_TRUE(std::string(ex.what()) == "boom");
    }

    // catch-by-base (slicing-safe reference) and rethrow
    int selected = 0;
    try {
        try {
            throw 7;
        } catch (...) {
            throw; // rethrow the in-flight exception
        }
    } catch (int) {
        selected = 1;
    }
    RXDK_TEST_EQ(selected, 1);
    return 0;
}

const cpp_test g_tests[] = {
    {"string", "basic", test_string_basic},
    {"vector", "basic", test_vector_basic},
    {"algorithm", "sort_find", test_algorithm_sort_find},
    {"numeric", "accumulate", test_numeric_accumulate},
    {"optional", "basic", test_optional_basic},
    {"optional", "monadic", test_optional_monadic},
    {"expected", "basic", test_expected_basic},
    {"variant", "basic", test_variant_basic},
    {"variant", "visit", test_variant_visit},
    {"tuple", "basic", test_tuple_basic},
    {"map", "ordered", test_map_ordered},
    {"unordered_map", "int", test_unordered_map_int},
    {"set", "basic", test_set_basic},
    {"smart_ptr", "unique", test_smart_ptr_unique},
    {"smart_ptr", "shared", test_smart_ptr_shared},
    {"functional", "function", test_functional_function},
    {"bit", "ops", test_bit_ops},
    {"string_view", "basic", test_string_view_basic},
    {"span", "basic", test_span_basic},
    {"ranges", "algorithms", test_ranges_algorithms},
    {"ranges", "views", test_ranges_views},
    {"array", "basic", test_array_basic},
    {"chrono", "duration", test_chrono_duration},
    {"chrono", "clock", test_chrono_clock},
    {"random", "engine", test_random_engine},
    {"random", "device", test_random_device},
    {"thread", "spawn", test_thread_spawn},
    {"mutex", "counter", test_mutex_counter},
    {"condition_variable", "pingpong", test_condition_variable},
    {"shared_mutex", "rw", test_shared_mutex},
    {"future", "async", test_future_async},
    {"thread", "sleep", test_thread_sleep},
    {"charconv", "integer", test_charconv_integer},
    {"regex", "match", test_regex_match},
    {"filesystem", "ops", test_filesystem_ops},
    {"exceptions", "basic", test_exceptions_basic},
};

} // namespace

unsigned cpp_test_count()
{
    return sizeof(g_tests) / sizeof(g_tests[0]);
}

const cpp_test *cpp_tests()
{
    return g_tests;
}
