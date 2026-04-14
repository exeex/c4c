int arr[3] = {1, 2, 3};
int *gp = arr;
int *gq = &arr[1];
int **ggp = &gp;
int ***gggp = &ggp;

int main(void) {
  **gggp = gq;
  return gp[1];
}
