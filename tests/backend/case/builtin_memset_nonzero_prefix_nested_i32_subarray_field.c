extern void *memset(void *dst, int byte, unsigned long size);

struct Wrapper {
  int tag;
  int inner[4];
  int tail;
};

int main(void) {
  struct Wrapper dst = {7, {1, 2, 3, 4}, 9};
  memset(&dst.inner[1], 255, sizeof(int) * 2);
  return (dst.tag == 7 && dst.inner[0] == 1 && dst.inner[1] == -1 &&
          dst.inner[2] == -1 && dst.inner[3] == 4 && dst.tail == 9)
             ? 0
             : 1;
}
