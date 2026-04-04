#include "backend_bir_test_support.hpp"

int main(int argc, char** argv) {
  if (argc > 1) test_filter() = argv[1];

  run_backend_bir_lowering_tests();
  run_backend_bir_pipeline_tests();
  run_backend_bir_pipeline_x86_64_tests();
  run_backend_bir_pipeline_aarch64_tests();
  run_backend_bir_pipeline_riscv64_tests();

  check_failures();
  return 0;
}
