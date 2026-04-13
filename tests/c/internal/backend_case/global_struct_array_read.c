struct Pair {
  int x;
  int y;
};

struct Pair pairs[2] = {{1, 2}, {3, 4}};

int main(void) {
  return pairs[1].y;
}
