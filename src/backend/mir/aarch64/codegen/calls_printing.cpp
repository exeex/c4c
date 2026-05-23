#include "calls.hpp"
#include "float_ops.hpp"
#include "machine_printer.hpp"
#include "memory.hpp"
#include "variadic.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <sstream>
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

[[nodiscard]] bool same_gp_register_index(abi::RegisterReference lhs,
                                          abi::RegisterReference rhs);
[[nodiscard]] bool call_boundary_frame_slot_direct_offset_is_encodable(
    const MemoryOperand& memory,
    std::size_t load_width_bytes);
std::vector<std::string> materialize_call_boundary_frame_slot_address_lines(
    abi::RegisterReference scratch,
    const MemoryOperand& memory);
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

mir::TargetInstructionPrintResult target_unsupported(std::string diagnostic) {
  return mir::target_instruction_unsupported(std::move(diagnostic));
}

mir::TargetInstructionPrintResult target_printed(std::vector<std::string> lines) {
  return mir::target_instruction_lines_printed(std::move(lines));
}

std::string bad_header(const InstructionRecord& instruction) {
  return std::string("cannot print AArch64 machine node family=") +
         std::string(instruction_family_name(instruction.family)) + " opcode=" +
         std::string(machine_opcode_name(instruction.opcode)) + ": ";
}

std::string_view required_primary_mnemonic(const InstructionRecord& instruction) {
  return machine_instruction_primary_printer_mnemonic(instruction);
}

std::string register_name(const RegisterOperand& operand) {
  return abi::register_name(operand.reg);
}

bool same_gp_register_index(abi::RegisterReference lhs, abi::RegisterReference rhs) {
  return lhs.index == rhs.index && abi::is_gp_register(lhs) && abi::is_gp_register(rhs);
}

std::optional<std::size_t> call_boundary_load_width_bytes(
    const RegisterOperand& destination) {
  switch (destination.reg.view) {
    case abi::RegisterView::S:
    case abi::RegisterView::W:
      return 4U;
    case abi::RegisterView::D:
    case abi::RegisterView::X:
      return 8U;
    case abi::RegisterView::Q:
      return 16U;
    default:
      return std::nullopt;
  }
}

bool call_boundary_frame_slot_direct_offset_is_encodable(
    const MemoryOperand& memory,
    std::size_t load_width_bytes) {
  if (memory.base_kind != MemoryBaseKind::FrameSlot || memory.byte_offset < 0 ||
      load_width_bytes == 0) {
    return false;
  }
  if (load_width_bytes != 1 && load_width_bytes != 2 && load_width_bytes != 4 &&
      load_width_bytes != 8 && load_width_bytes != 16) {
    return false;
  }
  const auto offset = static_cast<std::uint64_t>(memory.byte_offset);
  return offset % load_width_bytes == 0 && offset / load_width_bytes <= 4095U;
}

std::optional<abi::RegisterReference> call_boundary_address_scratch_register(
    abi::RegisterReference destination) {
  for (const auto scratch : abi::reserved_mir_scratch_gp_registers()) {
    if (!same_gp_register_index(scratch, destination)) {
      return abi::x_register(scratch.index);
    }
  }
  return std::nullopt;
}

std::vector<std::string> materialize_call_boundary_frame_slot_address_lines(
    abi::RegisterReference scratch,
    const MemoryOperand& memory) {
  if (memory.base_kind != MemoryBaseKind::FrameSlot || memory.byte_offset < 0) {
    return {};
  }
  const auto offset = static_cast<std::uint64_t>(memory.byte_offset);
  const std::string scratch_name = abi::register_name(scratch);
  if (offset <= 4095U) {
    return {"add " + scratch_name + ", sp, #" + std::to_string(offset)};
  }
  auto lines = materialize_integer_constant_lines(scratch, offset, 64);
  if (lines.empty()) {
    return {};
  }
  lines.push_back("add " + scratch_name + ", sp, " + scratch_name);
  return lines;
}

std::optional<std::vector<std::string>> print_aggregate_register_lane_publication_lines(
    const CallBoundaryMoveInstructionRecord& move) {
  if (!is_aggregate_register_lane_publication(move)) {
    return std::nullopt;
  }
  std::vector<std::string> lines;
  const auto scratch = aggregate_register_lane_scratch(*move.destination_register);
  if (!scratch.has_value()) {
    return std::nullopt;
  }
  const std::string scratch_x = abi::register_name(*scratch);
  std::size_t remaining = move.source_memory->size_bytes;
  std::size_t lane_index = 0;
  std::size_t source_offset = 0;
  while (remaining > 0) {
    const std::size_t lane_bytes = std::min<std::size_t>(remaining, 8);
    const auto lane_register =
        aggregate_register_lane_destination(*move.destination_register, lane_index);
    if (!lane_register.has_value()) {
      return std::nullopt;
    }
    std::size_t lane_offset = 0;
    while (lane_offset < lane_bytes) {
      const auto printable_chunk =
          aggregate_register_lane_printable_chunk(*move.source_memory,
                                                  source_offset + lane_offset,
                                                  lane_bytes - lane_offset);
      if (!printable_chunk.has_value()) {
        return std::nullopt;
      }
      const std::size_t chunk_width = *printable_chunk;
      const auto chunk_memory =
          aggregate_register_lane_memory(*move.source_memory,
                                         source_offset + lane_offset,
                                         chunk_width);
      const auto mnemonic = aggregate_register_lane_load_mnemonic(chunk_width);
      if (mnemonic.empty()) {
        return std::nullopt;
      }
      const bool first_chunk = lane_offset == 0;
      const auto load_register =
          first_chunk
              ? aggregate_register_lane_load_register(*lane_register, chunk_width)
              : aggregate_register_lane_load_register(*scratch, chunk_width);
      lines.push_back(std::string{mnemonic} + " " +
                      std::string{abi::register_name(load_register)} + ", " +
                      memory_address(chunk_memory));
      if (!first_chunk) {
        lines.push_back("orr " + std::string{abi::register_name(*lane_register)} +
                        ", " + std::string{abi::register_name(*lane_register)} +
                        ", " + scratch_x + ", lsl #" +
                        std::to_string(lane_offset * 8));
      }
      lane_offset += chunk_width;
    }
    remaining -= lane_bytes;
    source_offset += lane_bytes;
    ++lane_index;
  }
  return lines;
}

std::optional<std::vector<std::string>> print_call_boundary_frame_slot_load_lines(
    const CallBoundaryMoveInstructionRecord& move) {
  if (!move.source_memory.has_value() || !move.destination_register.has_value()) {
    return std::nullopt;
  }
  std::string address;
  std::vector<std::string> lines;
  const auto load_width = call_boundary_load_width_bytes(*move.destination_register);
  if (!load_width.has_value()) {
    return std::nullopt;
  }
  if (call_boundary_frame_slot_direct_offset_is_encodable(*move.source_memory,
                                                          *load_width)) {
    address = memory_address(*move.source_memory);
  } else {
    const auto scratch =
        call_boundary_address_scratch_register(move.destination_register->reg);
    if (!scratch.has_value()) {
      return std::nullopt;
    }
    lines =
        materialize_call_boundary_frame_slot_address_lines(*scratch, *move.source_memory);
    if (lines.empty()) {
      return std::nullopt;
    }
    address = "[" + std::string{abi::register_name(*scratch)} + "]";
  }
  if (address.empty()) {
    return std::nullopt;
  }
  lines.push_back("ldr " + register_name(*move.destination_register) + ", " + address);
  return lines;
}

std::optional<std::string> f128_call_boundary_vector_register_name(
    const RegisterOperand& operand) {
  if (operand.expected_view != abi::RegisterView::Q ||
      operand.prepared_bank != prepare::PreparedRegisterBank::Vreg ||
      operand.prepared_class != prepare::PreparedRegisterClass::Vector ||
      operand.contiguous_width != 1 ||
      !abi::is_fp_simd_register(operand.reg)) {
    return std::nullopt;
  }
  const auto viewed = abi::fp_simd_register(operand.reg.index, abi::RegisterView::V);
  if (!viewed.has_value()) {
    return std::nullopt;
  }
  return std::string{abi::register_name(*viewed)} + ".16b";
}

std::optional<unsigned> scalar_integer_width_bits(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
      return 32U;
    case bir::TypeKind::I64:
      return 64U;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

std::uint64_t scalar_integer_immediate_bits(const ImmediateOperand& immediate,
                                            unsigned width_bits) {
  if (width_bits == 32U) {
    return static_cast<std::uint32_t>(immediate.unsigned_value);
  }
  return immediate.unsigned_value;
}

bool is_single_move_wide_immediate(std::uint64_t value, unsigned width_bits) {
  const unsigned chunks = width_bits == 64U ? 4U : 2U;
  unsigned nonzero_chunks = 0;
  for (unsigned chunk = 0; chunk < chunks; ++chunk) {
    if (((value >> (chunk * 16U)) & 0xffffU) != 0U) {
      ++nonzero_chunks;
    }
  }
  return nonzero_chunks <= 1U;
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
      .defs = {call_effect_from_operand(destination_operand)},
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
    defs.push_back(call_effect_from_operand(destination));
  } else if (instruction.move.to_value_id != 0) {
    defs.push_back(call_prepared_value_def(instruction.move.to_value_id, c4c::kInvalidValueName));
  }
  if (instruction.source_register.has_value()) {
    const auto source = make_register_operand(*instruction.source_register);
    operands.push_back(source);
    uses.push_back(call_effect_from_operand(source));
  } else if (instruction.source_memory.has_value()) {
    const auto source = make_memory_operand(*instruction.source_memory);
    operands.push_back(source);
    if (!instruction.source_memory_materializes_address) {
      uses.push_back(call_effect_from_operand(source));
    }
  } else if (instruction.source_immediate.has_value()) {
    const auto source = make_immediate_operand(*instruction.source_immediate);
    operands.push_back(source);
    uses.push_back(call_effect_from_operand(source));
  } else if (instruction.move.from_value_id != 0) {
    uses.push_back(call_prepared_value_def(instruction.move.from_value_id, c4c::kInvalidValueName));
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
    defs.push_back(call_effect_from_operand(*instruction.result));
  }
  std::vector<MachineEffectResource> uses = call_effects_from_operands(operands);
  std::vector<MachineSideEffectKind> side_effects = {MachineSideEffectKind::Call};
  if (instruction.memory_return_storage.has_value()) {
    defs.push_back(call_effect_from_operand(make_memory_operand(*instruction.memory_return_storage)));
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_va_start.has_value()) {
    defs.push_back(call_prepared_value_def(
        instruction.variadic_va_start->destination_va_list.value_id,
        instruction.variadic_va_start->destination_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_scalar_va_arg.has_value()) {
    defs.push_back(call_prepared_value_def(
        instruction.variadic_scalar_va_arg->result_home.value_id,
        instruction.variadic_scalar_va_arg->result_home.value_name));
    uses.push_back(call_prepared_value_def(
        instruction.variadic_scalar_va_arg->source_va_list.value_id,
        instruction.variadic_scalar_va_arg->source_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryRead);
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_aggregate_va_arg.has_value()) {
    defs.push_back(call_prepared_value_def(
        instruction.variadic_aggregate_va_arg->destination_payload_home.value_id,
        instruction.variadic_aggregate_va_arg->destination_payload_home.value_name));
    uses.push_back(call_prepared_value_def(
        instruction.variadic_aggregate_va_arg->source_va_list.value_id,
        instruction.variadic_aggregate_va_arg->source_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryRead);
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_va_copy.has_value()) {
    defs.push_back(call_prepared_value_def(
        instruction.variadic_va_copy->destination_va_list.value_id,
        instruction.variadic_va_copy->destination_va_list.value_name));
    uses.push_back(call_prepared_value_def(
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

mir::TargetInstructionPrintResult print_call(const InstructionRecord& instruction,
                                             const CallInstructionRecord& call) {
  const auto mnemonic = required_primary_mnemonic(instruction);
  if (auto variadic_print = print_variadic_call(call, bad_header(instruction), mnemonic)) {
    return *variadic_print;
  }
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) + "call mnemonic is not printable");
  }
  if (call.is_indirect) {
    if (!call.indirect_callee.has_value() ||
        !call.prepared_indirect_callee.has_value() || call.source_call == nullptr) {
      return target_unsupported(bad_header(instruction) +
                                "indirect call node is missing prepared callee provenance");
    }
    const auto* callee = std::get_if<RegisterOperand>(&call.indirect_callee->payload);
    if (call.indirect_callee->kind != OperandKind::Register || callee == nullptr) {
      return target_unsupported(bad_header(instruction) +
                                "indirect call callee is not a register operand");
    }
    std::ostringstream out;
    out << mnemonic << " " << register_name(*callee);
    if (call.outgoing_stack_argument_bytes == 0) {
      return target_printed({out.str()});
    }
    return target_printed({
        "sub sp, sp, #" + std::to_string(call.outgoing_stack_argument_bytes),
        out.str(),
        "add sp, sp, #" + std::to_string(call.outgoing_stack_argument_bytes),
    });
  }
  if (!call.direct_callee.has_value() || call.direct_callee_label.empty() ||
      call.source_call == nullptr || !call.wrapper_kind.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "direct call node is missing prepared callee provenance");
  }

  std::ostringstream out;
  out << mnemonic << " " << call.direct_callee_label;
  if (call.outgoing_stack_argument_bytes == 0) {
    return target_printed({out.str()});
  }
  return target_printed({
      "sub sp, sp, #" + std::to_string(call.outgoing_stack_argument_bytes),
      out.str(),
      "add sp, sp, #" + std::to_string(call.outgoing_stack_argument_bytes),
  });
}

mir::TargetInstructionPrintResult print_call_boundary_move(
    const InstructionRecord& instruction,
    const CallBoundaryMoveInstructionRecord& move) {
  if (move.source_bundle == nullptr ||
      (move.source_move == nullptr && !move.source_immediate.has_value() &&
       !move.source_memory.has_value())) {
    return target_unsupported(bad_header(instruction) +
                              "call-boundary move node is missing prepared move provenance");
  }
    if ((!move.source_register.has_value() && !move.source_immediate.has_value() &&
         !move.source_memory.has_value()) ||
        !move.destination_register.has_value()) {
      return target_unsupported(
          bad_header(instruction) +
          "call-boundary move node requires prepared register source and destination");
    }
  if (const auto lines = print_aggregate_register_lane_publication_lines(move);
      lines.has_value()) {
    return target_printed(*lines);
  }
    if (move.source_memory.has_value()) {
      if (move.source_memory->support != MemoryOperandSupportKind::Prepared ||
          move.source_memory->base_kind != MemoryBaseKind::FrameSlot ||
        !move.source_memory->frame_slot_id.has_value() ||
        !move.source_memory->byte_offset_is_prepared_snapshot ||
        !move.source_memory->can_use_base_plus_offset) {
      return target_unsupported(
          bad_header(instruction) +
          "call-boundary frame-slot move requires a prepared frame-slot address");
    }
    if (move.source_memory_materializes_address) {
      const auto lines = materialize_call_boundary_frame_slot_address_lines(
          move.destination_register->reg, *move.source_memory);
      if (lines.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "call-boundary frame-slot address is not printable");
      }
      return target_printed(lines);
    }
    const auto lines = print_call_boundary_frame_slot_load_lines(move);
    if (!lines.has_value()) {
      return target_unsupported(bad_header(instruction) +
                                "call-boundary frame-slot address is not printable");
    }
    return target_printed(*lines);
  }
  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "call-boundary move mnemonic is not printable");
  }
  if (!move.source_register.has_value() && move.source_immediate.has_value()) {
    if (abi::is_fp_simd_register(move.destination_register->reg)) {
      const auto fp_view = scalar_fp_register_view(move.source_immediate->type);
      if (!fp_view.has_value() || move.destination_register->reg.view != *fp_view) {
        return target_unsupported(
            bad_header(instruction) +
            "call-boundary immediate FP move requires matching scalar FPR destination");
      }
      const auto gp_scratch_base = abi::reserved_mir_scratch_gp_registers().front();
      const auto gp_scratch = abi::gp_register(
          gp_scratch_base.index,
          move.source_immediate->type == bir::TypeKind::F32 ? abi::RegisterView::W
                                                            : abi::RegisterView::X);
      if (!gp_scratch.has_value()) {
        return target_unsupported(
            bad_header(instruction) +
            "call-boundary immediate FP move requires scratch GPR materialization");
      }
      const unsigned width_bits = move.source_immediate->type == bir::TypeKind::F32 ? 32U : 64U;
      auto lines = materialize_integer_constant_lines(
          *gp_scratch, move.source_immediate->unsigned_value, width_bits);
      if (lines.empty()) {
        return target_unsupported(
            bad_header(instruction) +
            "call-boundary immediate FP move could not materialize literal bits");
      }
      lines.push_back("fmov " + std::string{register_name(*move.destination_register)} +
                      ", " + std::string{abi::register_name(*gp_scratch)});
      return target_printed(std::move(lines));
    }
    const auto width_bits = scalar_integer_width_bits(move.source_immediate->type);
    if (!width_bits.has_value() || !abi::is_gp_register(move.destination_register->reg)) {
      return target_unsupported(
          bad_header(instruction) +
          "call-boundary immediate move requires scalar integer GPR materialization");
    }
    const auto value =
        scalar_integer_immediate_bits(*move.source_immediate, *width_bits);
    if (!is_single_move_wide_immediate(value, *width_bits)) {
      return target_printed(materialize_integer_constant_lines(
          move.destination_register->reg, value, *width_bits));
    }
  }
  std::ostringstream out;
  const bool f128_q_register_move =
      move.source_register.has_value() &&
      move.source_f128_carrier != nullptr &&
      move.source_register->expected_view == abi::RegisterView::Q &&
      move.destination_register->expected_view == abi::RegisterView::Q;
  if (f128_q_register_move) {
    const auto destination =
        f128_call_boundary_vector_register_name(*move.destination_register);
    const auto source =
        f128_call_boundary_vector_register_name(*move.source_register);
    if (!destination.has_value() || !source.has_value()) {
      return target_unsupported(
          bad_header(instruction) +
          "binary128 call-boundary q-register move is not printable");
    }
    out << "mov " << *destination << ", " << *source;
    return target_printed({out.str()});
  }
  const bool scalar_fp_register_move =
      move.source_register.has_value() &&
      abi::is_fp_simd_register(move.source_register->reg) &&
      abi::is_fp_simd_register(move.destination_register->reg) &&
      (move.source_register->reg.view == abi::RegisterView::S ||
       move.source_register->reg.view == abi::RegisterView::D) &&
      move.source_register->reg.view == move.destination_register->reg.view;
  out << (scalar_fp_register_move ? "fmov" : mnemonic) << " "
      << register_name(*move.destination_register) << ", ";
  if (move.source_register.has_value()) {
    out << register_name(*move.source_register);
  } else {
    out << "#" << move.source_immediate->signed_value;
  }
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_call_boundary_abi_binding(
    const InstructionRecord& instruction,
    const CallBoundaryAbiBindingInstructionRecord& binding) {
  if (binding.source_bundle == nullptr || binding.source_binding == nullptr) {
    return target_unsupported(
        bad_header(instruction) +
        "call-boundary ABI binding node is missing prepared binding provenance");
  }
  return target_unsupported(
      bad_header(instruction) +
      "call-boundary ABI binding node requires later AArch64 move lowering");
}


}  // namespace c4c::backend::aarch64::codegen
