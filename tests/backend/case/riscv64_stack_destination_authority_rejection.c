int main(void) {
  int lhs = 1;
  int rhs = 2;
  int dest;
  int cond = lhs != rhs;

  dest = cond ? lhs : rhs;
  volatile int *forced_home = &dest;
  return *forced_home - 1;
}
