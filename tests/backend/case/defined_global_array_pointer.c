int arr[2][2] = {{1, 2}, {3, 7}};

int main(void) {
  int *p = &arr[0][0];
  return *(p + 3);
}
