// HIR regression: templated delegating constructors should still match the
// owning instantiated record when the init target uses the unqualified source
// name instead of the lowered struct tag.

template <typename T1, typename T2>
struct Pair {
    int first;
    int second;

    Pair(int a, int b, int c, int d) : first(a + b), second(c + d) {}
    Pair(int a, int b) : Pair(a, 1, b, 2) {}
};

int main() {
    Pair<int, int> p(4, 9);
    if (p.first != 5) return 1;
    if (p.second != 11) return 2;
    return 0;
}
