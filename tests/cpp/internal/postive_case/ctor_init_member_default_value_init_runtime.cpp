// HIR regression: zero-argument member initializers in constructor init lists
// should value-initialize scalar template members instead of rejecting them as
// malformed scalar constructor calls.

template <typename T1, typename T2>
struct Pair {
    T1 first;
    T2 second;

    Pair() : first(), second() {}
};

int main() {
    Pair<int, int> p;
    if (p.first != 0) return 1;
    if (p.second != 0) return 2;
    return 0;
}
