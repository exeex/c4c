namespace ns {
template <class T>
struct box {
  using value_type = T;
};
}

int main() {
  int x = 3;
  using Alias = ns::box<int>::value_type;
  Alias y = (Alias)x;
  Alias* p = (Alias*)&x;
  return (y == 3 && *p == 3) ? 0 : 1;
}
