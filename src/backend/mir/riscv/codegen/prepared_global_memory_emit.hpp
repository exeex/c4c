#pragma once

#include "../../../bir/bir.hpp"
#include "../../../prealloc/addressing.hpp"
#include "../../../prealloc/module.hpp"
#include "../../../prealloc/prepared_lookups.hpp"
#include "object_emission.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace c4c::backend::riscv::codegen {

struct PreparedCurrentInstructionContext;

[[nodiscard]] std::optional<std::size_t> rv64_global_scalar_memory_size_for_type(
    c4c::backend::bir::TypeKind type);

[[nodiscard]] std::optional<RiscvEncodedFragment>
fragment_for_prepared_symbol_address_materialization(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::BinaryInst& binary);

[[nodiscard]] std::optional<RiscvEncodedFragment> fragment_for_prepared_load_global(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::LoadGlobalInst& load,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes);

[[nodiscard]] std::optional<RiscvEncodedFragment> fragment_for_prepared_store_global(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::StoreGlobalInst& store,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes);

[[nodiscard]] bool append_prepared_global_storage_asm(
    std::string& out,
    const c4c::backend::prepare::PreparedBirModule& prepared);

[[nodiscard]] std::optional<std::string> emit_riscv_direct_global_address_materialization(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedAddressMaterialization& materialization,
    std::string_view destination_register);

[[nodiscard]] std::optional<std::string> emit_riscv_direct_function_address_materialization(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::Value& value,
    std::string_view destination_register);

[[nodiscard]] std::optional<std::string> emit_riscv_simple_load_global(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::LoadGlobalInst& load,
    const PreparedCurrentInstructionContext& context);

[[nodiscard]] std::optional<std::string> emit_riscv_simple_store_global(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::StoreGlobalInst& store,
    const PreparedCurrentInstructionContext& context);

}  // namespace c4c::backend::riscv::codegen
