#include "records.hpp"

namespace c4c::backend::aarch64::codegen {

std::string_view operand_kind_name(OperandKind kind) {
  switch (kind) {
    case OperandKind::Register:
      return "register";
    case OperandKind::Immediate:
      return "immediate";
    case OperandKind::PreparedValue:
      return "prepared_value";
    case OperandKind::FrameSlot:
      return "frame_slot";
    case OperandKind::Symbol:
      return "symbol";
    case OperandKind::BranchTarget:
      return "branch_target";
    case OperandKind::Memory:
      return "memory";
  }
  return "unknown";
}

std::string_view record_surface_kind_name(RecordSurfaceKind kind) {
  switch (kind) {
    case RecordSurfaceKind::RecordOnly:
      return "record_only";
  }
  return "unknown";
}

std::string_view register_operand_role_name(RegisterOperandRole role) {
  switch (role) {
    case RegisterOperandRole::Physical:
      return "physical";
    case RegisterOperandRole::PreparedAssignment:
      return "prepared_assignment";
    case RegisterOperandRole::ValueHome:
      return "value_home";
    case RegisterOperandRole::SpillAuthority:
      return "spill_authority";
    case RegisterOperandRole::StoragePlan:
      return "storage_plan";
    case RegisterOperandRole::CallAbi:
      return "call_abi";
  }
  return "unknown";
}

std::string_view immediate_kind_name(ImmediateKind kind) {
  switch (kind) {
    case ImmediateKind::SignedInteger:
      return "signed_integer";
    case ImmediateKind::UnsignedInteger:
      return "unsigned_integer";
    case ImmediateKind::Boolean:
      return "boolean";
    case ImmediateKind::NullPointer:
      return "null_pointer";
  }
  return "unknown";
}

std::string_view memory_base_kind_name(MemoryBaseKind kind) {
  switch (kind) {
    case MemoryBaseKind::None:
      return "none";
    case MemoryBaseKind::FrameSlot:
      return "frame_slot";
    case MemoryBaseKind::Symbol:
      return "symbol";
    case MemoryBaseKind::PointerValue:
      return "pointer_value";
    case MemoryBaseKind::StringConstant:
      return "string_constant";
    case MemoryBaseKind::Register:
      return "register";
  }
  return "unknown";
}

OperandRecord make_register_operand(RegisterOperand operand) {
  return OperandRecord{.kind = OperandKind::Register, .payload = operand};
}

OperandRecord make_immediate_operand(ImmediateOperand operand) {
  return OperandRecord{.kind = OperandKind::Immediate, .payload = operand};
}

OperandRecord make_prepared_value_operand(PreparedValueOperand operand) {
  return OperandRecord{.kind = OperandKind::PreparedValue, .payload = operand};
}

OperandRecord make_frame_slot_operand(FrameSlotOperand operand) {
  return OperandRecord{.kind = OperandKind::FrameSlot, .payload = operand};
}

OperandRecord make_symbol_operand(SymbolOperand operand) {
  return OperandRecord{.kind = OperandKind::Symbol, .payload = operand};
}

OperandRecord make_branch_target_operand(BranchTargetOperand operand) {
  return OperandRecord{.kind = OperandKind::BranchTarget, .payload = operand};
}

OperandRecord make_memory_operand(MemoryOperand operand) {
  return OperandRecord{.kind = OperandKind::Memory, .payload = operand};
}

}  // namespace c4c::backend::aarch64::codegen
