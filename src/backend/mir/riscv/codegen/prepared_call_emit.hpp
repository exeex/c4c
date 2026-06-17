#pragma once

#include "../../../bir/bir.hpp"
#include <cstddef>
#include <optional>
#include <string>

namespace c4c::backend::riscv::codegen {

struct PreparedCurrentInstructionContext;

[[nodiscard]] std::optional<std::string> emit_riscv_simple_call(
    const c4c::backend::bir::CallInst& call,
    std::size_t block_index,
    const PreparedCurrentInstructionContext& context);

}  // namespace c4c::backend::riscv::codegen
