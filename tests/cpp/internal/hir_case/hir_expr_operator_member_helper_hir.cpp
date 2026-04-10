// HIR regression: overloaded operator/member-expression helpers extracted from
// hir_expr.cpp must preserve implicit operator-bool conversion together with
// operator-> member access.

struct Inner {
  int value;
};

struct Wrapper {
  Inner* ptr;

  operator bool() { return ptr != 0; }
  Inner* operator->() { return ptr; }
};

int main() {
  Inner inner;
  inner.value = 7;

  Wrapper w;
  w.ptr = &inner;

  if (w && w->value == 7) return w->value;
  return 0;
}
