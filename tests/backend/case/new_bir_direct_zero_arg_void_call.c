void direct_zero_arg_void_target(void);

void direct_zero_arg_void_caller(void) {
  direct_zero_arg_void_target();
}

void direct_zero_arg_void_target(void) {}
