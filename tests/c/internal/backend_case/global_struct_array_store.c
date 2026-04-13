struct Pair {
  int x;
  int y;
};

struct Pair pairs[2];

int main(void) {
  pairs[1].x = 9;
  return pairs[1].x;
}
