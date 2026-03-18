// Expected: compile fail (discard const qualifier in pointer assignment)
int main() {
  const int x = 1;
  int *p = &x;
  return *p;
}
