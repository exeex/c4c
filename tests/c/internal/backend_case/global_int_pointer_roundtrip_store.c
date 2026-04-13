int arr[2] = {4, 9};
int *gp = arr;
int *gq = &arr[1];

int main(void) {
  *(int **)(long)&gp = gq;
  return gp[0];
}
