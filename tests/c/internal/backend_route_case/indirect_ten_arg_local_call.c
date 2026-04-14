typedef int (*denary_fn)(int, int, int, int, int, int, int, int, int, int);

int call_local(denary_fn f, int u, int v, int w, int x, int y, int z, int q, int r, int s) {
  denary_fn g = f;
  return g(u, v, w, x, y, z, q, r, s, 10);
}
