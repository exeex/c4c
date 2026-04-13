int arr[3] = {1, 2, 3};
int *gp = arr;
int *gq = &arr[1];
int **ggp = &gp;

int main(void) {
  *ggp = gq;
  return gp[1];
}
