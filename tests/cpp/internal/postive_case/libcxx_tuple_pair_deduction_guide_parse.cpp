// Reduced from libc++ tuple: deduction guides with template-id parameter
// types should not be misparsed as grouped declarators.
// RUN: %c4cll --parse-only %s

template <class A, class B>
struct pair;

template <class... T>
struct tuple;

template <class A, class B>
tuple(pair<A, B>) -> tuple<A, B>;

int main() {
  return 0;
}
