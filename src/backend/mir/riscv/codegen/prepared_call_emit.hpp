#pragma once

#include "../../../bir/bir.hpp"
#include "../../../prealloc/names.hpp"
#include "../../../prealloc/prepared_lookups.hpp"

#include <cstddef>
#include <optional>
#include <string>

namespace c4c::backend::riscv::codegen {

[[nodiscard]] std::optional<std::string> emit_riscv_simple_call(
    const c4c::backend::bir::CallInst& call,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups);

}  // namespace c4c::backend::riscv::codegen
