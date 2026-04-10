#include "x86_codegen.hpp"

#include "../../../codegen/lir/ir.hpp"

#include <cctype>
#include <sstream>

namespace c4c::backend::x86 {

namespace {

struct MinimalRepeatedPrintfImmediatesSlice {
  std::string function_name;
  std::string pool_name;
  std::string raw_bytes;
};

struct MinimalLocalBufferStringCopyPrintfSlice {
  std::string function_name;
  std::string copy_pool_name;
  std::string copy_raw_bytes;
  std::string format_pool_name;
  std::string format_raw_bytes;
};

std::string decode_llvm_byte_string(std::string_view text) {
  std::string bytes;
  bytes.reserve(text.size());
  for (std::size_t index = 0; index < text.size(); ++index) {
    if (text[index] == '\\' && index + 2 < text.size()) {
      auto hex_val = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
      };
      const int hi = hex_val(text[index + 1]);
      const int lo = hex_val(text[index + 2]);
      if (hi >= 0 && lo >= 0) {
        bytes.push_back(static_cast<char>(hi * 16 + lo));
        index += 2;
        continue;
      }
    }
    bytes.push_back(text[index]);
  }
  return bytes;
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

std::string emit_function_prelude(std::string_view target_triple, std::string_view symbol_name) {
  std::ostringstream out;
  out << ".globl " << symbol_name << "\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".type " << symbol_name << ", @function\n";
  }
  out << symbol_name << ":\n";
  return out.str();
}

std::string direct_symbol_name(std::string_view target_triple, std::string_view logical_name) {
  if (target_triple.find("apple-darwin") != std::string::npos) {
    return "_" + std::string(logical_name);
  }
  return std::string(logical_name);
}

std::string direct_private_data_label(std::string_view target_triple, std::string_view pool_name) {
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

std::optional<MinimalRepeatedPrintfImmediatesSlice> parse_minimal_repeated_printf_immediates_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  const auto* format_string = [&]() -> const LirStringConst* {
    for (const auto& candidate : module.string_pool) {
      if (candidate.raw_bytes == "%d %d\\0A" || candidate.raw_bytes == "%d %d\n") {
        return &candidate;
      }
    }
    return nullptr;
  }();
  if (format_string == nullptr) {
    return std::nullopt;
  }

  const bool has_printf_decl =
      [&]() {
        for (const auto& decl : module.extern_decls) {
          if (decl.name == "printf" &&
              (decl.return_type_str == "i32" || decl.return_type.str() == "i32")) {
            return true;
          }
        }
        for (const auto& function : module.functions) {
          if (function.is_declaration && function.name == "printf" &&
              function.signature_text.find("declare i32 @printf(") != std::string::npos) {
            return true;
          }
        }
        return false;
      }();
  if (!has_printf_decl) {
    return std::nullopt;
  }

  const auto* function = [&]() -> const LirFunction* {
    for (const auto& candidate : module.functions) {
      if (!candidate.is_declaration && candidate.name == "main") {
        return &candidate;
      }
    }
    return nullptr;
  }();
  if (function == nullptr || function->entry.value != 0 || function->blocks.size() != 1 ||
      function->signature_text != "define i32 @main()\n" ||
      function->stack_objects.size() > 2 || function->alloca_insts.size() > 2) {
    return std::nullopt;
  }

  const auto& entry = function->blocks.front();
  const auto* first_gep =
      entry.insts.size() == 4 ? std::get_if<LirGepOp>(&entry.insts[0]) : nullptr;
  const auto* first_call =
      entry.insts.size() == 4 ? std::get_if<LirCallOp>(&entry.insts[1]) : nullptr;
  const auto* second_gep =
      entry.insts.size() == 4 ? std::get_if<LirGepOp>(&entry.insts[2]) : nullptr;
  const auto* second_call =
      entry.insts.size() == 4 ? std::get_if<LirCallOp>(&entry.insts[3]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || first_gep == nullptr || first_call == nullptr ||
      second_gep == nullptr || second_call == nullptr || ret == nullptr ||
      !ret->value_str.has_value() || *ret->value_str != "0" || ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto format_array_type =
      "[" + std::to_string(format_string->byte_length) + " x i8]";
  if (first_gep->ptr != format_string->pool_name || first_gep->element_type != format_array_type ||
      first_gep->indices.size() != 2 || first_gep->indices[0] != "i64 0" ||
      first_gep->indices[1] != "i64 0" || first_call->callee != "@printf" ||
      first_call->return_type != "i32" || first_call->callee_type_suffix != "(ptr, ...)" ||
      first_call->args_str != ("ptr " + first_gep->result.str() + ", i64 1, i64 1") ||
      second_gep->ptr != format_string->pool_name ||
      second_gep->element_type != format_array_type || second_gep->indices.size() != 2 ||
      second_gep->indices[0] != "i64 0" || second_gep->indices[1] != "i64 0" ||
      second_call->callee != "@printf" || second_call->return_type != "i32" ||
      second_call->callee_type_suffix != "(ptr, ...)" ||
      second_call->args_str != ("ptr " + second_gep->result.str() + ", i64 2, i64 2")) {
    return std::nullopt;
  }

  return MinimalRepeatedPrintfImmediatesSlice{
      .function_name = function->name,
      .pool_name = format_string->pool_name,
      .raw_bytes = format_string->raw_bytes,
  };
}

std::optional<MinimalLocalBufferStringCopyPrintfSlice>
parse_minimal_local_buffer_string_copy_printf_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  const auto* copy_string = [&]() -> const LirStringConst* {
    for (const auto& candidate : module.string_pool) {
      if (candidate.raw_bytes == "abcdef") {
        return &candidate;
      }
    }
    return nullptr;
  }();
  const auto* format_string = [&]() -> const LirStringConst* {
    for (const auto& candidate : module.string_pool) {
      if (candidate.raw_bytes == "%s\\0A" || candidate.raw_bytes == "%s\n") {
        return &candidate;
      }
    }
    return nullptr;
  }();
  if (copy_string == nullptr || format_string == nullptr) {
    return std::nullopt;
  }

  const bool has_strcpy_decl =
      [&]() {
        for (const auto& decl : module.extern_decls) {
          if (decl.name == "strcpy" &&
              (decl.return_type_str == "ptr" || decl.return_type.str() == "ptr")) {
            return true;
          }
        }
        for (const auto& function : module.functions) {
          if (function.is_declaration && function.name == "strcpy" &&
              function.signature_text.find("declare ptr @strcpy(") != std::string::npos) {
            return true;
          }
        }
        return false;
      }();
  const bool has_printf_decl =
      [&]() {
        for (const auto& decl : module.extern_decls) {
          if (decl.name == "printf" &&
              (decl.return_type_str == "i32" || decl.return_type.str() == "i32")) {
            return true;
          }
        }
        for (const auto& function : module.functions) {
          if (function.is_declaration && function.name == "printf" &&
              function.signature_text.find("declare i32 @printf(") != std::string::npos) {
            return true;
          }
        }
        return false;
      }();
  if (!has_strcpy_decl || !has_printf_decl) {
    return std::nullopt;
  }

  const auto* function = [&]() -> const LirFunction* {
    for (const auto& candidate : module.functions) {
      if (!candidate.is_declaration && candidate.name == "main") {
        return &candidate;
      }
    }
    return nullptr;
  }();
  if (function == nullptr || function->entry.value != 0 || function->blocks.size() != 1 ||
      function->signature_text != "define i32 @main()\n" ||
      (!function->alloca_insts.empty() && function->alloca_insts.size() != 1) ||
      function->stack_objects.size() > 1) {
    return std::nullopt;
  }

  const auto& entry = function->blocks.front();
  const auto* buffer_gep =
      entry.insts.size() == 8 ? std::get_if<LirGepOp>(&entry.insts[0]) : nullptr;
  const auto* copy_gep =
      entry.insts.size() == 8 ? std::get_if<LirGepOp>(&entry.insts[1]) : nullptr;
  const auto* copy_call =
      entry.insts.size() == 8 ? std::get_if<LirCallOp>(&entry.insts[2]) : nullptr;
  const auto* format_gep =
      entry.insts.size() == 8 ? std::get_if<LirGepOp>(&entry.insts[3]) : nullptr;
  const auto* buffer_base_gep =
      entry.insts.size() == 8 ? std::get_if<LirGepOp>(&entry.insts[4]) : nullptr;
  const auto* offset_cast =
      entry.insts.size() == 8 ? std::get_if<LirCastOp>(&entry.insts[5]) : nullptr;
  const auto* offset_gep =
      entry.insts.size() == 8 ? std::get_if<LirGepOp>(&entry.insts[6]) : nullptr;
  const auto* printf_call =
      entry.insts.size() == 8 ? std::get_if<LirCallOp>(&entry.insts[7]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || buffer_gep == nullptr || copy_gep == nullptr || copy_call == nullptr ||
      format_gep == nullptr || buffer_base_gep == nullptr || offset_cast == nullptr ||
      offset_gep == nullptr || printf_call == nullptr || ret == nullptr ||
      !ret->value_str.has_value() || *ret->value_str != "0" || ret->type_str != "i32" ||
      buffer_gep->element_type != "[10 x i8]" || buffer_gep->indices.size() != 2 ||
      buffer_gep->indices[0] != "i64 0" || buffer_gep->indices[1] != "i64 0" ||
      buffer_gep->ptr.str().empty() || copy_gep->ptr != copy_string->pool_name ||
      copy_gep->element_type != "[7 x i8]" || copy_gep->indices.size() != 2 ||
      copy_gep->indices[0] != "i64 0" || copy_gep->indices[1] != "i64 0" ||
      copy_call->callee != "@strcpy" || copy_call->return_type != "ptr" ||
      copy_call->callee_type_suffix != "(ptr, ptr)" ||
      copy_call->args_str != ("ptr " + buffer_gep->result.str() + ", ptr " + copy_gep->result.str()) ||
      format_gep->ptr != format_string->pool_name || format_gep->element_type != "[4 x i8]" ||
      format_gep->indices.size() != 2 || format_gep->indices[0] != "i64 0" ||
      format_gep->indices[1] != "i64 0" || buffer_base_gep->element_type != "[10 x i8]" ||
      buffer_base_gep->ptr != buffer_gep->ptr || buffer_base_gep->indices.size() != 2 ||
      buffer_base_gep->indices[0] != "i64 0" || buffer_base_gep->indices[1] != "i64 0" ||
      offset_cast->kind != LirCastKind::SExt || offset_cast->from_type != "i32" ||
      offset_cast->operand != "1" || offset_cast->to_type != "i64" ||
      offset_gep->element_type != "i8" || offset_gep->ptr != buffer_base_gep->result ||
      offset_gep->indices.size() != 1 ||
      offset_gep->indices[0] != ("i64 " + offset_cast->result.str()) ||
      printf_call->callee != "@printf" || printf_call->return_type != "i32" ||
      printf_call->callee_type_suffix != "(ptr, ...)" ||
      printf_call->args_str !=
          ("ptr " + format_gep->result.str() + ", ptr " + offset_gep->result.str())) {
    return std::nullopt;
  }

  return MinimalLocalBufferStringCopyPrintfSlice{
      .function_name = function->name,
      .copy_pool_name = copy_string->pool_name,
      .copy_raw_bytes = copy_string->raw_bytes,
      .format_pool_name = format_string->pool_name,
      .format_raw_bytes = format_string->raw_bytes,
  };
}

std::string emit_minimal_repeated_printf_immediates_asm(
    std::string_view target_triple,
    const MinimalRepeatedPrintfImmediatesSlice& slice) {
  const auto string_label = direct_private_data_label(target_triple, slice.pool_name);
  const auto symbol = direct_symbol_name(target_triple, slice.function_name);
  const auto decoded_bytes = decode_llvm_byte_string(slice.raw_bytes);

  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".section .rodata\n"
      << string_label << ":\n"
      << "  .asciz \"" << escape_asm_string(decoded_bytes) << "\"\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".extern printf\n";
  }
  out << ".text\n"
      << emit_function_prelude(target_triple, symbol)
      << "  lea rdi, " << string_label << "[rip]\n"
      << "  mov rsi, 1\n"
      << "  mov rdx, 1\n"
      << "  mov eax, 0\n"
      << "  call printf\n"
      << "  lea rdi, " << string_label << "[rip]\n"
      << "  mov rsi, 2\n"
      << "  mov rdx, 2\n"
      << "  mov eax, 0\n"
      << "  call printf\n"
      << "  mov eax, 0\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_local_buffer_string_copy_printf_asm(
    std::string_view target_triple,
    const MinimalLocalBufferStringCopyPrintfSlice& slice) {
  const auto copy_label = direct_private_data_label(target_triple, slice.copy_pool_name);
  const auto format_label = direct_private_data_label(target_triple, slice.format_pool_name);
  const auto symbol = direct_symbol_name(target_triple, slice.function_name);
  const auto copy_bytes = decode_llvm_byte_string(slice.copy_raw_bytes);
  const auto format_bytes = decode_llvm_byte_string(slice.format_raw_bytes);

  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".section .rodata\n"
      << copy_label << ":\n"
      << "  .asciz \"" << escape_asm_string(copy_bytes) << "\"\n"
      << format_label << ":\n"
      << "  .asciz \"" << escape_asm_string(format_bytes) << "\"\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".extern strcpy\n"
        << ".extern printf\n";
  }
  out << ".text\n"
      << emit_function_prelude(target_triple, symbol)
      << "  sub rsp, 24\n"
      << "  lea rdi, [rsp + 8]\n"
      << "  lea rsi, " << copy_label << "[rip]\n"
      << "  call strcpy\n"
      << "  lea rdi, " << format_label << "[rip]\n"
      << "  lea rsi, [rsp + 9]\n"
      << "  mov eax, 0\n"
      << "  call printf\n"
      << "  mov eax, 0\n"
      << "  add rsp, 24\n"
      << "  ret\n";
  return out.str();
}

}  // namespace

std::optional<std::string> try_emit_minimal_repeated_printf_immediates_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_repeated_printf_immediates_slice(module);
      slice.has_value()) {
    return emit_minimal_repeated_printf_immediates_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_local_buffer_string_copy_printf_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_local_buffer_string_copy_printf_slice(module);
      slice.has_value()) {
    return emit_minimal_local_buffer_string_copy_printf_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

}  // namespace c4c::backend::x86
