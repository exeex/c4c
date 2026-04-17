int pick(int&) { return 1; }
int pick(int&&) { return 2; }

int main() {
  int x = 3;
  using AliasL = int&;
  typedef int&& AliasR;

  AliasL l = (AliasL)x;
  l = 7;

  int lkind = pick((AliasL)x);
  int rkind = pick((AliasR)x);

  AliasR r = (AliasR)x;
  r = 11;

  return (x == 11 && lkind == 1 && rkind == 2 && l == 11 && r == 11) ? 0 : 1;
}
