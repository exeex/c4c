#include <EASTL/utility.h>

int main() {
    eastl::pair<int, int> p(4, 9);
    eastl::swap(p.first, p.second);
    return (p.first == 9 && p.second == 4) ? 0 : 1;
}
