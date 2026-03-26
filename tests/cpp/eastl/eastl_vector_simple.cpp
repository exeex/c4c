#include <EASTL/vector.h>

extern "C" void* malloc(unsigned long);

void* operator new[](unsigned long size, const char*, int, unsigned, const char*, int) {
    return malloc(size ? size : 1);
}

void* operator new[](unsigned long size, unsigned long, unsigned long, const char*, int,
                     unsigned, const char*, int) {
    return malloc(size ? size : 1);
}

int main() {
    eastl::vector<int> values;
    values.push_back(1);
    values.push_back(4);
    values.push_back(7);

    if (values.size() != 3)
        return 1;
    if (values.front() != 1)
        return 2;
    if (values.back() != 7)
        return 3;

    values.pop_back();
    if (values.size() != 2)
        return 4;

    return values[1] == 4 ? 0 : 5;
}
