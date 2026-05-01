int pick(int&) { return 1; }
int pick(int&&) { return 2; }

namespace ns {
struct Box {
  using AliasL = int&;
  typedef int&& AliasR;
};
}

int main() {
  int x = 3;

  ns::Box::AliasL local_l = (ns::Box::AliasL)x;
  local_l = 5;
  int local_lkind = pick((ns::Box::AliasL)x);
  int local_rkind = pick((ns::Box::AliasR)x);

  ::ns::Box::AliasL global_l = (::ns::Box::AliasL)x;
  global_l = 7;
  int global_lkind = pick((::ns::Box::AliasL)x);
  int global_rkind = pick((::ns::Box::AliasR)x);

  ::ns::Box::AliasR r = (::ns::Box::AliasR)x;
  r = 11;

  return (x == 11 && local_l == 11 && global_l == 11 &&
          local_lkind == 1 && local_rkind == 2 &&
          global_lkind == 1 && global_rkind == 2 && r == 11)
             ? 0
             : 1;
}
