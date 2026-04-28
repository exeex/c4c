#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace c4c::backend::prepare {
struct PreparedBirModule;
}

namespace c4c::backend::x86::debug {

[[nodiscard]] std::string summarize_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function = std::nullopt,
    std::optional<std::string_view> focus_block = std::nullopt,
    std::optional<std::string_view> focus_value = std::nullopt);

[[nodiscard]] std::string trace_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function = std::nullopt,
    std::optional<std::string_view> focus_block = std::nullopt,
    std::optional<std::string_view> focus_value = std::nullopt);

}  // namespace c4c::backend::x86::debug
