#include "backend_bir_test_support.hpp"

#include "../../src/backend/lowering/lir_to_bir.hpp"

#include <stdexcept>

namespace {

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

void test_backend_bir_pipeline_drives_x86_sext_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_sext_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl widen_signed",
                  "BIR pipeline should reach x86 backend emission for the bounded sext slice");
  expect_contains(rendered, "movsxd rax, edi",
                  "x86 BIR lowering should sign-extend the incoming i32 argument into the i64 return register");
}

void test_backend_bir_pipeline_drives_x86_zext_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_zext_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl widen_unsigned",
                  "BIR pipeline should reach x86 backend emission for the bounded zext slice");
  expect_contains(rendered, "movzx eax, dil",
                  "x86 BIR lowering should zero-extend the incoming i8 argument into the i32 return register");
}

void test_backend_bir_pipeline_drives_x86_trunc_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_trunc_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl narrow_signed",
                  "BIR pipeline should reach x86 backend emission for the bounded trunc slice");
  expect_contains(rendered, "mov eax, edi",
                  "x86 BIR lowering should truncate the incoming i64 argument by returning its low i32 half");
}

void test_backend_bir_pipeline_drives_x86_select_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_bir_return_select_eq_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "direct BIR select input should reach x86 backend emission without a caller-owned legacy backend module");
  expect_contains(rendered, "cmp eax, 7",
                  "x86 BIR lowering should compare the bounded select operands on the native backend path");
  expect_contains(rendered, "mov eax, 11",
                  "x86 BIR lowering should materialize the true-value arm for the bounded select slice");
  expect_contains(rendered, "mov eax, 4",
                  "x86 BIR lowering should materialize the false-value arm for the bounded select slice");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_single_param_select_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_bir_single_param_select_eq_phi_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose",
                  "direct BIR single-parameter select input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov eax, edi",
                  "x86 direct BIR single-parameter select input should seed the compare from the incoming integer argument register");
  expect_contains(rendered, "cmp eax, 7",
                  "x86 direct BIR single-parameter select input should compare the incoming integer argument against the bounded immediate");
  expect_contains(rendered, "mov eax, 11",
                  "x86 direct BIR single-parameter select input should materialize the true-value arm for the bounded parameter-fed select slice");
  expect_contains(rendered, "mov eax, 4",
                  "x86 direct BIR single-parameter select input should materialize the false-value arm for the bounded parameter-fed select slice");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_two_param_select_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_bir_two_param_select_eq_phi_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose2",
                  "direct BIR two-parameter select input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov eax, edi",
                  "x86 direct BIR two-parameter select input should seed the compare from the first incoming integer argument register");
  expect_contains(rendered, "cmp eax, esi",
                  "x86 direct BIR two-parameter select input should compare both incoming integer argument registers on the native backend path");
  expect_contains(rendered, "mov eax, esi",
                  "x86 direct BIR two-parameter select input should materialize the false-value arm from the second incoming integer argument register");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_post_join_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_predecessor_add_phi_post_join_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose2_add_post_ne_u",
                  "direct BIR u8 select-plus-tail input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov al, dil",
                  "x86 direct BIR u8 select-plus-tail input should compare the low byte from the first incoming integer argument");
  expect_contains(rendered, "cmp al, sil",
                  "x86 direct BIR u8 select-plus-tail input should compare the second incoming low byte on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  movzx eax, dil\n  add eax, 5\n  jmp .Lselect_join\n",
                  "x86 direct BIR u8 select-plus-tail input should materialize the bounded then-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  movzx eax, sil\n  add eax, 9\n",
                  "x86 direct BIR u8 select-plus-tail input should materialize the bounded else-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add eax, 6\n  ret\n",
                  "x86 direct BIR u8 select-plus-tail input should preserve the bounded post-select arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_post_join_add_sub_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_split_predecessor_add_phi_post_join_add_sub_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose2_add_post_chain_ne_u",
                  "direct BIR u8 select-plus-add/sub-tail input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov al, dil",
                  "x86 direct BIR u8 select-plus-add/sub-tail input should compare the low byte from the first incoming integer argument");
  expect_contains(rendered, "cmp al, sil",
                  "x86 direct BIR u8 select-plus-add/sub-tail input should compare the second incoming low byte on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  movzx eax, dil\n  add eax, 5\n  jmp .Lselect_join\n",
                  "x86 direct BIR u8 select-plus-add/sub-tail input should materialize the bounded then-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  movzx eax, sil\n  add eax, 9\n",
                  "x86 direct BIR u8 select-plus-add/sub-tail input should materialize the bounded else-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add eax, 6\n  sub eax, 2\n  ret\n",
                  "x86 direct BIR u8 select-plus-add/sub-tail input should preserve the bounded post-select add/sub arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_post_join_add_sub_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_split_predecessor_add_phi_post_join_add_sub_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose2_add_post_chain_tail_ne_u",
                  "direct BIR u8 select-plus-add/sub/add-tail input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov al, dil",
                  "x86 direct BIR u8 select-plus-add/sub/add-tail input should compare the low byte from the first incoming integer argument");
  expect_contains(rendered, "cmp al, sil",
                  "x86 direct BIR u8 select-plus-add/sub/add-tail input should compare the second incoming low byte on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  movzx eax, dil\n  add eax, 5\n  jmp .Lselect_join\n",
                  "x86 direct BIR u8 select-plus-add/sub/add-tail input should materialize the bounded then-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  movzx eax, sil\n  add eax, 9\n",
                  "x86 direct BIR u8 select-plus-add/sub/add-tail input should materialize the bounded else-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add eax, 6\n  sub eax, 2\n  add eax, 9\n  ret\n",
                  "x86 direct BIR u8 select-plus-add/sub/add-tail input should preserve the bounded post-select add/sub/add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_mixed_affine_post_join_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_split_predecessor_mixed_affine_phi_post_join_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose2_mixed_post_ne_u",
                  "direct BIR u8 mixed-affine select-plus-tail input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov al, dil",
                  "x86 direct BIR u8 mixed-affine select-plus-tail input should compare the low byte from the first incoming integer argument");
  expect_contains(rendered, "cmp al, sil",
                  "x86 direct BIR u8 mixed-affine select-plus-tail input should compare the second incoming low byte on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  movzx eax, dil\n  add eax, 8\n  sub eax, 3\n  jmp .Lselect_join\n",
                  "x86 direct BIR u8 mixed-affine select-plus-tail input should materialize the bounded then-arm affine arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  movzx eax, sil\n  add eax, 11\n  sub eax, 4\n",
                  "x86 direct BIR u8 mixed-affine select-plus-tail input should materialize the bounded else-arm affine arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add eax, 6\n  ret\n",
                  "x86 direct BIR u8 mixed-affine select-plus-tail input should preserve the bounded post-select add arithmetic tail on the native backend path");
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

}  // namespace

void run_backend_bir_pipeline_x86_64_tests() {
  RUN_TEST(test_backend_bir_pipeline_drives_x86_return_add_smoke_case_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_return_sub_smoke_case_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_zero_param_staged_constant_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_single_param_chain_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_two_param_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_input_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_staged_affine_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_two_param_add_sub_chain_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_two_param_staged_affine_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_sext_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_zext_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_trunc_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_select_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_single_param_select_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_two_param_select_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_post_join_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_post_join_add_sub_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_post_join_add_sub_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_mixed_affine_post_join_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_rejects_unsupported_direct_bir_input_on_x86);
}
