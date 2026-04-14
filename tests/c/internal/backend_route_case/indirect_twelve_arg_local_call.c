typedef int (*duodenary_fn)(int, int, int, int, int, int, int, int, int, int, int, int);

int call_local(duodenary_fn f,
               int u,
               int v,
               int w,
               int x,
               int y,
               int z,
               int q,
               int r,
               int s,
               int t,
               int m) {
  duodenary_fn g = f;
  return g(u, v, w, x, y, z, q, r, s, t, m, 12);
}
