// Expected: compile fail (duplicate case labels after constant folding)
int main(void) {
  switch (0) {
    case 1 + 1:
      return 1;
    case 2:
      return 2;
    default:
      return 0;
  }
}
