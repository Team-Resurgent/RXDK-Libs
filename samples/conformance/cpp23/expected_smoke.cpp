#include <expected>
#include <string_view>

int main(void)
{
    std::expected<int, int> ok = 42;
    if (!ok || *ok != 42)
        return 1;
    std::string_view sv = "cpp23";
    return sv.size() == 5 ? 0 : 2;
}
