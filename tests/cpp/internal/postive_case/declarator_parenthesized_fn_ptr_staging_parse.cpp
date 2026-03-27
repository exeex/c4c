// Parse-only: parenthesized function-pointer declarators should keep nested
// and function-returning-function-pointer wiring stable during staging.
// RUN: %c4cll --parse-only %s

int apply(int value);
int twice(int value);

int (*pick(int which))(int);
int (*(*pick_outer(int which))(int))(int);
int (*__attribute__((unused)) pick_attr(int which))(int);

typedef int (*UnaryFn)(int);
typedef UnaryFn (*FactoryFn)(int);

FactoryFn choose_factory(int which);

int main() {
    UnaryFn direct = pick(0);
    UnaryFn attr = pick_attr(1);
    FactoryFn nested = pick_outer(1);
    return choose_factory(apply(2))(twice(3)) + direct(4) + attr(5) + nested(6)(7);
}
