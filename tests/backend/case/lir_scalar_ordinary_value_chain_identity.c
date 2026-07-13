int lir_scalar_ordinary_value_chain_source;

int lir_scalar_ordinary_value_chain_identity(void) {
  return (lir_scalar_ordinary_value_chain_source + 1) * 2;
}
