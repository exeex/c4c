#include "x86_codegen.hpp"

#include <limits>
#include <sstream>
#include <stdexcept>

namespace c4c::backend::x86 {

namespace {

std::string symbol_name_for_target(std::string_view target_triple, std::string_view logical_name) {
  if (target_triple.find("apple-darwin") != std::string::npos) {
    return "_" + std::string(logical_name);
  }
  return std::string(logical_name);
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

std::string emit_global_symbol_prelude(std::string_view target_triple,
                                       std::string_view symbol_name,
                                       std::size_t align_bytes,
                                       bool zero_initializer) {
  std::ostringstream out;
  out << (zero_initializer ? ".bss\n" : ".data\n");
  out << ".globl " << symbol_name << "\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".type " << symbol_name << ", @object\n";
  }
  if (align_bytes > 1) {
    out << ".p2align " << (align_bytes == 2 ? 1 : 2) << "\n";
  }
  out << symbol_name << ":\n";
  return out.str();
}

}  // namespace

std::string emit_minimal_scalar_global_load_slice_asm(std::string_view target_triple,
                                                      std::string_view function_name,
                                                      std::string_view global_name,
                                                      std::int64_t init_imm,
                                                      std::size_t align_bytes,
                                                      bool zero_initializer) {
  if (init_imm < std::numeric_limits<std::int32_t>::min() ||
      init_imm > std::numeric_limits<std::int32_t>::max()) {
    throw std::invalid_argument(
        "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
  }

  const auto global_symbol = symbol_name_for_target(target_triple, global_name);
  const auto function_symbol = symbol_name_for_target(target_triple, function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << emit_global_symbol_prelude(target_triple, global_symbol, align_bytes, zero_initializer);
  if (zero_initializer) {
    out << "  .zero 4\n";
  } else {
    out << "  .long " << init_imm << "\n";
  }
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".size " << global_symbol << ", 4\n";
  }
  out << ".text\n"
      << emit_function_prelude(target_triple, function_symbol)
      << "  lea rax, " << global_symbol << "[rip]\n"
      << "  mov eax, dword ptr [rax]\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_extern_scalar_global_load_slice_asm(std::string_view target_triple,
                                                             std::string_view function_name,
                                                             std::string_view global_name) {
  const auto global_symbol = symbol_name_for_target(target_triple, global_name);
  const auto function_symbol = symbol_name_for_target(target_triple, function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".extern " << global_symbol << "\n";
  }
  out << ".text\n"
      << emit_function_prelude(target_triple, function_symbol)
      << "  lea rax, " << global_symbol << "[rip]\n"
      << "  mov eax, dword ptr [rax]\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_scalar_global_store_reload_slice_asm(std::string_view target_triple,
                                                              std::string_view function_name,
                                                              std::string_view global_name,
                                                              std::int64_t init_imm,
                                                              std::int64_t store_imm,
                                                              std::size_t align_bytes) {
  if (init_imm < std::numeric_limits<std::int32_t>::min() ||
      init_imm > std::numeric_limits<std::int32_t>::max() ||
      store_imm < std::numeric_limits<std::int32_t>::min() ||
      store_imm > std::numeric_limits<std::int32_t>::max()) {
    throw std::invalid_argument(
        "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
  }

  const auto global_symbol = symbol_name_for_target(target_triple, global_name);
  const auto function_symbol = symbol_name_for_target(target_triple, function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << emit_global_symbol_prelude(target_triple, global_symbol, align_bytes, false)
      << "  .long " << init_imm << "\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".size " << global_symbol << ", 4\n";
  }
  out << ".text\n"
      << emit_function_prelude(target_triple, function_symbol)
      << "  lea rax, " << global_symbol << "[rip]\n"
      << "  mov dword ptr [rax], " << store_imm << "\n"
      << "  mov eax, dword ptr [rax]\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_global_store_return_and_entry_return_asm(
    std::string_view target_triple,
    const MinimalGlobalStoreReturnAndEntryReturnSlice& slice) {
  if (slice.init_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.init_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.store_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.store_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.helper_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.helper_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.entry_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.entry_imm > std::numeric_limits<std::int32_t>::max()) {
    throw std::invalid_argument(
        "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
  }

  const auto global_symbol = symbol_name_for_target(target_triple, slice.global_name);
  const auto helper_symbol = symbol_name_for_target(target_triple, slice.helper_name);
  const auto entry_symbol = symbol_name_for_target(target_triple, slice.entry_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << emit_global_symbol_prelude(target_triple, global_symbol, slice.align_bytes,
                                    slice.zero_initializer);
  if (slice.zero_initializer) {
    out << "  .zero 4\n";
  } else {
    out << "  .long " << slice.init_imm << "\n";
  }
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".size " << global_symbol << ", 4\n";
  }
  out << ".text\n"
      << emit_function_prelude(target_triple, helper_symbol)
      << "  lea rax, " << global_symbol << "[rip]\n"
      << "  mov dword ptr [rax], " << slice.store_imm << "\n"
      << "  mov eax, " << slice.helper_imm << "\n"
      << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  mov eax, " << slice.entry_imm << "\n"
      << "  ret\n";
  return out.str();
}

}  // namespace c4c::backend::x86
