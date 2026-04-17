int g;
int *gp = &g;

int *id_ptr(int *p) { return p; }

int *use(void) {
  int *p = gp;
  return id_ptr(p);
}

int main(void) { return 0; }
