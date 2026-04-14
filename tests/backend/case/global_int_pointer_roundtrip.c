int arr[2] = {4, 9};
int *gp = arr;
int **ggp = &gp;

int main(void) { return (**(int ***)(long)&ggp)[1]; }
