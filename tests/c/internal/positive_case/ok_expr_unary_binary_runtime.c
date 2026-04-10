int bump(int *value) {
  *value += 1;
  return *value;
}

int main(void) {
  int value = 5;
  int arr[3] = {10, 20, 30};
  int *ptr = arr;
  unsigned char narrow = 28;
  int side = 0;
  __complex__ double x = 1.0 + 2.0i;
  __complex__ double y = 3.0 - 1.0i;
  __complex__ double sum = x + y;
  __complex__ double prod = x * y;

  if ((+value) != 5) return 1;
  if ((-value) != -5) return 2;
  if ((~value) != -6) return 3;
  if ((!0) != 1 || (!value) != 0) return 4;

  if (++value != 6 || value != 6) return 5;
  if (value++ != 6 || value != 7) return 6;

  if (*(ptr + 1) != 20) return 7;
  if (((ptr + 2) - ptr) != 2) return 8;

  if ((0 && bump(&side)) != 0 || side != 0) return 9;
  if ((1 || bump(&side)) != 1 || side != 0) return 10;
  if ((1 && bump(&side)) != 1 || side != 1) return 11;

  if ((narrow >>= 2) != 7 || narrow != 7) return 12;
  if ((narrow *= 3) != 21 || narrow != 21) return 13;

  if (__real__ sum != 4.0 || __imag__ sum != 1.0) return 14;
  if (__real__ prod != 5.0 || __imag__ prod != 5.0) return 15;
  if ((x == x) != 1 || (x == y) != 0 || (x != y) != 1) return 16;

  return 0;
}
