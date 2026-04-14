typedef int (*unary_fn)(int);

struct Holder {
  int tag;
  unary_fn fn;
};

int call_local_struct(unary_fn f, int x) {
  struct Holder holder;
  holder.tag = x;
  holder.fn = f;
  return holder.fn(x);
}
