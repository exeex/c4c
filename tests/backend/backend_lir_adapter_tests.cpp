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

void test_backend_call_helpers_decode_single_typed_local_operand() {
  auto module = make_typed_direct_call_local_arg_with_suffix_spacing_module();
  const auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.back());

  const auto operand = c4c::backend::parse_backend_single_typed_call_operand(call, "i32");
  expect_true(operand.has_value() && *operand == "%t0",
              "shared typed-call helper should decode spacing-tolerant local-call operands without repeating parsed single-arg shape checks");
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
  expect_true(signature.return_type == "i32",
              "adapter should preserve the function return type separately from the name");
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
  expect_true(block.terminator.type_str == "i32",
              "adapter should preserve the return type separately from the block text");
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
  expect_true(block.terminator.type_str == "i32",
              "adapter should preserve the subtraction return type separately from the block text");
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
  expect_true(decl.signature.name == "helper_ext",
              "adapter should preserve the extern declaration symbol name");
  expect_true(decl.signature.params.size() == 2,
              "adapter should infer the fixed extern parameter list from typed call sites");
  expect_true(decl.signature.params[0].type_str == "i32" &&
                  decl.signature.params[1].type_str == "i32",
              "adapter should preserve the inferred extern parameter types in order");
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
  test_backend_call_helpers_decode_lir_direct_global_vararg_prefix();
  test_backend_call_helpers_decode_single_typed_local_operand();
  test_backend_call_helpers_decode_two_typed_local_operands();
  test_backend_call_helpers_decode_structured_local_typed_operands();
  test_backend_call_helpers_match_zero_add_slot_rewrite_shapes();
  test_backend_call_helpers_decode_local_typed_call_shapes();
  test_backend_call_helpers_borrow_structured_typed_call_view();
  test_lir_typed_wrappers_preserve_legacy_opcode_and_predicate_strings();
  test_lir_typed_wrappers_leave_unknown_opcode_text_compatible();
  test_lir_printer_renders_verified_typed_ops();
  test_lir_printer_canonicalizes_typed_call_surface();
  test_lir_printer_rejects_malformed_typed_binary_surface();
  test_adapts_direct_return();
  test_renders_return_add();
  test_adapter_keeps_legacy_shim_aligned_with_lowering_entrypoint();
  test_adapter_normalizes_typed_direct_call_helper_slice();
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
