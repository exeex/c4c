#pragma once

#include "../../../bir/bir.hpp"
#include "../../../prealloc/module.hpp"
#include "../../../prealloc/names.hpp"
#include "../../../prealloc/prepared_lookups.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace c4c::backend::riscv::codegen {

struct PreparedCurrentInstructionContext;

[[nodiscard]] std::optional<std::string> emit_riscv_simple_store_local(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    const c4c::backend::bir::StoreLocalInst& store,
    const PreparedCurrentInstructionContext& context);

[[nodiscard]] std::optional<std::string> emit_riscv_simple_load_local(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    const c4c::backend::bir::LoadLocalInst& load,
    const PreparedCurrentInstructionContext& context);

}  // namespace c4c::backend::riscv::codegen
