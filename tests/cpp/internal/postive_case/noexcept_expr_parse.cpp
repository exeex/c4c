// Reduced from libstdc++ type_traits: parse noexcept(expr) in expression
// context instead of treating `noexcept` as an unexpected primary token.

int foo() noexcept;

constexpr bool probe() {
  return noexcept(foo());
}

int main() {
  return probe() ? 0 : 1;
}
