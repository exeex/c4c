// Phase 4: operator-> chaining — a->field where a.operator->() returns a class
// with its own operator->(), which returns a pointer.
// C++ chains operator-> until a raw pointer is obtained.
struct Inner {
  int x;
  int y;
};

struct Wrapper {
  Inner* ptr;

  Inner* operator->() {
    return ptr;
  }
};

struct Outer {
  Wrapper w;

  Wrapper operator->() {
    return w;
  }
};

int main() {
  Inner inner;
  inner.x = 42;
  inner.y = 99;

  // Single level: Wrapper -> Inner*
  Wrapper w;
  w.ptr = &inner;
  if (w->x != 42) return 1;
  if (w->y != 99) return 2;

  // Chained: Outer.operator->() returns Wrapper (class),
  // then Wrapper.operator->() returns Inner* (pointer).
  Outer o;
  o.w.ptr = &inner;
  if (o->x != 42) return 3;
  if (o->y != 99) return 4;

  // Modify through chain
  inner.x = 100;
  if (o->x != 100) return 5;

  return 0;
}
