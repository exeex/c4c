// Parse-only regression: specialization-selected member typedefs should stay
// available through parser-owned template-pattern selection in both type
// traits and functional-cast expression parsing.
// RUN: %c4cll --parse-only %s

template <class T>
struct pick {
  typedef T type;
};

template <class T>
struct pick<T*> {
  typedef int type;
};

int same_value = __is_same(pick<long*>::type, int);

int make_value() { return pick<long*>::type(7); }

int main() {
  pick<long*>::type value = pick<long*>::type(5);
  return value + make_value() + same_value;
}
