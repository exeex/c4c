extern void *memset(void *dst, int byte, unsigned long size);

int main(void) {
  int dst[3] = {1, 2, 3};
  memset(dst, 0, sizeof(dst));
  return dst[0] + dst[1] + dst[2];
}
