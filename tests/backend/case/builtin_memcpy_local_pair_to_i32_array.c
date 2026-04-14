struct Pair {
  int a;
  int b;
};

int main(void) {
  struct Pair src = {3, 5};
  int dst[2] = {0, 0};
  __builtin_memcpy(dst, &src, sizeof(dst));
  return dst[0] + dst[1];
}
