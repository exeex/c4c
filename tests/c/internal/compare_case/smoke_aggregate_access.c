// Compare-mode smoke test: aggregate member/index access and decay paths.

struct Buffer {
  int head;
  int values[3];
};

struct Wrapper {
  struct Buffer inner;
  int tail;
};

typedef struct Wrapper WrapperAlias;
typedef WrapperAlias *WrapperAliasPtr;

int sum3(int *ptr) {
  return ptr[0] + ptr[1] + ptr[2];
}

struct Wrapper make_wrapper(void) {
  struct Wrapper w = {{2, {3, 5, 7}}, 11};
  return w;
}

int run(void) {
  struct Wrapper w = {{5, {7, 11, 13}}, 17};
  struct Wrapper pair[2] = {{{19, {23, 29, 31}}, 37},
                            {{41, {43, 47, 53}}, 59}};
  struct Wrapper *pw = &w;
  WrapperAliasPtr pw_alias = pw;
  int (*pvalues)[3] = &w.inner.values;
  int (*cursor)[3] = &pair[0].inner.values;
  int first = w.inner.head;
  int second = pw->inner.values[1];
  int third = w.inner.values[2];
  int decay_dot = sum3(w.inner.values);
  int decay_arrow = sum3(pw->inner.values);
  int ptr_to_array = pvalues[0][0] + pvalues[0][2];
  int ptr_to_array_add = (cursor + 1)[0][1];
  int ptr_to_array_preinc = (++cursor)[0][0];
  int typedef_arrow = pw_alias->inner.head;
  int rvalue_field = make_wrapper().inner.values[1];
  int rvalue_decay = sum3(make_wrapper().inner.values);
  return first + second + third + decay_dot + decay_arrow + ptr_to_array +
         ptr_to_array_add + ptr_to_array_preinc + typedef_arrow + rvalue_field +
         rvalue_decay + pw->tail;
}

int main(void) {
  return run() == 219 ? 0 : 1;
}
