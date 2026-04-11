int global = 5;
typeof(global) global_copy = 7;

int f(void) {
  long local = 2;
  typeof(local) local_copy = 3;
  return (int)(global_copy + local + local_copy);
}

int main(void) {
  int result = f();
  int expect = 12;
  if (result == expect) return 0;
  return -1;
}
