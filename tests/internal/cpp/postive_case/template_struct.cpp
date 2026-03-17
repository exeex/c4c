template<typename T>
struct Pair {
    T first;
    T second;
};

template<typename T>
struct Box {
    T value;
};

int main() {
    // Basic template struct with int
    Pair<int> p;
    p.first = 10;
    p.second = 32;
    if (p.first + p.second != 42) return 1;

    // Different instantiation with long
    Pair<long> pl;
    pl.first = 100L;
    pl.second = -58L;
    if (pl.first + pl.second != 42L) return 2;

    // Nested template struct usage
    Box<int> b;
    b.value = 42;
    if (b.value != 42) return 3;

    return 0;
}
