// Expected: compile fail (assignment to const object)
int main() {
  const int x = 1;
  x = 2;
  return x;
}
