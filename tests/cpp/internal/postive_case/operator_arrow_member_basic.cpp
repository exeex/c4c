// Runtime test: operator-> member call on a struct (smart pointer pattern).
struct Inner {
  int x;
  int y;
};

struct Wrapper {
  Inner obj;

  Inner* operator->() {
    return &obj;
  }
};

int main() {
  Wrapper w;
  w.obj.x = 10;
  w.obj.y = 20;
  if (w->x != 10) return 1;
  if (w->y != 20) return 2;
  w->x = 30;
  if (w.obj.x != 30) return 3;
  return 0;
}
