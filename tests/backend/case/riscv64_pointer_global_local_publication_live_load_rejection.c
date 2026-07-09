short rv64_pointer_global_local_live_sink;

int main(void) {
  short *local;
  short *loaded;

  local = &rv64_pointer_global_local_live_sink;
  loaded = local;

  return *loaded;
}
