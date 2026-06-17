#pragma once

#include "../../../bir/bir.hpp"
#include "../../../prealloc/module.hpp"

#include <optional>
#include <string>

namespace c4c::backend::riscv::codegen {

struct PreparedCurrentInstructionContext;

[[nodiscard]] bool append_prepared_global_storage_asm(
    std::string& out,
    const c4c::backend::prepare::PreparedBirModule& prepared);

[[nodiscard]] std::optional<std::string> emit_riscv_simple_load_global(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::LoadGlobalInst& load,
    const PreparedCurrentInstructionContext& context);

[[nodiscard]] std::optional<std::string> emit_riscv_simple_store_global(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::StoreGlobalInst& store,
    const PreparedCurrentInstructionContext& context);

}  // namespace c4c::backend::riscv::codegen
