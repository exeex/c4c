int main(void) {
  int arr[2];
  int *p;

  arr[0] = 5;
  arr[1] = 9;

  p = &arr[0];
  if (*(p++) != 5) {
    return 1;
  }
  if (*p != 9) {
    return 2;
  }

  p = &arr[1];
  if (*(--p) != 5) {
    return 3;
  }

  p = &arr[0];
  if (*(p--) != 5) {
    return 4;
  }

  return 0;
}
