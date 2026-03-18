// Template function + template struct combined usage.
// Tests that template structs used as parameters, return types,
// and local variables inside template functions are correctly
// deferred and instantiated with concrete types.

template<typename T>
struct Pair {
    T first;
    T second;
};

template<typename T, int N>
struct Array {
    T data[N];
};

// Template function taking a template struct parameter.
template<typename T>
T sum(Pair<T> p) {
    return p.first + p.second;
}

// Template function returning a template struct.
template<typename T>
Pair<T> make_pair(T a, T b) {
    Pair<T> p;
    p.first = a;
    p.second = b;
    return p;
}

// Template function with local template struct variable.
template<typename T>
T swap_and_diff(T a, T b) {
    Pair<T> tmp;
    tmp.first = b;
    tmp.second = a;
    return tmp.first - tmp.second;
}

// Template function with NTTP template struct.
template<typename T>
T sum_array3(Array<T, 3> a) {
    return a.data[0] + a.data[1] + a.data[2];
}

int main() {
    // Test 1: template struct as function parameter
    Pair<int> pi;
    pi.first = 10;
    pi.second = 32;
    if (sum<int>(pi) != 42) return 1;

    Pair<long> pl;
    pl.first = 100L;
    pl.second = -58L;
    if (sum<long>(pl) != 42L) return 2;

    // Test 2: template function returning template struct
    Pair<int> p2 = make_pair<int>(10, 32);
    if (p2.first + p2.second != 42) return 3;

    Pair<long> p3 = make_pair<long>(100L, -58L);
    if (p3.first + p3.second != 42L) return 4;

    // Test 3: local template struct inside template function
    if (swap_and_diff<int>(10, 52) != 42) return 5;
    if (swap_and_diff<long>(100L, 142L) != 42L) return 6;

    // Test 4: NTTP template struct in template function
    Array<int, 3> ai;
    ai.data[0] = 10;
    ai.data[1] = 20;
    ai.data[2] = 12;
    if (sum_array3<int>(ai) != 42) return 7;

    return 0;
}
