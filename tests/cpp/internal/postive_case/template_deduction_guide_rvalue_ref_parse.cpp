// Parse-only regression: top-level deduction guides with forwarding-reference
// parameters should stay on the parameter-list path instead of being treated
// as grouped declarators.
// RUN: %c4cll --parse-only %s

template <class T>
struct box {};

template <class T>
box(T&&) -> box<T>;

int main() {
  return 0;
}
