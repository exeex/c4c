int main(void) {
  short left[39];
  short right[39];
  int i = 0;

  for (; i < 39; i = i + 1) {
    left[i] = (short)(i + 1);
    right[i] = 0;
  }

  short *from = &left[0];
  short *to = &right[0];
  int count = 39;
  int groups = (count + 7) / 8;

  switch (count % 8) {
  case 0:
    *to = *from;
    break;
  case 1:
    to[1] = from[1];
    break;
  case 2:
    to[2] = from[2];
    break;
  case 3:
    to[3] = from[3];
    break;
  case 4:
    to[4] = from[4];
    break;
  case 5:
    to[5] = from[5];
    break;
  case 6:
    to[6] = from[6];
    break;
  default:
    to[7] = from[7];
    break;
  }

  return groups == 5 ? 0 : 1;
}
