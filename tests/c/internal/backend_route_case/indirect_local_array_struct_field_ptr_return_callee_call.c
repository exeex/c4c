typedef int *(*ptr_ret_binary_fn)(int *, int);

struct Holder {
  int tag;
  ptr_ret_binary_fn slot;
};

int *call_local_array_struct_field_ptr_return(ptr_ret_binary_fn f,
                                              ptr_ret_binary_fn g,
                                              int which,
                                              int *p,
                                              int x) {
  struct Holder holders[2];
  holders[0].tag = x;
  holders[0].slot = f;
  holders[1].tag = x + 1;
  holders[1].slot = g;
  return holders[which].slot(p, x);
}
