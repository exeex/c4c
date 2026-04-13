int arr[2] = {4, 7};
int *gp = arr;

int main(void) {
  gp[1] = 9;
  return arr[1];
}
