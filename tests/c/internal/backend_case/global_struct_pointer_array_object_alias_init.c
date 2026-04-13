int arr[3] = {3, 5, 8};
int *gp = &arr[1];

struct Pair {
  int a;
  int **pp;
};

struct Pair pairs[1] = {{2, &gp}};

int main(void) { return **pairs[0].pp; }
