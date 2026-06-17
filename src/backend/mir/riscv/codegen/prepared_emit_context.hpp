#pragma once

#include "../../../bir/bir.hpp"
#include "../../../prealloc/names.hpp"
#include "../../../prealloc/prepared_lookups.hpp"

#include <cstddef>
#include <optional>
#include <string>

namespace c4c::backend::riscv::codegen {

struct PreparedCurrentInstructionContext {
  const c4c::backend::prepare::PreparedNameTables& names;
  const c4c::backend::prepare::PreparedFunctionLookups* lookups = nullptr;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::size_t instruction_index = 0;
};

[[nodiscard]] const c4c::backend::prepare::PreparedValueHome* prepared_value_home_for(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value);

[[nodiscard]] const c4c::backend::prepare::PreparedValueHome* prepared_value_home_for(
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::Value& value);

[[nodiscard]] std::optional<std::string> prepared_register_for_value(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value);

[[nodiscard]] std::optional<std::string> prepared_register_for_value(
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::Value& value);

[[nodiscard]] std::optional<std::string> prepared_pointer_register_for_value(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value);

[[nodiscard]] std::optional<std::string> prepared_pointer_register_for_value(
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::Value& value);

[[nodiscard]] std::optional<std::string> prepared_register_for_value_name_id(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::ValueNameId value_name);

[[nodiscard]] std::optional<std::string> prepared_register_for_value_name_id(
    const PreparedCurrentInstructionContext& context,
    c4c::ValueNameId value_name);

[[nodiscard]] bool has_frame_slot_address_materialization_at(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index);

[[nodiscard]] bool has_frame_slot_address_materialization_at(
    const PreparedCurrentInstructionContext& context);

}  // namespace c4c::backend::riscv::codegen
