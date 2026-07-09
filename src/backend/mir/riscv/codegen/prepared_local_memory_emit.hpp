#pragma once

#include "../../../bir/bir.hpp"
#include "../../../prealloc/module.hpp"
#include "../../../prealloc/names.hpp"
#include "../../../prealloc/prepared_lookups.hpp"
#include "object_emission.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace c4c::backend::riscv::codegen {

struct PreparedCurrentInstructionContext;
struct PreparedSretStackPointerAccess {
  std::int32_t pointer_home_offset = 0;
  std::int32_t pointee_offset = 0;
};

enum class Rv64LargeSelectedPointerOffsetMaterializationStatus {
  NotApplicable,
  MissingScratchClobberAuthority,
  MalformedScratchClobberAuthority,
  Available,
};

[[nodiscard]] std::optional<std::size_t> prepared_frame_slot_absolute_byte_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes = 4);

[[nodiscard]] std::optional<std::pair<std::uint32_t, std::int32_t>>
prepared_pointer_value_base_offset(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t size_bytes);

[[nodiscard]] Rv64LargeSelectedPointerOffsetMaterializationStatus
rv64_large_selected_pointer_offset_materialization_status(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t size_bytes);

[[nodiscard]] std::optional<std::size_t>
prepared_pointer_value_stack_home_base_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes);

[[nodiscard]] std::optional<std::int32_t>
prepared_byval_stack_slot_pointer_access_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes);

[[nodiscard]] std::optional<PreparedSretStackPointerAccess>
prepared_sret_stack_slot_pointer_access(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes);

[[nodiscard]] std::optional<std::size_t> rv64_local_memory_size_for_type(
    c4c::backend::bir::TypeKind type);

[[nodiscard]] std::optional<std::size_t>
prepared_frame_slot_address_materialization_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::Value& value,
    std::size_t stack_frame_bytes);

[[nodiscard]] std::optional<RiscvEncodedFragment> fragment_for_prepared_store_local(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::StoreLocalInst& store,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes);

[[nodiscard]] std::optional<RiscvEncodedFragment> fragment_for_prepared_load_local(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::LoadLocalInst& load,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t incoming_stack_base_bytes,
    std::size_t stack_frame_bytes);

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
