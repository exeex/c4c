struct Pair {
  int a;
  int b;
};

struct Wrapper {
  int tag;
  int src[3];
};

int main(void) {
  struct Wrapper src = {7, {3, 5, 11}};
  struct Pair dst = {0, 0};
  __builtin_memcpy(&dst, src.src, sizeof(dst));
  return src.tag + dst.a + dst.b + src.src[2];
}
