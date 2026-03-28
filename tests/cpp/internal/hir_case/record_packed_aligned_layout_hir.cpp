struct __attribute__((packed)) LayoutProbe {
  char a;
  int b;
  int data[2];
} __attribute__((aligned(8)));

int main() { return 0; }
