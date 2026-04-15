extern void *memset(void *dst, int byte, unsigned long size);

int main(void) {
  int dst = 7;
  memset(&dst, 255, sizeof(dst));
  return dst == -1 ? 0 : 1;
}
