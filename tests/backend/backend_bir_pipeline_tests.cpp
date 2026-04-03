#include "backend_bir_test_support.hpp"

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

void test_backend_bir_pipeline_routes_sub_cluster_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should keep the narrow sub slice on the BIR path");
  expect_contains(rendered, "%t0 = bir.sub i32 3, 3",
                  "explicit BIR selection should expose sub through the BIR-specific route surface");
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

}  // namespace

void run_backend_bir_pipeline_tests() {
  RUN_TEST(test_backend_default_path_remains_legacy_when_bir_pipeline_is_not_selected);
  RUN_TEST(test_backend_bir_pipeline_is_opt_in_through_backend_options);
  RUN_TEST(test_backend_bir_pipeline_routes_sub_cluster_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_return_add_smoke_case_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_return_sub_smoke_case_end_to_end);
}
