int main(void) {
  short values[4];
  int index = 2;
  short replacement = 9;

  values[0] = 1;
  values[1] = 2;
  values[2] = 3;
  values[3] = 4;

  values[index] = replacement;

  return values[2] == 9 ? 0 : 1;
}
