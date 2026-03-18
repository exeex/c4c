constexpr int base = 40;
constexpr int offset = 2;
constexpr int answer = base + offset;

int main() {
  int result = answer;
  int expect = 42;
  if (result == expect) return 0;
  else return -1;
}
