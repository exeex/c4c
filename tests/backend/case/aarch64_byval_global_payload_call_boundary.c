struct Pair {
  unsigned char first;
  unsigned char second;
};

extern void take_pair(struct Pair value);

struct Pair global_pair = {0x12, 0x34};

void call_pair(void) {
  take_pair(global_pair);
}
