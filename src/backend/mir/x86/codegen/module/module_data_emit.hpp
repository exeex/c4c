#pragma once

#include "../../../../bir/bir.hpp"
#include "../x86_codegen.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>

namespace c4c::backend::x86::module {

struct ModuleDataSupport {
  const c4c::backend::prepare::PreparedBirModule* module = nullptr;
  std::string_view target_triple;

  [[nodiscard]] std::string render_asm_symbol_name(std::string_view logical_name) const;
  [[nodiscard]] std::string render_private_data_label(std::string_view pool_name) const;
  [[nodiscard]] const c4c::backend::bir::StringConstant* find_string_constant(
      std::string_view name) const;
  [[nodiscard]] bool has_string_constant(std::string_view name) const;
  [[nodiscard]] const c4c::backend::bir::Global* find_same_module_global(
      std::string_view name) const;
  [[nodiscard]] bool has_same_module_global(std::string_view name) const;
  [[nodiscard]] bool same_module_global_supports_scalar_load(
      const c4c::backend::bir::Global& global,
      c4c::backend::bir::TypeKind type,
      std::size_t byte_offset) const;
  [[nodiscard]] std::string emit_string_constant_data(
      const c4c::backend::bir::StringConstant& string_constant) const;
  [[nodiscard]] std::optional<std::string> emit_same_module_global_data(
      const c4c::backend::bir::Global& global) const;
  [[nodiscard]] std::string emit_missing_same_module_global_data(std::string_view asm_text) const;
  [[nodiscard]] std::string emit_direct_variadic_runtime_helpers(std::string_view asm_text) const;
  void add_referenced_same_module_globals(
      std::string_view asm_text,
      std::unordered_set<std::string_view>* used_same_module_global_names) const;
  [[nodiscard]] std::string emit_selected_module_data(
      const std::unordered_set<std::string_view>& used_string_names,
      const std::unordered_set<std::string_view>& used_same_module_global_names) const;
};

[[nodiscard]] ModuleDataSupport make_module_data_support(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view target_triple);

[[nodiscard]] const c4c::backend::bir::StringConstant* find_string_constant(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view name);
[[nodiscard]] const c4c::backend::bir::Global* find_same_module_global(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view name);

[[nodiscard]] bool same_module_global_supports_scalar_load(
    const c4c::backend::bir::Global& global,
    c4c::backend::bir::TypeKind type,
    std::size_t byte_offset);

[[nodiscard]] std::string emit_string_constant_data(
    std::string_view target_triple,
    const c4c::backend::bir::StringConstant& string_constant);

[[nodiscard]] std::optional<std::string> emit_same_module_global_data(
    std::string_view target_triple,
    const c4c::backend::bir::Global& global);

[[nodiscard]] std::string emit_missing_same_module_global_data(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view target_triple,
    std::string_view asm_text);

[[nodiscard]] std::string emit_direct_variadic_runtime_helpers(
    std::string_view target_triple,
    std::string_view asm_text);

void add_referenced_same_module_globals(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view target_triple,
    std::string_view asm_text,
    std::unordered_set<std::string_view>* used_same_module_global_names);

[[nodiscard]] std::string emit_selected_module_data(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view target_triple,
    const std::unordered_set<std::string_view>& used_string_names,
    const std::unordered_set<std::string_view>& used_same_module_global_names);

}  // namespace c4c::backend::x86::module
