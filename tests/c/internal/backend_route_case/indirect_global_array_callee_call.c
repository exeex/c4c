typedef int (*unary_fn)(int);

int inc(int x) { return x + 1; }
int dec(int x) { return x - 1; }

unary_fn slots[2] = {inc, dec};

int call_global_array(int which, int x) {
  return slots[which](x);
}
