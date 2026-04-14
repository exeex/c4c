struct Pair {
  int a;
  int b;
};

struct Pair pairs[2] = {{1, 3}, {2, 5}};
int *gp = &pairs[1].b;
int **gpp = &gp;

int main(void) { return (*gpp)[0]; }
