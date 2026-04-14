typedef int (*fourteen_arg_fn)(int, int, int, int, int, int, int, int, int, int, int, int, int,
                               int);

int call_param(fourteen_arg_fn f,
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
               int m,
               int n,
               int o) {
  return f(u, v, w, x, y, z, q, r, s, t, m, n, o, 14);
}
