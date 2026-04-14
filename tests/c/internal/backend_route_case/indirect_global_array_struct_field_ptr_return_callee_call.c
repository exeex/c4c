typedef int *(*ptr_ret_binary_fn)(int *, int);

struct Holder {
  int tag;
  ptr_ret_binary_fn slot;
};

int *keep(int *p, int x);
int *bump(int *p, int x);

struct Holder holders[2] = {{0, keep}, {1, bump}};

int *call_global_array_struct_field_ptr_return(int which, int *p, int x) {
  return holders[which].slot(p, x);
}
