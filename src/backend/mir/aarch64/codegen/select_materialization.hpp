#pragma once

#include "alu.hpp"
#include "../../../prealloc/publication_plans.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

[[nodiscard]] std::string select_chain_label(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    c4c::ValueNameId root_value_name,
    std::uint8_t target_index,
    std::size_t package_index,
    std::size_t label_index,
    std::string_view suffix);

[[nodiscard]] std::string select_chain_local_label_reference(
    std::size_t label_index,
    std::string_view suffix);

[[nodiscard]] std::string select_chain_local_label_definition(
    std::size_t label_index,
    std::string_view suffix);

[[nodiscard]] bool emit_select_chain_value_to_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::size_t root_instruction_index,
    c4c::ValueNameId root_value_name,
    std::size_t package_index,
    std::vector<std::string>& lines,
    std::size_t& label_index,
    std::vector<std::string_view>& active_values,
    bool reload_current_memory_loads = false,
    const prepare::PreparedDirectGlobalSelectChainDependency*
        direct_global_dependency = nullptr);

[[nodiscard]] std::optional<module::MachineInstruction>
make_select_chain_materialization_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::vector<std::string> lines);

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_direct_global_select_chain_call_argument(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    const bir::Route6CallUseSourceIndex* call_use_source_index,
    const prepare::PreparedCallArgumentPlan* argument_plan,
    BlockScalarLoweringState& scalar_state);

}  // namespace c4c::backend::aarch64::codegen
