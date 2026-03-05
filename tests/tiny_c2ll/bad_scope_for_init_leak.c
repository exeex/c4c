// Expected: compile fail (for-init variable leaks out of scope)
int main() {
  for (int i = 0; i < 3; i++) {
  }
  return i;
}
