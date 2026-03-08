// Expected: compile fail (global object has incomplete union type)
union U;
union U u;

int main(void) {
  return 0;
}
