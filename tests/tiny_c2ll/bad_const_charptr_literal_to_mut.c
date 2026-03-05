// Expected: compile fail (string literal assigned to mutable char*)
int main() {
  char *p = "hello";
  return p[0];
}
