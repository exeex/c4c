#pragma once

#include "alu.hpp"
#include "../abi/abi.hpp"
#include "../module/module.hpp"
#include "../../query.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace abi = c4c::backend::aarch64::abi;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;


struct InstructionDispatchResult {
  std::size_t visited_operations = 0;
  bool visited_terminator = false;
  std::size_t emitted_instructions = 0;
};

[[nodiscard]] module::BlockLoweringContext make_block_lowering_context(
    module::FunctionLoweringContext function,
    const prepare::PreparedControlFlowBlock& block,
    std::size_t block_index);
[[nodiscard]] InstructionDispatchResult dispatch_prepared_block(
    const module::BlockLoweringContext& context,
    module::MachineBlock& block,
    module::ModuleLoweringDiagnostics& diagnostics);

// dispatch_publication.hpp
[[nodiscard]] std::uint64_t immediate_integer_bits(const bir::Value& value,
                                                   unsigned width_bits);

[[nodiscard]] bool is_byval_formal_value_name(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name);

[[nodiscard]] std::optional<std::size_t> prepared_local_load_offset(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index);

[[nodiscard]] std::optional<std::size_t> publication_parse_va_list_field_suffix(
    std::string_view base,
    std::string_view slot_name);

[[nodiscard]] std::optional<std::string> prepared_va_list_field_address(
    const module::BlockLoweringContext& context,
    std::string_view slot_name);

[[nodiscard]] std::optional<std::size_t> local_slot_address_frame_offset(
    const module::BlockLoweringContext& context,
    std::string_view local_slot_name);

[[nodiscard]] std::optional<std::size_t> local_aggregate_address_frame_offset(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name);

[[nodiscard]] bool emit_local_slot_address_publication_to_register(
    const module::BlockLoweringContext& context,
    const bir::BinaryInst& binary,
    std::uint8_t target_index,
    std::vector<std::string>& lines);

[[nodiscard]] bool register_operands_share_physical_register(
    const RegisterOperand& lhs,
    const RegisterOperand& rhs);

[[nodiscard]] const prepare::PreparedValueHome* prepared_value_home_for_value(
    const module::BlockLoweringContext& context,
    const bir::Value& value);

[[nodiscard]] bool value_has_current_block_entry_publication(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& home);

[[nodiscard]] std::optional<RegisterOperand> current_block_entry_publication_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    abi::RegisterView expected_view);

void record_current_block_entry_publication_registers(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state);

[[nodiscard]] bool block_entry_move_clobbers_current_join_publication(
    const module::BlockLoweringContext& context,
    const module::MachineInstruction& instruction);

[[nodiscard]] bool prepared_value_home_reads_register_index(
    const prepare::PreparedValueHome& home,
    std::uint8_t register_index);

[[nodiscard]] bool value_publication_may_read_register_index(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t register_index,
    unsigned depth = 0);

[[nodiscard]] std::optional<bir::Value> instruction_result_value(
    const bir::Inst& inst);

[[nodiscard]] const bir::Value* instruction_result_value_ref(const bir::Inst& inst);

// prologue_entry_formals.cpp
[[nodiscard]] std::vector<module::MachineInstruction> lower_entry_formal_publications(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state);

}  // namespace c4c::backend::aarch64::codegen
