struct Pair {
  int x;
  int y;
};

struct Pair sink;
struct Pair slots[2];

__attribute__((noinline)) struct Pair make_pair(int value) {
  struct Pair result;
  result.x = value;
  result.y = value + 1;
  return result;
}

int main(void) {
  struct Pair local = {3, 4};
  sink = (struct Pair){1, 2};
  slots[1] = local;
  sink = make_pair(5);
  return sink.y + slots[1].x;
}
