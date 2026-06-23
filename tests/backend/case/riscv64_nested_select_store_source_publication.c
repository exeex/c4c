int main(void) {
  int flag = 0;
  int payload = 7;
  void *base = &payload;
  void *chosen = 0;
  void *published = 0;

  chosen = flag ? base : 0;
  chosen = flag ? chosen : base;
  published = flag ? (flag ? chosen : 0) : chosen;
  published = flag ? 0 : (flag ? base : published);

  return published == 0 ? 1 : 0;
}
