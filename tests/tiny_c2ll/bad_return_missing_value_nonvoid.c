// Expected: compile fail (missing return value in non-void function)
int f(void) {
  return;
}

int main(void) {
  return f();
}
