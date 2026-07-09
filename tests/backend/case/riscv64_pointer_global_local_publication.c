short rv64_pointer_global_local_sink;

int main(void) {
  short *local;

  local = &rv64_pointer_global_local_sink;
  *local = 7;

  return rv64_pointer_global_local_sink;
}
