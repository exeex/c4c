char writeback_data[] = "012345678";

int main(void) {
  char *data = writeback_data;
  unsigned long long offset = 4;
  unsigned lhs = 5;
  unsigned long long rhs = 12;

  *(unsigned *)(data + offset) += lhs - rhs;

  return 0;
}
