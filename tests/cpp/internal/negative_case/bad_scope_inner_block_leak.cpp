// Negative test: names declared in an inner block must not be visible after
// the block ends.
int main() {
  {
    int hidden = 42;
    if (hidden != 42) return 1;
  }

  return hidden;
}
