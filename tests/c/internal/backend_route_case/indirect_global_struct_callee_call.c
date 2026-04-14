typedef int (*unary_fn)(int);

int dec(int x) { return x - 1; }

struct Holder {
  int tag;
  unary_fn fn;
};

struct Holder holder = {7, dec};

int call_holder(int x) { return holder.fn(x); }
