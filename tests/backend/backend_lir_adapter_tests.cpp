#include "backend.hpp"
#include "generation.hpp"
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

namespace {

[[noreturn]] void fail(const std::string& message) {
  std::cerr << "FAIL: " << message << "\n";
  std::exit(1);
}

void expect_true(bool condition, const std::string& message) {
  if (!condition) fail(message);
}

void expect_contains(const std::string& text,
                     const std::string& needle,
                     const std::string& message) {
  if (text.find(needle) == std::string::npos) {
    fail(message + "\nExpected: " + needle + "\nActual:\n" + text);
  }
}

void expect_not_contains(const std::string& text,
                         const std::string& needle,
                         const std::string& message) {
  if (text.find(needle) != std::string::npos) {
    fail(message + "\nUnexpected: " + needle + "\nActual:\n" + text);
  }
}

template <typename Fn>
void expect_throws_lir_verify(Fn&& fn,
                              const std::string& needle,
                              const std::string& message) {
  try {
    fn();
  } catch (const c4c::codegen::lir::LirVerifyError& ex) {
    expect_true(ex.kind() == c4c::codegen::lir::LirVerifyErrorKind::Malformed,
                message + "\nExpected malformed verifier error kind");
    expect_contains(ex.what(), needle, message);
    return;
  }
  fail(message + "\nExpected LIR verifier throw");
}

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

void test_backend_call_helpers_decode_structured_direct_global_call() {
  c4c::backend::BackendCallInst call{
      "%t0",
      "i32",
      "@add_pair",
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

std::string make_temp_output_path(const std::string& stem) {
  const auto base = std::filesystem::temp_directory_path();
  return (base / (stem + "_" + std::to_string(::getpid()) + ".o")).string();
}

std::string make_temp_path(const std::string& stem, const std::string& extension) {
  const auto base = std::filesystem::temp_directory_path();
  return (base / (stem + "_" + std::to_string(::getpid()) + extension)).string();
}

std::vector<std::uint8_t> read_file_bytes(const std::string& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    fail("failed to open file: " + path);
  }
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input),
                                   std::istreambuf_iterator<char>());
}

void append_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
  out.push_back(static_cast<std::uint8_t>(value & 0xff));
  out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
}

void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
  for (int shift = 0; shift < 32; shift += 8) {
    out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xff));
  }
}

void append_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
  for (int shift = 0; shift < 64; shift += 8) {
    out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xff));
  }
}

void append_i64(std::vector<std::uint8_t>& out, std::int64_t value) {
  append_u64(out, static_cast<std::uint64_t>(value));
}

void append_zeroes(std::vector<std::uint8_t>& out, std::size_t count) {
  out.insert(out.end(), count, 0);
}

std::size_t align_up(std::size_t value, std::size_t alignment) {
  if (alignment == 0) return value;
  const auto mask = alignment - 1;
  return (value + mask) & ~mask;
}

std::uint16_t read_u16(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
  return static_cast<std::uint16_t>(bytes[offset]) |
         (static_cast<std::uint16_t>(bytes[offset + 1]) << 8);
}

std::uint32_t read_u32(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
  return static_cast<std::uint32_t>(bytes[offset]) |
         (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
         (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) |
         (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

std::uint64_t read_u64(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
  std::uint64_t value = 0;
  for (int shift = 0; shift < 64; shift += 8) {
    value |= static_cast<std::uint64_t>(bytes[offset + (shift / 8)]) << shift;
  }
  return value;
}

std::vector<std::uint8_t> read_elf_entry_bytes(const std::vector<std::uint8_t>& image,
                                               std::size_t byte_count) {
  constexpr std::uint32_t kPtLoad = 1;

  if (image.size() < 64) fail("ELF image too small to read entry bytes");
  const auto entry_address = read_u64(image, 24);
  const auto program_header_offset = read_u64(image, 32);
  const auto program_header_size = read_u16(image, 54);
  const auto program_header_count = read_u16(image, 56);

  for (std::size_t index = 0; index < program_header_count; ++index) {
    const auto header_offset = program_header_offset + (index * program_header_size);
    if (header_offset + program_header_size > image.size()) break;
    if (read_u32(image, header_offset + 0) != kPtLoad) continue;

    const auto file_offset = read_u64(image, header_offset + 8);
    const auto virtual_address = read_u64(image, header_offset + 16);
    const auto file_size = read_u64(image, header_offset + 32);
    if (entry_address < virtual_address || entry_address >= virtual_address + file_size) continue;

    const auto entry_file_offset = file_offset + (entry_address - virtual_address);
    if (entry_file_offset + byte_count > image.size()) {
      fail("ELF entry bytes extend past the file image");
    }
    return std::vector<std::uint8_t>(image.begin() + static_cast<std::ptrdiff_t>(entry_file_offset),
                                     image.begin() + static_cast<std::ptrdiff_t>(entry_file_offset + byte_count));
  }

  fail("ELF image does not map the entry point through a PT_LOAD segment");
}

#if defined(__x86_64__)
int execute_x86_64_entry_bytes(const std::vector<std::uint8_t>& entry_bytes) {
  const long page_size = ::sysconf(_SC_PAGESIZE);
  if (page_size <= 0) fail("failed to read host page size for x86 runtime validation");
  const auto mapping_size = align_up(entry_bytes.size(), static_cast<std::size_t>(page_size));
  void* mapping = ::mmap(nullptr, mapping_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mapping == MAP_FAILED) fail("failed to map executable memory for x86 runtime validation");

  std::copy(entry_bytes.begin(), entry_bytes.end(), static_cast<std::uint8_t*>(mapping));
  const auto fn = reinterpret_cast<int (*)()>(mapping);
  const int result = fn();
  ::munmap(mapping, mapping_size);
  return result;
}
#endif

std::uint8_t symbol_info(std::uint8_t binding, std::uint8_t symbol_type) {
  return static_cast<std::uint8_t>((binding << 4) | symbol_type);
}

std::string format_ar_field(const std::string& value, std::size_t width) {
  if (value.size() >= width) return value.substr(0, width);
  return value + std::string(width - value.size(), ' ');
}

std::vector<std::uint8_t> make_minimal_relocation_object_fixture() {
  using namespace c4c::backend::elf;

  constexpr std::uint32_t kCall26Reloc = 279;
  constexpr std::size_t kElfHeaderSize = 64;
  constexpr std::size_t kSectionHeaderSize = 64;

  std::vector<std::uint8_t> text_bytes = {
      0x1f, 0x20, 0x03, 0xd5,
      0xc0, 0x03, 0x5f, 0xd6,
  };

  std::string strtab;
  strtab.push_back('\0');
  const auto main_name = static_cast<std::uint32_t>(strtab.size());
  strtab += "main";
  strtab.push_back('\0');
  const auto helper_name = static_cast<std::uint32_t>(strtab.size());
  strtab += "helper_ext";
  strtab.push_back('\0');

  std::string shstrtab;
  shstrtab.push_back('\0');
  const auto text_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".text";
  shstrtab.push_back('\0');
  const auto rela_text_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".rela.text";
  shstrtab.push_back('\0');
  const auto symtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".symtab";
  shstrtab.push_back('\0');
  const auto strtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".strtab";
  shstrtab.push_back('\0');
  const auto shstrtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".shstrtab";
  shstrtab.push_back('\0');

  std::vector<std::uint8_t> rela_text;
  append_u64(rela_text, 4);
  append_u64(rela_text, (static_cast<std::uint64_t>(3) << 32) | kCall26Reloc);
  append_i64(rela_text, 0);

  std::vector<std::uint8_t> symtab;
  append_zeroes(symtab, 24);
  append_u32(symtab, 0);
  symtab.push_back(symbol_info(STB_LOCAL, STT_SECTION));
  symtab.push_back(0);
  append_u16(symtab, 1);
  append_u64(symtab, 0);
  append_u64(symtab, 0);
  append_u32(symtab, main_name);
  symtab.push_back(symbol_info(STB_GLOBAL, STT_FUNC));
  symtab.push_back(0);
  append_u16(symtab, 1);
  append_u64(symtab, 0);
  append_u64(symtab, text_bytes.size());
  append_u32(symtab, helper_name);
  symtab.push_back(symbol_info(STB_GLOBAL, STT_NOTYPE));
  symtab.push_back(0);
  append_u16(symtab, SHN_UNDEF);
  append_u64(symtab, 0);
  append_u64(symtab, 0);

  std::size_t offset = kElfHeaderSize;
  const auto text_offset = offset;
  offset += text_bytes.size();
  offset = align_up(offset, 8);

  const auto rela_text_offset = offset;
  offset += rela_text.size();
  offset = align_up(offset, 8);

  const auto symtab_offset = offset;
  offset += symtab.size();

  const auto strtab_offset = offset;
  offset += strtab.size();

  const auto shstrtab_offset = offset;
  offset += shstrtab.size();
  offset = align_up(offset, 8);

  const auto section_header_offset = offset;

  std::vector<std::uint8_t> out;
  out.reserve(section_header_offset + 6 * kSectionHeaderSize);
  out.insert(out.end(), ELF_MAGIC.begin(), ELF_MAGIC.end());
  out.push_back(ELFCLASS64);
  out.push_back(ELFDATA2LSB);
  out.push_back(1);
  out.push_back(0);
  out.push_back(0);
  append_zeroes(out, 7);
  append_u16(out, ET_REL);
  append_u16(out, EM_AARCH64);
  append_u32(out, 1);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, section_header_offset);
  append_u32(out, 0);
  append_u16(out, kElfHeaderSize);
  append_u16(out, 0);
  append_u16(out, 0);
  append_u16(out, kSectionHeaderSize);
  append_u16(out, 6);
  append_u16(out, 5);

  out.insert(out.end(), text_bytes.begin(), text_bytes.end());
  append_zeroes(out, align_up(out.size(), 8) - out.size());
  out.insert(out.end(), rela_text.begin(), rela_text.end());
  append_zeroes(out, align_up(out.size(), 8) - out.size());
  out.insert(out.end(), symtab.begin(), symtab.end());
  out.insert(out.end(), strtab.begin(), strtab.end());
  out.insert(out.end(), shstrtab.begin(), shstrtab.end());
  append_zeroes(out, align_up(out.size(), 8) - out.size());

  append_zeroes(out, kSectionHeaderSize);

  append_u32(out, text_name);
  append_u32(out, SHT_PROGBITS);
  append_u64(out, SHF_ALLOC | SHF_EXECINSTR);
  append_u64(out, 0);
  append_u64(out, text_offset);
  append_u64(out, text_bytes.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 4);
  append_u64(out, 0);

  append_u32(out, rela_text_name);
  append_u32(out, SHT_RELA);
  append_u64(out, SHF_INFO_LINK);
  append_u64(out, 0);
  append_u64(out, rela_text_offset);
  append_u64(out, rela_text.size());
  append_u32(out, 3);
  append_u32(out, 1);
  append_u64(out, 8);
  append_u64(out, 24);

  append_u32(out, symtab_name);
  append_u32(out, SHT_SYMTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, symtab_offset);
  append_u64(out, symtab.size());
  append_u32(out, 4);
  append_u32(out, 2);
  append_u64(out, 8);
  append_u64(out, 24);

  append_u32(out, strtab_name);
  append_u32(out, SHT_STRTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, strtab_offset);
  append_u64(out, strtab.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 1);
  append_u64(out, 0);

  append_u32(out, shstrtab_name);
  append_u32(out, SHT_STRTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, shstrtab_offset);
  append_u64(out, shstrtab.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 1);
  append_u64(out, 0);

  return out;
}

std::vector<std::uint8_t> make_single_member_archive_fixture(
    const std::vector<std::uint8_t>& member,
    const std::string& member_name_with_trailing_slash) {

  std::vector<std::uint8_t> archive;
  archive.insert(archive.end(), {'!', '<', 'a', 'r', 'c', 'h', '>', '\n'});

  const auto append_text = [&](const std::string& text) {
    archive.insert(archive.end(), text.begin(), text.end());
  };

  append_text(format_ar_field(member_name_with_trailing_slash, 16));
  append_text(format_ar_field("0", 12));
  append_text(format_ar_field("0", 6));
  append_text(format_ar_field("0", 6));
  append_text(format_ar_field("644", 8));
  append_text(format_ar_field(std::to_string(member.size()), 10));
  archive.push_back('`');
  archive.push_back('\n');
  archive.insert(archive.end(), member.begin(), member.end());
  if ((archive.size() & 1u) != 0u) archive.push_back('\n');

  return archive;
}

std::vector<std::uint8_t> make_single_member_archive_fixture() {
  return make_single_member_archive_fixture(make_minimal_relocation_object_fixture(),
                                            "minimal_reloc.o/");
}

std::vector<std::uint8_t> make_minimal_helper_definition_object_fixture() {
  using namespace c4c::backend::elf;

  constexpr std::size_t kElfHeaderSize = 64;
  constexpr std::size_t kSectionHeaderSize = 64;

  std::vector<std::uint8_t> text_bytes = {
      0x40, 0x05, 0x80, 0x52,
      0xc0, 0x03, 0x5f, 0xd6,
  };

  std::string strtab;
  strtab.push_back('\0');
  const auto helper_name = static_cast<std::uint32_t>(strtab.size());
  strtab += "helper_ext";
  strtab.push_back('\0');

  std::string shstrtab;
  shstrtab.push_back('\0');
  const auto text_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".text";
  shstrtab.push_back('\0');
  const auto symtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".symtab";
  shstrtab.push_back('\0');
  const auto strtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".strtab";
  shstrtab.push_back('\0');
  const auto shstrtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".shstrtab";
  shstrtab.push_back('\0');

  std::vector<std::uint8_t> symtab;
  append_zeroes(symtab, 24);
  append_u32(symtab, 0);
  symtab.push_back(symbol_info(STB_LOCAL, STT_SECTION));
  symtab.push_back(0);
  append_u16(symtab, 1);
  append_u64(symtab, 0);
  append_u64(symtab, 0);
  append_u32(symtab, helper_name);
  symtab.push_back(symbol_info(STB_GLOBAL, STT_FUNC));
  symtab.push_back(0);
  append_u16(symtab, 1);
  append_u64(symtab, 0);
  append_u64(symtab, text_bytes.size());

  std::size_t offset = kElfHeaderSize;
  const auto text_offset = offset;
  offset += text_bytes.size();
  offset = align_up(offset, 8);

  const auto symtab_offset = offset;
  offset += symtab.size();

  const auto strtab_offset = offset;
  offset += strtab.size();

  const auto shstrtab_offset = offset;
  offset += shstrtab.size();
  offset = align_up(offset, 8);

  const auto section_header_offset = offset;

  std::vector<std::uint8_t> out;
  out.reserve(section_header_offset + 5 * kSectionHeaderSize);
  out.insert(out.end(), ELF_MAGIC.begin(), ELF_MAGIC.end());
  out.push_back(ELFCLASS64);
  out.push_back(ELFDATA2LSB);
  out.push_back(1);
  out.push_back(0);
  out.push_back(0);
  append_zeroes(out, 7);
  append_u16(out, ET_REL);
  append_u16(out, EM_AARCH64);
  append_u32(out, 1);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, section_header_offset);
  append_u32(out, 0);
  append_u16(out, kElfHeaderSize);
  append_u16(out, 0);
  append_u16(out, 0);
  append_u16(out, kSectionHeaderSize);
  append_u16(out, 5);
  append_u16(out, 4);

  out.insert(out.end(), text_bytes.begin(), text_bytes.end());
  append_zeroes(out, align_up(out.size(), 8) - out.size());
  out.insert(out.end(), symtab.begin(), symtab.end());
  out.insert(out.end(), strtab.begin(), strtab.end());
  out.insert(out.end(), shstrtab.begin(), shstrtab.end());
  append_zeroes(out, align_up(out.size(), 8) - out.size());

  append_zeroes(out, kSectionHeaderSize);

  append_u32(out, text_name);
  append_u32(out, SHT_PROGBITS);
  append_u64(out, SHF_ALLOC | SHF_EXECINSTR);
  append_u64(out, 0);
  append_u64(out, text_offset);
  append_u64(out, text_bytes.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 4);
  append_u64(out, 0);

  append_u32(out, symtab_name);
  append_u32(out, SHT_SYMTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, symtab_offset);
  append_u64(out, symtab.size());
  append_u32(out, 3);
  append_u32(out, 2);
  append_u64(out, 8);
  append_u64(out, 24);

  append_u32(out, strtab_name);
  append_u32(out, SHT_STRTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, strtab_offset);
  append_u64(out, strtab.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 1);
  append_u64(out, 0);

  append_u32(out, shstrtab_name);
  append_u32(out, SHT_STRTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, shstrtab_offset);
  append_u64(out, shstrtab.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 1);
  append_u64(out, 0);

  return out;
}

std::vector<std::uint8_t> make_minimal_x86_relocation_object_fixture() {
  using namespace c4c::backend::elf;

  constexpr std::uint32_t kPlt32Reloc = 4;
  constexpr std::size_t kElfHeaderSize = 64;
  constexpr std::size_t kSectionHeaderSize = 64;

  std::vector<std::uint8_t> text_bytes = {
      0xe8, 0x00, 0x00, 0x00, 0x00,
      0xc3,
  };

  std::string strtab;
  strtab.push_back('\0');
  const auto main_name = static_cast<std::uint32_t>(strtab.size());
  strtab += "main";
  strtab.push_back('\0');
  const auto helper_name = static_cast<std::uint32_t>(strtab.size());
  strtab += "helper_ext";
  strtab.push_back('\0');

  std::string shstrtab;
  shstrtab.push_back('\0');
  const auto text_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".text";
  shstrtab.push_back('\0');
  const auto rela_text_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".rela.text";
  shstrtab.push_back('\0');
  const auto symtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".symtab";
  shstrtab.push_back('\0');
  const auto strtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".strtab";
  shstrtab.push_back('\0');
  const auto shstrtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".shstrtab";
  shstrtab.push_back('\0');

  std::vector<std::uint8_t> rela_text;
  append_u64(rela_text, 1);
  append_u64(rela_text, (static_cast<std::uint64_t>(3) << 32) | kPlt32Reloc);
  append_i64(rela_text, -4);

  std::vector<std::uint8_t> symtab;
  append_zeroes(symtab, 24);
  append_u32(symtab, 0);
  symtab.push_back(symbol_info(STB_LOCAL, STT_SECTION));
  symtab.push_back(0);
  append_u16(symtab, 1);
  append_u64(symtab, 0);
  append_u64(symtab, 0);
  append_u32(symtab, main_name);
  symtab.push_back(symbol_info(STB_GLOBAL, STT_FUNC));
  symtab.push_back(0);
  append_u16(symtab, 1);
  append_u64(symtab, 0);
  append_u64(symtab, text_bytes.size());
  append_u32(symtab, helper_name);
  symtab.push_back(symbol_info(STB_GLOBAL, STT_NOTYPE));
  symtab.push_back(0);
  append_u16(symtab, SHN_UNDEF);
  append_u64(symtab, 0);
  append_u64(symtab, 0);

  std::size_t offset = kElfHeaderSize;
  const auto text_offset = offset;
  offset += text_bytes.size();
  offset = align_up(offset, 8);

  const auto rela_text_offset = offset;
  offset += rela_text.size();
  offset = align_up(offset, 8);

  const auto symtab_offset = offset;
  offset += symtab.size();

  const auto strtab_offset = offset;
  offset += strtab.size();

  const auto shstrtab_offset = offset;
  offset += shstrtab.size();
  offset = align_up(offset, 8);

  const auto section_header_offset = offset;

  std::vector<std::uint8_t> out;
  out.reserve(section_header_offset + 6 * kSectionHeaderSize);
  out.insert(out.end(), ELF_MAGIC.begin(), ELF_MAGIC.end());
  out.push_back(ELFCLASS64);
  out.push_back(ELFDATA2LSB);
  out.push_back(1);
  out.push_back(0);
  out.push_back(0);
  append_zeroes(out, 7);
  append_u16(out, ET_REL);
  append_u16(out, EM_X86_64);
  append_u32(out, 1);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, section_header_offset);
  append_u32(out, 0);
  append_u16(out, kElfHeaderSize);
  append_u16(out, 0);
  append_u16(out, 0);
  append_u16(out, kSectionHeaderSize);
  append_u16(out, 6);
  append_u16(out, 5);

  out.insert(out.end(), text_bytes.begin(), text_bytes.end());
  append_zeroes(out, align_up(out.size(), 8) - out.size());
  out.insert(out.end(), rela_text.begin(), rela_text.end());
  append_zeroes(out, align_up(out.size(), 8) - out.size());
  out.insert(out.end(), symtab.begin(), symtab.end());
  out.insert(out.end(), strtab.begin(), strtab.end());
  out.insert(out.end(), shstrtab.begin(), shstrtab.end());
  append_zeroes(out, align_up(out.size(), 8) - out.size());

  append_zeroes(out, kSectionHeaderSize);

  append_u32(out, text_name);
  append_u32(out, SHT_PROGBITS);
  append_u64(out, SHF_ALLOC | SHF_EXECINSTR);
  append_u64(out, 0);
  append_u64(out, text_offset);
  append_u64(out, text_bytes.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 1);
  append_u64(out, 0);

  append_u32(out, rela_text_name);
  append_u32(out, SHT_RELA);
  append_u64(out, SHF_INFO_LINK);
  append_u64(out, 0);
  append_u64(out, rela_text_offset);
  append_u64(out, rela_text.size());
  append_u32(out, 3);
  append_u32(out, 1);
  append_u64(out, 8);
  append_u64(out, 24);

  append_u32(out, symtab_name);
  append_u32(out, SHT_SYMTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, symtab_offset);
  append_u64(out, symtab.size());
  append_u32(out, 4);
  append_u32(out, 2);
  append_u64(out, 8);
  append_u64(out, 24);

  append_u32(out, strtab_name);
  append_u32(out, SHT_STRTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, strtab_offset);
  append_u64(out, strtab.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 1);
  append_u64(out, 0);

  append_u32(out, shstrtab_name);
  append_u32(out, SHT_STRTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, shstrtab_offset);
  append_u64(out, shstrtab.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 1);
  append_u64(out, 0);

  return out;
}

std::vector<std::uint8_t> make_minimal_x86_helper_definition_object_fixture() {
  using namespace c4c::backend::elf;

  constexpr std::size_t kElfHeaderSize = 64;
  constexpr std::size_t kSectionHeaderSize = 64;

  std::vector<std::uint8_t> text_bytes = {
      0xb8, 0x2a, 0x00, 0x00, 0x00,
      0xc3,
  };

  std::string strtab;
  strtab.push_back('\0');
  const auto helper_name = static_cast<std::uint32_t>(strtab.size());
  strtab += "helper_ext";
  strtab.push_back('\0');

  std::string shstrtab;
  shstrtab.push_back('\0');
  const auto text_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".text";
  shstrtab.push_back('\0');
  const auto symtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".symtab";
  shstrtab.push_back('\0');
  const auto strtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".strtab";
  shstrtab.push_back('\0');
  const auto shstrtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".shstrtab";
  shstrtab.push_back('\0');

  std::vector<std::uint8_t> symtab;
  append_zeroes(symtab, 24);
  append_u32(symtab, 0);
  symtab.push_back(symbol_info(STB_LOCAL, STT_SECTION));
  symtab.push_back(0);
  append_u16(symtab, 1);
  append_u64(symtab, 0);
  append_u64(symtab, 0);
  append_u32(symtab, helper_name);
  symtab.push_back(symbol_info(STB_GLOBAL, STT_FUNC));
  symtab.push_back(0);
  append_u16(symtab, 1);
  append_u64(symtab, 0);
  append_u64(symtab, text_bytes.size());

  std::size_t offset = kElfHeaderSize;
  const auto text_offset = offset;
  offset += text_bytes.size();
  offset = align_up(offset, 8);

  const auto symtab_offset = offset;
  offset += symtab.size();

  const auto strtab_offset = offset;
  offset += strtab.size();

  const auto shstrtab_offset = offset;
  offset += shstrtab.size();
  offset = align_up(offset, 8);

  const auto section_header_offset = offset;

  std::vector<std::uint8_t> out;
  out.reserve(section_header_offset + 5 * kSectionHeaderSize);
  out.insert(out.end(), ELF_MAGIC.begin(), ELF_MAGIC.end());
  out.push_back(ELFCLASS64);
  out.push_back(ELFDATA2LSB);
  out.push_back(1);
  out.push_back(0);
  out.push_back(0);
  append_zeroes(out, 7);
  append_u16(out, ET_REL);
  append_u16(out, EM_X86_64);
  append_u32(out, 1);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, section_header_offset);
  append_u32(out, 0);
  append_u16(out, kElfHeaderSize);
  append_u16(out, 0);
  append_u16(out, 0);
  append_u16(out, kSectionHeaderSize);
  append_u16(out, 5);
  append_u16(out, 4);

  out.insert(out.end(), text_bytes.begin(), text_bytes.end());
  append_zeroes(out, align_up(out.size(), 8) - out.size());
  out.insert(out.end(), symtab.begin(), symtab.end());
  out.insert(out.end(), strtab.begin(), strtab.end());
  out.insert(out.end(), shstrtab.begin(), shstrtab.end());
  append_zeroes(out, align_up(out.size(), 8) - out.size());

  append_zeroes(out, kSectionHeaderSize);

  append_u32(out, text_name);
  append_u32(out, SHT_PROGBITS);
  append_u64(out, SHF_ALLOC | SHF_EXECINSTR);
  append_u64(out, 0);
  append_u64(out, text_offset);
  append_u64(out, text_bytes.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 1);
  append_u64(out, 0);

  append_u32(out, symtab_name);
  append_u32(out, SHT_SYMTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, symtab_offset);
  append_u64(out, symtab.size());
  append_u32(out, 3);
  append_u32(out, 2);
  append_u64(out, 8);
  append_u64(out, 24);

  append_u32(out, strtab_name);
  append_u32(out, SHT_STRTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, strtab_offset);
  append_u64(out, strtab.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 1);
  append_u64(out, 0);

  append_u32(out, shstrtab_name);
  append_u32(out, SHT_STRTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, shstrtab_offset);
  append_u64(out, shstrtab.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 1);
  append_u64(out, 0);

  return out;
}

std::size_t find_section_index(const c4c::backend::linker_common::Elf64Object& object,
                               const std::string& name) {
  for (std::size_t index = 0; index < object.sections.size(); ++index) {
    if (object.sections[index].name == name) return index;
  }
  return object.sections.size();
}

void write_text_file(const std::string& path, const std::string& text) {
  std::ofstream output(path, std::ios::binary);
  if (!output) fail("failed to open file for write: " + path);
  output << text;
  if (!output.good()) fail("failed to write file: " + path);
}

void write_binary_file(const std::string& path, const std::vector<std::uint8_t>& bytes) {
  std::ofstream output(path, std::ios::binary);
  if (!output) fail("failed to open file for write: " + path);
  output.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
  if (!output.good()) fail("failed to write file: " + path);
}

std::string shell_quote(const std::string& text) {
  std::string quoted = "'";
  for (const char ch : text) {
    if (ch == '\'') {
      quoted += "'\\''";
    } else {
      quoted.push_back(ch);
    }
  }
  quoted.push_back('\'');
  return quoted;
}

std::string run_command_capture(const std::string& command) {
  FILE* pipe = popen(command.c_str(), "r");
  if (!pipe) fail("failed to run command: " + command);

  std::string output;
  char buffer[4096];
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }

  const int status = pclose(pipe);
  if (status != 0) {
    fail("command failed (" + std::to_string(status) + "): " + command + "\n" + output);
  }
  return output;
}

std::string normalize_objdump_surface(const std::string& text) {
  std::stringstream input(text);
  std::string line;
  std::string normalized;
  while (std::getline(input, line)) {
    const auto file_format_pos = line.find("file format ");
    if (file_format_pos != std::string::npos) {
      line = line.substr(file_format_pos);
    }
    normalized += line;
    normalized.push_back('\n');
  }
  return normalized;
}

std::string normalize_objdump_disassembly(const std::string& text) {
  std::stringstream input(text);
  std::string line;
  std::string normalized;
  while (std::getline(input, line)) {
    const auto file_format_pos = line.find("file format ");
    if (file_format_pos != std::string::npos) continue;

    const auto colon_pos = line.find(':');
    if (colon_pos != std::string::npos && colon_pos < 18) {
      const auto asm_pos = line.find_first_not_of(" \t", colon_pos + 1);
      if (asm_pos != std::string::npos) {
        line = line.substr(asm_pos);
      }
    }

    normalized += line;
    normalized.push_back('\n');
  }
  return normalized;
}

c4c::codegen::lir::LirModule make_return_zero_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-n8:16:32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock block;
  block.id = LirBlockId{0};
  block.label = "entry";
  block.terminator = LirRet{std::string("0"), "i32"};
  function.blocks.push_back(std::move(block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_return_add_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  auto& function = module.functions.front();
  auto& block = function.blocks.front();
  block.insts.push_back(LirBinOp{"%t0", "add", "i32", "2", "3"});
  block.terminator = LirRet{std::string("%t0"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_return_sub_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  auto& function = module.functions.front();
  auto& block = function.blocks.front();
  block.insts.push_back(LirBinOp{"%t0", "sub", "i32", "3", "3"});
  block.terminator = LirRet{std::string("%t0"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_typed_return_add_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  auto& function = module.functions.front();
  auto& block = function.blocks.front();
  block.insts.push_back(LirBinOp{"%t0", LirBinaryOpcode::Add, "i32", "2", "3"});
  block.terminator = LirRet{std::string("%t0"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_void_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "helper";
  function.signature_text = "define void @helper()\n";
  function.entry = LirBlockId{0};

  LirBlock block;
  block.id = LirBlockId{0};
  block.label = "entry";
  block.terminator = LirRet{};
  function.blocks.push_back(std::move(block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_declaration_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.Pair = type { i32, i32 }");

  LirFunction decl;
  decl.name = "helper";
  decl.signature_text = "declare i32 @helper(i32)\n";
  decl.is_declaration = true;

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@helper", "", "i32 5"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  main_fn.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(decl));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_call_crossing_interval_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction decl;
  decl.name = "helper";
  decl.signature_text = "declare i32 @helper(i32)\n";
  decl.is_declaration = true;

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "2", "3"});
  entry.insts.push_back(LirCallOp{"%t1", "i32", "@helper", "", "i32 %t0"});
  entry.insts.push_back(LirBinOp{"%t2", "add", "i32", "%t0", "%t1"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};
  main_fn.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(decl));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_call_crossing_direct_call_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_one";
  helper.signature_text = "define i32 @add_one(i32 %p.x)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "1"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "2", "3"});
  entry.insts.push_back(LirCallOp{"%t1", "i32", "@add_one", "(i32)", "i32 %t0"});
  entry.insts.push_back(LirBinOp{"%t2", "add", "i32", "%t0", "%t1"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};
  main_fn.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_call_crossing_direct_call_with_spacing_module() {
  auto module = make_typed_call_crossing_direct_call_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts[1]);
  call.callee_type_suffix = "( i32 )";
  call.args_str = "  i32   %t0  ";
  return module;
}

c4c::codegen::lir::LirModule make_interval_phi_join_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "slt", "i32", "1", "2"});
  entry.terminator = LirCondBr{"%t0", "then", "else"};

  LirBlock then_block;
  then_block.id = LirBlockId{1};
  then_block.label = "then";
  then_block.insts.push_back(LirBinOp{"%t1", "add", "i32", "10", "1"});
  then_block.terminator = LirBr{"join"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.insts.push_back(LirBinOp{"%t2", "add", "i32", "20", "2"});
  else_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(LirPhiOp{"%t3", "i32", {{"%t1", "then"}, {"%t2", "else"}}});
  join_block.insts.push_back(LirBinOp{"%t4", "add", "i32", "%t3", "5"});
  join_block.terminator = LirRet{std::string("%t4"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_non_overlapping_interval_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "1", "2"});
  entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%t0", "3"});
  entry.insts.push_back(LirBinOp{"%t2", "add", "i32", "4", "5"});
  entry.insts.push_back(LirBinOp{"%t3", "add", "i32", "%t2", "6"});
  entry.terminator = LirRet{std::string("%t3"), "i32"};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_overlapping_interval_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "1", "2"});
  entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "4", "5"});
  entry.insts.push_back(LirBinOp{"%t2", "add", "i32", "%t0", "%t1"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_conditional_return_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  auto& function = module.functions.front();
  function.blocks.clear();
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "slt", "i32", "2", "3"});
  entry.insts.push_back(
      LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "then", "else"};

  LirBlock then_block;
  then_block.id = LirBlockId{1};
  then_block.label = "then";
  then_block.terminator = LirRet{std::string("0"), "i32"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.terminator = LirRet{std::string("1"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  return module;
}

c4c::codegen::lir::LirModule make_conditional_return_le_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  auto& function = module.functions.front();
  function.blocks.clear();
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "sle", "i32", "2", "3"});
  entry.insts.push_back(
      LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "then", "else"};

  LirBlock then_block;
  then_block.id = LirBlockId{1};
  then_block.label = "then";
  then_block.terminator = LirRet{std::string("0"), "i32"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.terminator = LirRet{std::string("1"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  return module;
}

c4c::codegen::lir::LirModule make_conditional_return_gt_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  auto& function = module.functions.front();
  function.blocks.clear();
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "sgt", "i32", "3", "2"});
  entry.insts.push_back(
      LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "then", "else"};

  LirBlock then_block;
  then_block.id = LirBlockId{1};
  then_block.label = "then";
  then_block.terminator = LirRet{std::string("0"), "i32"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.terminator = LirRet{std::string("1"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  return module;
}

c4c::codegen::lir::LirModule make_conditional_return_ge_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  auto& function = module.functions.front();
  function.blocks.clear();
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "sge", "i32", "3", "2"});
  entry.insts.push_back(
      LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "then", "else"};

  LirBlock then_block;
  then_block.id = LirBlockId{1};
  then_block.label = "then";
  then_block.terminator = LirRet{std::string("0"), "i32"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.terminator = LirRet{std::string("1"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  return module;
}

c4c::codegen::lir::LirModule make_conditional_return_eq_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  auto& function = module.functions.front();
  function.blocks.clear();
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "2", "2"});
  entry.insts.push_back(
      LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "then", "else"};

  LirBlock then_block;
  then_block.id = LirBlockId{1};
  then_block.label = "then";
  then_block.terminator = LirRet{std::string("0"), "i32"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.terminator = LirRet{std::string("1"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  return module;
}

c4c::codegen::lir::LirModule make_conditional_return_ne_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  auto& function = module.functions.front();
  function.blocks.clear();
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "ne", "i32", "2", "3"});
  entry.insts.push_back(
      LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "then", "else"};

  LirBlock then_block;
  then_block.id = LirBlockId{1};
  then_block.label = "then";
  then_block.terminator = LirRet{std::string("0"), "i32"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.terminator = LirRet{std::string("1"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  return module;
}

c4c::codegen::lir::LirModule make_conditional_return_ult_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  auto& function = module.functions.front();
  function.blocks.clear();
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "ult", "i32", "2", "3"});
  entry.insts.push_back(
      LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "then", "else"};

  LirBlock then_block;
  then_block.id = LirBlockId{1};
  then_block.label = "then";
  then_block.terminator = LirRet{std::string("0"), "i32"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.terminator = LirRet{std::string("1"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  return module;
}

c4c::codegen::lir::LirModule make_conditional_return_ule_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  auto& function = module.functions.front();
  function.blocks.clear();
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "ule", "i32", "2", "3"});
  entry.insts.push_back(
      LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "then", "else"};

  LirBlock then_block;
  then_block.id = LirBlockId{1};
  then_block.label = "then";
  then_block.terminator = LirRet{std::string("0"), "i32"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.terminator = LirRet{std::string("1"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  return module;
}

c4c::codegen::lir::LirModule make_conditional_return_ugt_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  auto& function = module.functions.front();
  function.blocks.clear();
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "ugt", "i32", "3", "2"});
  entry.insts.push_back(
      LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "then", "else"};

  LirBlock then_block;
  then_block.id = LirBlockId{1};
  then_block.label = "then";
  then_block.terminator = LirRet{std::string("0"), "i32"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.terminator = LirRet{std::string("1"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  return module;
}

c4c::codegen::lir::LirModule make_conditional_return_uge_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  auto& function = module.functions.front();
  function.blocks.clear();
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "uge", "i32", "3", "2"});
  entry.insts.push_back(
      LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "then", "else"};

  LirBlock then_block;
  then_block.id = LirBlockId{1};
  then_block.label = "then";
  then_block.terminator = LirRet{std::string("0"), "i32"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.terminator = LirRet{std::string("1"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  return module;
}

c4c::codegen::lir::LirModule make_direct_call_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");

  LirFunction helper;
  helper.name = "helper";
  helper.signature_text = "define i32 @helper()\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.terminator = LirRet{std::string("7"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirCallOp{"%t0", "i32", "@helper", "", ""});
  main_entry.terminator = LirRet{std::string("%t0"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_local_temp_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_local_temp_sub_module() {
  using namespace c4c::codegen::lir;

  auto module = make_local_temp_module();
  auto& function = module.functions.front();
  auto& entry = function.blocks.front();
  entry.insts.push_back(LirBinOp{"%t1", "sub", "i32", "%t0", "4"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_local_temp_arithmetic_chain_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { i32, i32, ptr, ptr }");

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "1", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.insts.push_back(LirBinOp{"%t1", "mul", "i32", "%t0", "10"});
  entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  entry.insts.push_back(LirBinOp{"%t3", "sdiv", "i32", "%t2", "2"});
  entry.insts.push_back(LirStoreOp{"i32", "%t3", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t4", "i32", "%lv.x"});
  entry.insts.push_back(LirBinOp{"%t5", "srem", "i32", "%t4", "3"});
  entry.insts.push_back(LirStoreOp{"i32", "%t5", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t6", "i32", "%lv.x"});
  entry.insts.push_back(LirBinOp{"%t7", "sub", "i32", "%t6", "2"});
  entry.terminator = LirRet{std::string("%t7"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_two_local_temp_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.y"});
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_local_pointer_temp_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.p", "ptr", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "4", "%lv.x"});
  entry.insts.push_back(LirStoreOp{"ptr", "%lv.x", "%lv.p"});
  entry.insts.push_back(LirLoadOp{"%t0", "ptr", "%lv.p"});
  entry.insts.push_back(LirStoreOp{"i32", "0", "%t0"});
  entry.insts.push_back(LirLoadOp{"%t1", "ptr", "%lv.p"});
  entry.insts.push_back(LirLoadOp{"%t2", "i32", "%t1"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_x86_local_pointer_temp_return_module() {
  auto module = make_local_pointer_temp_return_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  return module;
}

c4c::codegen::lir::LirModule make_double_indirect_local_pointer_conditional_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { i32, i32, ptr, ptr }");

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.p", "ptr", "", 8});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.pp", "ptr", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.x"});
  entry.insts.push_back(LirStoreOp{"ptr", "%lv.x", "%lv.p"});
  entry.insts.push_back(LirStoreOp{"ptr", "%lv.p", "%lv.pp"});
  entry.insts.push_back(LirLoadOp{"%t0", "ptr", "%lv.p"});
  entry.insts.push_back(LirLoadOp{"%t1", "i32", "%t0"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "block_1", "block_2"};

  LirBlock block_1;
  block_1.id = LirBlockId{1};
  block_1.label = "block_1";
  block_1.terminator = LirRet{std::string("1"), "i32"};

  LirBlock block_2;
  block_2.id = LirBlockId{2};
  block_2.label = "block_2";
  block_2.insts.push_back(LirLoadOp{"%t3", "ptr", "%lv.pp"});
  block_2.insts.push_back(LirLoadOp{"%t4", "ptr", "%t3"});
  block_2.insts.push_back(LirLoadOp{"%t5", "i32", "%t4"});
  block_2.insts.push_back(LirCmpOp{"%t6", false, "ne", "i32", "%t5", "0"});
  block_2.terminator = LirCondBr{"%t6", "block_3", "block_4"};

  LirBlock block_3;
  block_3.id = LirBlockId{3};
  block_3.label = "block_3";
  block_3.terminator = LirRet{std::string("1"), "i32"};

  LirBlock block_4;
  block_4.id = LirBlockId{4};
  block_4.label = "block_4";
  block_4.insts.push_back(LirLoadOp{"%t7", "ptr", "%lv.pp"});
  block_4.insts.push_back(LirLoadOp{"%t8", "ptr", "%t7"});
  block_4.insts.push_back(LirStoreOp{"i32", "1", "%t8"});
  block_4.terminator = LirBr{"block_5"};

  LirBlock block_5;
  block_5.id = LirBlockId{5};
  block_5.label = "block_5";
  block_5.insts.push_back(LirLoadOp{"%t9", "i32", "%lv.x"});
  block_5.insts.push_back(LirCmpOp{"%t10", false, "ne", "i32", "%t9", "0"});
  block_5.terminator = LirCondBr{"%t10", "block_6", "block_7"};

  LirBlock block_6;
  block_6.id = LirBlockId{6};
  block_6.label = "block_6";
  block_6.terminator = LirRet{std::string("0"), "i32"};

  LirBlock block_7;
  block_7.id = LirBlockId{7};
  block_7.label = "block_7";
  block_7.terminator = LirRet{std::string("1"), "i32"};

  LirBlock block_8;
  block_8.id = LirBlockId{8};
  block_8.label = "block_8";
  block_8.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  function.blocks.push_back(std::move(block_3));
  function.blocks.push_back(std::move(block_4));
  function.blocks.push_back(std::move(block_5));
  function.blocks.push_back(std::move(block_6));
  function.blocks.push_back(std::move(block_7));
  function.blocks.push_back(std::move(block_8));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_goto_only_constant_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { i32, i32, ptr, ptr }");

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.terminator = LirBr{"ulbl_start"};

  LirBlock start;
  start.id = LirBlockId{1};
  start.label = "ulbl_start";
  start.terminator = LirBr{"block_1"};

  LirBlock block_1;
  block_1.id = LirBlockId{2};
  block_1.label = "block_1";
  block_1.terminator = LirBr{"ulbl_next"};

  LirBlock success;
  success.id = LirBlockId{3};
  success.label = "ulbl_success";
  success.terminator = LirBr{"block_2"};

  LirBlock block_2;
  block_2.id = LirBlockId{4};
  block_2.label = "block_2";
  block_2.terminator = LirRet{std::string("0"), "i32"};

  LirBlock next;
  next.id = LirBlockId{5};
  next.label = "ulbl_next";
  next.terminator = LirBr{"block_3"};

  LirBlock block_3;
  block_3.id = LirBlockId{6};
  block_3.label = "block_3";
  block_3.terminator = LirBr{"ulbl_foo"};

  LirBlock foo;
  foo.id = LirBlockId{7};
  foo.label = "ulbl_foo";
  foo.terminator = LirBr{"block_4"};

  LirBlock block_4;
  block_4.id = LirBlockId{8};
  block_4.label = "block_4";
  block_4.terminator = LirBr{"block_2"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(start));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(success));
  function.blocks.push_back(std::move(block_2));
  function.blocks.push_back(std::move(next));
  function.blocks.push_back(std::move(block_3));
  function.blocks.push_back(std::move(foo));
  function.blocks.push_back(std::move(block_4));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_countdown_while_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { i32, i32, ptr, ptr }");

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "50", "%lv.x"});
  entry.terminator = LirBr{"block_1"};

  LirBlock block_1;
  block_1.id = LirBlockId{1};
  block_1.label = "block_1";
  block_1.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  block_1.insts.push_back(LirCmpOp{"%t1", false, "ne", "i32", "%t0", "0"});
  block_1.terminator = LirCondBr{"%t1", "block_2", "block_3"};

  LirBlock block_2;
  block_2.id = LirBlockId{2};
  block_2.label = "block_2";
  block_2.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  block_2.insts.push_back(LirBinOp{"%t3", "sub", "i32", "%t2", "1"});
  block_2.insts.push_back(LirStoreOp{"i32", "%t3", "%lv.x"});
  block_2.terminator = LirBr{"block_1"};

  LirBlock block_3;
  block_3.id = LirBlockId{3};
  block_3.label = "block_3";
  block_3.insts.push_back(LirLoadOp{"%t4", "i32", "%lv.x"});
  block_3.terminator = LirRet{std::string("%t4"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  function.blocks.push_back(std::move(block_3));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_countdown_do_while_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { i32, i32, ptr, ptr }");

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "50", "%lv.x"});
  entry.terminator = LirBr{"block_1"};

  LirBlock block_1;
  block_1.id = LirBlockId{1};
  block_1.label = "block_1";
  block_1.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  block_1.insts.push_back(LirBinOp{"%t1", "sub", "i32", "%t0", "1"});
  block_1.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.x"});
  block_1.terminator = LirBr{"block_2"};

  LirBlock block_2;
  block_2.id = LirBlockId{2};
  block_2.label = "block_2";
  block_2.terminator = LirBr{"dowhile.cond.1"};

  LirBlock cond;
  cond.id = LirBlockId{3};
  cond.label = "dowhile.cond.1";
  cond.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  cond.insts.push_back(LirCmpOp{"%t3", false, "ne", "i32", "%t2", "0"});
  cond.terminator = LirCondBr{"%t3", "block_1", "block_3"};

  LirBlock block_3;
  block_3.id = LirBlockId{4};
  block_3.label = "block_3";
  block_3.insts.push_back(LirLoadOp{"%t4", "i32", "%lv.x"});
  block_3.terminator = LirRet{std::string("%t4"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  function.blocks.push_back(std::move(cond));
  function.blocks.push_back(std::move(block_3));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_typed_countdown_while_return_module() {
  using namespace c4c::codegen::lir;

  auto module = make_countdown_while_return_module();
  auto& function = module.functions.front();
  auto& cond_block = function.blocks[1];
  auto& body_block = function.blocks[2];
  std::get<LirCmpOp>(cond_block.insts[1]).predicate = LirCmpPredicate::Ne;
  std::get<LirBinOp>(body_block.insts[1]).opcode = LirBinaryOpcode::Sub;
  return module;
}

c4c::codegen::lir::LirModule make_typed_conditional_return_module() {
  using namespace c4c::codegen::lir;

  auto module = make_conditional_return_module();
  auto& function = module.functions.front();
  auto& entry = function.blocks.front();
  std::get<LirCmpOp>(entry.insts[0]).predicate = LirCmpPredicate::Slt;
  std::get<LirCmpOp>(entry.insts[2]).predicate = LirCmpPredicate::Ne;
  return module;
}

c4c::codegen::lir::LirModule make_param_slot_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main(i32 %p.x)\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.param.x", "i32", "", 4});
  function.alloca_insts.push_back(LirStoreOp{"i32", "%p.x", "%lv.param.x"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.param.x"});
  entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%t0", "1"});
  entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.param.x"});
  entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.param.x"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_param_slot_runtime_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");

  LirFunction helper;
  helper.name = "add_one";
  helper.signature_text = "define i32 @add_one(i32 %p.x)\n";
  helper.entry = LirBlockId{0};
  helper.alloca_insts.push_back(LirAllocaOp{"%lv.param.x", "i32", "", 4});
  helper.alloca_insts.push_back(LirStoreOp{"i32", "%p.x", "%lv.param.x"});

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.param.x"});
  helper_entry.insts.push_back(
      LirBinOp{"%t1", LirBinaryOpcode::Add, "i32", "%t0", "1"});
  helper_entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.param.x"});
  helper_entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.param.x"});
  helper_entry.terminator = LirRet{std::string("%t2"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirCallOp{
      "%t0",
      "i32",
      LirOperand(std::string("@add_one"), LirOperandKind::Global),
      "(i32)",
      "i32 5",
  });
  main_entry.terminator = LirRet{std::string("%t0"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_one";
  helper.signature_text = "define i32 @add_one(i32 %p.x)\n";
  helper.entry = LirBlockId{0};
  helper.alloca_insts.push_back(LirAllocaOp{"%lv.param.x", "i32", "", 4});
  helper.alloca_insts.push_back(LirStoreOp{"i32", "%p.x", "%lv.param.x"});

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.param.x"});
  helper_entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%t0", "1"});
  helper_entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.param.x"});
  helper_entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.param.x"});
  helper_entry.terminator = LirRet{std::string("%t2"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirCallOp{"%t0", "i32", "@add_one", "(i32)", "i32 5"});
  main_entry.terminator = LirRet{std::string("%t0"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_pair";
  helper.signature_text = "define i32 @add_pair(i32 %p.x, i32 %p.y)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(
      LirBinOp{"%t0", LirBinaryOpcode::Add, "i32", "%p.x", "%p.y"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirCallOp{
      "%t0",
      "i32",
      LirOperand(std::string("@add_pair"), LirOperandKind::Global),
      "(i32, i32)",
      "i32 5, i32 7",
  });
  main_entry.terminator = LirRet{std::string("%t0"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_local_arg_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_one";
  helper.signature_text = "define i32 @add_one(i32 %p.x)\n";
  helper.entry = LirBlockId{0};
  helper.alloca_insts.push_back(LirAllocaOp{"%lv.param.x", "i32", "", 4});
  helper.alloca_insts.push_back(LirStoreOp{"i32", "%p.x", "%lv.param.x"});

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.param.x"});
  helper_entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%t0", "1"});
  helper_entry.terminator = LirRet{std::string("%t1"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  main_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  main_entry.insts.push_back(LirCallOp{"%t1", "i32", "@add_one", "(i32)", "i32 %t0"});
  main_entry.terminator = LirRet{std::string("%t1"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_local_arg_with_spacing_module() {
  auto module = make_typed_direct_call_local_arg_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.back());
  call.args_str = "  i32   %t0  ";
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_local_arg_with_suffix_spacing_module() {
  auto module = make_typed_direct_call_local_arg_with_spacing_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.back());
  call.callee_type_suffix = "( i32 )";
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_local_arg_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_pair";
  helper.signature_text = "define i32 @add_pair(i32 %p.x, i32 %p.y)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "%p.y"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  main_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  main_entry.insts.push_back(
      LirCallOp{"%t1", "i32", "@add_pair", "(i32, i32)", "i32 %t0, i32 7"});
  main_entry.terminator = LirRet{std::string("%t1"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_local_arg_with_spacing_module() {
  auto module = make_typed_direct_call_two_arg_local_arg_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.back());
  call.callee_type_suffix = "( i32 , i32 )";
  call.args_str = "  i32   %t0 ,   i32  7  ";
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_second_local_arg_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_pair";
  helper.signature_text = "define i32 @add_pair(i32 %p.x, i32 %p.y)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "%p.y"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.y"});
  main_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.y"});
  main_entry.insts.push_back(
      LirCallOp{"%t1", "i32", "@add_pair", "(i32, i32)", "i32 5, i32 %t0"});
  main_entry.terminator = LirRet{std::string("%t1"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_second_local_rewrite_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_pair";
  helper.signature_text = "define i32 @add_pair(i32 %p.x, i32 %p.y)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "%p.y"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.y"});
  main_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.y"});
  main_entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%t0", "0"});
  main_entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.y"});
  main_entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.y"});
  main_entry.insts.push_back(
      LirCallOp{"%t3", "i32", "@add_pair", "(i32, i32)", "i32 5, i32 %t2"});
  main_entry.terminator = LirRet{std::string("%t3"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_first_local_rewrite_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_pair";
  helper.signature_text = "define i32 @add_pair(i32 %p.x, i32 %p.y)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "%p.y"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  main_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  main_entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%t0", "0"});
  main_entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.x"});
  main_entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  main_entry.insts.push_back(
      LirCallOp{"%t3", "i32", "@add_pair", "(i32, i32)", "i32 %t2, i32 7"});
  main_entry.terminator = LirRet{std::string("%t3"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule
make_typed_direct_call_two_arg_first_local_rewrite_with_spacing_module() {
  auto module = make_typed_direct_call_two_arg_first_local_rewrite_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.back());
  call.args_str = " i32   %t2 ,   i32 7 ";
  return module;
}

c4c::codegen::lir::LirModule
make_typed_direct_call_two_arg_first_local_rewrite_with_suffix_spacing_module() {
  auto module = make_typed_direct_call_two_arg_first_local_rewrite_with_spacing_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.back());
  call.callee_type_suffix = "( i32 , i32 )";
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_both_local_arg_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_pair";
  helper.signature_text = "define i32 @add_pair(i32 %p.x, i32 %p.y)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "%p.y"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  main_entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.y"});
  main_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  main_entry.insts.push_back(LirLoadOp{"%t1", "i32", "%lv.y"});
  main_entry.insts.push_back(
      LirCallOp{"%t2", "i32", "@add_pair", "(i32, i32)", "i32 %t0, i32 %t1"});
  main_entry.terminator = LirRet{std::string("%t2"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_both_local_first_rewrite_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_pair";
  helper.signature_text = "define i32 @add_pair(i32 %p.x, i32 %p.y)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "%p.y"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  main_entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.y"});
  main_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  main_entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%t0", "0"});
  main_entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.x"});
  main_entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  main_entry.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.y"});
  main_entry.insts.push_back(
      LirCallOp{"%t4", "i32", "@add_pair", "(i32, i32)", "i32 %t2, i32 %t3"});
  main_entry.terminator = LirRet{std::string("%t4"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_both_local_second_rewrite_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_pair";
  helper.signature_text = "define i32 @add_pair(i32 %p.x, i32 %p.y)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "%p.y"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  main_entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.y"});
  main_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.y"});
  main_entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%t0", "0"});
  main_entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.y"});
  main_entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  main_entry.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.y"});
  main_entry.insts.push_back(
      LirCallOp{"%t4", "i32", "@add_pair", "(i32, i32)", "i32 %t2, i32 %t3"});
  main_entry.terminator = LirRet{std::string("%t4"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_both_local_double_rewrite_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_pair";
  helper.signature_text = "define i32 @add_pair(i32 %p.x, i32 %p.y)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "%p.y"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  main_entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.y"});
  main_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  main_entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%t0", "0"});
  main_entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.x"});
  main_entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.y"});
  main_entry.insts.push_back(LirBinOp{"%t3", "add", "i32", "%t2", "0"});
  main_entry.insts.push_back(LirStoreOp{"i32", "%t3", "%lv.y"});
  main_entry.insts.push_back(LirLoadOp{"%t4", "i32", "%lv.x"});
  main_entry.insts.push_back(LirLoadOp{"%t5", "i32", "%lv.y"});
  main_entry.insts.push_back(
      LirCallOp{"%t6", "i32", "@add_pair", "(i32, i32)", "i32 %t4, i32 %t5"});
  main_entry.terminator = LirRet{std::string("%t6"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_local_array_gep_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.arr", "[2 x i32]", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "[2 x i32]", "%lv.arr", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(
      LirCastOp{"%t1", LirCastKind::SExt, "i32", "0", "i64"});
  entry.insts.push_back(LirGepOp{"%t2", "i32", "%t0", false, {"i64 %t1"}});
  entry.insts.push_back(LirStoreOp{"i32", "4", "%t2"});
  entry.insts.push_back(
      LirGepOp{"%t3", "[2 x i32]", "%lv.arr", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(
      LirCastOp{"%t4", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t5", "i32", "%t3", false, {"i64 %t4"}});
  entry.insts.push_back(LirStoreOp{"i32", "3", "%t5"});
  entry.insts.push_back(LirLoadOp{"%t6", "i32", "%t2"});
  entry.insts.push_back(LirLoadOp{"%t7", "i32", "%t5"});
  entry.insts.push_back(LirBinOp{"%t8", "add", "i32", "%t6", "%t7"});
  entry.terminator = LirRet{std::string("%t8"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_param_member_array_gep_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.Pair = type { [2 x i32] }");

  LirFunction function;
  function.name = "get_second";
  function.signature_text = "define i32 @get_second(%struct.Pair %p.p)\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.param.p", "%struct.Pair", "", 4});
  function.alloca_insts.push_back(
      LirStoreOp{"%struct.Pair", "%p.p", "%lv.param.p"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "%struct.Pair", "%lv.param.p", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(
      LirGepOp{"%t1", "[2 x i32]", "%t0", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(
      LirCastOp{"%t2", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t3", "i32", "%t1", false, {"i64 %t2"}});
  entry.insts.push_back(LirLoadOp{"%t4", "i32", "%t3"});
  entry.terminator = LirRet{std::string("%t4"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_nested_member_pointer_array_gep_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.Inner = type { [2 x i32] }");
  module.type_decls.push_back("%struct.Outer = type { ptr }");

  LirFunction function;
  function.name = "get_second";
  function.signature_text = "define i32 @get_second(ptr %p.o)\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "%struct.Outer", "%p.o", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(LirLoadOp{"%t1", "ptr", "%t0"});
  entry.insts.push_back(
      LirGepOp{"%t2", "%struct.Inner", "%t1", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(
      LirGepOp{"%t3", "[2 x i32]", "%t2", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(
      LirCastOp{"%t4", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t5", "i32", "%t3", false, {"i64 %t4"}});
  entry.insts.push_back(LirLoadOp{"%t6", "i32", "%t5"});
  entry.terminator = LirRet{std::string("%t6"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_nested_param_member_array_gep_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.Inner = type { [2 x i32] }");
  module.type_decls.push_back("%struct.Outer = type { %struct.Inner }");

  LirFunction function;
  function.name = "get_second";
  function.signature_text = "define i32 @get_second(%struct.Outer %p.o)\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.param.o", "%struct.Outer", "", 4});
  function.alloca_insts.push_back(
      LirStoreOp{"%struct.Outer", "%p.o", "%lv.param.o"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "%struct.Outer", "%lv.param.o", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(
      LirGepOp{"%t1", "%struct.Inner", "%t0", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(
      LirCastOp{"%t2", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t3", "i32", "%t1", false, {"i64 %t2"}});
  entry.insts.push_back(LirLoadOp{"%t4", "i32", "%t3"});
  entry.terminator = LirRet{std::string("%t4"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_global_load_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.globals.push_back(LirGlobal{
      LirGlobalId{0},
      "g_counter",
      {},
      false,
      false,
      "",
      "global ",
      "i32",
      "11",
      4,
      false,
  });

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "@g_counter"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_string_literal_char_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.string_pool.push_back(LirStringConst{"@.str0", "hi", 3});

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "[3 x i8]", "@.str0", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t2", "i8", "%t0", false, {"i64 %t1"}});
  entry.insts.push_back(LirLoadOp{"%t3", "i8", "%t2"});
  entry.insts.push_back(
      LirCastOp{"%t4", LirCastKind::SExt, "i8", "%t3", "i32"});
  entry.terminator = LirRet{std::string("%t4"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_extern_decl_call_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.extern_decls.push_back(LirExternDecl{"helper_ext", "i32"});

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@helper_ext", "", "i32 5"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_x86_extern_decl_object_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-n8:16:32:64-S128";
  module.extern_decls.push_back(LirExternDecl{"helper_ext", "i32"});

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@helper_ext", "", ""});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_extern_global_load_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.globals.push_back(LirGlobal{
      LirGlobalId{0},
      "ext_counter",
      {},
      false,
      false,
      "external ",
      "global ",
      "i32",
      "",
      4,
      true,
  });

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "@ext_counter"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_extern_global_array_load_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.globals.push_back(LirGlobal{
      LirGlobalId{0},
      "ext_arr",
      {},
      false,
      false,
      "external ",
      "global ",
      "[2 x i32]",
      "",
      4,
      true,
  });

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "[2 x i32]", "@ext_arr", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t2", "i32", "%t0", false, {"i64 %t1"}});
  entry.insts.push_back(LirLoadOp{"%t3", "i32", "%t2"});
  entry.terminator = LirRet{std::string("%t3"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_x86_extern_global_array_load_module() {
  auto module = make_extern_global_array_load_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  return module;
}

c4c::codegen::lir::LirModule make_x86_string_literal_char_module() {
  auto module = make_string_literal_char_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  return module;
}

c4c::codegen::lir::LirModule make_global_char_pointer_diff_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.globals.push_back(LirGlobal{
      LirGlobalId{0},
      "g_bytes",
      {},
      false,
      false,
      "",
      "global ",
      "[2 x i8]",
      "zeroinitializer",
      1,
      false,
  });

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "[2 x i8]", "@g_bytes", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(
      LirCastOp{"%t1", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t2", "i8", "%t0", false, {"i64 %t1"}});
  entry.insts.push_back(
      LirGepOp{"%t3", "[2 x i8]", "@g_bytes", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(
      LirCastOp{"%t4", LirCastKind::SExt, "i32", "0", "i64"});
  entry.insts.push_back(LirGepOp{"%t5", "i8", "%t3", false, {"i64 %t4"}});
  entry.insts.push_back(
      LirCastOp{"%t6", LirCastKind::PtrToInt, "ptr", "%t2", "i64"});
  entry.insts.push_back(
      LirCastOp{"%t7", LirCastKind::PtrToInt, "ptr", "%t5", "i64"});
  entry.insts.push_back(LirBinOp{"%t8", "sub", "i64", "%t6", "%t7"});
  entry.insts.push_back(
      LirCastOp{"%t9", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirCmpOp{"%t10", false, "eq", "i64", "%t8", "%t9"});
  entry.insts.push_back(
      LirCastOp{"%t11", LirCastKind::ZExt, "i1", "%t10", "i32"});
  entry.terminator = LirRet{std::string("%t11"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_x86_global_char_pointer_diff_module() {
  auto module = make_global_char_pointer_diff_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  return module;
}

c4c::codegen::lir::LirModule make_global_int_pointer_diff_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.globals.push_back(LirGlobal{
      LirGlobalId{0},
      "g_words",
      {},
      false,
      false,
      "",
      "global ",
      "[2 x i32]",
      "zeroinitializer",
      4,
      false,
  });

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "[2 x i32]", "@g_words", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(
      LirCastOp{"%t1", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t2", "i32", "%t0", false, {"i64 %t1"}});
  entry.insts.push_back(
      LirGepOp{"%t3", "[2 x i32]", "@g_words", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(
      LirCastOp{"%t4", LirCastKind::SExt, "i32", "0", "i64"});
  entry.insts.push_back(LirGepOp{"%t5", "i32", "%t3", false, {"i64 %t4"}});
  entry.insts.push_back(
      LirCastOp{"%t6", LirCastKind::PtrToInt, "ptr", "%t2", "i64"});
  entry.insts.push_back(
      LirCastOp{"%t7", LirCastKind::PtrToInt, "ptr", "%t5", "i64"});
  entry.insts.push_back(LirBinOp{"%t8", "sub", "i64", "%t6", "%t7"});
  entry.insts.push_back(LirBinOp{"%t9", "sdiv", "i64", "%t8", "4"});
  entry.insts.push_back(
      LirCastOp{"%t10", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirCmpOp{"%t11", false, "eq", "i64", "%t9", "%t10"});
  entry.insts.push_back(
      LirCastOp{"%t12", LirCastKind::ZExt, "i1", "%t11", "i32"});
  entry.terminator = LirRet{std::string("%t12"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_x86_global_int_pointer_diff_module() {
  auto module = make_global_int_pointer_diff_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  return module;
}

c4c::codegen::lir::LirModule make_typed_global_char_pointer_diff_module() {
  using namespace c4c::codegen::lir;

  auto module = make_global_char_pointer_diff_module();
  auto& entry = module.functions.front().blocks.front();
  std::get<LirBinOp>(entry.insts[8]).opcode = LirBinaryOpcode::Sub;
  std::get<LirCmpOp>(entry.insts[10]).predicate = LirCmpPredicate::Eq;
  return module;
}

c4c::codegen::lir::LirModule make_typed_x86_global_char_pointer_diff_module() {
  auto module = make_typed_global_char_pointer_diff_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  return module;
}

c4c::codegen::lir::LirModule make_typed_global_int_pointer_diff_module() {
  using namespace c4c::codegen::lir;

  auto module = make_global_int_pointer_diff_module();
  auto& entry = module.functions.front().blocks.front();
  std::get<LirBinOp>(entry.insts[8]).opcode = LirBinaryOpcode::Sub;
  std::get<LirBinOp>(entry.insts[9]).opcode = LirBinaryOpcode::SDiv;
  std::get<LirCmpOp>(entry.insts[11]).predicate = LirCmpPredicate::Eq;
  return module;
}

c4c::codegen::lir::LirModule make_typed_x86_global_int_pointer_diff_module() {
  auto module = make_typed_global_int_pointer_diff_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  return module;
}

c4c::codegen::lir::LirModule make_global_int_pointer_roundtrip_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.globals.push_back(LirGlobal{
      LirGlobalId{0},
      "g_value",
      {},
      false,
      false,
      "",
      "global ",
      "i32",
      "9",
      4,
      false,
  });

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.addr", "i64", "", 8});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.p", "ptr", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirCastOp{"%t0", LirCastKind::PtrToInt, "ptr", "@g_value", "i64"});
  entry.insts.push_back(LirStoreOp{"i64", "%t0", "%lv.addr"});
  entry.insts.push_back(LirLoadOp{"%t1", "i64", "%lv.addr"});
  entry.insts.push_back(
      LirCastOp{"%t2", LirCastKind::IntToPtr, "i64", "%t1", "ptr"});
  entry.insts.push_back(LirStoreOp{"ptr", "%t2", "%lv.p"});
  entry.insts.push_back(LirLoadOp{"%t3", "ptr", "%lv.p"});
  entry.insts.push_back(LirLoadOp{"%t4", "i32", "%t3"});
  entry.terminator = LirRet{std::string("%t4"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_x86_global_int_pointer_roundtrip_module() {
  auto module = make_global_int_pointer_roundtrip_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  return module;
}

c4c::codegen::lir::LirModule make_bitcast_scalar_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i64 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirCastOp{"%t0", LirCastKind::Bitcast, "double", "0.000000e+00", "i64"});
  entry.terminator = LirRet{std::string("%t0"), "i64"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_trunc_scalar_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i16 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirCastOp{"%t0", LirCastKind::Trunc, "i32", "13124", "i16"});
  entry.terminator = LirRet{std::string("%t0"), "i16"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_va_intrinsic_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");
  module.need_va_start = true;
  module.need_va_end = true;
  module.need_va_copy = true;

  LirFunction function;
  function.name = "variadic_probe";
  function.signature_text = "define void @variadic_probe(i32 %p.count, ...)\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(
      LirAllocaOp{"%lv.ap", "%struct.__va_list_tag_", "", 8});
  function.alloca_insts.push_back(
      LirAllocaOp{"%lv.ap_copy", "%struct.__va_list_tag_", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirVaStartOp{"%lv.ap"});
  entry.insts.push_back(LirVaCopyOp{"%lv.ap_copy", "%lv.ap"});
  entry.insts.push_back(LirVaEndOp{"%lv.ap_copy"});
  entry.insts.push_back(LirVaEndOp{"%lv.ap"});
  entry.terminator = LirRet{};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_va_arg_scalar_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");
  module.need_va_start = true;
  module.need_va_end = true;

  LirFunction function;
  function.name = "sum2";
  function.signature_text = "define i32 @sum2(i32 %p.first, ...)\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(
      LirAllocaOp{"%lv.ap", "%struct.__va_list_tag_", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirVaStartOp{"%lv.ap"});
  entry.insts.push_back(LirVaArgOp{"%t0", "%lv.ap", "i32"});
  entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%p.first", "%t0"});
  entry.insts.push_back(LirVaEndOp{"%lv.ap"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_va_arg_pair_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");
  module.type_decls.push_back("%struct.Pair = type { i32, i32 }");
  module.need_va_start = true;
  module.need_va_end = true;
  module.need_memcpy = true;

  LirFunction function;
  function.name = "pair_sum";
  function.signature_text = "define i32 @pair_sum(i32 %p.seed, ...)\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(
      LirAllocaOp{"%lv.ap", "%struct.__va_list_tag_", "", 8});
  function.alloca_insts.push_back(
      LirAllocaOp{"%lv.pair.tmp", "%struct.Pair", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirVaStartOp{"%lv.ap"});
  entry.insts.push_back(LirCmpOp{"%t0", false, "slt", "i32", "-8", "0"});
  entry.terminator = LirCondBr{"%t0", "reg", "stack"};

  LirBlock reg_block;
  reg_block.id = LirBlockId{1};
  reg_block.label = "reg";
  reg_block.insts.push_back(
      LirGepOp{"%t1", "%struct.__va_list_tag_", "%lv.ap", false, {"i32 0", "i32 1"}});
  reg_block.insts.push_back(LirLoadOp{"%t2", "ptr", "%t1"});
  reg_block.insts.push_back(LirGepOp{"%t3", "i8", "%t2", false, {"i32 -8"}});
  reg_block.terminator = LirBr{"join"};

  LirBlock stack_block;
  stack_block.id = LirBlockId{2};
  stack_block.label = "stack";
  stack_block.insts.push_back(
      LirGepOp{"%t4", "%struct.__va_list_tag_", "%lv.ap", false, {"i32 0", "i32 0"}});
  stack_block.insts.push_back(LirLoadOp{"%t5", "ptr", "%t4"});
  stack_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(
      LirPhiOp{"%t6", "ptr", {{"%t3", "reg"}, {"%t5", "stack"}}});
  join_block.insts.push_back(LirMemcpyOp{"%lv.pair.tmp", "%t6", "8", false});
  join_block.insts.push_back(LirVaEndOp{"%lv.ap"});
  join_block.terminator = LirRet{std::string("%p.seed"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(reg_block));
  function.blocks.push_back(std::move(stack_block));
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_va_arg_bigints_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");
  module.type_decls.push_back("%struct.BigInts = type { i32, i32, i32, i32, i32 }");
  module.need_va_start = true;
  module.need_va_end = true;
  module.need_memcpy = true;

  LirFunction function;
  function.name = "bigints_sum";
  function.signature_text = "define i32 @bigints_sum(i32 %p.seed, ...)\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(
      LirAllocaOp{"%lv.ap", "%struct.__va_list_tag_", "", 8});
  function.alloca_insts.push_back(
      LirAllocaOp{"%lv.bigints.tmp", "%struct.BigInts", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirVaStartOp{"%lv.ap"});
  entry.insts.push_back(
      LirGepOp{"%t0", "%struct.__va_list_tag_", "%lv.ap", false, {"i32 0", "i32 3"}});
  entry.insts.push_back(LirLoadOp{"%t1", "i32", "%t0"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "sge", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "stack", "regtry"};

  LirBlock reg_try_block;
  reg_try_block.id = LirBlockId{1};
  reg_try_block.label = "regtry";
  reg_try_block.insts.push_back(LirBinOp{"%t3", "add", "i32", "%t1", "8"});
  reg_try_block.insts.push_back(LirStoreOp{"i32", "%t3", "%t0"});
  reg_try_block.insts.push_back(LirCmpOp{"%t4", false, "sle", "i32", "%t3", "0"});
  reg_try_block.terminator = LirCondBr{"%t4", "reg", "stack"};

  LirBlock reg_block;
  reg_block.id = LirBlockId{2};
  reg_block.label = "reg";
  reg_block.insts.push_back(
      LirGepOp{"%t5", "%struct.__va_list_tag_", "%lv.ap", false, {"i32 0", "i32 1"}});
  reg_block.insts.push_back(LirLoadOp{"%t6", "ptr", "%t5"});
  reg_block.insts.push_back(LirGepOp{"%t7", "i8", "%t6", false, {"i32 %t1"}});
  reg_block.terminator = LirBr{"join"};

  LirBlock stack_block;
  stack_block.id = LirBlockId{3};
  stack_block.label = "stack";
  stack_block.insts.push_back(
      LirGepOp{"%t8", "%struct.__va_list_tag_", "%lv.ap", false, {"i32 0", "i32 0"}});
  stack_block.insts.push_back(LirLoadOp{"%t9", "ptr", "%t8"});
  stack_block.insts.push_back(LirGepOp{"%t10", "i8", "%t9", false, {"i64 8"}});
  stack_block.insts.push_back(LirStoreOp{"ptr", "%t10", "%t8"});
  stack_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{4};
  join_block.label = "join";
  join_block.insts.push_back(
      LirPhiOp{"%t11", "ptr", {{"%t7", "reg"}, {"%t9", "stack"}}});
  join_block.insts.push_back(LirLoadOp{"%t12", "ptr", "%t11"});
  join_block.insts.push_back(LirMemcpyOp{"%lv.bigints.tmp", "%t12", "20", false});
  join_block.insts.push_back(LirVaEndOp{"%lv.ap"});
  join_block.terminator = LirRet{std::string("%p.seed"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(reg_try_block));
  function.blocks.push_back(std::move(reg_block));
  function.blocks.push_back(std::move(stack_block));
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_phi_join_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "slt", "i32", "1", "2"});
  entry.terminator = LirCondBr{"%t0", "reg", "stack"};

  LirBlock reg_block;
  reg_block.id = LirBlockId{1};
  reg_block.label = "reg";
  reg_block.terminator = LirBr{"join"};

  LirBlock stack_block;
  stack_block.id = LirBlockId{2};
  stack_block.label = "stack";
  stack_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(LirPhiOp{"%t1", "ptr", {{"%reg.addr", "reg"}, {"%stack.addr", "stack"}}});
  join_block.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(reg_block));
  function.blocks.push_back(std::move(stack_block));
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_dead_param_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction decl;
  decl.name = "helper";
  decl.signature_text = "declare i32 @helper(i32)\n";
  decl.is_declaration = true;

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main(i32 %p.x)\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.param.x", "i32", "", 4});
  function.alloca_insts.push_back(LirStoreOp{"i32", "%p.x", "%lv.param.x"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@helper", "", "i32 %p.x"});
  entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%p.x", "%t0"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(decl));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_live_param_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main(i32 %p.x)\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.param.x", "i32", "", 4});
  function.alloca_insts.push_back(LirStoreOp{"i32", "%p.x", "%lv.param.x"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.param.x"});
  entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%t0", "1"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_dead_local_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.buf", "[2 x i32]", "", 4});
  function.alloca_insts.push_back(
      LirStoreOp{"[2 x i32]", "zeroinitializer", "%lv.buf"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.terminator = LirRet{std::string("0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_live_local_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.buf", "[2 x i32]", "", 4});
  function.alloca_insts.push_back(
      LirStoreOp{"[2 x i32]", "zeroinitializer", "%lv.buf"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "[2 x i32]", "%lv.buf", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirStoreOp{"i32", "7", "%t0"});
  entry.insts.push_back(LirLoadOp{"%t1", "i32", "%t0"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_disjoint_block_local_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main(i1 %p.cond)\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.then", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.else", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.terminator = LirCondBr{"%p.cond", "then", "else"};
  function.blocks.push_back(std::move(entry));

  LirBlock then_block;
  then_block.id = LirBlockId{1};
  then_block.label = "then";
  then_block.insts.push_back(LirStoreOp{"i32", "7", "%lv.then"});
  then_block.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.then"});
  then_block.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(then_block));

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.insts.push_back(LirStoreOp{"i32", "9", "%lv.else"});
  else_block.insts.push_back(LirLoadOp{"%t1", "i32", "%lv.else"});
  else_block.terminator = LirRet{std::string("%t1"), "i32"};
  function.blocks.push_back(std::move(else_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_same_block_local_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  entry.insts.push_back(LirStoreOp{"i32", "6", "%lv.y"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t1", "i32", "%lv.y"});
  entry.insts.push_back(LirBinOp{"%t2", "add", "i32", "%t0", "%t1"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_disjoint_block_call_arg_alloca_candidate_module() {
  auto module = make_disjoint_block_local_alloca_candidate_module();
  auto& else_block = module.functions.front().blocks[2];
  else_block.insts.insert(
      else_block.insts.begin() + 1,
      c4c::codegen::lir::LirCallOp{"", "void", "@sink", "(ptr)", "  ptr   %lv.else  "});
  return module;
}

c4c::codegen::lir::LirModule make_read_before_store_local_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.buf", "[2 x i32]", "", 4});
  function.alloca_insts.push_back(
      LirStoreOp{"[2 x i32]", "zeroinitializer", "%lv.buf"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "[2 x i32]", "%lv.buf", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirLoadOp{"%t1", "i32", "%t0"});
  entry.insts.push_back(LirStoreOp{"i32", "7", "%t0"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_overwrite_first_scalar_local_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  function.alloca_insts.push_back(LirStoreOp{"i32", "zeroinitializer", "%lv.x"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.x"});
  entry.terminator = LirBr{"exit"};
  function.blocks.push_back(std::move(entry));

  LirBlock exit;
  exit.id = LirBlockId{1};
  exit.label = "exit";
  exit.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  exit.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(exit));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_read_before_store_scalar_local_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  function.alloca_insts.push_back(LirStoreOp{"i32", "zeroinitializer", "%lv.x"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.x"});
  entry.terminator = LirBr{"exit"};
  function.blocks.push_back(std::move(entry));

  LirBlock exit;
  exit.id = LirBlockId{1};
  exit.label = "exit";
  exit.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(exit));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_escaped_local_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction decl;
  decl.name = "sink";
  decl.signature_text = "declare void @sink(ptr)\n";
  decl.is_declaration = true;

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.buf", "[2 x i32]", "", 4});
  function.alloca_insts.push_back(
      LirStoreOp{"[2 x i32]", "zeroinitializer", "%lv.buf"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"", "void", "@sink", "", "ptr %lv.buf"});
  entry.terminator = LirRet{std::string("0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(decl));
  module.functions.push_back(std::move(function));
  return module;
}

void test_adapts_direct_return() {
  const auto adapted = c4c::backend::adapt_minimal_module(make_return_zero_module());
  expect_true(adapted.functions.size() == 1, "adapter should preserve one function");
  expect_true(adapted.functions.front().blocks.size() == 1,
              "adapter should preserve one block");
  expect_true(!adapted.functions.front().blocks.front().terminator.value->compare("0"),
              "adapter should preserve the return literal");
}

void test_renders_return_add() {
  const auto adapted = c4c::backend::adapt_minimal_module(make_return_add_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "%t0 = add i32 2, 3",
                  "adapter renderer should emit the add instruction");
  expect_contains(rendered, "ret i32 %t0",
                  "adapter renderer should emit the adapted return");
}

void test_adapter_normalizes_typed_direct_call_helper_slice() {
  const auto adapted = c4c::backend::adapt_minimal_module(make_typed_direct_call_module());
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
  const auto adapted = c4c::backend::adapt_minimal_module(make_local_temp_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @main()",
                  "adapter should preserve the local-temp function signature");
  expect_contains(rendered, "ret i32 5",
                  "adapter should collapse the single-slot local-temp pattern into a direct return");
}

void test_adapter_normalizes_local_temp_sub_return_slice() {
  const auto adapted = c4c::backend::adapt_minimal_module(make_local_temp_sub_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "%t1 = sub i32 5, 4",
                  "adapter should rewrite the local-slot subtraction slice into a direct subtraction");
  expect_contains(rendered, "ret i32 %t1",
                  "adapter should return the normalized subtraction result directly");
}

void test_adapter_normalizes_two_local_temp_return_slice() {
  const auto adapted =
      c4c::backend::adapt_minimal_module(make_two_local_temp_return_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @main()",
                  "adapter should preserve the two-local function signature");
  expect_contains(rendered, "ret i32 0",
                  "adapter should collapse the selected two-local scalar slot pattern into a direct return");
}

void test_adapter_normalizes_local_pointer_temp_return_slice() {
  const auto adapted =
      c4c::backend::adapt_minimal_module(make_local_pointer_temp_return_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @main()",
                  "adapter should preserve the local-pointer round-trip function signature");
  expect_contains(rendered, "ret i32 0",
                  "adapter should collapse the bounded local pointer round-trip into a direct return");
}

void test_adapter_normalizes_double_indirect_local_pointer_conditional_return_slice() {
  const auto adapted = c4c::backend::adapt_minimal_module(
      make_double_indirect_local_pointer_conditional_return_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @main()",
                  "adapter should preserve the double-indirect local-pointer function signature");
  expect_contains(rendered, "ret i32 0",
                  "adapter should collapse the selected double-indirect local-pointer conditional chain into a direct return");
}

void test_adapter_normalizes_goto_only_constant_return_slice() {
  const auto adapted =
      c4c::backend::adapt_minimal_module(make_goto_only_constant_return_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @main()",
                  "adapter should preserve the goto-only function signature");
  expect_contains(rendered, "ret i32 0",
                  "adapter should collapse the selected goto-only branch chain into a direct return");
}

void test_adapter_normalizes_countdown_while_return_slice() {
  const auto adapted =
      c4c::backend::adapt_minimal_module(make_countdown_while_return_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @main()",
                  "adapter should preserve the while-countdown function signature");
  expect_contains(rendered, "ret i32 0",
                  "adapter should collapse the bounded while-countdown loop into a direct return");
}

void test_adapter_normalizes_typed_countdown_while_return_slice() {
  const auto adapted =
      c4c::backend::adapt_minimal_module(make_typed_countdown_while_return_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @main()",
                  "adapter should preserve the typed while-countdown function signature");
  expect_contains(rendered, "ret i32 0",
                  "adapter should collapse typed cmp/sub while-countdown loops into a direct return");
}

void test_adapter_normalizes_countdown_do_while_return_slice() {
  const auto adapted =
      c4c::backend::adapt_minimal_module(make_countdown_do_while_return_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @main()",
                  "adapter should preserve the do-while countdown function signature");
  expect_contains(rendered, "ret i32 0",
                  "adapter should collapse the bounded do-while countdown loop into a direct return");
}

void test_adapter_preserves_typed_two_arg_direct_call_helper_slice() {
  const auto adapted =
      c4c::backend::adapt_minimal_module(make_typed_direct_call_two_arg_module());
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
      c4c::backend::adapt_minimal_module(make_typed_direct_call_local_arg_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_one(i32 %p.x)",
                  "adapter should preserve the local-argument helper signature");
  expect_contains(rendered, "%t1 = add i32 %p.x, 1",
                  "adapter should still normalize the helper slot pattern");
  expect_contains(rendered, "call i32 (i32) @add_one(i32 5)",
                  "adapter should normalize the local slot direct-call argument into the backend slice");
}

void test_adapter_normalizes_typed_direct_call_local_arg_spacing_slice() {
  const auto adapted = c4c::backend::adapt_minimal_module(
      make_typed_direct_call_local_arg_with_spacing_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "call i32 (i32) @add_one(i32 5)",
                  "adapter should normalize local slot direct-call arguments even when compatibility spacing varies");
}

void test_adapter_canonicalizes_backend_owned_direct_call_rendering() {
  auto module = make_typed_direct_call_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.front());
  call.callee_type_suffix = " ( i32 ) ";
  call.args_str = "  i32   5  ";

  const auto adapted = c4c::backend::adapt_minimal_module(module);
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "call i32 (i32) @add_one(i32 5)",
                  "adapter should render backend-owned direct calls from canonical structured metadata instead of preserving raw compatibility spacing");
}

void test_adapter_normalizes_typed_two_arg_direct_call_local_arg_slice() {
  const auto adapted =
      c4c::backend::adapt_minimal_module(make_typed_direct_call_two_arg_local_arg_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_pair(i32 %p.x, i32 %p.y)",
                  "adapter should preserve the two-argument local-argument helper signature");
  expect_contains(rendered, "%t0 = add i32 %p.x, %p.y",
                  "adapter should still preserve the two-argument helper add");
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize the local slot first argument into the backend-owned two-argument slice");
}

void test_adapter_normalizes_typed_two_arg_direct_call_local_arg_spacing_slice() {
  const auto adapted = c4c::backend::adapt_minimal_module(
      make_typed_direct_call_two_arg_local_arg_with_spacing_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize first-local two-argument direct-call arguments even when compatibility spacing varies");
}

void test_adapter_normalizes_typed_two_arg_direct_call_second_local_arg_slice() {
  const auto adapted = c4c::backend::adapt_minimal_module(
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
  const auto adapted = c4c::backend::adapt_minimal_module(
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
  const auto adapted = c4c::backend::adapt_minimal_module(
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
  const auto adapted = c4c::backend::adapt_minimal_module(
      make_typed_direct_call_two_arg_first_local_rewrite_with_spacing_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize rewritten local call arguments even when compatibility spacing varies");
}

void test_adapter_normalizes_typed_two_arg_direct_call_both_local_arg_slice() {
  const auto adapted =
      c4c::backend::adapt_minimal_module(make_typed_direct_call_two_arg_both_local_arg_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_pair(i32 %p.x, i32 %p.y)",
                  "adapter should preserve the two-argument both-local helper signature");
  expect_contains(rendered, "%t0 = add i32 %p.x, %p.y",
                  "adapter should still preserve the two-argument helper add");
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize both local slot arguments into the backend-owned two-argument slice");
}

void test_adapter_normalizes_typed_two_arg_direct_call_both_local_first_rewrite_slice() {
  const auto adapted = c4c::backend::adapt_minimal_module(
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
  const auto adapted = c4c::backend::adapt_minimal_module(
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
  const auto adapted = c4c::backend::adapt_minimal_module(
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
  const auto adapted = c4c::backend::adapt_minimal_module(make_return_zero_module());
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
  const auto adapted = c4c::backend::adapt_minimal_module(make_return_add_module());
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
  const auto adapted = c4c::backend::adapt_minimal_module(make_typed_return_add_module());
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
  const auto adapted = c4c::backend::adapt_minimal_module(make_return_sub_module());
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
  const auto adapted = c4c::backend::adapt_minimal_module(make_local_temp_arithmetic_chain_module());
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

void test_rejects_unsupported_instruction() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(LirLoadOp{"%t0", "i32", "%ptr"});
  block.terminator = LirRet{std::string("%t0"), "i32"};

  try {
    (void)c4c::backend::adapt_minimal_module(module);
    fail("adapter should reject unsupported instructions");
  } catch (const c4c::backend::LirAdapterError& ex) {
    expect_contains(ex.what(), "non-binary/non-call instructions",
                    "adapter should explain unsupported instructions");
    expect_true(ex.kind() == c4c::backend::LirAdapterErrorKind::Unsupported,
                "adapter should classify unsupported instructions distinctly from malformed input");
  }
}

void test_aarch64_backend_scaffold_renders_supported_slice() {
  auto module = make_return_add_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 scaffold should emit a global entry symbol for the minimal asm slice");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 scaffold should materialize the folded return-add result in w0");
  expect_contains(rendered, "ret",
                  "aarch64 scaffold should terminate the minimal asm slice with ret");
}

void test_aarch64_backend_scaffold_renders_direct_return_immediate_slice() {
  auto module = make_return_zero_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 scaffold should emit a global entry symbol for direct return immediates");
  expect_contains(rendered, "mov w0, #0",
                  "aarch64 scaffold should materialize direct return immediates in w0");
  expect_contains(rendered, "ret",
                  "aarch64 scaffold should terminate direct return immediates with ret");
}

void test_x86_backend_scaffold_routes_through_explicit_emit_surface() {
  const auto module = make_return_add_module();
  const auto direct_rendered = c4c::backend::x86::emit_module(module);
  const auto backend_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});

  expect_true(backend_rendered == direct_rendered,
              "x86 backend selection should route through the explicit x86 emit seam");
  expect_contains(backend_rendered, ".text",
                  "x86 backend seam should emit assembly text for the active slice");
  expect_contains(backend_rendered, ".globl main",
                  "x86 backend seam should expose the global entry symbol");
  expect_contains(backend_rendered, "mov eax, 5",
                  "x86 backend seam should materialize the folded return-add result in eax");
  expect_contains(backend_rendered, "ret",
                  "x86 backend seam should terminate the minimal asm slice with ret");
  expect_not_contains(backend_rendered, "target triple =",
                      "x86 backend seam should stop falling back to LLVM text for the supported slice");
}

void test_x86_backend_scaffold_renders_direct_return_immediate_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_return_zero_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should emit a global entry symbol for direct return immediates");
  expect_contains(rendered, "mov eax, 0",
                  "x86 backend should materialize direct return immediates in eax");
  expect_contains(rendered, "ret",
                  "x86 backend should terminate direct return immediates with ret");
}

void test_x86_backend_scaffold_renders_direct_return_sub_immediate_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_return_sub_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should emit a global entry symbol for direct return subtraction slices");
  expect_contains(rendered, "mov eax, 0",
                  "x86 backend should fold the supported subtraction slice into an immediate return");
  expect_contains(rendered, "ret",
                  "x86 backend should terminate direct return subtraction slices with ret");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the supported subtraction slice");
}

void test_x86_backend_renders_local_temp_sub_slice() {
  auto module = make_local_temp_sub_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should emit a real entry symbol for the local-slot subtraction slice");
  expect_contains(rendered, "mov eax, 1",
                  "x86 backend should fold the normalized local-slot subtraction into the final immediate return");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the supported local-slot subtraction slice");
}

void test_x86_backend_renders_local_temp_arithmetic_chain_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_local_temp_arithmetic_chain_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should emit a real entry symbol for the bounded local-slot arithmetic slice");
  expect_contains(rendered, "mov eax, 0",
                  "x86 backend should fold the bounded mul-sdiv-srem-sub chain into the final immediate return");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the supported local-slot arithmetic slice");
}

void test_x86_backend_renders_two_local_temp_return_slice() {
  auto module = make_two_local_temp_return_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should emit a real entry symbol for the bounded two-local return slice");
  expect_contains(rendered, "mov eax, 0",
                  "x86 backend should fold the bounded two-local scalar slot slice into the final immediate return");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the supported two-local scalar slot slice");
}

void test_x86_backend_renders_local_pointer_temp_return_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_x86_local_pointer_temp_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should emit a real entry symbol for the bounded local pointer round-trip slice");
  expect_contains(rendered, "mov eax, 0",
                  "x86 backend should fold the normalized local pointer round-trip into the final immediate return");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the supported local pointer round-trip slice");
}

void test_x86_backend_renders_double_indirect_local_pointer_conditional_return_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_double_indirect_local_pointer_conditional_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should emit a real entry symbol for the bounded double-indirect local-pointer conditional slice");
  expect_contains(rendered, "mov eax, 0",
                  "x86 backend should fold the normalized double-indirect local-pointer conditional slice into the final immediate return");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the supported double-indirect local-pointer conditional slice");
}

void test_x86_backend_renders_goto_only_constant_return_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_goto_only_constant_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should emit a real entry symbol for the bounded goto-only slice");
  expect_contains(rendered, "mov eax, 0",
                  "x86 backend should fold the normalized goto-only branch chain into the final immediate return");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the supported goto-only slice");
}

void test_x86_backend_renders_countdown_while_return_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_countdown_while_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should emit a real entry symbol for the bounded while-countdown slice");
  expect_contains(rendered, "mov eax, 0",
                  "x86 backend should fold the normalized while-countdown loop into the final immediate return");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the supported while-countdown slice");
}

void test_x86_backend_renders_countdown_do_while_return_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_countdown_do_while_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should emit a real entry symbol for the bounded do-while countdown slice");
  expect_contains(rendered, "mov eax, 0",
                  "x86 backend should fold the normalized do-while countdown loop into the final immediate return");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the supported do-while countdown slice");
}

void test_x86_backend_renders_direct_call_slice() {
  auto module = make_direct_call_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".type helper, %function",
                  "x86 backend should lower the helper definition into a real function symbol");
  expect_contains(rendered, "helper:\n  mov eax, 7\n  ret\n",
                  "x86 backend should emit the minimal helper body as assembly");
  expect_contains(rendered, ".globl main",
                  "x86 backend should still publish main as the entry symbol");
  expect_contains(rendered, "call helper",
                  "x86 backend should keep the zero-arg helper call on the direct-call asm path");
  expect_contains(rendered, "ret\n",
                  "x86 backend should return directly after the helper call");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the direct helper-call slice");
}

void test_x86_backend_renders_zero_arg_typed_direct_call_slice_with_whitespace() {
  auto module = make_direct_call_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.front());
  call.callee_type_suffix = "( )";
  call.args_str = "  ";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, "call helper",
                  "x86 backend should keep zero-arg typed direct calls on the direct-call asm path even when compatibility whitespace remains");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should not fall back to LLVM text for whitespace-tolerant zero-arg typed direct calls");
}

void test_x86_backend_rejects_intrinsic_callee_from_direct_call_fast_path() {
  auto module = make_typed_direct_call_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.front());
  call.callee = c4c::codegen::lir::LirOperand(std::string("@llvm.abs.i32"),
                                              c4c::codegen::lir::LirOperandKind::Global);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, "target triple = \"x86_64-unknown-linux-gnu\"",
                  "x86 backend should fall back instead of treating llvm intrinsics as direct helper calls");
  expect_contains(rendered, "@llvm.abs.i32",
                  "x86 backend fallback should preserve the intrinsic callee text");
}

void test_x86_backend_rejects_indirect_callee_from_direct_call_fast_path() {
  auto module = make_typed_direct_call_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.front());
  call.callee = c4c::codegen::lir::LirOperand(std::string("%fp"),
                                              c4c::codegen::lir::LirOperandKind::SsaValue);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, "target triple = \"x86_64-unknown-linux-gnu\"",
                  "x86 backend should fall back instead of treating indirect callees as direct helper calls");
  expect_contains(rendered, "call i32 (i32) %fp(i32 5)",
                  "x86 backend fallback should preserve the indirect callee shape");
}

void test_x86_backend_renders_param_slot_slice() {
  auto module = make_param_slot_runtime_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".type main, %function",
                  "x86 backend should lower the parameter-slot main entry into assembly");
  expect_contains(rendered, ".type add_one, %function",
                  "x86 backend should lower the single-argument helper into assembly");
  expect_contains(rendered, "add_one:\n  mov eax, edi\n  add eax, 1\n  ret\n",
                  "x86 backend should lower the modified parameter slot helper through the integer argument register");
  expect_contains(rendered, "mov edi, 5",
                  "x86 backend should materialize the direct helper argument in the SysV integer argument register");
  expect_contains(rendered, "call add_one",
                  "x86 backend should keep the single-argument helper call on the asm path");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the parameter-slot slice");
}

void test_x86_backend_renders_typed_direct_call_local_arg_slice() {
  auto module = make_typed_direct_call_local_arg_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".type add_one, %function",
                  "x86 backend should lower the single-argument local-argument helper into a real function symbol");
  expect_contains(rendered, "add_one:\n  mov eax, edi\n  add eax, 1\n  ret\n",
                  "x86 backend should keep the normalized single-argument helper on the register add path");
  expect_contains(rendered, "mov edi, 5",
                  "x86 backend should materialize the normalized local argument in the first SysV integer register");
  expect_contains(rendered, "call add_one",
                  "x86 backend should lower the single-local direct call on the asm path");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the single-local direct-call slice");
}

void test_x86_backend_renders_typed_direct_call_local_arg_spacing_slice() {
  auto module = make_typed_direct_call_local_arg_with_suffix_spacing_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, "mov edi, 5",
                  "x86 backend should keep spacing-tolerant typed single-argument direct calls on the asm path");
  expect_contains(rendered, "call add_one",
                  "x86 backend should still lower spacing-tolerant typed single-argument calls directly");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should not fall back to LLVM text for spacing-tolerant typed single-argument calls");
}

void test_x86_backend_renders_typed_two_arg_direct_call_slice() {
  auto module = make_typed_direct_call_two_arg_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".type add_pair, %function",
                  "x86 backend should lower the two-argument helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  mov eax, edi\n  add eax, esi\n  ret\n",
                  "x86 backend should lower the register-only two-argument helper add");
  expect_contains(rendered, "mov edi, 5",
                  "x86 backend should materialize the first call argument in the first SysV integer register");
  expect_contains(rendered, "mov esi, 7",
                  "x86 backend should materialize the second call argument in the second SysV integer register");
  expect_contains(rendered, "call add_pair",
                  "x86 backend should lower the typed two-argument direct call on the asm path");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the two-argument direct-call slice");
}

void test_x86_backend_renders_typed_two_arg_direct_call_local_arg_slice() {
  auto module = make_typed_direct_call_two_arg_local_arg_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".type add_pair, %function",
                  "x86 backend should lower the two-argument local-argument helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  mov eax, edi\n  add eax, esi\n  ret\n",
                  "x86 backend should keep the local-argument helper on the register-only add path");
  expect_contains(rendered, "mov edi, 5",
                  "x86 backend should materialize the normalized local first argument in the first SysV integer register");
  expect_contains(rendered, "mov esi, 7",
                  "x86 backend should preserve the immediate second argument in the second SysV integer register");
  expect_contains(rendered, "call add_pair",
                  "x86 backend should lower the two-argument local-argument direct call on the asm path");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the two-argument local-argument slice");
}

void test_x86_backend_renders_typed_two_arg_direct_call_second_local_arg_slice() {
  auto module = make_typed_direct_call_two_arg_second_local_arg_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".type add_pair, %function",
                  "x86 backend should lower the two-argument second-local helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  mov eax, edi\n  add eax, esi\n  ret\n",
                  "x86 backend should keep the second-local helper on the register-only add path");
  expect_contains(rendered, "mov edi, 5",
                  "x86 backend should preserve the immediate first argument in the first SysV integer register");
  expect_contains(rendered, "mov esi, 7",
                  "x86 backend should materialize the normalized local second argument in the second SysV integer register");
  expect_contains(rendered, "call add_pair",
                  "x86 backend should lower the two-argument second-local direct call on the asm path");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the two-argument second-local slice");
}

void test_x86_backend_renders_typed_two_arg_direct_call_second_local_rewrite_slice() {
  auto module = make_typed_direct_call_two_arg_second_local_rewrite_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".type add_pair, %function",
                  "x86 backend should lower the rewritten second-local helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  mov eax, edi\n  add eax, esi\n  ret\n",
                  "x86 backend should keep the rewritten second-local helper on the register-only add path");
  expect_contains(rendered, "mov edi, 5",
                  "x86 backend should preserve the immediate first argument in the first SysV integer register for the rewritten second-local slice");
  expect_contains(rendered, "mov esi, 7",
                  "x86 backend should materialize the rewritten local second argument in the second SysV integer register");
  expect_contains(rendered, "call add_pair",
                  "x86 backend should lower the rewritten second-local direct call on the asm path");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the rewritten second-local slice");
}

void test_x86_backend_renders_typed_two_arg_direct_call_both_local_arg_slice() {
  auto module = make_typed_direct_call_two_arg_both_local_arg_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".type add_pair, %function",
                  "x86 backend should lower the both-local helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  mov eax, edi\n  add eax, esi\n  ret\n",
                  "x86 backend should keep the both-local helper on the register-only add path");
  expect_contains(rendered, "mov edi, 5",
                  "x86 backend should materialize the normalized first local in the first SysV integer register");
  expect_contains(rendered, "mov esi, 7",
                  "x86 backend should materialize the normalized second local in the second SysV integer register");
  expect_contains(rendered, "call add_pair",
                  "x86 backend should lower the both-local direct call on the asm path");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the both-local slice");
}

void test_x86_backend_renders_typed_two_arg_direct_call_both_local_first_rewrite_slice() {
  auto module = make_typed_direct_call_two_arg_both_local_first_rewrite_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".type add_pair, %function",
                  "x86 backend should lower the rewritten both-local helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  mov eax, edi\n  add eax, esi\n  ret\n",
                  "x86 backend should keep the rewritten both-local helper on the register-only add path");
  expect_contains(rendered, "mov edi, 5",
                  "x86 backend should materialize the rewritten first local in the first SysV integer register");
  expect_contains(rendered, "mov esi, 7",
                  "x86 backend should preserve the direct second local value in the second SysV integer register");
  expect_contains(rendered, "call add_pair",
                  "x86 backend should lower the rewritten first-local plus second-local direct call on the asm path");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the rewritten both-local first slice");
}

void test_x86_backend_renders_typed_two_arg_direct_call_both_local_second_rewrite_slice() {
  auto module = make_typed_direct_call_two_arg_both_local_second_rewrite_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".type add_pair, %function",
                  "x86 backend should lower the rewritten both-local helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  mov eax, edi\n  add eax, esi\n  ret\n",
                  "x86 backend should keep the rewritten both-local helper on the register-only add path");
  expect_contains(rendered, "mov edi, 5",
                  "x86 backend should preserve the direct first local value in the first SysV integer register");
  expect_contains(rendered, "mov esi, 7",
                  "x86 backend should materialize the rewritten second local in the second SysV integer register");
  expect_contains(rendered, "call add_pair",
                  "x86 backend should lower the first-local plus rewritten second-local direct call on the asm path");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the rewritten both-local second slice");
}

void test_x86_backend_renders_typed_two_arg_direct_call_both_local_double_rewrite_slice() {
  auto module = make_typed_direct_call_two_arg_both_local_double_rewrite_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".type add_pair, %function",
                  "x86 backend should lower the double-rewritten both-local helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  mov eax, edi\n  add eax, esi\n  ret\n",
                  "x86 backend should keep the double-rewritten both-local helper on the register-only add path");
  expect_contains(rendered, "mov edi, 5",
                  "x86 backend should materialize the rewritten first local in the first SysV integer register");
  expect_contains(rendered, "mov esi, 7",
                  "x86 backend should materialize the rewritten second local in the second SysV integer register");
  expect_contains(rendered, "call add_pair",
                  "x86 backend should lower the double-rewritten both-local direct call on the asm path");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the double-rewritten both-local slice");
}

void test_x86_backend_renders_typed_two_arg_direct_call_first_local_rewrite_spacing_slice() {
  auto module = make_typed_direct_call_two_arg_first_local_rewrite_with_suffix_spacing_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, "mov edi, 5",
                  "x86 backend should keep spacing-tolerant typed first-local rewrites on the asm path");
  expect_contains(rendered, "mov esi, 7",
                  "x86 backend should still recover the second typed argument through structured call parsing");
  expect_contains(rendered, "call add_pair",
                  "x86 backend should still lower spacing-tolerant typed two-argument calls directly");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should not fall back to LLVM text for spacing-tolerant typed two-argument calls");
}

void test_x86_backend_renders_local_array_slice() {
  auto module = make_local_array_gep_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should lower the bounded local-array slice to assembly");
  expect_contains(rendered, "sub rsp, 16",
                  "x86 backend should reserve one bounded local stack frame for the local-array slice");
  expect_contains(rendered, "lea rcx, [rbp - 8]",
                  "x86 backend should materialize the local stack-slot base explicitly");
  expect_contains(rendered, "mov dword ptr [rcx], 4",
                  "x86 backend should lower the first indexed local store through the backend-owned base address");
  expect_contains(rendered, "mov dword ptr [rcx + 4], 3",
                  "x86 backend should lower the second indexed local store through the backend-owned base address");
  expect_contains(rendered, "mov eax, dword ptr [rcx]",
                  "x86 backend should lower the first indexed local load through the same backend-owned base address");
  expect_contains(rendered, "add eax, dword ptr [rcx + 4]",
                  "x86 backend should fold the second indexed local load into the final add");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the bounded local-array slice");
}

void test_x86_backend_renders_global_load_slice() {
  auto module = make_global_load_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".data\n",
                  "x86 backend should place scalar global definitions in the data section");
  expect_contains(rendered, ".globl g_counter\n",
                  "x86 backend should publish the scalar global symbol");
  expect_contains(rendered, "g_counter:\n  .long 11\n",
                  "x86 backend should materialize the scalar global initializer");
  expect_contains(rendered, "lea rax, g_counter[rip]\n",
                  "x86 backend should form the scalar global address through RIP-relative addressing");
  expect_contains(rendered, "mov eax, dword ptr [rax]\n",
                  "x86 backend should load the scalar global value into eax");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the bounded global-load slice");
}

void test_x86_backend_uses_shared_regalloc_for_call_crossing_direct_call_slice() {
  auto module = make_typed_call_crossing_direct_call_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".type add_one, %function",
                  "x86 backend should lower the typed helper into a real function symbol");
  expect_contains(rendered, "mov qword ptr [rbp - 16], rbx",
                  "x86 backend should save the shared call-crossing callee-saved register in the prologue");
  expect_contains(rendered, "mov ebx, 5",
                  "x86 backend should materialize the call-crossing source value in the shared assigned register");
  expect_contains(rendered, "mov edi, ebx",
                  "x86 backend should pass the shared assigned register through the SysV integer argument register");
  expect_contains(rendered, "call add_one",
                  "x86 backend should keep the helper call on the direct-call asm path");
  expect_contains(rendered, "add eax, ebx",
                  "x86 backend should reuse the shared call-crossing source register after the call");
  expect_contains(rendered, "mov rbx, qword ptr [rbp - 16]",
                  "x86 backend should restore the shared callee-saved register in the epilogue");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the call-crossing direct-call slice");
}

void test_x86_backend_cleans_up_redundant_self_move_on_call_crossing_slice() {
  auto module = make_typed_call_crossing_direct_call_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_not_contains(rendered, "mov eax, eax",
                      "x86 backend should remove the redundant backend-owned self-move after the helper call");
  expect_contains(rendered, "add eax, ebx",
                  "x86 backend should still consume the helper result directly from eax after cleanup");
}

void test_x86_backend_keeps_spacing_tolerant_call_crossing_slice_on_asm_path() {
  auto module = make_typed_call_crossing_direct_call_with_spacing_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, "mov edi, ebx",
                  "x86 backend should still decode spacing-tolerant typed call-crossing arguments structurally");
  expect_contains(rendered, "call add_one",
                  "x86 backend should keep the spacing-tolerant call-crossing helper call on the asm path");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should not fall back to LLVM text for spacing-tolerant typed call-crossing slices");
}

void test_x86_backend_renders_compare_and_branch_slice() {
  auto module = make_conditional_return_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should lower the conditional-return slice to assembly");
  expect_contains(rendered, "  mov eax, 2\n",
                  "x86 backend should materialize the first compare immediate");
  expect_contains(rendered, "  cmp eax, 3\n",
                  "x86 backend should compare the materialized value against the second immediate");
  expect_contains(rendered, "  jge .Lelse\n",
                  "x86 backend should branch to the else label when the signed less-than test fails");
  expect_contains(rendered, ".Lthen:\n  mov eax, 0\n  ret\n",
                  "x86 backend should lower the then return block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov eax, 1\n  ret\n",
                  "x86 backend should lower the else return block directly in assembly");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the conditional-return slice");
}

void test_x86_backend_renders_compare_and_branch_slice_from_typed_predicates() {
  auto module = make_typed_conditional_return_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should lower the typed conditional-return slice to assembly");
  expect_contains(rendered, "  mov eax, 2\n",
                  "x86 backend should still materialize the typed compare lhs immediate");
  expect_contains(rendered, "  cmp eax, 3\n",
                  "x86 backend should still compare the typed predicate slice against the rhs immediate");
  expect_contains(rendered, "  jge .Lelse\n",
                  "x86 backend should map typed signed less-than predicates onto the same fail branch");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should keep typed compare-and-branch lowering on the asm path");
}

void test_x86_backend_renders_compare_and_branch_le_slice() {
  auto module = make_conditional_return_le_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should lower the signed less-or-equal conditional-return slice to assembly");
  expect_contains(rendered, "  mov eax, 2\n",
                  "x86 backend should materialize the first signed less-or-equal compare immediate");
  expect_contains(rendered, "  cmp eax, 3\n",
                  "x86 backend should compare the materialized less-or-equal lhs against the rhs immediate");
  expect_contains(rendered, "  jg .Lelse\n",
                  "x86 backend should branch to the else label when the signed less-or-equal test fails");
  expect_contains(rendered, ".Lthen:\n  mov eax, 0\n  ret\n",
                  "x86 backend should lower the signed less-or-equal then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov eax, 1\n  ret\n",
                  "x86 backend should lower the signed less-or-equal else block directly in assembly");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the signed less-or-equal slice");
}

void test_x86_backend_renders_compare_and_branch_gt_slice() {
  auto module = make_conditional_return_gt_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should lower the signed greater-than conditional-return slice to assembly");
  expect_contains(rendered, "  mov eax, 3\n",
                  "x86 backend should materialize the first signed greater-than compare immediate");
  expect_contains(rendered, "  cmp eax, 2\n",
                  "x86 backend should compare the materialized greater-than lhs against the rhs immediate");
  expect_contains(rendered, "  jle .Lelse\n",
                  "x86 backend should branch to the else label when the signed greater-than test fails");
  expect_contains(rendered, ".Lthen:\n  mov eax, 0\n  ret\n",
                  "x86 backend should lower the signed greater-than then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov eax, 1\n  ret\n",
                  "x86 backend should lower the signed greater-than else block directly in assembly");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the signed greater-than slice");
}

void test_x86_backend_renders_compare_and_branch_ge_slice() {
  auto module = make_conditional_return_ge_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should lower the signed greater-or-equal conditional-return slice to assembly");
  expect_contains(rendered, "  mov eax, 3\n",
                  "x86 backend should materialize the first signed greater-or-equal compare immediate");
  expect_contains(rendered, "  cmp eax, 2\n",
                  "x86 backend should compare the materialized greater-or-equal lhs against the rhs immediate");
  expect_contains(rendered, "  jl .Lelse\n",
                  "x86 backend should branch to the else label when the signed greater-or-equal test fails");
  expect_contains(rendered, ".Lthen:\n  mov eax, 0\n  ret\n",
                  "x86 backend should lower the signed greater-or-equal then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov eax, 1\n  ret\n",
                  "x86 backend should lower the signed greater-or-equal else block directly in assembly");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the signed greater-or-equal slice");
}

void test_x86_backend_renders_compare_and_branch_eq_slice() {
  auto module = make_conditional_return_eq_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should lower the equal conditional-return slice to assembly");
  expect_contains(rendered, "  mov eax, 2\n",
                  "x86 backend should materialize the first equal compare immediate");
  expect_contains(rendered, "  cmp eax, 2\n",
                  "x86 backend should compare the materialized equal lhs against the rhs immediate");
  expect_contains(rendered, "  jne .Lelse\n",
                  "x86 backend should branch to the else label when the equality test fails");
  expect_contains(rendered, ".Lthen:\n  mov eax, 0\n  ret\n",
                  "x86 backend should lower the equal then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov eax, 1\n  ret\n",
                  "x86 backend should lower the equal else block directly in assembly");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the equal slice");
}

void test_x86_backend_renders_compare_and_branch_ne_slice() {
  auto module = make_conditional_return_ne_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should lower the not-equal conditional-return slice to assembly");
  expect_contains(rendered, "  mov eax, 2\n",
                  "x86 backend should materialize the first not-equal compare immediate");
  expect_contains(rendered, "  cmp eax, 3\n",
                  "x86 backend should compare the materialized not-equal lhs against the rhs immediate");
  expect_contains(rendered, "  je .Lelse\n",
                  "x86 backend should branch to the else label when the not-equal test fails");
  expect_contains(rendered, ".Lthen:\n  mov eax, 0\n  ret\n",
                  "x86 backend should lower the not-equal then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov eax, 1\n  ret\n",
                  "x86 backend should lower the not-equal else block directly in assembly");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the not-equal slice");
}

void test_x86_backend_renders_compare_and_branch_ult_slice() {
  auto module = make_conditional_return_ult_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should lower the unsigned less-than conditional-return slice to assembly");
  expect_contains(rendered, "  mov eax, 2\n",
                  "x86 backend should materialize the first unsigned less-than compare immediate");
  expect_contains(rendered, "  cmp eax, 3\n",
                  "x86 backend should compare the materialized unsigned less-than lhs against the rhs immediate");
  expect_contains(rendered, "  jae .Lelse\n",
                  "x86 backend should branch to the else label when the unsigned less-than test fails");
  expect_contains(rendered, ".Lthen:\n  mov eax, 0\n  ret\n",
                  "x86 backend should lower the unsigned less-than then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov eax, 1\n  ret\n",
                  "x86 backend should lower the unsigned less-than else block directly in assembly");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the unsigned less-than slice");
}

void test_x86_backend_renders_compare_and_branch_ule_slice() {
  auto module = make_conditional_return_ule_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should lower the unsigned less-or-equal conditional-return slice to assembly");
  expect_contains(rendered, "  mov eax, 2\n",
                  "x86 backend should materialize the first unsigned less-or-equal compare immediate");
  expect_contains(rendered, "  cmp eax, 3\n",
                  "x86 backend should compare the materialized unsigned less-or-equal lhs against the rhs immediate");
  expect_contains(rendered, "  ja .Lelse\n",
                  "x86 backend should branch to the else label when the unsigned less-or-equal test fails");
  expect_contains(rendered, ".Lthen:\n  mov eax, 0\n  ret\n",
                  "x86 backend should lower the unsigned less-or-equal then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov eax, 1\n  ret\n",
                  "x86 backend should lower the unsigned less-or-equal else block directly in assembly");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the unsigned less-or-equal slice");
}

void test_x86_backend_renders_compare_and_branch_ugt_slice() {
  auto module = make_conditional_return_ugt_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should lower the unsigned greater-than conditional-return slice to assembly");
  expect_contains(rendered, "  mov eax, 3\n",
                  "x86 backend should materialize the first unsigned greater-than compare immediate");
  expect_contains(rendered, "  cmp eax, 2\n",
                  "x86 backend should compare the materialized unsigned greater-than lhs against the rhs immediate");
  expect_contains(rendered, "  jbe .Lelse\n",
                  "x86 backend should branch to the else label when the unsigned greater-than test fails");
  expect_contains(rendered, ".Lthen:\n  mov eax, 0\n  ret\n",
                  "x86 backend should lower the unsigned greater-than then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov eax, 1\n  ret\n",
                  "x86 backend should lower the unsigned greater-than else block directly in assembly");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the unsigned greater-than slice");
}

void test_x86_backend_renders_compare_and_branch_uge_slice() {
  auto module = make_conditional_return_uge_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".globl main",
                  "x86 backend should lower the unsigned greater-or-equal conditional-return slice to assembly");
  expect_contains(rendered, "  mov eax, 3\n",
                  "x86 backend should materialize the first unsigned greater-or-equal compare immediate");
  expect_contains(rendered, "  cmp eax, 2\n",
                  "x86 backend should compare the materialized unsigned greater-or-equal lhs against the rhs immediate");
  expect_contains(rendered, "  jb .Lelse\n",
                  "x86 backend should branch to the else label when the unsigned greater-or-equal test fails");
  expect_contains(rendered, ".Lthen:\n  mov eax, 0\n  ret\n",
                  "x86 backend should lower the unsigned greater-or-equal then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov eax, 1\n  ret\n",
                  "x86 backend should lower the unsigned greater-or-equal else block directly in assembly");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should stop falling back to LLVM text for the unsigned greater-or-equal slice");
}

void test_x86_backend_renders_extern_global_array_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_x86_extern_global_array_load_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".intel_syntax noprefix\n",
                  "x86 backend should lower the extern global array slice to assembly");
  expect_contains(rendered, ".globl main\n",
                  "x86 backend should still publish main as the entry symbol");
  expect_contains(rendered, "lea rax, ext_arr[rip]",
                  "x86 backend should materialize the extern global array base with RIP-relative addressing");
  expect_contains(rendered, "mov eax, dword ptr [rax + 4]",
                  "x86 backend should fold the bounded indexed load into the backend-owned base address");
  expect_contains(rendered, "ret\n",
                  "x86 backend should return the extern global array load result");
  expect_not_contains(rendered, "getelementptr",
                      "x86 backend should no longer fall back to LLVM text for the extern global array slice");
}

void test_x86_backend_renders_extern_decl_object_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_x86_extern_decl_object_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".intel_syntax noprefix\n",
                  "x86 backend should lower the bounded extern-decl call slice to assembly");
  expect_contains(rendered, ".globl main\n",
                  "x86 backend should still publish main as the entry symbol for the bounded extern-decl call slice");
  expect_contains(rendered, "call helper_ext\n",
                  "x86 backend should preserve the direct undefined helper call in the bounded object slice");
  expect_contains(rendered, "ret\n",
                  "x86 backend should return directly after the bounded undefined helper call");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should no longer fall back to LLVM text for the bounded extern-decl object slice");
}

void test_x86_backend_renders_extern_decl_object_slice_with_typed_zero_arg_spacing() {
  auto module = make_x86_extern_decl_object_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.front().blocks.front().insts.front());
  call.callee_type_suffix = " ( ) ";
  call.args_str = "  ";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, "call helper_ext\n",
                  "x86 backend should keep spacing-tolerant typed zero-arg extern calls on the direct-call asm path");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should not fall back for spacing-tolerant typed zero-arg extern calls");
}

void test_x86_backend_renders_string_literal_char_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_x86_string_literal_char_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".intel_syntax noprefix\n",
                  "x86 backend should lower the bounded string-literal char slice to assembly");
  expect_contains(rendered, ".section .rodata\n",
                  "x86 backend should place the promoted string literal into read-only data");
  expect_contains(rendered, ".L.str0:\n",
                  "x86 backend should emit a local string-pool label for the literal");
  expect_contains(rendered, "  .asciz \"hi\"\n",
                  "x86 backend should materialize the promoted string-literal bytes");
  expect_contains(rendered, ".text\n",
                  "x86 backend should resume emission in the text section for main");
  expect_contains(rendered, ".globl main\n",
                  "x86 backend should still publish main as the entry symbol");
  expect_contains(rendered, "lea rax, .L.str0[rip]\n",
                  "x86 backend should materialize the string-literal base with RIP-relative addressing");
  expect_contains(rendered, "movsx eax, byte ptr [rax + 1]\n",
                  "x86 backend should load and sign-extend the indexed string-literal byte");
  expect_contains(rendered, "ret\n",
                  "x86 backend should return the promoted string-literal result");
  expect_not_contains(rendered, "getelementptr",
                      "x86 backend should no longer fall back to LLVM text for the string-literal char slice");
}

void test_x86_backend_renders_global_char_pointer_diff_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_x86_global_char_pointer_diff_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".intel_syntax noprefix\n",
                  "x86 backend should lower the bounded global char pointer-difference slice to assembly");
  expect_contains(rendered, ".bss\n",
                  "x86 backend should place mutable zero-initialized byte arrays into BSS");
  expect_contains(rendered, ".globl g_bytes\n",
                  "x86 backend should publish the bounded byte-array symbol");
  expect_contains(rendered, "g_bytes:\n  .zero 2\n",
                  "x86 backend should materialize the bounded byte-array storage");
  expect_contains(rendered, ".text\n",
                  "x86 backend should resume emission in the text section for main");
  expect_contains(rendered, ".globl main\n",
                  "x86 backend should still publish main as the entry symbol");
  expect_contains(rendered, "lea rax, g_bytes[rip]\n",
                  "x86 backend should materialize the global byte-array base with RIP-relative addressing");
  expect_contains(rendered, "lea rcx, [rax + 1]\n",
                  "x86 backend should form the bounded byte-offset address from the global base");
  expect_contains(rendered, "sub rcx, rax\n",
                  "x86 backend should preserve the byte-granular pointer subtraction");
  expect_contains(rendered, "cmp rcx, 1\n",
                  "x86 backend should compare the pointer difference against one byte");
  expect_contains(rendered, "sete al\n",
                  "x86 backend should lower the bounded equality result into the low return register");
  expect_contains(rendered, "movzx eax, al\n",
                  "x86 backend should zero-extend the boolean result into the return register");
  expect_contains(rendered, "ret\n",
                  "x86 backend should return the bounded pointer-difference comparison result");
  expect_not_contains(rendered, "getelementptr",
                      "x86 backend should no longer fall back to LLVM text for the bounded global char pointer-difference slice");
}

void test_x86_backend_renders_global_char_pointer_diff_slice_from_typed_ops() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_typed_x86_global_char_pointer_diff_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, "lea rax, g_bytes[rip]\n",
                  "x86 backend should keep the typed byte-array pointer-difference slice on the asm path");
  expect_contains(rendered, "sub rcx, rax\n",
                  "x86 backend should decode typed subtraction wrappers in the byte pointer-difference slice");
  expect_contains(rendered, "cmp rcx, 1\n",
                  "x86 backend should still compare the typed byte pointer difference against one byte");
  expect_contains(rendered, "sete al\n",
                  "x86 backend should lower typed equality wrappers for the byte pointer-difference slice");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should not fall back to LLVM text for the typed byte pointer-difference slice");
}

void test_x86_backend_renders_global_int_pointer_diff_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_x86_global_int_pointer_diff_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".intel_syntax noprefix\n",
                  "x86 backend should lower the bounded global int pointer-difference slice to assembly");
  expect_contains(rendered, ".bss\n",
                  "x86 backend should place mutable zero-initialized int arrays into BSS");
  expect_contains(rendered, ".globl g_words\n",
                  "x86 backend should publish the bounded int-array symbol");
  expect_contains(rendered, "g_words:\n  .zero 8\n",
                  "x86 backend should materialize the bounded int-array storage");
  expect_contains(rendered, ".text\n",
                  "x86 backend should resume emission in the text section for main");
  expect_contains(rendered, ".globl main\n",
                  "x86 backend should still publish main as the entry symbol");
  expect_contains(rendered, "lea rax, g_words[rip]\n",
                  "x86 backend should materialize the global int-array base with RIP-relative addressing");
  expect_contains(rendered, "lea rcx, [rax + 4]\n",
                  "x86 backend should form the one-element int offset in bytes");
  expect_contains(rendered, "sub rcx, rax\n",
                  "x86 backend should preserve byte-granular pointer subtraction before scaling");
  expect_contains(rendered, "sar rcx, 2\n",
                  "x86 backend should lower the divide-by-four scaling step for the bounded int slice");
  expect_contains(rendered, "cmp rcx, 1\n",
                  "x86 backend should compare the scaled pointer difference against one element");
  expect_contains(rendered, "sete al\n",
                  "x86 backend should lower the bounded equality result into the low return register");
  expect_contains(rendered, "movzx eax, al\n",
                  "x86 backend should zero-extend the boolean result into the return register");
  expect_contains(rendered, "ret\n",
                  "x86 backend should return the bounded scaled pointer-difference comparison result");
  expect_not_contains(rendered, "getelementptr",
                      "x86 backend should no longer fall back to LLVM text for the bounded global int pointer-difference slice");
}

void test_x86_backend_renders_global_int_pointer_diff_slice_from_typed_ops() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_typed_x86_global_int_pointer_diff_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, "lea rax, g_words[rip]\n",
                  "x86 backend should keep the typed int-array pointer-difference slice on the asm path");
  expect_contains(rendered, "sub rcx, rax\n",
                  "x86 backend should decode typed subtraction wrappers before scaling int pointer differences");
  expect_contains(rendered, "sar rcx, 2\n",
                  "x86 backend should decode typed signed-divide wrappers for the scaled int pointer-difference slice");
  expect_contains(rendered, "cmp rcx, 1\n",
                  "x86 backend should still compare the typed scaled pointer difference against one element");
  expect_contains(rendered, "sete al\n",
                  "x86 backend should lower typed equality wrappers for the scaled int pointer-difference slice");
  expect_not_contains(rendered, "target triple =",
                      "x86 backend should not fall back to LLVM text for the typed int pointer-difference slice");
}

void test_x86_backend_renders_global_int_pointer_roundtrip_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_x86_global_int_pointer_roundtrip_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  expect_contains(rendered, ".intel_syntax noprefix\n",
                  "x86 backend should lower the bounded global int pointer round-trip slice to assembly");
  expect_contains(rendered, ".data\n",
                  "x86 backend should place the round-trip global definition in the data section");
  expect_contains(rendered, ".globl g_value\n",
                  "x86 backend should publish the round-trip global symbol");
  expect_contains(rendered, "g_value:\n  .long 9\n",
                  "x86 backend should materialize the round-trip global initializer");
  expect_contains(rendered, ".text\n",
                  "x86 backend should resume emission in the text section for main");
  expect_contains(rendered, ".globl main\n",
                  "x86 backend should still publish main as the entry symbol");
  expect_contains(rendered, "lea rax, g_value[rip]\n",
                  "x86 backend should materialize the bounded round-trip global address with RIP-relative addressing");
  expect_contains(rendered, "mov eax, dword ptr [rax]\n",
                  "x86 backend should lower the bounded round-trip back into a direct global load");
  expect_contains(rendered, "ret\n",
                  "x86 backend should return the bounded round-trip load result");
  expect_not_contains(rendered, "ptrtoint",
                      "x86 backend should no longer fall back to LLVM text ptrtoint for the bounded round-trip slice");
  expect_not_contains(rendered, "inttoptr",
                      "x86 backend should no longer fall back to LLVM text inttoptr for the bounded round-trip slice");
  expect_not_contains(rendered, "alloca",
                      "x86 backend should no longer fall back to LLVM text allocas for the bounded round-trip slice");
}

void test_aarch64_backend_renders_void_return_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_void_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "define void @helper()",
                  "aarch64 backend should preserve void signatures");
  expect_contains(rendered, "ret void",
                  "aarch64 backend should render void returns");
}

void test_aarch64_backend_preserves_module_headers_and_declarations() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_declaration_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "target datalayout = \"e-m:e-i64:64-i128:128-n32:64-S128\"",
                  "aarch64 backend should preserve the module datalayout");
  expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                  "aarch64 backend should preserve the module target triple");
  expect_contains(rendered, "%struct.Pair = type { i32, i32 }",
                  "aarch64 backend should preserve module type declarations");
  expect_contains(rendered, "declare i32 @helper(i32)\n",
                  "aarch64 backend should preserve declarations without bodies");
  expect_contains(rendered, "define i32 @main()\n{\nentry:\n",
                  "aarch64 backend should still open function bodies for definitions");
}

void test_aarch64_backend_propagates_malformed_signature_in_supported_slice() {
  auto module = make_return_add_module();
  module.functions.front().signature_text = "define @main()\n";

  try {
    (void)c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{module},
        c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
    fail("aarch64 backend should not hide malformed supported-slice signatures behind fallback");
  } catch (const c4c::backend::LirAdapterError& ex) {
    expect_true(ex.kind() == c4c::backend::LirAdapterErrorKind::Malformed,
                "aarch64 backend should preserve malformed-signature classification");
    expect_contains(ex.what(), "could not parse signature",
                    "aarch64 backend should surface the malformed signature detail");
  }
}

void test_aarch64_backend_renders_compare_and_branch_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #2\n",
                  "aarch64 backend should materialize the first compare immediate");
  expect_contains(rendered, "  cmp w8, #3\n",
                  "aarch64 backend should compare the materialized value against the second immediate");
  expect_contains(rendered, "  b.ge .Lelse\n",
                  "aarch64 backend should branch to the else label when the signed less-than test fails");
  expect_contains(rendered, ".Lthen:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the then return block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should lower the else return block directly in assembly");
}

void test_aarch64_backend_renders_compare_and_branch_slice_from_typed_predicates() {
  auto module = make_typed_conditional_return_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the typed conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #2\n",
                  "aarch64 backend should still materialize the typed compare lhs immediate");
  expect_contains(rendered, "  cmp w8, #3\n",
                  "aarch64 backend should still compare the typed predicate slice against the rhs immediate");
  expect_contains(rendered, "  b.ge .Lelse\n",
                  "aarch64 backend should map typed signed less-than predicates onto the same fail branch");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend should keep typed compare-and-branch lowering on the asm path");
}

void test_aarch64_backend_renders_compare_and_branch_le_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_le_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the signed less-or-equal conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #2\n",
                  "aarch64 backend should materialize the first signed less-or-equal compare immediate");
  expect_contains(rendered, "  cmp w8, #3\n",
                  "aarch64 backend should compare the materialized less-or-equal lhs against the rhs immediate");
  expect_contains(rendered, "  b.gt .Lelse\n",
                  "aarch64 backend should branch to the else label when the signed less-or-equal test fails");
  expect_contains(rendered, ".Lthen:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the signed less-or-equal then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should lower the signed less-or-equal else block directly in assembly");
}

void test_aarch64_backend_renders_compare_and_branch_gt_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_gt_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the signed greater-than conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #3\n",
                  "aarch64 backend should materialize the first signed greater-than compare immediate");
  expect_contains(rendered, "  cmp w8, #2\n",
                  "aarch64 backend should compare the materialized greater-than lhs against the rhs immediate");
  expect_contains(rendered, "  b.le .Lelse\n",
                  "aarch64 backend should branch to the else label when the signed greater-than test fails");
  expect_contains(rendered, ".Lthen:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the signed greater-than then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should lower the signed greater-than else block directly in assembly");
}

void test_aarch64_backend_renders_compare_and_branch_ge_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_ge_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the signed greater-or-equal conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #3\n",
                  "aarch64 backend should materialize the first signed greater-or-equal compare immediate");
  expect_contains(rendered, "  cmp w8, #2\n",
                  "aarch64 backend should compare the materialized greater-or-equal lhs against the rhs immediate");
  expect_contains(rendered, "  b.lt .Lelse\n",
                  "aarch64 backend should branch to the else label when the signed greater-or-equal test fails");
  expect_contains(rendered, ".Lthen:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the signed greater-or-equal then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should lower the signed greater-or-equal else block directly in assembly");
}

void test_aarch64_backend_renders_compare_and_branch_eq_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_eq_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the equal conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #2\n",
                  "aarch64 backend should materialize the first equal compare immediate");
  expect_contains(rendered, "  cmp w8, #2\n",
                  "aarch64 backend should compare the materialized equal lhs against the rhs immediate");
  expect_contains(rendered, "  b.ne .Lelse\n",
                  "aarch64 backend should branch to the else label when the equality test fails");
  expect_contains(rendered, ".Lthen:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the equal then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should lower the equal else block directly in assembly");
}

void test_aarch64_backend_renders_compare_and_branch_ne_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_ne_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the not-equal conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #2\n",
                  "aarch64 backend should materialize the first not-equal compare immediate");
  expect_contains(rendered, "  cmp w8, #3\n",
                  "aarch64 backend should compare the materialized not-equal lhs against the rhs immediate");
  expect_contains(rendered, "  b.eq .Lelse\n",
                  "aarch64 backend should branch to the else label when the not-equal test fails");
  expect_contains(rendered, ".Lthen:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the not-equal then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should lower the not-equal else block directly in assembly");
}

void test_aarch64_backend_renders_compare_and_branch_ult_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_ult_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the unsigned less-than conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #2\n",
                  "aarch64 backend should materialize the first unsigned less-than compare immediate");
  expect_contains(rendered, "  cmp w8, #3\n",
                  "aarch64 backend should compare the materialized unsigned less-than lhs against the rhs immediate");
  expect_contains(rendered, "  b.hs .Lelse\n",
                  "aarch64 backend should branch to the else label when the unsigned less-than test fails");
  expect_contains(rendered, ".Lthen:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the unsigned less-than then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should lower the unsigned less-than else block directly in assembly");
}

void test_aarch64_backend_renders_compare_and_branch_ule_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_ule_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the unsigned less-or-equal conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #2\n",
                  "aarch64 backend should materialize the first unsigned less-or-equal compare immediate");
  expect_contains(rendered, "  cmp w8, #3\n",
                  "aarch64 backend should compare the materialized unsigned less-or-equal lhs against the rhs immediate");
  expect_contains(rendered, "  b.hi .Lelse\n",
                  "aarch64 backend should branch to the else label when the unsigned less-or-equal test fails");
  expect_contains(rendered, ".Lthen:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the unsigned less-or-equal then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should lower the unsigned less-or-equal else block directly in assembly");
}

void test_aarch64_backend_renders_compare_and_branch_ugt_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_ugt_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the unsigned greater-than conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #3\n",
                  "aarch64 backend should materialize the first unsigned greater-than compare immediate");
  expect_contains(rendered, "  cmp w8, #2\n",
                  "aarch64 backend should compare the materialized unsigned greater-than lhs against the rhs immediate");
  expect_contains(rendered, "  b.ls .Lelse\n",
                  "aarch64 backend should branch to the else label when the unsigned greater-than test fails");
  expect_contains(rendered, ".Lthen:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the unsigned greater-than then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should lower the unsigned greater-than else block directly in assembly");
}

void test_aarch64_backend_renders_compare_and_branch_uge_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_uge_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the unsigned greater-or-equal conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #3\n",
                  "aarch64 backend should materialize the first unsigned greater-or-equal compare immediate");
  expect_contains(rendered, "  cmp w8, #2\n",
                  "aarch64 backend should compare the materialized unsigned greater-or-equal lhs against the rhs immediate");
  expect_contains(rendered, "  b.lo .Lelse\n",
                  "aarch64 backend should branch to the else label when the unsigned greater-or-equal test fails");
  expect_contains(rendered, ".Lthen:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the unsigned greater-or-equal then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should lower the unsigned greater-or-equal else block directly in assembly");
}

void test_aarch64_backend_scaffold_rejects_out_of_slice_ir() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(LirSelectOp{"%t0", "i32", "true", "1", "0"});

  try {
    (void)c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{module},
        c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
    fail("aarch64 scaffold should reject IR outside the adapter slice");
  } catch (const std::invalid_argument& ex) {
    expect_contains(ex.what(), "aarch64 backend emitter does not support",
                    "aarch64 emitter should identify target-local support limits");
    expect_contains(ex.what(), "non-ALU/non-branch/non-call/non-memory instructions",
                    "aarch64 emitter should preserve the unsupported detail");
  }
}

void test_aarch64_backend_renders_direct_call_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_direct_call_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type helper, %function",
                  "aarch64 backend should lower the helper definition into a real function symbol");
  expect_contains(rendered, "helper:\n  mov w0, #7\n  ret\n",
                  "aarch64 backend should emit the minimal helper body as assembly");
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should still publish main as the entry symbol");
  expect_contains(rendered, "sub sp, sp, #16",
                  "aarch64 backend should preserve the link register before a helper call");
  expect_contains(rendered, "str x30, [sp, #8]",
                  "aarch64 backend should spill x30 in the minimal helper-call frame");
  expect_contains(rendered, "bl helper",
                  "aarch64 backend should lower the supported direct call slice with bl");
  expect_contains(rendered, "ldr x30, [sp, #8]",
                  "aarch64 backend should restore x30 after the helper call");
  expect_contains(rendered, "add sp, sp, #16",
                  "aarch64 backend should tear down the minimal helper-call frame");
}

void test_aarch64_backend_renders_zero_arg_typed_direct_call_slice_with_whitespace() {
  auto module = make_direct_call_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.front());
  call.callee_type_suffix = "( )";
  call.args_str = "  ";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "bl helper",
                  "aarch64 backend should keep zero-arg typed direct calls on the direct-call asm path even when compatibility whitespace remains");
  expect_not_contains(rendered, "define i32 @main()",
                      "aarch64 backend should not fall back to LLVM text for whitespace-tolerant zero-arg typed direct calls");
}

void test_aarch64_backend_rejects_intrinsic_callee_from_direct_call_fast_path() {
  auto module = make_typed_direct_call_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.front());
  call.callee = c4c::codegen::lir::LirOperand(std::string("@llvm.abs.i32"),
                                              c4c::codegen::lir::LirOperandKind::Global);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                  "aarch64 backend should fall back instead of treating llvm intrinsics as direct helper calls");
  expect_contains(rendered, "@llvm.abs.i32",
                  "aarch64 backend fallback should preserve the intrinsic callee text");
}

void test_aarch64_backend_rejects_indirect_callee_from_direct_call_fast_path() {
  auto module = make_typed_direct_call_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.front());
  call.callee = c4c::codegen::lir::LirOperand(std::string("%fp"),
                                              c4c::codegen::lir::LirOperandKind::SsaValue);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                  "aarch64 backend should fall back instead of treating indirect callees as direct helper calls");
  expect_contains(rendered, "call i32 (i32) %fp(i32 5)",
                  "aarch64 backend fallback should preserve the indirect callee shape");
}

void test_aarch64_backend_renders_local_temp_memory_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_local_temp_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the single-slot local-temp slice to assembly");
  expect_contains(rendered, "main:\n  mov w0, #5\n  ret\n",
                  "aarch64 backend should collapse the local-temp literal slot pattern into a direct return");
}

void test_aarch64_backend_renders_param_slot_memory_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_param_slot_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "define i32 @main(i32 %p.x)",
                  "aarch64 backend should preserve parameterized signatures");
  expect_contains(rendered, "%lv.param.x = alloca i32, align 4",
                  "aarch64 backend should render modified parameter slots");
  expect_contains(rendered, "store i32 %p.x, ptr %lv.param.x",
                  "aarch64 backend should spill modified parameters into entry slots");
  expect_contains(rendered, "%t0 = load i32, ptr %lv.param.x",
                  "aarch64 backend should reload parameter slots");
  expect_contains(rendered, "store i32 %t1, ptr %lv.param.x",
                  "aarch64 backend should write updated parameter values back to slots");
  expect_contains(rendered, "ret i32 %t2",
                  "aarch64 backend should preserve the final reloaded parameter value");
}

void test_aarch64_backend_renders_typed_direct_call_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_typed_direct_call_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_one, %function",
                  "aarch64 backend should lower the single-argument helper into a real function symbol");
  expect_contains(rendered, "add_one:\n  add w0, w0, #1\n  ret\n",
                  "aarch64 backend should lower the normalized helper add into register-based assembly");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should materialize the direct-call argument in w0 before bl");
  expect_contains(rendered, "bl add_one",
                  "aarch64 backend should lower the typed direct call with bl");
}

void test_aarch64_backend_uses_shared_regalloc_for_call_crossing_direct_call_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_typed_call_crossing_direct_call_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "sub sp, sp, #16",
                  "aarch64 backend should allocate a real frame for the surviving shared callee-saved call-crossing value");
  expect_contains(rendered, "str x20, [sp, #0]",
                  "aarch64 backend should save the first shared call-crossing callee-saved register");
  expect_contains(rendered, "str x30, [sp, #8]",
                  "aarch64 backend should still preserve lr in the shared call frame");
  expect_contains(rendered, "mov w20, #5",
                  "aarch64 backend should materialize the call-crossing source value in the shared assigned register");
  expect_contains(rendered, "mov w0, w20",
                  "aarch64 backend should pass the shared assigned register through the ABI argument register");
  expect_contains(rendered, "bl add_one",
                  "aarch64 backend should keep the helper call on the direct-call asm path");
  expect_contains(rendered, "add w0, w20, w0",
                  "aarch64 backend should keep reusing the shared call-crossing source register while consuming the helper result directly from w0");
  expect_contains(rendered, "ldr x20, [sp, #0]",
                  "aarch64 backend should restore the shared call-crossing source register");
}

void test_aarch64_backend_cleans_up_redundant_call_result_traffic_on_call_crossing_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_typed_call_crossing_direct_call_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "sub sp, sp, #16",
                  "aarch64 backend should shrink the shared call-crossing frame once the helper result stays in w0");
  expect_not_contains(rendered, "str x21, [sp, #8]",
                      "aarch64 backend should not save a redundant call-result callee-saved register");
  expect_not_contains(rendered, "mov w21, w0",
                      "aarch64 backend should not hand the helper result through a redundant callee-saved temp");
  expect_contains(rendered, "add w0, w20, w0",
                  "aarch64 backend should consume the helper result directly from the ABI return register");
}

void test_aarch64_backend_keeps_spacing_tolerant_call_crossing_slice_on_asm_path() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_typed_call_crossing_direct_call_with_spacing_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "mov w0, w20",
                  "aarch64 backend should still decode spacing-tolerant typed call-crossing arguments structurally");
  expect_contains(rendered, "bl add_one",
                  "aarch64 backend should keep spacing-tolerant typed call-crossing slices on the asm path");
  expect_contains(rendered, "add w0, w20, w0",
                  "aarch64 backend should still combine the decoded helper result with the preserved source register");
}

void test_aarch64_backend_renders_typed_two_arg_direct_call_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_typed_direct_call_two_arg_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend should lower the two-argument helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  add w0, w0, w1\n  ret\n",
                  "aarch64 backend should lower the register-only two-argument helper add");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should materialize the first call argument in w0 before bl");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend should materialize the second call argument in w1 before bl");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend should lower the typed two-argument direct call with bl");
}

void test_aarch64_backend_renders_typed_direct_call_local_arg_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_typed_direct_call_local_arg_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_one, %function",
                  "aarch64 backend should lower the local-argument helper into a real function symbol");
  expect_contains(rendered, "add_one:\n  add w0, w0, #1\n  ret\n",
                  "aarch64 backend should keep the helper on the register-based add path");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should materialize the local direct-call argument in w0 before bl");
  expect_contains(rendered, "bl add_one",
                  "aarch64 backend should lower the local-argument direct call with bl");
}

void test_aarch64_backend_renders_typed_direct_call_local_arg_spacing_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_typed_direct_call_local_arg_with_suffix_spacing_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should keep spacing-tolerant typed single-argument calls on the asm path");
  expect_contains(rendered, "bl add_one",
                  "aarch64 backend should still lower spacing-tolerant typed single-argument direct calls with bl");
}

void test_aarch64_backend_renders_typed_two_arg_direct_call_local_arg_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_typed_direct_call_two_arg_local_arg_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend should lower the two-argument local-argument helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  add w0, w0, w1\n  ret\n",
                  "aarch64 backend should keep the local-argument helper on the register-only add path");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should materialize the normalized local first argument in w0 before bl");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend should preserve the immediate second argument in w1 before bl");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend should lower the two-argument local-argument direct call with bl");
}

void test_aarch64_backend_renders_typed_two_arg_direct_call_second_local_arg_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_typed_direct_call_two_arg_second_local_arg_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend should lower the two-argument second-local helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  add w0, w0, w1\n  ret\n",
                  "aarch64 backend should keep the second-local helper on the register-only add path");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should preserve the immediate first argument in w0 before bl");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend should materialize the normalized local second argument in w1 before bl");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend should lower the two-argument second-local direct call with bl");
}

void test_aarch64_backend_renders_typed_two_arg_direct_call_second_local_rewrite_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_typed_direct_call_two_arg_second_local_rewrite_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend should lower the rewritten second-local helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  add w0, w0, w1\n  ret\n",
                  "aarch64 backend should keep the rewritten second-local helper on the register-only add path");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should preserve the immediate first argument in w0 before bl for the rewritten second-local slice");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend should materialize the rewritten second-local argument in w1 before bl");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend should lower the rewritten second-local direct call with bl");
}

void test_aarch64_backend_renders_typed_two_arg_direct_call_first_local_rewrite_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_typed_direct_call_two_arg_first_local_rewrite_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend should lower the rewritten first-local helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  add w0, w0, w1\n  ret\n",
                  "aarch64 backend should keep the rewritten first-local helper on the register-only add path");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should materialize the rewritten first-local argument in w0 before bl");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend should preserve the immediate second argument in w1 before bl for the rewritten first-local slice");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend should lower the rewritten first-local direct call with bl");
}

void test_aarch64_backend_renders_typed_two_arg_direct_call_first_local_rewrite_spacing_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_typed_direct_call_two_arg_first_local_rewrite_with_suffix_spacing_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should keep spacing-tolerant typed first-local rewrites on the asm path");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend should still recover the second typed argument through structured call parsing");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend should still lower spacing-tolerant typed two-argument direct calls with bl");
}

void test_aarch64_backend_renders_typed_two_arg_direct_call_both_local_arg_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_typed_direct_call_two_arg_both_local_arg_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend should lower the two-argument both-local helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  add w0, w0, w1\n  ret\n",
                  "aarch64 backend should keep the both-local helper on the register-only add path");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should materialize the normalized first local argument in w0 before bl");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend should materialize the normalized second local argument in w1 before bl");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend should lower the two-argument both-local direct call with bl");
}

void test_aarch64_backend_renders_typed_two_arg_direct_call_both_local_first_rewrite_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_typed_direct_call_two_arg_both_local_first_rewrite_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend should lower the rewritten both-local helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  add w0, w0, w1\n  ret\n",
                  "aarch64 backend should keep the rewritten both-local helper on the register-only add path");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should materialize the rewritten first local argument in w0 before bl");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend should materialize the preserved second local argument in w1 before bl");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend should lower the rewritten both-local direct call with bl");
}

void test_aarch64_backend_renders_typed_two_arg_direct_call_both_local_second_rewrite_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_typed_direct_call_two_arg_both_local_second_rewrite_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend should lower the rewritten both-local helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  add w0, w0, w1\n  ret\n",
                  "aarch64 backend should keep the rewritten both-local helper on the register-only add path");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should materialize the preserved first local argument in w0 before bl");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend should materialize the rewritten second local argument in w1 before bl");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend should lower the rewritten both-local second-slot direct call with bl");
}

void test_aarch64_backend_renders_typed_two_arg_direct_call_both_local_double_rewrite_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_typed_direct_call_two_arg_both_local_double_rewrite_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend should lower the double-rewritten both-local helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  add w0, w0, w1\n  ret\n",
                  "aarch64 backend should keep the double-rewritten both-local helper on the register-only add path");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should materialize the double-rewritten first local argument in w0 before bl");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend should materialize the double-rewritten second local argument in w1 before bl");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend should lower the double-rewritten both-local direct call with bl");
}

void test_aarch64_backend_renders_local_array_gep_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_local_array_gep_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should promote the bounded local-array slice onto the asm path");
  expect_contains(rendered, "sub sp, sp, #16",
                  "aarch64 backend should reserve one aligned stack frame for the local array slice");
  expect_contains(rendered, "add x8, sp, #8",
                  "aarch64 backend should materialize the local-array base address from the stack slot");
  expect_contains(rendered, "str w9, [x8]",
                  "aarch64 backend should store the first local-array element through the explicit base address");
  expect_contains(rendered, "str w9, [x8, #4]",
                  "aarch64 backend should store the second local-array element at the folded element offset");
  expect_contains(rendered, "ldr w9, [x8]",
                  "aarch64 backend should reload the first local-array element through the explicit base address");
  expect_contains(rendered, "ldr w10, [x8, #4]",
                  "aarch64 backend should reload the second local-array element at the folded element offset");
  expect_contains(rendered, "add w0, w9, w10",
                  "aarch64 backend should return the local-array element sum from registers");
  expect_contains(rendered, "add sp, sp, #16",
                  "aarch64 backend should restore the bounded local-array stack frame before returning");
  expect_not_contains(rendered, "getelementptr",
                      "aarch64 backend should not fall back to LLVM-text GEP rendering for the bounded local-array slice");
}

void test_aarch64_backend_renders_param_member_array_gep_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_param_member_array_gep_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "%struct.Pair = type { [2 x i32] }",
                  "aarch64 backend should preserve struct member array type declarations");
  expect_contains(rendered, "%lv.param.p = alloca %struct.Pair, align 4",
                  "aarch64 backend should spill by-value struct parameters into stack slots");
  expect_contains(rendered, "store %struct.Pair %p.p, ptr %lv.param.p",
                  "aarch64 backend should store by-value struct parameters into their slots");
  expect_contains(rendered, "%t0 = getelementptr %struct.Pair, ptr %lv.param.p, i32 0, i32 0",
                  "aarch64 backend should render the member-addressing GEP");
  expect_contains(rendered, "%t1 = getelementptr [2 x i32], ptr %t0, i64 0, i64 0",
                  "aarch64 backend should render array decay from struct members");
  expect_contains(rendered, "%t3 = getelementptr i32, ptr %t1, i64 %t2",
                  "aarch64 backend should render indexed member-array addressing");
  expect_contains(rendered, "ret i32 %t4",
                  "aarch64 backend should preserve the loaded member-array result");
}

void test_aarch64_backend_renders_nested_member_pointer_array_gep_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_nested_member_pointer_array_gep_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "%struct.Inner = type { [2 x i32] }",
                  "aarch64 backend should preserve nested pointee type declarations");
  expect_contains(rendered, "%struct.Outer = type { ptr }",
                  "aarch64 backend should preserve nested pointer-holder type declarations");
  expect_contains(rendered, "%t0 = getelementptr %struct.Outer, ptr %p.o, i32 0, i32 0",
                  "aarch64 backend should render outer member-addressing GEP");
  expect_contains(rendered, "%t1 = load ptr, ptr %t0",
                  "aarch64 backend should reload nested struct pointers from outer members");
  expect_contains(rendered, "%t2 = getelementptr %struct.Inner, ptr %t1, i32 0, i32 0",
                  "aarch64 backend should render nested pointee member-addressing GEP");
  expect_contains(rendered, "%t3 = getelementptr [2 x i32], ptr %t2, i64 0, i64 0",
                  "aarch64 backend should render array decay from nested pointee members");
  expect_contains(rendered, "%t5 = getelementptr i32, ptr %t3, i64 %t4",
                  "aarch64 backend should render indexed nested member-pointer addressing");
  expect_contains(rendered, "ret i32 %t6",
                  "aarch64 backend should preserve the loaded nested member-pointer result");
}

void test_aarch64_backend_renders_nested_param_member_array_gep_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_nested_param_member_array_gep_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "%struct.Inner = type { [2 x i32] }",
                  "aarch64 backend should preserve nested by-value member type declarations");
  expect_contains(rendered, "%struct.Outer = type { %struct.Inner }",
                  "aarch64 backend should preserve nested by-value outer type declarations");
  expect_contains(rendered, "%lv.param.o = alloca %struct.Outer, align 4",
                  "aarch64 backend should spill nested by-value parameters into stack slots");
  expect_contains(rendered, "store %struct.Outer %p.o, ptr %lv.param.o",
                  "aarch64 backend should store nested by-value parameters into their slots");
  expect_contains(rendered, "%t0 = getelementptr %struct.Outer, ptr %lv.param.o, i32 0, i32 0",
                  "aarch64 backend should render the outer nested by-value member-addressing GEP");
  expect_contains(rendered, "%t1 = getelementptr %struct.Inner, ptr %t0, i32 0, i32 0",
                  "aarch64 backend should render the inner nested by-value member-addressing GEP");
  expect_contains(rendered, "%t3 = getelementptr i32, ptr %t1, i64 %t2",
                  "aarch64 backend should render indexed nested by-value member-array addressing");
  expect_contains(rendered, "ret i32 %t4",
                  "aarch64 backend should preserve the loaded nested by-value member-array result");
}

void test_aarch64_backend_renders_global_definition_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_global_load_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".data\n",
                  "aarch64 backend should place scalar global definitions in the data section");
  expect_contains(rendered, ".globl g_counter\n",
                  "aarch64 backend should publish the scalar global symbol");
  expect_contains(rendered, "g_counter:\n  .long 11\n",
                  "aarch64 backend should materialize the scalar global initializer");
  expect_contains(rendered, ".globl main\n",
                  "aarch64 backend should still publish main as the entry symbol");
  expect_contains(rendered, "adrp x8, g_counter\n",
                  "aarch64 backend should form the scalar global page address");
  expect_contains(rendered, "ldr w0, [x8, :lo12:g_counter]\n",
                  "aarch64 backend should load the scalar global directly into the return register");
  expect_contains(rendered, "ret\n",
                  "aarch64 backend should return the loaded scalar global value");
}

void test_aarch64_backend_renders_string_pool_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_string_literal_char_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".section .rodata\n",
                  "aarch64 backend should place string literals into read-only data");
  expect_contains(rendered, ".L.str0:\n",
                  "aarch64 backend should emit a local string-pool label for the literal");
  expect_contains(rendered, "  .asciz \"hi\"\n",
                  "aarch64 backend should materialize the string-literal bytes");
  expect_contains(rendered, ".globl main\n",
                  "aarch64 backend should still publish main as the entry symbol");
  expect_contains(rendered, "  adrp x8, .L.str0\n",
                  "aarch64 backend should form the string-literal page address");
  expect_contains(rendered, "  add x8, x8, :lo12:.L.str0\n",
                  "aarch64 backend should form the string-literal byte base address");
  expect_contains(rendered, "  ldrb w0, [x8, #1]\n",
                  "aarch64 backend should load the indexed string-literal byte");
  expect_contains(rendered, "  sxtb w0, w0\n",
                  "aarch64 backend should preserve byte-to-int sign extension");
  expect_contains(rendered, "  ret\n",
                  "aarch64 backend should return the promoted string-literal result");
}

void test_aarch64_backend_renders_extern_decl_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_extern_decl_call_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "declare i32 @helper_ext(...)\n",
                  "aarch64 backend should render module extern declarations");
  expect_contains(rendered, "define i32 @main()\n{\nentry:\n  %t0 = call i32 @helper_ext(i32 5)\n",
                  "aarch64 backend should preserve direct calls that target extern declarations");
  expect_contains(rendered, "ret i32 %t0",
                  "aarch64 backend should preserve the extern call result");
}

void test_aarch64_backend_renders_extern_global_load_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_extern_global_load_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".extern ext_counter\n",
                  "aarch64 backend should declare extern scalar globals for ELF assembly");
  expect_contains(rendered, ".globl main\n",
                  "aarch64 backend should still publish main as the entry symbol");
  expect_contains(rendered, "adrp x8, ext_counter\n",
                  "aarch64 backend should form the extern scalar global page address");
  expect_contains(rendered, "ldr w0, [x8, :lo12:ext_counter]\n",
                  "aarch64 backend should load the extern scalar global directly into the return register");
  expect_contains(rendered, "ret\n",
                  "aarch64 backend should return the loaded extern scalar global value");
}

void test_aarch64_backend_renders_extern_global_array_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_extern_global_array_load_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".extern ext_arr\n",
                  "aarch64 backend should declare extern global arrays for ELF assembly");
  expect_contains(rendered, ".globl main\n",
                  "aarch64 backend should still publish main as the entry symbol");
  expect_contains(rendered, "adrp x8, ext_arr\n",
                  "aarch64 backend should form the extern global array page address");
  expect_contains(rendered, "add x8, x8, :lo12:ext_arr\n",
                  "aarch64 backend should materialize the extern global array base address");
  expect_contains(rendered, "ldr w0, [x8, #4]\n",
                  "aarch64 backend should fold the bounded indexed load into the extern global array access");
  expect_contains(rendered, "ret\n",
                  "aarch64 backend should return the extern global array load result");
  expect_not_contains(rendered, "getelementptr",
                      "aarch64 backend should no longer fall back to LLVM IR for the extern global array slice");
}

void test_aarch64_backend_renders_global_char_pointer_diff_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_global_char_pointer_diff_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".bss\n",
                  "aarch64 backend should place mutable zero-initialized byte arrays into BSS");
  expect_contains(rendered, ".globl g_bytes\n",
                  "aarch64 backend should publish the bounded byte-array symbol");
  expect_contains(rendered, "g_bytes:\n  .zero 2\n",
                  "aarch64 backend should materialize the bounded byte-array storage");
  expect_contains(rendered, ".globl main\n",
                  "aarch64 backend should still publish main as the entry symbol");
  expect_contains(rendered, "adrp x8, g_bytes\n",
                  "aarch64 backend should materialize the global byte-array page address");
  expect_contains(rendered, "add x8, x8, :lo12:g_bytes\n",
                  "aarch64 backend should materialize the global byte-array base address");
  expect_contains(rendered, "add x9, x8, #1\n",
                  "aarch64 backend should form the byte-offset address for the bounded slice");
  expect_contains(rendered, "sub x8, x9, x8\n",
                  "aarch64 backend should preserve the byte-granular pointer subtraction");
  expect_contains(rendered, "cmp x8, #1\n",
                  "aarch64 backend should compare the pointer difference against one byte");
  expect_contains(rendered, "cset w0, eq\n",
                  "aarch64 backend should lower the bounded equality result into the return register");
  expect_contains(rendered, "ret\n",
                  "aarch64 backend should return the bounded pointer-difference comparison result");
  expect_not_contains(rendered, "getelementptr",
                      "aarch64 backend should no longer fall back to LLVM IR for the bounded global char pointer-difference slice");
}

void test_aarch64_backend_renders_global_char_pointer_diff_slice_from_typed_ops() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_typed_global_char_pointer_diff_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "adrp x8, g_bytes\n",
                  "aarch64 backend should keep the typed byte pointer-difference slice on the asm path");
  expect_contains(rendered, "sub x8, x9, x8\n",
                  "aarch64 backend should decode typed subtraction wrappers in the byte pointer-difference slice");
  expect_contains(rendered, "cmp x8, #1\n",
                  "aarch64 backend should still compare the typed byte pointer difference against one byte");
  expect_contains(rendered, "cset w0, eq\n",
                  "aarch64 backend should lower typed equality wrappers for the byte pointer-difference slice");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend should not fall back to LLVM IR for the typed byte pointer-difference slice");
}

void test_aarch64_backend_renders_global_int_pointer_diff_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_global_int_pointer_diff_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".bss\n",
                  "aarch64 backend should place mutable zero-initialized int arrays into BSS");
  expect_contains(rendered, ".globl g_words\n",
                  "aarch64 backend should publish the bounded int-array symbol");
  expect_contains(rendered, "g_words:\n  .zero 8\n",
                  "aarch64 backend should materialize the bounded int-array storage");
  expect_contains(rendered, ".globl main\n",
                  "aarch64 backend should still publish main as the entry symbol");
  expect_contains(rendered, "adrp x8, g_words\n",
                  "aarch64 backend should materialize the global int-array page address");
  expect_contains(rendered, "add x8, x8, :lo12:g_words\n",
                  "aarch64 backend should materialize the global int-array base address");
  expect_contains(rendered, "add x9, x8, #4\n",
                  "aarch64 backend should form the one-element int offset in bytes");
  expect_contains(rendered, "sub x8, x9, x8\n",
                  "aarch64 backend should preserve byte-granular pointer subtraction before scaling");
  expect_contains(rendered, "lsr x8, x8, #2\n",
                  "aarch64 backend should lower the divide-by-four scaling step for the bounded int slice");
  expect_contains(rendered, "cmp x8, #1\n",
                  "aarch64 backend should compare the scaled pointer difference against one element");
  expect_contains(rendered, "cset w0, eq\n",
                  "aarch64 backend should lower the bounded equality result into the return register");
  expect_contains(rendered, "ret\n",
                  "aarch64 backend should return the bounded scaled pointer-difference comparison result");
  expect_not_contains(rendered, "getelementptr",
                      "aarch64 backend should no longer fall back to LLVM IR for the bounded global int pointer-difference slice");
}

void test_aarch64_backend_renders_global_int_pointer_diff_slice_from_typed_ops() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_typed_global_int_pointer_diff_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "adrp x8, g_words\n",
                  "aarch64 backend should keep the typed int pointer-difference slice on the asm path");
  expect_contains(rendered, "sub x8, x9, x8\n",
                  "aarch64 backend should decode typed subtraction wrappers before scaling int pointer differences");
  expect_contains(rendered, "lsr x8, x8, #2\n",
                  "aarch64 backend should decode typed signed-divide wrappers for the scaled int pointer-difference slice");
  expect_contains(rendered, "cmp x8, #1\n",
                  "aarch64 backend should still compare the typed scaled pointer difference against one element");
  expect_contains(rendered, "cset w0, eq\n",
                  "aarch64 backend should lower typed equality wrappers for the scaled int pointer-difference slice");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend should not fall back to LLVM IR for the typed int pointer-difference slice");
}

void test_aarch64_backend_renders_global_int_pointer_roundtrip_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_global_int_pointer_roundtrip_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl g_value\n",
                  "aarch64 backend should publish the round-trip global symbol");
  expect_contains(rendered, "g_value:\n  .long 9\n",
                  "aarch64 backend should materialize the round-trip global initializer");
  expect_contains(rendered, "adrp x8, g_value\n",
                  "aarch64 backend should materialize the global address for the bounded round-trip slice");
  expect_contains(rendered, "ldr w0, [x8, :lo12:g_value]\n",
                  "aarch64 backend should lower the bounded round-trip back into a direct global load");
  expect_contains(rendered, "ret\n",
                  "aarch64 backend should return the bounded round-trip load result");
  expect_not_contains(rendered, "ptrtoint",
                      "aarch64 backend should no longer fall back to LLVM IR ptrtoint for the bounded round-trip slice");
  expect_not_contains(rendered, "inttoptr",
                      "aarch64 backend should no longer fall back to LLVM IR inttoptr for the bounded round-trip slice");
  expect_not_contains(rendered, "alloca",
                      "aarch64 backend should no longer fall back to LLVM IR allocas for the bounded round-trip slice");
}

void test_aarch64_backend_renders_bitcast_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bitcast_scalar_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "%t0 = bitcast double 0.000000e+00 to i64",
                  "aarch64 backend should render bitcast within the target-local cast path");
  expect_contains(rendered, "ret i64 %t0",
                  "aarch64 backend should preserve the bitcast result");
}

void test_aarch64_backend_renders_trunc_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_trunc_scalar_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "%t0 = trunc i32 13124 to i16",
                  "aarch64 backend should render trunc within the target-local cast path");
  expect_contains(rendered, "ret i16 %t0",
                  "aarch64 backend should preserve the trunc result");
}

void test_aarch64_backend_renders_va_intrinsic_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_va_intrinsic_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "declare void @llvm.va_start.p0(ptr)",
                  "aarch64 backend should render llvm.va_start declarations");
  expect_contains(rendered, "declare void @llvm.va_end.p0(ptr)",
                  "aarch64 backend should render llvm.va_end declarations");
  expect_contains(rendered, "declare void @llvm.va_copy.p0.p0(ptr, ptr)",
                  "aarch64 backend should render llvm.va_copy declarations");
  expect_contains(rendered, "call void @llvm.va_start.p0(ptr %lv.ap)",
                  "aarch64 backend should render llvm.va_start calls");
  expect_contains(rendered, "call void @llvm.va_copy.p0.p0(ptr %lv.ap_copy, ptr %lv.ap)",
                  "aarch64 backend should render llvm.va_copy calls");
  expect_contains(rendered, "call void @llvm.va_end.p0(ptr %lv.ap_copy)",
                  "aarch64 backend should render llvm.va_end calls for copied lists");
  expect_contains(rendered, "call void @llvm.va_end.p0(ptr %lv.ap)",
                  "aarch64 backend should render llvm.va_end calls for the original list");
}

void test_aarch64_backend_renders_va_arg_scalar_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_va_arg_scalar_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "call void @llvm.va_start.p0(ptr %lv.ap)",
                  "aarch64 backend should preserve va_start before va_arg");
  expect_contains(rendered, "%t0 = va_arg ptr %lv.ap, i32",
                  "aarch64 backend should render scalar va_arg in the target-local memory path");
  expect_contains(rendered, "%t1 = add i32 %p.first, %t0",
                  "aarch64 backend should preserve arithmetic that consumes va_arg results");
  expect_contains(rendered, "call void @llvm.va_end.p0(ptr %lv.ap)",
                  "aarch64 backend should preserve va_end after va_arg");
  expect_contains(rendered, "ret i32 %t1",
                  "aarch64 backend should preserve the scalar va_arg result");
}

void test_aarch64_backend_renders_va_arg_pair_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_va_arg_pair_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)",
                  "aarch64 backend should render llvm.memcpy declarations for aggregate va_arg");
  expect_contains(rendered, "%t6 = phi ptr [ %t3, %reg ], [ %t5, %stack ]",
                  "aarch64 backend should preserve the aggregate va_arg helper phi join");
  expect_contains(rendered,
                  "call void @llvm.memcpy.p0.p0.i64(ptr %lv.pair.tmp, ptr %t6, i64 8, i1 false)",
                  "aarch64 backend should render aggregate va_arg copies through llvm.memcpy");
  expect_contains(rendered, "call void @llvm.va_end.p0(ptr %lv.ap)",
                  "aarch64 backend should preserve va_end after aggregate va_arg handling");
  expect_contains(rendered, "ret i32 %p.seed",
                  "aarch64 backend should preserve the enclosing function return");
}

void test_aarch64_backend_renders_va_arg_bigints_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_va_arg_bigints_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "%struct.BigInts = type { i32, i32, i32, i32, i32 }",
                  "aarch64 backend should preserve larger aggregate type declarations");
  expect_contains(rendered, "%t11 = phi ptr [ %t7, %reg ], [ %t9, %stack ]",
                  "aarch64 backend should preserve the indirect aggregate helper phi join");
  expect_contains(rendered, "%t12 = load ptr, ptr %t11",
                  "aarch64 backend should reload the indirect aggregate source pointer");
  expect_contains(rendered,
                  "call void @llvm.memcpy.p0.p0.i64(ptr %lv.bigints.tmp, ptr %t12, i64 20, i1 false)",
                  "aarch64 backend should render indirect aggregate va_arg copies through llvm.memcpy");
  expect_contains(rendered, "call void @llvm.va_end.p0(ptr %lv.ap)",
                  "aarch64 backend should preserve va_end after indirect aggregate va_arg handling");
}

void test_aarch64_backend_renders_phi_join_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_phi_join_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "br i1 %t0, label %reg, label %stack",
                  "aarch64 backend should preserve the helper control-flow split before a phi join");
  expect_contains(rendered, "%t1 = phi ptr [ %reg.addr, %reg ], [ %stack.addr, %stack ]",
                  "aarch64 backend should render pointer phi joins used by target-local va_arg helpers");
}

void test_aarch64_assembler_parser_stub_preserves_text() {
  const std::string asm_text = ".text\n.globl main\nmain:\n  ret\n";
  const auto statements = c4c::backend::aarch64::assembler::parse_asm(asm_text);
  expect_true(statements.size() == 4,
              "aarch64 assembler parser should split the minimal text slice into line-oriented statements");
  expect_true(statements[0].text == ".text" && statements[1].text == ".globl main" &&
                  statements[2].text == "main:" && statements[3].text == "ret",
              "aarch64 assembler parser should preserve the directive, label, and instruction text for the minimal slice");
}

void test_x86_assembler_parser_accepts_bounded_return_add_slice() {
  const auto asm_text = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_return_add_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  const auto statements = c4c::backend::x86::assembler::parse_asm(asm_text);

  expect_true(statements.size() == 6,
              "x86 assembler parser should keep the bounded return-add slice as six structured statements");
  expect_true(statements[0].text == ".intel_syntax noprefix" &&
                  statements[1].text == ".text" &&
                  statements[2].text == ".globl main" &&
                  statements[3].text == "main:" &&
                  statements[4].text == "mov eax, 5" &&
                  statements[5].text == "ret",
              "x86 assembler parser should preserve the Step 2 directive, label, and instruction surface for the bounded return-add slice");
  expect_true(statements[4].operands.size() == 2 &&
                  statements[4].operands[0].text == "eax" &&
                  statements[4].operands[1].text == "5",
              "x86 assembler parser should split the bounded mov eax, imm32 instruction into destination and immediate operands");
}

void test_x86_assembler_parser_rejects_out_of_scope_forms() {
  try {
    (void)c4c::backend::x86::assembler::parse_asm(
        ".intel_syntax noprefix\n.text\n.globl main\nmain:\n  mov eax, dword ptr [rax]\n  ret\n");
    fail("x86 assembler parser should reject out-of-scope memory operand forms");
  } catch (const std::runtime_error& ex) {
    expect_contains(ex.what(), "unsupported x86 immediate",
                    "x86 assembler parser should explain why Step 2 rejects memory-form mov sources");
  }

  try {
    (void)c4c::backend::x86::assembler::parse_asm(
        ".intel_syntax noprefix\n.text\n.globl main\nmain:\n.Lblock_1:\n  mov eax, 5\n  ret\n");
    fail("x86 assembler parser should reject local labels outside the first bounded slice");
  } catch (const std::runtime_error& ex) {
    expect_contains(ex.what(), "unsupported x86 label",
                    "x86 assembler parser should keep local labels explicitly unsupported in the Step 2 slice");
  }
}

void test_x86_assembler_encoder_emits_bounded_return_add_bytes() {
  namespace encoder = c4c::backend::x86::assembler::encoder;

  const auto asm_text = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_return_add_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  const auto statements = c4c::backend::x86::assembler::parse_asm(asm_text);
  const auto encoded = encoder::encode_function(statements);

  expect_true(encoded.encoded,
              "x86 assembler encoder should accept the bounded return-add Step 3 slice");
  expect_true(encoded.bytes ==
                  std::vector<std::uint8_t>{0xB8, 0x05, 0x00, 0x00, 0x00, 0xC3},
              "x86 assembler encoder should emit mov eax, imm32; ret bytes matching the Step 3 contract");
}

void test_x86_assembler_encoder_emits_bounded_extern_call_bytes() {
  namespace encoder = c4c::backend::x86::assembler::encoder;

  const auto asm_text = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_x86_extern_decl_object_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  const auto statements = c4c::backend::x86::assembler::parse_asm(asm_text);
  const auto encoded = encoder::encode_function(statements);

  expect_true(encoded.encoded,
              "x86 assembler encoder should accept the bounded extern-call relocation slice");
  expect_true(encoded.bytes ==
                  std::vector<std::uint8_t>{0xE8, 0x00, 0x00, 0x00, 0x00, 0xC3},
              "x86 assembler encoder should emit call rel32; ret bytes for the bounded extern-call slice");
  expect_true(encoded.relocations.size() == 1,
              "x86 assembler encoder should record one relocation for the bounded extern-call slice");
  expect_true(encoded.relocations[0].offset == 1 &&
                  encoded.relocations[0].symbol == "helper_ext" &&
                  encoded.relocations[0].reloc_type == 4 &&
                  encoded.relocations[0].addend == -4,
              "x86 assembler encoder should preserve the staged PLT32 relocation contract for the bounded extern-call slice");
}

void test_x86_assembler_encoder_rejects_out_of_scope_instruction_forms() {
  namespace encoder = c4c::backend::x86::assembler::encoder;

  c4c::backend::x86::assembler::AsmStatement unsupported;
  unsupported.kind = c4c::backend::x86::assembler::AsmStatementKind::Instruction;
  unsupported.text = "push rbp";
  unsupported.op = "push";

  const auto unsupported_result = encoder::encode_instruction(unsupported);
  expect_true(!unsupported_result.encoded,
              "x86 assembler encoder should reject instructions outside the first Step 3 slice");
  expect_contains(unsupported_result.error, "unsupported x86 instruction",
                  "x86 assembler encoder should explain unsupported instruction rejections");

  c4c::backend::x86::assembler::AsmStatement wrong_mov;
  wrong_mov.kind = c4c::backend::x86::assembler::AsmStatementKind::Instruction;
  wrong_mov.text = "mov ebx, 7";
  wrong_mov.op = "mov";
  wrong_mov.operands = {
      c4c::backend::x86::assembler::Operand{"ebx"},
      c4c::backend::x86::assembler::Operand{"7"},
  };

  const auto wrong_mov_result = encoder::encode_instruction(wrong_mov);
  expect_true(!wrong_mov_result.encoded,
              "x86 assembler encoder should keep register coverage bounded to eax in the first slice");
  expect_contains(wrong_mov_result.error, "mov eax, imm32",
                  "x86 assembler encoder should spell out the current bounded mov contract");
}

void test_x86_builtin_object_matches_external_return_add_surface() {
#if defined(C4C_TEST_CLANG_PATH) && defined(C4C_TEST_OBJDUMP_PATH)
  auto module = make_return_add_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-n8:16:32:64-S128";

  const auto asm_text = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  const auto asm_path = make_temp_path("c4c_x86_return_add_surface", ".s");
  const auto builtin_object_path = make_temp_output_path("c4c_x86_return_add_builtin");
  const auto external_object_path = make_temp_output_path("c4c_x86_return_add_external");

  write_text_file(asm_path, asm_text);
  const auto builtin = c4c::backend::x86::assemble_module(module, builtin_object_path);
  expect_true(builtin.object_emitted,
              "x86 backend handoff helper should emit an object for the bounded return_add slice");
  expect_true(builtin.error.empty(),
              "x86 backend handoff helper should not report an error for the bounded return_add slice");

  run_command_capture(shell_quote(C4C_TEST_CLANG_PATH) +
                      " --target=x86_64-unknown-linux-gnu -c " +
                      shell_quote(asm_path) + " -o " +
                      shell_quote(external_object_path) + " 2>&1");

  const auto builtin_objdump = normalize_objdump_surface(
      run_command_capture(shell_quote(C4C_TEST_OBJDUMP_PATH) + " -h -r -t " +
                          shell_quote(builtin_object_path) + " 2>&1"));
  const auto external_objdump = normalize_objdump_surface(
      run_command_capture(shell_quote(C4C_TEST_OBJDUMP_PATH) + " -h -r -t " +
                          shell_quote(external_object_path) + " 2>&1"));
  expect_contains(builtin_objdump, ".text         00000006",
                  "x86 built-in assembler should emit the same bounded .text size as the external baseline");
  expect_contains(external_objdump, ".text         00000006",
                  "external assembler baseline should keep the bounded .text size for x86 return_add");
  expect_not_contains(builtin_objdump, "RELOCATION RECORDS",
                      "x86 built-in assembler should not introduce relocations for the bounded return_add slice");
  expect_not_contains(external_objdump, "RELOCATION RECORDS",
                      "external assembler baseline should not need relocations for the bounded x86 return_add slice");
  expect_contains(builtin_objdump, "g     F .text",
                  "x86 built-in assembler should expose a global function symbol in .text");
  expect_contains(builtin_objdump, "main",
                  "x86 built-in assembler should preserve the main symbol name");
  expect_contains(external_objdump, "g       .text",
                  "external assembler baseline should expose a global function symbol in .text");
  expect_contains(external_objdump, "main",
                  "external assembler baseline should preserve the main symbol name");

  const auto builtin_disasm = normalize_objdump_disassembly(
      run_command_capture(shell_quote(C4C_TEST_OBJDUMP_PATH) + " -d " +
                          shell_quote(builtin_object_path) + " 2>&1"));
  const auto external_disasm = normalize_objdump_disassembly(
      run_command_capture(shell_quote(C4C_TEST_OBJDUMP_PATH) + " -d " +
                          shell_quote(external_object_path) + " 2>&1"));
  expect_true(builtin_disasm == external_disasm,
              "x86 built-in assembler disassembly should match the external assembler baseline for return_add\n--- built-in ---\n" +
                  builtin_disasm + "--- external ---\n" + external_disasm);

  std::filesystem::remove(asm_path);
  std::filesystem::remove(builtin_object_path);
  std::filesystem::remove(external_object_path);
#endif
}

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

void test_x86_builtin_object_matches_external_extern_call_surface() {
#if defined(C4C_TEST_CLANG_PATH) && defined(C4C_TEST_OBJDUMP_PATH)
  const auto module = make_x86_extern_decl_object_module();
  const auto asm_text = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  const auto asm_path = make_temp_path("c4c_x86_extern_call_surface", ".s");
  const auto builtin_object_path = make_temp_output_path("c4c_x86_extern_call_builtin");
  const auto external_object_path = make_temp_output_path("c4c_x86_extern_call_external");

  write_text_file(asm_path, asm_text);
  const auto builtin = c4c::backend::x86::assemble_module(module, builtin_object_path);
  expect_true(builtin.object_emitted,
              "x86 backend handoff helper should emit an object for the bounded extern-call slice");
  expect_true(builtin.error.empty(),
              "x86 backend handoff helper should not report an error for the bounded extern-call slice");

  run_command_capture(shell_quote(C4C_TEST_CLANG_PATH) +
                      " --target=x86_64-unknown-linux-gnu -c " +
                      shell_quote(asm_path) + " -o " +
                      shell_quote(external_object_path) + " 2>&1");

  const auto builtin_objdump = normalize_objdump_surface(
      run_command_capture(shell_quote(C4C_TEST_OBJDUMP_PATH) + " -h -r -t " +
                          shell_quote(builtin_object_path) + " 2>&1"));
  const auto external_objdump = normalize_objdump_surface(
      run_command_capture(shell_quote(C4C_TEST_OBJDUMP_PATH) + " -h -r -t " +
                          shell_quote(external_object_path) + " 2>&1"));
  expect_contains(builtin_objdump, ".text         00000006",
                  "x86 built-in assembler should emit the bounded extern-call .text size");
  expect_contains(external_objdump, ".text         00000006",
                  "external assembler baseline should keep the bounded extern-call .text size");
  expect_contains(builtin_objdump, "RELOCATION RECORDS FOR [.text]:",
                  "x86 built-in assembler should emit relocation records for the bounded extern-call slice");
  expect_contains(external_objdump, "RELOCATION RECORDS FOR [.text]:",
                  "external assembler baseline should emit relocation records for the bounded extern-call slice");
  expect_contains(builtin_objdump, "R_X86_64_PLT32",
                  "x86 built-in assembler should emit the staged PLT32 relocation type");
  expect_contains(external_objdump, "R_X86_64_PLT32",
                  "external assembler baseline should emit the staged PLT32 relocation type");
  expect_contains(builtin_objdump, "helper_ext",
                  "x86 built-in assembler should preserve the bounded undefined helper symbol");
  expect_contains(external_objdump, "helper_ext",
                  "external assembler baseline should preserve the bounded undefined helper symbol");

  const auto builtin_disasm = normalize_objdump_disassembly(
      run_command_capture(shell_quote(C4C_TEST_OBJDUMP_PATH) + " -d " +
                          shell_quote(builtin_object_path) + " 2>&1"));
  const auto external_disasm = normalize_objdump_disassembly(
      run_command_capture(shell_quote(C4C_TEST_OBJDUMP_PATH) + " -d " +
                          shell_quote(external_object_path) + " 2>&1"));
  expect_true(builtin_disasm == external_disasm,
              "x86 built-in assembler disassembly should match the external assembler baseline for the bounded extern-call slice\n--- built-in ---\n" +
                  builtin_disasm + "--- external ---\n" + external_disasm);

  std::filesystem::remove(asm_path);
  std::filesystem::remove(builtin_object_path);
  std::filesystem::remove(external_object_path);
#endif
}

void test_aarch64_assembler_elf_writer_branch_reloc_helper() {
  using c4c::backend::aarch64::assembler::is_branch_reloc_type;

  expect_true(is_branch_reloc_type(279),
              "aarch64 elf writer helper should treat R_AARCH64_CALL26 as a branch reloc");
  expect_true(is_branch_reloc_type(280),
              "aarch64 elf writer helper should treat R_AARCH64_JUMP26 as a branch reloc");
  expect_true(is_branch_reloc_type(282),
              "aarch64 elf writer helper should treat R_AARCH64_CONDBR19 as a branch reloc");
  expect_true(is_branch_reloc_type(283),
              "aarch64 elf writer helper should treat R_AARCH64_TSTBR14 as a branch reloc");
  expect_true(!is_branch_reloc_type(257),
              "aarch64 elf writer helper should keep non-branch relocations out of the branch-only set");
}

void test_aarch64_assembler_encoder_helper_smoke() {
  namespace encoder = c4c::backend::aarch64::assembler::encoder;

  expect_true(encoder::is_64bit_reg("x9"),
              "aarch64 encoder helper should recognize x-register names as 64-bit GPRs");
  expect_true(!encoder::is_64bit_reg("w9"),
              "aarch64 encoder helper should reject w-register names as 64-bit GPRs");
  expect_true(encoder::is_32bit_reg("w3"),
              "aarch64 encoder helper should recognize w-register names as 32-bit GPRs");
  expect_true(!encoder::is_32bit_reg("x3"),
              "aarch64 encoder helper should reject x-register names as 32-bit GPRs");
  expect_true(encoder::is_fp_reg("d0") && encoder::is_fp_reg("s1") &&
                  encoder::is_fp_reg("q2") && encoder::is_fp_reg("v3"),
              "aarch64 encoder helper should recognize the current FP/SIMD register prefixes");
  expect_true(!encoder::is_fp_reg("x0"),
              "aarch64 encoder helper should keep integer register names out of the FP/SIMD set");
  expect_true(encoder::sf_bit(true) == 1u && encoder::sf_bit(false) == 0u,
              "aarch64 encoder helper should map the sf bit directly from operand width");
}

void test_backend_shared_liveness_surface_tracks_result_names() {
  const auto module = make_declaration_module();
  const auto& function = module.functions.back();
  const auto liveness = c4c::backend::compute_live_intervals(function);

  expect_true(liveness.call_points.size() == 1 && liveness.call_points.front() == 0,
              "shared liveness surface should record call program points for typed call ops");
  const auto* interval = liveness.find_interval("%t0");
  expect_true(interval != nullptr,
              "shared liveness surface should expose interval lookup by SSA result name");
  expect_true(interval->start == 0 && interval->end == 1,
              "shared liveness surface should extend intervals through later uses instead of keeping point-only placeholders");
}

void test_backend_shared_liveness_surface_tracks_call_crossing_ranges() {
  const auto module = make_call_crossing_interval_module();
  const auto& function = module.functions.back();
  const auto liveness = c4c::backend::compute_live_intervals(function);

  expect_true(liveness.call_points.size() == 1 && liveness.call_points.front() == 1,
              "shared liveness surface should record the direct call program point");
  const auto* before_call = liveness.find_interval("%t0");
  const auto* call_result = liveness.find_interval("%t1");
  const auto* final_sum = liveness.find_interval("%t2");
  expect_true(before_call != nullptr && before_call->start == 0 && before_call->end == 2,
              "shared liveness surface should extend values that remain live across a call through their later use");
  expect_true(call_result != nullptr && call_result->start == 1 && call_result->end == 2,
              "shared liveness surface should keep the call result live until its consuming instruction");
  expect_true(final_sum != nullptr && final_sum->start == 2 && final_sum->end == 3,
              "shared liveness surface should include terminator uses in the final interval endpoint");
}

void test_backend_shared_liveness_surface_tracks_phi_join_ranges() {
  const auto module = make_interval_phi_join_module();
  const auto& function = module.functions.front();
  const auto liveness = c4c::backend::compute_live_intervals(function);

  const auto* cond = liveness.find_interval("%t0");
  const auto* then_value = liveness.find_interval("%t1");
  const auto* else_value = liveness.find_interval("%t2");
  const auto* phi_value = liveness.find_interval("%t3");
  const auto* final_sum = liveness.find_interval("%t4");

  expect_true(cond != nullptr && cond->start == 0 && cond->end == 1,
              "shared liveness surface should extend branch conditions through the conditional terminator");
  expect_true(then_value != nullptr && then_value->start == 2 && then_value->end == 3,
              "shared liveness surface should treat phi incoming values as used on the predecessor edge");
  expect_true(else_value != nullptr && else_value->start == 4 && else_value->end == 5,
              "shared liveness surface should track the alternate phi incoming value on its predecessor edge");
  expect_true(phi_value != nullptr && phi_value->start == 6 && phi_value->end == 7,
              "shared liveness surface should start phi results in the join block and extend them to their consuming instruction");
  expect_true(final_sum != nullptr && final_sum->start == 7 && final_sum->end == 8,
              "shared liveness surface should keep the join result live through the return terminator");
}

void test_backend_shared_regalloc_surface_uses_caller_saved_for_non_call_interval() {
  const auto module = make_return_add_module();
  const auto& function = module.functions.front();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}, {21}};
  config.caller_saved_regs = {{13}};

  const auto regalloc = c4c::backend::allocate_registers(function, config);
  expect_true(regalloc.assignments.size() == 1,
              "shared regalloc surface should assign one register for the single-result helper slice");
  const auto it = regalloc.assignments.find("%t0");
  expect_true(it != regalloc.assignments.end() && it->second.index == 13,
              "shared regalloc surface should prefer the caller-saved pool for intervals that do not span calls");
  expect_true(regalloc.used_regs.empty(),
              "shared regalloc surface should leave the callee-saved usage set empty when only caller-saved registers are needed");
  expect_true(regalloc.liveness.has_value() &&
                  regalloc.liveness->find_interval("%t0") != nullptr,
              "shared regalloc surface should retain cached liveness for the handoff boundary");
}

void test_backend_shared_regalloc_prefers_callee_saved_for_call_spanning_values() {
  const auto module = make_call_crossing_interval_module();
  const auto& function = module.functions.back();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto regalloc = c4c::backend::allocate_registers(function, config);

  const auto before_call = regalloc.assignments.find("%t0");
  const auto call_result = regalloc.assignments.find("%t1");
  const auto final_sum = regalloc.assignments.find("%t2");

  expect_true(before_call != regalloc.assignments.end() && before_call->second.index == 20,
              "shared regalloc should keep call-spanning intervals on the callee-saved pool first");
  expect_true(call_result == regalloc.assignments.end(),
              "shared regalloc should spill overlapping call-spanning intervals when the callee-saved pool is exhausted");
  expect_true(final_sum != regalloc.assignments.end() && final_sum->second.index == 13,
              "shared regalloc should use caller-saved registers for non-call-spanning intervals");
  expect_true(regalloc.used_regs.size() == 1 && regalloc.used_regs.front().index == 20,
              "shared regalloc should report only the used callee-saved register set");
}

void test_backend_shared_regalloc_reuses_register_after_interval_ends() {
  const auto module = make_non_overlapping_interval_module();
  const auto& function = module.functions.front();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};

  const auto regalloc = c4c::backend::allocate_registers(function, config);

  const auto first_value = regalloc.assignments.find("%t0");
  const auto later_value = regalloc.assignments.find("%t2");

  expect_true(first_value != regalloc.assignments.end() && first_value->second.index == 20,
              "shared regalloc should assign the available register to the first live interval");
  expect_true(later_value != regalloc.assignments.end() && later_value->second.index == 20,
              "shared regalloc should reuse a freed register for a later non-overlapping interval");
  expect_true(regalloc.used_regs.size() == 1 && regalloc.used_regs.front().index == 20,
              "shared regalloc should keep the callee-saved usage set deduplicated when a register is reused");
}

void test_backend_shared_regalloc_spills_overlapping_values_without_reusing_busy_reg() {
  const auto module = make_overlapping_interval_module();
  const auto& function = module.functions.front();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};

  const auto regalloc = c4c::backend::allocate_registers(function, config);

  expect_true(regalloc.assignments.size() == 1,
              "shared regalloc should spill one of two overlapping intervals when only one register is available");
  const bool first_assigned = regalloc.assignments.find("%t0") != regalloc.assignments.end();
  const bool second_assigned = regalloc.assignments.find("%t1") != regalloc.assignments.end();
  expect_true(first_assigned != second_assigned,
              "shared regalloc should not assign the same busy register to overlapping intervals");
}

void test_backend_shared_regalloc_helper_filters_and_merges_clobbers() {
  const std::vector<c4c::backend::PhysReg> callee_saved = {{20}, {21}, {22}};
  const std::vector<c4c::backend::PhysReg> asm_clobbered = {{21}};
  const auto filtered =
      c4c::backend::stack_layout::filter_available_regs(callee_saved, asm_clobbered);

  expect_true(filtered.size() == 2 && filtered.front().index == 20 &&
                  filtered.back().index == 22,
              "shared stack-layout helper should remove inline-asm clobbered registers from the available set");

  const auto module = make_return_add_module();
  c4c::backend::RegAllocConfig config;
  config.available_regs = filtered;
  const auto merged = c4c::backend::run_regalloc_and_merge_clobbers(
      module.functions.front(), config, asm_clobbered);

  expect_true(merged.used_callee_saved.size() == 2 &&
                  merged.used_callee_saved.front().index == 20 &&
                  merged.used_callee_saved.back().index == 21,
              "shared regalloc helper should merge allocator usage with asm clobbers in sorted order");
}

void test_backend_shared_stack_layout_regalloc_helper_exposes_handoff_view() {
  const auto module = make_call_crossing_interval_module();
  const auto& function = module.functions.back();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};
  const std::vector<c4c::backend::PhysReg> asm_clobbered = {{21}};

  const auto merged =
      c4c::backend::run_regalloc_and_merge_clobbers(function, config, asm_clobbered);

  const auto* before_call =
      c4c::backend::stack_layout::find_assigned_reg(merged, "%t0");
  const auto* call_result =
      c4c::backend::stack_layout::find_assigned_reg(merged, "%t1");
  const auto* final_sum =
      c4c::backend::stack_layout::find_assigned_reg(merged, "%t2");
  const auto* cached_liveness =
      c4c::backend::stack_layout::find_cached_liveness(merged);

  expect_true(before_call != nullptr && before_call->index == 20,
              "shared stack-layout helper should expose assigned call-spanning values through the regalloc handoff");
  expect_true(call_result == nullptr,
              "shared stack-layout helper should preserve spilled values as missing from the assigned-register view");
  expect_true(final_sum != nullptr && final_sum->index == 13,
              "shared stack-layout helper should expose caller-saved assignments through the same handoff seam");
  expect_true(c4c::backend::stack_layout::uses_callee_saved_reg(merged, c4c::backend::PhysReg{20}),
              "shared stack-layout helper should report allocator-used callee-saved registers");
  expect_true(c4c::backend::stack_layout::uses_callee_saved_reg(merged, c4c::backend::PhysReg{21}),
              "shared stack-layout helper should also report inline-asm-clobbered callee-saved registers after merge");
  expect_true(!c4c::backend::stack_layout::uses_callee_saved_reg(merged, c4c::backend::PhysReg{22}),
              "shared stack-layout helper should reject untouched callee-saved registers");
  expect_true(cached_liveness != nullptr &&
                  cached_liveness->find_interval("%t0") != nullptr &&
                  cached_liveness->find_interval("%t1") != nullptr &&
                  cached_liveness->find_interval("%t2") != nullptr,
              "shared stack-layout helper should expose cached liveness for downstream stack-layout analysis");
}

void test_backend_shared_stack_layout_analysis_tracks_phi_use_blocks() {
  const auto module = make_interval_phi_join_module();
  const auto& function = module.functions.front();
  const c4c::backend::RegAllocIntegrationResult regalloc;
  const auto analysis =
      c4c::backend::stack_layout::analyze_stack_layout(function, regalloc, {});

  const auto* then_value_uses = analysis.find_use_blocks("%t1");
  const auto* else_value_uses = analysis.find_use_blocks("%t2");
  const auto* phi_value_uses = analysis.find_use_blocks("%t3");

  expect_true(then_value_uses != nullptr && then_value_uses->size() == 1 &&
                  then_value_uses->front() == 1,
              "shared stack-layout analysis should attribute phi incoming uses to the predecessor block");
  expect_true(else_value_uses != nullptr && else_value_uses->size() == 1 &&
                  else_value_uses->front() == 2,
              "shared stack-layout analysis should keep alternate phi incoming values on their own predecessor block");
  expect_true(phi_value_uses != nullptr && phi_value_uses->size() == 1 &&
                  phi_value_uses->front() == 3,
              "shared stack-layout analysis should record normal instruction uses in the consuming block");
}

void test_backend_shared_stack_layout_analysis_detects_dead_param_allocas() {
  const auto module = make_dead_param_alloca_candidate_module();
  const auto& function = module.functions.back();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};
  const auto regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(function, config, {});
  const auto analysis = c4c::backend::stack_layout::analyze_stack_layout(
      function, regalloc, {{20}, {21}, {22}});

  expect_true(analysis.uses_value("%p.x"),
              "shared stack-layout analysis should collect body-used SSA values");
  expect_true(!analysis.uses_value("%lv.param.x"),
              "shared stack-layout analysis should not treat the synthesized param spill as a body use");
  expect_true(analysis.is_dead_param_alloca("%lv.param.x"),
              "shared stack-layout analysis should mark dead param allocas when the corresponding parameter value is kept in a callee-saved register");
}

void test_backend_shared_stack_layout_analysis_tracks_call_arg_values() {
  const auto module = make_escaped_local_alloca_candidate_module();
  const auto& function = module.functions.back();
  const c4c::backend::RegAllocIntegrationResult regalloc;
  const auto analysis =
      c4c::backend::stack_layout::analyze_stack_layout(function, regalloc, {});

  expect_true(analysis.uses_value("%lv.buf"),
              "shared stack-layout analysis should treat typed call-argument operands as real uses");
}

void test_backend_shared_stack_layout_analysis_detects_entry_alloca_overwrite_before_read() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto overwrite_first_module = make_overwrite_first_scalar_local_alloca_candidate_module();
  const auto& overwrite_first_function = overwrite_first_module.functions.front();
  const auto overwrite_first_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(overwrite_first_function, regalloc, {});
  expect_true(overwrite_first_analysis.is_entry_alloca_overwritten_before_read("%lv.x"),
              "shared stack-layout analysis should recognize live entry allocas whose first real access overwrites the slot before any read");

  const auto read_first_module = make_read_before_store_scalar_local_alloca_candidate_module();
  const auto& read_first_function = read_first_module.functions.front();
  const auto read_first_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(read_first_function, regalloc, {});
  expect_true(!read_first_analysis.is_entry_alloca_overwritten_before_read("%lv.x"),
              "shared stack-layout analysis should keep entry zero-init stores when the slot is read before it is overwritten");
}

void test_backend_shared_alloca_coalescing_classifies_non_param_allocas() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto dead_module = make_dead_local_alloca_candidate_module();
  const auto& dead_function = dead_module.functions.front();
  const auto dead_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(dead_function, regalloc, {});
  const auto dead_coalescing =
      c4c::backend::stack_layout::compute_coalescable_allocas(dead_function,
                                                              dead_analysis);
  expect_true(dead_coalescing.is_dead_alloca("%lv.buf") &&
                  !dead_coalescing.find_single_block("%lv.buf").has_value(),
              "shared alloca-coalescing should classify unused non-param allocas as dead instead of forcing a permanent slot");

  const auto single_block_module = make_live_local_alloca_candidate_module();
  const auto& single_block_function = single_block_module.functions.front();
  const auto single_block_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(single_block_function, regalloc, {});
  const auto single_block_coalescing =
      c4c::backend::stack_layout::compute_coalescable_allocas(single_block_function,
                                                              single_block_analysis);
  expect_true(!single_block_coalescing.is_dead_alloca("%lv.buf") &&
                  single_block_coalescing.find_single_block("%lv.buf") == 0,
              "shared alloca-coalescing should recognize GEP-tracked non-param allocas whose uses stay within one block");

  const auto escaped_module = make_escaped_local_alloca_candidate_module();
  const auto& escaped_function = escaped_module.functions.back();
  const auto escaped_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(escaped_function, regalloc, {});
  const auto escaped_coalescing =
      c4c::backend::stack_layout::compute_coalescable_allocas(escaped_function,
                                                              escaped_analysis);
  expect_true(!escaped_coalescing.is_dead_alloca("%lv.buf") &&
                  !escaped_coalescing.find_single_block("%lv.buf").has_value(),
              "shared alloca-coalescing should leave call-escaped allocas out of the single-block pool");
}

void test_backend_shared_slot_assignment_plans_param_alloca_slots() {
  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto dead_module = make_dead_param_alloca_candidate_module();
  const auto& dead_function = dead_module.functions.back();
  const auto dead_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(dead_function, config, {});
  const auto dead_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      dead_function, dead_regalloc, {{20}, {21}, {22}});
  const auto dead_plans =
      c4c::backend::stack_layout::plan_param_alloca_slots(dead_function, dead_analysis);

  expect_true(dead_plans.size() == 1 && dead_plans.front().alloca_name == "%lv.param.x" &&
                  dead_plans.front().param_name == "%p.x" &&
                  !dead_plans.front().needs_stack_slot,
              "shared slot-assignment planning should skip dead param allocas once analysis proves the parameter is preserved in a callee-saved register");

  const auto live_module = make_live_param_alloca_candidate_module();
  const auto& live_function = live_module.functions.front();
  const auto live_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(live_function, config, {});
  const auto live_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      live_function, live_regalloc, {{20}, {21}, {22}});
  const auto live_plans =
      c4c::backend::stack_layout::plan_param_alloca_slots(live_function, live_analysis);

  expect_true(live_plans.size() == 1 && live_plans.front().alloca_name == "%lv.param.x" &&
                  live_plans.front().param_name == "%p.x" &&
                  live_plans.front().needs_stack_slot,
              "shared slot-assignment planning should retain live param allocas when the function body still reads from the parameter slot");
}

void test_backend_shared_slot_assignment_prunes_dead_param_alloca_insts() {
  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto dead_module = make_dead_param_alloca_candidate_module();
  const auto& dead_function = dead_module.functions.back();
  const auto dead_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(dead_function, config, {});
  const auto dead_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      dead_function, dead_regalloc, {{20}, {21}, {22}});
  const auto dead_plans =
      c4c::backend::stack_layout::plan_param_alloca_slots(dead_function, dead_analysis);
  const auto dead_pruned =
      c4c::backend::stack_layout::prune_dead_param_alloca_insts(dead_function, dead_plans);

  expect_true(dead_pruned.empty(),
              "shared slot-assignment pruning should drop dead param allocas and their entry stores");

  const auto live_module = make_live_param_alloca_candidate_module();
  const auto& live_function = live_module.functions.front();
  const auto live_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(live_function, config, {});
  const auto live_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      live_function, live_regalloc, {{20}, {21}, {22}});
  const auto live_plans =
      c4c::backend::stack_layout::plan_param_alloca_slots(live_function, live_analysis);
  const auto live_pruned =
      c4c::backend::stack_layout::prune_dead_param_alloca_insts(live_function, live_plans);

  expect_true(live_pruned.size() == live_function.alloca_insts.size(),
              "shared slot-assignment pruning should preserve live param allocas");
}

void test_backend_shared_slot_assignment_plans_entry_alloca_slots() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto dead_module = make_dead_local_alloca_candidate_module();
  const auto& dead_function = dead_module.functions.front();
  const auto dead_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(dead_function, regalloc, {});
  const auto dead_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(dead_function, dead_analysis);

  expect_true(dead_plans.size() == 1 && dead_plans.front().alloca_name == "%lv.buf" &&
                  !dead_plans.front().needs_stack_slot &&
                  dead_plans.front().remove_following_entry_store,
              "shared slot-assignment planning should drop dead non-param entry allocas and their paired zero-init stores when analysis finds no uses");

  const auto live_module = make_live_local_alloca_candidate_module();
  const auto& live_function = live_module.functions.front();
  const auto live_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(live_function, regalloc, {});
  const auto live_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(live_function, live_analysis);

  expect_true(live_plans.size() == 1 && live_plans.front().alloca_name == "%lv.buf" &&
                  live_plans.front().needs_stack_slot &&
                  !live_plans.front().remove_following_entry_store &&
                  live_plans.front().coalesced_block == 0 &&
                  live_plans.front().assigned_slot == 0,
              "shared slot-assignment planning should preserve paired zero-init stores for aggregate entry allocas while exposing the shared single-block reuse classification");

  const auto disjoint_module = make_disjoint_block_local_alloca_candidate_module();
  const auto& disjoint_function = disjoint_module.functions.front();
  const auto disjoint_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(disjoint_function, regalloc, {});
  const auto disjoint_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(disjoint_function,
                                                          disjoint_analysis);

  expect_true(disjoint_plans.size() == 2 &&
                  disjoint_plans[0].alloca_name == "%lv.then" &&
                  disjoint_plans[0].coalesced_block == 1 &&
                  disjoint_plans[1].alloca_name == "%lv.else" &&
                  disjoint_plans[1].coalesced_block == 2 &&
                  disjoint_plans[0].assigned_slot.has_value() &&
                  disjoint_plans[0].assigned_slot == disjoint_plans[1].assigned_slot,
              "shared slot-assignment planning should turn disjoint single-block allocas into a concrete shared slot decision");

  const auto same_block_module = make_same_block_local_alloca_candidate_module();
  const auto& same_block_function = same_block_module.functions.front();
  const auto same_block_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(same_block_function, regalloc, {});
  const auto same_block_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(same_block_function,
                                                          same_block_analysis);

  expect_true(same_block_plans.size() == 2 &&
                  same_block_plans[0].coalesced_block == 0 &&
                  same_block_plans[1].coalesced_block == 0 &&
                  same_block_plans[0].assigned_slot.has_value() &&
                  same_block_plans[1].assigned_slot.has_value() &&
                  same_block_plans[0].assigned_slot != same_block_plans[1].assigned_slot,
              "shared slot-assignment planning should keep same-block local allocas in distinct slots even when both are individually coalescable");

  const auto read_first_module = make_read_before_store_local_alloca_candidate_module();
  const auto& read_first_function = read_first_module.functions.front();
  const auto read_first_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(read_first_function, regalloc, {});
  const auto read_first_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(read_first_function,
                                                          read_first_analysis);

  expect_true(read_first_plans.size() == 1 &&
                  read_first_plans.front().alloca_name == "%lv.buf" &&
                  read_first_plans.front().needs_stack_slot &&
                  !read_first_plans.front().remove_following_entry_store,
              "shared slot-assignment planning should preserve paired zero-init stores when a live entry alloca is read before the first overwrite");

  const auto scalar_overwrite_module = make_overwrite_first_scalar_local_alloca_candidate_module();
  const auto& scalar_overwrite_function = scalar_overwrite_module.functions.front();
  const auto scalar_overwrite_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(scalar_overwrite_function, regalloc, {});
  const auto scalar_overwrite_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(scalar_overwrite_function,
                                                          scalar_overwrite_analysis);

  expect_true(scalar_overwrite_plans.size() == 1 &&
                  scalar_overwrite_plans.front().alloca_name == "%lv.x" &&
                  scalar_overwrite_plans.front().needs_stack_slot &&
                  scalar_overwrite_plans.front().remove_following_entry_store,
              "shared slot-assignment planning should drop redundant paired zero-init stores for scalar entry allocas that are overwritten before any read");

  const auto scalar_read_first_module =
      make_read_before_store_scalar_local_alloca_candidate_module();
  const auto& scalar_read_first_function = scalar_read_first_module.functions.front();
  const auto scalar_read_first_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      scalar_read_first_function, regalloc, {});
  const auto scalar_read_first_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(scalar_read_first_function,
                                                          scalar_read_first_analysis);

  expect_true(scalar_read_first_plans.size() == 1 &&
                  scalar_read_first_plans.front().alloca_name == "%lv.x" &&
                  scalar_read_first_plans.front().needs_stack_slot &&
                  !scalar_read_first_plans.front().remove_following_entry_store,
              "shared slot-assignment planning should preserve paired zero-init stores for scalar entry allocas that are read before the first overwrite");

  const auto escaped_module = make_escaped_local_alloca_candidate_module();
  const auto& escaped_function = escaped_module.functions.back();
  const auto escaped_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(escaped_function, regalloc, {});
  const auto escaped_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(escaped_function,
                                                          escaped_analysis);

  expect_true(escaped_plans.size() == 1 &&
                  escaped_plans.front().alloca_name == "%lv.buf" &&
                  escaped_plans.front().needs_stack_slot &&
                  !escaped_plans.front().coalesced_block.has_value(),
              "shared slot-assignment planning should leave call-escaped local allocas out of the single-block reuse pool");
}

void test_backend_shared_slot_assignment_prunes_dead_entry_alloca_insts() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto dead_module = make_dead_local_alloca_candidate_module();
  const auto& dead_function = dead_module.functions.front();
  const auto dead_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(dead_function, regalloc, {});
  const auto dead_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(dead_function, dead_analysis);
  const auto dead_pruned =
      c4c::backend::stack_layout::prune_dead_entry_alloca_insts(dead_function, dead_plans);

  expect_true(dead_pruned.empty(),
              "shared slot-assignment pruning should drop dead non-param entry allocas and their paired zero-init stores");

  const auto live_module = make_live_local_alloca_candidate_module();
  const auto& live_function = live_module.functions.front();
  const auto live_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(live_function, regalloc, {});
  const auto live_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(live_function, live_analysis);
  const auto live_pruned =
      c4c::backend::stack_layout::prune_dead_entry_alloca_insts(live_function, live_plans);

  expect_true(live_pruned.size() == live_function.alloca_insts.size(),
              "shared slot-assignment pruning should keep aggregate live non-param entry allocas and their paired zero-init stores");

  const auto read_first_module = make_read_before_store_local_alloca_candidate_module();
  const auto& read_first_function = read_first_module.functions.front();
  const auto read_first_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(read_first_function, regalloc, {});
  const auto read_first_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(read_first_function,
                                                          read_first_analysis);
  const auto read_first_pruned = c4c::backend::stack_layout::prune_dead_entry_alloca_insts(
      read_first_function, read_first_plans);

  expect_true(read_first_pruned.size() == read_first_function.alloca_insts.size(),
              "shared slot-assignment pruning should preserve paired zero-init stores when a live entry alloca is read before the first overwrite");

  const auto scalar_overwrite_module = make_overwrite_first_scalar_local_alloca_candidate_module();
  const auto& scalar_overwrite_function = scalar_overwrite_module.functions.front();
  const auto scalar_overwrite_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(scalar_overwrite_function, regalloc, {});
  const auto scalar_overwrite_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(scalar_overwrite_function,
                                                          scalar_overwrite_analysis);
  const auto scalar_overwrite_pruned =
      c4c::backend::stack_layout::prune_dead_entry_alloca_insts(scalar_overwrite_function,
                                                                scalar_overwrite_plans);

  expect_true(
      scalar_overwrite_pruned.size() + 1 == scalar_overwrite_function.alloca_insts.size() &&
          std::holds_alternative<c4c::codegen::lir::LirAllocaOp>(
              scalar_overwrite_pruned.front()),
      "shared slot-assignment pruning should keep scalar live entry allocas while dropping redundant paired zero-init stores");
}

void test_backend_shared_slot_assignment_applies_coalesced_entry_slots() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto disjoint_module = make_disjoint_block_local_alloca_candidate_module();
  auto disjoint_function = disjoint_module.functions.front();
  const auto disjoint_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(disjoint_function, regalloc, {});
  const auto disjoint_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(disjoint_function,
                                                          disjoint_analysis);
  c4c::backend::stack_layout::apply_entry_alloca_slot_plan(disjoint_function, disjoint_plans);

  expect_true(disjoint_function.alloca_insts.size() == 1,
              "shared slot-assignment application should collapse disjoint single-block allocas onto one retained entry slot");
  const auto* disjoint_alloca =
      std::get_if<c4c::codegen::lir::LirAllocaOp>(&disjoint_function.alloca_insts.front());
  expect_true(disjoint_alloca != nullptr && disjoint_alloca->result == "%lv.then",
              "shared slot-assignment application should retain the first assigned-slot owner as the canonical entry alloca");
  const auto* else_store =
      std::get_if<c4c::codegen::lir::LirStoreOp>(&disjoint_function.blocks[2].insts[0]);
  const auto* else_load =
      std::get_if<c4c::codegen::lir::LirLoadOp>(&disjoint_function.blocks[2].insts[1]);
  expect_true(else_store != nullptr && else_store->ptr == "%lv.then" &&
                  else_load != nullptr && else_load->ptr == "%lv.then",
              "shared slot-assignment application should rewrite disjoint-block users to the canonical shared slot");

  const auto same_block_module = make_same_block_local_alloca_candidate_module();
  auto same_block_function = same_block_module.functions.front();
  const auto same_block_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(same_block_function, regalloc, {});
  const auto same_block_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(same_block_function,
                                                          same_block_analysis);
  c4c::backend::stack_layout::apply_entry_alloca_slot_plan(same_block_function, same_block_plans);

  expect_true(same_block_function.alloca_insts.size() == 2,
              "shared slot-assignment application should keep same-block allocas distinct when planning assigned them different slots");

  const auto call_arg_module = make_disjoint_block_call_arg_alloca_candidate_module();
  auto call_arg_function = call_arg_module.functions.front();
  const std::vector<c4c::backend::stack_layout::EntryAllocaSlotPlan> call_arg_plans = {
      c4c::backend::stack_layout::EntryAllocaSlotPlan{
          "%lv.then", true, false, std::nullopt, std::size_t{0}},
      c4c::backend::stack_layout::EntryAllocaSlotPlan{
          "%lv.else", true, false, std::nullopt, std::size_t{0}},
  };
  c4c::backend::stack_layout::apply_entry_alloca_slot_plan(call_arg_function,
                                                           call_arg_plans);

  const auto* rewritten_call =
      std::get_if<c4c::codegen::lir::LirCallOp>(&call_arg_function.blocks[2].insts[1]);
  expect_true(rewritten_call != nullptr && rewritten_call->args_str == "ptr %lv.then",
              "shared slot-assignment application should rewrite typed call arguments to the canonical coalesced entry alloca");
}

void test_aarch64_backend_prunes_dead_param_allocas_from_fallback_lir() {
  const auto dead_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_dead_param_alloca_candidate_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_true(dead_rendered.find("%lv.param.x = alloca i32, align 4") == std::string::npos,
              "aarch64 backend fallback should drop dead param allocas once shared slot planning proves the stack slot is unnecessary");
  expect_true(dead_rendered.find("store i32 %p.x, ptr %lv.param.x") == std::string::npos,
              "aarch64 backend fallback should also remove the dead param alloca initialization store");
  expect_contains(dead_rendered, "call i32 @helper(i32 %p.x)",
                  "aarch64 backend fallback should preserve the live function body after pruning dead param allocas");

  const auto live_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_live_param_alloca_candidate_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(live_rendered, "%lv.param.x = alloca i32, align 4",
                  "aarch64 backend fallback should keep live param allocas when the function body still reloads them");
  expect_contains(live_rendered, "store i32 %p.x, ptr %lv.param.x",
                  "aarch64 backend fallback should keep the param alloca initialization store for live slots");
}

void test_aarch64_backend_prunes_dead_local_allocas_from_fallback_lir() {
  const auto dead_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_dead_local_alloca_candidate_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_true(dead_rendered.find("%lv.buf = alloca [2 x i32], align 4") == std::string::npos,
              "aarch64 backend fallback should drop dead local entry allocas once shared slot planning proves the slot is unused");
  expect_true(
      dead_rendered.find("store [2 x i32] zeroinitializer, ptr %lv.buf") ==
          std::string::npos,
      "aarch64 backend fallback should also remove the paired zero-init store for dead local entry allocas");

  const auto live_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_live_local_alloca_candidate_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(live_rendered, "%lv.buf = alloca [2 x i32], align 4",
                  "aarch64 backend fallback should preserve live local entry allocas");
  expect_contains(live_rendered, "store [2 x i32] zeroinitializer, ptr %lv.buf",
                  "aarch64 backend fallback should preserve aggregate zero-init stores for live local entry allocas");

  const auto read_first_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_read_before_store_local_alloca_candidate_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(read_first_rendered, "%lv.buf = alloca [2 x i32], align 4",
                  "aarch64 backend fallback should preserve live local entry allocas in read-before-store cases");
  expect_contains(read_first_rendered, "store [2 x i32] zeroinitializer, ptr %lv.buf",
                  "aarch64 backend fallback should preserve zero-init stores when a live local entry alloca is read before the first overwrite");

  const auto disjoint_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_disjoint_block_local_alloca_candidate_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(disjoint_rendered, "%lv.then = alloca i32, align 4",
                  "aarch64 backend fallback should keep the canonical shared entry slot for disjoint-block allocas");
  expect_true(disjoint_rendered.find("%lv.else = alloca i32, align 4") == std::string::npos,
              "aarch64 backend fallback should drop duplicate entry allocas once shared slot planning coalesces them");
  expect_contains(disjoint_rendered, "store i32 9, ptr %lv.then",
                  "aarch64 backend fallback should rewrite disjoint-block users onto the retained shared slot");

  const auto same_block_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_same_block_local_alloca_candidate_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(same_block_rendered, "%lv.x = alloca i32, align 4",
                  "aarch64 backend fallback should keep the first same-block local alloca when slot planning keeps it distinct");
  expect_contains(same_block_rendered, "%lv.y = alloca i32, align 4",
                  "aarch64 backend fallback should also keep the second same-block local alloca when no shared slot is assigned");
}

void test_backend_binary_utils_contract_headers_are_include_reachable() {
  [[maybe_unused]] c4c::backend::elf::ContractSurface elf_surface;
  [[maybe_unused]] c4c::backend::linker_common::ContractSurface linker_common_surface;
  [[maybe_unused]] c4c::backend::aarch64::assembler::ContractSurface assembler_surface;
  [[maybe_unused]] c4c::backend::aarch64::linker::ContractSurface linker_surface;
  [[maybe_unused]] c4c::backend::x86::assembler::ContractSurface x86_assembler_surface;
  [[maybe_unused]] c4c::backend::x86::linker::ContractSurface x86_linker_surface;

  const auto emitted = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_return_add_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto parsed = c4c::backend::aarch64::assembler::parse_asm(emitted);
  expect_true(parsed.size() >= 4, "binary-utils contract headers should expose line-oriented parser output for backend-emitted assembly");
  expect_true(parsed.front().text == ".text",
              "binary-utils contract parser should keep the opening section directive");
  expect_true(parsed.back().text == "ret",
              "binary-utils contract parser should preserve the backend instruction tail");

  const auto object_path = make_temp_output_path("c4c_aarch64_contract");
  const auto staged = c4c::backend::aarch64::assembler::assemble(
      c4c::backend::aarch64::assembler::AssembleRequest{
          .asm_text = emitted,
          .output_path = object_path,
      });
  expect_true(staged.staged_text == emitted && staged.output_path == object_path &&
                  staged.object_emitted,
              "binary-utils contract headers should expose the active assembler request/result seam and report successful object emission for the minimal backend slice");
  const auto object_bytes = read_file_bytes(object_path);
  expect_true(object_bytes.size() >= 4 && object_bytes[0] == 0x7f &&
                  object_bytes[1] == 'E' && object_bytes[2] == 'L' &&
                  object_bytes[3] == 'F',
              "binary-utils contract assembler seam should emit an ELF object for the minimal backend slice");
  expect_true(std::string(object_bytes.begin(), object_bytes.end()).find("main") != std::string::npos,
              "binary-utils contract assembler seam should preserve the function symbol in the emitted object");
  std::filesystem::remove(object_path);

  const auto compat_path = make_temp_output_path("c4c_aarch64_contract_compat");
  const auto assembled = c4c::backend::aarch64::assembler::assemble(emitted, compat_path);
  expect_true(assembled == emitted,
              "binary-utils contract headers should keep the compatibility overload aligned with the staged assembler text seam");
  expect_true(std::filesystem::exists(compat_path),
              "binary-utils contract compatibility overload should still drive object emission through the named assembler seam");
  std::filesystem::remove(compat_path);
}

void test_shared_linker_parses_minimal_relocation_object_fixture() {
  const auto fixture = make_minimal_relocation_object_fixture();
  std::string error;
  const auto parsed = c4c::backend::linker_common::parse_elf64_object(
      fixture, "minimal_reloc.o", c4c::backend::elf::EM_AARCH64, &error);
  expect_true(parsed.has_value(),
              "shared linker object parser should accept the bounded relocation-bearing fixture: " +
                  error);

  const auto& object = *parsed;
  expect_true(object.source_name == "minimal_reloc.o",
              "shared linker object parser should preserve the source name");
  expect_true(object.sections.size() == 6,
              "shared linker object parser should preserve the full section inventory including the null section");
  expect_true(object.symbols.size() == 4,
              "shared linker object parser should preserve the full symbol inventory for the bounded object slice");
  expect_true(object.relocations.size() == object.sections.size(),
              "shared linker object parser should index relocation vectors by section");

  const auto text_index = find_section_index(object, ".text");
  const auto rela_text_index = find_section_index(object, ".rela.text");
  const auto symtab_index = find_section_index(object, ".symtab");
  const auto strtab_index = find_section_index(object, ".strtab");
  const auto shstrtab_index = find_section_index(object, ".shstrtab");
  expect_true(text_index < object.sections.size() && rela_text_index < object.sections.size() &&
                  symtab_index < object.sections.size() && strtab_index < object.sections.size() &&
                  shstrtab_index < object.sections.size(),
              "shared linker object parser should name the expected bounded ELF sections");

  expect_true(object.section_data[text_index].size() == 8,
              "shared linker object parser should preserve the bounded .text payload");
  expect_true(object.section_data[rela_text_index].size() == 24,
              "shared linker object parser should preserve the raw RELA payload for the bounded fixture");
  expect_true(object.symbols[2].name == "main" && object.symbols[2].is_global() &&
                  object.symbols[2].sym_type() == c4c::backend::elf::STT_FUNC &&
                  object.symbols[2].shndx == text_index,
              "shared linker object parser should preserve the bounded function symbol inventory");
  expect_true(object.symbols[3].name == "helper_ext" && object.symbols[3].is_undefined(),
              "shared linker object parser should preserve undefined extern symbols for later linker resolution");

  expect_true(object.relocations[text_index].size() == 1,
              "shared linker object parser should associate .rela.text entries with the .text section they target");
  const auto& relocation = object.relocations[text_index].front();
  expect_true(relocation.offset == 4 && relocation.sym_idx == 3 &&
                  relocation.rela_type == 279 && relocation.addend == 0,
              "shared linker object parser should preserve the bounded relocation inventory for later linker work");
}

void test_shared_linker_parses_builtin_return_add_object() {
  const auto object_path = make_temp_output_path("c4c_aarch64_return_add_parse");
  const auto staged = c4c::backend::aarch64::assemble_module(make_return_add_module(), object_path);
  expect_true(staged.object_emitted,
              "built-in assembler should emit an object for the current bounded return-add slice");

  const auto object_bytes = read_file_bytes(object_path);
  std::string error;
  const auto parsed = c4c::backend::linker_common::parse_elf64_object(
      object_bytes, object_path, c4c::backend::elf::EM_AARCH64, &error);
  expect_true(parsed.has_value(),
              "shared linker object parser should accept the current built-in return-add object: " +
                  error);

  const auto& object = *parsed;
  const auto text_index = find_section_index(object, ".text");
  const auto symtab_index = find_section_index(object, ".symtab");
  const auto strtab_index = find_section_index(object, ".strtab");
  const auto shstrtab_index = find_section_index(object, ".shstrtab");
  expect_true(text_index < object.sections.size() && symtab_index < object.sections.size() &&
                  strtab_index < object.sections.size() &&
                  shstrtab_index < object.sections.size(),
              "shared linker object parser should preserve the current built-in return-add section inventory");
  expect_true(object.relocations.size() == object.sections.size(),
              "shared linker object parser should keep relocation vectors indexed by section for built-in emitted objects");
  expect_true(object.section_data[text_index].size() == 8,
              "shared linker object parser should preserve the built-in emitted return-add text payload");

  const auto main_symbol = std::find_if(
      object.symbols.begin(), object.symbols.end(),
      [&](const c4c::backend::linker_common::Elf64Symbol& symbol) {
        return symbol.name == "main";
      });
  expect_true(main_symbol != object.symbols.end() && main_symbol->is_global() &&
                  main_symbol->sym_type() == c4c::backend::elf::STT_FUNC &&
                  main_symbol->shndx == text_index,
              "shared linker object parser should preserve the built-in emitted function symbol inventory");

  expect_true(object.relocations[text_index].empty(),
              "shared linker object parser should preserve the current built-in return-add object as relocation-free");

  std::filesystem::remove(object_path);
}

void test_shared_linker_parses_single_member_archive_fixture() {
  const auto fixture = make_single_member_archive_fixture();
  std::string error;
  const auto parsed = c4c::backend::linker_common::parse_elf64_archive(
      fixture, "libminimal.a", c4c::backend::elf::EM_AARCH64, &error);
  expect_true(parsed.has_value(),
              "shared linker archive parser should accept the bounded single-member fixture: " +
                  error);

  const auto& archive = *parsed;
  expect_true(archive.source_name == "libminimal.a",
              "shared linker archive parser should preserve the archive source name");
  expect_true(archive.members.size() == 1,
              "shared linker archive parser should preserve the bounded single-member inventory");
  expect_true(archive.find_member_index_for_symbol("main").has_value(),
              "shared linker archive parser should support symbol-driven member lookup for the bounded archive slice");

  const auto member_index = *archive.find_member_index_for_symbol("main");
  const auto& member = archive.members[member_index];
  expect_true(member.name == "minimal_reloc.o",
              "shared linker archive parser should normalize the bounded member name");
  expect_true(member.object.has_value(),
              "shared linker archive parser should parse the bounded ELF member into a shared object surface");
  expect_true(member.defines_symbol("main") && !member.defines_symbol("helper_ext"),
              "shared linker archive parser should index defined symbols without treating unresolved externs as providers");

  const auto& object = *member.object;
  const auto text_index = find_section_index(object, ".text");
  expect_true(text_index < object.sections.size(),
              "shared linker archive parser should preserve section names for the parsed member object");
  expect_true(object.relocations[text_index].size() == 1,
              "shared linker archive parser should preserve relocation inventory for the bounded archive member");
}

void test_aarch64_linker_names_first_static_call26_slice() {
  const auto caller_path = make_temp_path("c4c_aarch64_linker_caller", ".o");
  const auto helper_path = make_temp_path("c4c_aarch64_linker_helper", ".o");
  write_binary_file(caller_path, make_minimal_relocation_object_fixture());
  write_binary_file(helper_path, make_minimal_helper_definition_object_fixture());

  std::string error;
  const auto slice = c4c::backend::aarch64::linker::inspect_first_static_link_slice(
      {caller_path, helper_path}, &error);
  expect_true(slice.has_value(),
              "aarch64 linker should load the first bounded static-link slice through shared object parsing: " +
                  error);

  expect_true(slice->case_name == "aarch64-static-call26-two-object",
              "aarch64 linker should name the first bounded static-link slice explicitly");
  expect_true(slice->objects.size() == 2,
              "aarch64 linker should keep the first static-link slice scoped to two explicit input objects");
  expect_true(slice->objects[0].defined_symbols.size() == 1 &&
                  slice->objects[0].defined_symbols[0] == "main" &&
                  slice->objects[0].undefined_symbols.size() == 1 &&
                  slice->objects[0].undefined_symbols[0] == "helper_ext",
              "aarch64 linker should record that the caller object defines main and imports helper_ext for the first slice");
  expect_true(slice->objects[1].defined_symbols.size() == 1 &&
                  slice->objects[1].defined_symbols[0] == "helper_ext" &&
                  slice->objects[1].undefined_symbols.empty(),
              "aarch64 linker should record that the helper object provides the only external definition needed by the first slice");

  expect_true(slice->relocations.size() == 1,
              "aarch64 linker should preserve the single relocation-bearing edge for the first static-link slice");
  const auto& relocation = slice->relocations.front();
  expect_true(relocation.object_path == caller_path && relocation.section_name == ".text" &&
                  relocation.symbol_name == "helper_ext" && relocation.relocation_type == 279 &&
                  relocation.offset == 4 && relocation.addend == 0 && relocation.resolved &&
                  relocation.resolved_object_path == helper_path,
              "aarch64 linker should record the first slice as one resolved .text CALL26 edge from main to helper_ext");

  expect_true(slice->merged_output_sections.size() == 1 &&
                  slice->merged_output_sections[0] == ".text",
              "aarch64 linker should record that the first static-link slice only needs a merged .text output section");
  expect_true(slice->unresolved_symbols.empty(),
              "aarch64 linker should show no unresolved globals once the helper object is present");

  std::filesystem::remove(caller_path);
  std::filesystem::remove(helper_path);
}

void test_aarch64_linker_loads_first_static_objects_through_shared_input_seam() {
  const auto caller_path = make_temp_path("c4c_aarch64_linker_input_caller", ".o");
  const auto helper_path = make_temp_path("c4c_aarch64_linker_input_helper", ".o");
  write_binary_file(caller_path, make_minimal_relocation_object_fixture());
  write_binary_file(helper_path, make_minimal_helper_definition_object_fixture());

  std::string error;
  const auto loaded = c4c::backend::aarch64::linker::load_first_static_input_objects(
      {caller_path, helper_path}, &error);
  expect_true(loaded.has_value(),
              "aarch64 linker input seam should load the bounded two-object CALL26 slice through shared ELF parsing: " +
                  error);
  expect_true(loaded->size() == 2,
              "aarch64 linker input seam should preserve the first static-link slice as two ordered object inputs");
  expect_true((*loaded)[0].path == caller_path &&
                  (*loaded)[0].object.source_name == caller_path &&
                  (*loaded)[0].object.symbols.size() >= 3,
              "aarch64 linker input seam should preserve caller object path and parsed symbol inventory");
  expect_true((*loaded)[1].path == helper_path &&
                  (*loaded)[1].object.source_name == helper_path &&
                  (*loaded)[1].object.symbols.size() >= 2,
              "aarch64 linker input seam should preserve helper object path and parsed symbol inventory");

  const auto caller_text_index = find_section_index((*loaded)[0].object, ".text");
  expect_true(caller_text_index < (*loaded)[0].object.sections.size() &&
                  (*loaded)[0].object.relocations[caller_text_index].size() == 1,
              "aarch64 linker input seam should preserve the caller .text relocation inventory for the bounded CALL26 slice");

  std::filesystem::remove(caller_path);
  std::filesystem::remove(helper_path);
}

void test_aarch64_linker_loads_first_static_objects_from_archive_through_shared_input_seam() {
  const auto caller_path = make_temp_path("c4c_aarch64_linker_archive_caller", ".o");
  const auto helper_archive_path = make_temp_path("c4c_aarch64_linker_helper", ".a");
  write_binary_file(caller_path, make_minimal_relocation_object_fixture());
  write_binary_file(helper_archive_path,
                    make_single_member_archive_fixture(
                        make_minimal_helper_definition_object_fixture(), "helper_def.o/"));

  std::string error;
  const auto loaded = c4c::backend::aarch64::linker::load_first_static_input_objects(
      {caller_path, helper_archive_path}, &error);
  expect_true(loaded.has_value(),
              "aarch64 linker input seam should load the bounded CALL26 slice when the helper definition comes from a shared-parsed archive: " +
                  error);
  expect_true(loaded->size() == 2,
              "aarch64 linker input seam should resolve the bounded archive-backed slice into the same two loaded object surfaces");
  expect_true((*loaded)[0].path == caller_path && (*loaded)[0].object.source_name == caller_path,
              "aarch64 linker input seam should preserve the caller object identity when archive loading is enabled");
  expect_true((*loaded)[1].path == helper_archive_path + "(helper_def.o)" &&
                  (*loaded)[1].object.source_name == helper_archive_path + "(helper_def.o)",
              "aarch64 linker input seam should surface the selected archive member as the helper provider");

  const auto caller_text_index = find_section_index((*loaded)[0].object, ".text");
  expect_true(caller_text_index < (*loaded)[0].object.sections.size() &&
                  (*loaded)[0].object.relocations[caller_text_index].size() == 1 &&
                  (*loaded)[0].object.relocations[caller_text_index][0].sym_idx <
                      (*loaded)[0].object.symbols.size() &&
                  (*loaded)[0].object.symbols[(*loaded)[0].object.relocations[caller_text_index][0].sym_idx]
                          .name == "helper_ext",
              "aarch64 linker input seam should keep the caller relocation pointed at helper_ext before the archive member is linked");
  expect_true((*loaded)[1].object.symbols.size() >= 2,
              "aarch64 linker input seam should parse the selected helper archive member into the shared object surface");

  std::filesystem::remove(caller_path);
  std::filesystem::remove(helper_archive_path);
}

void test_x86_linker_names_first_static_plt32_slice() {
  const auto caller_path = make_temp_path("c4c_x86_linker_caller", ".o");
  const auto helper_path = make_temp_path("c4c_x86_linker_helper", ".o");
  write_binary_file(caller_path, make_minimal_x86_relocation_object_fixture());
  write_binary_file(helper_path, make_minimal_x86_helper_definition_object_fixture());

  std::string error;
  const auto slice = c4c::backend::x86::linker::inspect_first_static_link_slice(
      {caller_path, helper_path}, &error);
  expect_true(slice.has_value(),
              "x86 linker should load the first bounded static-link slice through shared object parsing: " +
                  error);

  expect_true(slice->case_name == "x86_64-static-plt32-two-object",
              "x86 linker should name the first bounded static-link slice explicitly");
  expect_true(slice->objects.size() == 2,
              "x86 linker should keep the first static-link slice scoped to two explicit input objects");
  expect_true(slice->objects[0].defined_symbols.size() == 1 &&
                  slice->objects[0].defined_symbols[0] == "main" &&
                  slice->objects[0].undefined_symbols.size() == 1 &&
                  slice->objects[0].undefined_symbols[0] == "helper_ext",
              "x86 linker should record that the caller object defines main and imports helper_ext for the first slice");
  expect_true(slice->objects[1].defined_symbols.size() == 1 &&
                  slice->objects[1].defined_symbols[0] == "helper_ext" &&
                  slice->objects[1].undefined_symbols.empty(),
              "x86 linker should record that the helper object provides the only external definition needed by the first slice");

  expect_true(slice->relocations.size() == 1,
              "x86 linker should preserve the single relocation-bearing edge for the first static-link slice");
  const auto& relocation = slice->relocations.front();
  expect_true(relocation.object_path == caller_path && relocation.section_name == ".text" &&
                  relocation.symbol_name == "helper_ext" && relocation.relocation_type == 4 &&
                  relocation.offset == 1 && relocation.addend == -4 && relocation.resolved &&
                  relocation.resolved_object_path == helper_path,
              "x86 linker should record the first slice as one resolved .text PLT32 edge from main to helper_ext");

  expect_true(slice->merged_output_sections.size() == 1 &&
                  slice->merged_output_sections[0] == ".text",
              "x86 linker should record that the first static-link slice only needs a merged .text output section");
  expect_true(slice->unresolved_symbols.empty(),
              "x86 linker should show no unresolved globals once the helper object is present");

  std::filesystem::remove(caller_path);
  std::filesystem::remove(helper_path);
}

void test_x86_linker_loads_first_static_objects_through_shared_input_seam() {
  const auto caller_path = make_temp_path("c4c_x86_linker_input_caller", ".o");
  const auto helper_path = make_temp_path("c4c_x86_linker_input_helper", ".o");
  write_binary_file(caller_path, make_minimal_x86_relocation_object_fixture());
  write_binary_file(helper_path, make_minimal_x86_helper_definition_object_fixture());

  std::string error;
  const auto loaded = c4c::backend::x86::linker::load_first_static_input_objects(
      {caller_path, helper_path}, &error);
  expect_true(loaded.has_value(),
              "x86 linker input seam should load the bounded two-object PLT32 slice through shared ELF parsing: " +
                  error);
  expect_true(loaded->size() == 2,
              "x86 linker input seam should preserve the first static-link slice as two ordered object inputs");
  expect_true((*loaded)[0].path == caller_path &&
                  (*loaded)[0].object.source_name == caller_path &&
                  (*loaded)[0].object.symbols.size() >= 3,
              "x86 linker input seam should preserve caller object path and parsed symbol inventory");
  expect_true((*loaded)[1].path == helper_path &&
                  (*loaded)[1].object.source_name == helper_path &&
                  (*loaded)[1].object.symbols.size() >= 2,
              "x86 linker input seam should preserve helper object path and parsed symbol inventory");

  const auto caller_text_index = find_section_index((*loaded)[0].object, ".text");
  expect_true(caller_text_index < (*loaded)[0].object.sections.size() &&
                  (*loaded)[0].object.relocations[caller_text_index].size() == 1,
              "x86 linker input seam should preserve the caller .text relocation inventory for the bounded PLT32 slice");

  std::filesystem::remove(caller_path);
  std::filesystem::remove(helper_path);
}

void test_x86_linker_loads_first_static_objects_from_archive_through_shared_input_seam() {
  const auto caller_path = make_temp_path("c4c_x86_linker_archive_caller", ".o");
  const auto helper_archive_path = make_temp_path("c4c_x86_linker_helper", ".a");
  write_binary_file(caller_path, make_minimal_x86_relocation_object_fixture());
  write_binary_file(helper_archive_path,
                    make_single_member_archive_fixture(
                        make_minimal_x86_helper_definition_object_fixture(), "helper_def.o/"));

  std::string error;
  const auto loaded = c4c::backend::x86::linker::load_first_static_input_objects(
      {caller_path, helper_archive_path}, &error);
  expect_true(loaded.has_value(),
              "x86 linker input seam should load the bounded PLT32 slice when the helper definition comes from a shared-parsed archive: " +
                  error);
  expect_true(loaded->size() == 2,
              "x86 linker input seam should resolve the bounded archive-backed slice into the same two loaded object surfaces");
  expect_true((*loaded)[0].path == caller_path && (*loaded)[0].object.source_name == caller_path,
              "x86 linker input seam should preserve the caller object identity when archive loading is enabled");
  expect_true((*loaded)[1].path == helper_archive_path + "(helper_def.o)" &&
                  (*loaded)[1].object.source_name == helper_archive_path + "(helper_def.o)",
              "x86 linker input seam should surface the selected archive member as the helper provider");

  const auto caller_text_index = find_section_index((*loaded)[0].object, ".text");
  expect_true(caller_text_index < (*loaded)[0].object.sections.size() &&
                  (*loaded)[0].object.relocations[caller_text_index].size() == 1 &&
                  (*loaded)[0].object.relocations[caller_text_index][0].sym_idx <
                      (*loaded)[0].object.symbols.size() &&
                  (*loaded)[0].object.symbols[(*loaded)[0].object.relocations[caller_text_index][0].sym_idx]
                          .name == "helper_ext",
              "x86 linker input seam should keep the caller relocation pointed at helper_ext before the archive member is linked");
  expect_true((*loaded)[1].object.symbols.size() >= 2,
              "x86 linker input seam should parse the selected helper archive member into the shared object surface");

  std::filesystem::remove(caller_path);
  std::filesystem::remove(helper_archive_path);
}

void test_x86_linker_emits_first_static_plt32_executable_slice() {
  const auto caller_path = make_temp_path("c4c_x86_link_exec_caller", ".o");
  const auto helper_path = make_temp_path("c4c_x86_link_exec_helper", ".o");
  write_binary_file(caller_path, make_minimal_x86_relocation_object_fixture());
  write_binary_file(helper_path, make_minimal_x86_helper_definition_object_fixture());

  std::string error;
  const auto executable = c4c::backend::x86::linker::link_first_static_executable(
      {caller_path, helper_path}, &error);
  expect_true(executable.has_value(),
              "x86 linker should link the first static PLT32 slice into a bounded executable image: " +
                  error);

  expect_true(executable->image.size() >= executable->text_file_offset + executable->text_size,
              "x86 linker should emit enough bytes to cover the merged .text image");
  expect_true(executable->text_size == 12,
              "x86 linker should merge the bounded caller/helper .text slices into one 12-byte executable surface");
  expect_true(executable->entry_address == executable->text_virtual_address,
              "x86 linker should use the merged main symbol as the executable entry point");
  expect_true(executable->symbol_addresses.find("main") != executable->symbol_addresses.end() &&
                  executable->symbol_addresses.find("helper_ext") != executable->symbol_addresses.end() &&
                  executable->symbol_addresses.at("main") == executable->text_virtual_address &&
                  executable->symbol_addresses.at("helper_ext") ==
                      executable->text_virtual_address + 6,
              "x86 linker should assign stable merged .text addresses to the bounded main/helper symbols");

  const auto& image = executable->image;
  expect_true(image[0] == 0x7f && image[1] == 'E' && image[2] == 'L' && image[3] == 'F',
              "x86 linker should emit an ELF header for the first static executable slice");
  expect_true(read_u16(image, 16) == 2 && read_u16(image, 18) == c4c::backend::elf::EM_X86_64,
              "x86 linker should mark the first emitted image as an x86-64 ET_EXEC executable");
  expect_true(read_u64(image, 24) == executable->entry_address,
              "x86 linker should write the merged main address into the executable entry field");
  expect_true(read_u64(image, 32) == 64 && read_u16(image, 56) == 1,
              "x86 linker should emit one bounded program header for the first executable slice");

  expect_true(image[executable->text_file_offset + 0] == 0xe8 &&
                  read_u32(image, executable->text_file_offset + 1) == 1 &&
                  image[executable->text_file_offset + 5] == 0xc3 &&
                  image[executable->text_file_offset + 6] == 0xb8 &&
                  read_u32(image, executable->text_file_offset + 7) == 42 &&
                  image[executable->text_file_offset + 11] == 0xc3,
              "x86 linker should patch the bounded PLT32 call and preserve the merged helper instructions in executable order");

  std::filesystem::remove(caller_path);
  std::filesystem::remove(helper_path);
}

void test_x86_linker_matches_external_first_static_executable_slice() {
#if defined(C4C_TEST_CLANG_PATH)
  const auto caller_path = make_temp_path("c4c_x86_link_validate_caller", ".o");
  const auto helper_path = make_temp_path("c4c_x86_link_validate_helper", ".o");
  const auto external_executable_path = make_temp_path("c4c_x86_link_validate_external", ".out");
  write_binary_file(caller_path, make_minimal_x86_relocation_object_fixture());
  write_binary_file(helper_path, make_minimal_x86_helper_definition_object_fixture());

  std::string error;
  const auto builtin_executable = c4c::backend::x86::linker::link_first_static_executable(
      {caller_path, helper_path}, &error);
  expect_true(builtin_executable.has_value(),
              "x86 linker should produce the bounded built-in executable before external validation: " +
                  error);

  run_command_capture(shell_quote(C4C_TEST_CLANG_PATH) +
                      " -nostdlib -static -no-pie -Wl,-e,main -Wl,--build-id=none " +
                      shell_quote(caller_path) + " " + shell_quote(helper_path) + " -o " +
                      shell_quote(external_executable_path) + " 2>&1");

  const auto external_image = read_file_bytes(external_executable_path);
  expect_true(read_u16(external_image, 16) == 2 &&
                  read_u16(external_image, 18) == c4c::backend::elf::EM_X86_64,
              "external linker baseline should emit an x86-64 ET_EXEC image for the bounded slice");

  const auto builtin_entry_bytes =
      read_elf_entry_bytes(builtin_executable->image, builtin_executable->text_size);
  const auto external_entry_bytes =
      read_elf_entry_bytes(external_image, builtin_executable->text_size);
  expect_true(builtin_entry_bytes == external_entry_bytes,
              "x86 built-in linker entry bytes should match the external linker baseline for the bounded main -> helper_ext slice");

#if defined(__x86_64__)
  expect_true(execute_x86_64_entry_bytes(builtin_entry_bytes) == 42,
              "x86 built-in linker entry bytes should execute the bounded main -> helper_ext slice and return 42");
  expect_true(execute_x86_64_entry_bytes(external_entry_bytes) == 42,
              "external linker baseline entry bytes should execute the bounded main -> helper_ext slice and return 42");
#endif

  std::filesystem::remove(caller_path);
  std::filesystem::remove(helper_path);
  std::filesystem::remove(external_executable_path);
#endif
}

void test_aarch64_linker_emits_first_static_call26_executable_slice() {
  const auto caller_path = make_temp_path("c4c_aarch64_link_exec_caller", ".o");
  const auto helper_path = make_temp_path("c4c_aarch64_link_exec_helper", ".o");
  write_binary_file(caller_path, make_minimal_relocation_object_fixture());
  write_binary_file(helper_path, make_minimal_helper_definition_object_fixture());

  std::string error;
  const auto executable = c4c::backend::aarch64::linker::link_first_static_executable(
      {caller_path, helper_path}, &error);
  expect_true(executable.has_value(),
              "aarch64 linker should link the first static CALL26 slice into a bounded executable image: " +
                  error);

  expect_true(executable->image.size() >= executable->text_file_offset + executable->text_size,
              "aarch64 linker should emit enough bytes to cover the merged .text image");
  expect_true(executable->text_size == 16,
              "aarch64 linker should merge the bounded caller/helper .text slices into one 16-byte executable surface");
  expect_true(executable->entry_address == executable->text_virtual_address,
              "aarch64 linker should use the merged main symbol as the executable entry point");
  expect_true(executable->symbol_addresses.find("main") != executable->symbol_addresses.end() &&
                  executable->symbol_addresses.find("helper_ext") != executable->symbol_addresses.end() &&
                  executable->symbol_addresses.at("main") == executable->text_virtual_address &&
                  executable->symbol_addresses.at("helper_ext") ==
                      executable->text_virtual_address + 8,
              "aarch64 linker should assign stable merged .text addresses to the bounded main/helper symbols");

  const auto& image = executable->image;
  expect_true(image[0] == 0x7f && image[1] == 'E' && image[2] == 'L' && image[3] == 'F',
              "aarch64 linker should emit an ELF header for the first static executable slice");
  expect_true(read_u16(image, 16) == 2 && read_u16(image, 18) == c4c::backend::elf::EM_AARCH64,
              "aarch64 linker should mark the first emitted image as an AArch64 ET_EXEC executable");
  expect_true(read_u64(image, 24) == executable->entry_address,
              "aarch64 linker should write the merged main address into the executable entry field");
  expect_true(read_u64(image, 32) == 64 && read_u16(image, 56) == 1,
              "aarch64 linker should emit one bounded program header for the first executable slice");

  expect_true(read_u32(image, executable->text_file_offset + 0) == 0xd503201f &&
                  read_u32(image, executable->text_file_offset + 4) == 0x94000001 &&
                  read_u32(image, executable->text_file_offset + 8) == 0x52800540 &&
                  read_u32(image, executable->text_file_offset + 12) == 0xd65f03c0,
              "aarch64 linker should patch the bounded CALL26 branch and preserve the merged helper instructions in executable order");

  std::filesystem::remove(caller_path);
  std::filesystem::remove(helper_path);
}

void test_aarch64_backend_assembler_handoff_helper_stages_emitted_text() {
  const auto output_path = make_temp_output_path("c4c_aarch64_handoff");
  const auto staged = c4c::backend::aarch64::assemble_module(make_return_add_module(), output_path);

  expect_true(staged.staged_text ==
                  c4c::backend::emit_module(
                      c4c::backend::BackendModuleInput{make_return_add_module()},
                      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64}),
              "aarch64 backend handoff helper should route production backend text through the staged assembler seam");
  expect_true(staged.output_path == output_path && staged.object_emitted,
              "aarch64 backend handoff helper should preserve output-path metadata and report successful object emission for the minimal backend slice");
  expect_true(std::filesystem::exists(output_path),
              "aarch64 backend handoff helper should write the assembled object to the requested output path");
  std::filesystem::remove(output_path);
}

void test_x86_backend_assembler_handoff_helper_stages_emitted_text() {
  const auto output_path = make_temp_output_path("c4c_x86_handoff");
  const auto staged = c4c::backend::x86::assemble_module(make_return_add_module(), output_path);

  expect_true(staged.staged_text ==
                  c4c::backend::emit_module(
                      c4c::backend::BackendModuleInput{make_return_add_module()},
                      c4c::backend::BackendOptions{c4c::backend::Target::X86_64}),
              "x86 backend handoff helper should route production backend text through the staged assembler seam");
  expect_true(staged.output_path == output_path && staged.object_emitted,
              "x86 backend handoff helper should preserve output-path metadata and report successful object emission for the minimal backend slice");
  expect_true(std::filesystem::exists(output_path),
              "x86 backend handoff helper should write the assembled object to the requested output path");
  std::filesystem::remove(output_path);
}

void test_aarch64_builtin_object_matches_external_return_add_surface() {
#if defined(C4C_TEST_CLANG_PATH) && defined(C4C_TEST_OBJDUMP_PATH)
  auto module = make_return_add_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  const auto asm_text = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto asm_path = make_temp_path("c4c_aarch64_return_add_surface", ".s");
  const auto builtin_object_path = make_temp_output_path("c4c_aarch64_return_add_builtin");
  const auto external_object_path = make_temp_output_path("c4c_aarch64_return_add_external");

  write_text_file(asm_path, asm_text);
  const auto builtin = c4c::backend::aarch64::assembler::assemble(
      c4c::backend::aarch64::assembler::AssembleRequest{
          .asm_text = asm_text,
          .output_path = builtin_object_path,
      });
  expect_true(builtin.object_emitted,
              "built-in assembler should emit an object for the bounded return_add slice");

  run_command_capture(shell_quote(C4C_TEST_CLANG_PATH) +
                      " --target=aarch64-unknown-linux-gnu -c " +
                      shell_quote(asm_path) + " -o " +
                      shell_quote(external_object_path) + " 2>&1");

  const auto builtin_objdump = normalize_objdump_surface(
      run_command_capture(shell_quote(C4C_TEST_OBJDUMP_PATH) + " -h -r -t " +
                          shell_quote(builtin_object_path) + " 2>&1"));
  const auto external_objdump = normalize_objdump_surface(
      run_command_capture(shell_quote(C4C_TEST_OBJDUMP_PATH) + " -h -r -t " +
                          shell_quote(external_object_path) + " 2>&1"));
  expect_contains(builtin_objdump, ".text         00000008",
                  "built-in assembler should emit the same bounded .text size as the external baseline");
  expect_contains(external_objdump, ".text         00000008",
                  "external assembler baseline should keep the bounded .text size for return_add");
  expect_not_contains(builtin_objdump, "RELOCATION RECORDS",
                      "built-in assembler should not introduce relocations for the bounded return_add slice");
  expect_not_contains(external_objdump, "RELOCATION RECORDS",
                      "external assembler baseline should not need relocations for the bounded return_add slice");
  expect_contains(builtin_objdump, "g     F .text",
                  "built-in assembler should expose a global function symbol in .text");
  expect_contains(builtin_objdump, "main",
                  "built-in assembler should preserve the main symbol name");
  expect_contains(external_objdump, "g     F .text",
                  "external assembler baseline should expose a global function symbol in .text");
  expect_contains(external_objdump, "main",
                  "external assembler baseline should preserve the main symbol name");

  const auto builtin_disasm = normalize_objdump_disassembly(
      run_command_capture(shell_quote(C4C_TEST_OBJDUMP_PATH) + " -d " +
                          shell_quote(builtin_object_path) + " 2>&1"));
  const auto external_disasm = normalize_objdump_disassembly(
      run_command_capture(shell_quote(C4C_TEST_OBJDUMP_PATH) + " -d " +
                          shell_quote(external_object_path) + " 2>&1"));
  expect_true(builtin_disasm == external_disasm,
              "built-in assembler disassembly should match the external assembler baseline for return_add\n--- built-in ---\n" +
                  builtin_disasm + "--- external ---\n" + external_disasm);

  std::filesystem::remove(asm_path);
  std::filesystem::remove(builtin_object_path);
  std::filesystem::remove(external_object_path);
#endif
}

}  // namespace

int main() {
  test_lir_typed_wrappers_classify_basic_operands();
  test_lir_typed_wrappers_classify_basic_types();
  test_lir_call_arg_helpers_split_nested_typed_args();
  test_lir_call_arg_helpers_collect_value_names();
  test_lir_call_arg_helpers_collect_full_call_value_names();
  test_lir_call_arg_helpers_rewrite_full_call_operands();
  test_lir_call_arg_helpers_decode_single_typed_operand();
  test_lir_call_arg_helpers_decode_two_typed_operands();
  test_lir_call_arg_helpers_classify_call_callee_shape();
  test_lir_call_arg_helpers_decode_direct_global_typed_call();
  test_lir_call_arg_helpers_decode_zero_arg_and_call_crossing_direct_calls();
  test_lir_call_arg_helpers_format_typed_call_round_trip();
  test_lir_call_arg_helpers_format_full_call_site_round_trip();
  test_lir_call_arg_helpers_format_call_fields_with_suffix();
  test_lir_call_arg_helpers_make_typed_call_op();
  test_lir_call_arg_helpers_make_direct_call_op_without_suffix();
  test_backend_call_helpers_decode_structured_direct_global_call();
  test_lir_typed_wrappers_preserve_legacy_opcode_and_predicate_strings();
  test_lir_typed_wrappers_leave_unknown_opcode_text_compatible();
  test_lir_printer_renders_verified_typed_ops();
  test_lir_printer_canonicalizes_typed_call_surface();
  test_lir_printer_rejects_malformed_typed_binary_surface();
  test_adapts_direct_return();
  test_renders_return_add();
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
  test_rejects_unsupported_instruction();
  test_aarch64_backend_scaffold_renders_supported_slice();
  test_aarch64_backend_scaffold_renders_direct_return_immediate_slice();
  test_x86_backend_scaffold_routes_through_explicit_emit_surface();
  test_x86_backend_scaffold_renders_direct_return_immediate_slice();
  test_x86_backend_scaffold_renders_direct_return_sub_immediate_slice();
  test_x86_backend_renders_local_temp_sub_slice();
  test_x86_backend_renders_local_temp_arithmetic_chain_slice();
  test_x86_backend_renders_two_local_temp_return_slice();
  test_x86_backend_renders_local_pointer_temp_return_slice();
  test_x86_backend_renders_double_indirect_local_pointer_conditional_return_slice();
  test_x86_backend_renders_goto_only_constant_return_slice();
  test_x86_backend_renders_countdown_while_return_slice();
  test_x86_backend_renders_countdown_do_while_return_slice();
  test_x86_backend_renders_direct_call_slice();
  test_x86_backend_renders_zero_arg_typed_direct_call_slice_with_whitespace();
  test_x86_backend_rejects_intrinsic_callee_from_direct_call_fast_path();
  test_x86_backend_rejects_indirect_callee_from_direct_call_fast_path();
  test_x86_backend_renders_param_slot_slice();
  test_x86_backend_renders_typed_direct_call_local_arg_slice();
  test_x86_backend_renders_typed_direct_call_local_arg_spacing_slice();
  test_x86_backend_renders_typed_two_arg_direct_call_slice();
  test_x86_backend_renders_typed_two_arg_direct_call_local_arg_slice();
  test_x86_backend_renders_typed_two_arg_direct_call_second_local_arg_slice();
  test_x86_backend_renders_typed_two_arg_direct_call_second_local_rewrite_slice();
  test_x86_backend_renders_typed_two_arg_direct_call_first_local_rewrite_spacing_slice();
  test_x86_backend_renders_typed_two_arg_direct_call_both_local_arg_slice();
  test_x86_backend_renders_typed_two_arg_direct_call_both_local_first_rewrite_slice();
  test_x86_backend_renders_typed_two_arg_direct_call_both_local_second_rewrite_slice();
  test_x86_backend_renders_typed_two_arg_direct_call_both_local_double_rewrite_slice();
  test_x86_backend_renders_local_array_slice();
  test_x86_backend_renders_global_load_slice();
  test_x86_backend_uses_shared_regalloc_for_call_crossing_direct_call_slice();
  test_x86_backend_cleans_up_redundant_self_move_on_call_crossing_slice();
  test_x86_backend_keeps_spacing_tolerant_call_crossing_slice_on_asm_path();
  test_x86_backend_renders_compare_and_branch_slice();
  test_x86_backend_renders_compare_and_branch_slice_from_typed_predicates();
  test_x86_backend_renders_compare_and_branch_le_slice();
  test_x86_backend_renders_compare_and_branch_gt_slice();
  test_x86_backend_renders_compare_and_branch_ge_slice();
  test_x86_backend_renders_compare_and_branch_eq_slice();
  test_x86_backend_renders_compare_and_branch_ne_slice();
  test_x86_backend_renders_compare_and_branch_ult_slice();
  test_x86_backend_renders_compare_and_branch_ule_slice();
  test_x86_backend_renders_compare_and_branch_ugt_slice();
  test_x86_backend_renders_compare_and_branch_uge_slice();
  test_x86_backend_renders_extern_global_array_slice();
  test_x86_backend_renders_extern_decl_object_slice();
  test_x86_backend_renders_extern_decl_object_slice_with_typed_zero_arg_spacing();
  test_x86_backend_renders_string_literal_char_slice();
  test_x86_backend_renders_global_char_pointer_diff_slice();
  test_x86_backend_renders_global_char_pointer_diff_slice_from_typed_ops();
  test_x86_backend_renders_global_int_pointer_diff_slice();
  test_x86_backend_renders_global_int_pointer_diff_slice_from_typed_ops();
  test_x86_backend_renders_global_int_pointer_roundtrip_slice();
  test_aarch64_backend_renders_void_return_slice();
  test_aarch64_backend_preserves_module_headers_and_declarations();
  test_aarch64_backend_propagates_malformed_signature_in_supported_slice();
  test_aarch64_backend_renders_compare_and_branch_slice();
  test_aarch64_backend_renders_compare_and_branch_slice_from_typed_predicates();
  test_aarch64_backend_renders_compare_and_branch_le_slice();
  test_aarch64_backend_renders_compare_and_branch_gt_slice();
  test_aarch64_backend_renders_compare_and_branch_ge_slice();
  test_aarch64_backend_renders_compare_and_branch_eq_slice();
  test_aarch64_backend_renders_compare_and_branch_ne_slice();
  test_aarch64_backend_renders_compare_and_branch_ult_slice();
  test_aarch64_backend_renders_compare_and_branch_ule_slice();
  test_aarch64_backend_renders_compare_and_branch_ugt_slice();
  test_aarch64_backend_renders_compare_and_branch_uge_slice();
  test_aarch64_backend_scaffold_rejects_out_of_slice_ir();
  test_aarch64_backend_renders_direct_call_slice();
  test_aarch64_backend_renders_zero_arg_typed_direct_call_slice_with_whitespace();
  test_aarch64_backend_rejects_intrinsic_callee_from_direct_call_fast_path();
  test_aarch64_backend_rejects_indirect_callee_from_direct_call_fast_path();
  test_aarch64_backend_renders_local_temp_memory_slice();
  test_aarch64_backend_renders_param_slot_memory_slice();
  test_aarch64_backend_renders_typed_direct_call_slice();
  test_aarch64_backend_uses_shared_regalloc_for_call_crossing_direct_call_slice();
  test_aarch64_backend_cleans_up_redundant_call_result_traffic_on_call_crossing_slice();
  test_aarch64_backend_keeps_spacing_tolerant_call_crossing_slice_on_asm_path();
  test_aarch64_backend_renders_typed_two_arg_direct_call_slice();
  test_aarch64_backend_renders_typed_direct_call_local_arg_slice();
  test_aarch64_backend_renders_typed_direct_call_local_arg_spacing_slice();
  test_aarch64_backend_renders_typed_two_arg_direct_call_local_arg_slice();
  test_aarch64_backend_renders_typed_two_arg_direct_call_second_local_arg_slice();
  test_aarch64_backend_renders_typed_two_arg_direct_call_second_local_rewrite_slice();
  test_aarch64_backend_renders_typed_two_arg_direct_call_first_local_rewrite_slice();
  test_aarch64_backend_renders_typed_two_arg_direct_call_first_local_rewrite_spacing_slice();
  test_aarch64_backend_renders_typed_two_arg_direct_call_both_local_arg_slice();
  test_aarch64_backend_renders_typed_two_arg_direct_call_both_local_first_rewrite_slice();
  test_aarch64_backend_renders_typed_two_arg_direct_call_both_local_second_rewrite_slice();
  test_aarch64_backend_renders_typed_two_arg_direct_call_both_local_double_rewrite_slice();
  test_aarch64_backend_renders_local_array_gep_slice();
  test_aarch64_backend_renders_param_member_array_gep_slice();
  test_aarch64_backend_renders_nested_member_pointer_array_gep_slice();
  test_aarch64_backend_renders_nested_param_member_array_gep_slice();
  test_aarch64_backend_renders_global_definition_slice();
  test_aarch64_backend_renders_string_pool_slice();
  test_aarch64_backend_renders_extern_decl_slice();
  test_aarch64_backend_renders_extern_global_load_slice();
  test_aarch64_backend_renders_extern_global_array_slice();
  test_aarch64_backend_renders_global_char_pointer_diff_slice();
  test_aarch64_backend_renders_global_char_pointer_diff_slice_from_typed_ops();
  test_aarch64_backend_renders_global_int_pointer_diff_slice();
  test_aarch64_backend_renders_global_int_pointer_diff_slice_from_typed_ops();
  test_aarch64_backend_renders_global_int_pointer_roundtrip_slice();
  test_aarch64_backend_renders_bitcast_slice();
  test_aarch64_backend_renders_trunc_slice();
  test_aarch64_backend_renders_va_intrinsic_slice();
  test_aarch64_backend_renders_va_arg_scalar_slice();
  test_aarch64_backend_renders_va_arg_pair_slice();
  test_aarch64_backend_renders_va_arg_bigints_slice();
  test_aarch64_backend_renders_phi_join_slice();
  test_x86_assembler_parser_accepts_bounded_return_add_slice();
  test_x86_assembler_parser_rejects_out_of_scope_forms();
  test_x86_assembler_encoder_emits_bounded_return_add_bytes();
  test_x86_assembler_encoder_emits_bounded_extern_call_bytes();
  test_x86_assembler_encoder_rejects_out_of_scope_instruction_forms();
  test_x86_builtin_object_matches_external_return_add_surface();
  test_aarch64_assembler_parser_stub_preserves_text();
  test_aarch64_assembler_elf_writer_branch_reloc_helper();
  test_aarch64_assembler_encoder_helper_smoke();
  test_backend_shared_liveness_surface_tracks_result_names();
  test_backend_shared_liveness_surface_tracks_call_crossing_ranges();
  test_backend_shared_liveness_surface_tracks_phi_join_ranges();
  test_backend_shared_regalloc_surface_uses_caller_saved_for_non_call_interval();
  test_backend_shared_regalloc_prefers_callee_saved_for_call_spanning_values();
  test_backend_shared_regalloc_reuses_register_after_interval_ends();
  test_backend_shared_regalloc_spills_overlapping_values_without_reusing_busy_reg();
  test_backend_shared_regalloc_helper_filters_and_merges_clobbers();
  test_backend_shared_stack_layout_regalloc_helper_exposes_handoff_view();
  test_backend_shared_stack_layout_analysis_tracks_phi_use_blocks();
  test_backend_shared_stack_layout_analysis_detects_dead_param_allocas();
  test_backend_shared_stack_layout_analysis_tracks_call_arg_values();
  test_backend_shared_stack_layout_analysis_detects_entry_alloca_overwrite_before_read();
  test_backend_shared_alloca_coalescing_classifies_non_param_allocas();
  test_backend_shared_slot_assignment_plans_param_alloca_slots();
  test_backend_shared_slot_assignment_prunes_dead_param_alloca_insts();
  test_backend_shared_slot_assignment_plans_entry_alloca_slots();
  test_backend_shared_slot_assignment_prunes_dead_entry_alloca_insts();
  test_backend_shared_slot_assignment_applies_coalesced_entry_slots();
  test_aarch64_backend_prunes_dead_param_allocas_from_fallback_lir();
  test_aarch64_backend_prunes_dead_local_allocas_from_fallback_lir();
  test_backend_binary_utils_contract_headers_are_include_reachable();
  test_shared_linker_parses_minimal_relocation_object_fixture();
  test_shared_linker_parses_builtin_return_add_object();
  test_shared_linker_parses_builtin_x86_extern_call_object();
  test_shared_linker_parses_single_member_archive_fixture();
  test_aarch64_linker_names_first_static_call26_slice();
  test_aarch64_linker_loads_first_static_objects_through_shared_input_seam();
  test_aarch64_linker_loads_first_static_objects_from_archive_through_shared_input_seam();
  test_x86_linker_names_first_static_plt32_slice();
  test_x86_linker_loads_first_static_objects_through_shared_input_seam();
  test_x86_linker_loads_first_static_objects_from_archive_through_shared_input_seam();
  test_x86_linker_emits_first_static_plt32_executable_slice();
  test_x86_linker_matches_external_first_static_executable_slice();
  test_aarch64_linker_emits_first_static_call26_executable_slice();
  test_aarch64_backend_assembler_handoff_helper_stages_emitted_text();
  test_x86_backend_assembler_handoff_helper_stages_emitted_text();
  test_aarch64_builtin_object_matches_external_return_add_surface();
  test_x86_builtin_object_matches_external_extern_call_surface();
  return 0;
}
