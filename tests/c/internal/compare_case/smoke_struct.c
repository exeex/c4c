// Compare-mode smoke test: structs, pointers, arrays.
struct Point { int x; int y; };

int dot(struct Point a, struct Point b) {
  return a.x * b.x + a.y * b.y;
}

int sum_array(int *arr, int n) {
  int s = 0;
  for (int i = 0; i < n; i++)
    s += arr[i];
  return s;
}

int main() {
  struct Point p = {3, 4};
  struct Point q = {1, 2};
  int arr[] = {10, 20, 30};
  return dot(p, q) + sum_array(arr, 3) == 71 ? 0 : 1;
}
