#pragma once

#include "alu.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace abi = c4c::backend::aarch64::abi;
namespace bir = c4c::backend::bir;

[[nodiscard]] bool emit_fp_immediate_to_register(std::vector<std::string>& lines,
                                                 const bir::Value& value,
                                                 abi::RegisterReference fp_destination,
                                                 std::uint8_t gp_scratch_index);

[[nodiscard]] bool emit_fp_value_to_register(const module::BlockLoweringContext& context,
                                             const bir::Value& value,
                                             std::size_t before_instruction_index,
                                             abi::RegisterReference destination,
                                             std::uint8_t gp_scratch_index,
                                             std::vector<std::string>& lines);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_scalar_fp_binary_publication_to_prepared_register(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state);

}  // namespace c4c::backend::aarch64::codegen
