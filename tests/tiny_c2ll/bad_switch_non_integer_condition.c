// Expected: compile fail (switch controlling expression must be integer)
int main(void) {
  double d = 1.0;
  switch (d) {
    case 1:
      return 0;
    default:
      return 1;
  }
}
