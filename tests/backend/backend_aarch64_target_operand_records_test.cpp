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

int typed_register_operands_preserve_prepared_facts() {
  const auto operand = aarch64_codegen::make_register_operand(
      aarch64_codegen::RegisterOperand{
          .reg = aarch64_abi::x_register(9),
          .role = aarch64_codegen::RegisterOperandRole::PreparedAssignment,
          .value_id = prepare::PreparedValueId{17},
          .value_name = c4c::ValueNameId{4},
          .prepared_class = prepare::PreparedRegisterClass::General,
          .prepared_bank = prepare::PreparedRegisterBank::Gpr,
          .expected_view = aarch64_abi::RegisterView::X,
          .contiguous_width = 1,
      });

  if (operand.kind != aarch64_codegen::OperandKind::Register ||
      aarch64_codegen::operand_kind_name(operand.kind) != "register") {
    return fail("expected register operand kind");
  }
  const auto* reg = std::get_if<aarch64_codegen::RegisterOperand>(&operand.payload);
  if (reg == nullptr || reg->reg != aarch64_abi::x_register(9) ||
      reg->value_id != prepare::PreparedValueId{17} ||
      reg->value_name != c4c::ValueNameId{4} ||
      reg->prepared_class != prepare::PreparedRegisterClass::General ||
      reg->prepared_bank != prepare::PreparedRegisterBank::Gpr ||
      reg->expected_view != aarch64_abi::RegisterView::X) {
    return fail("expected typed register operand to retain prepared identity fields");
  }
  return 0;
}

int prepared_value_immediate_frame_and_symbol_operands_are_structured() {
  const auto value = aarch64_codegen::make_prepared_value_operand(
      aarch64_codegen::PreparedValueOperand{
          .value_id = prepare::PreparedValueId{23},
          .function_name = c4c::FunctionNameId{3},
          .value_name = c4c::ValueNameId{8},
          .type = bir::TypeKind::I64,
          .value_kind = prepare::PreparedValueKind::Temporary,
          .home_kind = prepare::PreparedValueHomeKind::Register,
          .storage_encoding = prepare::PreparedStorageEncodingKind::Register,
          .register_class = prepare::PreparedRegisterClass::General,
          .storage_bank = prepare::PreparedRegisterBank::Gpr,
          .register_group_width = 1,
      });
  const auto immediate = aarch64_codegen::make_immediate_operand(
      aarch64_codegen::ImmediateOperand{
          .kind = aarch64_codegen::ImmediateKind::SignedInteger,
          .type = bir::TypeKind::I32,
          .signed_value = -12,
          .source_value_id = prepare::PreparedValueId{24},
          .source_value_name = c4c::ValueNameId{9},
      });
  const auto frame_slot = aarch64_codegen::make_frame_slot_operand(
      aarch64_codegen::FrameSlotOperand{
          .slot_id = prepare::PreparedFrameSlotId{5},
          .object_id = prepare::PreparedObjectId{7},
          .function_name = c4c::FunctionNameId{3},
          .slot_name = c4c::SlotNameId{2},
          .value_name = c4c::ValueNameId{10},
          .type = bir::TypeKind::I32,
          .offset_bytes = 16,
          .size_bytes = 4,
          .align_bytes = 4,
          .fixed_location = true,
      });
  const auto symbol = aarch64_codegen::make_symbol_operand(
      aarch64_codegen::SymbolOperand{
          .link_name = c4c::LinkNameId{6},
          .type = bir::TypeKind::Ptr,
          .byte_offset = 32,
          .is_extern = true,
      });

  const auto* prepared_value =
      std::get_if<aarch64_codegen::PreparedValueOperand>(&value.payload);
  const auto* imm = std::get_if<aarch64_codegen::ImmediateOperand>(&immediate.payload);
  const auto* slot = std::get_if<aarch64_codegen::FrameSlotOperand>(&frame_slot.payload);
  const auto* sym = std::get_if<aarch64_codegen::SymbolOperand>(&symbol.payload);

  if (prepared_value == nullptr || prepared_value->value_id != prepare::PreparedValueId{23} ||
      prepared_value->value_name != c4c::ValueNameId{8} ||
      prepared_value->storage_encoding != prepare::PreparedStorageEncodingKind::Register) {
    return fail("expected prepared value operand to retain prepared ids and storage facts");
  }
  if (imm == nullptr || imm->signed_value != -12 ||
      imm->source_value_id != prepare::PreparedValueId{24} ||
      aarch64_codegen::immediate_kind_name(imm->kind) != "signed_integer") {
    return fail("expected immediate operand to retain typed source value facts");
  }
  if (slot == nullptr || slot->slot_id != prepare::PreparedFrameSlotId{5} ||
      slot->object_id != prepare::PreparedObjectId{7} || slot->offset_bytes != 16 ||
      !slot->fixed_location) {
    return fail("expected frame slot operand to retain prepared slot identity");
  }
  if (sym == nullptr || sym->link_name != c4c::LinkNameId{6} || !sym->is_extern ||
      aarch64_codegen::operand_kind_name(symbol.kind) != "symbol") {
    return fail("expected symbol operand to retain structured link name");
  }
  return 0;
}

int branch_and_memory_operands_are_record_only_surfaces() {
  const auto branch = aarch64_codegen::make_branch_target_operand(
      aarch64_codegen::BranchTargetOperand{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .block_label = c4c::BlockLabelId{11},
          .function_name = c4c::FunctionNameId{3},
          .condition_value_id = prepare::PreparedValueId{25},
      });
  const auto memory = aarch64_codegen::make_memory_operand(
      aarch64_codegen::MemoryOperand{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .base_kind = aarch64_codegen::MemoryBaseKind::FrameSlot,
          .frame_slot_id = prepare::PreparedFrameSlotId{5},
          .pointer_value_name = c4c::ValueNameId{12},
          .pointer_value_id = prepare::PreparedValueId{26},
          .byte_offset = 8,
          .size_bytes = 8,
          .align_bytes = 8,
          .address_space = bir::AddressSpace::Default,
          .is_volatile = true,
          .can_use_base_plus_offset = true,
      });

  const auto* target = std::get_if<aarch64_codegen::BranchTargetOperand>(&branch.payload);
  const auto* mem = std::get_if<aarch64_codegen::MemoryOperand>(&memory.payload);
  if (target == nullptr ||
      aarch64_codegen::record_surface_kind_name(target->surface) != "record_only" ||
      target->block_label != c4c::BlockLabelId{11} ||
      target->condition_value_id != prepare::PreparedValueId{25}) {
    return fail("expected branch target operand to be a structured record-only surface");
  }
  if (mem == nullptr || aarch64_codegen::memory_base_kind_name(mem->base_kind) != "frame_slot" ||
      aarch64_codegen::record_surface_kind_name(mem->surface) != "record_only" ||
      mem->frame_slot_id != prepare::PreparedFrameSlotId{5} ||
      mem->pointer_value_id != prepare::PreparedValueId{26} || !mem->is_volatile ||
      !mem->can_use_base_plus_offset) {
    return fail("expected memory operand to retain prepared address fields as record-only data");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = typed_register_operands_preserve_prepared_facts(); status != 0) {
    return status;
  }
  if (const int status = prepared_value_immediate_frame_and_symbol_operands_are_structured();
      status != 0) {
    return status;
  }
  if (const int status = branch_and_memory_operands_are_record_only_surfaces(); status != 0) {
    return status;
  }
  return 0;
}
