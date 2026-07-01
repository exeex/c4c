void store_through_pointer_slots(int **first_slot, int **second_slot) {
  **first_slot = 1;
  **second_slot = 6;
}

int main(void) {
  int left;
  int right;
  int *first;
  int *second;

  left = 2;
  right = 5;
  first = &left;
  second = &right;

  store_through_pointer_slots(&first, &second);

  return left + right;
}
