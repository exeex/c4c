int main(void) {
  int lhs;
  int rhs;
  int selected;

  lhs = 5;
  rhs = 7;
  selected = lhs != 5 ? rhs != 7 : 0;

  if (selected != 0)
    return 2;
  return 0;
}
