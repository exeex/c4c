#include "backend_bir_test_support.hpp"

#include "../../src/backend/ir_printer.hpp"
#include "../../src/backend/lowering/lir_to_backend_ir.hpp"
#include "../../src/backend/lowering/lir_to_bir.hpp"

namespace {

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

void test_backend_bir_pipeline_drives_aarch64_sext_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_sext_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl widen_signed",
                  "BIR pipeline should reach aarch64 backend emission for the bounded sext slice");
  expect_contains(rendered, "sxtw x0, w0",
                  "aarch64 BIR lowering should sign-extend the incoming i32 argument into the i64 return register");
}

void test_backend_bir_pipeline_drives_aarch64_zext_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_zext_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl widen_unsigned",
                  "BIR pipeline should reach aarch64 backend emission for the bounded zext slice");
  expect_contains(rendered, "and w0, w0, #0xff",
                  "aarch64 BIR lowering should zero-extend the incoming i8 argument into the i32 return register");
}

void test_backend_bir_pipeline_drives_aarch64_trunc_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_trunc_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl narrow_signed",
                  "BIR pipeline should reach aarch64 backend emission for the bounded trunc slice");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 BIR lowering should keep the bounded trunc slice on the native asm path");
}

void test_backend_bir_pipeline_drives_aarch64_select_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_bir_return_select_eq_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl main",
                  "direct BIR select input should reach aarch64 backend emission without a caller-owned legacy backend module");
  expect_contains(rendered, "cmp w8, #7",
                  "aarch64 BIR lowering should compare the bounded select operands on the native backend path");
  expect_contains(rendered, "mov w0, #11",
                  "aarch64 BIR lowering should materialize the true-value arm for the bounded select slice");
  expect_contains(rendered, "mov w0, #4",
                  "aarch64 BIR lowering should materialize the false-value arm for the bounded select slice");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_single_param_select_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_bir_single_param_select_eq_phi_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose",
                  "direct BIR single-parameter select input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov w8, w0",
                  "aarch64 direct BIR single-parameter select input should stage the incoming integer argument into the compare scratch register");
  expect_contains(rendered, "cmp w8, #7",
                  "aarch64 direct BIR single-parameter select input should compare the staged integer argument against the bounded immediate");
  expect_contains(rendered, "mov w0, #11",
                  "aarch64 direct BIR single-parameter select input should materialize the true-value arm for the bounded parameter-fed select slice");
  expect_contains(rendered, "mov w0, #4",
                  "aarch64 direct BIR single-parameter select input should materialize the false-value arm for the bounded parameter-fed select slice");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_two_param_select_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_bir_two_param_select_eq_phi_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2",
                  "direct BIR two-parameter select input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov w8, w0",
                  "aarch64 direct BIR two-parameter select input should stage the first incoming integer argument into the compare scratch register");
  expect_contains(rendered, "mov w9, w1",
                  "aarch64 direct BIR two-parameter select input should stage the second incoming integer argument into the compare scratch register");
  expect_contains(rendered, "cmp w8, w9",
                  "aarch64 direct BIR two-parameter select input should compare both staged integer argument registers on the native backend path");
  expect_contains(rendered, "mov w0, w1",
                  "aarch64 direct BIR two-parameter select input should materialize the false-value arm from the second incoming integer argument register");
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

}  // namespace

void run_backend_bir_pipeline_aarch64_tests() {
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_single_param_chain_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_zero_param_staged_constant_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_two_param_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_input_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_staged_affine_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_two_param_add_sub_chain_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_two_param_staged_affine_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_sext_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_zext_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_trunc_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_select_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_single_param_select_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_two_param_select_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_rejects_unsupported_direct_bir_input_on_aarch64);
}
