#include <EASTL/tuple.h>

struct Sum3 {
    int operator()(int a, int b, int c) const {
        return a + b + c;
    }
};

int main() {
    auto t = eastl::make_tuple(2, 3, 5);
    if (eastl::get<0>(t) != 2)
        return 1;
    if (eastl::get<2>(t) != 5)
        return 2;
    return eastl::apply(Sum3{}, t) == 10 ? 0 : 3;
}
