// Expected: compile fail (incompatible pointer initializer type)
int *p = 1;

int main(void) {
  return p != 0;
}
