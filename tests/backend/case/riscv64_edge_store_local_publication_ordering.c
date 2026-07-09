int rv64_edge_store_d = 1;
int rv64_edge_store_f;
int rv64_edge_store_q;
int rv64_edge_store_w = 1;
short rv64_edge_store_t;

int main(void) {
  char g;

  for (; rv64_edge_store_d; rv64_edge_store_d--) {
    if (rv64_edge_store_t < 1) {
      g = rv64_edge_store_w;
    }
    rv64_edge_store_f = g;
    g && (rv64_edge_store_q = 1);
  }

  return rv64_edge_store_q != 1;
}
