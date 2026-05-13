#include "src/backend/mir/aarch64/codegen/records.hpp"

#include <iostream>
#include <variant>

namespace {

namespace aarch64_abi = c4c::backend::aarch64::abi;
namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

aarch64_codegen::RegisterOperand converted_prepared_register_operand() {
  const prepare::PreparedPhysicalRegisterAssignment assignment{
      .reg_class = prepare::PreparedRegisterClass::General,
      .register_name = "x19",
      .contiguous_width = 1,
      .occupied_register_names = {"x19"},
  };
  const auto converted =
      aarch64_abi::convert_prepared_register(assignment, aarch64_abi::RegisterView::X);

  return aarch64_codegen::RegisterOperand{
      .reg = converted.reg.value_or(aarch64_abi::x_register(0)),
      .role = aarch64_codegen::RegisterOperandRole::PreparedAssignment,
      .value_id = prepare::PreparedValueId{42},
      .value_name = c4c::ValueNameId{7},
      .prepared_class = assignment.reg_class,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = aarch64_abi::RegisterView::X,
      .contiguous_width = assignment.contiguous_width,
  };
}

int prepared_register_identity_survives_target_record_layers() {
  const auto reg_operand = converted_prepared_register_operand();
  const auto operand = aarch64_codegen::make_register_operand(reg_operand);
  const auto scalar = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::ScalarInstructionRecord{
          .result_value_id = prepare::PreparedValueId{43},
          .result_value_name = c4c::ValueNameId{8},
          .result_type = bir::TypeKind::I64,
          .inputs = {operand},
          .source_binary_opcode = bir::BinaryOpcode::Add,
      });
  const auto memory = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address =
              aarch64_codegen::MemoryOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .base_kind = aarch64_codegen::MemoryBaseKind::Register,
                  .base_register = reg_operand,
                  .pointer_value_id = prepare::PreparedValueId{44},
                  .byte_offset = 16,
                  .size_bytes = 8,
                  .align_bytes = 8,
                  .address_space = bir::AddressSpace::Default,
                  .can_use_base_plus_offset = true,
              },
          .value = operand,
          .value_type = bir::TypeKind::I64,
      });

  const auto* scalar_payload =
      std::get_if<aarch64_codegen::ScalarInstructionRecord>(&scalar.payload);
  const auto* scalar_input = scalar_payload == nullptr || scalar_payload->inputs.empty()
                                 ? nullptr
                                 : std::get_if<aarch64_codegen::RegisterOperand>(
                                       &scalar_payload->inputs.front().payload);
  const auto* memory_payload =
      std::get_if<aarch64_codegen::MemoryInstructionRecord>(&memory.payload);
  const auto* stored_value =
      memory_payload == nullptr || !memory_payload->value.has_value()
          ? nullptr
          : std::get_if<aarch64_codegen::RegisterOperand>(&memory_payload->value->payload);

  if (scalar_input == nullptr || scalar_input->reg != aarch64_abi::x_register(19) ||
      scalar_input->value_id != prepare::PreparedValueId{42} ||
      scalar_input->value_name != c4c::ValueNameId{7} ||
      scalar_input->prepared_class != prepare::PreparedRegisterClass::General ||
      scalar_input->prepared_bank != prepare::PreparedRegisterBank::Gpr ||
      scalar_input->expected_view != aarch64_abi::RegisterView::X ||
      scalar_input->contiguous_width != 1) {
    return fail("expected converted prepared register identity to survive scalar operands");
  }
  if (memory_payload == nullptr ||
      memory_payload->address.surface != aarch64_codegen::RecordSurfaceKind::RecordOnly ||
      memory_payload->address.base_kind != aarch64_codegen::MemoryBaseKind::Register ||
      !memory_payload->address.base_register.has_value() ||
      memory_payload->address.base_register->reg != aarch64_abi::x_register(19) ||
      stored_value == nullptr || stored_value->value_id != prepare::PreparedValueId{42}) {
    return fail("expected converted prepared register identity to survive memory records");
  }
  return 0;
}

int deferred_family_records_remain_record_only_surfaces() {
  const auto reg_operand =
      aarch64_codegen::make_register_operand(converted_prepared_register_operand());
  const auto branch = aarch64_codegen::make_branch_instruction(
      aarch64_codegen::BranchInstructionRecord{
          .target =
              aarch64_codegen::BranchTargetOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .block_label = c4c::BlockLabelId{3},
                  .function_name = c4c::FunctionNameId{2},
                  .condition_value_id = prepare::PreparedValueId{42},
              },
          .condition = reg_operand,
          .conditional = true,
      });
  const auto call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{5},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .arguments = {reg_operand},
          .result = reg_operand,
          .calling_convention = bir::CallingConv::C,
      });
  const auto ret = aarch64_codegen::make_return_instruction(
      aarch64_codegen::ReturnInstructionRecord{
          .value = reg_operand,
          .value_type = bir::TypeKind::I64,
      });
  const auto assembler = aarch64_codegen::make_assembler_instruction(
      aarch64_codegen::AssemblerInstructionRecord{
          .operands = {reg_operand},
          .has_inline_asm_payload = false,
          .side_effects = false,
      });
  const auto object = aarch64_codegen::make_object_instruction(
      aarch64_codegen::ObjectInstructionRecord{
          .value = reg_operand,
          .object_type = bir::TypeKind::I64,
      });

  if (branch.surface != aarch64_codegen::RecordSurfaceKind::RecordOnly ||
      call.surface != aarch64_codegen::RecordSurfaceKind::RecordOnly ||
      ret.surface != aarch64_codegen::RecordSurfaceKind::RecordOnly ||
      assembler.surface != aarch64_codegen::RecordSurfaceKind::RecordOnly ||
      object.surface != aarch64_codegen::RecordSurfaceKind::RecordOnly) {
    return fail("expected all deferred instruction families to stay record-only");
  }
  if (aarch64_codegen::instruction_family_name(branch.family) != "branch" ||
      aarch64_codegen::instruction_family_name(call.family) != "call" ||
      aarch64_codegen::instruction_family_name(ret.family) != "return" ||
      aarch64_codegen::instruction_family_name(assembler.family) != "assembler" ||
      aarch64_codegen::instruction_family_name(object.family) != "object") {
    return fail("expected deferred instruction family names to stay explicit");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = prepared_register_identity_survives_target_record_layers();
      status != 0) {
    return status;
  }
  if (const int status = deferred_family_records_remain_record_only_surfaces(); status != 0) {
    return status;
  }
  return 0;
}
