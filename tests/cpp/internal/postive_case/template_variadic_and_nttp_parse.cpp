// Test: variadic template parameter packs, typedef NTTPs, free operators, delete[]
// Validates parser improvements for EASTL/type_traits bring-up.

typedef unsigned long size_t;

// Non-type template parameter with typedef type (size_t N)
template <typename T, size_t N>
struct Array {
    T data[N];
};

// Template with T as NTTP type (T v where T is a type param)
template <typename T, T v>
struct integral_constant {
    static T value;
    typedef T value_type;
    typedef integral_constant<T, v> type;
};

typedef integral_constant<bool, true> true_type;
typedef integral_constant<bool, false> false_type;

// Template struct with inheritance
template <typename T> struct is_void : public false_type {};
template <> struct is_void<void> : public true_type {};

// Variadic template parameter packs (parse-only, no instantiation)
template <typename... Args>
struct tuple_size {};

// Variadic NTTP pack (parse-only)
template <bool... Bn>
struct bool_pack {};

// Free operator functions
struct Point { int x; int y; };
inline bool operator==(const Point& a, const Point& b) {
    return a.x == b.x && a.y == b.y;
}

// delete / delete[] expressions (parsed but not codegen'd)
void test_delete(int* p) {
    delete p;
    delete[] p;
}

int main() {
    Array<int, 4> arr;
    arr.data[0] = 42;
    arr.data[3] = 99;

    // Validate: arr.data[0]==42, arr.data[3]==99
    if (arr.data[0] != 42) return 1;
    if (arr.data[3] != 99) return 2;
    return 0;
}
