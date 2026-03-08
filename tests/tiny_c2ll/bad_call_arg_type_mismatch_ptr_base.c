// Expected: compile fail (pointer base type mismatch in call argument)
int takes_int_ptr(int *p) {
  return *p;
}

int main(void) {
  double d = 3.14;
  return takes_int_ptr(&d);
}
