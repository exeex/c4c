// Expected: compile fail (case label is not an integer constant expression)
int main(void) {
  int x = 1;
  switch (0) {
    case x:
      return 1;
    default:
      return 0;
  }
}
