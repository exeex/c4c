struct one_byte {
  char x;
};

struct one_byte global_one_byte = {'7'};

struct one_byte return_global_one_byte(void) {
  return global_one_byte;
}
