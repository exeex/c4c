extern void *memset(void *dst, int byte, unsigned long size);

struct Inner {
  int value;
  int tail;
};

struct Wrapper {
  int tag;
  struct Inner inner;
};

int main(void) {
  struct Wrapper dst = {7, {5, 9}};
  memset(&dst.inner.value, 255, sizeof(dst.inner.value));
  return (dst.tag == 7 && dst.inner.value == -1 && dst.inner.tail == 9) ? 0 : 1;
}
