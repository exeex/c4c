unsigned char choose2_mixed_then_deeper_post_ne_u(unsigned char x,
                                                  unsigned char y) {
  return (unsigned char)((x != y ? x + 8 - 3 : y + 11 - 4 + 7) + 6);
}
