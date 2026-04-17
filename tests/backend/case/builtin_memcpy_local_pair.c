struct Pair {
  int a;
  int b;
};

int main(void) {
  struct Pair src = {1, 2};
  struct Pair dst;
  __builtin_memcpy(&dst, &src, sizeof(dst));
  return dst.a + dst.b;
}
