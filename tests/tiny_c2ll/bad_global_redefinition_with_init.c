// Expected: compile fail (discarding const qualifier in global pointer initialization)
const int cx = 1;
int *gp = &cx;

int main(void) {
  return 0;
}
