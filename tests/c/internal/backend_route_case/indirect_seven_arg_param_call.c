typedef int (*septenary_fn)(int, int, int, int, int, int, int);

int call_param(septenary_fn f, int u, int v, int w, int x, int y, int z, int q) {
  return f(u, v, w, x, y, z, q);
}
