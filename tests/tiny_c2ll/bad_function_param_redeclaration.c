// Expected: compile fail (duplicate parameter name in same scope)
int bad(int x, int x) {
  return x;
}
