// Expected: compile fail (write through pointer to const)
int main() {
  const int x = 1;
  const int *p = &x;
  *p = 3;
  return x;
}
