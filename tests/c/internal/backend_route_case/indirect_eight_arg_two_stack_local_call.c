typedef int (*octonary_fn)(int, int, int, int, int, int, int, int);

int call_local(octonary_fn f, int s, int t, int u, int v, int w, int x, int y, int z, int q) {
  octonary_fn g = f;
  return g(t, u, v, w, x, y, z, q);
}
