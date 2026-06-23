struct Link {
  struct Link *next;
  int value;
};

int main(void) {
  struct Link node;

  node.value = 9;
  node.next = &node;

  return node.next->next->next->value - 9;
}
