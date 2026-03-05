// Expected: compile fail (x is out of scope)
int main() {
  {
    int x = 1;
  }
  return x;
}
