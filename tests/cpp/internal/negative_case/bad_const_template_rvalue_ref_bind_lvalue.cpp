// Negative regression: `const T&&` is not a forwarding reference and must not
// accept lvalue arguments during template deduction.
template <class T>
int probe(const T&&) {
  return 1;
}

int main() {
  int value = 0;
  return probe(value);
}
