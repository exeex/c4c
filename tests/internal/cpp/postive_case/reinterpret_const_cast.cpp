// Test: reinterpret_cast<T>(expr) and const_cast<T>(expr) C++ named cast syntax.

int main() {
  // reinterpret_cast: int* -> long (pointer to integer)
  int x = 42;
  long addr = reinterpret_cast<long>(&x);
  if (addr == 0) return 1;  // address should be non-zero

  // reinterpret_cast: long -> int* (integer to pointer)
  int* p = reinterpret_cast<int*>(addr);
  if (*p != 42) return 2;

  // reinterpret_cast: int* -> char* (pointer to pointer)
  char* cp = reinterpret_cast<char*>(&x);
  if (*cp != 42) return 3;  // little-endian: first byte is 42

  // const_cast: remove const from pointer
  const int y = 99;
  const int* cy = &y;
  int* my = const_cast<int*>(cy);
  if (*my != 99) return 4;

  // const_cast: add const to pointer (identity)
  int z = 77;
  const int* cz = const_cast<const int*>(&z);
  if (*cz != 77) return 5;

  return 0;
}
