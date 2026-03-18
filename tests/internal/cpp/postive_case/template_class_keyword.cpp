// Test: 'class' keyword as synonym for 'struct' in template contexts
// 1. template<class T> as parameter introducer
// 2. class Name { ... } in template definition
// 3. class Name<args> for template struct instantiation

template<class T>
class Pair {
public:
    T first;
    T second;
};

template<class T, int N>
class Array {
public:
    T data[N];
};

// Function using class keyword in param and return type
class Pair<long> make_pair_long(long a, long b) {
    class Pair<long> p;
    p.first = a;
    p.second = b;
    return p;
}

int main() {
    // Local decl with class keyword
    class Pair<int> p;
    p.first = 10;
    p.second = 32;
    int sum = p.first + p.second;  // 42

    // NTTP with class keyword
    class Array<int, 3> a;
    a.data[0] = 1;
    a.data[1] = 2;
    a.data[2] = 3;
    int arr_sum = a.data[0] + a.data[1] + a.data[2];  // 6

    // Function returning class template
    class Pair<long> lp = make_pair_long(100L, -58L);
    int lsum = (int)(lp.first + lp.second);  // 42

    // Mixed: class and struct keywords interchangeably
    struct Pair<int> p2;
    p2.first = 5;
    p2.second = 7;
    class Pair<int> p3;
    p3.first = p2.first;
    p3.second = p2.second;
    int mixed = p3.first + p3.second;  // 12

    // template<class T> with struct instantiation, and vice versa
    Pair<int> p4;
    p4.first = 20;
    p4.second = 22;
    int plain = p4.first + p4.second;  // 42

    return sum + arr_sum + lsum + mixed + plain - 144;
    // 42 + 6 + 42 + 12 + 42 - 144 = 0
}
