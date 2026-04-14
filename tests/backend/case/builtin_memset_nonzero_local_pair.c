extern void *memset(void *dst, int byte, unsigned long size);

typedef struct {
  int x;
  int y;
} Pair;

int main(void) {
  Pair dst = {1, 2};
  memset(&dst, 255, sizeof(dst));
  return (dst.x == -1 && dst.y == -1) ? 0 : 1;
}
