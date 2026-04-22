#pragma once

#include "../x86_codegen.hpp"

#include <string>

namespace c4c::backend::x86::module {

[[nodiscard]] std::string emit_prepared_module_text(
    const c4c::backend::prepare::PreparedBirModule& module);

}  // namespace c4c::backend::x86::module
