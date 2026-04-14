typedef int (*denary_fn)(int, int, int, int, int, int, int, int, int, int);

int call_param(denary_fn f, int u, int v, int w, int x, int y, int z, int q, int r, int s) {
  return f(u, v, w, x, y, z, q, r, s, 10);
}
