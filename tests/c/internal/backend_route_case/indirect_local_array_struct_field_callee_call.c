typedef int (*unary_fn)(int);

struct Holder {
  int tag;
  unary_fn slot;
};

int call_local_array_struct_field(unary_fn f, unary_fn g, int which, int x) {
  struct Holder holders[2];
  holders[0].tag = x;
  holders[0].slot = f;
  holders[1].tag = x + 1;
  holders[1].slot = g;
  return holders[which].slot(x);
}
