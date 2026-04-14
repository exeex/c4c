int arr[3] = {3, 5, 8};
int *gp = &arr[1];
int **gpp = &gp;

struct S {
  int ***ppp;
};

struct S s = {&gpp};

int main(void) { return (**s.ppp)[1]; }
