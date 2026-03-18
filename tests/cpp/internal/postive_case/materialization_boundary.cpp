// Phase 6: materialization boundary test.
// consteval functions are lowered to HIR but not materialized for emission.
// Template instantiations are materialized and track their origin template.

consteval int cube(int x) { return x * x * x; }
consteval int double_it(int x) { return x + x; }

template<typename T>
T apply(T val) {
    return val + cube(3) + double_it(5);
}

int main() {
    int a = cube(4);       // consteval: 64
    int b = double_it(7);  // consteval: 14
    int c = apply<int>(1); // runtime: 1 + 27 + 10 = 38
    if (a != 64) return 1;
    if (b != 14) return 2;
    if (c != 38) return 3;
    return 0;
}
