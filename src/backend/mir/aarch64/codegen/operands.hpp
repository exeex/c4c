#pragma once

#include "../abi/abi.hpp"
#include "../module/module.hpp"
#include "../../mir.hpp"

#include <cstdint>
#include <optional>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
namespace abi = c4c::backend::aarch64::abi;

enum class OperandAuthority {
  None,
  RegallocAssignment,
  StoragePlan,
  PreparedValueHome,
  FrameSlot,
  SpillSlot,
  Immediate,
  Symbol,
  Label,
  TargetRegister,
};

struct ResolvedOperand {
  c4c::backend::mir::Operand operand;
  OperandAuthority authority = OperandAuthority::None;
  std::optional<abi::RegisterReference> register_reference;
  std::optional<prepare::PreparedFrameSlotId> frame_slot_id;
  std::optional<prepare::PreparedValueId> value_id;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
};

[[nodiscard]] std::optional<ResolvedOperand> resolve_value_operand(
    prepare::PreparedValueId value_id,
    const module::FunctionLoweringContext& context,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] ResolvedOperand resolve_immediate_operand(
    c4c::backend::mir::Immediate immediate);
[[nodiscard]] ResolvedOperand resolve_label_operand(c4c::BlockLabelId label);
[[nodiscard]] ResolvedOperand resolve_symbol_operand(c4c::LinkNameId symbol,
                                                     std::int64_t addend = 0);

}  // namespace c4c::backend::aarch64::codegen
