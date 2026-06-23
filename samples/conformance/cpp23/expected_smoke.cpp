#include <expected>
#include <iostream>
#include <string_view>

int main(void)
{
    std::expected<int, int> ok = 42;
    if (!ok || *ok != 42)
        return 1;
    std::string_view sv = "cpp23";
    if (sv.size() != 5)
        return 2;
    std::cout << "RXDK-LibsZig cpp23-expected-smoke OK\n";
    for (;;)
        ;
}
