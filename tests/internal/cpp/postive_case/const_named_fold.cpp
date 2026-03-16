const int x = 4;
const int y = 5;
const int z = x + y;

int main() {
  int result = z;
  int expect = 9;
  if (result == expect) return 0;
  else return -1;
}
