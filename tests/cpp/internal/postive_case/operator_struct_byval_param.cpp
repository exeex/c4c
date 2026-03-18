// Runtime test: operator methods taking struct-by-value parameters.
// Verifies that the injected-class-name typedef is resolved to a proper
// struct type for method parameters, return types, and local declarations.
struct Vec2 {
  int x;
  int y;

  int operator==(Vec2 other) {
    return x == other.x && y == other.y;
  }

  int operator!=(Vec2 other) {
    return x != other.x || y != other.y;
  }

  Vec2 operator+(Vec2 other) {
    Vec2 r;
    r.x = x + other.x;
    r.y = y + other.y;
    return r;
  }

  Vec2 operator-(Vec2 other) {
    Vec2 r;
    r.x = x - other.x;
    r.y = y - other.y;
    return r;
  }
};

int main() {
  Vec2 a;
  a.x = 1; a.y = 2;
  Vec2 b;
  b.x = 1; b.y = 2;
  Vec2 c;
  c.x = 3; c.y = 4;

  // operator== with struct-by-value arg
  if (!(a == b)) return 1;
  if (a == c) return 2;

  // operator!= with struct-by-value arg
  if (a != b) return 3;
  if (!(a != c)) return 4;

  // operator+ returning struct by value, with local Vec2 decl
  Vec2 d = a + c;
  if (d.x != 4 || d.y != 6) return 5;

  // operator- returning struct by value
  Vec2 e = c - a;
  if (e.x != 2 || e.y != 2) return 6;

  // chained: (a + c) == d
  Vec2 f = a + c;
  if (!(f == d)) return 7;

  return 0;
}
