int main(void) {
  short input[5];
  short output[5];
  short *src;
  short *dst;
  int count;
  int rounds;

  input[0] = 3;
  input[1] = 5;
  input[2] = 7;
  input[3] = 11;
  input[4] = 13;
  output[0] = 0;
  output[1] = 0;
  output[2] = 0;
  output[3] = 0;
  output[4] = 0;

  src = input;
  dst = output;
  count = 5;
  rounds = (count + 2) / 3;

  switch (count % 3) {
    case 0:
      do {
        *dst++ = *src++;
    case 2:
        *dst++ = *src++;
    case 1:
        *dst++ = *src++;
      } while (--rounds > 0);
  }

  if (output[0] != 3) {
    return 1;
  }
  if (output[1] != 5) {
    return 2;
  }
  if (output[2] != 7) {
    return 3;
  }
  if (output[3] != 11) {
    return 4;
  }
  if (output[4] != 13) {
    return 5;
  }
  return 0;
}
