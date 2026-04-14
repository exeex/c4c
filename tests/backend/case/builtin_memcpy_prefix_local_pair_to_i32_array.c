struct Pair {
  int a;
  int b;
};

struct Wrapper {
  int tag;
  int dst[3];
};

int main(void) {
  struct Pair src = {3, 5};
  struct Wrapper dst = {7, {0, 0, 11}};
  __builtin_memcpy(dst.dst, &src, sizeof(src));
  return dst.tag + dst.dst[0] + dst.dst[1] + dst.dst[2];
}
