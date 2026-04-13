typedef int (*quaternary_fn)(int, int, int, int);

int call_local(quaternary_fn f, int w, int x, int y, int z) {
  quaternary_fn g = f;
  return g(w, x, y, z);
}
