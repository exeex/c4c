// Parse test: operator+ and operator- declarations.
struct Vec2 {
  int x;
  int y;

  Vec2 operator+(Vec2 other) {
    Vec2 res{x + other.x, y + other.y};
    return res;
  }

  Vec2 operator-(Vec2 other) {
    Vec2 res{x - other.x, y - other.y};
    return res;
  }
};

int main() {
  return 0;
}
