#pragma once

#include "../../../prealloc/module.hpp"

#include <string>

namespace c4c::backend::riscv::codegen {

[[nodiscard]] std::string emit_prepared_module_text(
    const c4c::backend::prepare::PreparedBirModule& module);

}  // namespace c4c::backend::riscv::codegen
