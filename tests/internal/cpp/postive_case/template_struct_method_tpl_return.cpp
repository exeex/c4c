// Test: template struct methods returning other template structs.
// Verifies that tpl_struct_origin types are resolved in method
// return types and bodies during template struct instantiation.

template<typename T>
struct Pair {
    T first;
    T second;
};

template<typename T>
struct Container {
    T item;

    // Method returning a different template struct instantiation
    Pair<T> as_pair() {
        Pair<T> p;
        p.first = item;
        p.second = item;
        return p;
    }
};

// Template struct with method taking template struct param
template<typename T>
struct Transformer {
    T scale;

    Pair<T> make_scaled_pair(T x) {
        Pair<T> p;
        p.first = x;
        p.second = x * scale;
        return p;
    }
};

int main() {
    // Basic: method returns template struct
    Container<int> c;
    c.item = 21;
    Pair<int> p = c.as_pair();
    if (p.first + p.second != 42) return 1;

    // Different instantiation
    Container<long> cl;
    cl.item = 100L;
    Pair<long> pl = cl.as_pair();
    if (pl.first + pl.second != 200L) return 2;

    // Method with computation returning template struct
    Transformer<int> t;
    t.scale = 3;
    Pair<int> tp = t.make_scaled_pair(10);
    if (tp.first != 10) return 3;
    if (tp.second != 30) return 4;

    return 0;
}
