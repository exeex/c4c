// Expected: compile fail (pass const pointer to non-const parameter)
int takes_mut(int *p) {
  *p = 7;
  return *p;
}

int main() {
  const int x = 1;
  return takes_mut(&x);
}
