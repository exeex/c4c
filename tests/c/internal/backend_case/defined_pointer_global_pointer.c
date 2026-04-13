int arr[3] = {4, 7, 9};
int *gp = &arr[1];
int *gpp = gp;

int main(void) { return gpp[1]; }
