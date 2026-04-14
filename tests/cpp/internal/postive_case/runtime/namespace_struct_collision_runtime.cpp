// Runtime test: same-named structs in different namespaces must not collide.
namespace alpha {
struct Data {
  int x;
};
int get(Data d) { return d.x; }
}

namespace beta {
struct Data {
  int x;
  int y;
};
int get(Data d) { return d.x + d.y; }
}

int main() {
  alpha::Data a;
  a.x = 10;
  if (alpha::get(a) != 10) return 1;

  beta::Data b;
  b.x = 3;
  b.y = 7;
  if (beta::get(b) != 10) return 2;

  return 0;
}
