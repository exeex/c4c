struct Pair {
  int a;
  int b;
};

struct SrcWrapper {
  int tag;
  int src[4];
};

struct DstWrapper {
  int pad[2];
  struct Pair inner;
  int tail;
};

int main(void) {
  struct SrcWrapper src = {7, {3, 5, 11, 13}};
  struct DstWrapper dst = {{17, 19}, {0, 0}, 23};
  __builtin_memcpy(&dst.inner, &src.src[1], sizeof(dst.inner));
  return src.tag + dst.pad[0] + dst.pad[1] + dst.inner.a + dst.inner.b + dst.tail;
}
