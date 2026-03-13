// Expected: compile fail (return value in void function)
void f(void) {
  return 1;
}

int main(void) {
  f();
  return 0;
}
