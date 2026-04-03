int main() {
  int x = 1;
  int& y = (int&)x;
  y = 7;
  return x == 7 ? 0 : 1;
}
