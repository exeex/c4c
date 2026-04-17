// Frontend regression: unqualified parameter/local lookup inside a namespace
// function must not be forced through same-namespace value canonicalization.
namespace eastl {
template <class C>
auto size(const C& c) -> decltype(c.size()) {
    return c.size();
}

int align(int size, int space) {
    if (space >= size)
        return space - size;
    return -1;
}
}  // namespace eastl

int main() {
    return eastl::align(3, 10) == 7 ? 0 : 1;
}
