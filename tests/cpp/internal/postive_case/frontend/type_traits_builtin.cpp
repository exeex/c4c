consteval int same_int() {
  return __builtin_types_compatible_p(int, int);
}

consteval int same_mixed() {
  return __builtin_types_compatible_p(int, char);
}

int main() {
  int result = same_int() * 42 + same_mixed();
  int expect = 42;
  if (result == expect) return 0;
  else return -1;
}
