extern void *memset(void *dst, int byte, unsigned long size);

typedef struct {
  int x;
  int y;
} Pair;

int main(void) {
  Pair dst = {1, 2};
  memset(&dst, 0, sizeof(dst));
  return dst.x + dst.y;
}
