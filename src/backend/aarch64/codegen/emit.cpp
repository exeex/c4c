#include "emit.hpp"

#include "../../lir_adapter.hpp"
#include "../../generation.hpp"
#include "../../stack_layout/analysis.hpp"
#include "../../stack_layout/slot_assignment.hpp"
#include "../../../codegen/lir/lir_printer.hpp"

#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace c4c::backend::aarch64 {

namespace {

[[noreturn]] void fail_unsupported(const char* detail) {
  throw std::invalid_argument(std::string("aarch64 backend emitter does not support ") +
                              detail);
}

void validate_inst(const c4c::codegen::lir::LirInst& inst) {
  if (std::holds_alternative<c4c::codegen::lir::LirSelectOp>(inst)) {
    fail_unsupported("non-ALU/non-branch/non-call/non-memory instructions");
  }
}

void validate_block(const c4c::codegen::lir::LirBlock& block) {
  for (const auto& inst : block.insts) {
    validate_inst(inst);
  }
}

void validate_function(const c4c::codegen::lir::LirFunction& function) {
  for (const auto& inst : function.alloca_insts) {
    validate_inst(inst);
  }
  for (const auto& block : function.blocks) {
    validate_block(block);
  }
}

void validate_module(const c4c::codegen::lir::LirModule& module) {
  for (const auto& function : module.functions) {
    validate_function(function);
  }
}

std::optional<std::int64_t> parse_i64(std::string_view text) {
  std::int64_t value = 0;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc() || result.ptr != end) {
    return std::nullopt;
  }
  return value;
}

bool is_minimal_single_function_asm_slice(const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1) return false;

  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature.linkage != "define" ||
      function.signature.return_type != "i32" || function.signature.name != "main" ||
      !function.signature.params.empty() || function.signature.is_vararg ||
      function.blocks.size() != 1) {
    return false;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || !block.terminator.value.has_value() ||
      block.terminator.type_str != "i32") {
    return false;
  }

  return true;
}

constexpr std::array<c4c::backend::PhysReg, 9> kAarch64CalleeSavedRegs = {
    c4c::backend::PhysReg{20}, c4c::backend::PhysReg{21}, c4c::backend::PhysReg{22},
    c4c::backend::PhysReg{23}, c4c::backend::PhysReg{24}, c4c::backend::PhysReg{25},
    c4c::backend::PhysReg{26}, c4c::backend::PhysReg{27}, c4c::backend::PhysReg{28},
};

constexpr std::array<c4c::backend::PhysReg, 2> kAarch64CallerSavedRegs = {
    c4c::backend::PhysReg{13},
    c4c::backend::PhysReg{14},
};

void prune_dead_entry_allocas(c4c::codegen::lir::LirFunction& function) {
  c4c::backend::RegAllocConfig config;
  config.available_regs.assign(kAarch64CalleeSavedRegs.begin(), kAarch64CalleeSavedRegs.end());
  config.caller_saved_regs.assign(kAarch64CallerSavedRegs.begin(), kAarch64CallerSavedRegs.end());

  const auto regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(function, config, {});
  const std::vector<c4c::backend::PhysReg> callee_saved(kAarch64CalleeSavedRegs.begin(),
                                                        kAarch64CalleeSavedRegs.end());
  const auto analysis =
      c4c::backend::stack_layout::analyze_stack_layout(function, regalloc, callee_saved);
  const auto plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(function, analysis);
  c4c::backend::stack_layout::apply_entry_alloca_slot_plan(function, plans);
}

c4c::codegen::lir::LirModule prepare_module_for_fallback(
    const c4c::codegen::lir::LirModule& module) {
  auto prepared = module;
  for (auto& function : prepared.functions) {
    prune_dead_entry_allocas(function);
  }
  return prepared;
}

std::string asm_symbol_name(const c4c::backend::BackendModule& module,
                            std::string_view logical_name) {
  const bool is_darwin =
      module.target_triple.find("apple-darwin") != std::string::npos;
  if (!is_darwin) return std::string(logical_name);
  return std::string("_") + std::string(logical_name);
}

std::optional<std::int64_t> parse_minimal_return_imm(
    const c4c::backend::BackendModule& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& block = module.functions.front().blocks.front();
  if (!block.insts.empty()) {
    return std::nullopt;
  }

  return parse_i64(*block.terminator.value);
}

std::optional<std::int64_t> parse_minimal_return_add_imm(
    const c4c::backend::BackendModule& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& block = module.functions.front().blocks.front();
  if (block.insts.size() != 1 ||
      !block.terminator.value.has_value() || block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* add = std::get_if<c4c::backend::BackendBinaryInst>(&block.insts.front());
  if (add == nullptr || add->opcode != c4c::backend::BackendBinaryOpcode::Add ||
      add->type_str != "i32" || *block.terminator.value != add->result) {
    return std::nullopt;
  }

  const auto lhs = parse_i64(add->lhs);
  const auto rhs = parse_i64(add->rhs);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }
  return *lhs + *rhs;
}

const c4c::backend::BackendFunction* find_function(
    const c4c::backend::BackendModule& module,
    std::string_view name) {
  for (const auto& function : module.functions) {
    if (function.signature.name == name) return &function;
  }
  return nullptr;
}

std::optional<std::int64_t> parse_single_block_return_imm(
    const c4c::backend::BackendFunction& function) {
  if (function.is_declaration || function.signature.linkage != "define" ||
      function.signature.return_type != "i32" || !function.signature.params.empty() ||
      function.signature.is_vararg || function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || !block.terminator.value.has_value() ||
      block.terminator.type_str != "i32" || !block.insts.empty()) {
    return std::nullopt;
  }

  return parse_i64(*block.terminator.value);
}

struct MinimalDirectCallSlice {
  std::string callee_name;
  std::int64_t callee_imm = 0;
};

struct MinimalDirectCallAddImmSlice {
  std::string callee_name;
  std::int64_t arg_imm = 0;
  std::int64_t add_imm = 0;
};

struct MinimalDirectCallTwoArgAddSlice {
  std::string callee_name;
  std::int64_t arg0_imm = 0;
  std::int64_t arg1_imm = 0;
};

struct MinimalConditionalReturnSlice {
  std::int64_t lhs_imm = 0;
  std::int64_t rhs_imm = 0;
  std::string predicate;
  std::string true_label;
  std::string false_label;
  std::int64_t true_return_imm = 0;
  std::int64_t false_return_imm = 0;
};

struct MinimalScalarGlobalLoadSlice {
  std::string global_name;
  std::int64_t init_imm = 0;
  int align_bytes = 4;
};

struct MinimalExternScalarGlobalLoadSlice {
  std::string global_name;
};

std::optional<MinimalConditionalReturnSlice> parse_minimal_conditional_return_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1) return std::nullopt;

  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature_text != "define i32 @main()\n" ||
      function.entry.value != 0 || function.blocks.size() != 3 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks[0];
  if (entry.label != "entry" || entry.insts.size() != 3) {
    return std::nullopt;
  }

  const auto* cmp0 = std::get_if<LirCmpOp>(&entry.insts[0]);
  const auto* cast = std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* cmp1 = std::get_if<LirCmpOp>(&entry.insts[2]);
  const auto* condbr = std::get_if<LirCondBr>(&entry.terminator);
  if (cmp0 == nullptr || cast == nullptr || cmp1 == nullptr || condbr == nullptr ||
      cmp0->is_float || cmp0->type_str != "i32" ||
      cast->kind != LirCastKind::ZExt || cast->from_type != "i1" ||
      cast->operand != cmp0->result || cast->to_type != "i32" || cmp1->is_float ||
      cmp1->predicate != "ne" || cmp1->type_str != "i32" || cmp1->lhs != cast->result ||
      cmp1->rhs != "0" || condbr->cond_name != cmp1->result) {
    return std::nullopt;
  }
  if (cmp0->predicate != "slt" && cmp0->predicate != "sle" &&
      cmp0->predicate != "sgt" && cmp0->predicate != "sge" &&
      cmp0->predicate != "ult" && cmp0->predicate != "ule" &&
      cmp0->predicate != "ugt" && cmp0->predicate != "uge" &&
      cmp0->predicate != "eq" &&
      cmp0->predicate != "ne") {
    return std::nullopt;
  }

  const auto lhs_imm = parse_i64(cmp0->lhs);
  const auto rhs_imm = parse_i64(cmp0->rhs);
  if (!lhs_imm.has_value() || !rhs_imm.has_value()) {
    return std::nullopt;
  }

  const auto& true_block = function.blocks[1];
  const auto& false_block = function.blocks[2];
  if (true_block.label != condbr->true_label || false_block.label != condbr->false_label ||
      !true_block.insts.empty() || !false_block.insts.empty()) {
    return std::nullopt;
  }

  const auto* true_ret = std::get_if<LirRet>(&true_block.terminator);
  const auto* false_ret = std::get_if<LirRet>(&false_block.terminator);
  if (true_ret == nullptr || false_ret == nullptr || !true_ret->value_str.has_value() ||
      !false_ret->value_str.has_value() || true_ret->type_str != "i32" ||
      false_ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto true_return_imm = parse_i64(*true_ret->value_str);
  const auto false_return_imm = parse_i64(*false_ret->value_str);
  if (!true_return_imm.has_value() || !false_return_imm.has_value()) {
    return std::nullopt;
  }

  return MinimalConditionalReturnSlice{*lhs_imm,
                                       *rhs_imm,
                                       cmp0->predicate,
                                       condbr->true_label,
                                       condbr->false_label,
                                       *true_return_imm,
                                       *false_return_imm};
}

std::optional<MinimalScalarGlobalLoadSlice> parse_minimal_scalar_global_load_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_internal || global.is_const || global.is_extern_decl ||
      global.linkage_vis != "" || global.qualifier != "global " ||
      global.llvm_type != "i32") {
    return std::nullopt;
  }
  const auto init_imm = parse_i64(global.init_text);
  if (!init_imm.has_value()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature_text != "define i32 @main()\n" ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry" || entry.insts.size() != 1) {
    return std::nullopt;
  }

  const auto* load = std::get_if<LirLoadOp>(&entry.insts.front());
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (load == nullptr || ret == nullptr || load->type_str != "i32" ||
      load->ptr != "@" + global.name || !ret->value_str.has_value() ||
      *ret->value_str != load->result || ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalScalarGlobalLoadSlice{global.name, *init_imm,
                                      global.align_bytes > 0 ? global.align_bytes : 4};
}

std::optional<MinimalExternScalarGlobalLoadSlice> parse_minimal_extern_scalar_global_load_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_internal || global.is_const || !global.is_extern_decl ||
      global.linkage_vis != "external " || global.qualifier != "global " ||
      global.llvm_type != "i32" || !global.init_text.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature_text != "define i32 @main()\n" ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry" || entry.insts.size() != 1) {
    return std::nullopt;
  }

  const auto* load = std::get_if<LirLoadOp>(&entry.insts.front());
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (load == nullptr || ret == nullptr || load->type_str != "i32" ||
      load->ptr != "@" + global.name || !ret->value_str.has_value() ||
      *ret->value_str != load->result || ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalExternScalarGlobalLoadSlice{global.name};
}

std::optional<MinimalDirectCallSlice> parse_minimal_direct_call_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 2) return std::nullopt;

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      main_fn->signature.linkage != "define" ||
      main_fn->signature.return_type != "i32" ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& main_block = main_fn->blocks.front();
  if (main_block.label != "entry" || main_block.insts.size() != 1 ||
      !main_block.terminator.value.has_value() ||
      main_block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* call = std::get_if<c4c::backend::BackendCallInst>(&main_block.insts.front());
  if (call == nullptr || call->return_type != "i32" || call->result.empty() ||
      *main_block.terminator.value != call->result ||
      !call->callee_type_suffix.empty() || !call->args_str.empty() ||
      call->callee.empty() || call->callee.front() != '@') {
    return std::nullopt;
  }

  const std::string callee_name = call->callee.substr(1);
  if (callee_name == "main") return std::nullopt;

  const auto* callee_fn = find_function(module, callee_name);
  if (callee_fn == nullptr) return std::nullopt;
  const auto callee_imm = parse_single_block_return_imm(*callee_fn);
  if (!callee_imm.has_value()) return std::nullopt;

  return MinimalDirectCallSlice{callee_name, *callee_imm};
}

std::optional<MinimalDirectCallAddImmSlice> parse_minimal_direct_call_add_imm_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 2) return std::nullopt;

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      main_fn->signature.linkage != "define" ||
      main_fn->signature.return_type != "i32" ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& main_block = main_fn->blocks.front();
  if (main_block.label != "entry" || main_block.insts.size() != 1 ||
      !main_block.terminator.value.has_value() ||
      main_block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* call = std::get_if<c4c::backend::BackendCallInst>(&main_block.insts.front());
  if (call == nullptr || call->return_type != "i32" || call->result.empty() ||
      *main_block.terminator.value != call->result ||
      call->callee_type_suffix != "(i32)" || call->callee.empty() ||
      call->callee.front() != '@') {
    return std::nullopt;
  }

  const std::string callee_name = call->callee.substr(1);
  if (callee_name == "main") return std::nullopt;

  const std::string_view args_str = call->args_str;
  if (args_str.size() < 5 || args_str.substr(0, 4) != "i32 ") {
    return std::nullopt;
  }
  const auto arg_imm = parse_i64(args_str.substr(4));
  if (!arg_imm.has_value()) {
    return std::nullopt;
  }

  const auto* callee_fn = find_function(module, callee_name);
  if (callee_fn == nullptr || callee_fn->is_declaration ||
      callee_fn->signature.linkage != "define" ||
      callee_fn->signature.return_type != "i32" ||
      callee_fn->signature.params.size() != 1 ||
      callee_fn->signature.params.front().type_str != "i32" ||
      callee_fn->signature.params.front().name.empty() ||
      callee_fn->signature.is_vararg || callee_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& callee_block = callee_fn->blocks.front();
  if (callee_block.label != "entry" || callee_block.insts.size() != 1 ||
      !callee_block.terminator.value.has_value() ||
      callee_block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* add = std::get_if<c4c::backend::BackendBinaryInst>(&callee_block.insts.front());
  if (add == nullptr || add->opcode != c4c::backend::BackendBinaryOpcode::Add ||
      add->type_str != "i32" || *callee_block.terminator.value != add->result ||
      add->lhs != callee_fn->signature.params.front().name) {
    return std::nullopt;
  }

  const auto add_imm = parse_i64(add->rhs);
  if (!add_imm.has_value()) {
    return std::nullopt;
  }

  return MinimalDirectCallAddImmSlice{callee_name, *arg_imm, *add_imm};
}

std::optional<MinimalDirectCallTwoArgAddSlice>
parse_minimal_direct_call_two_arg_add_slice(const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 2) return std::nullopt;

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      main_fn->signature.linkage != "define" ||
      main_fn->signature.return_type != "i32" ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& main_block = main_fn->blocks.front();
  if (main_block.label != "entry" || main_block.insts.size() != 1 ||
      !main_block.terminator.value.has_value() ||
      main_block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* call = std::get_if<c4c::backend::BackendCallInst>(&main_block.insts.front());
  if (call == nullptr || call->return_type != "i32" || call->result.empty() ||
      *main_block.terminator.value != call->result ||
      call->callee_type_suffix != "(i32, i32)" || call->callee.empty() ||
      call->callee.front() != '@') {
    return std::nullopt;
  }

  const std::string callee_name = call->callee.substr(1);
  if (callee_name == "main") return std::nullopt;

  const std::string_view args_str = call->args_str;
  const size_t comma = args_str.find(", ");
  if (comma == std::string_view::npos) {
    return std::nullopt;
  }
  const std::string_view arg0 = args_str.substr(0, comma);
  const std::string_view arg1 = args_str.substr(comma + 2);
  if (arg0.size() < 5 || arg1.size() < 5 || arg0.substr(0, 4) != "i32 " ||
      arg1.substr(0, 4) != "i32 ") {
    return std::nullopt;
  }
  const auto arg0_imm = parse_i64(arg0.substr(4));
  const auto arg1_imm = parse_i64(arg1.substr(4));
  if (!arg0_imm.has_value() || !arg1_imm.has_value()) {
    return std::nullopt;
  }

  const auto* callee_fn = find_function(module, callee_name);
  if (callee_fn == nullptr || callee_fn->is_declaration ||
      callee_fn->signature.linkage != "define" ||
      callee_fn->signature.return_type != "i32" ||
      callee_fn->signature.params.size() != 2 ||
      callee_fn->signature.params[0].type_str != "i32" ||
      callee_fn->signature.params[1].type_str != "i32" ||
      callee_fn->signature.params[0].name.empty() ||
      callee_fn->signature.params[1].name.empty() ||
      callee_fn->signature.is_vararg || callee_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& callee_block = callee_fn->blocks.front();
  if (callee_block.label != "entry" || callee_block.insts.size() != 1 ||
      !callee_block.terminator.value.has_value() ||
      callee_block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* add = std::get_if<c4c::backend::BackendBinaryInst>(&callee_block.insts.front());
  if (add == nullptr || add->opcode != c4c::backend::BackendBinaryOpcode::Add ||
      add->type_str != "i32" || *callee_block.terminator.value != add->result ||
      add->lhs != callee_fn->signature.params[0].name ||
      add->rhs != callee_fn->signature.params[1].name) {
    return std::nullopt;
  }

  return MinimalDirectCallTwoArgAddSlice{callee_name, *arg0_imm, *arg1_imm};
}

void emit_function_prelude(std::ostringstream& out,
                           const c4c::backend::BackendModule& module,
                           std::string_view symbol,
                           bool is_global) {
  const bool is_darwin =
      module.target_triple.find("apple-darwin") != std::string::npos;
  if (is_global) {
    out << ".globl " << symbol << "\n";
  }
  if (is_darwin) {
    out << ".p2align 2\n";
  } else {
    out << ".type " << symbol << ", %function\n";
  }
  out << symbol << ":\n";
}

std::string emit_minimal_return_imm_asm(const c4c::backend::BackendModule& module,
                                        std::int64_t imm) {
  if (imm < 0 || imm > std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported("return immediates outside the minimal mov-supported range");
  }

  std::ostringstream out;
  const std::string symbol = asm_symbol_name(module, "main");

  out << ".text\n";
  emit_function_prelude(out, module, symbol, true);
  out
      << "  mov w0, #" << imm << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_direct_call_asm(const c4c::backend::BackendModule& module,
                                         const MinimalDirectCallSlice& slice) {
  if (slice.callee_imm < 0 ||
      slice.callee_imm > std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported("helper return immediates outside the minimal mov-supported range");
  }

  std::ostringstream out;
  const std::string helper_symbol = asm_symbol_name(module, slice.callee_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  out << ".text\n";
  emit_function_prelude(out, module, helper_symbol, false);
  out << "  mov w0, #" << slice.callee_imm << "\n"
      << "  ret\n";
  emit_function_prelude(out, module, main_symbol, true);
  out << "  sub sp, sp, #16\n"
      << "  str x30, [sp, #8]\n"
      << "  bl " << helper_symbol << "\n"
      << "  ldr x30, [sp, #8]\n"
      << "  add sp, sp, #16\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_direct_call_add_imm_asm(
    const c4c::backend::BackendModule& module,
    const MinimalDirectCallAddImmSlice& slice) {
  if (slice.arg_imm < 0 ||
      slice.arg_imm > std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported("direct-call argument immediates outside the minimal mov-supported range");
  }
  if (slice.add_imm < 0 || slice.add_imm > 4095) {
    fail_unsupported("helper add immediates outside the minimal add-supported range");
  }

  std::ostringstream out;
  const std::string helper_symbol = asm_symbol_name(module, slice.callee_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  out << ".text\n";
  emit_function_prelude(out, module, helper_symbol, false);
  out << "  add w0, w0, #" << slice.add_imm << "\n"
      << "  ret\n";
  emit_function_prelude(out, module, main_symbol, true);
  out << "  sub sp, sp, #16\n"
      << "  str x30, [sp, #8]\n"
      << "  mov w0, #" << slice.arg_imm << "\n"
      << "  bl " << helper_symbol << "\n"
      << "  ldr x30, [sp, #8]\n"
      << "  add sp, sp, #16\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_direct_call_two_arg_add_asm(
    const c4c::backend::BackendModule& module,
    const MinimalDirectCallTwoArgAddSlice& slice) {
  if (slice.arg0_imm < 0 ||
      slice.arg0_imm > std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported("first direct-call argument immediates outside the minimal mov-supported range");
  }
  if (slice.arg1_imm < 0 ||
      slice.arg1_imm > std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported("second direct-call argument immediates outside the minimal mov-supported range");
  }

  std::ostringstream out;
  const std::string helper_symbol = asm_symbol_name(module, slice.callee_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  out << ".text\n";
  emit_function_prelude(out, module, helper_symbol, false);
  out << "  add w0, w0, w1\n"
      << "  ret\n";
  emit_function_prelude(out, module, main_symbol, true);
  out << "  sub sp, sp, #16\n"
      << "  str x30, [sp, #8]\n"
      << "  mov w0, #" << slice.arg0_imm << "\n"
      << "  mov w1, #" << slice.arg1_imm << "\n"
      << "  bl " << helper_symbol << "\n"
      << "  ldr x30, [sp, #8]\n"
      << "  add sp, sp, #16\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_conditional_return_asm(
    const c4c::codegen::lir::LirModule& module,
    const MinimalConditionalReturnSlice& slice) {
  const auto in_mov_range = [](std::int64_t imm) {
    return imm >= 0 && imm <= std::numeric_limits<std::uint16_t>::max();
  };
  if (!in_mov_range(slice.lhs_imm) || !in_mov_range(slice.rhs_imm) ||
      !in_mov_range(slice.true_return_imm) || !in_mov_range(slice.false_return_imm)) {
    fail_unsupported("conditional-return immediates outside the minimal mov-supported range");
  }

  std::ostringstream out;
  c4c::backend::BackendModule backend_module;
  backend_module.target_triple = module.target_triple;
  const std::string main_symbol = asm_symbol_name(backend_module, "main");
  const char* fail_branch = nullptr;
  if (slice.predicate == "slt") {
    fail_branch = "b.ge";
  } else if (slice.predicate == "sle") {
    fail_branch = "b.gt";
  } else if (slice.predicate == "sgt") {
    fail_branch = "b.le";
  } else if (slice.predicate == "sge") {
    fail_branch = "b.lt";
  } else if (slice.predicate == "ult") {
    fail_branch = "b.hs";
  } else if (slice.predicate == "ule") {
    fail_branch = "b.hi";
  } else if (slice.predicate == "ugt") {
    fail_branch = "b.ls";
  } else if (slice.predicate == "uge") {
    fail_branch = "b.lo";
  } else if (slice.predicate == "eq") {
    fail_branch = "b.ne";
  } else if (slice.predicate == "ne") {
    fail_branch = "b.eq";
  } else {
    fail_unsupported("conditional-return predicates outside the current compare-and-branch slice");
  }

  out << ".text\n";
  emit_function_prelude(out, backend_module, main_symbol, true);
  out << "  mov w8, #" << slice.lhs_imm << "\n"
      << "  cmp w8, #" << slice.rhs_imm << "\n"
      << "  " << fail_branch << " .L" << slice.false_label << "\n"
      << ".L" << slice.true_label << ":\n"
      << "  mov w0, #" << slice.true_return_imm << "\n"
      << "  ret\n"
      << ".L" << slice.false_label << ":\n"
      << "  mov w0, #" << slice.false_return_imm << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_scalar_global_load_asm(
    const c4c::codegen::lir::LirModule& module,
    const MinimalScalarGlobalLoadSlice& slice) {
  if (slice.init_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.init_imm > std::numeric_limits<std::int32_t>::max()) {
    fail_unsupported("scalar global initializers outside the minimal .long-supported range");
  }

  c4c::backend::BackendModule backend_module;
  backend_module.target_triple = module.target_triple;
  const bool is_darwin =
      backend_module.target_triple.find("apple-darwin") != std::string::npos;
  const std::string global_symbol =
      asm_symbol_name(backend_module, slice.global_name);
  const std::string main_symbol = asm_symbol_name(backend_module, "main");
  const int align_pow2 = slice.align_bytes <= 1
                             ? 0
                             : (slice.align_bytes == 2 ? 1 : 2);

  std::ostringstream out;
  out << ".data\n"
      << ".globl " << global_symbol << "\n";
  if (!is_darwin) {
    out << ".type " << global_symbol << ", %object\n";
  }
  out << ".p2align " << align_pow2 << "\n"
      << global_symbol << ":\n"
      << "  .long " << slice.init_imm << "\n";
  if (!is_darwin) {
    out << ".size " << global_symbol << ", 4\n";
  }

  out << ".text\n";
  emit_function_prelude(out, backend_module, main_symbol, true);
  if (is_darwin) {
    out << "  adrp x8, " << global_symbol << "@PAGE\n"
        << "  ldr w0, [x8, " << global_symbol << "@PAGEOFF]\n";
  } else {
    out << "  adrp x8, " << global_symbol << "\n"
        << "  ldr w0, [x8, :lo12:" << global_symbol << "]\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_extern_scalar_global_load_asm(
    const c4c::codegen::lir::LirModule& module,
    const MinimalExternScalarGlobalLoadSlice& slice) {
  c4c::backend::BackendModule backend_module;
  backend_module.target_triple = module.target_triple;
  const bool is_darwin =
      backend_module.target_triple.find("apple-darwin") != std::string::npos;
  const std::string global_symbol =
      asm_symbol_name(backend_module, slice.global_name);
  const std::string main_symbol = asm_symbol_name(backend_module, "main");

  std::ostringstream out;
  if (!is_darwin) {
    out << ".extern " << global_symbol << "\n";
  }
  out << ".text\n";
  emit_function_prelude(out, backend_module, main_symbol, true);
  if (is_darwin) {
    out << "  adrp x8, " << global_symbol << "@PAGE\n"
        << "  ldr w0, [x8, " << global_symbol << "@PAGEOFF]\n";
  } else {
    out << "  adrp x8, " << global_symbol << "\n"
        << "  ldr w0, [x8, :lo12:" << global_symbol << "]\n";
  }
  out << "  ret\n";
  return out.str();
}

}  // namespace

std::string emit_module(const c4c::codegen::lir::LirModule& module) {
  const auto prepared = prepare_module_for_fallback(module);
  validate_module(prepared);
  if (const auto slice = parse_minimal_conditional_return_slice(prepared);
      slice.has_value()) {
    return emit_minimal_conditional_return_asm(prepared, *slice);
  }
  if (const auto slice = parse_minimal_scalar_global_load_slice(prepared);
      slice.has_value()) {
    return emit_minimal_scalar_global_load_asm(prepared, *slice);
  }
  if (const auto slice = parse_minimal_extern_scalar_global_load_slice(prepared);
      slice.has_value()) {
    return emit_minimal_extern_scalar_global_load_asm(prepared, *slice);
  }
  try {
    const auto adapted = c4c::backend::adapt_minimal_module(prepared);
    if (const auto imm = parse_minimal_return_imm(adapted); imm.has_value()) {
      return emit_minimal_return_imm_asm(adapted, *imm);
    }
    if (const auto imm = parse_minimal_return_add_imm(adapted); imm.has_value()) {
      return emit_minimal_return_imm_asm(adapted, *imm);
    }
    if (const auto slice = parse_minimal_direct_call_slice(adapted); slice.has_value()) {
      return emit_minimal_direct_call_asm(adapted, *slice);
    }
    if (const auto slice = parse_minimal_direct_call_add_imm_slice(adapted);
        slice.has_value()) {
      return emit_minimal_direct_call_add_imm_asm(adapted, *slice);
    }
    if (const auto slice = parse_minimal_direct_call_two_arg_add_slice(adapted);
        slice.has_value()) {
      return emit_minimal_direct_call_two_arg_add_asm(adapted, *slice);
    }
    return c4c::backend::render_module(adapted);
  } catch (const c4c::backend::LirAdapterError& ex) {
    if (ex.kind() != c4c::backend::LirAdapterErrorKind::Unsupported) {
      throw;
    }
  }
  return c4c::codegen::lir::print_llvm(prepared);
}

}  // namespace c4c::backend::aarch64
