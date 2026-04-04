#include "backend_bir_test_support.hpp"

#include "../../src/backend/ir_printer.hpp"
#include "../../src/backend/lowering/lir_to_backend_ir.hpp"
#include "../../src/backend/lowering/lir_to_bir.hpp"

namespace {

void test_backend_bir_pipeline_selection_only_applies_at_lir_entry_input_riscv64() {
  const auto lir_module = make_bir_return_add_module();
  const c4c::backend::BackendModule lowered_backend =
      c4c::backend::lower_lir_to_backend_module(lir_module);
  const auto expected = c4c::backend::print_backend_module(lowered_backend);

  const auto rendered =
      c4c::backend::emit_module(lowered_backend,
                                make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_true(rendered == expected,
              "explicit lowered backend-module emission should stay on the backend-owned passthrough text surface");
  expect_not_contains(rendered, "bir.func",
                      "explicit lowered backend-module emission should not re-enter the BIR text path");
}

void test_backend_lowered_riscv_passthrough_is_detached_from_lir_metadata_riscv64() {
  const auto lowered_backend =
      c4c::backend::lower_lir_to_backend_module(make_bir_return_add_module());
  const auto expected = c4c::backend::print_backend_module(lowered_backend);

  const auto rendered =
      c4c::backend::emit_module(lowered_backend,
                                make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_true(rendered == expected,
              "once callers emit an explicit lowered RISC-V backend module, the output should come only from the backend-owned passthrough text surface");
  expect_not_contains(rendered, "bir.func",
                      "explicit lowered RISC-V backend-module emission should stay off the BIR text surface");
}

}  // namespace

void run_backend_bir_pipeline_riscv64_tests() {
  RUN_TEST(test_backend_bir_pipeline_selection_only_applies_at_lir_entry_input_riscv64);
  RUN_TEST(test_backend_lowered_riscv_passthrough_is_detached_from_lir_metadata_riscv64);
}
