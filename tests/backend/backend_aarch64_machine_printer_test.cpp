#include "src/backend/mir/aarch64/codegen/machine_printer.hpp"

#include <iostream>
#include <string>

namespace {

namespace aarch64_abi = c4c::backend::aarch64::abi;
namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(const std::string& message) {
  std::cerr << message << "\n";
  return 1;
}

aarch64_codegen::RegisterOperand xreg(unsigned index) {
  return aarch64_codegen::RegisterOperand{
      .reg = aarch64_abi::x_register(static_cast<std::uint8_t>(index)),
      .role = aarch64_codegen::RegisterOperandRole::SpillAuthority,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = aarch64_abi::RegisterView::X,
      .contiguous_width = 1,
  };
}

aarch64_codegen::MemoryOperand frame_slot(std::int64_t offset) {
  return aarch64_codegen::MemoryOperand{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
      .function_name = c4c::FunctionNameId{2},
      .block_label = c4c::BlockLabelId{3},
      .base_kind = aarch64_codegen::MemoryBaseKind::FrameSlot,
      .frame_slot_id = prepare::PreparedFrameSlotId{4},
      .byte_offset = offset,
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = 8,
      .align_bytes = 8,
      .address_space = bir::AddressSpace::Default,
      .can_use_base_plus_offset = true,
  };
}

int selected_spill_reload_nodes_print_gnu_aarch64_text() {
  int source_op = 0;
  const auto spill = aarch64_codegen::make_spill_reload_instruction(
      aarch64_codegen::SpillReloadInstructionRecord{
          .value_id = prepare::PreparedValueId{10},
          .value_name = c4c::ValueNameId{11},
          .value_type = bir::TypeKind::I64,
          .op_kind = prepare::PreparedSpillReloadOpKind::Spill,
          .pseudo_kind = aarch64_codegen::MachinePseudoKind::SpillToSlot,
          .slot = frame_slot(16),
          .scratch = xreg(9),
          .occupied_scratch_registers = {"x9"},
          .scratch_register_authority = std::size_t{0},
          .slot_id = prepare::PreparedFrameSlotId{4},
          .stack_offset_bytes = std::size_t{16},
          .stack_offset_is_prepared_snapshot = true,
          .source_spill_reload =
              reinterpret_cast<const prepare::PreparedSpillReloadOp*>(&source_op),
      });
  const auto reload = aarch64_codegen::make_spill_reload_instruction(
      aarch64_codegen::SpillReloadInstructionRecord{
          .value_id = prepare::PreparedValueId{10},
          .value_name = c4c::ValueNameId{11},
          .value_type = bir::TypeKind::I64,
          .op_kind = prepare::PreparedSpillReloadOpKind::Reload,
          .pseudo_kind = aarch64_codegen::MachinePseudoKind::ReloadFromSlot,
          .slot = frame_slot(16),
          .scratch = xreg(9),
          .occupied_scratch_registers = {"x9"},
          .scratch_register_authority = std::size_t{0},
          .slot_id = prepare::PreparedFrameSlotId{4},
          .stack_offset_bytes = std::size_t{16},
          .stack_offset_is_prepared_snapshot = true,
          .source_spill_reload =
              reinterpret_cast<const prepare::PreparedSpillReloadOp*>(&source_op),
      });

  const auto result = aarch64_codegen::print_machine_instruction_nodes({spill, reload});
  const std::string expected = "    str x9, [sp, #16]\n    ldr x9, [sp, #16]\n";
  if (!result.ok || result.assembly != expected || !result.diagnostic.empty()) {
    return fail("expected selected spill/reload nodes to print canonical AArch64 text");
  }
  return 0;
}

int selected_branch_and_store_nodes_print_without_semantic_roundtrip() {
  const auto condition = aarch64_codegen::make_register_operand(xreg(1));
  const auto branch = aarch64_codegen::make_branch_instruction(
      aarch64_codegen::BranchInstructionRecord{
          .target =
              aarch64_codegen::BranchTargetOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .block_label = c4c::BlockLabelId{7},
                  .function_name = c4c::FunctionNameId{2},
                  .condition_value_id = prepare::PreparedValueId{20},
              },
          .target_pair =
              aarch64_codegen::BranchTargetPairRecord{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .true_target =
                      aarch64_codegen::BranchTargetOperand{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .block_label = c4c::BlockLabelId{7},
                          .function_name = c4c::FunctionNameId{2},
                          .condition_value_id = prepare::PreparedValueId{20},
                      },
                  .false_target =
                      aarch64_codegen::BranchTargetOperand{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .block_label = c4c::BlockLabelId{8},
                          .function_name = c4c::FunctionNameId{2},
                          .condition_value_id = prepare::PreparedValueId{20},
                      },
              },
          .condition = condition,
          .condition_record =
              aarch64_codegen::BranchConditionRecord{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .form = aarch64_codegen::BranchConditionForm::MaterializedBool,
                  .condition_value_id = prepare::PreparedValueId{20},
                  .condition_value_name = c4c::ValueNameId{21},
                  .condition_type = bir::TypeKind::I1,
              },
          .conditional = true,
      });

  auto store_address = frame_slot(24);
  store_address.stored_value_id = prepare::PreparedValueId{22};
  store_address.stored_value_name = c4c::ValueNameId{23};
  const auto store = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address = store_address,
          .value = aarch64_codegen::make_register_operand(xreg(10)),
          .value_type = bir::TypeKind::I64,
      });

  const auto result = aarch64_codegen::print_machine_instruction_nodes({branch, store});
  const std::string expected =
      "    cbnz x1, .LBB2_7\n"
      "    b .LBB2_8\n"
      "    str x10, [sp, #24]\n";
  if (!result.ok || result.assembly != expected) {
    return fail("expected branch and store nodes to print from structured operands: " +
                result.diagnostic);
  }
  return 0;
}

int unsupported_surfaces_statuses_and_missing_operands_fail_closed() {
  const auto assembler = aarch64_codegen::make_assembler_instruction(
      aarch64_codegen::AssemblerInstructionRecord{});
  const auto assembler_result = aarch64_codegen::print_machine_instruction_node(assembler);
  if (assembler_result.ok ||
      assembler_result.diagnostic.find("surface machine_instruction_node") ==
          std::string::npos) {
    return fail("expected external assembler input to fail closed before printing");
  }

  const auto unsupported = aarch64_codegen::make_unsupported_machine_instruction(
      aarch64_codegen::InstructionFamily::Memory,
      aarch64_codegen::MachineNodeSelectionStatus::DeferredUnsupported,
      "fixture unsupported");
  const auto unsupported_result =
      aarch64_codegen::print_machine_instruction_node(unsupported);
  if (unsupported_result.ok ||
      unsupported_result.diagnostic.find("deferred_unsupported: fixture unsupported") ==
          std::string::npos) {
    return fail("expected non-selected machine node to report selection diagnostic");
  }

  const auto scalar = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Add,
          .source_binary_opcode = bir::BinaryOpcode::Add,
          .operand_type = bir::TypeKind::I64,
          .result_value_id = prepare::PreparedValueId{30},
          .result_value_name = c4c::ValueNameId{31},
          .result_type = bir::TypeKind::I64,
          .lhs = aarch64_codegen::make_register_operand(xreg(2)),
          .rhs = aarch64_codegen::make_register_operand(xreg(3)),
          .supported_integer_operation = true,
      }));
  const auto scalar_result = aarch64_codegen::print_machine_instruction_node(scalar);
  if (scalar_result.ok ||
      scalar_result.diagnostic.find("missing a structured destination register operand") ==
          std::string::npos) {
    return fail("expected selected scalar without destination register to fail closed");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int result = selected_spill_reload_nodes_print_gnu_aarch64_text(); result != 0) {
    return result;
  }
  if (const int result = selected_branch_and_store_nodes_print_without_semantic_roundtrip();
      result != 0) {
    return result;
  }
  if (const int result = unsupported_surfaces_statuses_and_missing_operands_fail_closed();
      result != 0) {
    return result;
  }
  return 0;
}
