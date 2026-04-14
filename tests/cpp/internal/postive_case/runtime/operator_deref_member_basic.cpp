// Runtime test: operator* member call on a struct (iterator-like dereference).
struct IntPtr {
  int* ptr;

  int operator*() {
    return *ptr;
  }
};

int main() {
  int val = 42;
  IntPtr ip;
  ip.ptr = &val;

  if (*ip != 42) return 1;
  return 0;
}
