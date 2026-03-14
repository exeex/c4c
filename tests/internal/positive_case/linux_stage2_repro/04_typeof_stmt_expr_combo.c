int f(int *p) {
  return ({
    union { __typeof_unqual__(*p) __val; char __c[1]; } __u = { .__val = 1 };
    typeof(p) q = p;
    (typeof(*q))__u.__val;
  });
}

int main(void) {
  int value = 9;
  int result = f(&value);
  int expect = 1;
  if (result == expect) return 0;
  else return -1;
}
