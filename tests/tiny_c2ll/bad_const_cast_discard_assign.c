// Expected: compile fail (explicit cast drops const qualifier)
int main() {
  const int x = 1;
  int *p = (int *)&x;
  return *p;
}
