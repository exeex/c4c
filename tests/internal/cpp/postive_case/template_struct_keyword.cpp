// Test: 'struct' keyword syntax with template structs
// struct Pair<int> p; should work the same as Pair<int> p;

template<typename T>
struct Pair {
    T first;
    T second;
};

template<typename T, int N>
struct Array {
    T data[N];
};

// Function using struct keyword in param and return type
struct Pair<long> make_pair_long(long a, long b) {
    struct Pair<long> p;
    p.first = a;
    p.second = b;
    return p;
}

int main() {
    // Local decl with struct keyword
    struct Pair<int> p;
    p.first = 10;
    p.second = 32;
    int sum = p.first + p.second;  // 42

    // NTTP struct with struct keyword
    struct Array<int, 3> a;
    a.data[0] = 1;
    a.data[1] = 2;
    a.data[2] = 3;
    int arr_sum = a.data[0] + a.data[1] + a.data[2];  // 6

    // Function returning struct template
    struct Pair<long> lp = make_pair_long(100L, -58L);
    int lsum = (int)(lp.first + lp.second);  // 42

    // Mixed: some with struct keyword, some without
    Pair<int> p2;
    p2.first = 5;
    p2.second = 7;
    struct Pair<int> p3;
    p3.first = p2.first;
    p3.second = p2.second;
    int mixed = p3.first + p3.second;  // 12

    return sum + arr_sum + lsum + mixed - 102;
    // 42 + 6 + 42 + 12 - 102 = 0
}
