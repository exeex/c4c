// CCC_EXPECT: fail
int main(void) {
  int c = L'\u03B1';
  return (c == 945) ? 0 : 1;
}
