int main(void) {
  int value;
  int *p;
  int **pp;

  value = 0;
  p = &value;
  pp = &p;
  **pp = 7;

  return value - 7;
}
