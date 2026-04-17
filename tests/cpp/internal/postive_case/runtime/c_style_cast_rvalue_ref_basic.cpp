int main() {
  int x = 1;
  int&& y = (int&&)x;
  return y == 1 ? 0 : 1;
}
