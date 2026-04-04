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

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered_backend, &lir_module},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_true(rendered == expected,
              "once callers provide a lowered backend module, backend pipeline selection should stop at the LIR entry seam");
  expect_not_contains(rendered, "bir.func",
                      "pre-lowered backend-module entry should not re-enter the BIR text path");
}

void test_backend_lowered_riscv_passthrough_ignores_broken_legacy_fallback_riscv64() {
  auto legacy_module = make_bir_return_add_module();
  const c4c::backend::BackendModule lowered_backend =
      c4c::backend::lower_lir_to_backend_module(legacy_module);
  const auto expected = c4c::backend::print_backend_module(lowered_backend);

  for (auto& function : legacy_module.functions) {
    if (function.name == "main") {
      function.name = "legacy_main_broken";
      function.signature_text = "define i32 @legacy_main_broken()\n";
      break;
    }
  }

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered_backend, &legacy_module},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_true(rendered == expected,
              "once callers provide a lowered RISC-V backend module, malformed attached legacy fallback metadata should not override the backend-owned passthrough text surface");
  expect_not_contains(rendered, "define i32 @legacy_main_broken()",
                      "pre-lowered RISC-V backend-module emission should ignore broken legacy fallback LLVM text");
}

}  // namespace

void run_backend_bir_pipeline_riscv64_tests() {
  RUN_TEST(test_backend_bir_pipeline_selection_only_applies_at_lir_entry_input_riscv64);
  RUN_TEST(test_backend_lowered_riscv_passthrough_ignores_broken_legacy_fallback_riscv64);
}
