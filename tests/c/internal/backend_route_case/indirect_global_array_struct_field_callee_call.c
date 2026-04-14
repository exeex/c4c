typedef int (*unary_fn)(int);

struct Holder {
  unary_fn slot;
  int pad;
};

int inc(int x) { return x + 1; }
int dec(int x) { return x - 1; }

struct Holder holders[2] = {{inc, 0}, {dec, 0}};

int call_global_array_struct_field(int which, int x) {
  return holders[which].slot(x);
}
