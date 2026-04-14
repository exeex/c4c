// Phase 5 slice 16: Nested template struct usage.
// Tests Pair<Pair<int>>, Box<Pair<int>>, and template functions
// operating on nested template structs.

template<typename T>
struct Pair {
    T first;
    T second;
};

template<typename T>
struct Box {
    T value;
};

// Nested: Box containing a Pair
int test_box_of_pair() {
    Box<Pair<int>> bp;
    bp.value.first = 10;
    bp.value.second = 32;
    if (bp.value.first + bp.value.second != 42) return 1;
    return 0;
}

// Nested: Pair of Pairs
int test_pair_of_pair() {
    Pair<Pair<int>> pp;
    pp.first.first = 1;
    pp.first.second = 2;
    pp.second.first = 3;
    pp.second.second = 4;
    if (pp.first.first + pp.first.second + pp.second.first + pp.second.second != 10) return 1;
    return 0;
}

// Template function with nested template struct param
template<typename T>
T unbox_sum(Box<Pair<T>> bp) {
    return bp.value.first + bp.value.second;
}

// Template function returning nested template struct
template<typename T>
Box<Pair<T>> make_boxed_pair(T a, T b) {
    Box<Pair<T>> result;
    result.value.first = a;
    result.value.second = b;
    return result;
}

int main() {
    if (test_box_of_pair() != 0) return 1;
    if (test_pair_of_pair() != 0) return 2;

    // Template function with nested template struct
    Box<Pair<int>> bp;
    bp.value.first = 20;
    bp.value.second = 22;
    if (unbox_sum<int>(bp) != 42) return 3;

    Box<Pair<long>> bpl;
    bpl.value.first = 100L;
    bpl.value.second = -58L;
    if (unbox_sum<long>(bpl) != 42L) return 4;

    // Template function returning nested template struct
    Box<Pair<int>> bp2 = make_boxed_pair<int>(10, 32);
    if (bp2.value.first + bp2.value.second != 42) return 5;

    return 0;
}
