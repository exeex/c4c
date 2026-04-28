#pragma once

#include "../debug/debug.hpp"

namespace c4c::backend::x86 {

[[nodiscard]] inline std::string summarize_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function = std::nullopt,
    std::optional<std::string_view> focus_block = std::nullopt,
    std::optional<std::string_view> focus_value = std::nullopt) {
  return c4c::backend::x86::debug::summarize_prepared_module_routes(
      module, focus_function, focus_block, focus_value);
}

[[nodiscard]] inline std::string trace_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function = std::nullopt,
    std::optional<std::string_view> focus_block = std::nullopt,
    std::optional<std::string_view> focus_value = std::nullopt) {
  return c4c::backend::x86::debug::trace_prepared_module_routes(
      module, focus_function, focus_block, focus_value);
}

}  // namespace c4c::backend::x86
