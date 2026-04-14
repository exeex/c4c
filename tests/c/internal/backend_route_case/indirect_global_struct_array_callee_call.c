typedef int (*unary_fn)(int);

int inc(int x) { return x + 1; }
int dec(int x) { return x - 1; }

struct Holder {
  int tag;
  unary_fn slots[2];
};

struct Holder holder = {7, {inc, dec}};

int call_global_struct_array(int which, int x) {
  return holder.slots[which](x);
}
