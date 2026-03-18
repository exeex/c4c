// Expected: compile fail (arity mismatch: too many arguments)
int add2(int a, int b) {
  return a + b;
}

int main(void) {
  return add2(1, 2, 3);
}
