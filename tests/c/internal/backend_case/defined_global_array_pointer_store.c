int arr[2][2] = {{1, 2}, {3, 7}};

int main(void) {
  int *p = &arr[0][0];
  p[2] = 9;
  return arr[1][0];
}
