// libcpp-smoke conformance tests: C++23 / libc++ features that work today on the
// Xbox target. DWARF/Itanium exceptions, <charconv>, <chrono>, <thread>,
// <random>, <regex>, <filesystem>, <format>, and <print> are all enabled. Each
// test returns 0 (pass) or __LINE__ (fail).

#include "tests.hpp"

#include <cstdio>

#include <algorithm>
#include <array>
#include <atomic>
#include <barrier>
#include <bit>
#include <charconv>
#include <chrono>
#include <condition_variable>
#include <coroutine>
#include <latch>
#include <semaphore>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory_resource>
#include <mutex>
#include <print>
#include <sstream>
#include <valarray>
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

#include <threads.h> // C11 tss (thread-specific storage) for the tss-dtor test

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

int test_charconv_floating()
{
    // Floating-point to_chars (libs/libcpp/charconv_fp_to_chars.cpp + ryu).
    // (from_chars for float is intentionally not provided; see that TU.)
    char buf[64];
    auto r = std::to_chars(buf, buf + sizeof(buf), 3.5);
    RXDK_TEST_TRUE(r.ec == std::errc{});
    *r.ptr = '\0';
    RXDK_TEST_TRUE(std::string_view(buf) == "3.5");

    auto p = std::to_chars(buf, buf + sizeof(buf), 0.5, std::chars_format::fixed, 3);
    RXDK_TEST_TRUE(p.ec == std::errc{});
    *p.ptr = '\0';
    RXDK_TEST_TRUE(std::string_view(buf) == "0.500");
    return 0;
}

int test_format_basic()
{
    // std::format: positional/auto args, fill/align/width, integer bases, float.
    RXDK_TEST_TRUE(std::format("{}", 42) == "42");
    RXDK_TEST_TRUE(std::format("{} {}", "ab", "cd") == "ab cd");
    RXDK_TEST_TRUE(std::format("{1}{0}", "a", "b") == "ba");
    RXDK_TEST_TRUE(std::format("{:05}", 42) == "00042");
    RXDK_TEST_TRUE(std::format("{:>4}", "x") == "   x");
    RXDK_TEST_TRUE(std::format("{:*^5}", "ab") == "*ab**");
    RXDK_TEST_TRUE(std::format("{:#x}", 255) == "0xff");
    RXDK_TEST_TRUE(std::format("{:b}", 5) == "101");
    // floating point routes through std::to_chars (the FP to_chars TU + ryu)
    RXDK_TEST_TRUE(std::format("{}", 3.5) == "3.5");
    RXDK_TEST_TRUE(std::format("{:.2f}", 3.14159) == "3.14");
    return 0;
}

int test_print_basic()
{
    // std::print / std::println write to stdout (-> write() -> DbgPrint on the
    // kit), so these lines are visible on the debug monitor.
    std::print("RXDK-LibsZig: std::print {} {}\n", "ok", 7);
    std::println("RXDK-LibsZig: std::println {}", 42);

    // Verify the formatting engine deterministically via format_to into a buffer.
    std::string s;
    std::format_to(std::back_inserter(s), "{}-{:02d}", "v", 3);
    RXDK_TEST_TRUE(s == "v-03");
    return 0;
}

// ---- coroutines (C++20, header-only <coroutine>; no <generator> in this libc++) ----

// Lazy synchronous generator: initial_suspend suspends, each next() resumes to
// the next co_yield. Exercises co_yield, coroutine_handle, suspend points, and
// the compiler-allocated coroutine frame (operator new/delete).
struct co_gen {
    struct promise_type {
        int current;
        co_gen get_return_object() {
            return co_gen{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(int v) noexcept {
            current = v;
            return {};
        }
        void return_void() noexcept {}
        void unhandled_exception() {}
    };
    std::coroutine_handle<promise_type> h;
    explicit co_gen(std::coroutine_handle<promise_type> hh) : h(hh) {}
    co_gen(co_gen &&o) noexcept : h(o.h) { o.h = {}; }
    ~co_gen() { if (h) h.destroy(); }
    bool next() {
        h.resume();
        return !h.done();
    }
    int current() const { return h.promise().current; }
};

co_gen co_range(int n)
{
    for (int i = 0; i < n; ++i)
        co_yield i;
}

int test_coroutine_generator()
{
    co_gen g = co_range(4);
    int count = 0;
    int sum = 0;
    while (g.next()) {
        sum += g.current();
        ++count;
    }
    RXDK_TEST_EQ(count, 4);
    RXDK_TEST_EQ(sum, 0 + 1 + 2 + 3); // 6
    return 0;
}

// Eager task: initial_suspend never suspends, so the body runs to co_return at
// call time. Exercises co_await (suspend_never) and co_return <value>.
struct co_task {
    struct promise_type {
        int result;
        co_task get_return_object() {
            return co_task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_value(int v) noexcept { result = v; }
        void unhandled_exception() {}
    };
    std::coroutine_handle<promise_type> h;
    explicit co_task(std::coroutine_handle<promise_type> hh) : h(hh) {}
    co_task(co_task &&o) noexcept : h(o.h) { o.h = {}; }
    ~co_task() { if (h) h.destroy(); }
    int get() const { return h.promise().result; }
};

co_task co_add(int a, int b)
{
    co_await std::suspend_never{};
    co_return a + b;
}

int test_coroutine_task()
{
    co_task t = co_add(20, 22);
    RXDK_TEST_EQ(t.get(), 42);
    return 0;
}

int test_iostream_sstream()
{
    // ostringstream: integer, hex/dec manipulators, and floating point (num_put).
    std::ostringstream os;
    os << "x=" << 42 << " hex=" << std::hex << 255 << std::dec << " f=" << 3.5;
    RXDK_TEST_TRUE(os.str() == "x=42 hex=ff f=3.5");

    // istringstream: extract int, word, double (num_get).
    std::istringstream is("7 abc 2.5");
    int i = 0;
    std::string w;
    double d = 0.0;
    is >> i >> w >> d;
    RXDK_TEST_EQ(i, 7);
    RXDK_TEST_TRUE(w == "abc");
    RXDK_TEST_TRUE(d == 2.5);

    // Real std::cout (global stream object init) -> visible on the debug monitor.
    std::cout << "RXDK-LibsZig: std::cout works " << 123 << " pi=" << 3.14159 << std::endl;
    return 0;
}

int test_fstream_roundtrip()
{
    // std::ofstream/ifstream over FATX (basic_filebuf -> fopen/fread/fwrite).
    // E: is mounted to Harddisk0\Partition1 by the harness (see mount_e_drive).
    const char *path = "E:\\rxdk_fstream.txt";
    {
        std::ofstream out(path, std::ios::trunc);
        RXDK_TEST_TRUE(out.is_open());
        out << "line1\n" << 42 << " " << 3.5 << "\n";
    }
    {
        std::ifstream in(path);
        RXDK_TEST_TRUE(in.is_open());
        std::string l1;
        std::getline(in, l1);
        RXDK_TEST_TRUE(l1 == "line1");
        int n = 0;
        double d = 0.0;
        in >> n >> d;
        RXDK_TEST_EQ(n, 42);
        RXDK_TEST_TRUE(d == 3.5);
    }
    std::error_code ec;
    std::filesystem::remove(path, ec);
    return 0;
}

int test_pmr_vector()
{
    // Polymorphic allocators over a fixed stack buffer (no global heap churn).
    std::array<std::byte, 2048> buf;
    std::pmr::monotonic_buffer_resource pool(buf.data(), buf.size());
    std::pmr::vector<int> v(&pool);
    for (int i = 0; i < 16; ++i)
        v.push_back(i * i);
    RXDK_TEST_EQ(v.size(), 16u);
    RXDK_TEST_EQ(v[15], 225);

    std::pmr::string s("hello", &pool);
    s += " world";
    RXDK_TEST_TRUE(s == "hello world");
    return 0;
}

int test_valarray_basic()
{
    std::valarray<int> a = {1, 2, 3, 4};
    std::valarray<int> b = a * 2;
    RXDK_TEST_EQ(b.sum(), 20); // 2+4+6+8
    RXDK_TEST_EQ(a.max(), 4);

    std::valarray<double> d = {1.0, 4.0, 9.0};
    std::valarray<double> r = std::sqrt(d);
    RXDK_TEST_TRUE(r[2] == 3.0);

    std::valarray<int> sub = a[std::slice(1, 2, 2)]; // indices 1,3 -> {2,4}
    RXDK_TEST_EQ(sub.size(), 2u);
    RXDK_TEST_EQ(sub[0], 2);
    RXDK_TEST_EQ(sub[1], 4);
    return 0;
}

// ---- C++20 blocking sync (atomic wait/notify backend = atomic.cpp) ----

int test_atomic_wait()
{
    std::atomic<int> a{0};
    std::thread setter([&] {
        a.store(7);
        a.notify_one();
    });
    a.wait(0); // blocks until the value differs from 0
    RXDK_TEST_EQ(a.load(), 7);
    setter.join();
    return 0;
}

int test_latch_wait()
{
    std::latch l(3);
    std::atomic<int> counter{0};
    std::vector<std::thread> ts;
    for (int i = 0; i < 3; ++i)
        ts.emplace_back([&] {
            counter.fetch_add(1);
            l.count_down();
        });
    l.wait();
    RXDK_TEST_EQ(counter.load(), 3);
    for (auto &t : ts)
        t.join();
    return 0;
}

int test_semaphore_acquire()
{
    std::counting_semaphore<4> sem(0);
    std::atomic<int> produced{0};
    std::thread producer([&] {
        for (int i = 0; i < 3; ++i) {
            produced.fetch_add(1);
            sem.release();
        }
    });
    for (int i = 0; i < 3; ++i)
        sem.acquire();
    producer.join();
    RXDK_TEST_EQ(produced.load(), 3);
    return 0;
}

int test_barrier_phases()
{
    std::atomic<int> ticks{0};
    std::barrier b(2);
    auto worker = [&] {
        b.arrive_and_wait();
        ticks.fetch_add(1);
        b.arrive_and_wait();
        ticks.fetch_add(1);
    };
    std::thread t1(worker);
    std::thread t2(worker);
    t1.join();
    t2.join();
    RXDK_TEST_EQ(ticks.load(), 4); // 2 threads x 2 phases
    return 0;
}

int test_charconv_from_floating()
{
    // std::from_chars(string -> double), inverse of the to_chars test above.
    double v = 0;
    const char *s = "2.5xyz";
    auto r = std::from_chars(s, s + 6, v);
    RXDK_TEST_TRUE(r.ec == std::errc{});
    RXDK_TEST_TRUE(v == 2.5);
    RXDK_TEST_TRUE(r.ptr == s + 3); // consumed "2.5"

    double sci = 0;
    const char *s2 = "1.5e3";
    auto r2 = std::from_chars(s2, s2 + 5, sci);
    RXDK_TEST_TRUE(r2.ec == std::errc{});
    RXDK_TEST_TRUE(sci == 1500.0);

    double bad = -1;
    const char *s3 = "abc";
    auto r3 = std::from_chars(s3, s3 + 3, bad);
    RXDK_TEST_TRUE(r3.ec == std::errc::invalid_argument);
    return 0;
}

// C11 tss (thread-specific storage) destructors must run when the owning thread
// exits (threads.c rxdk_run_tss_dtors). This is the mechanism C++ thread_local
// dtors layer on; thread_local *storage* itself (Windows TLS model) is a separate
// gap for raw libc/libcpp threads, so we exercise the tss layer directly.
std::atomic<int> g_tss_dtor_count{0};
void rxdk_tss_dtor(void *p)
{
    if (p)
        g_tss_dtor_count.fetch_add(1);
}

int test_tss_dtor()
{
    g_tss_dtor_count.store(0);
    tss_t key;
    RXDK_TEST_EQ(tss_create(&key, rxdk_tss_dtor), (int)thrd_success);
    std::thread t([&] { tss_set(key, reinterpret_cast<void *>(0x1234)); });
    t.join(); // thread exited -> its tss destructor must have run
    RXDK_TEST_EQ(g_tss_dtor_count.load(), 1);
    tss_delete(key);
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
    {"charconv", "floating", test_charconv_floating},
    {"regex", "match", test_regex_match},
    {"filesystem", "ops", test_filesystem_ops},
    {"exceptions", "basic", test_exceptions_basic},
    {"format", "basic", test_format_basic},
    {"print", "basic", test_print_basic},
    {"coroutine", "generator", test_coroutine_generator},
    {"coroutine", "task", test_coroutine_task},
    {"iostream", "sstream", test_iostream_sstream},
    {"fstream", "roundtrip", test_fstream_roundtrip},
    {"pmr", "vector", test_pmr_vector},
    {"valarray", "basic", test_valarray_basic},
    {"atomic", "wait", test_atomic_wait},
    {"latch", "wait", test_latch_wait},
    {"semaphore", "acquire", test_semaphore_acquire},
    {"barrier", "phases", test_barrier_phases},
    {"charconv", "from_floating", test_charconv_from_floating},
    {"tss", "dtor", test_tss_dtor},
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
