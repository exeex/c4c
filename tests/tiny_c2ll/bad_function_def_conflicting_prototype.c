// Expected: compile fail (definition conflicts with prior prototype)
int foo(int x);
int foo(double x) {
  return (int)x;
}

int main(void) {
  return foo(1);
}
