// Parse-only regression: parameter lists should accept unresolved template-id
// type starts so later type parsing can apply the existing fallback.

template<typename T>
struct marker {};

template<typename T>
struct marker<holder<T>> : marker<T> {};

template<typename T>
auto accept(holder<T> value) -> decltype(value);

int main() {
  return 0;
}
