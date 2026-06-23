int main(void) {
  int flag;
  int value;
  int *selected;
  int *published;

  flag = 0;
  value = 9;
  selected = flag ? &value : 0;
  published = selected;

  return published ? 1 : 0;
}
