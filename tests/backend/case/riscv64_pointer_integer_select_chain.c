int main(void) {
  int flag;
  void *p;
  int *q;

  flag = 0;
  p = flag ? 0 : 0;
  p = flag ? (flag ? 0 : 0) : (flag ? 0 : 0);
  q = p;

  return q ? 1 : 0;
}
