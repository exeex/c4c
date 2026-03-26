// Parse-only regression: a member function template inside a class template
// should see both the enclosing class-template parameter and its own template
// parameter without flattening their ownership.

template <typename T>
struct S {
  template <typename U>
  void f(T lhs, U rhs);
};

template <typename T>
template <typename U>
void S<T>::f(T lhs, U rhs) {
  (void)lhs;
  (void)rhs;
}

int main() {
  S<int> s;
  s.f(1, 2L);
  return 0;
}
