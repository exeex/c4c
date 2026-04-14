// Phase 2: Iterator operator-> for member access on pointed-to struct.
struct Point {
  int x;
  int y;
};

struct PointIter {
  Point* p;

  Point* operator->() {
    return p;
  }

  int operator*() {
    return p->x;
  }

  PointIter operator++() {
    p = p + 1;
    PointIter next;
    next.p = p;
    return next;
  }
};

int main() {
  Point pts[2];
  pts[0].x = 10;
  pts[0].y = 20;
  pts[1].x = 30;
  pts[1].y = 40;

  PointIter it;
  it.p = pts;

  // operator-> for field access
  if (it->x != 10) return 1;
  if (it->y != 20) return 2;

  ++it;
  if (it->x != 30) return 3;
  if (it->y != 40) return 4;

  return 0;
}
