struct PairBox {
  int left;
  int right;
  int pad0;
  int pad1;
};

int consume_pairbox(struct PairBox value, int *mirror) {
  if (value.left != mirror[0])
    return 10;
  if (value.right != mirror[1])
    return 11;
  return value.left + value.right;
}

int main(void) {
  struct PairBox value;
  int first;
  int second;

  value.left = 3;
  value.right = 4;
  value.pad0 = 0;
  value.pad1 = 0;

  first = consume_pairbox(value, (int *)&value);
  second = consume_pairbox(value, (int *)&value);
  if (first != 7)
    return 1;
  if (second != 7)
    return 2;
  return 0;
}
