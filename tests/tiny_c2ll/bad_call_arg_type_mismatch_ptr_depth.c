// Expected: compile fail (pointer depth mismatch in call argument)
int takes_int_pp(int **pp) {
  return **pp;
}

int main(void) {
  int x = 1;
  int *p = &x;
  return takes_int_pp(p);
}
