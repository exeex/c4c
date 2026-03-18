// Phase 7: specialization identity test.
// Verifies that multiple template instantiations produce distinct, stable
// specialization keys and that the runtime behavior is correct.

template<typename T>
T add(T a, T b) { return a + b; }

template<typename T>
T mul(T a, T b) { return a * b; }

int main() {
    int ri = add<int>(3, 4);     // spec-key: add<T=int>
    int mi = mul<int>(5, 6);     // spec-key: mul<T=int>
    int rd = add<double>(10, 20); // spec-key: add<T=double>
    if (ri != 7) return 1;
    if (mi != 30) return 2;
    if (rd != 30) return 3;
    return 0;
}
