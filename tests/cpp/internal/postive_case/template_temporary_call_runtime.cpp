// Reduced repro for calling operator() on a template-id temporary.

template<int N>
struct Cmp {
    int operator()(int a, int b) const { return a == b && N == 2 ? 1 : 0; }
};

template<typename A, typename B>
int same(A a, B b) {
    return Cmp<2>()(a, b) ? 0 : 1;
}

int main() {
    return same(5, 5);
}
