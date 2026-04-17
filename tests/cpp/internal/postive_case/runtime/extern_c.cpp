extern "C" int c_add(int lhs, int rhs) {
  return lhs + rhs;
}

int main() {
  int result = c_add(19, 23);
  int expect = 42;
  if (result == expect) return 0;
  else return -1;
}
