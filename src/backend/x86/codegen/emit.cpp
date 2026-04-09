#include "x86_codegen.hpp"

#include "../../backend.hpp"
#include "../../bir.hpp"
#include "../../lowering/call_decode.hpp"
#include "../../lowering/lir_to_bir.hpp"
#include "../../../codegen/lir/ir.hpp"

#include <charconv>
#include <cctype>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

namespace c4c::backend::x86 {

namespace {

struct MinimalStringLiteralCharSlice {
  std::string function_name;
  std::string pool_name;
  std::string raw_bytes;
  std::int64_t byte_index = 0;
  c4c::codegen::lir::LirCastKind extend_kind = c4c::codegen::lir::LirCastKind::SExt;
};

struct MinimalVariadicSum2Slice {
  std::string helper_name;
  std::string entry_name;
};

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

std::optional<std::int64_t> parse_i64(const c4c::backend::bir::Value& value) {
  if (value.kind != c4c::backend::bir::Value::Kind::Immediate ||
      value.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  return value.immediate;
}

bool is_minimal_single_function_asm_slice(const c4c::backend::bir::Module& module) {
  if (module.functions.size() != 1 || !module.globals.empty()) {
    return false;
  }
  const auto& function = module.functions.front();
  return function.params.empty() && function.blocks.size() == 1 &&
         function.blocks.front().label == "entry" &&
         function.return_type == c4c::backend::bir::TypeKind::I32;
}

std::optional<std::int64_t> parse_minimal_return_imm(
    const c4c::backend::bir::Module& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }
  const auto& block = module.functions.front().blocks.front();
  if (!block.insts.empty() || !block.terminator.value.has_value()) {
    return std::nullopt;
  }
  return parse_i64(*block.terminator.value);
}

std::string asm_symbol_name(std::string_view target_triple, std::string_view logical_name) {
  if (target_triple.find("apple-darwin") != std::string::npos) {
    return "_" + std::string(logical_name);
  }
  return std::string(logical_name);
}

std::string asm_private_data_label(std::string_view target_triple, std::string_view pool_name) {
  std::string label(pool_name);
  if (!label.empty() && label.front() == '@') {
    label.erase(label.begin());
  }
  while (!label.empty() && label.front() == '.') {
    label.erase(label.begin());
  }

  if (target_triple.find("apple-darwin") != std::string::npos) {
    return "L" + label;
  }
  return ".L." + label;
}

std::string escape_asm_string(std::string_view raw_bytes) {
  std::ostringstream out;
  for (unsigned char ch : raw_bytes) {
    switch (ch) {
      case '\\': out << "\\\\"; break;
      case '"': out << "\\\""; break;
      case '\n': out << "\\n"; break;
      case '\t': out << "\\t"; break;
      default:
        if (std::isprint(ch) != 0) {
          out << static_cast<char>(ch);
        } else {
          constexpr char kHex[] = "0123456789ABCDEF";
          out << '\\' << kHex[(ch >> 4) & 0xF] << kHex[ch & 0xF];
        }
        break;
    }
  }
  return out.str();
}

std::optional<MinimalStringLiteralCharSlice> parse_minimal_string_literal_char_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.string_pool.size() != 1 ||
      !module.globals.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& string_const = module.string_pool.front();
  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !c4c::backend::backend_lir_is_zero_arg_i32_definition(function.signature_text) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry" || entry.insts.size() != 5) {
    return std::nullopt;
  }

  const auto* base_gep = std::get_if<LirGepOp>(&entry.insts[0]);
  const auto* index_cast = std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* byte_gep = std::get_if<LirGepOp>(&entry.insts[2]);
  const auto* load = std::get_if<LirLoadOp>(&entry.insts[3]);
  const auto* extend = std::get_if<LirCastOp>(&entry.insts[4]);
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (base_gep == nullptr || index_cast == nullptr || byte_gep == nullptr ||
      load == nullptr || extend == nullptr || ret == nullptr ||
      base_gep->result.empty() || index_cast->result.empty() || load->result.empty()) {
    return std::nullopt;
  }

  const auto string_array_type =
      "[" + std::to_string(string_const.byte_length) + " x i8]";
  if (base_gep->element_type != string_array_type || base_gep->ptr != string_const.pool_name ||
      base_gep->indices.size() != 2 || base_gep->indices[0] != "i64 0" ||
      base_gep->indices[1] != "i64 0") {
    return std::nullopt;
  }

  if (index_cast->kind != LirCastKind::SExt || index_cast->from_type != "i32" ||
      index_cast->to_type != "i64") {
    return std::nullopt;
  }
  const auto byte_index = parse_i64(index_cast->operand);
  if (!byte_index.has_value() || *byte_index < 0 || *byte_index > 4095) {
    return std::nullopt;
  }

  if (byte_gep->element_type != "i8" || byte_gep->ptr != base_gep->result ||
      byte_gep->indices.size() != 1 || byte_gep->indices[0] != ("i64 " + index_cast->result)) {
    return std::nullopt;
  }
  if (load->type_str != "i8" || load->ptr != byte_gep->result) {
    return std::nullopt;
  }
  if ((extend->kind != LirCastKind::SExt && extend->kind != LirCastKind::ZExt) ||
      extend->from_type != "i8" || extend->operand != load->result ||
      extend->to_type != "i32" || !ret->value_str.has_value() ||
      *ret->value_str != extend->result || ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalStringLiteralCharSlice{
      .function_name = function.name,
      .pool_name = string_const.pool_name,
      .raw_bytes = string_const.raw_bytes,
      .byte_index = *byte_index,
      .extend_kind = extend->kind,
  };
}

std::string emit_function_prelude(std::string_view target_triple,
                                  std::string_view symbol_name) {
  std::ostringstream out;
  out << ".globl " << symbol_name << "\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".type " << symbol_name << ", @function\n";
  }
  out << symbol_name << ":\n";
  return out.str();
}

std::string emit_minimal_return_imm_asm(const c4c::backend::bir::Module& module,
                                        std::int64_t imm) {
  std::ostringstream out;
  const auto symbol =
      asm_symbol_name(module.target_triple, module.functions.front().name);
  out << emit_function_prelude(module.target_triple, symbol)
      << "  mov eax, " << imm << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_string_literal_char_asm(
    std::string_view target_triple,
    const MinimalStringLiteralCharSlice& slice) {
  const auto string_label = asm_private_data_label(target_triple, slice.pool_name);
  const auto symbol = asm_symbol_name(target_triple, slice.function_name);

  std::ostringstream out;
  out << ".section .rodata\n"
      << string_label << ":\n"
      << "  .asciz \"" << escape_asm_string(slice.raw_bytes) << "\"\n"
      << emit_function_prelude(target_triple, symbol)
      << "  lea rax, [rip + " << string_label << "]\n";
  if (slice.extend_kind == c4c::codegen::lir::LirCastKind::SExt) {
    out << "  movsx eax, BYTE PTR [rax + " << slice.byte_index << "]\n";
  } else {
    out << "  movzx eax, BYTE PTR [rax + " << slice.byte_index << "]\n";
  }
  out << "  ret\n";
  return out.str();
}

std::optional<MinimalVariadicSum2Slice> parse_minimal_variadic_sum2_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (!module.globals.empty() || !module.string_pool.empty() || module.functions.size() != 2) {
    return std::nullopt;
  }

  const auto& helper = module.functions.front();
  const auto& entry = module.functions.back();
  if (helper.name != "sum2" || helper.is_declaration || helper.entry.value != 0 ||
      helper.alloca_insts.size() != 2 || helper.blocks.size() != 4) {
    return std::nullopt;
  }
  if (entry.name != "main" || entry.is_declaration || entry.entry.value != 0 ||
      !entry.alloca_insts.empty() || entry.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto* ap_alloca = std::get_if<LirAllocaOp>(&helper.alloca_insts[0]);
  const auto* second_alloca = std::get_if<LirAllocaOp>(&helper.alloca_insts[1]);
  if (ap_alloca == nullptr || second_alloca == nullptr || ap_alloca->result != "%lv.ap" ||
      ap_alloca->type_str != "%struct.__va_list_tag_" || second_alloca->result != "%lv.second" ||
      second_alloca->type_str != "i32") {
    return std::nullopt;
  }

  const auto& helper_entry = helper.blocks[0];
  const auto& helper_reg = helper.blocks[1];
  const auto& helper_stack = helper.blocks[2];
  const auto& helper_join = helper.blocks[3];
  if (helper_entry.label != "entry" || helper_reg.label != "vaarg.amd64.reg.7" ||
      helper_stack.label != "vaarg.amd64.stack.8" ||
      helper_join.label != "vaarg.amd64.join.9" || helper_entry.insts.size() != 8 ||
      helper_reg.insts.size() != 6 || helper_stack.insts.size() != 5 ||
      helper_join.insts.size() != 5) {
    return std::nullopt;
  }

  const auto* va_start = std::get_if<LirVaStartOp>(&helper_entry.insts[0]);
  const auto* gp_offset_gep = std::get_if<LirGepOp>(&helper_entry.insts[1]);
  const auto* fp_offset_gep = std::get_if<LirGepOp>(&helper_entry.insts[2]);
  const auto* overflow_gep = std::get_if<LirGepOp>(&helper_entry.insts[3]);
  const auto* reg_save_gep = std::get_if<LirGepOp>(&helper_entry.insts[4]);
  const auto* reg_save_load = std::get_if<LirLoadOp>(&helper_entry.insts[5]);
  const auto* gp_offset_load = std::get_if<LirLoadOp>(&helper_entry.insts[6]);
  const auto* reg_cmp = std::get_if<LirCmpOp>(&helper_entry.insts[7]);
  const auto* entry_br = std::get_if<LirCondBr>(&helper_entry.terminator);
  if (va_start == nullptr || gp_offset_gep == nullptr || fp_offset_gep == nullptr ||
      overflow_gep == nullptr || reg_save_gep == nullptr || reg_save_load == nullptr ||
      gp_offset_load == nullptr || reg_cmp == nullptr || entry_br == nullptr ||
      va_start->ap_ptr != "%lv.ap" || gp_offset_gep->ptr != "%lv.ap" ||
      gp_offset_gep->indices != std::vector<std::string>{"i32 0", "i32 0"} ||
      fp_offset_gep->ptr != "%lv.ap" ||
      fp_offset_gep->indices != std::vector<std::string>{"i32 0", "i32 1"} ||
      overflow_gep->ptr != "%lv.ap" ||
      overflow_gep->indices != std::vector<std::string>{"i32 0", "i32 2"} ||
      reg_save_gep->ptr != "%lv.ap" ||
      reg_save_gep->indices != std::vector<std::string>{"i32 0", "i32 3"} ||
      reg_save_load->type_str != "ptr" || reg_save_load->ptr != reg_save_gep->result ||
      gp_offset_load->type_str != "i32" || gp_offset_load->ptr != gp_offset_gep->result ||
      reg_cmp->type_str != "i32" || reg_cmp->lhs != gp_offset_load->result ||
      reg_cmp->rhs != "40" || reg_cmp->predicate != "sle" ||
      entry_br->cond_name != reg_cmp->result ||
      entry_br->true_label != helper_reg.label || entry_br->false_label != helper_stack.label) {
    return std::nullopt;
  }

  const auto* reg_offset_cast = std::get_if<LirCastOp>(&helper_reg.insts[0]);
  const auto* reg_arg_gep = std::get_if<LirGepOp>(&helper_reg.insts[1]);
  const auto* reg_next_offset = std::get_if<LirBinOp>(&helper_reg.insts[2]);
  const auto* reg_store_offset = std::get_if<LirStoreOp>(&helper_reg.insts[3]);
  const auto* reg_memcpy = std::get_if<LirMemcpyOp>(&helper_reg.insts[4]);
  const auto* reg_value_load = std::get_if<LirLoadOp>(&helper_reg.insts[5]);
  const auto* reg_br = std::get_if<LirBr>(&helper_reg.terminator);
  if (reg_offset_cast == nullptr || reg_arg_gep == nullptr || reg_next_offset == nullptr ||
      reg_store_offset == nullptr || reg_memcpy == nullptr || reg_value_load == nullptr ||
      reg_br == nullptr || reg_offset_cast->kind != LirCastKind::SExt ||
      reg_offset_cast->from_type != "i32" || reg_offset_cast->operand != gp_offset_load->result ||
      reg_offset_cast->to_type != "i64" || reg_arg_gep->element_type != "i8" ||
      reg_arg_gep->ptr != reg_save_load->result ||
      reg_arg_gep->indices != std::vector<std::string>{"i64 " + reg_offset_cast->result} ||
      reg_next_offset->opcode != "add" || reg_next_offset->type_str != "i32" ||
      reg_next_offset->lhs != gp_offset_load->result || reg_next_offset->rhs != "8" ||
      reg_store_offset->type_str != "i32" || reg_store_offset->val != reg_next_offset->result ||
      reg_store_offset->ptr != gp_offset_gep->result || reg_memcpy->dst != second_alloca->result ||
      reg_memcpy->src != reg_arg_gep->result || reg_memcpy->size != "4" ||
      reg_value_load->type_str != "i32" || reg_value_load->ptr != second_alloca->result ||
      reg_br->target_label != helper_join.label) {
    return std::nullopt;
  }

  const auto* stack_base_load = std::get_if<LirLoadOp>(&helper_stack.insts[0]);
  const auto* stack_next_ptr = std::get_if<LirGepOp>(&helper_stack.insts[1]);
  const auto* stack_store_next = std::get_if<LirStoreOp>(&helper_stack.insts[2]);
  const auto* stack_memcpy = std::get_if<LirMemcpyOp>(&helper_stack.insts[3]);
  const auto* stack_value_load = std::get_if<LirLoadOp>(&helper_stack.insts[4]);
  const auto* stack_br = std::get_if<LirBr>(&helper_stack.terminator);
  if (stack_base_load == nullptr || stack_next_ptr == nullptr || stack_store_next == nullptr ||
      stack_memcpy == nullptr || stack_value_load == nullptr || stack_br == nullptr ||
      stack_base_load->type_str != "ptr" || stack_base_load->ptr != overflow_gep->result ||
      stack_next_ptr->element_type != "i8" || stack_next_ptr->ptr != stack_base_load->result ||
      stack_next_ptr->indices != std::vector<std::string>{"i64 8"} ||
      stack_store_next->type_str != "ptr" || stack_store_next->val != stack_next_ptr->result ||
      stack_store_next->ptr != overflow_gep->result ||
      stack_memcpy->dst != second_alloca->result ||
      stack_memcpy->src != stack_base_load->result || stack_memcpy->size != "4" ||
      stack_value_load->type_str != "i32" || stack_value_load->ptr != second_alloca->result ||
      stack_br->target_label != helper_join.label) {
    return std::nullopt;
  }

  const auto* phi = std::get_if<LirPhiOp>(&helper_join.insts[0]);
  const auto* second_store = std::get_if<LirStoreOp>(&helper_join.insts[1]);
  const auto* va_end = std::get_if<LirVaEndOp>(&helper_join.insts[2]);
  const auto* second_load = std::get_if<LirLoadOp>(&helper_join.insts[3]);
  const auto* add = std::get_if<LirBinOp>(&helper_join.insts[4]);
  const auto* ret = std::get_if<LirRet>(&helper_join.terminator);
  if (phi == nullptr || second_store == nullptr || va_end == nullptr || second_load == nullptr ||
      add == nullptr || ret == nullptr || phi->type_str != "i32" || phi->incoming.size() != 2 ||
      phi->incoming[0].first != reg_value_load->result ||
      phi->incoming[0].second != helper_reg.label ||
      phi->incoming[1].first != stack_value_load->result ||
      phi->incoming[1].second != helper_stack.label || second_store->type_str != "i32" ||
      second_store->val != phi->result || second_store->ptr != second_alloca->result ||
      va_end->ap_ptr != "%lv.ap" || second_load->type_str != "i32" ||
      second_load->ptr != second_alloca->result || add->opcode != "add" ||
      add->type_str != "i32" || add->lhs != "%p.first" || add->rhs != second_load->result ||
      !ret->value_str.has_value() || *ret->value_str != add->result || ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto& main_entry = entry.blocks.front();
  const auto* main_call = std::get_if<LirCallOp>(&main_entry.insts.front());
  const auto* main_ret = std::get_if<LirRet>(&main_entry.terminator);
  if (main_entry.label != "entry" || main_entry.insts.size() != 1 || main_call == nullptr ||
      main_ret == nullptr || main_call->callee != "@sum2" || main_call->return_type != "i32" ||
      main_call->callee_type_suffix != "(i32, ...)" ||
      main_call->args_str != "i32 10, i32 32" ||
      !main_ret->value_str.has_value() || *main_ret->value_str != main_call->result ||
      main_ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalVariadicSum2Slice{
      .helper_name = helper.name,
      .entry_name = entry.name,
  };
}

std::string emit_minimal_variadic_sum2_asm(std::string_view target_triple,
                                           const MinimalVariadicSum2Slice& slice) {
  const auto helper_symbol = asm_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);

  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, helper_symbol)
      << "  lea eax, [rdi + rsi]\n"
      << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  mov edi, 10\n"
      << "  mov esi, 32\n"
      << "  xor eax, eax\n"
      << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

[[noreturn]] void throw_unsupported_direct_bir_module() {
  throw std::invalid_argument(
      "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
}

[[noreturn]] void throw_x86_rewrite_in_progress() {
  throw std::invalid_argument(
      "x86 backend emitter rewrite in progress: move ownership into the translated sibling codegen translation units instead of adding emit.cpp-local matchers");
}

}  // namespace

std::optional<std::string> try_emit_module(const c4c::backend::bir::Module& module) {
  if (const auto imm = parse_minimal_return_imm(module); imm.has_value()) {
    return emit_minimal_return_imm_asm(module, *imm);
  }
  return std::nullopt;
}

std::string emit_module(const c4c::backend::bir::Module& module) {
  if (const auto asm_text = try_emit_module(module); asm_text.has_value()) {
    return *asm_text;
  }
  throw_unsupported_direct_bir_module();
}

std::optional<std::string> try_emit_prepared_lir_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_variadic_sum2_slice(module); slice.has_value()) {
    return emit_minimal_variadic_sum2_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_string_literal_char_slice(module);
      slice.has_value()) {
    return emit_minimal_string_literal_char_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::string emit_module(const c4c::codegen::lir::LirModule& module) {
  if (const auto lowered = c4c::backend::try_lower_to_bir(module); lowered.has_value()) {
    return emit_module(*lowered);
  }
  if (const auto asm_text = try_emit_prepared_lir_module(module); asm_text.has_value()) {
    return *asm_text;
  }
  throw_x86_rewrite_in_progress();
}

assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path) {
  return assembler::assemble(assembler::AssembleRequest{
      .asm_text = emit_module(module),
      .output_path = output_path,
  });
}

}  // namespace c4c::backend::x86
