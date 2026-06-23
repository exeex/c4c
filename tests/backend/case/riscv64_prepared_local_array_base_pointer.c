int main(void) {
  int arr[2];
  int *p;

  arr[0] = 11;
  arr[1] = 31;
  p = arr;

  return p[0] + p[1] - 42;
}
