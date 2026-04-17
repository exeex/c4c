struct Wrapper {
  int tag;
  int src[3];
  int dst[3];
};

int main(void) {
  struct Wrapper w = {7, {1, 2, 3}, {0, 0, 0}};
  __builtin_memcpy(w.dst, w.src, sizeof(w.dst));
  return w.tag + w.dst[0] + w.dst[1] + w.dst[2];
}
