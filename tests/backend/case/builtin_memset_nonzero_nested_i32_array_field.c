extern void *memset(void *dst, int byte, unsigned long size);

struct Wrapper {
  int tag;
  int inner[3];
};

int main(void) {
  struct Wrapper dst = {7, {1, 2, 3}};
  memset(dst.inner, 255, sizeof(dst.inner));
  return (dst.tag == 7 && dst.inner[0] == -1 && dst.inner[1] == -1 && dst.inner[2] == -1) ? 0 : 1;
}
