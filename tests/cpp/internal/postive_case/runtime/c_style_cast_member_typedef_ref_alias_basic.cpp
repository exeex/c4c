int pick(int&) { return 1; }
int pick(int&&) { return 2; }

struct Box {
  using AliasL = int&;
  typedef int&& AliasR;
};

int main() {
  int x = 3;

  Box::AliasL l = (Box::AliasL)x;
  l = 7;

  int lkind = pick((Box::AliasL)x);
  int rkind = pick((Box::AliasR)x);

  Box::AliasR r = (Box::AliasR)x;
  r = 11;

  return (x == 11 && lkind == 1 && rkind == 2 && l == 11 && r == 11) ? 0 : 1;
}
