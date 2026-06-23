struct FormalBox {
  int left;
  int right;
  int pad0;
  int pad1;
};

int consume_formal_box(struct FormalBox value, int *mirror, int delta, ...) {
  if (value.left != mirror[0])
    return 10;
  if (value.right != mirror[1])
    return 11;
  return mirror[0] + delta;
}

int main(void) {
  struct FormalBox value;
  int result;

  value.left = 6;
  value.right = 8;
  value.pad0 = 0;
  value.pad1 = 0;

  result = consume_formal_box(value, (int *)&value, 5);
  if (result != 11)
    return 1;
  return 0;
}
