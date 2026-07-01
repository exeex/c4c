int main(void) {
  int lhs = 5;
  int rhs = 7;
  int comparison = lhs != rhs;
  volatile int *forced_home = &comparison;

  return *forced_home != 0 ? 0 : 1;
}
