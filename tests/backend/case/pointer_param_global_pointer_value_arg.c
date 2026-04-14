int g;
int *gp = &g;

int consume(int *p) { return 0; }

int main(void) {
  int *p = gp;
  return consume(p);
}
