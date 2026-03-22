// Parse-only test: template using aliases with <args> in various contexts.
// Validates that the parser correctly skips <...> after a resolved template
// using alias (typedef resolved but not a primary template struct entry).

template <typename T, T... Ints>
struct integer_sequence {};

template <unsigned long... Is>
using index_sequence = integer_sequence<unsigned long, Is...>;

// 1. Template using alias in function parameter
void f(index_sequence<1, 2, 3> x);

// 2. Template using alias in struct body method parameter
struct pair {
    template <unsigned long... I1, unsigned long... I2>
    void construct(index_sequence<I1...>, index_sequence<I2...>);
};

// 3. Simple template using alias
template <typename T>
struct Box {};

template <typename T>
using BoxAlias = Box<T>;

void g(BoxAlias<int> b);

// 4. Template forward declaration (class keyword) with template registration
template <typename... T> class tuple;

// 5. Nested template using alias
template <typename T>
using BoxBox = Box<T>;

void h(BoxBox<BoxAlias<int>> bb);

int main() { return 0; }
