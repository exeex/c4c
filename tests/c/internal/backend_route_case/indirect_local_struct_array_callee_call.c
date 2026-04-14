typedef int (*unary_fn)(int);

struct Holder {
  int tag;
  unary_fn slots[2];
};

int call_local_struct_array(unary_fn f, unary_fn g, int which, int x) {
  struct Holder holder;
  holder.tag = x;
  holder.slots[0] = f;
  holder.slots[1] = g;
  return holder.slots[which](x);
}
