struct Parcel {
  int left;
  int right;
  char *tag;
};

int consume_parcel(struct Parcel value, int *mirror) {
  if (value.left != mirror[0])
    return 10;
  if (value.right != mirror[1])
    return 11;
  if (value.tag != (char *)0)
    return 12;
  return value.left + value.right;
}

int main(void) {
  struct Parcel value;
  int mirror[2];

  mirror[0] = 3;
  mirror[1] = 4;
  value.left = 3;
  value.right = 4;
  value.tag = (char *)0;

  if (consume_parcel(value, mirror) != 7)
    return 1;
  return 0;
}
