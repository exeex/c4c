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
  out << ".text\n"
      << ".globl " << symbol_name << "\n";
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
