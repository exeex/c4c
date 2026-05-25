#include "calls.hpp"
#include "constant_materialization.hpp"
#include "float_ops.hpp"
#include "memory.hpp"
#include "variadic.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;
namespace abi = c4c::backend::aarch64::abi;

[[nodiscard]] bool publish_prepared_call_preserve_effects();

namespace {

[[nodiscard]] module::MachineInstruction make_call_boundary_machine_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    InstructionRecord target);
[[nodiscard]] std::optional<module::MachineInstruction>
make_immediate_cast_call_argument_publication_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& source_home,
    const RegisterOperand& destination,
    std::size_t instruction_index);
[[nodiscard]] module::MachineInstruction make_call_boundary_machine_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    InstructionRecord target) {
  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = instruction_index,
      },
  };
}


}  // namespace
namespace {

MachineNodeStatusRecord call_boundary_move_selection_status(
    const CallBoundaryMoveInstructionRecord& instruction) {
  if (instruction.source_bundle == nullptr ||
      (instruction.source_move == nullptr && !instruction.source_immediate.has_value() &&
       !instruction.source_memory.has_value()) ||
      instruction.function_name == c4c::kInvalidFunctionName) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "call-boundary move node is missing prepared move provenance"};
  }
  const bool selected_register_argument_move =
      instruction.phase == prepare::PreparedMovePhase::BeforeCall &&
      instruction.move.destination_kind ==
          prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      instruction.move.destination_storage_kind ==
          prepare::PreparedMoveStorageKind::Register &&
      instruction.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move;
  const bool selected_register_result_move =
      instruction.phase == prepare::PreparedMovePhase::AfterCall &&
      instruction.move.destination_kind ==
          prepare::PreparedMoveDestinationKind::CallResultAbi &&
      instruction.move.destination_storage_kind ==
          prepare::PreparedMoveStorageKind::Register &&
      instruction.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move;
  const bool selected_register_return_move =
      instruction.phase == prepare::PreparedMovePhase::BeforeReturn &&
      instruction.move.destination_kind ==
          prepare::PreparedMoveDestinationKind::FunctionReturnAbi &&
      instruction.move.destination_storage_kind ==
          prepare::PreparedMoveStorageKind::Register &&
      instruction.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move;
  const bool selected_value_register_move =
      (instruction.phase == prepare::PreparedMovePhase::BlockEntry ||
       instruction.phase == prepare::PreparedMovePhase::BeforeInstruction) &&
      instruction.move.destination_kind == prepare::PreparedMoveDestinationKind::Value &&
      instruction.move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      instruction.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move;
  const bool selected_preservation_home_population =
      instruction.phase == prepare::PreparedMovePhase::BeforeCall &&
      instruction.move.destination_kind == prepare::PreparedMoveDestinationKind::Value &&
      instruction.move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      instruction.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      instruction.move.reason == "callee_saved_preservation_home_population";
  const bool stack_argument_move =
      instruction.phase == prepare::PreparedMovePhase::BeforeCall &&
      instruction.move.destination_kind ==
          prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      instruction.move.destination_storage_kind ==
          prepare::PreparedMoveStorageKind::StackSlot &&
      instruction.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move;
  if (stack_argument_move) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary stack argument move requires AArch64 stack-copy lowering"};
  }
  if (!selected_register_argument_move && !selected_register_result_move &&
      !selected_register_return_move && !selected_value_register_move &&
      !selected_preservation_home_population) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node is outside the selected register call-boundary move subset"};
  }
  if ((!instruction.source_register.has_value() && !instruction.source_immediate.has_value() &&
       !instruction.source_memory.has_value()) ||
      !instruction.destination_register.has_value()) {
    const bool selected_f128_constant_argument_move =
        selected_register_argument_move &&
        !instruction.source_register.has_value() &&
        instruction.destination_register.has_value() &&
        instruction.destination_register->prepared_bank ==
            prepare::PreparedRegisterBank::Vreg &&
        instruction.destination_register->expected_view == abi::RegisterView::Q &&
        instruction.source_f128_carrier != nullptr &&
        instruction.source_f128_carrier->source_type == bir::TypeKind::F128 &&
        instruction.source_f128_carrier->kind ==
            prepare::PreparedF128CarrierKind::Missing &&
        instruction.source_f128_carrier->missing_required_facts.empty() &&
        instruction.source_f128_carrier->total_size_bytes == 16 &&
        instruction.source_f128_carrier->total_align_bytes == 16 &&
        instruction.source_f128_carrier->constant_payload.has_value() &&
        instruction.source_f128_constant_payload.has_value() &&
        instruction.source_f128_constant_payload->low_bits ==
            instruction.source_f128_carrier->constant_payload->low_bits &&
        instruction.source_f128_constant_payload->high_bits ==
            instruction.source_f128_carrier->constant_payload->high_bits;
    if (selected_f128_constant_argument_move) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node requires prepared register source and destination"};
  }
    if (instruction.source_immediate.has_value() &&
        (instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Gpr ||
         (instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Fpr &&
          scalar_fp_register_view(instruction.source_immediate->type).has_value() &&
          instruction.destination_register->expected_view ==
              *scalar_fp_register_view(instruction.source_immediate->type)))) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
  if (is_aggregate_register_lane_publication(instruction)) {
    if (instruction.source_memory->base_kind == MemoryBaseKind::PointerValue &&
        instruction.source_memory->base_register.has_value()) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
    if (instruction.source_memory->base_kind == MemoryBaseKind::FrameSlot &&
        instruction.source_memory->frame_slot_id.has_value() &&
        instruction.source_memory->byte_offset_is_prepared_snapshot &&
        instruction.source_memory->can_use_base_plus_offset) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
  }
  if (instruction.source_memory.has_value() &&
        instruction.source_memory->support == MemoryOperandSupportKind::Prepared &&
      instruction.source_memory->base_kind == MemoryBaseKind::FrameSlot &&
      instruction.source_memory->frame_slot_id.has_value() &&
      instruction.source_memory->byte_offset_is_prepared_snapshot &&
      instruction.source_memory->can_use_base_plus_offset &&
      (instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Gpr ||
       instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Fpr ||
       (instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Vreg &&
        instruction.destination_register->expected_view == abi::RegisterView::Q &&
        instruction.source_memory->size_bytes == 16))) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  if (instruction.source_memory.has_value()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node requires prepared frame-slot source and GPR destination"};
  }
  if (!instruction.source_register.has_value()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node requires prepared register source and destination"};
  }
  if (instruction.source_register->prepared_bank == prepare::PreparedRegisterBank::Gpr &&
      instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Gpr) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  const bool selected_scalar_fpr_register_move =
      instruction.source_register->prepared_bank == prepare::PreparedRegisterBank::Fpr &&
      instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Fpr &&
      (instruction.source_register->expected_view == abi::RegisterView::S ||
       instruction.source_register->expected_view == abi::RegisterView::D) &&
      instruction.source_register->expected_view ==
          instruction.destination_register->expected_view;
  if (selected_scalar_fpr_register_move) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  const auto* f128_carrier =
      instruction.source_f128_carrier != nullptr
          ? instruction.source_f128_carrier
          : instruction.destination_f128_carrier;
  const bool selected_f128_register_move =
      instruction.source_register->prepared_bank == prepare::PreparedRegisterBank::Vreg &&
      instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Vreg &&
      instruction.source_register->expected_view == abi::RegisterView::Q &&
      instruction.destination_register->expected_view == abi::RegisterView::Q &&
      f128_carrier != nullptr &&
      f128_carrier->kind == prepare::PreparedF128CarrierKind::FullWidthRegister &&
      f128_carrier->missing_required_facts.empty() &&
      f128_carrier->total_size_bytes == 16 && f128_carrier->total_align_bytes == 16;
  if (!selected_f128_register_move) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node requires prepared GPR registers, scalar FPR registers, or structured f128 q-register authority"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

MachineNodeStatusRecord call_boundary_abi_binding_selection_status(
    const CallBoundaryAbiBindingInstructionRecord& instruction) {
  if (instruction.source_bundle == nullptr || instruction.source_binding == nullptr ||
      instruction.function_name == c4c::kInvalidFunctionName) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "call-boundary ABI binding node is missing prepared binding provenance"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

MachineNodeStatusRecord call_selection_status(const CallInstructionRecord& instruction) {
  if (auto variadic_status = variadic_call_selection_status(instruction)) {
    return *variadic_status;
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

[[nodiscard]] const bir::CastInst* find_same_block_cast_producer(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name,
    std::size_t before_instruction_index) {
  if (context.function.prepared == nullptr || context.bir_block == nullptr ||
      value_name == c4c::kInvalidValueName) {
    return nullptr;
  }
  const auto spelling = context.function.prepared->names.value_names.spelling(value_name);
  if (spelling.empty()) {
    return nullptr;
  }
  const auto limit = std::min(before_instruction_index, context.bir_block->insts.size());
  for (std::size_t index = 0; index < limit; ++index) {
    const auto* cast = std::get_if<bir::CastInst>(&context.bir_block->insts[index]);
    if (cast != nullptr && cast->result.kind == bir::Value::Kind::Named &&
        cast->result.name == spelling) {
      return cast;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<unsigned> integer_width_bits_for_type(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
      return 32U;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return 64U;
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::uint64_t immediate_integer_bits(const bir::Value& value,
                                                   unsigned width_bits) {
  if (value.immediate_bits != 0U) {
    return width_bits == 32U ? static_cast<std::uint32_t>(value.immediate_bits)
                             : value.immediate_bits;
  }
  return width_bits == 32U ? static_cast<std::uint32_t>(value.immediate)
                           : static_cast<std::uint64_t>(value.immediate);
}

[[nodiscard]] std::optional<abi::RegisterReference> scalar_fp_register_view(
    abi::RegisterReference reg,
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::F32:
      return abi::fp_simd_register(reg.index, abi::RegisterView::S);
    case bir::TypeKind::F64:
      return abi::fp_simd_register(reg.index, abi::RegisterView::D);
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<abi::RegisterReference> scalar_gp_register_view(
    abi::RegisterReference reg,
    unsigned width_bits) {
  if (width_bits == 32U) {
    return abi::gp_register(reg.index, abi::RegisterView::W);
  }
  if (width_bits == 64U) {
    return abi::gp_register(reg.index, abi::RegisterView::X);
  }
  return std::nullopt;
}

bool append_materialize_fp_immediate(std::vector<std::string>& lines,
                                     const bir::Value& value,
                                     abi::RegisterReference gp_scratch_base,
                                     abi::RegisterReference fp_scratch_base,
                                     abi::RegisterReference& fp_scratch) {
  const auto fp_view = scalar_fp_register_view(fp_scratch_base, value.type);
  if (!fp_view.has_value() || value.kind != bir::Value::Kind::Immediate) {
    return false;
  }
  fp_scratch = *fp_view;
  if (value.type == bir::TypeKind::F32) {
    const auto gp_scratch = abi::gp_register(gp_scratch_base.index, abi::RegisterView::W);
    if (!gp_scratch.has_value()) {
      return false;
    }
    auto materialize =
        materialize_integer_constant_lines(*gp_scratch, value.immediate_bits, 32);
    if (materialize.empty()) {
      return false;
    }
    lines.insert(lines.end(), materialize.begin(), materialize.end());
    lines.push_back("fmov " + std::string{abi::register_name(*fp_view)} + ", " +
                    std::string{abi::register_name(*gp_scratch)});
    return true;
  }
  if (value.type == bir::TypeKind::F64) {
    const auto gp_scratch = abi::gp_register(gp_scratch_base.index, abi::RegisterView::X);
    if (!gp_scratch.has_value()) {
      return false;
    }
    auto materialize =
        materialize_integer_constant_lines(*gp_scratch, value.immediate_bits, 64);
    if (materialize.empty()) {
      return false;
    }
    lines.insert(lines.end(), materialize.begin(), materialize.end());
    lines.push_back("fmov " + std::string{abi::register_name(*fp_view)} + ", " +
                    std::string{abi::register_name(*gp_scratch)});
    return true;
  }
  return false;
}

[[nodiscard]] std::optional<std::vector<std::string>>
make_immediate_cast_call_argument_publication_lines(
    const bir::CastInst& cast,
    const RegisterOperand& destination) {
  if (cast.operand.kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  const auto gp_scratch_base = abi::reserved_mir_scratch_gp_registers().front();
  const auto fp_scratch_base = abi::reserved_mir_scratch_fp_simd_registers().front();
  std::vector<std::string> lines;
  switch (cast.opcode) {
    case bir::CastOpcode::FPToSI:
    case bir::CastOpcode::FPToUI: {
      if (!abi::is_gp_register(destination.reg)) {
        return std::nullopt;
      }
      abi::RegisterReference fp_source{};
      if (!append_materialize_fp_immediate(
              lines, cast.operand, gp_scratch_base, fp_scratch_base, fp_source)) {
        return std::nullopt;
      }
      const auto width_bits = integer_width_bits_for_type(cast.result.type);
      const auto viewed_destination =
          width_bits.has_value()
              ? scalar_gp_register_view(destination.reg, *width_bits)
              : std::nullopt;
      if (!viewed_destination.has_value()) {
        return std::nullopt;
      }
      lines.push_back(std::string{cast.opcode == bir::CastOpcode::FPToSI ? "fcvtzs "
                                                                         : "fcvtzu "} +
                      std::string{abi::register_name(*viewed_destination)} + ", " +
                      std::string{abi::register_name(fp_source)});
      return lines;
    }
    case bir::CastOpcode::SIToFP:
    case bir::CastOpcode::UIToFP: {
      if (!abi::is_fp_simd_register(destination.reg)) {
        return std::nullopt;
      }
      const auto source_width = integer_width_bits_for_type(cast.operand.type);
      const auto gp_scratch = source_width.has_value()
                                  ? scalar_gp_register_view(gp_scratch_base, *source_width)
                                  : std::nullopt;
      const auto fp_destination = scalar_fp_register_view(destination.reg, cast.result.type);
      if (!source_width.has_value() || !gp_scratch.has_value() ||
          !fp_destination.has_value()) {
        return std::nullopt;
      }
      auto materialize = materialize_integer_constant_lines(
          *gp_scratch, immediate_integer_bits(cast.operand, *source_width), *source_width);
      if (materialize.empty()) {
        return std::nullopt;
      }
      lines.insert(lines.end(), materialize.begin(), materialize.end());
      lines.push_back(std::string{cast.opcode == bir::CastOpcode::SIToFP ? "scvtf "
                                                                         : "ucvtf "} +
                      std::string{abi::register_name(*fp_destination)} + ", " +
                      std::string{abi::register_name(*gp_scratch)});
      return lines;
    }
    case bir::CastOpcode::FPExt:
    case bir::CastOpcode::FPTrunc: {
      if (!abi::is_fp_simd_register(destination.reg)) {
        return std::nullopt;
      }
      abi::RegisterReference fp_source{};
      if (!append_materialize_fp_immediate(
              lines, cast.operand, gp_scratch_base, fp_scratch_base, fp_source)) {
        return std::nullopt;
      }
      const auto fp_destination = scalar_fp_register_view(destination.reg, cast.result.type);
      if (!fp_destination.has_value()) {
        return std::nullopt;
      }
      lines.push_back("fcvt " + std::string{abi::register_name(*fp_destination)} +
                      ", " + std::string{abi::register_name(fp_source)});
      return lines;
    }
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_immediate_cast_call_argument_publication_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& source_home,
    const RegisterOperand& destination,
    std::size_t instruction_index) {
  const auto* cast =
      find_same_block_cast_producer(context, source_home.value_name, instruction_index);
  if (cast == nullptr) {
    return std::nullopt;
  }
  const auto lines =
      make_immediate_cast_call_argument_publication_lines(*cast, destination);
  if (!lines.has_value() || lines->empty()) {
    return std::nullopt;
  }
  std::string asm_text;
  for (std::size_t index = 0; index < lines->size(); ++index) {
    if (index != 0) {
      asm_text += '\n';
    }
    asm_text += (*lines)[index];
  }
  const auto destination_operand = make_register_operand(destination);
  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected},
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .operands = {destination_operand},
      .defs = {machine_effect_from_operand(destination_operand)},
      .uses = {MachineEffectResource{
          .kind = MachineEffectResourceKind::PreparedValue,
          .value_id = source_home.value_id,
          .value_name = source_home.value_name,
      }},
      .side_effects = {MachineSideEffectKind::InlineAssembly},
      .payload =
          AssemblerInstructionRecord{
              .operands = {destination_operand},
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  return make_call_boundary_machine_instruction(context, instruction_index, std::move(target));
}

}  // namespace

InstructionRecord make_call_boundary_move_instruction(
    CallBoundaryMoveInstructionRecord instruction) {
  std::vector<OperandRecord> operands;
  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  if (instruction.destination_register.has_value()) {
    const auto destination = make_register_operand(*instruction.destination_register);
    operands.push_back(destination);
    defs.push_back(machine_effect_from_operand(destination));
  } else if (instruction.move.to_value_id != 0) {
    defs.push_back(machine_prepared_value_def(instruction.move.to_value_id,
                                              c4c::kInvalidValueName));
  }
  if (instruction.source_register.has_value()) {
    const auto source = make_register_operand(*instruction.source_register);
    operands.push_back(source);
    uses.push_back(machine_effect_from_operand(source));
  } else if (instruction.source_memory.has_value()) {
    const auto source = make_memory_operand(*instruction.source_memory);
    operands.push_back(source);
    if (!instruction.source_memory_materializes_address) {
      uses.push_back(machine_effect_from_operand(source));
    }
  } else if (instruction.source_immediate.has_value()) {
    const auto source = make_immediate_operand(*instruction.source_immediate);
    operands.push_back(source);
    uses.push_back(machine_effect_from_operand(source));
  } else if (instruction.move.from_value_id != 0) {
    uses.push_back(machine_prepared_value_def(instruction.move.from_value_id,
                                              c4c::kInvalidValueName));
  }
  return InstructionRecord{
      .family = InstructionFamily::CallBoundary,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::CallBoundaryMove,
      .selection = call_boundary_move_selection_status(instruction),
      .function_name = instruction.function_name,
      .block_index = instruction.block_index,
      .instruction_index = instruction.instruction_index,
      .operands = operands,
      .defs = defs,
      .uses = uses,
      .payload = instruction,
  };
}

InstructionRecord make_call_boundary_abi_binding_instruction(
    CallBoundaryAbiBindingInstructionRecord instruction) {
  return InstructionRecord{
      .family = InstructionFamily::CallBoundary,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::CallBoundaryAbiBinding,
      .selection = call_boundary_abi_binding_selection_status(instruction),
      .function_name = instruction.function_name,
      .block_index = instruction.block_index,
      .instruction_index = instruction.instruction_index,
      .payload = instruction,
  };
}

InstructionRecord make_call_instruction(CallInstructionRecord instruction) {
  complete_variadic_call_record(instruction);

  std::vector<OperandRecord> operands = instruction.arguments;
  if (instruction.indirect_callee.has_value()) {
    operands.insert(operands.begin(), *instruction.indirect_callee);
  } else if (instruction.direct_callee.has_value()) {
    operands.insert(operands.begin(), make_symbol_operand(*instruction.direct_callee));
  }
  std::vector<MachineEffectResource> defs;
  if (instruction.result.has_value()) {
    defs.push_back(machine_effect_from_operand(*instruction.result));
  }
  std::vector<MachineEffectResource> uses = machine_effects_from_operands(operands);
  std::vector<MachineSideEffectKind> side_effects = {MachineSideEffectKind::Call};
  if (instruction.memory_return_storage.has_value()) {
    defs.push_back(machine_effect_from_operand(
        make_memory_operand(*instruction.memory_return_storage)));
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_va_start.has_value()) {
    defs.push_back(machine_prepared_value_def(
        instruction.variadic_va_start->destination_va_list.value_id,
        instruction.variadic_va_start->destination_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_scalar_va_arg.has_value()) {
    defs.push_back(machine_prepared_value_def(
        instruction.variadic_scalar_va_arg->result_home.value_id,
        instruction.variadic_scalar_va_arg->result_home.value_name));
    uses.push_back(machine_prepared_value_def(
        instruction.variadic_scalar_va_arg->source_va_list.value_id,
        instruction.variadic_scalar_va_arg->source_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryRead);
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_aggregate_va_arg.has_value()) {
    defs.push_back(machine_prepared_value_def(
        instruction.variadic_aggregate_va_arg->destination_payload_home.value_id,
        instruction.variadic_aggregate_va_arg->destination_payload_home.value_name));
    uses.push_back(machine_prepared_value_def(
        instruction.variadic_aggregate_va_arg->source_va_list.value_id,
        instruction.variadic_aggregate_va_arg->source_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryRead);
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_va_copy.has_value()) {
    defs.push_back(machine_prepared_value_def(
        instruction.variadic_va_copy->destination_va_list.value_id,
        instruction.variadic_va_copy->destination_va_list.value_name));
    uses.push_back(machine_prepared_value_def(
        instruction.variadic_va_copy->source_va_list.value_id,
        instruction.variadic_va_copy->source_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryRead);
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  return InstructionRecord{
      .family = InstructionFamily::Call,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = instruction.variadic_va_start.has_value()
                    ? MachineOpcode::VariadicVaStart
                    : (instruction.variadic_scalar_va_arg.has_value()
                           ? MachineOpcode::VariadicVaArgScalar
                           : (instruction.variadic_aggregate_va_arg.has_value()
                                  ? MachineOpcode::VariadicVaArgAggregate
                                  : (instruction.variadic_va_copy.has_value()
                                         ? MachineOpcode::VariadicVaCopy
                                         : (instruction.is_indirect
                                                ? MachineOpcode::IndirectCall
                                                : MachineOpcode::DirectCall)))),
      .selection = call_selection_status(instruction),
      .operands = std::move(operands),
      .defs = std::move(defs),
      .uses = std::move(uses),
      .clobbers = effects_from_prepared_call_clobbers(instruction.clobbered_registers),
      .preserves = publish_prepared_call_preserve_effects()
                       ? effects_from_prepared_call_preserved_values(
                             instruction.preserved_values)
                       : std::vector<MachineEffectResource>{},
      .side_effects = std::move(side_effects),
      .payload = std::move(instruction),
  };
}

}  // namespace c4c::backend::aarch64::codegen
