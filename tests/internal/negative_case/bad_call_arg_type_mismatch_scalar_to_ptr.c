// Expected: compile fail (scalar passed where pointer is required)
int takes_int_ptr(int *p) {
  return *p;
}

int main(void) {
  return takes_int_ptr(1);
}
