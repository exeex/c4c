struct Pair {
  int a;
  int b;
};

struct SrcWrapper {
  int tag;
  struct Pair inner;
  int tail;
};

struct DstWrapper {
  int head;
  int dst[3];
  int tail;
};

int main(void) {
  struct SrcWrapper src = {7, {3, 5}, 11};
  struct DstWrapper dst = {13, {0, 0, 17}, 19};
  __builtin_memcpy(&dst.dst[1], &src.inner, sizeof(src.inner));
  return src.tag + dst.head + dst.dst[0] + dst.dst[1] + dst.dst[2] + dst.tail + src.tail;
}
