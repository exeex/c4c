// Expected: compile fail (incompatible pointer assignment types)
int main(void) {
  int *ip;
  double *dp;
  ip = dp;
  return 0;
}
