int main(void) {
  int arr[2][4];
  int *p;

  arr[1][3] = 37;
  p = &arr[1][3];

  return *p - 37;
}
