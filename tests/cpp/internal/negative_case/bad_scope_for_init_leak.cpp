// Negative test: names declared in a for-init scope must not be visible after
// the loop.
int main() {
  for (int i = 0; i < 1; ++i) {
  }

  return i;
}
