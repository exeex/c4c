// CCC_EXPECT: fail
int main(void) {
  double x = 0x1.8p+1;
  return (x == 3.0) ? 0 : 1;
}

