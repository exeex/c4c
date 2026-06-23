int main(void) {
  short source[4];
  short sink[4];
  short *from;
  short *to;
  int remaining;

  source[0] = 11;
  source[1] = 22;
  source[2] = 33;
  source[3] = 44;
  sink[0] = 0;
  sink[1] = 0;
  sink[2] = 0;
  sink[3] = 0;

  from = &source[0];
  to = &sink[0];
  remaining = 3;

  do {
    *to++ = *from++;
    remaining = remaining - 1;
  } while (remaining != 0);

  if (sink[0] != 11) {
    return 1;
  }
  if (sink[1] != 22) {
    return 2;
  }
  if (sink[2] != 33) {
    return 3;
  }
  return 0;
}
