// Runtime test: structs defined in namespaces should work correctly.
namespace geo {
struct Point {
  int x;
  int y;
};
int dot(Point a, Point b) { return a.x * b.x + a.y * b.y; }
}

int main() {
  geo::Point a;
  a.x = 2;
  a.y = 3;
  geo::Point b;
  b.x = 4;
  b.y = 5;
  if (geo::dot(a, b) != 23) return 1;
  return 0;
}
