#include "backend_bir_test_support.hpp"

#include "../../src/backend/lowering/lir_to_bir.hpp"

namespace {

c4c::codegen::lir::LirModule make_lir_minimal_direct_call_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "helper";
  helper.signature_text = "define i32 @helper()\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.terminator = LirRet{std::string("42"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_function;
  main_function.name = "main";
  main_function.signature_text = "define i32 @main()\n";
  main_function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@helper", "", ""});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(main_function));
  module.functions.push_back(std::move(helper));
  return module;
}

c4c::codegen::lir::LirModule make_lir_declared_direct_call_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.extern_decls.push_back(LirExternDecl{"puts_like", "i32"});
  module.string_pool.push_back(LirStringConst{"@msg", "hello\\0A", 7});

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@puts_like", "", "ptr @msg"});
  entry.terminator = LirRet{std::string("7"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_void_direct_call_imm_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "voidfn";
  helper.signature_text = "define void @voidfn()\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.terminator = LirRet{std::nullopt, "void"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_function;
  main_function.name = "main";
  main_function.signature_text = "define i32 @main()\n";
  main_function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"", "void", "@voidfn", "", ""});
  entry.terminator = LirRet{std::string("9"), "i32"};
  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(main_function));
  module.functions.push_back(std::move(helper));
  return module;
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_direct_call_end_to_end() {
  c4c::backend::bir::Module module;
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "helper",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::immediate_i32(42),
          },
      }},
      .is_declaration = false,
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "main",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {c4c::backend::bir::CallInst{
              .result = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
              .callee = "helper",
              .args = {},
              .return_type_name = "i32",
          }},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
          },
      }},
      .is_declaration = false,
  });

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".type helper, %function\nhelper:",
                  "direct BIR zero-argument direct-call input should emit the helper body on the native aarch64 path");
  expect_contains(rendered, "mov w0, #42",
                  "direct BIR zero-argument direct-call input should preserve the helper return immediate on the native aarch64 path");
  expect_contains(rendered, ".globl main",
                  "direct BIR zero-argument direct-call input should emit the caller symbol without routing through legacy backend IR");
  expect_contains(rendered, "bl helper",
                  "direct BIR zero-argument direct-call input should branch to the helper on the native aarch64 backend path");
  expect_not_contains(rendered, "target triple =",
                      "direct BIR zero-argument direct-call input should stay on native aarch64 asm emission");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_declared_direct_call_end_to_end() {
  c4c::backend::bir::Module module;
  module.string_constants.push_back(c4c::backend::bir::StringConstant{
      .name = "msg",
      .bytes = "hello\\n",
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "puts_like",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {c4c::backend::bir::Param{
          .type = c4c::backend::bir::TypeKind::I64,
          .name = "%arg0",
      }},
      .local_slots = {},
      .blocks = {},
      .is_declaration = true,
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "main",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {c4c::backend::bir::CallInst{
              .result = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
              .callee = "puts_like",
              .args = {c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I64, "msg")},
              .return_type_name = "i32",
          }},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::immediate_i32(7),
          },
      }},
      .is_declaration = false,
  });

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".section .rodata",
                  "direct BIR declared direct-call input should materialize referenced string constants on the native aarch64 path");
  expect_contains(rendered, ".asciz \"hello\\n\"",
                  "direct BIR declared direct-call input should preserve string-constant bytes for native aarch64 emission");
  expect_contains(rendered, "adrp x0, ",
                  "direct BIR declared direct-call input should materialize pointer-style extern arguments through address formation on the native aarch64 path");
  expect_contains(rendered, "bl puts_like",
                  "direct BIR declared direct-call input should branch to the declared callee without routing through legacy backend IR");
  expect_contains(rendered, "mov w0, #7",
                  "direct BIR declared direct-call input should preserve the fixed immediate return override on the native aarch64 path");
  expect_not_contains(rendered, "target triple =",
                      "direct BIR declared direct-call input should stay on native aarch64 asm emission");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_void_direct_call_imm_return_end_to_end() {
  c4c::backend::bir::Module module;
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "voidfn",
      .return_type = c4c::backend::bir::TypeKind::Void,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {},
          .terminator = c4c::backend::bir::ReturnTerminator{},
      }},
      .is_declaration = false,
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "main",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {c4c::backend::bir::CallInst{
              .result = std::nullopt,
              .callee = "voidfn",
              .args = {},
              .return_type_name = "void",
          }},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::immediate_i32(9),
          },
      }},
      .is_declaration = false,
  });

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".type voidfn, %function\nvoidfn:",
                  "direct BIR void direct-call input should emit the helper body on the native aarch64 path");
  expect_contains(rendered, "bl voidfn",
                  "direct BIR void direct-call input should branch to the helper on the native aarch64 backend path");
  expect_contains(rendered, "mov w0, #9",
                  "direct BIR void direct-call input should preserve the fixed caller return immediate on the native aarch64 path");
  expect_not_contains(rendered, "target triple =",
                      "direct BIR void direct-call input should stay on native aarch64 asm emission");
}

void test_backend_bir_pipeline_drives_aarch64_lir_minimal_direct_call_through_bir_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_direct_call_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".type helper, %function\nhelper:",
                  "aarch64 LIR minimal direct-call input should still emit the helper definition after routing through the shared BIR path");
  expect_contains(rendered, "mov w0, #42",
                  "aarch64 LIR minimal direct-call input should preserve the helper immediate on the BIR-owned route");
  expect_contains(rendered, "bl helper",
                  "aarch64 LIR minimal direct-call input should lower the helper call on the native aarch64 path");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR minimal direct-call input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_minimal_void_direct_call_imm_return_through_bir_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_void_direct_call_imm_return_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".type voidfn, %function\nvoidfn:",
                  "aarch64 LIR void direct-call input should still emit the helper definition after routing through the shared BIR path");
  expect_contains(rendered, "bl voidfn",
                  "aarch64 LIR void direct-call input should lower the helper call on the native aarch64 path");
  expect_contains(rendered, "mov w0, #9",
                  "aarch64 LIR void direct-call input should preserve the fixed caller return immediate on the BIR-owned route");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR void direct-call input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_declared_direct_call_through_bir_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_declared_direct_call_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".section .rodata",
                  "aarch64 LIR declared direct-call input should now reach the BIR-owned extern-call emitter path");
  expect_contains(rendered, ".asciz \"hello\\n\"",
                  "aarch64 LIR declared direct-call input should preserve string bytes through the BIR lowering seam");
  expect_contains(rendered, "bl puts_like",
                  "aarch64 LIR declared direct-call input should still lower the declared call after the aarch64 legacy seam is bypassed");
  expect_contains(rendered, "mov w0, #7",
                  "aarch64 LIR declared direct-call input should preserve the fixed immediate return override on the BIR path");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR declared direct-call input should stay on native asm emission instead of falling back to LLVM text");
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

void test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_post_join_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_predecessor_add_phi_post_join_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_add_post_ne_u",
                  "direct BIR u8 select-plus-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "and w8, w0, #0xff",
                  "aarch64 direct BIR u8 select-plus-tail input should mask the first incoming byte before comparison");
  expect_contains(rendered, "and w9, w1, #0xff\n  cmp w8, w9\n",
                  "aarch64 direct BIR u8 select-plus-tail input should compare the masked incoming bytes on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  and w0, w0, #0xff\n  add w0, w0, #5\n  b .Lselect_join\n",
                  "aarch64 direct BIR u8 select-plus-tail input should materialize the bounded then-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  and w0, w1, #0xff\n  add w0, w0, #9\n",
                  "aarch64 direct BIR u8 select-plus-tail input should materialize the bounded else-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  ret\n",
                  "aarch64 direct BIR u8 select-plus-tail input should preserve the bounded post-select arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_post_join_add_sub_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_split_predecessor_add_phi_post_join_add_sub_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_add_post_chain_ne_u",
                  "direct BIR u8 select-plus-add/sub-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "and w8, w0, #0xff",
                  "aarch64 direct BIR u8 select-plus-add/sub-tail input should mask the first incoming byte before comparison");
  expect_contains(rendered, "and w9, w1, #0xff\n  cmp w8, w9\n",
                  "aarch64 direct BIR u8 select-plus-add/sub-tail input should compare the masked incoming bytes on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  and w0, w0, #0xff\n  add w0, w0, #5\n  b .Lselect_join\n",
                  "aarch64 direct BIR u8 select-plus-add/sub-tail input should materialize the bounded then-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  and w0, w1, #0xff\n  add w0, w0, #9\n",
                  "aarch64 direct BIR u8 select-plus-add/sub-tail input should materialize the bounded else-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  sub w0, w0, #2\n  ret\n",
                  "aarch64 direct BIR u8 select-plus-add/sub-tail input should preserve the bounded post-select add/sub arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_post_join_add_sub_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_split_predecessor_add_phi_post_join_add_sub_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_add_post_chain_tail_ne_u",
                  "direct BIR u8 select-plus-add/sub/add-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "and w8, w0, #0xff",
                  "aarch64 direct BIR u8 select-plus-add/sub/add-tail input should mask the first incoming byte before comparison");
  expect_contains(rendered, "and w9, w1, #0xff\n  cmp w8, w9\n",
                  "aarch64 direct BIR u8 select-plus-add/sub/add-tail input should compare the masked incoming bytes on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  and w0, w0, #0xff\n  add w0, w0, #5\n  b .Lselect_join\n",
                  "aarch64 direct BIR u8 select-plus-add/sub/add-tail input should materialize the bounded then-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  and w0, w1, #0xff\n  add w0, w0, #9\n",
                  "aarch64 direct BIR u8 select-plus-add/sub/add-tail input should materialize the bounded else-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  sub w0, w0, #2\n  add w0, w0, #9\n  ret\n",
                  "aarch64 direct BIR u8 select-plus-add/sub/add-tail input should preserve the bounded post-select add/sub/add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_mixed_affine_post_join_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_split_predecessor_mixed_affine_phi_post_join_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_mixed_post_ne_u",
                  "direct BIR u8 mixed-affine select-plus-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "and w8, w0, #0xff",
                  "aarch64 direct BIR u8 mixed-affine select-plus-tail input should mask the first incoming byte before comparison");
  expect_contains(rendered, "and w9, w1, #0xff\n  cmp w8, w9\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-tail input should compare the masked incoming bytes on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  and w0, w0, #0xff\n  add w0, w0, #8\n  sub w0, w0, #3\n  b .Lselect_join\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-tail input should materialize the bounded then-arm affine arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  and w0, w1, #0xff\n  add w0, w0, #11\n  sub w0, w0, #4\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-tail input should materialize the bounded else-arm affine arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  ret\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-tail input should preserve the bounded post-select add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_mixed_affine_post_join_add_sub_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_split_predecessor_mixed_affine_phi_post_join_add_sub_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_mixed_post_chain_ne_u",
                  "direct BIR u8 mixed-affine select-plus-add/sub-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "and w8, w0, #0xff",
                  "aarch64 direct BIR u8 mixed-affine select-plus-add/sub-tail input should mask the first incoming byte before comparison");
  expect_contains(rendered, "and w9, w1, #0xff\n  cmp w8, w9\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-add/sub-tail input should compare the masked incoming bytes on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  and w0, w0, #0xff\n  add w0, w0, #8\n  sub w0, w0, #3\n  b .Lselect_join\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-add/sub-tail input should materialize the bounded then-arm affine arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  and w0, w1, #0xff\n  add w0, w0, #11\n  sub w0, w0, #4\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-add/sub-tail input should materialize the bounded else-arm affine arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  sub w0, w0, #2\n  ret\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-add/sub-tail input should preserve the bounded post-select add/sub arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_mixed_affine_post_join_add_sub_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_eq_split_predecessor_mixed_affine_phi_post_join_add_sub_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_mixed_post_chain_tail_u",
                  "direct BIR u8 mixed-affine select-plus-add/sub/add-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "and w8, w0, #0xff",
                  "aarch64 direct BIR u8 mixed-affine select-plus-add/sub/add-tail input should mask the first incoming byte before comparison");
  expect_contains(rendered, "and w9, w1, #0xff\n  cmp w8, w9\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-add/sub/add-tail input should compare the masked incoming bytes on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  and w0, w0, #0xff\n  add w0, w0, #8\n  sub w0, w0, #3\n  b .Lselect_join\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-add/sub/add-tail input should materialize the bounded then-arm affine arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  and w0, w1, #0xff\n  add w0, w0, #11\n  sub w0, w0, #4\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-add/sub/add-tail input should materialize the bounded else-arm affine arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  sub w0, w0, #2\n  add w0, w0, #9\n  ret\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-add/sub/add-tail input should preserve the bounded post-select add/sub/add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_deeper_then_mixed_affine_post_join_add_sub_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_deeper_post_chain_tail_u",
                  "direct BIR u8 deeper-then-mixed-affine select-plus-add/sub/add-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "and w8, w0, #0xff",
                  "aarch64 direct BIR u8 deeper-then-mixed-affine select-plus-add/sub/add-tail input should mask the first incoming byte before comparison");
  expect_contains(rendered, "and w9, w1, #0xff\n  cmp w8, w9\n",
                  "aarch64 direct BIR u8 deeper-then-mixed-affine select-plus-add/sub/add-tail input should compare the masked incoming bytes on the native backend path");
  expect_contains(rendered,
                  ".Lselect_true:\n  and w0, w0, #0xff\n  add w0, w0, #8\n  sub w0, w0, #3\n  add w0, w0, #5\n  b .Lselect_join\n",
                  "aarch64 direct BIR u8 deeper-then-mixed-affine select-plus-add/sub/add-tail input should materialize the bounded deeper then-arm arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  and w0, w1, #0xff\n  add w0, w0, #11\n  sub w0, w0, #4\n",
                  "aarch64 direct BIR u8 deeper-then-mixed-affine select-plus-add/sub/add-tail input should materialize the bounded else-arm mixed arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  sub w0, w0, #2\n  add w0, w0, #9\n  ret\n",
                  "aarch64 direct BIR u8 deeper-then-mixed-affine select-plus-add/sub/add-tail input should preserve the bounded post-select add/sub/add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_deeper_affine_on_both_sides_post_join_add_sub_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_eq_split_predecessor_deeper_affine_phi_post_join_add_sub_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_deeper_both_post_chain_tail_u",
                  "direct BIR u8 deeper-affine-both-sides select-plus-add/sub/add-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "and w8, w0, #0xff",
                  "aarch64 direct BIR u8 deeper-affine-both-sides select-plus-add/sub/add-tail input should mask the first incoming byte before comparison");
  expect_contains(rendered, "and w9, w1, #0xff\n  cmp w8, w9\n",
                  "aarch64 direct BIR u8 deeper-affine-both-sides select-plus-add/sub/add-tail input should compare the masked incoming bytes on the native backend path");
  expect_contains(rendered,
                  ".Lselect_true:\n  and w0, w0, #0xff\n  add w0, w0, #8\n  sub w0, w0, #3\n  add w0, w0, #5\n  b .Lselect_join\n",
                  "aarch64 direct BIR u8 deeper-affine-both-sides select-plus-add/sub/add-tail input should materialize the bounded deeper then-arm arithmetic before the synthetic join");
  expect_contains(rendered,
                  ".Lselect_false:\n  and w0, w1, #0xff\n  add w0, w0, #11\n  sub w0, w0, #4\n  add w0, w0, #7\n",
                  "aarch64 direct BIR u8 deeper-affine-both-sides select-plus-add/sub/add-tail input should materialize the bounded deeper else-arm arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  sub w0, w0, #2\n  add w0, w0, #9\n  ret\n",
                  "aarch64 direct BIR u8 deeper-affine-both-sides select-plus-add/sub/add-tail input should preserve the bounded post-select add/sub/add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_mixed_then_deeper_affine_post_join_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_mixed_then_deeper_post_ne_u",
                  "direct BIR u8 mixed-then-deeper-affine select-plus-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "and w8, w0, #0xff",
                  "aarch64 direct BIR u8 mixed-then-deeper-affine select-plus-tail input should mask the first incoming byte before comparison");
  expect_contains(rendered, "and w9, w1, #0xff\n  cmp w8, w9\n",
                  "aarch64 direct BIR u8 mixed-then-deeper-affine select-plus-tail input should compare the masked incoming bytes on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  and w0, w0, #0xff\n  add w0, w0, #8\n  sub w0, w0, #3\n  b .Lselect_join\n",
                  "aarch64 direct BIR u8 mixed-then-deeper-affine select-plus-tail input should materialize the bounded then-arm mixed arithmetic before the synthetic join");
  expect_contains(rendered,
                  ".Lselect_false:\n  and w0, w1, #0xff\n  add w0, w0, #11\n  sub w0, w0, #4\n  add w0, w0, #7\n",
                  "aarch64 direct BIR u8 mixed-then-deeper-affine select-plus-tail input should materialize the bounded else-arm deeper arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  ret\n",
                  "aarch64 direct BIR u8 mixed-then-deeper-affine select-plus-tail input should preserve the bounded post-select add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_i32_select_mixed_then_deeper_affine_post_join_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_mixed_then_deeper_post",
                  "direct BIR i32 mixed-then-deeper-affine select-plus-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov w8, w0",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-tail input should stage the first incoming integer argument into the compare scratch register");
  expect_contains(rendered, "mov w9, w1",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-tail input should stage the second incoming integer argument into the compare scratch register");
  expect_contains(rendered, "cmp w8, w9",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-tail input should compare both staged integer argument registers on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  add w0, w0, #8\n  sub w0, w0, #3\n  b .Lselect_join\n",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-tail input should materialize the bounded then-arm mixed arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  mov w0, w1\n  add w0, w0, #11\n  sub w0, w0, #4\n  add w0, w0, #7\n",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-tail input should materialize the bounded else-arm deeper arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  ret\n",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-tail input should preserve the bounded post-select add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_i32_select_mixed_then_deeper_affine_post_join_add_sub_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_mixed_then_deeper_post_chain",
                  "direct BIR i32 mixed-then-deeper-affine select-plus-add/sub-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov w8, w0",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub-tail input should stage the first incoming integer argument into the compare scratch register");
  expect_contains(rendered, "mov w9, w1",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub-tail input should stage the second incoming integer argument into the compare scratch register");
  expect_contains(rendered, "cmp w8, w9",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub-tail input should compare both staged integer argument registers on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  add w0, w0, #8\n  sub w0, w0, #3\n  b .Lselect_join\n",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub-tail input should materialize the bounded then-arm mixed arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  mov w0, w1\n  add w0, w0, #11\n  sub w0, w0, #4\n  add w0, w0, #7\n",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub-tail input should materialize the bounded else-arm deeper arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  sub w0, w0, #2\n  ret\n",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub-tail input should preserve the bounded post-select add/sub arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_i32_select_mixed_then_deeper_affine_post_join_add_sub_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_mixed_then_deeper_post_chain_tail",
                  "direct BIR i32 mixed-then-deeper-affine select-plus-add/sub/add-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov w8, w0",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub/add-tail input should stage the first incoming integer argument into the compare scratch register");
  expect_contains(rendered, "mov w9, w1",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub/add-tail input should stage the second incoming integer argument into the compare scratch register");
  expect_contains(rendered, "cmp w8, w9",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub/add-tail input should compare both staged integer argument registers on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  add w0, w0, #8\n  sub w0, w0, #3\n  b .Lselect_join\n",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub/add-tail input should materialize the bounded then-arm mixed arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  mov w0, w1\n  add w0, w0, #11\n  sub w0, w0, #4\n  add w0, w0, #7\n",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub/add-tail input should materialize the bounded else-arm deeper arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  sub w0, w0, #2\n  add w0, w0, #9\n  ret\n",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub/add-tail input should preserve the bounded post-select add/sub/add arithmetic tail on the native backend path");
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
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_direct_call_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_declared_direct_call_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_void_direct_call_imm_return_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_minimal_direct_call_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_minimal_void_direct_call_imm_return_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_declared_direct_call_through_bir_end_to_end);
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
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_post_join_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_post_join_add_sub_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_post_join_add_sub_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_mixed_affine_post_join_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_mixed_affine_post_join_add_sub_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_mixed_affine_post_join_add_sub_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_deeper_then_mixed_affine_post_join_add_sub_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_deeper_affine_on_both_sides_post_join_add_sub_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_mixed_then_deeper_affine_post_join_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_i32_select_mixed_then_deeper_affine_post_join_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_i32_select_mixed_then_deeper_affine_post_join_add_sub_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_i32_select_mixed_then_deeper_affine_post_join_add_sub_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_rejects_unsupported_direct_bir_input_on_aarch64);
}
