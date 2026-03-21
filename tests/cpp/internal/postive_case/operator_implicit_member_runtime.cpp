// Runtime test: implicit member access (unqualified field name) in
// out-of-class method bodies resolves as this->field.
struct Point {
  int x;
  int y;

  int sum() const;
  void scale(int factor);
};

int Point::sum() const {
  // 'x' and 'y' are unqualified — should resolve as this->x, this->y.
  return x + y;
}

void Point::scale(int factor) {
  x = x * factor;
  y = y * factor;
}

int main() {
  Point p;
  p.x = 3;
  p.y = 7;
  if (p.sum() != 10) return 1;

  p.scale(2);
  if (p.x != 6) return 2;
  if (p.y != 14) return 3;
  if (p.sum() != 20) return 4;

  return 0;
}
