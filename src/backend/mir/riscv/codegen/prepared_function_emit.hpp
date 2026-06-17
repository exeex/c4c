#pragma once

#include "../../../bir/bir.hpp"
#include "../../../prealloc/module.hpp"
#include "../../../prealloc/prepared_lookups.hpp"

#include <string>

namespace c4c::backend::riscv::codegen {

[[nodiscard]] bool append_simple_prepared_bir_function_asm(
    std::string& out,
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Function& function);

}  // namespace c4c::backend::riscv::codegen
