// Reduced repro for allocator-like dependent member calls with forwarded packs.

struct Alloc {
    void construct(int* p, int a, int b) { *p = a + b; }
};

template<typename T>
T&& forward(T& t) { return static_cast<T&&>(t); }

template<typename Allocator, typename... Args>
int run(Allocator& alloc, Args... args) {
    int out = 0;
    alloc.construct(&out, forward<Args>(args)...);
    return out == 9 ? 0 : 1;
}

int main() {
    int a = 4;
    int b = 5;
    Alloc alloc;
    return run<Alloc, int, int>(alloc, a, b);
}
