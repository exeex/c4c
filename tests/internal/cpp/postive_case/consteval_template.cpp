template <typename T>
consteval T square(T value) {
  return value * value;
}

int main() {
  int result = square<int>(6);
  int expect = 36;
  if (result == expect) return 0;
  else return -1;
}
