typedef int (*nonary_fn)(int, int, int, int, int, int, int, int, int);

int call_param(nonary_fn f, int u, int v, int w, int x, int y, int z, int q, int r) {
  return f(u, v, w, x, y, z, q, r, 9);
}
