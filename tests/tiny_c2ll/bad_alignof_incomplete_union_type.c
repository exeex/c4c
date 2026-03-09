// Expected: compile fail (_Alignof on incomplete union type)
union U;

int main(void) {
  return _Alignof(union U);
}
