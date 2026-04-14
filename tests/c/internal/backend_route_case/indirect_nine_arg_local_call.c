typedef int (*nonary_fn)(int, int, int, int, int, int, int, int, int);

int call_local(nonary_fn f, int u, int v, int w, int x, int y, int z, int q, int r) {
  nonary_fn g = f;
  return g(u, v, w, x, y, z, q, r, 9);
}
