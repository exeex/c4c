typedef int (*senary_fn)(int, int, int, int, int, int);

int call_local(senary_fn f, int u, int v, int w, int x, int y, int z) {
  senary_fn g = f;
  return g(u, v, w, x, y, z);
}
