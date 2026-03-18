// Negative test: consteval function called with result of a runtime function.

int get_value();

consteval int double_it(int x) {
  return x * 2;
}

int main() {
  int r = double_it(get_value());  // error: argument is not a constant
  return r;
}
