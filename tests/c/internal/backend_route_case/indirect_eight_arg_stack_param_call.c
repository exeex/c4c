typedef int (*octonary_fn)(int, int, int, int, int, int, int, int);

int call_param(octonary_fn f, int u, int v, int w, int x, int y, int z, int q, int r) {
  return f(u, v, w, x, y, z, q, r);
}
