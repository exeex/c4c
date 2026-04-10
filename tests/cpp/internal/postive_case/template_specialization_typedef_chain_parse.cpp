// Parse-only regression: template specialization pattern matching should
// resolve typedef-chain arguments through parser-owned helpers instead of
// threading raw typedef maps through the template selection helpers.
// RUN: %c4cll --parse-only %s

typedef long value_type;
typedef value_type* value_ptr;

template <class T>
struct pick {
  typedef T type;
};

template <class T>
struct pick<T*> {
  typedef int type;
};

int same_value = __is_same(pick<value_ptr>::type, int);

int make_value() { return pick<value_ptr>::type(7); }

int main() {
  pick<value_ptr>::type value = pick<value_ptr>::type(5);
  return value + make_value() + same_value;
}
