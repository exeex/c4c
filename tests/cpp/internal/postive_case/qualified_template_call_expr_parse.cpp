// Parse-only regression: unresolved qualified template function calls in
// expression context should not pay the broad qualified-type/functional-cast
// probe before normal expression parsing.
// RUN: %c4cll --parse-only %s

namespace ns {

template <class T>
T&& forward(T& value) {
  return static_cast<T&&>(value);
}

template <class T>
struct box {
  T value;
};

}  // namespace ns

template <class T>
T pass_through(T& value) {
  return ns::forward<T>(value);
}

template <class T>
T make_value() {
  return ns::box<T>{T{}}.value;
}

int main() {
  int value = 7;
  if (pass_through(value) != 7)
    return 1;
  if (make_value<int>() != 0)
    return 2;
  return 0;
}
