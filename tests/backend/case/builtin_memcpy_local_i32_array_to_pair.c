struct Pair {
  int a;
  int b;
};

int main(void) {
  int src[2] = {3, 5};
  struct Pair dst = {0, 0};
  __builtin_memcpy(&dst, src, sizeof(dst));
  return dst.a + dst.b;
}
