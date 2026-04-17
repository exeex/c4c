struct Pair {
  int a;
  int b;
};

struct Wrapper {
  int tag;
  struct Pair inner;
};

int main(void) {
  struct Wrapper src = {5, {1, 2}};
  struct Wrapper dst = {0, {0, 0}};
  __builtin_memcpy(&dst.inner, &src.inner, sizeof(dst.inner));
  return dst.inner.a + dst.inner.b + dst.tag;
}
