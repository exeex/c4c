struct FloatParcel {
  int left;
  float weight;
  int right;
  int tail;
};

int consume_float_parcel(struct FloatParcel value) {
  if (value.left != 5)
    return 10;
  if (value.right != 7)
    return 11;
  if (value.tail != 9)
    return 12;
  return 0;
}
