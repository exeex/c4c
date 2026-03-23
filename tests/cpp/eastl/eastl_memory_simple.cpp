#include <EASTL/memory.h>

struct Box {
    int value;

    Box() : value(0) {}
    explicit Box(int v) : value(v) {}
};

int main() {
    eastl::late_constructed<Box, false> storage;
    if (storage.is_constructed())
        return 1;

    storage.construct(42);
    if (!storage.is_constructed())
        return 2;
    if (storage->value != 42)
        return 3;

    storage.destruct();
    if (storage.is_constructed())
        return 4;

    alignas(16) char buffer[64];
    void* ptr = buffer;
    size_t space = sizeof(buffer);
    void* aligned = eastl::align(16, sizeof(Box), ptr, space);
    if (!aligned)
        return 5;

    Box* box = ::new (aligned) Box(9);
    int value = box->value;
    eastl::destroy_at(box);
    return value == 9 ? 0 : 6;
}
