typedef int (*undecenary_fn)(int, int, int, int, int, int, int, int, int, int, int);

int call_param(undecenary_fn f,
               int u,
               int v,
               int w,
               int x,
               int y,
               int z,
               int q,
               int r,
               int s,
               int t) {
  return f(u, v, w, x, y, z, q, r, s, t, 11);
}
