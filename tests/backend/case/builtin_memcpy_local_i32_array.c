int main(void) {
  int src[3] = {4, 5, 6};
  int dst[3] = {0, 0, 0};
  __builtin_memcpy(dst, src, sizeof(dst));
  return dst[0] + dst[1] + dst[2];
}
