int main(void) {
  int i;
  int left[16];
  int right[16];

  for (i = 0; i < 16; i++) {
    left[i] = i;
    right[i] = 0;
  }

  switch (i % 3) {
    case 0:
      right[0] = left[0];
      break;
    case 1:
      right[1] = left[1];
      break;
    default:
      right[2] = left[2];
      break;
  }

  return (right[0] + right[1] + right[2]) == 1 ? 0 : 1;
}
