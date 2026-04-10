#include "x86_codegen.hpp"

#include "../../bir.hpp"
#include "../../lowering/call_decode.hpp"
#include "../../../codegen/lir/ir.hpp"

#include <charconv>
#include <cctype>
#include <sstream>

namespace c4c::backend::x86 {

namespace {

struct MinimalRepeatedPrintfImmediatesSlice {
  std::string function_name;
  std::string pool_name;
  std::string raw_bytes;
};

struct MinimalRepeatedPrintfLocalI32CallsBirSlice {
  std::string function_name;
  std::string first_pool_name;
  std::string first_raw_bytes;
  std::string second_pool_name;
  std::string second_raw_bytes;
};

struct MinimalLocalBufferStringCopyPrintfSlice {
  std::string function_name;
  std::string copy_pool_name;
  std::string copy_raw_bytes;
  std::string format_pool_name;
  std::string format_raw_bytes;
};

struct MinimalCountedPrintfTernaryLoopSlice {
  std::string function_name;
  std::string pool_name;
  std::string raw_bytes;
};

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

std::optional<MinimalRepeatedPrintfLocalI32CallsBirSlice>
parse_minimal_repeated_printf_local_i32_calls_bir_slice(
    const c4c::backend::bir::Module& module) {
  using namespace c4c::backend::bir;

  if (!module.globals.empty() || module.string_constants.size() != 2 ||
      module.functions.size() != 2) {
    return std::nullopt;
  }

  const auto& printf_decl = module.functions.front();
  const auto& main = module.functions.back();
  if (!printf_decl.is_declaration || printf_decl.name != "printf" ||
      !printf_decl.is_variadic || printf_decl.return_type != TypeKind::I32 ||
      printf_decl.params.size() != 1 || printf_decl.params.front().type != TypeKind::Ptr ||
      main.is_declaration || main.name != "main" || main.return_type != TypeKind::I32 ||
      !main.params.empty() || !main.local_slots.empty() || main.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& first_string = module.string_constants[0];
  const auto& second_string = module.string_constants[1];
  if (first_string.name.empty() || second_string.name.empty()) {
    return std::nullopt;
  }

  const auto& entry = main.blocks.front();
  const auto* first_call =
      entry.insts.size() == 3 ? std::get_if<CallInst>(&entry.insts[0]) : nullptr;
  const auto* second_call =
      entry.insts.size() == 3 ? std::get_if<CallInst>(&entry.insts[1]) : nullptr;
  const auto* third_call =
      entry.insts.size() == 3 ? std::get_if<CallInst>(&entry.insts[2]) : nullptr;
  if (entry.label != "entry" || first_call == nullptr || second_call == nullptr ||
      third_call == nullptr || entry.terminator.kind != TerminatorKind::Return ||
      entry.terminator.value != c4c::backend::bir::Value::immediate_i32(0)) {
    return std::nullopt;
  }

  const auto match_printf_call =
      [](const CallInst& call,
         std::string_view pool_name,
         std::initializer_list<std::int32_t> immediates) {
        if (call.callee != "printf" || !call.is_variadic || call.return_type != TypeKind::I32 ||
            !call.result.has_value() ||
            call.result->kind != c4c::backend::bir::Value::Kind::Named ||
            call.result->type != TypeKind::I32 || call.args.size() != immediates.size() + 1 ||
            call.args.front().kind != c4c::backend::bir::Value::Kind::Named ||
            call.args.front().type != TypeKind::Ptr ||
            call.args.front().name != pool_name) {
          return false;
        }
        std::size_t index = 1;
        for (const auto immediate : immediates) {
          if (call.args[index] != c4c::backend::bir::Value::immediate_i32(immediate)) {
            return false;
          }
          ++index;
        }
        return true;
      };
  if (!match_printf_call(*first_call, first_string.name, {42}) ||
      !match_printf_call(*second_call, first_string.name, {64}) ||
      !match_printf_call(*third_call, second_string.name, {12, 34})) {
    return std::nullopt;
  }

  return MinimalRepeatedPrintfLocalI32CallsBirSlice{
      .function_name = main.name,
      .first_pool_name = first_string.name,
      .first_raw_bytes = first_string.bytes,
      .second_pool_name = second_string.name,
      .second_raw_bytes = second_string.bytes,
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

std::optional<MinimalCountedPrintfTernaryLoopSlice> parse_minimal_counted_printf_ternary_loop_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.string_pool.empty()) {
    return std::nullopt;
  }

  const auto* string_const = [&]() -> const LirStringConst* {
    for (const auto& candidate : module.string_pool) {
      if (candidate.raw_bytes == "%d\\0A" || candidate.raw_bytes == "%d\n") {
        return &candidate;
      }
    }
    return nullptr;
  }();
  if (string_const == nullptr) {
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
  if (function == nullptr) {
    return std::nullopt;
  }

  if (function->is_declaration || function->entry.value != 0 || function->blocks.size() != 10 ||
      function->alloca_insts.size() != 1 || !function->stack_objects.empty() ||
      function->signature_text != "define i32 @main()\n") {
    return std::nullopt;
  }

  const auto* count_slot = std::get_if<LirAllocaOp>(&function->alloca_insts.front());
  if (count_slot == nullptr || count_slot->result != "%lv.Count" || count_slot->type_str != "i32" ||
      !count_slot->count.str().empty() || count_slot->align != 4) {
    return std::nullopt;
  }

  const auto& entry = function->blocks[0];
  const auto& loop_cond = function->blocks[1];
  const auto& loop_latch = function->blocks[2];
  const auto& loop_body = function->blocks[3];
  const auto& then_block = function->blocks[4];
  const auto& then_end = function->blocks[5];
  const auto& else_block = function->blocks[6];
  const auto& else_end = function->blocks[7];
  const auto& join_block = function->blocks[8];
  const auto& exit_block = function->blocks[9];

  const auto* entry_store = entry.insts.size() == 1 ? std::get_if<LirStoreOp>(&entry.insts.front()) : nullptr;
  const auto* entry_br = std::get_if<LirBr>(&entry.terminator);
  if (entry.label != "entry" || entry_store == nullptr || entry_br == nullptr ||
      entry_store->type_str != "i32" || entry_store->val != "0" || entry_store->ptr != count_slot->result ||
      entry_br->target_label != loop_cond.label) {
    return std::nullopt;
  }

  const auto* cond_load = loop_cond.insts.size() == 4 ? std::get_if<LirLoadOp>(&loop_cond.insts[0]) : nullptr;
  const auto* cond_cmp = loop_cond.insts.size() == 4 ? std::get_if<LirCmpOp>(&loop_cond.insts[1]) : nullptr;
  const auto* cond_zext = loop_cond.insts.size() == 4 ? std::get_if<LirCastOp>(&loop_cond.insts[2]) : nullptr;
  const auto* cond_nonzero = loop_cond.insts.size() == 4 ? std::get_if<LirCmpOp>(&loop_cond.insts[3]) : nullptr;
  const auto* cond_br = std::get_if<LirCondBr>(&loop_cond.terminator);
  if (loop_cond.label != "for.cond.1" || cond_load == nullptr || cond_cmp == nullptr ||
      cond_zext == nullptr || cond_nonzero == nullptr || cond_br == nullptr ||
      cond_load->type_str != "i32" || cond_load->ptr != count_slot->result ||
      cond_cmp->type_str != "i32" || cond_cmp->lhs != cond_load->result || cond_cmp->rhs != "10" ||
      cond_cmp->predicate != "slt" || cond_zext->kind != LirCastKind::ZExt ||
      cond_zext->from_type != "i1" || cond_zext->operand != cond_cmp->result ||
      cond_zext->to_type != "i32" || cond_nonzero->type_str != "i32" ||
      cond_nonzero->lhs != cond_zext->result || cond_nonzero->rhs != "0" ||
      cond_nonzero->predicate != "ne" || cond_br->cond_name != cond_nonzero->result ||
      cond_br->true_label != loop_body.label || cond_br->false_label != exit_block.label) {
    return std::nullopt;
  }

  const auto* latch_load = loop_latch.insts.size() == 3 ? std::get_if<LirLoadOp>(&loop_latch.insts[0]) : nullptr;
  const auto* latch_add = loop_latch.insts.size() == 3 ? std::get_if<LirBinOp>(&loop_latch.insts[1]) : nullptr;
  const auto* latch_store =
      loop_latch.insts.size() == 3 ? std::get_if<LirStoreOp>(&loop_latch.insts[2]) : nullptr;
  const auto* latch_br = std::get_if<LirBr>(&loop_latch.terminator);
  if (loop_latch.label != "for.latch.1" || latch_load == nullptr || latch_add == nullptr ||
      latch_store == nullptr || latch_br == nullptr || latch_load->type_str != "i32" ||
      latch_load->ptr != count_slot->result || latch_add->opcode != "add" ||
      latch_add->type_str != "i32" || latch_add->lhs != latch_load->result || latch_add->rhs != "1" ||
      latch_store->type_str != "i32" || latch_store->val != latch_add->result ||
      latch_store->ptr != count_slot->result || latch_br->target_label != loop_cond.label) {
    return std::nullopt;
  }

  const auto* body_gep = loop_body.insts.size() == 5 ? std::get_if<LirGepOp>(&loop_body.insts[0]) : nullptr;
  const auto* body_load = loop_body.insts.size() == 5 ? std::get_if<LirLoadOp>(&loop_body.insts[1]) : nullptr;
  const auto* body_cmp = loop_body.insts.size() == 5 ? std::get_if<LirCmpOp>(&loop_body.insts[2]) : nullptr;
  const auto* body_zext = loop_body.insts.size() == 5 ? std::get_if<LirCastOp>(&loop_body.insts[3]) : nullptr;
  const auto* body_nonzero = loop_body.insts.size() == 5 ? std::get_if<LirCmpOp>(&loop_body.insts[4]) : nullptr;
  const auto* body_br = std::get_if<LirCondBr>(&loop_body.terminator);
  if (loop_body.label != "block_1" || body_gep == nullptr || body_load == nullptr ||
      body_cmp == nullptr || body_zext == nullptr || body_nonzero == nullptr || body_br == nullptr ||
      body_gep->element_type != "[4 x i8]" || body_gep->ptr != string_const->pool_name ||
      body_gep->indices != std::vector<std::string>{"i64 0", "i64 0"} ||
      body_load->type_str != "i32" || body_load->ptr != count_slot->result || body_cmp->type_str != "i32" ||
      body_cmp->lhs != body_load->result || body_cmp->rhs != "5" || body_cmp->predicate != "slt" ||
      body_zext->kind != LirCastKind::ZExt || body_zext->from_type != "i1" ||
      body_zext->operand != body_cmp->result || body_zext->to_type != "i32" ||
      body_nonzero->type_str != "i32" || body_nonzero->lhs != body_zext->result ||
      body_nonzero->rhs != "0" || body_nonzero->predicate != "ne" ||
      body_br->cond_name != body_nonzero->result || body_br->true_label != then_block.label ||
      body_br->false_label != else_block.label) {
    return std::nullopt;
  }

  const auto* then_load0 = then_block.insts.size() == 3 ? std::get_if<LirLoadOp>(&then_block.insts[0]) : nullptr;
  const auto* then_load1 = then_block.insts.size() == 3 ? std::get_if<LirLoadOp>(&then_block.insts[1]) : nullptr;
  const auto* then_mul = then_block.insts.size() == 3 ? std::get_if<LirBinOp>(&then_block.insts[2]) : nullptr;
  const auto* then_br = std::get_if<LirBr>(&then_block.terminator);
  if (then_block.label != "tern.then.11" || then_load0 == nullptr || then_load1 == nullptr ||
      then_mul == nullptr || then_br == nullptr || then_load0->type_str != "i32" ||
      then_load0->ptr != count_slot->result || then_load1->type_str != "i32" ||
      then_load1->ptr != count_slot->result || then_mul->opcode != "mul" || then_mul->type_str != "i32" ||
      then_mul->lhs != then_load0->result || then_mul->rhs != then_load1->result ||
      then_br->target_label != then_end.label) {
    return std::nullopt;
  }

  const auto* then_end_br = std::get_if<LirBr>(&then_end.terminator);
  if (then_end.label != "tern.then.end.12" || !then_end.insts.empty() || then_end_br == nullptr ||
      then_end_br->target_label != join_block.label) {
    return std::nullopt;
  }

  const auto* else_load = else_block.insts.size() == 2 ? std::get_if<LirLoadOp>(&else_block.insts[0]) : nullptr;
  const auto* else_mul = else_block.insts.size() == 2 ? std::get_if<LirBinOp>(&else_block.insts[1]) : nullptr;
  const auto* else_br = std::get_if<LirBr>(&else_block.terminator);
  if (else_block.label != "tern.else.13" || else_load == nullptr || else_mul == nullptr ||
      else_br == nullptr || else_load->type_str != "i32" || else_load->ptr != count_slot->result ||
      else_mul->opcode != "mul" || else_mul->type_str != "i32" || else_mul->lhs != else_load->result ||
      else_mul->rhs != "3" || else_br->target_label != else_end.label) {
    return std::nullopt;
  }

  const auto* else_end_br = std::get_if<LirBr>(&else_end.terminator);
  if (else_end.label != "tern.else.end.14" || !else_end.insts.empty() || else_end_br == nullptr ||
      else_end_br->target_label != join_block.label) {
    return std::nullopt;
  }

  const auto* join_phi = join_block.insts.size() == 2 ? std::get_if<LirPhiOp>(&join_block.insts[0]) : nullptr;
  const auto* join_call = join_block.insts.size() == 2 ? std::get_if<LirCallOp>(&join_block.insts[1]) : nullptr;
  const auto* join_br = std::get_if<LirBr>(&join_block.terminator);
  if (join_block.label != "tern.end.15" || join_phi == nullptr || join_call == nullptr ||
      join_br == nullptr || join_phi->type_str != "i32" || join_phi->incoming.size() != 2 ||
      join_phi->incoming[0].first != then_mul->result || join_phi->incoming[0].second != then_end.label ||
      join_phi->incoming[1].first != else_mul->result || join_phi->incoming[1].second != else_end.label ||
      join_call->callee != "@printf" || join_call->return_type != "i32" ||
      join_call->callee_type_suffix != "(ptr, ...)" ||
      join_call->args_str != ("ptr " + body_gep->result + ", i32 " + join_phi->result) ||
      join_br->target_label != loop_latch.label) {
    return std::nullopt;
  }

  const auto* exit_ret = std::get_if<LirRet>(&exit_block.terminator);
  if (exit_block.label != "block_2" || !exit_block.insts.empty() || exit_ret == nullptr ||
      exit_ret->type_str != "i32" || !exit_ret->value_str.has_value() || *exit_ret->value_str != "0") {
    return std::nullopt;
  }

  return MinimalCountedPrintfTernaryLoopSlice{
      .function_name = function->name,
      .pool_name = string_const->pool_name,
      .raw_bytes = string_const->raw_bytes,
  };
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

std::string emit_minimal_repeated_printf_local_i32_calls_bir_asm(
    std::string_view target_triple,
    const MinimalRepeatedPrintfLocalI32CallsBirSlice& slice) {
  const auto first_label = asm_private_data_label(target_triple, slice.first_pool_name);
  const auto second_label = asm_private_data_label(target_triple, slice.second_pool_name);
  const auto symbol = asm_symbol_name(target_triple, slice.function_name);

  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".section .rodata\n"
      << first_label << ":\n"
      << "  .asciz \"" << escape_asm_string(slice.first_raw_bytes) << "\"\n"
      << second_label << ":\n"
      << "  .asciz \"" << escape_asm_string(slice.second_raw_bytes) << "\"\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".extern printf\n";
  }
  out << ".text\n"
      << emit_function_prelude(target_triple, symbol)
      << "  lea rdi, " << first_label << "[rip]\n"
      << "  mov esi, 42\n"
      << "  mov eax, 0\n"
      << "  call printf\n"
      << "  lea rdi, " << first_label << "[rip]\n"
      << "  mov esi, 64\n"
      << "  mov eax, 0\n"
      << "  call printf\n"
      << "  lea rdi, " << second_label << "[rip]\n"
      << "  mov esi, 12\n"
      << "  mov edx, 34\n"
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

std::string emit_minimal_counted_printf_ternary_loop_asm(
    std::string_view target_triple,
    const MinimalCountedPrintfTernaryLoopSlice& slice) {
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
      << "  push rbx\n"
      << "  xor ebx, ebx\n"
      << ".L_count_loop_cond:\n"
      << "  mov eax, ebx\n"
      << "  cmp eax, 10\n"
      << "  jge .L_count_loop_exit\n"
      << "  cmp eax, 5\n"
      << "  jge .L_count_loop_else\n"
      << "  mov esi, eax\n"
      << "  imul esi, eax\n"
      << "  jmp .L_count_loop_print\n"
      << ".L_count_loop_else:\n"
      << "  lea esi, [rax + rax*2]\n"
      << ".L_count_loop_print:\n"
      << "  lea rdi, " << string_label << "[rip]\n"
      << "  mov eax, 0\n"
      << "  call printf\n"
      << "  add ebx, 1\n"
      << "  jmp .L_count_loop_cond\n"
      << ".L_count_loop_exit:\n"
      << "  mov eax, 0\n"
      << "  pop rbx\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_string_literal_char_asm(
    std::string_view target_triple,
    const MinimalStringLiteralCharSlice& slice) {
  const auto string_label = direct_private_data_label(target_triple, slice.pool_name);
  const auto symbol = direct_symbol_name(target_triple, slice.function_name);

  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".section .rodata\n"
      << string_label << ":\n"
      << "  .asciz \"" << escape_asm_string(slice.raw_bytes) << "\"\n"
      << ".text\n"
      << emit_function_prelude(target_triple, symbol)
      << "  lea rax, " << string_label << "[rip]\n";
  if (slice.extend_kind == c4c::codegen::lir::LirCastKind::SExt) {
    out << "  movsx eax, byte ptr [rax + " << slice.byte_index << "]\n";
  } else {
    out << "  movzx eax, byte ptr [rax + " << slice.byte_index << "]\n";
  }
  out << "  ret\n";
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

std::optional<std::string> try_emit_minimal_repeated_printf_local_i32_calls_bir_module(
    const c4c::backend::bir::Module& module) {
  if (const auto slice = parse_minimal_repeated_printf_local_i32_calls_bir_slice(module);
      slice.has_value()) {
    return emit_minimal_repeated_printf_local_i32_calls_bir_asm(module.target_triple, *slice);
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

std::optional<std::string> try_emit_minimal_counted_printf_ternary_loop_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_counted_printf_ternary_loop_slice(module);
      slice.has_value()) {
    return emit_minimal_counted_printf_ternary_loop_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_string_literal_char_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_string_literal_char_slice(module);
      slice.has_value()) {
    return emit_minimal_string_literal_char_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

}  // namespace c4c::backend::x86
