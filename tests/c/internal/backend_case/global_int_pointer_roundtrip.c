int g_value = 9;

int main(void) {
  long addr = (long)&g_value;
  int *p = (int *)addr;
  return *p;
}
