// Compare-mode smoke test: aggregate member/index access and decay paths.

struct Buffer {
  int head;
  int values[3];
};

struct Wrapper {
  struct Buffer inner;
  int tail;
};

struct WithAnon {
  struct {
    int left;
    int right;
  };
  struct {
    unsigned low : 3;
    signed high : 5;
  };
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

struct WithAnon make_anon(void) {
  struct WithAnon v = {.left = 71, .right = 73, .low = 6, .high = -4};
  return v;
}

int run(void) {
  struct Wrapper w = {{5, {7, 11, 13}}, 17};
  struct Wrapper pair[2] = {{{19, {23, 29, 31}}, 37},
                            {{41, {43, 47, 53}}, 59}};
  struct Wrapper *pw = &w;
  WrapperAliasPtr pw_alias = pw;
  struct WithAnon anon = {.left = 61, .right = 67, .low = 5, .high = -3};
  struct WithAnon *anon_ptr = &anon;
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
  int indexed_member_scalar = pair[1].inner.values[0];
  int indexed_member_decay = sum3(pair[1].inner.values);
  int typedef_arrow = pw_alias->inner.head;
  int rvalue_field = make_wrapper().inner.values[1];
  int rvalue_decay = sum3(make_wrapper().inner.values);
  int anon_rvalue = make_anon().right + make_anon().low + make_anon().high;
  int anon_fields = anon.left + anon_ptr->right + anon.low + anon.high;
  int anon_preinc = ++anon.high;
  int anon_postinc = anon.high++;
  int anon_final = anon.high;
  return first + second + third + decay_dot + decay_arrow + ptr_to_array +
         ptr_to_array_add + ptr_to_array_preinc + indexed_member_scalar +
         indexed_member_decay + typedef_arrow + rvalue_field + rvalue_decay +
         pw->tail + anon_rvalue + anon_fields + anon_preinc + anon_postinc +
         anon_final;
}

int main(void) {
  return run() == 562 ? 0 : 1;
}
