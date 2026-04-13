typedef int (*septenary_fn)(int, int, int, int, int, int, int);

int call_local(septenary_fn f, int u, int v, int w, int x, int y, int z, int q) {
  septenary_fn g = f;
  return g(u, v, w, x, y, z, q);
}
