struct __attribute__((aligned(16))) QueryBox {
  int data[5];
};

int sizeof_querybox() {
  return sizeof(QueryBox);
}

int alignof_querybox() {
  return alignof(QueryBox);
}

int alignof_expr_querybox() {
  QueryBox box;
  return alignof(box);
}

int main() { return 0; }
