// Expected: compile fail (conflicting function declarations: parameter type mismatch)
int foo(int x);
int foo(double x);

int main(void) {
  return 0;
}
