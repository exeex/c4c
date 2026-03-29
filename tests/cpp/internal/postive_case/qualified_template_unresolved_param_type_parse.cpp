// Parse-only regression: parameter lists should accept unresolved qualified
// template-id type starts so later type parsing can handle the declarator.
// RUN: %c4cll --parse-only %s

template<typename T>
void accept(T& value, ns::holder<T> other);

int main() {
  return 0;
}
