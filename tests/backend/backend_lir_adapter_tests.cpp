#include "backend.hpp"
#include "generation.hpp"
#include "ir_printer.hpp"
#include "ir_validate.hpp"
#include "liveness.hpp"
#include "lir_adapter.hpp"
#include "regalloc.hpp"
#include "stack_layout/analysis.hpp"
#include "stack_layout/alloca_coalescing.hpp"
#include "stack_layout/regalloc_helpers.hpp"
#include "stack_layout/slot_assignment.hpp"
#include "target.hpp"
#include "../../src/codegen/lir/call_args.hpp"
#include "../../src/codegen/lir/lir_printer.hpp"
#include "../../src/codegen/lir/verify.hpp"
#include "../../src/backend/elf/mod.hpp"
#include "../../src/backend/lowering/call_decode.hpp"
#include "../../src/backend/lowering/lir_to_backend_ir.hpp"
#include "../../src/backend/linker_common/mod.hpp"
#include "../../src/backend/aarch64/assembler/mod.hpp"
#include "../../src/backend/aarch64/codegen/emit.hpp"
#include "../../src/backend/aarch64/linker/mod.hpp"
#include "../../src/backend/aarch64/assembler/parser.hpp"
#include "../../src/backend/x86/assembler/mod.hpp"
#include "../../src/backend/x86/assembler/encoder/mod.hpp"
#include "../../src/backend/x86/assembler/parser.hpp"
#include "../../src/backend/x86/codegen/emit.hpp"
#include "../../src/backend/x86/linker/mod.hpp"
#include "backend_test_helper.hpp"
#include "backend_test_fixtures.hpp"


#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>

namespace c4c::backend::aarch64::assembler {

bool is_branch_reloc_type(std::uint32_t elf_type);

}  // namespace c4c::backend::aarch64::assembler

namespace c4c::backend::aarch64::assembler::encoder {

bool is_64bit_reg(const std::string& name);
bool is_32bit_reg(const std::string& name);
bool is_fp_reg(const std::string& name);
std::uint32_t sf_bit(bool is_64);

}  // namespace c4c::backend::aarch64::assembler::encoder



void test_lir_typed_wrappers_classify_basic_operands() {
  using namespace c4c::codegen::lir;

  const LirOperand ssa("%t4");
  const LirOperand global("@helper");
  const LirOperand immediate("42");
  const LirOperand special("zeroinitializer");
  const LirOperand raw("i32 %t1");

  expect_true(ssa.kind() == LirOperandKind::SsaValue, "ssa operand classification should be typed");
  expect_true(global.kind() == LirOperandKind::Global, "global operand classification should be typed");
  expect_true(immediate.kind() == LirOperandKind::Immediate, "immediate operand classification should be typed");
  expect_true(special.kind() == LirOperandKind::SpecialToken, "special operand classification should be typed");
  expect_true(raw.kind() == LirOperandKind::RawText, "formatted argument text should remain raw");
}

void test_lir_typed_wrappers_classify_basic_types() {
  using namespace c4c::codegen::lir;

  const LirTypeRef i32("i32");
  const LirTypeRef ptr("ptr");
  const LirTypeRef vec("<4 x i32>");
  const LirTypeRef fn("ptr (i32, ptr)");

  expect_true(i32.kind() == LirTypeKind::Integer, "integer llvm types should classify as integer");
  expect_true(ptr.kind() == LirTypeKind::Pointer, "ptr llvm types should classify as pointer");
  expect_true(vec.kind() == LirTypeKind::Vector, "vector llvm types should classify as vector");
  expect_true(fn.kind() == LirTypeKind::Function, "function-like llvm types should classify as function");
}

void test_lir_call_arg_helpers_split_nested_typed_args() {
  using namespace c4c::codegen::lir;

  std::vector<std::string_view> args;
  for_each_lir_call_arg("ptr byval({ i32, i32 }) align 8 %agg, i32 %t0",
                        [&](std::string_view arg) { args.push_back(arg); });

  expect_true(args.size() == 2,
              "typed call-arg splitting should ignore commas nested inside aggregate type syntax");
  expect_true(lir_call_arg_operand(args[0]).has_value() &&
                  *lir_call_arg_operand(args[0]) == "%agg",
              "typed call-arg decoding should recover the operand suffix from byval-style args");
  expect_true(lir_call_arg_operand(args[1]).has_value() &&
                  *lir_call_arg_operand(args[1]) == "%t0",
              "typed call-arg decoding should preserve simple typed operands");
}

void test_lir_call_arg_helpers_collect_value_names() {
  using namespace c4c::codegen::lir;

  std::vector<std::string> values;
  collect_lir_value_names_from_call_args(
      "ptr byval({ i32, i32 }) align 8 %agg, ptr getelementptr inbounds "
      "([2 x i32], ptr %lv.buf, i64 0, i64 1), i32 %t0",
      values);

  expect_true(values.size() == 3 && values[0] == "%agg" && values[1] == "%lv.buf" &&
                  values[2] == "%t0",
              "typed call-arg value collection should keep direct operands and nested expression operands");
}

void test_lir_call_arg_helpers_collect_full_call_value_names() {
  using namespace c4c::codegen::lir;

  std::vector<std::string> values;
  collect_lir_value_names_from_call(
      "%fp",
      "ptr byval({ i32, i32 }) align 8 %agg, ptr getelementptr inbounds "
      "([2 x i32], ptr %lv.buf, i64 0, i64 1), i32 %t0",
      values);

  expect_true(values.size() == 4 && values[0] == "%fp" && values[1] == "%agg" &&
                  values[2] == "%lv.buf" && values[3] == "%t0",
              "shared full-call value collection should preserve indirect callees and typed argument operands");
}

void test_lir_call_arg_helpers_collect_full_call_global_refs() {
  using namespace c4c::codegen::lir;

  std::vector<std::string> refs;
  collect_lir_global_symbol_refs_from_call(
      " @\"helper.with.dot\" ",
      "ptr @glob, ptr getelementptr inbounds ([2 x ptr], ptr @\"dispatch.table\", i64 0, i64 1), "
      "ptr %fp",
      [&](std::string_view ref) { refs.emplace_back(ref); });

  expect_true(refs.size() == 3 && refs[0] == "\"helper.with.dot\"" &&
                  refs[1] == "glob" && refs[2] == "\"dispatch.table\"",
              "shared full-call global-ref collection should preserve direct callees and nested typed-arg global refs");
}

void test_lir_call_arg_helpers_rewrite_full_call_operands() {
  using namespace c4c::codegen::lir;

  std::string callee = "%fp";
  std::string args = "ptr %lv.buf, i32 %t0";
  rewrite_lir_call_operands(
      callee, args, [](std::string_view operand) -> std::optional<std::string_view> {
        if (operand == "%fp") {
          return std::string_view("%fp.coalesced");
        }
        if (operand == "%lv.buf") {
          return std::string_view("%lv.buf.coalesced");
        }
        return std::nullopt;
      });

  expect_true(callee == "%fp.coalesced",
              "shared full-call rewrite should rewrite indirect call operands through one helper");
  expect_true(args == "ptr %lv.buf.coalesced, i32 %t0",
              "shared full-call rewrite should rewrite typed call arguments without reimplementing arg parsing");
}

void test_lir_call_arg_helpers_collect_call_op_value_names() {
  using namespace c4c::codegen::lir;

  const LirCallOp call{"%t1", "i32", "%fp", "(ptr, i32)", "ptr %lv.buf, i32 %t0"};
  std::vector<std::string> values;
  collect_lir_value_names_from_call(call, values);

  expect_true(values.size() == 3 && values[0] == "%fp" && values[1] == "%lv.buf" &&
                  values[2] == "%t0",
              "shared call-op value collection should let backend consumers read full call operands without manually threading fields");
}

void test_lir_call_arg_helpers_rewrite_call_op_operands() {
  using namespace c4c::codegen::lir;

  LirCallOp call{"%t1", "i32", "%fp", "(ptr, i32)", "ptr %lv.buf, i32 %t0"};
  rewrite_lir_call_operands(
      call, [](std::string_view operand) -> std::optional<std::string_view> {
        if (operand == "%fp") {
          return std::string_view("%fp.coalesced");
        }
        if (operand == "%lv.buf") {
          return std::string_view("%lv.buf.coalesced");
        }
        return std::nullopt;
      });

  expect_true(call.callee == "%fp.coalesced",
              "shared call-op rewrite should update indirect callees through the call object overload");
  expect_true(call.args_str == "ptr %lv.buf.coalesced, i32 %t0",
              "shared call-op rewrite should update typed arguments through the call object overload");
}

void test_lir_call_arg_helpers_decode_single_typed_operand() {
  using namespace c4c::codegen::lir;

  const auto operand = parse_lir_single_typed_call_operand(
      "( i32 )", " i32 %t7 ", "i32");
  expect_true(operand.has_value() && *operand == "%t7",
              "shared typed-call helper should recover a whitespace-tolerant single typed operand");

  expect_true(!parse_lir_single_typed_call_operand("(ptr)", "i32 %t7", "i32").has_value(),
              "shared typed-call helper should reject single-arg calls whose structured type does not match");
}

void test_lir_call_arg_helpers_decode_two_typed_operands() {
  using namespace c4c::codegen::lir;

  const auto operands = parse_lir_two_typed_call_operands(
      "( i32 , i32 )", "i32 %lhs, i32 %rhs", "i32", "i32");
  expect_true(operands.has_value() && operands->first == "%lhs" &&
                  operands->second == "%rhs",
              "shared typed-call helper should recover both operands from a spacing-tolerant two-argument call");

  expect_true(!parse_lir_two_typed_call_operands("(i32, ptr)",
                                                 "i32 %lhs, i32 %rhs",
                                                 "i32",
                                                 "i32")
                   .has_value(),
              "shared typed-call helper should reject two-arg calls whose structured parameter types drift");
}

void test_lir_call_arg_helpers_classify_call_callee_shape() {
  using namespace c4c::codegen::lir;

  const auto direct = parse_lir_call_callee(" @helper ");
  expect_true(direct.has_value() &&
                  direct->kind == LirCallCalleeKind::DirectGlobal &&
                  direct->symbol_name == "helper",
              "shared call-callee helper should classify direct global calls");

  const auto intrinsic = parse_lir_call_callee("@llvm.abs.i32");
  expect_true(intrinsic.has_value() &&
                  intrinsic->kind == LirCallCalleeKind::DirectIntrinsic &&
                  intrinsic->symbol_name == "llvm.abs.i32",
              "shared call-callee helper should classify llvm intrinsics separately");

  const auto indirect = parse_lir_call_callee("%fn.ptr");
  expect_true(indirect.has_value() &&
                  indirect->kind == LirCallCalleeKind::Indirect &&
                  indirect->symbol_name.empty(),
              "shared call-callee helper should classify indirect calls");

  expect_true(parse_lir_direct_global_callee("@helper").has_value() &&
                  *parse_lir_direct_global_callee("@helper") == "helper",
              "direct-global helper should recover the symbol name");
  expect_true(!parse_lir_direct_global_callee("@llvm.abs.i32").has_value(),
              "direct-global helper should reject llvm intrinsics");
}

void test_lir_call_arg_helpers_decode_direct_global_typed_call() {
  using namespace c4c::codegen::lir;

  const auto parsed = parse_lir_direct_global_typed_call(
      " @add_pair ", "( i32 , i32 )", " i32 %lhs , i32 %rhs ");
  expect_true(parsed.has_value() && parsed->symbol_name == "add_pair" &&
                  parsed->typed_call.args.size() == 2 &&
                  parsed->typed_call.args[0].operand == "%lhs" &&
                  parsed->typed_call.args[1].operand == "%rhs",
              "shared direct-global typed-call helper should recover the callee name and spacing-tolerant structured operands");

  expect_true(!parse_lir_direct_global_typed_call("@llvm.abs.i32", "(i32)", "i32 %x").has_value(),
              "shared direct-global typed-call helper should reject llvm intrinsics");
  expect_true(!parse_lir_direct_global_typed_call("%fp", "(i32)", "i32 %x").has_value(),
              "shared direct-global typed-call helper should reject indirect callees");
}

void test_lir_call_arg_helpers_decode_zero_arg_and_call_crossing_direct_calls() {
  using namespace c4c::codegen::lir;

  const auto zero_arg = parse_lir_direct_global_typed_call(" @helper ", " ( ) ", "  ");
  expect_true(zero_arg.has_value() && zero_arg->symbol_name == "helper" &&
                  zero_arg->typed_call.param_types.empty() &&
                  zero_arg->typed_call.args.empty(),
              "shared direct-global typed-call helper should preserve zero-argument direct-call shape");

  const auto call_crossing =
      parse_lir_direct_global_typed_call(" @add_one ", " ( i32 ) ", " i32 %saved ");
  expect_true(call_crossing.has_value() && call_crossing->symbol_name == "add_one" &&
                  call_crossing->typed_call.args.size() == 1 &&
                  call_crossing->typed_call.args.front().operand == "%saved",
              "shared direct-global typed-call helper should preserve single-operand call-crossing direct-call shape");
}

void test_lir_call_arg_helpers_format_typed_call_round_trip() {
  using namespace c4c::codegen::lir;

  const std::vector<OwnedLirTypedCallArg> args{
      {"ptr byval({ i32, i32 }) align 8", "%agg"},
      {"i32", "%t0"},
  };
  const std::vector<std::string> param_types{
      "ptr byval({ i32, i32 }) align 8",
      "i32",
  };

  const std::string formatted_args = format_lir_typed_call_args(args);
  const std::string formatted_params = format_lir_call_param_types(param_types);
  const auto parsed = parse_lir_typed_call(formatted_params, formatted_args);

  expect_true(parsed.has_value(),
              "shared typed-call formatting should round-trip through the structured parser");
  expect_true(parsed->args.size() == 2 && parsed->args[0].operand == "%agg" &&
                  parsed->args[1].operand == "%t0",
              "shared typed-call formatting should preserve the original operands");
}

void test_lir_call_arg_helpers_infer_param_types_when_suffix_missing() {
  using namespace c4c::codegen::lir;

  const auto parsed = parse_lir_typed_call_or_infer_params(
      "   ", " ptr %base , i32 7 ");
  expect_true(parsed.has_value() &&
                  parsed->param_types.size() == 2 &&
                  parsed->param_types[0] == "ptr" &&
                  parsed->param_types[1] == "i32" &&
                  parsed->args.size() == 2 &&
                  parsed->args[0].operand == "%base" &&
                  parsed->args[1].operand == "7",
              "shared typed-call parser should infer param types from typed args when compatibility suffixes are absent");

  expect_true(!parse_lir_typed_call_or_infer_params("(i32)", "ptr %base").has_value(),
              "shared typed-call parser should not infer param types when a non-empty suffix is malformed");
}

void test_lir_call_arg_helpers_format_full_call_site_round_trip() {
  using namespace c4c::codegen::lir;

  const std::vector<OwnedLirTypedCallArg> args{
      {"ptr", "%base"},
      {"i32", "%idx"},
  };
  const auto typed_call = format_lir_typed_call(args);
  const auto formatted = format_lir_call_site(
      " %fn.ptr ",
      std::string("  ") + typed_call.callee_type_suffix + " ",
      std::string("  ") + typed_call.args_str + " ");

  expect_true(formatted == "(ptr, i32) %fn.ptr(ptr %base, i32 %idx)",
              "shared full-call formatter should canonicalize typed indirect-call suffix and args");
}

void test_lir_call_arg_helpers_format_call_fields_with_suffix() {
  using namespace c4c::codegen::lir;

  const std::vector<OwnedLirTypedCallArg> args{
      {" ptr ", " %base "},
      {" i32 ", " %idx "},
  };
  const auto formatted = format_lir_call_fields(" ( ptr , i32 ) ", args);

  expect_true(formatted.callee_type_suffix == "(ptr, i32)",
              "shared call-field formatter should canonicalize typed call parameter suffixes");
  expect_true(formatted.args_str == "ptr %base, i32 %idx",
              "shared call-field formatter should canonicalize typed call arguments");
}

void test_lir_call_arg_helpers_make_typed_call_op() {
  using namespace c4c::codegen::lir;

  const auto call = make_lir_call_op(
      "%t0",
      " i32 ",
      " %fn.ptr ",
      " ( ptr , i32 ) ",
      {{" ptr ", " %base "}, {" i32 ", " %idx "}});

  expect_true(call.result == "%t0" && call.return_type == "i32" &&
                  call.callee == "%fn.ptr" &&
                  call.callee_type_suffix == "(ptr, i32)" &&
                  call.args_str == "ptr %base, i32 %idx",
              "shared typed-call builder should canonicalize call-site metadata into one LIR construction seam");
}

void test_lir_call_arg_helpers_make_direct_call_op_without_suffix() {
  using namespace c4c::codegen::lir;

  const auto call = make_lir_call_op("%t1",
                                     " ptr ",
                                     " @llvm.ptrmask.p0.i64 ",
                                     "",
                                     {{" ptr ", " %stack.plus "}, {" i64 ", " -16 "}});

  expect_true(call.result == "%t1" && call.return_type == "ptr" &&
                  call.callee == "@llvm.ptrmask.p0.i64" &&
                  call.callee_type_suffix.empty() &&
                  call.args_str == "ptr %stack.plus, i64 -16",
              "shared typed-call builder should canonicalize empty-suffix direct-call metadata for special intrinsic call sites");
}

void test_lir_call_arg_helpers_parse_and_format_call_op_views() {
  using namespace c4c::codegen::lir;

  const auto call = make_lir_call_op(
      "%t0",
      " i32 ",
      " @add_pair ",
      " ( i32 , i32 ) ",
      {{" i32 ", " %lhs "}, {" i32 ", " %rhs "}});
  const auto parsed = parse_lir_direct_global_typed_call(call);

  expect_true(parsed.has_value() && parsed->symbol_name == "add_pair" &&
                  parsed->typed_call.args.size() == 2 &&
                  parsed->typed_call.args[0].operand == "%lhs" &&
                  parsed->typed_call.args[1].operand == "%rhs",
              "shared call-op direct-global parser should recover structured call metadata from canonical LIR call storage");
  expect_true(format_lir_call_site(call) == "(i32, i32) @add_pair(i32 %lhs, i32 %rhs)",
              "shared call-op formatter should render canonical call text directly from LIR call storage");
}

void test_lir_call_arg_helpers_collect_call_op_global_refs() {
  using namespace c4c::codegen::lir;

  const LirCallOp call{
      "",
      "void",
      "@\"helper.func\"",
      "(ptr)",
      "ptr @\"global.data\"",
  };
  std::vector<std::string> refs;
  collect_lir_global_symbol_refs_from_call(
      call, [&](std::string_view ref) { refs.emplace_back(ref); });

  expect_true(refs.size() == 2 && refs[0] == "\"helper.func\"" &&
                  refs[1] == "\"global.data\"",
              "shared call-op global-ref collector should preserve quoted callee and argument symbol references for LIR persistence scans");
}

void test_backend_call_helpers_decode_structured_direct_global_call() {
  c4c::backend::BackendCallInst call{
      "%t0",
      "i32",
      c4c::backend::BackendCallCallee::direct_global("add_pair"),
      {"i32", "i32"},
      {{"i32", "%lhs"}, {"i32", "%rhs"}},
  };

  const auto parsed = c4c::backend::parse_backend_direct_global_typed_call(call);
  expect_true(parsed.has_value() && parsed->symbol_name == "add_pair" &&
                  parsed->typed_call.param_types.size() == 2 &&
                  parsed->typed_call.args.size() == 2 &&
                  parsed->typed_call.args[0].operand == "%lhs" &&
                  parsed->typed_call.args[1].operand == "%rhs",
              "backend-owned call helper should expose structured direct-call metadata without reparsing raw adapter text");
}

void test_backend_call_helpers_decode_structured_direct_global_vararg_prefix() {
  c4c::backend::BackendCallInst call{
      "%t0",
      "i32",
      c4c::backend::BackendCallCallee::direct_global("printf"),
      {"ptr"},
      {{"ptr", "@.str"}},
  };

  const auto parsed = c4c::backend::parse_backend_direct_global_typed_call(call);
  expect_true(parsed.has_value() && parsed->symbol_name == "printf" &&
                  parsed->typed_call.param_types.size() == 1 &&
                  parsed->typed_call.param_types.front() == "ptr" &&
                  parsed->typed_call.args.size() == 1 &&
                  parsed->typed_call.args.front().type == "ptr" &&
                  parsed->typed_call.args.front().operand == "@.str",
              "backend-owned direct-global helper should preserve the lowered fixed typed prefix for structured vararg calls");
}

void test_backend_call_helpers_decode_direct_global_single_typed_operand() {
  c4c::backend::BackendCallInst call{
      "%t0",
      "i32",
      c4c::backend::BackendCallCallee::direct_global("add_one"),
      {" i32 "},
      {{" i32 ", " %arg "}},
  };

  const auto operand = c4c::backend::parse_backend_direct_global_single_typed_call_operand(
      call, "add_one", "i32");
  expect_true(operand.has_value() && *operand == " %arg ",
              "backend-owned direct-global single-operand helper should preserve the borrowed typed operand");
  expect_true(!c4c::backend::parse_backend_direct_global_single_typed_call_operand(
                   call, "add_pair", "i32")
                   .has_value(),
              "backend-owned direct-global single-operand helper should reject mismatched callees");
}

void test_backend_call_helpers_decode_direct_global_two_typed_operands() {
  c4c::backend::BackendCallInst call{
      "%t1",
      "i32",
      c4c::backend::BackendCallCallee::direct_global("add_pair"),
      {"i32", "i32"},
      {{"i32", "%lhs"}, {"i32", "%rhs"}},
  };

  const auto operands = c4c::backend::parse_backend_direct_global_two_typed_call_operands(
      call, "add_pair", "i32", "i32");
  expect_true(operands.has_value() && operands->first == "%lhs" &&
                  operands->second == "%rhs",
              "backend-owned direct-global two-operand helper should expose the shared typed operands in order");
  expect_true(!c4c::backend::parse_backend_direct_global_two_typed_call_operands(
                   call, "add_pair", "i32", "ptr")
                   .has_value(),
              "backend-owned direct-global two-operand helper should reject mismatched typed signatures");
}

void test_backend_call_helpers_decode_zero_arg_direct_global_calls() {
  c4c::backend::BackendCallInst backend_call{
      "%t2",
      "i32",
      c4c::backend::BackendCallCallee::direct_global("helper"),
      {},
      {},
  };

  const auto backend_callee =
      c4c::backend::parse_backend_zero_arg_direct_global_typed_call(backend_call);
  expect_true(backend_callee.has_value() && *backend_callee == "helper",
              "backend-owned zero-arg direct-call helper should expose structured direct callees without repeating empty-shape checks");

  c4c::codegen::lir::LirCallOp lir_call{
      "%t3",
      "i32",
      "@helper",
      " ( ) ",
      "  ",
  };
  const auto lir_callee =
      c4c::backend::parse_backend_zero_arg_direct_global_typed_call(lir_call);
  expect_true(lir_callee.has_value() && *lir_callee == "helper",
              "shared zero-arg direct-call helper should decode spacing-tolerant LIR call metadata");

  backend_call.param_types = {"i32"};
  backend_call.args = {{"i32", "%arg"}};
  expect_true(!c4c::backend::parse_backend_zero_arg_direct_global_typed_call(backend_call)
                   .has_value(),
              "backend-owned zero-arg direct-call helper should reject non-empty structured argument shapes");
}

void test_backend_call_helpers_decode_lir_direct_global_typed_operands() {
  c4c::codegen::lir::LirCallOp single{
      "%t0",
      "i32",
      "@add_one",
      "( i32 )",
      " i32 %arg ",
  };
  const auto single_operand = c4c::backend::parse_backend_direct_global_single_typed_call_operand(
      single, "add_one", "i32");
  expect_true(single_operand.has_value() && *single_operand == "%arg",
              "shared direct-global single-operand helper should decode spacing-tolerant LIR call operands");
  expect_true(!c4c::backend::parse_backend_direct_global_single_typed_call_operand(
                   single, "add_pair", "i32")
                   .has_value(),
              "shared direct-global single-operand helper should reject mismatched LIR callees");

  c4c::codegen::lir::LirCallOp two{
      "%t1",
      "i32",
      "@add_pair",
      "( i32 , i32 )",
      " i32 %lhs , i32 %rhs ",
  };
  const auto two_operands = c4c::backend::parse_backend_direct_global_two_typed_call_operands(
      two, "add_pair", "i32", "i32");
  expect_true(two_operands.has_value() && two_operands->first == "%lhs" &&
                  two_operands->second == "%rhs",
              "shared direct-global two-operand helper should decode spacing-tolerant LIR call operands");
}

void test_backend_call_helpers_parse_single_helper_zero_arg_main_module_shape() {
  auto module = make_param_slot_runtime_module();
  std::swap(module.functions.front(), module.functions.back());

  auto& helper = module.functions.back();
  helper.name = "inc_value";
  helper.signature_text = "define i32 @inc_value(i32 %p.input)\n";
  auto& helper_alloca = std::get<c4c::codegen::lir::LirAllocaOp>(helper.alloca_insts.front());
  helper_alloca.result = "%lv.shadow.input";
  auto& helper_arg_store = std::get<c4c::codegen::lir::LirStoreOp>(helper.alloca_insts[1]);
  helper_arg_store.val = "%p.input";
  helper_arg_store.ptr = "%lv.shadow.input";

  auto& main_fn = module.functions.front();
  auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&main_fn.blocks.front().insts.front());
  expect_true(call != nullptr,
              "shared single-helper main-module parser regression test needs the direct call to mutate");
  if (call == nullptr) {
    return;
  }
  call->callee = c4c::codegen::lir::LirOperand(std::string("@inc_value"),
                                               c4c::codegen::lir::LirOperandKind::Global);

  const auto parsed =
      c4c::backend::parse_backend_single_helper_zero_arg_main_lir_module(module, 1);
  expect_true(parsed.has_value() && parsed->helper != nullptr &&
                  parsed->main_function != nullptr && parsed->main_block != nullptr &&
                  parsed->main_ret != nullptr,
              "shared single-helper main-module parser should identify the minimal helper/main module contract without target-local scans");
  expect_true(parsed.has_value() && parsed->helper->name == "inc_value" &&
                  parsed->main_function->name == "main" &&
                  parsed->main_block->insts.size() == 1 &&
                  parsed->main_ret->value_str.has_value() &&
                  *parsed->main_ret->value_str == "%t0",
              "shared single-helper main-module parser should preserve renamed helper metadata and remain independent from function ordering");
}

void test_backend_call_helpers_parse_single_helper_call_crossing_source_value() {
  auto module = make_typed_call_crossing_direct_call_module();
  std::swap(module.functions.front(), module.functions.back());

  auto& helper = module.functions.back();
  helper.name = "inc_value";
  helper.signature_text = "define i32 @inc_value(i32 %p.input) {\n";

  auto& main_fn = module.functions.front();
  auto& entry = main_fn.blocks.front();
  auto* source_add = std::get_if<c4c::codegen::lir::LirBinOp>(&entry.insts[0]);
  auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&entry.insts[1]);
  auto* final_add = std::get_if<c4c::codegen::lir::LirBinOp>(&entry.insts[2]);
  expect_true(source_add != nullptr && call != nullptr && final_add != nullptr,
              "shared call-crossing source parser regression test needs the source add, direct call, and final add");
  if (source_add == nullptr || call == nullptr || final_add == nullptr) {
    return;
  }

  source_add->result = "%t.crossing.source.renamed";
  call->result = "%t.crossing.call.renamed";
  call->callee = c4c::codegen::lir::LirOperand(std::string("@inc_value"),
                                               c4c::codegen::lir::LirOperandKind::Global);
  call->args_str = "i32 %t.crossing.source.renamed";
  final_add->lhs = "%t.crossing.source.renamed";
  final_add->rhs = "%t.crossing.call.renamed";
  final_add->result = "%t.crossing.sum.renamed";
  auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&entry.terminator);
  expect_true(ret != nullptr && ret->value_str.has_value(),
              "shared call-crossing source parser regression test needs the return to mutate");
  if (ret == nullptr || !ret->value_str.has_value()) {
    return;
  }
  ret->value_str = "%t.crossing.sum.renamed";

  const auto parsed = c4c::backend::parse_backend_single_helper_call_crossing_source_value(module);
  expect_true(parsed.has_value() && *parsed == "%t.crossing.source.renamed",
              "shared call-crossing source parser should preserve renamed helper symbols, function ordering, and source/call/return SSA names without target-local main scans");
}

void test_backend_call_helpers_parse_single_helper_direct_call() {
  auto param_slot = make_param_slot_runtime_module();
  std::swap(param_slot.functions.front(), param_slot.functions.back());

  auto& param_helper = param_slot.functions.back();
  param_helper.name = "inc_value";
  param_helper.signature_text = "define i32 @inc_value(i32 %p.input)\n";

  auto& param_main = param_slot.functions.front();
  auto* param_call =
      std::get_if<c4c::codegen::lir::LirCallOp>(&param_main.blocks.front().insts.front());
  expect_true(param_call != nullptr,
              "shared single-helper direct-call parser regression test needs the param-slot direct call");
  if (param_call == nullptr) {
    return;
  }
  param_call->result = "%t.param_slot.call.renamed";
  param_call->callee = c4c::codegen::lir::LirOperand(std::string("@inc_value"),
                                                     c4c::codegen::lir::LirOperandKind::Global);

  const auto parsed_param_module =
      c4c::backend::parse_backend_single_helper_zero_arg_main_lir_module(param_slot, 1);
  expect_true(parsed_param_module.has_value(),
              "shared single-helper direct-call parser regression test needs the param-slot helper/main module shape");
  if (!parsed_param_module.has_value()) {
    return;
  }

  const auto parsed_param_call =
      c4c::backend::parse_backend_single_helper_direct_global_call(*parsed_param_module, 0);
  expect_true(parsed_param_call.has_value() &&
                  parsed_param_call->callee_name == "inc_value" &&
                  parsed_param_call->operand == "5" &&
                  parsed_param_call->call != nullptr &&
                  parsed_param_call->call->result == "%t.param_slot.call.renamed",
              "shared single-helper direct-call parser should preserve renamed helper symbols and direct-call result names for the one-call param-slot module seam");

  auto call_crossing = make_typed_call_crossing_direct_call_module();
  std::swap(call_crossing.functions.front(), call_crossing.functions.back());

  auto& crossing_helper = call_crossing.functions.back();
  crossing_helper.name = "inc_value";
  crossing_helper.signature_text = "define i32 @inc_value(i32 %p.input)\n";

  auto& crossing_main = call_crossing.functions.front();
  auto& crossing_entry = crossing_main.blocks.front();
  auto* crossing_source_add =
      std::get_if<c4c::codegen::lir::LirBinOp>(&crossing_entry.insts.front());
  auto* crossing_call = std::get_if<c4c::codegen::lir::LirCallOp>(&crossing_entry.insts[1]);
  expect_true(crossing_source_add != nullptr && crossing_call != nullptr,
              "shared single-helper direct-call parser regression test needs the call-crossing source add and direct call");
  if (crossing_source_add == nullptr || crossing_call == nullptr) {
    return;
  }

  crossing_source_add->result = "%t.crossing.source.renamed";
  crossing_call->result = "%t.crossing.call.renamed";
  crossing_call->callee = c4c::codegen::lir::LirOperand(std::string("@inc_value"),
                                                        c4c::codegen::lir::LirOperandKind::Global);
  crossing_call->args_str = "i32 %t.crossing.source.renamed";

  const auto parsed_crossing_module =
      c4c::backend::parse_backend_single_helper_zero_arg_main_lir_module(call_crossing, 3);
  expect_true(parsed_crossing_module.has_value(),
              "shared single-helper direct-call parser regression test needs the call-crossing helper/main module shape");
  if (!parsed_crossing_module.has_value()) {
    return;
  }

  const auto parsed_crossing_call =
      c4c::backend::parse_backend_single_helper_direct_global_call(*parsed_crossing_module, 1);
  expect_true(parsed_crossing_call.has_value() &&
                  parsed_crossing_call->callee_name == "inc_value" &&
                  parsed_crossing_call->operand == "%t.crossing.source.renamed" &&
                  parsed_crossing_call->call != nullptr &&
                  parsed_crossing_call->call->result == "%t.crossing.call.renamed",
              "shared single-helper direct-call parser should preserve renamed helper symbols plus operand/result SSA names for the call-crossing seam without target-local scans");
}

void test_backend_call_helpers_parse_single_add_imm_function() {
  auto param_slot = make_param_slot_runtime_module();
  std::swap(param_slot.functions.front(), param_slot.functions.back());

  auto& slot_helper = param_slot.functions.back();
  slot_helper.name = "inc_value";
  slot_helper.signature_text = "define i32 @inc_value(i32 %p.input)\n";
  auto& slot_arg_store =
      std::get<c4c::codegen::lir::LirStoreOp>(slot_helper.alloca_insts[1]);
  slot_arg_store.val = "%p.input";

  const auto parsed_slot =
      c4c::backend::parse_backend_single_add_imm_function(slot_helper);
  expect_true(parsed_slot.has_value() && parsed_slot->param_name == "%p.input" &&
                  parsed_slot->add != nullptr && parsed_slot->add->rhs == "1",
              "shared single-add-immediate helper parser should decode slot-backed single-argument helpers without target-local helper-body scans");

  auto call_crossing = make_typed_call_crossing_direct_call_module();
  std::swap(call_crossing.functions.front(), call_crossing.functions.back());

  auto& helper = call_crossing.functions.back();
  helper.name = "inc_value";
  helper.signature_text = "define i32 @inc_value(i32 %p.input)\n";
  auto* add = helper.blocks.empty() || helper.blocks.front().insts.empty()
                  ? nullptr
                  : std::get_if<c4c::codegen::lir::LirBinOp>(&helper.blocks.front().insts.front());
  expect_true(add != nullptr,
              "shared single-add-immediate helper parser regression test needs the helper add instruction");
  if (add == nullptr) {
    return;
  }
  add->lhs = "%p.input";
  add->result = "%t.helper.add.renamed";
  auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&helper.blocks.front().terminator);
  expect_true(ret != nullptr && ret->value_str.has_value(),
              "shared single-add-immediate helper parser regression test needs the helper return");
  if (ret == nullptr || !ret->value_str.has_value()) {
    return;
  }
  ret->value_str = "%t.helper.add.renamed";

  const auto parsed_helper =
      c4c::backend::parse_backend_single_add_imm_function(helper);
  expect_true(parsed_helper.has_value() && parsed_helper->param_name == "%p.input" &&
                  parsed_helper->add != nullptr &&
                  parsed_helper->add->result == "%t.helper.add.renamed" &&
                  parsed_helper->add->rhs == "1",
              "shared single-add-immediate helper parser should preserve renamed helper symbols and helper SSA values for direct single-argument add helpers");
}

void test_backend_call_helpers_parse_two_param_add_function() {
  auto module = make_typed_direct_call_two_arg_module();
  auto& helper = module.functions.front();
  helper.name = "sum_pair";
  helper.signature_text = "define i32 @sum_pair(i32 %p.left, i32 %p.right)\n";

  auto* add = helper.blocks.empty() || helper.blocks.front().insts.empty()
                  ? nullptr
                  : std::get_if<c4c::codegen::lir::LirBinOp>(&helper.blocks.front().insts.front());
  expect_true(add != nullptr,
              "shared two-parameter add helper parser regression test needs the helper add instruction");
  if (add == nullptr) {
    return;
  }
  add->lhs = "%p.left";
  add->rhs = "%p.right";
  add->result = "%t.helper.sum.renamed";

  auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&helper.blocks.front().terminator);
  expect_true(ret != nullptr && ret->value_str.has_value(),
              "shared two-parameter add helper parser regression test needs the helper return");
  if (ret == nullptr || !ret->value_str.has_value()) {
    return;
  }
  ret->value_str = "%t.helper.sum.renamed";

  const auto parsed = c4c::backend::parse_backend_two_param_add_function(helper);
  expect_true(parsed.has_value() && parsed->lhs_param_name == "%p.left" &&
                  parsed->rhs_param_name == "%p.right" && parsed->add != nullptr &&
                  parsed->add->result == "%t.helper.sum.renamed",
              "shared two-parameter add helper parser should preserve renamed helper symbols, parameter names, and helper SSA values without target-local helper-body scans");
}

void test_backend_call_helpers_parse_minimal_two_arg_direct_call_lir_module() {
  auto module = make_typed_direct_call_two_arg_local_arg_module();
  std::swap(module.functions.front(), module.functions.back());

  c4c::codegen::lir::LirFunction* helper = nullptr;
  c4c::codegen::lir::LirFunction* main_fn = nullptr;
  for (auto& function : module.functions) {
    if (function.name == "add_pair") {
      helper = &function;
    } else if (function.name == "main") {
      main_fn = &function;
    }
  }
  expect_true(helper != nullptr && main_fn != nullptr,
              "shared two-argument minimal module parser regression test needs helper and main functions");
  if (helper == nullptr || main_fn == nullptr) {
    return;
  }

  helper->name = "sum_pair";
  helper->signature_text = "define i32 @sum_pair(i32 %p.left, i32 %p.right)\n";
  auto& helper_add = std::get<c4c::codegen::lir::LirBinOp>(helper->blocks.front().insts.front());
  helper_add.result = "%t.helper.sum";
  helper_add.lhs = "%p.left";
  helper_add.rhs = "%p.right";
  helper->blocks.front().terminator = c4c::codegen::lir::LirRet{std::string("%t.helper.sum"),
                                                                "i32"};

  auto& local = std::get<c4c::codegen::lir::LirAllocaOp>(main_fn->alloca_insts.front());
  local.result = "%lv.shadow.left";

  auto& store = std::get<c4c::codegen::lir::LirStoreOp>(main_fn->blocks.front().insts[0]);
  auto& load = std::get<c4c::codegen::lir::LirLoadOp>(main_fn->blocks.front().insts[1]);
  store.ptr = "%lv.shadow.left";
  load.result = "%t.call.left";
  load.ptr = "%lv.shadow.left";
  main_fn->blocks.front().insts.insert(
      main_fn->blocks.front().insts.begin() + 2,
      c4c::codegen::lir::LirLoadOp{"%t.unused.left", "i32", "%lv.shadow.left"});

  auto& call = std::get<c4c::codegen::lir::LirCallOp>(main_fn->blocks.front().insts.back());
  call.result = "%t.sum.result";
  call.callee = c4c::codegen::lir::LirOperand(std::string("@sum_pair"),
                                              c4c::codegen::lir::LirOperandKind::Global);
  call.args_str = "i32 %t.call.left, i32 7";
  main_fn->blocks.front().terminator =
      c4c::codegen::lir::LirRet{std::string("%t.sum.result"), "i32"};

  const auto parsed = c4c::backend::parse_backend_minimal_two_arg_direct_call_lir_module(module);
  expect_true(parsed.has_value() && parsed->helper != nullptr && parsed->main_function != nullptr &&
                  parsed->call != nullptr && parsed->helper->name == "sum_pair" &&
                  parsed->main_function->name == "main" &&
                  parsed->call->result == "%t.sum.result" &&
                  parsed->lhs_call_arg_imm == 5 && parsed->rhs_call_arg_imm == 7,
              "shared two-argument minimal module parser should preserve reordered helper discovery plus renamed local-slot call operands without target-local module-shape scans");
}

void test_backend_call_helpers_parse_structured_two_param_add_function() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_module());

  c4c::backend::BackendFunction* helper = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name != "main") {
      helper = &function;
      break;
    }
  }
  expect_true(helper != nullptr,
              "shared structured two-parameter add helper parser regression test needs the lowered helper function");
  if (helper == nullptr) {
    return;
  }

  helper->signature.name = "sum_pair";
  helper->signature.params[0].name = "%p.left";
  helper->signature.params[1].name = "%p.right";

  auto* add = helper->blocks.empty() || helper->blocks.front().insts.empty()
                  ? nullptr
                  : std::get_if<c4c::backend::BackendBinaryInst>(&helper->blocks.front().insts.front());
  expect_true(add != nullptr,
              "shared structured two-parameter add helper parser regression test needs the lowered helper add instruction");
  if (add == nullptr) {
    return;
  }
  add->lhs = "%p.left";
  add->rhs = "%p.right";
  add->result = "%t.helper.sum.structured";

  helper->blocks.front().terminator.value = "%t.helper.sum.structured";

  const auto parsed = c4c::backend::parse_backend_structured_two_param_add_function(*helper);
  expect_true(parsed.has_value() && parsed->lhs_param_name == "%p.left" &&
                  parsed->rhs_param_name == "%p.right" && parsed->add != nullptr &&
                  parsed->add->result == "%t.helper.sum.structured",
              "shared structured two-parameter add helper parser should preserve renamed structured parameter names and helper SSA values without target-local helper-body scans");
}

void test_backend_call_helpers_parse_structured_zero_arg_return_imm_function() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_direct_call_module());

  c4c::backend::BackendFunction* helper = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name != "main") {
      helper = &function;
      break;
    }
  }
  expect_true(helper != nullptr,
              "shared structured zero-argument return helper parser regression test needs the lowered helper function");
  if (helper == nullptr) {
    return;
  }

  helper->signature.name = "const_value";
  helper->blocks.front().terminator.value = "9";

  const auto parsed =
      c4c::backend::parse_backend_structured_zero_arg_return_imm_function(*helper);
  expect_true(parsed.has_value() && parsed->return_imm == 9,
              "shared structured zero-argument return helper parser should preserve renamed helper symbols and decode the immediate return contract without target-local helper-body scans");
}

void test_backend_call_helpers_parse_structured_single_add_imm_function() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_local_arg_module());

  c4c::backend::BackendFunction* helper = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name != "main") {
      helper = &function;
      break;
    }
  }
  expect_true(helper != nullptr,
              "shared structured single-add-immediate helper parser regression test needs the lowered helper function");
  if (helper == nullptr) {
    return;
  }

  helper->signature.name = "sum_one";
  helper->signature.params.front().name = "%p.input";
  auto* add = helper->blocks.empty() || helper->blocks.front().insts.empty()
                  ? nullptr
                  : std::get_if<c4c::backend::BackendBinaryInst>(&helper->blocks.front().insts.front());
  expect_true(add != nullptr,
              "shared structured single-add-immediate helper parser regression test needs the lowered helper add instruction");
  if (add == nullptr) {
    return;
  }
  add->lhs = "%p.input";
  add->result = "%t.helper.add.structured";
  helper->blocks.front().terminator.value = "%t.helper.add.structured";

  const auto parsed =
      c4c::backend::parse_backend_structured_single_add_imm_function(*helper);
  expect_true(parsed.has_value() && parsed->param_name == "%p.input" &&
                  parsed->add != nullptr &&
                  parsed->add->result == "%t.helper.add.structured" &&
                  parsed->add->rhs == "1",
              "shared structured single-add-immediate helper parser should preserve renamed structured parameter names and helper SSA values without target-local helper-body scans");
}

void test_backend_call_helpers_parse_structured_zero_arg_direct_call_module() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_direct_call_module());

  c4c::backend::BackendFunction* helper = nullptr;
  c4c::backend::BackendFunction* main_fn = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "main") {
      main_fn = &function;
    } else {
      helper = &function;
    }
  }
  expect_true(helper != nullptr && main_fn != nullptr,
              "shared structured zero-argument direct-call module parser regression test needs lowered helper and main functions");
  if (helper == nullptr || main_fn == nullptr) {
    return;
  }

  helper->signature.name = "const_value";
  helper->blocks.front().terminator.value = "9";

  auto* call =
      main_fn->blocks.empty() || main_fn->blocks.front().insts.empty()
          ? nullptr
          : std::get_if<c4c::backend::BackendCallInst>(&main_fn->blocks.front().insts.front());
  expect_true(call != nullptr,
              "shared structured zero-argument direct-call module parser regression test needs the lowered main call instruction");
  if (call == nullptr) {
    return;
  }

  call->callee.symbol_name = "const_value";
  call->result = "%t.main.const.structured";
  main_fn->blocks.front().terminator.value = "%t.main.const.structured";

  const auto parsed = c4c::backend::parse_backend_minimal_direct_call_module(lowered);
  expect_true(parsed.has_value() && parsed->helper != nullptr && parsed->main_function != nullptr &&
                  parsed->call != nullptr && parsed->helper->signature.name == "const_value" &&
                  parsed->main_function->signature.name == "main" &&
                  parsed->call->result == "%t.main.const.structured" &&
                  parsed->return_imm == 9,
              "shared structured zero-argument direct-call module parser should preserve renamed helper symbols and return immediates without target-local backend-module scans");
}

void test_backend_call_helpers_parse_structured_single_add_imm_direct_call_module() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_local_arg_module());

  c4c::backend::BackendFunction* helper = nullptr;
  c4c::backend::BackendFunction* main_fn = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "main") {
      main_fn = &function;
    } else {
      helper = &function;
    }
  }
  expect_true(helper != nullptr && main_fn != nullptr,
              "shared structured single-add-immediate direct-call module parser regression test needs lowered helper and main functions");
  if (helper == nullptr || main_fn == nullptr) {
    return;
  }

  helper->signature.name = "sum_one";
  helper->signature.params.front().name = "%p.input";
  auto* helper_add = helper->blocks.empty() || helper->blocks.front().insts.empty()
                         ? nullptr
                         : std::get_if<c4c::backend::BackendBinaryInst>(&helper->blocks.front().insts.front());
  auto* call =
      main_fn->blocks.empty() || main_fn->blocks.front().insts.empty()
          ? nullptr
          : std::get_if<c4c::backend::BackendCallInst>(&main_fn->blocks.front().insts.front());
  expect_true(helper_add != nullptr && call != nullptr,
              "shared structured single-add-immediate direct-call module parser regression test needs mutable helper add and main call instructions");
  if (helper_add == nullptr || call == nullptr) {
    return;
  }

  helper_add->lhs = "%p.input";
  helper_add->result = "%t.helper.add.structured";
  helper->blocks.front().terminator.value = "%t.helper.add.structured";

  call->callee.symbol_name = "sum_one";
  call->args.front().operand = "17";
  call->result = "%t.main.sum_one.structured";
  main_fn->blocks.front().terminator.value = "%t.main.sum_one.structured";

  const auto parsed = c4c::backend::parse_backend_minimal_direct_call_add_imm_module(lowered);
  expect_true(parsed.has_value() && parsed->helper != nullptr && parsed->main_function != nullptr &&
                  parsed->call != nullptr && parsed->helper->signature.name == "sum_one" &&
                  parsed->main_function->signature.name == "main" &&
                  parsed->call->result == "%t.main.sum_one.structured" &&
                  parsed->call_arg_imm == 17 && parsed->add_imm == 1,
              "shared structured single-add-immediate direct-call module parser should preserve renamed helper symbols and direct-call immediates without target-local backend-module scans");
}

void test_backend_call_helpers_parse_param_slot_runtime_as_structured_single_add_imm_direct_call() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_param_slot_runtime_module());

  const auto parsed = c4c::backend::parse_backend_minimal_direct_call_add_imm_module(lowered);
  expect_true(parsed.has_value() && parsed->helper != nullptr && parsed->main_function != nullptr &&
                  parsed->call != nullptr &&
                  parsed->helper->signature.name == "add_one" &&
                  parsed->main_function->signature.name == "main" &&
                  parsed->call_arg_imm == 5 && parsed->add_imm == 1,
              "shared structured single-add-immediate direct-call module parser should classify lowered param-slot runtime helpers without x86-local compatibility parsing");
}

void test_backend_call_helpers_parse_structured_call_crossing_direct_call_module() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_typed_call_crossing_direct_call_module());

  c4c::backend::BackendFunction* helper = nullptr;
  c4c::backend::BackendFunction* main_fn = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "main") {
      main_fn = &function;
    } else {
      helper = &function;
    }
  }
  expect_true(helper != nullptr && main_fn != nullptr,
              "shared structured call-crossing direct-call module parser regression test needs lowered helper and main functions");
  if (helper == nullptr || main_fn == nullptr || main_fn->blocks.empty() ||
      main_fn->blocks.front().insts.size() != 3) {
    return;
  }

  helper->signature.name = "inc_value";
  helper->signature.params.front().name = "%p.input";
  auto* helper_add = helper->blocks.front().insts.empty()
                         ? nullptr
                         : std::get_if<c4c::backend::BackendBinaryInst>(&helper->blocks.front().insts.front());
  auto& entry = main_fn->blocks.front();
  auto* source_add = std::get_if<c4c::backend::BackendBinaryInst>(&entry.insts[0]);
  auto* call = std::get_if<c4c::backend::BackendCallInst>(&entry.insts[1]);
  auto* final_add = std::get_if<c4c::backend::BackendBinaryInst>(&entry.insts[2]);
  expect_true(helper_add != nullptr && source_add != nullptr && call != nullptr &&
                  final_add != nullptr,
              "shared structured call-crossing direct-call module parser regression test needs mutable helper/body and main seam instructions");
  if (helper_add == nullptr || source_add == nullptr || call == nullptr || final_add == nullptr) {
    return;
  }

  helper_add->lhs = "%p.input";
  helper_add->result = "%t.helper.add.structured";
  helper->blocks.front().terminator.value = "%t.helper.add.structured";

  source_add->result = "%t.crossing.source.structured";
  call->callee.symbol_name = "inc_value";
  call->args.front().operand = "%t.crossing.source.structured";
  call->result = "%t.crossing.call.structured";
  final_add->lhs = "%t.crossing.source.structured";
  final_add->rhs = "%t.crossing.call.structured";
  final_add->result = "%t.crossing.sum.structured";
  entry.terminator.value = "%t.crossing.sum.structured";

  const auto parsed =
      c4c::backend::parse_backend_minimal_call_crossing_direct_call_module(lowered);
  expect_true(parsed.has_value() && parsed->helper != nullptr && parsed->main_function != nullptr &&
                  parsed->source_add != nullptr && parsed->call != nullptr &&
                  parsed->final_add != nullptr &&
                  parsed->helper->signature.name == "inc_value" &&
                  parsed->main_function->signature.name == "main" &&
                  parsed->source_add->result == "%t.crossing.source.structured" &&
                  parsed->call->result == "%t.crossing.call.structured" &&
                  parsed->final_add->result == "%t.crossing.sum.structured" &&
                  parsed->source_imm == 5 && parsed->helper_add_imm == 1,
              "shared structured call-crossing direct-call module parser should preserve renamed helper symbols, SSA names, and summed source immediates without target-local backend-module scans");
}

void test_backend_call_helpers_parse_structured_folded_two_arg_function() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_module());

  c4c::backend::BackendFunction* helper = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name != "main") {
      helper = &function;
      break;
    }
  }
  expect_true(helper != nullptr,
              "shared structured folded two-argument helper parser regression test needs the lowered helper function");
  if (helper == nullptr) {
    return;
  }

  helper->signature.name = "const_pair";
  helper->signature.params[0].name = "%p.left";
  helper->signature.params[1].name = "%p.right";

  auto* add = helper->blocks.empty() || helper->blocks.front().insts.size() < 2
                  ? nullptr
                  : std::get_if<c4c::backend::BackendBinaryInst>(&helper->blocks.front().insts[0]);
  auto* sub = helper->blocks.empty() || helper->blocks.front().insts.size() < 2
                  ? nullptr
                  : std::get_if<c4c::backend::BackendBinaryInst>(&helper->blocks.front().insts[1]);
  if (add == nullptr || sub == nullptr) {
    auto* entry = helper->blocks.empty() ? nullptr : &helper->blocks.front();
    expect_true(entry != nullptr,
                "shared structured folded two-argument helper parser regression test needs the lowered helper entry block");
    if (entry == nullptr) {
      return;
    }

    entry->insts.push_back(c4c::backend::BackendBinaryInst{
        c4c::backend::BackendBinaryOpcode::Sub,
        "%t.helper.folded.sub",
        "i32",
        "%t.helper.folded.add",
        "%p.right",
        c4c::backend::BackendScalarType::I32,
    });

    add = std::get_if<c4c::backend::BackendBinaryInst>(&entry->insts[0]);
    sub = std::get_if<c4c::backend::BackendBinaryInst>(&entry->insts[1]);
    expect_true(add != nullptr && sub != nullptr,
                "shared structured folded two-argument helper parser regression test needs mutable lowered helper add/sub instructions");
    if (add == nullptr || sub == nullptr) {
      return;
    }
  }

  add->lhs = "9";
  add->rhs = "%p.left";
  add->result = "%t.helper.folded.add";
  sub->lhs = "%t.helper.folded.add";
  sub->rhs = "%p.right";
  sub->result = "%t.helper.folded.sub";
  helper->blocks.front().terminator.value = "%t.helper.folded.sub";

  const auto parsed =
      c4c::backend::parse_backend_structured_folded_two_arg_function(*helper);
  expect_true(parsed.has_value() && parsed->lhs_param_name == "%p.left" &&
                  parsed->rhs_param_name == "%p.right" && parsed->base_imm == 9 &&
                  parsed->add != nullptr && parsed->sub != nullptr &&
                  parsed->add->result == "%t.helper.folded.add" &&
                  parsed->sub->result == "%t.helper.folded.sub",
              "shared structured folded two-argument helper parser should preserve renamed parameter names, helper SSA values, and folded base immediates without target-local helper-body scans");
}

void test_backend_call_helpers_parse_structured_two_arg_direct_call_module() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_module());

  c4c::backend::BackendFunction* helper = nullptr;
  c4c::backend::BackendFunction* main_fn = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "add_pair") {
      helper = &function;
    } else if (function.signature.name == "main") {
      main_fn = &function;
    }
  }
  expect_true(helper != nullptr && main_fn != nullptr,
              "shared structured two-argument direct-call module parser regression test needs lowered helper and main functions");
  if (helper == nullptr || main_fn == nullptr) {
    return;
  }

  helper->signature.name = "sum_pair";
  helper->signature.params[0].name = "%p.left";
  helper->signature.params[1].name = "%p.right";

  auto* helper_add = helper->blocks.empty() || helper->blocks.front().insts.empty()
                         ? nullptr
                         : std::get_if<c4c::backend::BackendBinaryInst>(&helper->blocks.front().insts.front());
  auto* call =
      main_fn->blocks.empty() || main_fn->blocks.front().insts.empty()
          ? nullptr
          : std::get_if<c4c::backend::BackendCallInst>(&main_fn->blocks.front().insts.front());
  expect_true(helper_add != nullptr && call != nullptr,
              "shared structured two-argument direct-call module parser regression test needs mutable helper add and main call instructions");
  if (helper_add == nullptr || call == nullptr) {
    return;
  }

  helper_add->lhs = "%p.left";
  helper_add->rhs = "%p.right";
  helper_add->result = "%t.helper.sum.structured";
  helper->blocks.front().terminator.value = "%t.helper.sum.structured";

  call->callee.symbol_name = "sum_pair";
  call->args[0].operand = "11";
  call->args[1].operand = "13";
  call->result = "%t.main.sum.structured";
  main_fn->blocks.front().terminator.value = "%t.main.sum.structured";

  const auto parsed = c4c::backend::parse_backend_minimal_two_arg_direct_call_module(lowered);
  expect_true(parsed.has_value() && parsed->helper != nullptr && parsed->main_function != nullptr &&
                  parsed->call != nullptr && parsed->helper->signature.name == "sum_pair" &&
                  parsed->main_function->signature.name == "main" &&
                  parsed->call->result == "%t.main.sum.structured" &&
                  parsed->lhs_call_arg_imm == 11 && parsed->rhs_call_arg_imm == 13,
              "shared structured two-argument direct-call module parser should preserve renamed helper symbols and direct-call immediates without target-local backend-module scans");
}

void test_backend_call_helpers_match_structured_direct_call_module() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_module());

  c4c::backend::BackendFunction* helper = nullptr;
  c4c::backend::BackendFunction* main_fn = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "main") {
      main_fn = &function;
    } else {
      helper = &function;
    }
  }
  expect_true(helper != nullptr && main_fn != nullptr,
              "shared structured direct-call matcher regression test needs lowered helper and main functions");
  if (helper == nullptr || main_fn == nullptr || main_fn->blocks.empty() ||
      main_fn->blocks.front().insts.empty()) {
    return;
  }

  helper->signature.name = "sum_pair";
  helper->signature.params[0].name = "%p.left";
  helper->signature.params[1].name = "%p.right";

  auto* call =
      std::get_if<c4c::backend::BackendCallInst>(&main_fn->blocks.front().insts.front());
  expect_true(call != nullptr,
              "shared structured direct-call matcher regression test needs the lowered main call instruction");
  if (call == nullptr) {
    return;
  }

  call->callee.symbol_name = "sum_pair";
  call->args[0].operand = "11";
  call->args[1].operand = "13";
  call->result = "%t.main.sum_pair.structured";
  main_fn->blocks.front().terminator.value = "%t.main.sum_pair.structured";

  const auto parsed =
      c4c::backend::parse_backend_minimal_structured_direct_call_module(lowered, 2);
  expect_true(parsed.has_value() && parsed->helper != nullptr && parsed->main_function != nullptr &&
                  parsed->main_block != nullptr && parsed->call != nullptr &&
                  parsed->helper->signature.name == "sum_pair" &&
                  parsed->helper->signature.params[0].name == "%p.left" &&
                  parsed->helper->signature.params[1].name == "%p.right" &&
                  parsed->main_function->signature.name == "main" &&
                  parsed->call->result == "%t.main.sum_pair.structured" &&
                  parsed->parsed_call.symbol_name == "sum_pair" &&
                  parsed->parsed_call.typed_call.args.size() == 2 &&
                  parsed->parsed_call.typed_call.args[0].operand == "11" &&
                  parsed->parsed_call.typed_call.args[1].operand == "13",
              "shared structured direct-call matcher should preserve renamed helper symbols, parameters, and call operands without target-local backend-module scans");
}

void test_backend_call_helpers_parse_structured_folded_two_arg_direct_call_module() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_module());

  c4c::backend::BackendFunction* helper = nullptr;
  c4c::backend::BackendFunction* main_fn = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "main") {
      main_fn = &function;
    } else {
      helper = &function;
    }
  }
  expect_true(helper != nullptr && main_fn != nullptr,
              "shared structured folded two-argument direct-call module parser regression test needs lowered helper and main functions");
  if (helper == nullptr || main_fn == nullptr || helper->blocks.empty() ||
      main_fn->blocks.empty()) {
    return;
  }

  helper->signature.name = "const_pair";
  helper->signature.params[0].name = "%p.left";
  helper->signature.params[1].name = "%p.right";

  auto& helper_entry = helper->blocks.front();
  if (helper_entry.insts.size() < 2) {
    helper_entry.insts.push_back(c4c::backend::BackendBinaryInst{
        c4c::backend::BackendBinaryOpcode::Sub,
        "%t.helper.folded.sub",
        "i32",
        "%t.helper.folded.add",
        "%p.right",
        c4c::backend::BackendScalarType::I32,
    });
  }

  auto* add = helper_entry.insts.empty()
                  ? nullptr
                  : std::get_if<c4c::backend::BackendBinaryInst>(&helper_entry.insts[0]);
  auto* sub = helper_entry.insts.size() < 2
                  ? nullptr
                  : std::get_if<c4c::backend::BackendBinaryInst>(&helper_entry.insts[1]);
  auto* call =
      main_fn->blocks.front().insts.empty()
          ? nullptr
          : std::get_if<c4c::backend::BackendCallInst>(&main_fn->blocks.front().insts.front());
  expect_true(add != nullptr && sub != nullptr && call != nullptr,
              "shared structured folded two-argument direct-call module parser regression test needs mutable helper add/sub and main call instructions");
  if (add == nullptr || sub == nullptr || call == nullptr) {
    return;
  }

  add->lhs = "9";
  add->rhs = "%p.left";
  add->result = "%t.helper.folded.add";
  sub->lhs = "%t.helper.folded.add";
  sub->rhs = "%p.right";
  sub->result = "%t.helper.folded.sub";
  helper_entry.terminator.value = "%t.helper.folded.sub";

  call->callee.symbol_name = "const_pair";
  call->args[0].operand = "11";
  call->args[1].operand = "13";
  call->result = "%t.main.const_pair.structured";
  main_fn->blocks.front().terminator.value = "%t.main.const_pair.structured";

  const auto parsed =
      c4c::backend::parse_backend_minimal_folded_two_arg_direct_call_module(lowered);
  expect_true(parsed.has_value() && parsed->helper != nullptr && parsed->main_function != nullptr &&
                  parsed->call != nullptr && parsed->helper->signature.name == "const_pair" &&
                  parsed->helper->signature.params[0].name == "%p.left" &&
                  parsed->helper->signature.params[1].name == "%p.right" &&
                  parsed->main_function->signature.name == "main" &&
                  parsed->call->result == "%t.main.const_pair.structured" &&
                  parsed->lhs_call_arg_imm == 11 && parsed->rhs_call_arg_imm == 13 &&
                  parsed->return_imm == 7,
              "shared structured folded two-argument direct-call module parser should preserve renamed helper symbols, parameters, direct-call operands, and folded return immediates without target-local helper-body scans");
}

void test_backend_call_helpers_parse_structured_declared_direct_call_module() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_x86_extern_decl_inferred_param_module());

  c4c::backend::BackendFunction* decl = nullptr;
  c4c::backend::BackendFunction* main_fn = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "main") {
      main_fn = &function;
    } else {
      decl = &function;
    }
  }
  expect_true(decl != nullptr && main_fn != nullptr,
              "shared structured declared direct-call module parser regression test needs lowered declaration and main functions");
  if (decl == nullptr || main_fn == nullptr || main_fn->blocks.empty() ||
      main_fn->blocks.front().insts.empty()) {
    return;
  }

  decl->signature.name = "puts_like";
  decl->signature.params[0].name = "%p.left";
  decl->signature.params[1].name = "%p.right";
  decl->signature.is_vararg = true;

  auto* call =
      std::get_if<c4c::backend::BackendCallInst>(&main_fn->blocks.front().insts.front());
  expect_true(call != nullptr,
              "shared structured declared direct-call module parser regression test needs the lowered main call instruction");
  if (call == nullptr) {
    return;
  }

  call->callee.symbol_name = "puts_like";
  call->result = "%t.main.puts_like.structured";
  call->args[0].operand = "17";
  call->args[1].operand = "23";
  main_fn->blocks.front().terminator.value = "29";

  const auto parsed = c4c::backend::parse_backend_minimal_declared_direct_call_module(lowered);
  expect_true(parsed.has_value() && parsed->callee != nullptr && parsed->main_function != nullptr &&
                  parsed->main_block != nullptr && parsed->call != nullptr &&
                  parsed->callee->signature.name == "puts_like" &&
                  parsed->callee->signature.is_vararg &&
                  parsed->main_function->signature.name == "main" &&
                  parsed->call->result == "%t.main.puts_like.structured" &&
                  parsed->parsed_call.symbol_name == "puts_like" &&
                  parsed->parsed_call.typed_call.args.size() == 2 &&
                  parsed->parsed_call.typed_call.args[0].operand == "17" &&
                  parsed->parsed_call.typed_call.args[1].operand == "23" &&
                  parsed->args.size() == 2 &&
                  parsed->args[0].kind == c4c::backend::ParsedBackendExternCallArg::Kind::I32Imm &&
                  parsed->args[0].imm == 17 &&
                  parsed->args[1].kind == c4c::backend::ParsedBackendExternCallArg::Kind::I32Imm &&
                  parsed->args[1].imm == 23 &&
                  !parsed->return_call_result && parsed->return_imm == 29,
              "shared structured declared direct-call module parser should preserve renamed declaration symbols, typed operands, vararg signatures, and fixed return immediates without target-local backend-module scans");
}

void test_backend_call_helpers_decode_lir_direct_global_vararg_prefix() {
  c4c::codegen::lir::LirCallOp call{
      "%t2",
      "i32",
      "@printf",
      "( ptr , ... )",
      " ptr @.str , double %x ",
  };

  const auto parsed = c4c::backend::parse_backend_direct_global_typed_call(call);
  expect_true(parsed.has_value() && parsed->symbol_name == "printf" &&
                  parsed->typed_call.param_types.size() == 1 &&
                  parsed->typed_call.param_types.front() == "ptr" &&
                  parsed->typed_call.args.size() == 1 &&
                  parsed->typed_call.args.front().type == "ptr" &&
                  parsed->typed_call.args.front().operand == "@.str",
              "shared direct-global helper should preserve the fixed typed prefix for vararg LIR calls");
}

void test_backend_call_helpers_classify_lir_nonminimal_call_types() {
  c4c::codegen::lir::LirCallOp i32_call{
      "%t0",
      "i32",
      "@add_one",
      " ( i32 ) ",
      " i32 %arg ",
  };
  expect_true(!c4c::backend::backend_lir_call_uses_nonminimal_types(i32_call),
              "lowering-owned nonminimal call classifier should keep plain i32 call metadata on the minimal path");

  c4c::codegen::lir::LirCallOp i64_call{
      "%t1",
      "i32",
      "@wide",
      " ( i64 ) ",
      " i64 %wide ",
  };
  expect_true(c4c::backend::backend_lir_call_uses_nonminimal_types(i64_call),
              "lowering-owned nonminimal call classifier should reject wide integer call metadata through the shared decode seam");

  c4c::codegen::lir::LirCallOp malformed_suffix{
      "%t2",
      "i32",
      "@wide",
      " ( i64 ",
      " i32 %arg ",
  };
  expect_true(c4c::backend::backend_lir_call_uses_nonminimal_types(malformed_suffix),
              "lowering-owned nonminimal call classifier should preserve the conservative fallback when call metadata does not parse cleanly");
}

void test_backend_call_helpers_classify_lir_nonminimal_signature_types() {
  expect_true(
      !c4c::backend::backend_lir_function_signature_uses_nonminimal_types(
          "define i32 @helper(i32 %arg)\n"),
      "lowering-owned nonminimal signature classifier should keep plain i32 helpers on the minimal path");

  expect_true(
      c4c::backend::backend_lir_function_signature_uses_nonminimal_types(
          "define i32 @helper(i64 %arg)\n"),
      "lowering-owned nonminimal signature classifier should reject wide integer parameter signatures through the shared decode seam");

  expect_true(
      c4c::backend::backend_lir_function_signature_uses_nonminimal_types(
          "define double @helper(i32 %arg)\n"),
      "lowering-owned nonminimal signature classifier should reject nonminimal return types through the shared decode seam");
}

void test_backend_call_helpers_classify_i32_main_signature_shapes() {
  expect_true(
      c4c::backend::backend_lir_is_i32_main_definition(
          "define i32 @main(i32 %argc, ptr %argv)\n"),
      "lowering-owned main-signature classifier should recognize i32 main definitions without target-local text scans");

  expect_true(
      c4c::backend::backend_lir_is_zero_arg_i32_main_definition(
          " define  i32   @main( ) \n"),
      "lowering-owned zero-arg main classifier should tolerate compatibility spacing while keeping raw signature parsing centralized");

  expect_true(
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(
          "define i32 @main(i32 %argc)\n"),
      "lowering-owned zero-arg main classifier should reject signatures that still declare parameters");

  expect_true(
      !c4c::backend::backend_lir_is_i32_main_definition(
          "declare i32 @main()\n"),
      "lowering-owned main-signature classifier should reject declarations");

  expect_true(
      !c4c::backend::backend_lir_is_i32_main_definition(
          "define i64 @main()\n"),
      "lowering-owned main-signature classifier should reject non-i32 return signatures");
}

void test_backend_call_helpers_match_lir_helper_signature_shapes() {
  expect_true(
      c4c::backend::backend_lir_signature_matches(
          " define  i32  @add_one( i32 %p.x ) \n",
          "define",
          "i32",
          "add_one",
          {"i32"}),
      "lowering-owned helper-signature matcher should keep spacing-tolerant single-parameter helpers on the shared decode seam");

  expect_true(
      c4c::backend::backend_lir_signature_matches(
          "define i32 @add_pair(i32 %p.x, i32 %p.y)\n",
          "define",
          "i32",
          "add_pair",
          {"i32", "i32"}),
      "lowering-owned helper-signature matcher should preserve exact two-parameter helper matching without target-local string equality");

  expect_true(
      !c4c::backend::backend_lir_signature_matches(
          "define i32 @add_pair(i32 %p.x, ptr %p.y)\n",
          "define",
          "i32",
          "add_pair",
          {"i32", "i32"}),
              "lowering-owned helper-signature matcher should reject helper signatures whose parameter types drift");
}

void test_backend_call_helpers_decode_signature_params_with_spacing_and_varargs() {
  const auto params = c4c::backend::parse_backend_function_signature_params(
      " define i32 @pair_sum( i32 %p.seed , ptr %p.buf , ... ) \n");
  expect_true(params.has_value() && params->size() == 3 &&
                  !(*params)[0].is_varargs && (*params)[0].type == "i32" &&
                  (*params)[0].operand == "%p.seed" && !(*params)[1].is_varargs &&
                  (*params)[1].type == "ptr" && (*params)[1].operand == "%p.buf" &&
                  (*params)[2].is_varargs,
              "lowering-owned signature-param decoder should preserve spacing-tolerant fixed params and trailing varargs on the shared decode seam");
}

void test_backend_call_helpers_classify_lir_nonminimal_global_and_return_types() {
  c4c::codegen::lir::LirGlobal i32_global{};
  i32_global.llvm_type = "i32";
  i32_global.init_text = "0";
  expect_true(
      !c4c::backend::backend_lir_global_uses_nonminimal_types(i32_global),
      "lowering-owned nonminimal global classifier should keep plain i32 globals on the minimal path");

  c4c::codegen::lir::LirGlobal fp_global{};
  fp_global.llvm_type = "[2 x i8]";
  fp_global.init_text = "double 1.0";
  expect_true(
      c4c::backend::backend_lir_global_uses_nonminimal_types(fp_global),
      "lowering-owned nonminimal global classifier should reject globals whose initializer still carries floating-point text");

  c4c::codegen::lir::LirRet i32_ret{};
  i32_ret.type_str = "i32";
  expect_true(
      !c4c::backend::backend_lir_return_uses_nonminimal_types(i32_ret),
      "lowering-owned nonminimal return classifier should keep plain i32 returns on the minimal path");

  c4c::codegen::lir::LirRet i64_ret{};
  i64_ret.type_str = "i64";
  expect_true(
      c4c::backend::backend_lir_return_uses_nonminimal_types(i64_ret),
      "lowering-owned nonminimal return classifier should reject wide integer returns through the shared decode seam");
}

void test_backend_call_helpers_decode_single_typed_local_operand() {
  auto module = make_typed_direct_call_local_arg_with_suffix_spacing_module();
  const auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.back());

  const auto operand = c4c::backend::parse_backend_single_typed_call_operand(call, "i32");
  expect_true(operand.has_value() && *operand == "%t0",
              "shared typed-call helper should decode spacing-tolerant local-call operands without repeating parsed single-arg shape checks");
}

void test_backend_call_helpers_decode_owned_typed_args_from_lir_call() {
  auto module = make_typed_direct_call_local_arg_with_suffix_spacing_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.back());
  call.args_str = "  i32   %t0 ,   i32  7  ";

  const auto args = c4c::backend::parse_backend_owned_typed_call_args(call);
  expect_true(args.has_value() && args->size() == 2 && (*args)[0].type == "i32" &&
                  (*args)[0].operand == "%t0" && (*args)[1].type == "i32" &&
                  (*args)[1].operand == "7",
              "lowering-owned typed-call decoder should materialize owned LIR call args from the shared call helper instead of target-local args_str parsing");
}

void test_backend_call_helpers_decode_two_typed_local_operands() {
  auto module = make_typed_direct_call_two_arg_local_arg_with_spacing_module();
  const auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.back());

  const auto operands = c4c::backend::parse_backend_two_typed_call_operands(
      call, "i32", "i32");
  expect_true(operands.has_value() && operands->first == "%t0" &&
                  operands->second == "7",
              "shared typed-call helper should decode spacing-tolerant local two-arg call operands without repeating parsed shape checks");
}

void test_backend_call_helpers_decode_structured_local_typed_operands() {
  c4c::backend::BackendCallInst single{
      "%t4",
      "i32",
      c4c::backend::BackendCallCallee::indirect("%fn.ptr"),
      {" i32 "},
      {{" i32 ", " %arg "}},
      true,
  };
  const auto single_operand = c4c::backend::parse_backend_single_typed_call_operand(single, "i32");
  expect_true(single_operand.has_value() && *single_operand == " %arg ",
              "lowering-owned typed-call helper should decode structured single local operands without adapter-owned wrappers");

  c4c::backend::BackendCallInst two{
      "%t5",
      "i32",
      c4c::backend::BackendCallCallee::indirect("%fn.ptr"),
      {" i32 ", " i32 "},
      {{" i32 ", " %lhs "}, {" i32 ", " %rhs "}},
      true,
  };
  const auto two_operands = c4c::backend::parse_backend_two_typed_call_operands(
      two, "i32", "i32");
  expect_true(two_operands.has_value() && two_operands->first == " %lhs " &&
                  two_operands->second == " %rhs ",
              "lowering-owned typed-call helper should decode structured two-operand local calls in order");
}

void test_backend_call_helpers_match_zero_add_slot_rewrite_shapes() {
  using namespace c4c::codegen::lir;

  const LirLoadOp load{"%t0", "i32", "%lv.slot"};
  const LirBinOp add_rhs_zero{"%t1", LirBinaryOpcode::Add, "i32", "%t0", "0"};
  const LirStoreOp store{"i32", "%t1", "%lv.slot"};
  const LirLoadOp reload{"%t2", "i32", "%lv.slot"};

  expect_true(c4c::backend::matches_backend_zero_add_slot_rewrite(
                  load, add_rhs_zero, store, "%lv.slot"),
              "lowering-owned slot-rewrite helper should match load-add-zero-store compatibility chains");
  expect_true(c4c::backend::matches_backend_zero_add_slot_rewrite_with_reload(
                  load, add_rhs_zero, store, reload, "%lv.slot"),
              "lowering-owned slot-rewrite helper should match compatibility chains that reload the rewritten slot");

  const LirBinOp add_lhs_zero{"%t1", LirBinaryOpcode::Add, "i32", "0", "%t0"};
  expect_true(c4c::backend::matches_backend_zero_add_slot_rewrite(
                  load, add_lhs_zero, store, "%lv.slot"),
              "lowering-owned slot-rewrite helper should accept either zero-add operand ordering");

  const LirLoadOp wrong_reload{"%t2", "i32", "%lv.other"};
  expect_true(!c4c::backend::matches_backend_zero_add_slot_rewrite_with_reload(
                  load, add_rhs_zero, store, wrong_reload, "%lv.slot"),
              "lowering-owned slot-rewrite helper should reject reloads from a different slot");
}

void test_backend_call_helpers_decode_local_typed_call_shapes() {
  {
    const auto module = make_typed_direct_call_local_arg_module();
    const auto& function = module.functions.back();
    const auto* slot = std::get_if<c4c::codegen::lir::LirAllocaOp>(&function.alloca_insts.front());
    const auto parsed =
        c4c::backend::parse_backend_single_local_typed_call(function.blocks.front().insts,
                                                            slot->result);
    expect_true(parsed.has_value() && parsed->arg_load != nullptr && parsed->call != nullptr &&
                    parsed->arg_load->result == "%t0" && parsed->call->result == "%t1",
                "lowering-owned local-call helper should decode the single-slot direct-load shape");
  }

  {
    const auto module = make_typed_direct_call_two_arg_both_local_second_rewrite_module();
    const auto& function = module.functions.back();
    const auto* slot0 = std::get_if<c4c::codegen::lir::LirAllocaOp>(&function.alloca_insts.front());
    const auto* slot1 = std::get_if<c4c::codegen::lir::LirAllocaOp>(&function.alloca_insts[1]);
    const auto parsed =
        c4c::backend::parse_backend_two_local_typed_call(
            function.blocks.front().insts,
            slot0->result,
            slot1->result);
    expect_true(parsed.has_value() && parsed->arg0_load != nullptr &&
                    parsed->arg1_load != nullptr && parsed->call != nullptr &&
                    parsed->arg0_load->result == "%t2" && parsed->arg1_load->result == "%t3" &&
                    parsed->call->result == "%t4",
                "lowering-owned local-call helper should decode the rewritten-second two-slot compatibility shape");
  }

  {
    const auto module = make_typed_direct_call_two_arg_both_local_double_rewrite_module();
    const auto& function = module.functions.back();
    const auto* slot0 = std::get_if<c4c::codegen::lir::LirAllocaOp>(&function.alloca_insts[0]);
    const auto* slot1 = std::get_if<c4c::codegen::lir::LirAllocaOp>(&function.alloca_insts[1]);
    const auto parsed = c4c::backend::parse_backend_two_local_typed_call(
        function.blocks.front().insts, slot0->result, slot1->result);
    expect_true(parsed.has_value() && parsed->arg0_load != nullptr &&
                    parsed->arg1_load != nullptr && parsed->call != nullptr &&
                    parsed->arg0_load->result == "%t4" && parsed->arg1_load->result == "%t5" &&
                    parsed->call->result == "%t6",
                "lowering-owned local-call helper should decode double-rewritten two-slot compatibility shapes");
  }

  {
    const auto module = make_typed_direct_call_two_arg_both_local_first_rewrite_module();
    const auto& function = module.functions.back();
    const auto* slot0 = std::get_if<c4c::codegen::lir::LirAllocaOp>(&function.alloca_insts[0]);
    const auto* slot1 = std::get_if<c4c::codegen::lir::LirAllocaOp>(&function.alloca_insts[1]);
    const auto parsed = c4c::backend::parse_backend_two_local_typed_call(
        function.blocks.front().insts, slot0->result, slot1->result);
    expect_true(parsed.has_value() && parsed->arg0_load != nullptr &&
                    parsed->arg1_load != nullptr && parsed->call != nullptr &&
                    parsed->arg0_load->result == "%t2" && parsed->arg1_load->result == "%t3" &&
                    parsed->call->result == "%t4",
                "lowering-owned local-call helper should decode mixed rewritten plus direct two-slot shapes");
  }
}

void test_backend_call_helpers_borrow_structured_typed_call_view() {
  c4c::backend::BackendCallInst call{
      "%t2",
      "i32",
      c4c::backend::BackendCallCallee::indirect("%fn.ptr"),
      {" ptr ", " i32 "},
      {{" ptr ", " %base "}, {" i32 ", " %idx "}},
      true,
  };

  const auto parsed = c4c::backend::parse_backend_typed_call(call);
  expect_true(parsed.has_value() && parsed->param_types.size() == 2 &&
                  parsed->param_types[0] == " ptr " &&
                  parsed->param_types[1] == " i32 " &&
                  parsed->args.size() == 2 &&
                  parsed->args[0].type == " ptr " &&
                  parsed->args[0].operand == " %base " &&
                  parsed->args[1].type == " i32 " &&
                  parsed->args[1].operand == " %idx ",
              "backend-owned typed-call view should borrow shared structured metadata directly from owned storage");
}

void test_backend_call_helpers_reconstruct_typed_call_view_from_structured_param_metadata() {
  c4c::backend::BackendCallInst call{
      "%t2",
      "i32",
      c4c::backend::BackendCallCallee::indirect("%fn.ptr"),
      {"ptr", "i32"},
      {{"ptr", "%base"}, {"i32", "%idx"}},
      true,
  };
  call.param_types[0].clear();
  call.param_types[1].clear();

  const auto parsed = c4c::backend::parse_backend_typed_call(call);
  expect_true(parsed.has_value() && parsed->param_types.size() == 2 &&
                  parsed->param_types[0] == "ptr" &&
                  parsed->param_types[1] == "i32" &&
                  parsed->args.size() == 2 &&
                  parsed->args[0].type == "ptr" &&
                  parsed->args[0].operand == "%base" &&
                  parsed->args[1].type == "i32" &&
                  parsed->args[1].operand == "%idx",
              "backend-owned typed-call view should reconstruct cleared param type text from structured backend call metadata");
}

void test_backend_call_helpers_match_declared_signature_prefix_and_varargs() {
  c4c::backend::BackendFunctionSignature signature;
  signature.return_type = "i32";
  signature.name = "printf";
  signature.params = {c4c::backend::BackendParam{"ptr", {}},
                      c4c::backend::BackendParam{"i32", {}}};

  c4c::backend::BackendCallInst call{
      "%t2",
      "i32",
      c4c::backend::BackendCallCallee::direct_global("printf"),
      {"ptr", "i32", "ptr"},
      {{"ptr", "@.str"}, {"i32", "7"}, {"ptr", "@tail"}},
      true,
  };

  const auto parsed = c4c::backend::parse_backend_typed_call(call);
  expect_true(parsed.has_value() &&
                  c4c::backend::backend_typed_call_matches_signature(
                      *parsed, signature, true),
              "shared backend call helpers should accept fixed declared params with trailing varargs");

  signature.params[1] = c4c::backend::BackendParam{"i64", {}};
  expect_true(parsed.has_value() &&
                  !c4c::backend::backend_typed_call_matches_signature(
                      *parsed, signature, true),
              "shared backend call helpers should reject mismatched declared param types");
}

void test_backend_call_helpers_parse_extern_args_from_typed_call() {
  c4c::backend::BackendCallInst call{
      "%t2",
      "i32",
      c4c::backend::BackendCallCallee::direct_global("puts"),
      {"i32", "ptr", "i64"},
      {{"i32", "7"}, {"ptr", "@.str"}, {"i64", "42"}},
      true,
  };

  const auto parsed = c4c::backend::parse_backend_typed_call(call);
  const auto args =
      parsed.has_value() ? c4c::backend::parse_backend_extern_call_args(*parsed) : std::nullopt;
  expect_true(args.has_value() && args->size() == 3 &&
                  (*args)[0].kind == c4c::backend::ParsedBackendExternCallArg::Kind::I32Imm &&
                  (*args)[0].imm == 7 &&
                  (*args)[1].kind == c4c::backend::ParsedBackendExternCallArg::Kind::Ptr &&
                  (*args)[1].operand == "@.str" &&
                  (*args)[2].kind == c4c::backend::ParsedBackendExternCallArg::Kind::I64Imm &&
                  (*args)[2].imm == 42,
              "shared backend call helpers should decode extern-call argument classes from typed calls");

  call.args[1].operand = "%stack.slot";
  const auto malformed = c4c::backend::parse_backend_typed_call(call);
  expect_true(malformed.has_value() &&
                  !c4c::backend::parse_backend_extern_call_args(*malformed).has_value(),
              "shared backend call helpers should reject unsupported non-global ptr extern-call operands");
}

void test_lir_typed_wrappers_preserve_legacy_opcode_and_predicate_strings() {
  using namespace c4c::codegen::lir;

  const LirBinOp add{"%t0", "add", "i32", "%lhs", "%rhs"};
  const LirCmpOp cmp{"%t1", true, "une", "double", "%lhs", "%rhs"};

  expect_true(add.opcode.typed().has_value(), "known binary opcode should map to typed enum");
  expect_true(add.opcode == "add", "binary opcode wrapper should preserve legacy string formatting");
  expect_true(cmp.predicate.typed().has_value(), "known cmp predicate should map to typed enum");
  expect_true(cmp.predicate == "une", "cmp predicate wrapper should preserve legacy string formatting");
}

void test_lir_typed_wrappers_leave_unknown_opcode_text_compatible() {
  using namespace c4c::codegen::lir;

  const LirBinaryOpcodeRef custom_opcode("future.add.sat");
  const LirCmpPredicateRef custom_predicate("future_pred");

  expect_true(!custom_opcode.typed().has_value(), "unknown binary opcode should stay text-compatible");
  expect_true(custom_opcode == "future.add.sat", "unknown binary opcode text should be preserved");
  expect_true(!custom_predicate.typed().has_value(), "unknown cmp predicate should stay text-compatible");
  expect_true(custom_predicate == "future_pred", "unknown cmp predicate text should be preserved");
}

void test_lir_printer_renders_verified_typed_ops() {
  using namespace c4c::codegen::lir;

  LirModule module;
  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", LirBinaryOpcode::Add, "i32", "2", "3"});
  entry.insts.push_back(LirCmpOp{"%t1", false, LirCmpPredicate::Slt, "i32", "%t0", "9"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  const auto rendered = print_llvm(module);
  expect_contains(rendered, "%t0 = add i32 2, 3",
                  "printer should render typed binary opcodes through verifier-backed helpers");
  expect_contains(rendered, "%t1 = icmp slt i32 %t0, 9",
                  "printer should render typed cmp predicates through verifier-backed helpers");
}

void test_lir_printer_canonicalizes_typed_call_surface() {
  using namespace c4c::codegen::lir;

  LirModule module;
  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"%t0", "i32", "%fn.ptr", " ( ptr , i32 ) ",
                                  "  ptr   %base ,   i32 %idx  "});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  const auto rendered = print_llvm(module);
  expect_contains(rendered, "%t0 = call i32 (ptr, i32) %fn.ptr(ptr %base, i32 %idx)",
                  "printer should route typed call rendering through the shared full-call formatter");
}

void test_lir_printer_rejects_malformed_typed_binary_surface() {
  using namespace c4c::codegen::lir;

  LirModule module;
  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"i32 %broken", "future.add.sat", "i32", "2", "3"});
  entry.terminator = LirRet{std::string("0"), "i32"};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  expect_throws_lir_verify(
      [&]() { (void)print_llvm(module); },
      "LirBinOp.result",
      "printer should reject malformed typed binary instructions before emitting LLVM text");
}




void test_adapts_direct_return() {
  const auto adapted = c4c::backend::lower_to_backend_ir(make_return_zero_module());
  expect_true(adapted.functions.size() == 1, "adapter should preserve one function");
  expect_true(adapted.functions.front().blocks.size() == 1,
              "adapter should preserve one block");
  expect_true(!adapted.functions.front().blocks.front().terminator.value->compare("0"),
              "adapter should preserve the return literal");
}

void test_renders_return_add() {
  const auto adapted = c4c::backend::lower_to_backend_ir(make_return_add_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "%t0 = add i32 2, 3",
                  "adapter renderer should emit the add instruction");
  expect_contains(rendered, "ret i32 %t0",
                  "adapter renderer should emit the adapted return");
}

void test_adapter_keeps_legacy_shim_aligned_with_lowering_entrypoint() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_return_add_module());
  const auto adapted = c4c::backend::adapt_minimal_module(make_return_add_module());

  expect_true(c4c::backend::render_module(lowered) == c4c::backend::render_module(adapted),
              "adapter shim should remain behaviorally identical to the lowering-named entrypoint");
}

void test_lowering_header_exposes_behavior_without_legacy_adapter_entrypoint() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_return_zero_module());

  expect_true(lowered.functions.size() == 1,
              "lowering-owned header should expose the production lowering entrypoint directly");
}



void test_adapter_normalizes_typed_direct_call_helper_slice() {
  const auto adapted = c4c::backend::lower_to_backend_ir(make_typed_direct_call_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_one(i32 %p.x)",
                  "adapter should preserve the single-argument helper signature");
  expect_contains(rendered, "%t1 = add i32 %p.x, 1",
                  "adapter should normalize the helper slot pattern into a backend-owned add");
  expect_contains(rendered, "ret i32 %t1",
                  "adapter should return the normalized helper add result directly");
  expect_contains(rendered, "call i32 (i32) @add_one(i32 5)",
                  "adapter should preserve the typed direct-call site");
}

void test_adapter_normalizes_renamed_param_slot_helper_slice() {
  auto module = make_typed_direct_call_module();

  c4c::codegen::lir::LirFunction* helper = nullptr;
  c4c::codegen::lir::LirFunction* main_fn = nullptr;
  for (auto& function : module.functions) {
    if (function.name == "add_one") {
      helper = &function;
    } else if (function.name == "main") {
      main_fn = &function;
    }
  }
  expect_true(helper != nullptr && main_fn != nullptr,
              "adapter renamed param-slot regression test needs helper and main functions");
  if (helper == nullptr || main_fn == nullptr) {
    return;
  }

  helper->name = "inc_value";
  helper->signature_text = "define i32 @inc_value(i32 %p.input)\n";
  auto& helper_alloca = std::get<c4c::codegen::lir::LirAllocaOp>(helper->alloca_insts.front());
  helper_alloca.result = "%lv.shadow.input";
  auto& helper_arg_store = std::get<c4c::codegen::lir::LirStoreOp>(helper->alloca_insts[1]);
  helper_arg_store.val = "%p.input";
  helper_arg_store.ptr = "%lv.shadow.input";
  auto& helper_load0 = std::get<c4c::codegen::lir::LirLoadOp>(helper->blocks.front().insts[0]);
  helper_load0.result = "%t.shadow.load";
  helper_load0.ptr = "%lv.shadow.input";
  auto& helper_add = std::get<c4c::codegen::lir::LirBinOp>(helper->blocks.front().insts[1]);
  helper_add.result = "%t.shadow.add";
  helper_add.lhs = "%t.shadow.load";
  auto& helper_store = std::get<c4c::codegen::lir::LirStoreOp>(helper->blocks.front().insts[2]);
  helper_store.val = "%t.shadow.add";
  helper_store.ptr = "%lv.shadow.input";
  auto& helper_load1 = std::get<c4c::codegen::lir::LirLoadOp>(helper->blocks.front().insts[3]);
  helper_load1.result = "%t.shadow.reload";
  helper_load1.ptr = "%lv.shadow.input";
  auto& helper_ret = std::get<c4c::codegen::lir::LirRet>(helper->blocks.front().terminator);
  helper_ret.value_str = "%t.shadow.reload";

  auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&main_fn->blocks.front().insts.front());
  expect_true(call != nullptr,
              "adapter renamed param-slot regression test needs the direct call to mutate");
  if (call == nullptr) {
    return;
  }
  call->callee = c4c::codegen::lir::LirOperand(std::string("@inc_value"),
                                               c4c::codegen::lir::LirOperandKind::Global);

  const auto adapted = c4c::backend::lower_to_backend_ir(module);
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @inc_value(i32 %p.input)",
                  "adapter should preserve renamed single-argument helper signatures");
  expect_contains(rendered, "%t.shadow.add = add i32 %p.input, 1",
                  "adapter should normalize renamed param-slot helpers through the shared slot-backed add contract");
  expect_contains(rendered, "call i32 (i32) @inc_value(i32 5)",
                  "adapter should keep renamed param-slot direct calls on the backend-owned direct-call surface");
}

void test_adapter_normalizes_local_temp_return_slice() {
  const auto adapted = c4c::backend::lower_to_backend_ir(make_local_temp_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @main()",
                  "adapter should preserve the local-temp function signature");
  expect_contains(rendered, "ret i32 5",
                  "adapter should collapse the single-slot local-temp pattern into a direct return");
}

void test_adapter_normalizes_local_temp_sub_return_slice() {
  const auto adapted = c4c::backend::lower_to_backend_ir(make_local_temp_sub_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "%t1 = sub i32 5, 4",
                  "adapter should rewrite the local-slot subtraction slice into a direct subtraction");
  expect_contains(rendered, "ret i32 %t1",
                  "adapter should return the normalized subtraction result directly");
}

void test_adapter_normalizes_two_local_temp_return_slice() {
  const auto adapted =
      c4c::backend::lower_to_backend_ir(make_two_local_temp_return_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @main()",
                  "adapter should preserve the two-local function signature");
  expect_contains(rendered, "ret i32 0",
                  "adapter should collapse the selected two-local scalar slot pattern into a direct return");
}

void test_adapter_normalizes_local_pointer_temp_return_slice() {
  const auto adapted =
      c4c::backend::lower_to_backend_ir(make_local_pointer_temp_return_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @main()",
                  "adapter should preserve the local-pointer round-trip function signature");
  expect_contains(rendered, "ret i32 0",
                  "adapter should collapse the bounded local pointer round-trip into a direct return");
}

void test_adapter_normalizes_double_indirect_local_pointer_conditional_return_slice() {
  const auto adapted = c4c::backend::lower_to_backend_ir(
      make_double_indirect_local_pointer_conditional_return_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @main()",
                  "adapter should preserve the double-indirect local-pointer function signature");
  expect_contains(rendered, "ret i32 0",
                  "adapter should collapse the selected double-indirect local-pointer conditional chain into a direct return");
}

void test_adapter_normalizes_goto_only_constant_return_slice() {
  const auto adapted =
      c4c::backend::lower_to_backend_ir(make_goto_only_constant_return_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @main()",
                  "adapter should preserve the goto-only function signature");
  expect_contains(rendered, "ret i32 0",
                  "adapter should collapse the selected goto-only branch chain into a direct return");
}

void test_adapter_normalizes_countdown_while_return_slice() {
  const auto adapted =
      c4c::backend::lower_to_backend_ir(make_countdown_while_return_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @main()",
                  "adapter should preserve the while-countdown function signature");
  expect_contains(rendered, "%t.iter = phi i32 [ 50, %entry ], [ %t.dec, %block_2 ]",
                  "adapter should lower the bounded while-countdown loop into explicit loop-carried IR");
  expect_contains(rendered, "br i1 %t.keep_going, label %block_2, label %block_3",
                  "adapter should preserve the explicit countdown loop backedge split");
}

void test_adapter_normalizes_typed_countdown_while_return_slice() {
  const auto adapted =
      c4c::backend::lower_to_backend_ir(make_typed_countdown_while_return_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @main()",
                  "adapter should preserve the typed while-countdown function signature");
  expect_contains(rendered, "%t.iter = phi i32 [ 50, %entry ], [ %t.dec, %block_2 ]",
                  "adapter should lower typed cmp/sub while-countdown loops into explicit loop-carried IR");
  expect_contains(rendered, "%t.dec = sub i32 %t.iter, 1",
                  "adapter should preserve the typed countdown decrement in backend-owned IR");
}

void test_adapter_normalizes_countdown_do_while_return_slice() {
  const auto adapted =
      c4c::backend::lower_to_backend_ir(make_countdown_do_while_return_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @main()",
                  "adapter should preserve the do-while countdown function signature");
  expect_contains(rendered, "ret i32 0",
                  "adapter should collapse the bounded do-while countdown loop into a direct return");
}

void test_adapter_preserves_typed_two_arg_direct_call_helper_slice() {
  const auto adapted =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_pair(i32 %p.x, i32 %p.y)",
                  "adapter should preserve the two-argument helper signature");
  expect_contains(rendered, "%t0 = add i32 %p.x, %p.y",
                  "adapter should preserve the register-only two-argument helper add");
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should preserve the typed two-argument direct-call site");
}

void test_adapter_normalizes_typed_direct_call_local_arg_slice() {
  const auto adapted =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_local_arg_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_one(i32 %p.x)",
                  "adapter should preserve the local-argument helper signature");
  expect_contains(rendered, "%t1 = add i32 %p.x, 1",
                  "adapter should still normalize the helper slot pattern");
  expect_contains(rendered, "call i32 (i32) @add_one(i32 5)",
                  "adapter should normalize the local slot direct-call argument into the backend slice");
}

void test_adapter_normalizes_typed_direct_call_local_arg_spacing_slice() {
  const auto adapted = c4c::backend::lower_to_backend_ir(
      make_typed_direct_call_local_arg_with_spacing_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "call i32 (i32) @add_one(i32 5)",
                  "adapter should normalize local slot direct-call arguments even when compatibility spacing varies");
}

void test_adapter_normalizes_typed_direct_call_local_arg_without_suffix_slice() {
  auto module = make_typed_direct_call_local_arg_with_spacing_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.back());
  call.callee_type_suffix.clear();

  const auto adapted = c4c::backend::lower_to_backend_ir(module);
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "call i32 (i32) @add_one(i32 5)",
                  "adapter should infer backend call param types from typed args when local direct-call compatibility suffixes are absent");
}

void test_adapter_canonicalizes_backend_owned_direct_call_rendering() {
  auto module = make_typed_direct_call_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.front());
  call.callee_type_suffix = " ( i32 ) ";
  call.args_str = "  i32   5  ";

  const auto adapted = c4c::backend::lower_to_backend_ir(module);
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "call i32 (i32) @add_one(i32 5)",
                  "adapter should render backend-owned direct calls from canonical structured metadata instead of preserving raw compatibility spacing");
}

void test_adapter_normalizes_typed_two_arg_direct_call_local_arg_slice() {
  const auto adapted =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_local_arg_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_pair(i32 %p.x, i32 %p.y)",
                  "adapter should preserve the two-argument local-argument helper signature");
  expect_contains(rendered, "%t0 = add i32 %p.x, %p.y",
                  "adapter should still preserve the two-argument helper add");
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize the local slot first argument into the backend-owned two-argument slice");
}

void test_adapter_normalizes_typed_two_arg_direct_call_local_arg_spacing_slice() {
  const auto adapted = c4c::backend::lower_to_backend_ir(
      make_typed_direct_call_two_arg_local_arg_with_spacing_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize first-local two-argument direct-call arguments even when compatibility spacing varies");
}

void test_adapter_normalizes_typed_two_arg_direct_call_second_local_arg_slice() {
  const auto adapted = c4c::backend::lower_to_backend_ir(
      make_typed_direct_call_two_arg_second_local_arg_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_pair(i32 %p.x, i32 %p.y)",
                  "adapter should preserve the two-argument second-local helper signature");
  expect_contains(rendered, "%t0 = add i32 %p.x, %p.y",
                  "adapter should still preserve the two-argument helper add");
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize the local slot second argument into the backend-owned two-argument slice");
}

void test_adapter_normalizes_typed_two_arg_direct_call_second_local_rewrite_slice() {
  const auto adapted = c4c::backend::lower_to_backend_ir(
      make_typed_direct_call_two_arg_second_local_rewrite_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_pair(i32 %p.x, i32 %p.y)",
                  "adapter should preserve the rewritten second-local helper signature");
  expect_contains(rendered, "%t0 = add i32 %p.x, %p.y",
                  "adapter should still preserve the two-argument helper add");
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize the rewritten second-local slot into the backend-owned two-argument slice");
}

void test_adapter_normalizes_typed_two_arg_direct_call_first_local_rewrite_slice() {
  const auto adapted = c4c::backend::lower_to_backend_ir(
      make_typed_direct_call_two_arg_first_local_rewrite_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_pair(i32 %p.x, i32 %p.y)",
                  "adapter should preserve the rewritten first-local helper signature");
  expect_contains(rendered, "%t0 = add i32 %p.x, %p.y",
                  "adapter should still preserve the two-argument helper add");
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize the rewritten first-local slot into the backend-owned two-argument slice");
}

void test_adapter_normalizes_typed_two_arg_direct_call_first_local_rewrite_spacing_slice() {
  const auto adapted = c4c::backend::lower_to_backend_ir(
      make_typed_direct_call_two_arg_first_local_rewrite_with_spacing_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize rewritten local call arguments even when compatibility spacing varies");
}

void test_adapter_normalizes_typed_two_arg_direct_call_both_local_arg_slice() {
  const auto adapted =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_both_local_arg_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_pair(i32 %p.x, i32 %p.y)",
                  "adapter should preserve the two-argument both-local helper signature");
  expect_contains(rendered, "%t0 = add i32 %p.x, %p.y",
                  "adapter should still preserve the two-argument helper add");
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize both local slot arguments into the backend-owned two-argument slice");
}

void test_adapter_normalizes_typed_two_arg_direct_call_both_local_first_rewrite_slice() {
  const auto adapted = c4c::backend::lower_to_backend_ir(
      make_typed_direct_call_two_arg_both_local_first_rewrite_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_pair(i32 %p.x, i32 %p.y)",
                  "adapter should preserve the rewritten both-local helper signature");
  expect_contains(rendered, "%t0 = add i32 %p.x, %p.y",
                  "adapter should still preserve the rewritten both-local helper add");
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize the rewritten first local plus second local caller into the backend-owned two-argument slice");
}

void test_adapter_normalizes_typed_two_arg_direct_call_both_local_second_rewrite_slice() {
  const auto adapted = c4c::backend::lower_to_backend_ir(
      make_typed_direct_call_two_arg_both_local_second_rewrite_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_pair(i32 %p.x, i32 %p.y)",
                  "adapter should preserve the rewritten both-local helper signature");
  expect_contains(rendered, "%t0 = add i32 %p.x, %p.y",
                  "adapter should still preserve the rewritten both-local helper add");
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize the preserved first local plus rewritten second local caller into the backend-owned two-argument slice");
}

void test_adapter_normalizes_typed_two_arg_direct_call_both_local_double_rewrite_slice() {
  const auto adapted = c4c::backend::lower_to_backend_ir(
      make_typed_direct_call_two_arg_both_local_double_rewrite_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_pair(i32 %p.x, i32 %p.y)",
                  "adapter should preserve the double-rewritten both-local helper signature");
  expect_contains(rendered, "%t0 = add i32 %p.x, %p.y",
                  "adapter should still preserve the double-rewritten both-local helper add");
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize the double-rewritten both-local caller into the backend-owned two-argument slice");
}

void test_adapter_tracks_structured_signature_contract() {
  const auto adapted = c4c::backend::lower_to_backend_ir(make_return_zero_module());
  const auto& signature = adapted.functions.front().signature;
  expect_true(signature.linkage == "define",
              "adapter should preserve whether a function is defined or declared");
  expect_true(signature.linkage_kind == c4c::backend::BackendFunctionLinkage::Define,
              "adapter should preserve structured function linkage separately from the compatibility text");
  expect_true(signature.return_type == "i32",
              "adapter should preserve the function return type separately from the name");
  expect_true(signature.return_type_kind == c4c::backend::BackendValueTypeKind::Scalar,
              "adapter should preserve the structured function return kind separately from compatibility text");
  expect_true(signature.return_scalar_type == c4c::backend::BackendScalarType::I32,
              "adapter should preserve the structured function return scalar separately from compatibility text");
  expect_true(signature.name == "main",
              "adapter should preserve the function name separately from the signature text");
  expect_true(signature.params.empty(),
              "adapter should preserve the empty parameter list for the minimal return-only slice");
  expect_true(!signature.is_vararg,
              "adapter should keep the minimal return-only slice non-variadic");
}

void test_adapter_tracks_structured_entry_block_and_return_contract() {
  const auto adapted = c4c::backend::lower_to_backend_ir(make_return_add_module());
  const auto& function = adapted.functions.front();
  expect_true(function.blocks.size() == 1,
              "adapter should preserve the single-block bring-up slice");
  const auto& block = function.blocks.front();
  expect_true(block.label == "entry",
              "adapter should preserve the entry block label separately from rendering");
  expect_true(block.insts.size() == 1,
              "adapter should preserve the current single add instruction");
  const auto* add = std::get_if<c4c::backend::BackendBinaryInst>(&block.insts.front());
  expect_true(add != nullptr,
              "adapter should materialize the return-add instruction as a backend-owned op");
  expect_true(add->opcode == c4c::backend::BackendBinaryOpcode::Add,
              "adapter should preserve the add opcode in backend-owned form");
  expect_true(add->result == "%t0",
              "adapter should preserve the add result name");
  expect_true(add->type_str == "i32",
              "adapter should preserve the add type");
  expect_true(add->lhs == "2" && add->rhs == "3",
              "adapter should preserve the add operands");
  expect_true(block.terminator.value.has_value() && *block.terminator.value == "%t0",
              "adapter should preserve the return value separately from the block text");
  expect_true(c4c::backend::backend_return_scalar_type(block.terminator) ==
                  c4c::backend::BackendScalarType::I32,
              "adapter should preserve the structured return scalar type separately from the block text");
  expect_true(block.terminator.type_str == "i32",
              "adapter should keep the return type compatibility text for current backend consumers");
}

void test_adapter_tracks_structured_typed_add_entry_block_and_return_contract() {
  const auto adapted = c4c::backend::lower_to_backend_ir(make_typed_return_add_module());
  const auto& function = adapted.functions.front();
  expect_true(function.blocks.size() == 1,
              "adapter should preserve the single-block typed-add slice");
  const auto& block = function.blocks.front();
  expect_true(block.insts.size() == 1,
              "adapter should preserve the current single typed add instruction");
  const auto* add = std::get_if<c4c::backend::BackendBinaryInst>(&block.insts.front());
  expect_true(add != nullptr,
              "adapter should materialize typed add instructions as backend-owned ops");
  expect_true(add->opcode == c4c::backend::BackendBinaryOpcode::Add,
              "adapter should decode typed binary opcode wrappers without raw string dispatch");
  expect_true(add->lhs == "2" && add->rhs == "3",
              "adapter should preserve typed add operands");
}

void test_adapter_tracks_structured_sub_entry_block_and_return_contract() {
  const auto adapted = c4c::backend::lower_to_backend_ir(make_return_sub_module());
  const auto& function = adapted.functions.front();
  expect_true(function.blocks.size() == 1,
              "adapter should preserve the single-block subtraction slice");
  const auto& block = function.blocks.front();
  expect_true(block.label == "entry",
              "adapter should preserve the entry block label for subtraction slices");
  expect_true(block.insts.size() == 1,
              "adapter should preserve the current single subtraction instruction");
  const auto* sub = std::get_if<c4c::backend::BackendBinaryInst>(&block.insts.front());
  expect_true(sub != nullptr,
              "adapter should materialize the return-sub instruction as a backend-owned op");
  expect_true(sub->opcode == c4c::backend::BackendBinaryOpcode::Sub,
              "adapter should preserve the subtraction opcode in backend-owned form");
  expect_true(sub->result == "%t0",
              "adapter should preserve the subtraction result name");
  expect_true(sub->type_str == "i32",
              "adapter should preserve the subtraction type");
  expect_true(sub->lhs == "3" && sub->rhs == "3",
              "adapter should preserve the subtraction operands");
  expect_true(block.terminator.value.has_value() && *block.terminator.value == "%t0",
              "adapter should preserve the subtraction return value separately from the block text");
  expect_true(c4c::backend::backend_return_scalar_type(block.terminator) ==
                  c4c::backend::BackendScalarType::I32,
              "adapter should preserve the subtraction return scalar type separately from the block text");
  expect_true(block.terminator.type_str == "i32",
              "adapter should keep the subtraction return type compatibility text for current backend consumers");
}

void test_adapter_normalizes_local_temp_arithmetic_chain_slice() {
  const auto adapted = c4c::backend::lower_to_backend_ir(make_local_temp_arithmetic_chain_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "ret i32 0",
                  "adapter should collapse the bounded one-slot arithmetic chain into a direct return immediate");
  expect_not_contains(rendered, "mul i32",
                      "adapter should finish the bounded arithmetic chain instead of leaking unsupported backend ops");
  expect_not_contains(rendered, "sdiv i32",
                      "adapter should fully normalize the bounded arithmetic chain before backend rendering");
  expect_not_contains(rendered, "srem i32",
                      "adapter should fully normalize the bounded arithmetic chain before backend rendering");
}

void test_adapter_infers_extern_decl_params_from_typed_calls() {
  const auto adapted =
      c4c::backend::lower_to_backend_ir(make_x86_extern_decl_inferred_param_module());

  expect_true(adapted.functions.size() == 2,
              "adapter should preserve the extern declaration alongside the caller");
  const auto& decl = adapted.functions.front();
  expect_true(decl.is_declaration,
              "adapter should keep inferred extern signatures as declarations");
  expect_true(decl.signature.linkage_kind ==
                  c4c::backend::BackendFunctionLinkage::Declare,
              "adapter should preserve inferred extern declarations as structured declaration linkage");
  expect_true(decl.signature.name == "helper_ext",
              "adapter should preserve the extern declaration symbol name");
  expect_true(decl.signature.params.size() == 2,
              "adapter should infer the fixed extern parameter list from typed call sites");
  expect_true(decl.signature.params[0].type_str == "i32" &&
                  decl.signature.params[1].type_str == "i32",
              "adapter should preserve the inferred extern parameter types in order");
  expect_true(decl.signature.params[0].type_kind ==
                      c4c::backend::BackendValueTypeKind::Scalar &&
                  decl.signature.params[1].type_kind ==
                      c4c::backend::BackendValueTypeKind::Scalar,
              "adapter should preserve structured extern parameter kinds alongside the compatibility text");
  expect_true(decl.signature.params[0].scalar_type ==
                      c4c::backend::BackendScalarType::I32 &&
                  decl.signature.params[1].scalar_type ==
                      c4c::backend::BackendScalarType::I32,
              "adapter should preserve structured extern parameter scalar types alongside the compatibility text");
}

void test_adapter_tracks_structured_call_type_contract() {
  const auto adapted =
      c4c::backend::lower_to_backend_ir(make_x86_extern_decl_inferred_param_module());
  const auto& main_block = adapted.functions.back().blocks.front();
  const auto* call = std::get_if<c4c::backend::BackendCallInst>(&main_block.insts.front());
  expect_true(call != nullptr,
              "adapter should materialize the inferred extern caller instruction as a backend-owned call");
  expect_true(call->return_type == "i32",
              "adapter should preserve the call return type compatibility text");
  expect_true(call->return_type_kind == c4c::backend::BackendValueTypeKind::Scalar,
              "adapter should preserve the structured call return kind");
  expect_true(call->return_scalar_type == c4c::backend::BackendScalarType::I32,
              "adapter should preserve the structured call return scalar type");
  expect_true(call->param_types.size() == 2 && call->param_types[0] == "i32" &&
                  call->param_types[1] == "i32",
              "adapter should preserve the call parameter type compatibility text in order");
  expect_true(call->param_type_kinds.size() == 2 &&
                  call->param_type_kinds[0] ==
                      c4c::backend::BackendValueTypeKind::Scalar &&
                  call->param_type_kinds[1] ==
                      c4c::backend::BackendValueTypeKind::Scalar,
              "adapter should preserve structured call parameter kinds in order");
  expect_true(call->param_scalar_types.size() == 2 &&
                  call->param_scalar_types[0] == c4c::backend::BackendScalarType::I32 &&
                  call->param_scalar_types[1] == c4c::backend::BackendScalarType::I32,
              "adapter should preserve structured call parameter scalar types in order");
}

void test_adapter_rejects_inconsistent_extern_decl_call_surfaces() {
  try {
    (void)c4c::backend::lower_to_backend_ir(make_x86_extern_decl_inconsistent_param_module());
    fail("adapter should reject extern declarations whose typed call surfaces disagree");
  } catch (const c4c::backend::LirAdapterError& ex) {
    expect_true(ex.kind() == c4c::backend::LirAdapterErrorKind::Unsupported,
                "adapter should classify inconsistent extern call surfaces as unsupported");
    expect_contains(ex.what(), "inconsistent typed call surfaces",
                    "adapter should explain inconsistent extern call inference failures");
  }
}

void test_rejects_unsupported_instruction() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(LirLoadOp{"%t0", "i32", "%ptr"});
  block.terminator = LirRet{std::string("%t0"), "i32"};

  try {
    (void)c4c::backend::lower_to_backend_ir(module);
    fail("adapter should reject unsupported instructions");
  } catch (const c4c::backend::LirAdapterError& ex) {
    expect_contains(ex.what(), "non-binary/non-call instructions",
                    "adapter should explain unsupported instructions");
    expect_true(ex.kind() == c4c::backend::LirAdapterErrorKind::Unsupported,
                "adapter should classify unsupported instructions distinctly from malformed input");
  }
}

#if 0
// TODO(phase2): linker/assembler-heavy test kept out for now while we stabilize file-scope split.
void test_shared_linker_parses_builtin_x86_extern_call_object() {
  const auto object_path = make_temp_output_path("c4c_x86_extern_call_parse");
  const auto staged = c4c::backend::x86::assemble_module(
      make_x86_extern_decl_object_module(), object_path);
  expect_true(staged.object_emitted,
              "x86 built-in assembler should emit an object for the bounded extern-call slice");

  const auto object_bytes = read_file_bytes(object_path);
  std::string error;
  const auto parsed = c4c::backend::linker_common::parse_elf64_object(
      object_bytes, object_path, c4c::backend::elf::EM_X86_64, &error);
  expect_true(parsed.has_value(),
              "shared linker object parser should accept the built-in x86 extern-call object: " +
                  error);

  const auto& object = *parsed;
  const auto text_index = find_section_index(object, ".text");
  const auto rela_text_index = find_section_index(object, ".rela.text");
  const auto symtab_index = find_section_index(object, ".symtab");
  const auto strtab_index = find_section_index(object, ".strtab");
  const auto shstrtab_index = find_section_index(object, ".shstrtab");
  expect_true(text_index < object.sections.size() && rela_text_index < object.sections.size() &&
                  symtab_index < object.sections.size() && strtab_index < object.sections.size() &&
                  shstrtab_index < object.sections.size(),
              "shared linker object parser should preserve the relocation-bearing x86 object section inventory");
  expect_true(object.relocations.size() == object.sections.size(),
              "shared linker object parser should keep relocation vectors indexed by section for built-in x86 emitted objects");
  expect_true(object.section_data[text_index].size() == 6,
              "shared linker object parser should preserve the built-in emitted x86 extern-call text payload");

  const auto main_symbol = std::find_if(
      object.symbols.begin(), object.symbols.end(),
      [&](const c4c::backend::linker_common::Elf64Symbol& symbol) {
        return symbol.name == "main";
      });
  const auto helper_symbol = std::find_if(
      object.symbols.begin(), object.symbols.end(),
      [&](const c4c::backend::linker_common::Elf64Symbol& symbol) {
        return symbol.name == "helper_ext";
      });
  expect_true(main_symbol != object.symbols.end() && main_symbol->is_global() &&
                  main_symbol->sym_type() == c4c::backend::elf::STT_FUNC &&
                  main_symbol->shndx == text_index,
              "shared linker object parser should preserve the built-in emitted x86 main symbol inventory");
  expect_true(helper_symbol != object.symbols.end() && helper_symbol->is_undefined(),
              "shared linker object parser should preserve the built-in emitted x86 undefined helper symbol");

  expect_true(object.relocations[text_index].size() == 1,
              "shared linker object parser should preserve one relocation for the bounded built-in x86 extern-call object");
  const auto& relocation = object.relocations[text_index].front();
  expect_true(relocation.offset == 1 && relocation.rela_type == 4 &&
                  relocation.addend == -4 &&
                  relocation.sym_idx < object.symbols.size() &&
                  object.symbols[relocation.sym_idx].name == "helper_ext",
              "shared linker object parser should preserve the staged x86 PLT32 relocation contract for the built-in extern-call object");

  std::filesystem::remove(object_path);
}
#endif


int main(int argc, char* argv[]) {
  if (argc >= 2) test_filter() = argv[1];
  test_lir_typed_wrappers_classify_basic_operands();
  test_lir_typed_wrappers_classify_basic_types();
  test_lir_call_arg_helpers_split_nested_typed_args();
  test_lir_call_arg_helpers_collect_value_names();
  test_lir_call_arg_helpers_collect_full_call_value_names();
  test_lir_call_arg_helpers_collect_full_call_global_refs();
  test_lir_call_arg_helpers_rewrite_full_call_operands();
  test_lir_call_arg_helpers_collect_call_op_value_names();
  test_lir_call_arg_helpers_rewrite_call_op_operands();
  test_lir_call_arg_helpers_decode_single_typed_operand();
  test_lir_call_arg_helpers_decode_two_typed_operands();
  test_lir_call_arg_helpers_classify_call_callee_shape();
  test_lir_call_arg_helpers_decode_direct_global_typed_call();
  test_lir_call_arg_helpers_decode_zero_arg_and_call_crossing_direct_calls();
  test_lir_call_arg_helpers_format_typed_call_round_trip();
  test_lir_call_arg_helpers_infer_param_types_when_suffix_missing();
  test_lir_call_arg_helpers_format_full_call_site_round_trip();
  test_lir_call_arg_helpers_format_call_fields_with_suffix();
  test_lir_call_arg_helpers_make_typed_call_op();
  test_lir_call_arg_helpers_make_direct_call_op_without_suffix();
  test_lir_call_arg_helpers_parse_and_format_call_op_views();
  test_lir_call_arg_helpers_collect_call_op_global_refs();
  test_backend_call_helpers_decode_structured_direct_global_call();
  test_backend_call_helpers_decode_structured_direct_global_vararg_prefix();
  test_backend_call_helpers_decode_direct_global_single_typed_operand();
  test_backend_call_helpers_decode_direct_global_two_typed_operands();
  test_backend_call_helpers_decode_zero_arg_direct_global_calls();
  test_backend_call_helpers_decode_lir_direct_global_typed_operands();
  test_backend_call_helpers_parse_single_helper_zero_arg_main_module_shape();
  test_backend_call_helpers_parse_single_helper_call_crossing_source_value();
  test_backend_call_helpers_parse_single_helper_direct_call();
  test_backend_call_helpers_parse_single_add_imm_function();
  test_backend_call_helpers_parse_two_param_add_function();
  test_backend_call_helpers_parse_minimal_two_arg_direct_call_lir_module();
  test_backend_call_helpers_parse_structured_two_param_add_function();
  test_backend_call_helpers_parse_structured_zero_arg_return_imm_function();
  test_backend_call_helpers_parse_structured_single_add_imm_function();
  test_backend_call_helpers_parse_structured_zero_arg_direct_call_module();
  test_backend_call_helpers_parse_structured_single_add_imm_direct_call_module();
  test_backend_call_helpers_parse_structured_call_crossing_direct_call_module();
  test_backend_call_helpers_parse_structured_folded_two_arg_function();
  test_backend_call_helpers_parse_structured_two_arg_direct_call_module();
  test_backend_call_helpers_match_structured_direct_call_module();
  test_backend_call_helpers_parse_structured_folded_two_arg_direct_call_module();
  test_backend_call_helpers_parse_structured_declared_direct_call_module();
  test_backend_call_helpers_decode_lir_direct_global_vararg_prefix();
  test_backend_call_helpers_classify_lir_nonminimal_call_types();
  test_backend_call_helpers_classify_lir_nonminimal_signature_types();
  test_backend_call_helpers_classify_i32_main_signature_shapes();
  test_backend_call_helpers_match_lir_helper_signature_shapes();
  test_backend_call_helpers_decode_signature_params_with_spacing_and_varargs();
  test_backend_call_helpers_classify_lir_nonminimal_global_and_return_types();
  test_backend_call_helpers_decode_single_typed_local_operand();
  test_backend_call_helpers_decode_owned_typed_args_from_lir_call();
  test_backend_call_helpers_decode_two_typed_local_operands();
  test_backend_call_helpers_decode_structured_local_typed_operands();
  test_backend_call_helpers_match_zero_add_slot_rewrite_shapes();
  test_backend_call_helpers_decode_local_typed_call_shapes();
  test_backend_call_helpers_borrow_structured_typed_call_view();
  test_backend_call_helpers_reconstruct_typed_call_view_from_structured_param_metadata();
  test_backend_call_helpers_match_declared_signature_prefix_and_varargs();
  test_backend_call_helpers_parse_extern_args_from_typed_call();
  test_backend_call_helpers_parse_param_slot_runtime_as_structured_single_add_imm_direct_call();
  test_lir_typed_wrappers_preserve_legacy_opcode_and_predicate_strings();
  test_lir_typed_wrappers_leave_unknown_opcode_text_compatible();
  test_lir_printer_renders_verified_typed_ops();
  test_lir_printer_canonicalizes_typed_call_surface();
  test_lir_printer_rejects_malformed_typed_binary_surface();
  test_adapts_direct_return();
  test_renders_return_add();
  test_adapter_keeps_legacy_shim_aligned_with_lowering_entrypoint();
  test_lowering_header_exposes_behavior_without_legacy_adapter_entrypoint();
  test_adapter_normalizes_typed_direct_call_helper_slice();
  test_adapter_normalizes_renamed_param_slot_helper_slice();
  test_adapter_normalizes_local_temp_return_slice();
  test_adapter_normalizes_local_temp_sub_return_slice();
  test_adapter_normalizes_two_local_temp_return_slice();
  test_adapter_normalizes_local_pointer_temp_return_slice();
  test_adapter_normalizes_double_indirect_local_pointer_conditional_return_slice();
  test_adapter_normalizes_goto_only_constant_return_slice();
  test_adapter_normalizes_countdown_while_return_slice();
  test_adapter_normalizes_typed_countdown_while_return_slice();
  test_adapter_normalizes_countdown_do_while_return_slice();
  test_adapter_preserves_typed_two_arg_direct_call_helper_slice();
  test_adapter_normalizes_typed_direct_call_local_arg_slice();
  test_adapter_normalizes_typed_direct_call_local_arg_spacing_slice();
  test_adapter_normalizes_typed_direct_call_local_arg_without_suffix_slice();
  test_adapter_canonicalizes_backend_owned_direct_call_rendering();
  test_adapter_normalizes_typed_two_arg_direct_call_local_arg_slice();
  test_adapter_normalizes_typed_two_arg_direct_call_local_arg_spacing_slice();
  test_adapter_normalizes_typed_two_arg_direct_call_second_local_arg_slice();
  test_adapter_normalizes_typed_two_arg_direct_call_second_local_rewrite_slice();
  test_adapter_normalizes_typed_two_arg_direct_call_first_local_rewrite_slice();
  test_adapter_normalizes_typed_two_arg_direct_call_first_local_rewrite_spacing_slice();
  test_adapter_normalizes_typed_two_arg_direct_call_both_local_arg_slice();
  test_adapter_normalizes_typed_two_arg_direct_call_both_local_first_rewrite_slice();
  test_adapter_normalizes_typed_two_arg_direct_call_both_local_second_rewrite_slice();
  test_adapter_normalizes_typed_two_arg_direct_call_both_local_double_rewrite_slice();
  test_adapter_tracks_structured_signature_contract();
  test_adapter_tracks_structured_call_type_contract();
  test_adapter_tracks_structured_entry_block_and_return_contract();
  test_adapter_tracks_structured_typed_add_entry_block_and_return_contract();
  test_adapter_tracks_structured_sub_entry_block_and_return_contract();
  test_adapter_normalizes_local_temp_arithmetic_chain_slice();
  test_adapter_infers_extern_decl_params_from_typed_calls();
  test_adapter_rejects_inconsistent_extern_decl_call_surfaces();
  test_rejects_unsupported_instruction();

  check_failures();
  return 0;
}
