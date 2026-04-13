typedef int (*senary_fn)(int, int, int, int, int, int);

int call_param(senary_fn f, int u, int v, int w, int x, int y, int z) {
  return f(u, v, w, x, y, z);
}
