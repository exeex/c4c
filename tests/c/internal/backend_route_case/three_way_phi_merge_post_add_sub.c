int choose3_phi_post_chain(int x, int y, int z) {
  return ((x == y) ? x + 5 : ((x == z) ? y + 9 : z + 13)) + 6 - 2;
}
