// RUN: %c4cll --parse-only %s
// Test: member function templates with nested template calls in default template args.
// Regression test for angle-bracket depth tracking inside parenthesized expressions.

struct true_type {};
struct false_type {};

template<typename T> T declval();

template<typename From, typename To>
class ConvertHelper {
    template<typename T1>
    static void test_aux(T1);

    // The key pattern: decltype(call<A>(call<B>())) as default template argument
    // inside a class body. The > from call<B> must not close the outer template<>.
    template<typename F1, typename T1,
             typename = decltype(test_aux<T1>(declval<F1>()))>
    static true_type test(int);

    template<typename, typename>
    static false_type test(...);

public:
    using type = decltype(test<From, To>(0));
};

int main() {
    return 0;
}
