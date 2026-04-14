typedef int (*undecenary_fn)(int, int, int, int, int, int, int, int, int, int, int);

int call_local(undecenary_fn f,
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
  undecenary_fn g = f;
  return g(u, v, w, x, y, z, q, r, s, t, 11);
}
