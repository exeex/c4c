extern void *memset(void *dst, int byte, unsigned long size);

int main(void) {
  int dst[5] = {1, 2, 3, 4, 9};
  memset(&dst[1], 255, sizeof(int) * 2);
  return (dst[0] == 1 && dst[1] == -1 && dst[2] == -1 && dst[3] == 4 &&
          dst[4] == 9)
             ? 0
             : 1;
}
