// Parse-only regression: dependent member typedef caching and concrete
// template-instantiation member typedef registration should both route through
// the parser-local typedef helpers.
// RUN: %c4cll --parse-only %s

template <typename T>
struct box {
  using value_type = T;
  using type = value_type;
};

template <typename T>
struct outer {
  using alias = typename box<T>::type;
  typename box<T>::type field;
};

using concrete = box<int>::type;

outer<int>::alias make_alias(box<int>::type value) {
  outer<int>::alias copy = value;
  return copy;
}

int main() {
  outer<int> value{};
  value.field = make_alias(7);
  concrete result = value.field;
  return result - 7;
}
