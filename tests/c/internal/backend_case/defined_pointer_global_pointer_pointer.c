int arr[3] = {4, 7, 9};
int *gp = &arr[1];
int **ggp = &gp;

int main(void) { return (*ggp)[1]; }
