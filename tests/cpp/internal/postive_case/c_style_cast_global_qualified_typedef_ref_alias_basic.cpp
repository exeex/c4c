int pick(int&) { return 1; }
int pick(int&&) { return 2; }

namespace ns {
using AliasL = int&;
typedef int&& AliasR;
}

int main() {
  int x = 3;

  ::ns::AliasL l = (::ns::AliasL)x;
  l = 7;

  int lkind = pick((::ns::AliasL)x);
  int rkind = pick((::ns::AliasR)x);

  ::ns::AliasR r = (::ns::AliasR)x;
  r = 11;

  return (x == 11 && lkind == 1 && rkind == 2 && l == 11 && r == 11) ? 0 : 1;
}
