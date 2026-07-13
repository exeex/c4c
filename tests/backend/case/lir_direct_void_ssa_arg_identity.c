int lir_direct_void_ssa_arg_source;

void lir_direct_void_ssa_arg_target(int value);

void lir_direct_void_ssa_arg_identity(void) {
  lir_direct_void_ssa_arg_target(lir_direct_void_ssa_arg_source);
}
