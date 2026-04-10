// HIR regression: constructor initializer lists should treat typedef/alias
// members as records when deciding whether to emit a member constructor call.

template <typename T>
struct Pair {
    int x;
    int y;

    Pair(int a, int b) : x(a), y(b) {}
};

using PairInt = Pair<int>;

struct Holder {
    PairInt first;
    int total;

    Holder(int a, int b) : first(a, b), total(a + b) {}
};

int main() {
    Holder h(10, 32);
    if (h.first.x != 10) return 1;
    if (h.first.y != 32) return 2;
    if (h.total != 42) return 3;
    return 0;
}
