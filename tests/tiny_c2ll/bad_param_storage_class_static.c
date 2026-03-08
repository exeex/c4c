// Expected: compile fail (invalid use of static in parameter declaration)
int f(static int x) {
  return x;
}
