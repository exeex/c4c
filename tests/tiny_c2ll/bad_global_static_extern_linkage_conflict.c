// Expected: compile fail (extern declaration has initializer with undeclared symbol)
extern int ext = missing_value;

int main(void) {
  return ext;
}
