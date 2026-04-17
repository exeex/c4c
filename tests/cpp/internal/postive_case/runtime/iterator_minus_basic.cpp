// Runtime test: operator- for iterator backward stepping.
struct Iter {
  int* ptr;

  Iter operator-(int n) {
    Iter result;
    result.ptr = ptr - n;
    return result;
  }

  int operator*() {
    return *ptr;
  }
};

int main() {
  int arr[5];
  arr[0] = 10;
  arr[1] = 20;
  arr[2] = 30;
  arr[3] = 40;
  arr[4] = 50;

  Iter end;
  end.ptr = arr + 5;

  // operator-(int): step backward
  Iter it = end - 1;
  if (*it != 50) return 1;

  it = end - 3;
  if (*it != 30) return 2;

  it = end - 5;
  if (*it != 10) return 3;

  return 0;
}
