int arr[4] = {3, 5, 8, 13};
int *gp = &arr[1];
int **gpp = &gp;

struct Pair {
  int a;
  int ***ppp;
};

struct Pair pairs[2] = {{1, &gpp}, {2, &gpp}};

int main(void) { return (**pairs[1].ppp)[1]; }
