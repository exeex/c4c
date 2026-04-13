typedef int (*binary_fn)(int, int);

int call_local(binary_fn f, int x, int y) {
  binary_fn g = f;
  return g(x, y);
}
