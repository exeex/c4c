typedef int (*unary_fn)(int);

int call_local_array(unary_fn f, unary_fn g, int which, int x) {
  unary_fn slots[2];
  slots[0] = f;
  slots[1] = g;
  return slots[which](x);
}
