// CCC_EXPECT: pass
int main(void) {
  const char *s = "\u00E9";
  return ((unsigned char)s[0] == 0xC3 && (unsigned char)s[1] == 0xA9) ? 0 : 1;
}
