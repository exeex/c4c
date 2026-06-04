char large_stack_publication_source = 'Q';

extern void consume_large_stack_publication(char*);

void large_stack_global_byte_publication(void) {
  char buffer[8200];
  buffer[8112] = large_stack_publication_source;
  consume_large_stack_publication(buffer);
}
