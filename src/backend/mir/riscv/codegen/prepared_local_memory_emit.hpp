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

[[nodiscard]] std::optional<std::string> emit_i32_load_from_stack_offset(
    std::string_view destination_register,
    std::int64_t stack_offset);

[[nodiscard]] std::optional<std::string> emit_i32_store_to_stack_offset(
    std::string_view source_register,
    std::int64_t stack_offset);

[[nodiscard]] std::optional<std::string> emit_riscv_simple_store_local(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    const c4c::backend::bir::StoreLocalInst& store,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups);

[[nodiscard]] std::optional<std::string> emit_riscv_simple_load_local(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    const c4c::backend::bir::LoadLocalInst& load,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups);

}  // namespace c4c::backend::riscv::codegen
