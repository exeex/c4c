#include "backend_bir_test_support.hpp"

#include "../../src/backend/bir_printer.hpp"
#include "../../src/backend/lowering/lir_to_bir.hpp"

namespace {

void test_backend_bir_pipeline_selection_only_applies_at_lir_entry_input_riscv64() {
  const auto lir_module = make_bir_return_add_module();
  const auto bir_module = c4c::backend::lower_to_bir(lir_module);
  const auto expected = c4c::backend::bir::print(bir_module);

  const auto rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{bir_module},
                                make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_true(rendered == expected,
              "explicit prelowered BIR emission should stay on the BIR-owned passthrough text surface");
  expect_contains(rendered, "bir.func",
                  "explicit prelowered BIR emission should stay on the BIR text path");
}

void test_backend_lowered_riscv_passthrough_is_detached_from_lir_metadata_riscv64() {
  const auto bir_module = c4c::backend::lower_to_bir(make_bir_return_add_module());
  const auto expected = c4c::backend::bir::print(bir_module);

  const auto rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{bir_module},
                                make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_true(rendered == expected,
              "once callers emit an explicit prelowered RISC-V BIR module, the output should come only from the BIR-owned passthrough text surface");
  expect_contains(rendered, "bir.func",
                  "explicit prelowered RISC-V BIR emission should stay on the BIR text surface");
}

}  // namespace

void run_backend_bir_pipeline_riscv64_tests() {
  RUN_TEST(test_backend_bir_pipeline_selection_only_applies_at_lir_entry_input_riscv64);
  RUN_TEST(test_backend_lowered_riscv_passthrough_is_detached_from_lir_metadata_riscv64);
}
