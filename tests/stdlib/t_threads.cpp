/* std::thread / mutex / condition_variable / atomic. */
#include "rxdk_test.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <unistd.h>   // sysconf(_SC_NPROCESSORS_*)

int main() {
    // join returns a computed value
    int result = 0;
    std::thread t([&]{ result = 21 * 2; });
    t.join();
    CHECK_EQI(result, 42, "thread runs and join synchronises");

    // mutex-guarded counter across N threads
    std::mutex m;
    int counter = 0;
    const int N = 8, PER = 1000;
    std::vector<std::thread> ts;
    for (int i = 0; i < N; ++i)
        ts.emplace_back([&]{ for (int k = 0; k < PER; ++k) { std::lock_guard<std::mutex> lk(m); ++counter; } });
    for (auto &th : ts) th.join();
    CHECK_EQI(counter, N * PER, "mutex-guarded increments are not lost");

    // atomic counter
    std::atomic<int> acount{0};
    std::vector<std::thread> at;
    for (int i = 0; i < N; ++i)
        at.emplace_back([&]{ for (int k = 0; k < PER; ++k) acount.fetch_add(1); });
    for (auto &th : at) th.join();
    CHECK_EQI(acount.load(), N * PER, "atomic increments consistent");

    // condition_variable rendezvous
    std::mutex cm;
    std::condition_variable cv;
    bool go = false;
    int woke = 0;
    std::thread w([&]{
        std::unique_lock<std::mutex> lk(cm);
        cv.wait(lk, [&]{ return go; });
        woke = 1;
    });
    { std::lock_guard<std::mutex> lk(cm); go = true; }
    cv.notify_one();
    w.join();
    CHECK(woke, "condition_variable wait/notify");

    // The original Xbox is a single-core Pentium III (no SMT). Assert the exact
    // count (not just >= 1): hardware_concurrency() flows from
    // sysconf(_SC_NPROCESSORS_ONLN), which the picolibc fallback returns as 1 on
    // i386. (The 360 copy asserts 6 -- Xenon's 3 cores x 2 HW threads.)
    CHECK_EQI((long)std::thread::hardware_concurrency(), 1,
              "hardware_concurrency == 1 (single-core Pentium III)");
    CHECK_EQI((long)sysconf(_SC_NPROCESSORS_ONLN), 1, "sysconf(_SC_NPROCESSORS_ONLN) == 1");
    CHECK_EQI((long)sysconf(_SC_NPROCESSORS_CONF), 1, "sysconf(_SC_NPROCESSORS_CONF) == 1");

    CHECK_DONE("threads");
    return 0;
}
