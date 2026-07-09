int consume_label_pointer(const char *p) {
  (void)p;
  return 1;
}

int main(void) {
  const char *local = "label";
  return consume_label_pointer(local) - 1;
}
