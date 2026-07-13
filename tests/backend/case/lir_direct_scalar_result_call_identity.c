int lir_direct_scalar_result_target(void) {
  return 7;
}

int lir_direct_scalar_result_call_identity(void) {
  return lir_direct_scalar_result_target();
}
