#include "x86_codegen.hpp"

#include "../../lowering/call_decode.hpp"
#include "../../../codegen/lir/ir.hpp"

#include <charconv>
#include <sstream>

namespace c4c::backend::x86 {

namespace {

struct MinimalVoidHelperCallSlice {
  std::string helper_name;
  std::string entry_name;
  std::int64_t return_imm = 0;
};

struct MinimalVoidReturnSlice {
  std::string function_name;
};

struct MinimalVoidExternCallReturnImmSlice {
  std::string extern_name;
  std::string function_name;
  std::int64_t return_imm = 0;
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

std::optional<MinimalVoidHelperCallSlice> parse_minimal_void_helper_call_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() || !module.extern_decls.empty() ||
      !module.string_pool.empty()) {
    return std::nullopt;
  }

  const auto try_match =
      [](const LirFunction& helper,
         const LirFunction& entry) -> std::optional<MinimalVoidHelperCallSlice> {
    if (helper.is_declaration || entry.is_declaration || helper.blocks.size() != 1 ||
        entry.blocks.size() != 1 || !helper.alloca_insts.empty() || !entry.alloca_insts.empty() ||
        !helper.stack_objects.empty() || !entry.stack_objects.empty() ||
        !c4c::backend::backend_lir_signature_matches(helper.signature_text,
                                                     "define",
                                                     "void",
                                                     helper.name,
                                                     {}) ||
        !c4c::backend::backend_lir_signature_matches(entry.signature_text,
                                                     "define",
                                                     "i32",
                                                     entry.name,
                                                     {})) {
      return std::nullopt;
    }

    const auto& helper_block = helper.blocks.front();
    const auto* helper_ret = std::get_if<LirRet>(&helper_block.terminator);
    if (helper_block.label != "entry" || !helper_block.insts.empty() || helper_ret == nullptr ||
        helper_ret->value_str.has_value() || helper_ret->type_str != "void") {
      return std::nullopt;
    }

    const auto& entry_block = entry.blocks.front();
    const auto* call =
        entry_block.insts.size() == 1 ? std::get_if<LirCallOp>(&entry_block.insts.front()) : nullptr;
    const auto* ret = std::get_if<LirRet>(&entry_block.terminator);
    const auto called_name =
        call == nullptr ? std::nullopt
                        : c4c::backend::parse_backend_zero_arg_direct_global_typed_call(*call);
    const auto return_imm =
        ret != nullptr && ret->value_str.has_value() ? parse_i64(*ret->value_str) : std::nullopt;
    if (entry_block.label != "entry" || call == nullptr || ret == nullptr ||
        !called_name.has_value() || *called_name != helper.name ||
        call->return_type != "void" || !call->result.empty() || !return_imm.has_value() ||
        c4c::backend::backend_lir_lower_function_return_type(entry, *ret) !=
            c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }

    return MinimalVoidHelperCallSlice{
        .helper_name = helper.name,
        .entry_name = entry.name,
        .return_imm = *return_imm,
    };
  };

  if (const auto parsed = try_match(module.functions.front(), module.functions.back());
      parsed.has_value()) {
    return parsed;
  }
  return try_match(module.functions.back(), module.functions.front());
}

std::optional<MinimalVoidReturnSlice> parse_minimal_void_return_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() || !module.extern_decls.empty() ||
      !module.string_pool.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty() ||
      !c4c::backend::backend_lir_signature_matches(function.signature_text,
                                                   "define",
                                                   "void",
                                                   function.name,
                                                   {})) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || !entry.insts.empty() || ret == nullptr ||
      ret->value_str.has_value() || ret->type_str != "void") {
    return std::nullopt;
  }

  return MinimalVoidReturnSlice{
      .function_name = function.name,
  };
}

std::optional<MinimalVoidExternCallReturnImmSlice> parse_minimal_void_extern_call_return_imm_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() || !module.extern_decls.empty() ||
      !module.string_pool.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty() ||
      !c4c::backend::backend_lir_signature_matches(function.signature_text,
                                                   "define",
                                                   "i32",
                                                   function.name,
                                                   {})) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* call = entry.insts.size() == 1 ? std::get_if<LirCallOp>(&entry.insts.front()) : nullptr;
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  const auto called_name =
      call == nullptr ? std::nullopt : c4c::backend::parse_backend_zero_arg_direct_global_typed_call(*call);
  const auto return_imm =
      ret != nullptr && ret->value_str.has_value() ? parse_i64(*ret->value_str) : std::nullopt;
  if (entry.label != "entry" || call == nullptr || ret == nullptr || !called_name.has_value() ||
      call->return_type != "void" || !call->result.empty() || !return_imm.has_value() ||
      c4c::backend::backend_lir_lower_function_return_type(function, *ret) !=
          c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }

  return MinimalVoidExternCallReturnImmSlice{
      .extern_name = std::string(*called_name),
      .function_name = function.name,
      .return_imm = *return_imm,
  };
}

std::string emit_minimal_void_helper_call_asm(std::string_view target_triple,
                                              const MinimalVoidHelperCallSlice& slice) {
  const auto helper_symbol = asm_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, helper_symbol)
      << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  call " << helper_symbol << "\n"
      << "  mov eax, " << slice.return_imm << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_void_return_asm(std::string_view target_triple,
                                         const MinimalVoidReturnSlice& slice) {
  const auto function_symbol = asm_symbol_name(target_triple, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, function_symbol)
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_void_extern_call_return_imm_asm(
    std::string_view target_triple,
    const MinimalVoidExternCallReturnImmSlice& slice) {
  const auto extern_symbol = asm_symbol_name(target_triple, slice.extern_name);
  const auto function_symbol = asm_symbol_name(target_triple, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".extern " << extern_symbol << "\n";
  }
  out << ".text\n"
      << emit_function_prelude(target_triple, function_symbol)
      << "  call " << extern_symbol << "\n"
      << "  mov eax, " << slice.return_imm << "\n"
      << "  ret\n";
  return out.str();
}

}  // namespace

std::optional<std::string> try_emit_minimal_void_helper_call_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_void_helper_call_slice(module); slice.has_value()) {
    return emit_minimal_void_helper_call_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_void_return_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_void_return_slice(module); slice.has_value()) {
    return emit_minimal_void_return_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_void_extern_call_return_imm_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_void_extern_call_return_imm_slice(module);
      slice.has_value()) {
    return emit_minimal_void_extern_call_return_imm_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

}  // namespace c4c::backend::x86
