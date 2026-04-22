#pragma once

#include "../../../../bir/bir.hpp"
#include "../x86_codegen.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace c4c::backend::x86::module {

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

}  // namespace c4c::backend::x86::module
