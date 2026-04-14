int arr[4] = {1, 2, 3, 4};
int *gp = &arr[1];
int *gpp = gp + 1;

int main(void) { return gpp[1]; }
