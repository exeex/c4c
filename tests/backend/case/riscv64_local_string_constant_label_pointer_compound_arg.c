struct payload {
  char *text;
  int tag;
};

extern void consume_payload(struct payload *);

void pass_compound_literal(void) {
  consume_payload(&(struct payload){"hi", 1});
}
