int dynamic_stack_fixed_slot_uses_fp_anchor(int n) {
  int fixed = n;
  int total = 0;

  {
    int xs[n];
    xs[0] = fixed;
    total = total + xs[0];
  }

  fixed = fixed + 1;
  return total + fixed;
}
