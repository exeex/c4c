#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace c4c::backend::prepare {
struct PreparedBirModule;
}

namespace c4c::backend::x86 {

// Residual observational compatibility surface. Canonical prepared-route debug
// ownership lives under debug/, and these declarations remain only so older
// top-level callers do not reach through to the new owner directly.
[[nodiscard]] std::string summarize_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function = std::nullopt,
    std::optional<std::string_view> focus_block = std::nullopt);

[[nodiscard]] std::string trace_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function = std::nullopt,
    std::optional<std::string_view> focus_block = std::nullopt);

}  // namespace c4c::backend::x86
