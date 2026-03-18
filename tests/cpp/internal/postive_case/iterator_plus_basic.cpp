// Runtime test: operator+ and operator- for iterator stepping.
struct Iter {
  int* ptr;

  Iter operator+(int n) {
    Iter result;
    result.ptr = ptr + n;
    return result;
  }

  Iter operator-(int n) {
    Iter result;
    result.ptr = ptr - n;
    return result;
  }

  int operator*() {
    return *ptr;
  }

  bool operator!=(Iter other) {
    return ptr != other.ptr;
  }
};

int main() {
  int arr[5];
  arr[0] = 10;
  arr[1] = 20;
  arr[2] = 30;
  arr[3] = 40;
  arr[4] = 50;

  Iter it;
  it.ptr = arr;

  // operator+
  Iter it2 = it + 2;
  if (*it2 != 30) return 1;

  Iter it3 = it + 4;
  if (*it3 != 50) return 2;

  // operator-
  Iter it4 = it3 - 2;
  if (*it4 != 30) return 3;

  Iter it5 = it3 - 4;
  if (*it5 != 10) return 4;

  // chained stepping
  Iter it6 = it + 1;
  if (*it6 != 20) return 5;

  return 0;
}
