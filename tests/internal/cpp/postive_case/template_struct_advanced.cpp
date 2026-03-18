// Test: advanced template struct scenarios.
// Triple-nested, non-template struct with template struct fields,
// deferred double-nesting, and pointers to template structs.

template<typename T>
struct Pair {
    T first;
    T second;
};

template<typename T>
struct Box {
    T value;
};

// Triple-nested template struct
int test_triple_nested() {
    Box<Box<Box<int>>> b;
    b.value.value.value = 42;
    if (b.value.value.value != 42) return 1;
    return 0;
}

// Deferred double-nested: template function returning Box<Box<T>>
template<typename T>
Box<Box<T>> nest_twice(T x) {
    Box<Box<T>> result;
    result.value.value = x;
    return result;
}

// Non-template struct containing template struct fields
struct Holder {
    Box<Pair<int>> boxed_pair;
    Pair<Box<int>> paired_box;
};

// Pointer to template struct
int test_pointer() {
    Box<int> b;
    b.value = 42;
    Box<int>* bp = &b;
    if (bp->value != 42) return 1;

    Box<Pair<int>> bp2;
    bp2.value.first = 10;
    bp2.value.second = 32;
    Box<Pair<int>>* pp = &bp2;
    if (pp->value.first + pp->value.second != 42) return 2;
    return 0;
}

int main() {
    if (test_triple_nested() != 0) return 1;

    Box<Box<int>> bb = nest_twice<int>(42);
    if (bb.value.value != 42) return 2;

    Holder h;
    h.boxed_pair.value.first = 10;
    h.boxed_pair.value.second = 32;
    if (h.boxed_pair.value.first + h.boxed_pair.value.second != 42) return 3;

    h.paired_box.first.value = 20;
    h.paired_box.second.value = 22;
    if (h.paired_box.first.value + h.paired_box.second.value != 42) return 4;

    if (test_pointer() != 0) return 5;

    return 0;
}
