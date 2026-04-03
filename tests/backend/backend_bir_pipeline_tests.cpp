#include "backend_bir_test_support.hpp"

#include "../../src/backend/ir_printer.hpp"
#include "../../src/backend/lowering/lir_to_backend_ir.hpp"
#include "../../src/backend/lowering/lir_to_bir.hpp"

namespace {

void test_backend_default_path_remains_legacy_when_bir_pipeline_is_not_selected() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_add_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Riscv64});

  expect_contains(rendered, "define i32 @main()",
                  "default backend flow should keep returning legacy LLVM text when the BIR path is not selected");
  expect_not_contains(rendered, "bir.func",
                      "default backend flow should not silently switch into the BIR scaffold");
}

void test_backend_bir_pipeline_is_opt_in_through_backend_options() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit backend options should select the BIR pipeline");
  expect_contains(rendered, "%t0 = bir.add i32 2, 3",
                  "explicit BIR selection should route the tiny add slice through the BIR lowering seam");
}

void test_backend_bir_pipeline_accepts_direct_bir_module_input() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_return_immediate_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @tiny_ret() -> i32 {",
                  "backend input should accept a direct BIR module without forcing a legacy LIR wrapper");
  expect_contains(rendered, "bir.ret i32 7",
                  "direct BIR input should flow through the BIR text surface unchanged");
}

void test_backend_bir_pipeline_routes_sub_cluster_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should keep the narrow sub slice on the BIR path");
  expect_contains(rendered, "%t0 = bir.sub i32 3, 3",
                  "explicit BIR selection should expose sub through the BIR-specific route surface");
}

void test_backend_bir_pipeline_routes_straight_line_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "%t0 = bir.add i32 2, 3",
                  "explicit BIR selection should preserve the head of the straight-line arithmetic chain");
  expect_contains(rendered, "%t1 = bir.sub i32 %t0, 1",
                  "explicit BIR selection should let later chain instructions consume prior BIR values");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should expose the chain tail on the BIR route surface");
}

void test_backend_bir_pipeline_routes_zero_param_staged_constant_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_staged_constant_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the zero-parameter staged constant signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.add i32 2, 3",
                  "explicit BIR selection should expose the staged constant head on the BIR text path");
  expect_contains(rendered, "%t1 = bir.sub i32 %t0, 1",
                  "explicit BIR selection should preserve the middle subtraction of the staged constant chain");
  expect_contains(rendered, "%t2 = bir.add i32 %t1, 4",
                  "explicit BIR selection should preserve the tail add of the staged constant chain");
}

void test_backend_bir_pipeline_routes_mul_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_mul_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny mul signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.mul i32 6, 7",
                  "explicit BIR selection should expose the widened mul slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the mul result on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @tiny_char(i8 %p.x) -> i8 {",
                  "explicit BIR selection should preserve widened i8 function signatures on the BIR text path");
  expect_contains(rendered, "%t0 = bir.add i8 %p.x, 2",
                  "explicit BIR selection should route i8 arithmetic through the BIR text surface");
  expect_contains(rendered, "%t1 = bir.sub i8 %t0, 1",
                  "explicit BIR selection should preserve the trailing i8 adjustment on the BIR text path");
}

void test_backend_bir_pipeline_routes_i64_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i64_return_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @wide_add(i64 %p.x) -> i64 {",
                  "explicit BIR selection should preserve widened i64 function signatures on the BIR text path");
  expect_contains(rendered, "%t0 = bir.add i64 %p.x, 2",
                  "explicit BIR selection should route i64 arithmetic through the BIR text surface");
  expect_contains(rendered, "%t1 = bir.sub i64 %t0, 1",
                  "explicit BIR selection should preserve the trailing i64 adjustment on the BIR text path");
}

void test_backend_bir_pipeline_routes_single_param_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_single_param_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @add_one(i32 %p.x) -> i32 {",
                  "explicit BIR selection should preserve the bounded one-parameter signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.add i32 %p.x, 2",
                  "explicit BIR selection should route parameter-fed arithmetic through the BIR surface");
  expect_contains(rendered, "%t1 = bir.sub i32 %t0, 1",
                  "explicit BIR selection should keep the bounded parameter-fed chain in BIR text form");
}

void test_backend_bir_pipeline_routes_two_param_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @add_pair(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the bounded two-parameter signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.add i32 %p.x, %p.y",
                  "explicit BIR selection should route two-parameter affine arithmetic through the BIR surface");
}

void test_backend_bir_pipeline_routes_two_param_add_sub_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @add_pair(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the bounded two-parameter signature for the affine chain");
  expect_contains(rendered, "%t0 = bir.add i32 %p.x, %p.y",
                  "explicit BIR selection should keep the two-parameter add head of the bounded affine chain on the BIR text path");
  expect_contains(rendered, "%t1 = bir.sub i32 %t0, 1",
                  "explicit BIR selection should expose the trailing immediate adjustment on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_staged_affine_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_staged_affine_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @add_pair(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the two-parameter signature for the staged affine chain");
  expect_contains(rendered, "%t0 = bir.add i32 %p.x, 2",
                  "explicit BIR selection should expose the staged immediate head on the BIR text path");
  expect_contains(rendered, "%t1 = bir.add i32 %t0, %p.y",
                  "explicit BIR selection should keep the later second-parameter add on the BIR text path");
  expect_contains(rendered, "%t2 = bir.sub i32 %t1, 1",
                  "explicit BIR selection should expose the trailing staged subtraction on the BIR text path");
}

void test_backend_bir_pipeline_drives_x86_return_add_smoke_case_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "BIR pipeline should still reach x86 backend emission for the tiny scaffold case");
  expect_contains(rendered, "mov eax, 5",
                  "BIR pipeline should lower the tiny add/return case to the expected x86 return-immediate asm");
}

void test_backend_bir_pipeline_drives_x86_return_sub_smoke_case_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "BIR pipeline should still reach x86 backend emission for the tiny sub scaffold case");
  expect_contains(rendered, "mov eax, 0",
                  "BIR pipeline should lower the tiny sub/return case to the expected x86 return-immediate asm");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_zero_param_staged_constant_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_bir_return_staged_constant_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "direct BIR zero-parameter staged constant input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov eax, 8",
                  "x86 direct BIR zero-parameter staged constant input should collapse the full chain to the surviving constant");
}

void test_backend_bir_pipeline_drives_x86_single_param_chain_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_single_param_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl add_one",
                  "BIR pipeline should still reach x86 backend emission for the bounded one-parameter slice");
  expect_contains(rendered, "mov eax, edi",
                  "x86 BIR lowering should seed the result from the incoming integer argument register");
  expect_contains(rendered, "add eax, 1",
                  "x86 BIR lowering should collapse the bounded parameter-fed chain into the expected affine adjustment");
}

void test_backend_bir_pipeline_drives_aarch64_single_param_chain_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_single_param_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl add_one",
                  "BIR pipeline should still reach aarch64 backend emission for the bounded one-parameter slice");
  expect_contains(rendered, "add w0, w0, #1",
                  "aarch64 BIR lowering should collapse the bounded parameter-fed chain into the expected affine adjustment");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_zero_param_staged_constant_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_bir_return_staged_constant_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl main",
                  "direct BIR zero-parameter staged constant input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov w0, #8",
                  "aarch64 direct BIR zero-parameter staged constant input should collapse the full chain to the surviving constant");
}

void test_backend_bir_pipeline_drives_x86_two_param_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl add_pair",
                  "BIR pipeline should still reach x86 backend emission for the bounded two-parameter slice");
  expect_contains(rendered, "mov eax, edi",
                  "x86 BIR lowering should seed the result from the first incoming integer argument register");
  expect_contains(rendered, "add eax, esi",
                  "x86 BIR lowering should combine the second integer argument register for the bounded two-parameter add");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_input_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{c4c::backend::lower_to_bir(make_bir_two_param_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl add_pair",
                  "direct BIR module input should reach x86 backend emission without a caller-owned legacy backend module");
  expect_contains(rendered, "mov eax, edi",
                  "x86 direct BIR input should still seed the result from the first incoming integer argument register");
  expect_contains(rendered, "add eax, esi",
                  "x86 direct BIR input should still combine the second integer argument register on the backend path");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_staged_affine_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_bir_two_param_staged_affine_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl add_pair",
                  "direct BIR staged affine input should reach x86 backend emission without caller-owned legacy backend IR");
  expect_contains(rendered, "mov eax, edi",
                  "x86 direct BIR staged affine input should still seed from the first integer argument register");
  expect_contains(rendered, "add eax, esi",
                  "x86 direct BIR staged affine input should still combine the second integer argument register");
  expect_contains(rendered, "add eax, 1",
                  "x86 direct BIR staged affine input should preserve the collapsed affine constant");
}

void test_backend_bir_pipeline_drives_aarch64_two_param_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl add_pair",
                  "BIR pipeline should still reach aarch64 backend emission for the bounded two-parameter slice");
  expect_contains(rendered, "add w0, w0, w1",
                  "aarch64 BIR lowering should combine both incoming integer argument registers for the bounded two-parameter add");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_input_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{c4c::backend::lower_to_bir(make_bir_two_param_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl add_pair",
                  "direct BIR module input should reach aarch64 backend emission without a caller-owned legacy backend module");
  expect_contains(rendered, "add w0, w0, w1",
                  "aarch64 direct BIR input should still combine both incoming integer argument registers on the backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_staged_affine_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_bir_two_param_staged_affine_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl add_pair",
                  "direct BIR staged affine input should reach aarch64 backend emission without caller-owned legacy backend IR");
  expect_contains(rendered, "add w0, w0, w1",
                  "aarch64 direct BIR staged affine input should still combine both incoming integer argument registers");
  expect_contains(rendered, "add w0, w0, #1",
                  "aarch64 direct BIR staged affine input should preserve the collapsed affine constant");
}

void test_backend_bir_pipeline_drives_x86_two_param_add_sub_chain_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl add_pair",
                  "BIR pipeline should still reach x86 backend emission for the bounded two-parameter affine chain");
  expect_contains(rendered, "mov eax, edi",
                  "x86 BIR lowering should seed the result from the first incoming integer argument register for the affine chain");
  expect_contains(rendered, "add eax, esi",
                  "x86 BIR lowering should combine the second integer argument register before the immediate adjustment");
  expect_contains(rendered, "sub eax, 1",
                  "x86 BIR lowering should preserve the trailing immediate adjustment for the bounded two-parameter affine chain");
}

void test_backend_bir_pipeline_drives_aarch64_two_param_add_sub_chain_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl add_pair",
                  "BIR pipeline should still reach aarch64 backend emission for the bounded two-parameter affine chain");
  expect_contains(rendered, "add w0, w0, w1",
                  "aarch64 BIR lowering should combine both incoming integer argument registers before the immediate adjustment");
  expect_contains(rendered, "sub w0, w0, #1",
                  "aarch64 BIR lowering should preserve the trailing immediate adjustment for the bounded two-parameter affine chain");
}

void test_backend_bir_pipeline_drives_x86_two_param_staged_affine_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_staged_affine_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl add_pair",
                  "BIR pipeline should still reach x86 backend emission for the staged two-parameter affine slice");
  expect_contains(rendered, "mov eax, edi",
                  "x86 BIR lowering should seed the staged affine slice from the first incoming integer argument register");
  expect_contains(rendered, "add eax, esi",
                  "x86 BIR lowering should still combine the second incoming integer argument register for the staged affine slice");
  expect_contains(rendered, "add eax, 1",
                  "x86 BIR lowering should collapse the staged immediate adjustments to the surviving affine constant");
}

void test_backend_bir_pipeline_drives_aarch64_two_param_staged_affine_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_staged_affine_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl add_pair",
                  "BIR pipeline should still reach aarch64 backend emission for the staged two-parameter affine slice");
  expect_contains(rendered, "add w0, w0, w1",
                  "aarch64 BIR lowering should combine both incoming integer argument registers for the staged affine slice");
  expect_contains(rendered, "add w0, w0, #1",
                  "aarch64 BIR lowering should collapse the staged immediate adjustments to the surviving affine constant");
}

void test_backend_bir_pipeline_rejects_unsupported_direct_bir_input_on_x86() {
  try {
    (void)c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{make_unsupported_multi_function_bir_module()},
        make_bir_pipeline_options(c4c::backend::Target::X86_64));
  } catch (const std::invalid_argument& ex) {
    expect_contains(ex.what(), "direct BIR module",
                    "x86 direct BIR rejection should explain that unsupported direct BIR input no longer falls back through legacy backend IR");
    return;
  }

  fail("x86 direct BIR input should fail explicitly once the emitter-side bir_to_backend_ir fallback is removed");
}

void test_backend_bir_pipeline_rejects_unsupported_direct_bir_input_on_aarch64() {
  try {
    (void)c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{make_unsupported_multi_function_bir_module()},
        make_bir_pipeline_options(c4c::backend::Target::Aarch64));
  } catch (const std::invalid_argument& ex) {
    expect_contains(ex.what(), "direct BIR module",
                    "aarch64 direct BIR rejection should explain that unsupported direct BIR input no longer falls back through legacy backend IR");
    return;
  }

  fail("aarch64 direct BIR input should fail explicitly once the emitter-side bir_to_backend_ir fallback is removed");
}

void test_backend_bir_pipeline_selection_only_applies_at_lir_entry_input() {
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

void test_backend_lowered_riscv_passthrough_ignores_broken_legacy_fallback() {
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

void run_backend_bir_pipeline_tests() {
  RUN_TEST(test_backend_default_path_remains_legacy_when_bir_pipeline_is_not_selected);
  RUN_TEST(test_backend_bir_pipeline_is_opt_in_through_backend_options);
  RUN_TEST(test_backend_bir_pipeline_accepts_direct_bir_module_input);
  RUN_TEST(test_backend_bir_pipeline_routes_sub_cluster_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_straight_line_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_zero_param_staged_constant_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_mul_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i64_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_single_param_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_add_sub_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_staged_affine_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_return_add_smoke_case_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_return_sub_smoke_case_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_zero_param_staged_constant_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_single_param_chain_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_two_param_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_input_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_staged_affine_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_two_param_add_sub_chain_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_two_param_staged_affine_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_single_param_chain_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_zero_param_staged_constant_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_two_param_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_input_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_staged_affine_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_two_param_add_sub_chain_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_two_param_staged_affine_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_rejects_unsupported_direct_bir_input_on_x86);
  RUN_TEST(test_backend_bir_pipeline_rejects_unsupported_direct_bir_input_on_aarch64);
  RUN_TEST(test_backend_bir_pipeline_selection_only_applies_at_lir_entry_input);
  RUN_TEST(test_backend_lowered_riscv_passthrough_ignores_broken_legacy_fallback);
}
