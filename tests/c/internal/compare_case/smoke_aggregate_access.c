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
  struct WithAnon anon_assign = {.left = 0, .right = 0, .low = 1, .high = -6};
  int (*pvalues)[3] = &w.inner.values;
  int (*cursor)[3] = &pair[0].inner.values;
  int (*cursor_set)[3] = &pair[0].inner.values;
  int (*cursor_compound)[3] = &pair[0].inner.values;
  int (*cursor_postdec)[3] = &pair[1].inner.values;
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
  int scalar_assign = (pair[0].tail = 101);
  int scalar_after_assign = pair[0].tail;
  int scalar_compound = (pair[0].tail += 4);
  int scalar_after_compound = pair[0].tail;
  int scalar_postdec = pair[1].tail--;
  int scalar_after_postdec = pair[1].tail;
  int ptr_set = (cursor_set = &pair[1].inner.values)[0][1];
  int ptr_after_set = cursor_set[0][2];
  int ptr_compound = (cursor_compound += 1)[0][2];
  int ptr_after_compound = cursor_compound[0][1];
  int ptr_postdec = (cursor_postdec--)[0][0];
  int ptr_after_postdec = cursor_postdec[0][2];
  int bit_set = (anon_assign.high = -5);
  int bit_add = (anon_assign.high += 2);
  int bit_after_assign = anon_assign.high;
  int low_set = (anon_assign.low = 3);
  int low_xor = (anon_assign.low ^= 6);
  int low_after_assign = anon_assign.low;
  return first + second + third + decay_dot + decay_arrow + ptr_to_array +
         ptr_to_array_add + ptr_to_array_preinc + indexed_member_scalar +
         indexed_member_decay + typedef_arrow + rvalue_field + rvalue_decay +
         pw->tail + anon_rvalue + anon_fields + anon_preinc + anon_postinc +
         anon_final + scalar_assign + scalar_after_assign + scalar_compound +
         scalar_after_compound + scalar_postdec + scalar_after_postdec + ptr_set +
         ptr_after_set + ptr_compound + ptr_after_compound + ptr_postdec +
         ptr_after_postdec + bit_set + bit_add + bit_after_assign + low_set + low_xor +
         low_after_assign;
}

int main(void) {
  return run() == 1369 ? 0 : 1;
}
