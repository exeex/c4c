typedef int (*quinary_fn)(int, int, int, int, int);

int call_local(quinary_fn f, int v, int w, int x, int y, int z) {
  quinary_fn g = f;
  return g(v, w, x, y, z);
}
