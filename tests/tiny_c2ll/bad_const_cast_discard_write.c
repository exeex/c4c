// Expected: compile fail (explicit cast drops const qualifier then writes)
int main() {
  const int x = 1;
  *((int *)&x) = 2;
  return x;
}
