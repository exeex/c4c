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

std::string_view instruction_family_name(InstructionFamily family) {
  switch (family) {
    case InstructionFamily::Branch:
      return "branch";
    case InstructionFamily::Scalar:
      return "scalar";
    case InstructionFamily::Memory:
      return "memory";
    case InstructionFamily::Call:
      return "call";
    case InstructionFamily::Return:
      return "return";
    case InstructionFamily::Assembler:
      return "assembler";
    case InstructionFamily::Object:
      return "object";
  }
  return "unknown";
}

std::string_view memory_instruction_kind_name(MemoryInstructionKind kind) {
  switch (kind) {
    case MemoryInstructionKind::Load:
      return "load";
    case MemoryInstructionKind::Store:
      return "store";
  }
  return "unknown";
}

std::string_view branch_condition_form_name(BranchConditionForm form) {
  switch (form) {
    case BranchConditionForm::Unconditional:
      return "unconditional";
    case BranchConditionForm::MaterializedBool:
      return "materialized_bool";
    case BranchConditionForm::FusedCompare:
      return "fused_compare";
  }
  return "unknown";
}

bool is_compare_predicate(c4c::backend::bir::BinaryOpcode opcode) {
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Eq:
    case c4c::backend::bir::BinaryOpcode::Ne:
    case c4c::backend::bir::BinaryOpcode::Slt:
    case c4c::backend::bir::BinaryOpcode::Sle:
    case c4c::backend::bir::BinaryOpcode::Sgt:
    case c4c::backend::bir::BinaryOpcode::Sge:
    case c4c::backend::bir::BinaryOpcode::Ult:
    case c4c::backend::bir::BinaryOpcode::Ule:
    case c4c::backend::bir::BinaryOpcode::Ugt:
    case c4c::backend::bir::BinaryOpcode::Uge:
      return true;
    case c4c::backend::bir::BinaryOpcode::Add:
    case c4c::backend::bir::BinaryOpcode::Sub:
    case c4c::backend::bir::BinaryOpcode::Mul:
    case c4c::backend::bir::BinaryOpcode::And:
    case c4c::backend::bir::BinaryOpcode::Or:
    case c4c::backend::bir::BinaryOpcode::Xor:
    case c4c::backend::bir::BinaryOpcode::Shl:
    case c4c::backend::bir::BinaryOpcode::LShr:
    case c4c::backend::bir::BinaryOpcode::AShr:
    case c4c::backend::bir::BinaryOpcode::SDiv:
    case c4c::backend::bir::BinaryOpcode::UDiv:
    case c4c::backend::bir::BinaryOpcode::SRem:
    case c4c::backend::bir::BinaryOpcode::URem:
      return false;
  }
  return false;
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

InstructionRecord make_branch_instruction(BranchInstructionRecord instruction) {
  return InstructionRecord{
      .family = InstructionFamily::Branch,
      .surface = RecordSurfaceKind::RecordOnly,
      .payload = instruction,
  };
}

InstructionRecord make_scalar_instruction(ScalarInstructionRecord instruction) {
  return InstructionRecord{
      .family = InstructionFamily::Scalar,
      .surface = RecordSurfaceKind::RecordOnly,
      .payload = instruction,
  };
}

InstructionRecord make_memory_instruction(MemoryInstructionRecord instruction) {
  return InstructionRecord{
      .family = InstructionFamily::Memory,
      .surface = RecordSurfaceKind::RecordOnly,
      .payload = instruction,
  };
}

InstructionRecord make_call_instruction(CallInstructionRecord instruction) {
  return InstructionRecord{
      .family = InstructionFamily::Call,
      .surface = RecordSurfaceKind::RecordOnly,
      .payload = instruction,
  };
}

InstructionRecord make_return_instruction(ReturnInstructionRecord instruction) {
  return InstructionRecord{
      .family = InstructionFamily::Return,
      .surface = RecordSurfaceKind::RecordOnly,
      .payload = instruction,
  };
}

InstructionRecord make_assembler_instruction(AssemblerInstructionRecord instruction) {
  return InstructionRecord{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::RecordOnly,
      .payload = instruction,
  };
}

InstructionRecord make_object_instruction(ObjectInstructionRecord instruction) {
  return InstructionRecord{
      .family = InstructionFamily::Object,
      .surface = RecordSurfaceKind::RecordOnly,
      .payload = instruction,
  };
}

}  // namespace c4c::backend::aarch64::codegen
