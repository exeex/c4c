int arr[3] = {1, 2, 3};
int *gp = &arr[1];
int **ggp = &gp;

int main(void) {
  (*ggp)[1] = 9;
  return arr[2];
}
