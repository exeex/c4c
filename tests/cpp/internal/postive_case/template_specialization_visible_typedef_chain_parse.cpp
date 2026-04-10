// Parse-only regression: namespace-visible typedef chains should still drive
// template specialization selection and typedef-owned functional casts after
// the parser drops the flattened typedef/value compatibility bridge members.
// RUN: %c4cll --parse-only %s

namespace lib {

typedef long value_type;

}  // namespace lib

using lib::value_type;
typedef value_type* value_ptr;

template <class T>
struct pick {
  typedef T type;
};

template <class T>
struct pick<T*> {
  typedef short type;
};

int same_value = __is_same(pick<value_ptr>::type, short);

int make_value() { return pick<value_ptr>::type(7); }

int main() {
  pick<value_ptr>::type value = pick<value_ptr>::type(5);
  return value + make_value() + same_value;
}
