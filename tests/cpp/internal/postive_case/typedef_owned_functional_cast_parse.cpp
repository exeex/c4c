// Parse-only regression: unqualified typedef-owned functional casts inside
// record methods should parse directly without rewinding through the generic
// identifier-expression path.
// RUN: %c4cll --parse-only %s

template <class T>
struct box {
  typedef T value_type;

  static constexpr value_type zero() { return value_type(); }
  static constexpr value_type zero_cast() {
    return static_cast<value_type>(0);
  }
};

int main() {
  return box<int>::zero() + box<int>::zero_cast();
}
