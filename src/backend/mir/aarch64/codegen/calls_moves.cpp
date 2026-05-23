#include "calls_moves.hpp"

#include "calls_argument_sources.hpp"
#include "calls_byval_aggregates.hpp"
#include "calls_common.hpp"
#include "calls_preservation.hpp"
#include "dispatch_lookup.hpp"
#include "f128.hpp"
#include "machine_printer.hpp"
#include "float_ops.hpp"
#include "memory.hpp"

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

namespace {

MachineEffectResource effect_from_operand(const OperandRecord& operand) {
  MachineEffectResource resource;
  resource.operand = operand;
  switch (operand.kind) {
    case OperandKind::Register: {
      const auto* reg = std::get_if<RegisterOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::Register;
      if (reg != nullptr) {
        resource.value_id = reg->value_id;
        resource.value_name = reg->value_name;
        resource.reg = reg->reg;
      }
      break;
    }
    case OperandKind::Immediate: {
      const auto* immediate = std::get_if<ImmediateOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::PreparedValue;
      if (immediate != nullptr) {
        resource.value_id = immediate->source_value_id;
        resource.value_name = immediate->source_value_name;
      }
      break;
    }
    case OperandKind::PreparedValue: {
      const auto* value = std::get_if<PreparedValueOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::PreparedValue;
      if (value != nullptr) {
        resource.value_id = value->value_id;
        resource.value_name = value->value_name;
      }
      break;
    }
    case OperandKind::FrameSlot: {
      const auto* slot = std::get_if<FrameSlotOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::FrameSlot;
      if (slot != nullptr) {
        resource.frame_slot_id = slot->slot_id;
        if (slot->value_name.has_value()) {
          resource.value_name = *slot->value_name;
        }
      }
      break;
    }
    case OperandKind::Symbol: {
      const auto* symbol = std::get_if<SymbolOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::Symbol;
      if (symbol != nullptr) {
        resource.symbol_name = symbol->link_name;
      }
      break;
    }
    case OperandKind::BranchTarget: {
      const auto* target = std::get_if<BranchTargetOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::BranchTarget;
      if (target != nullptr) {
        resource.value_id = target->condition_value_id;
        resource.block_label = target->block_label;
      }
      break;
    }
    case OperandKind::Memory: {
      const auto* memory = std::get_if<MemoryOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::Memory;
      if (memory != nullptr) {
        resource.value_id = memory->result_value_id.has_value() ? memory->result_value_id
                                                                : memory->stored_value_id;
        if (memory->result_value_name.has_value()) {
          resource.value_name = *memory->result_value_name;
        } else if (memory->stored_value_name.has_value()) {
          resource.value_name = *memory->stored_value_name;
        }
        resource.frame_slot_id = memory->frame_slot_id;
        resource.symbol_name = memory->symbol_name.has_value() ? memory->symbol_name
                                                               : memory->string_symbol_name;
      }
      break;
    }
  }
  return resource;
}

[[nodiscard]] bool call_boundary_frame_slot_direct_offset_is_encodable(
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

[[nodiscard]] std::string stack_copy_address(std::string_view base,
                                             std::int64_t offset) {
  std::ostringstream out;
  out << "[" << base;
  if (offset != 0) {
    out << ", #" << offset;
  }
  out << "]";
  return out.str();
}

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

[[nodiscard]] module::MachineInstruction make_outgoing_stack_base_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::size_t outgoing_bytes) {
  const auto scratch = outgoing_stack_argument_base_register();
  std::ostringstream asm_text;
  asm_text << "sub " << abi::register_name(scratch) << ", sp, #" << outgoing_bytes;
  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .defs = {MachineEffectResource{
          .kind = MachineEffectResourceKind::Register,
          .reg = scratch,
      }},
      .side_effects = {MachineSideEffectKind::InlineAssembly},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = asm_text.str(),
      },
  };
  return make_call_boundary_machine_instruction(context, instruction_index, std::move(target));
}

[[nodiscard]] std::optional<std::vector<std::string>>
fragmented_aggregate_register_lane_publication_lines(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    const RegisterOperand& destination,
    std::size_t size_bytes,
    std::size_t source_instruction_index,
    std::size_t instruction_index,
    std::vector<MemoryOperand>& source_operands) {
  const auto stores =
      collect_byval_register_lane_stores(context, source_home, source_instruction_index);
  if (stores.empty() || stores.front().source_offset != 0) {
    return std::nullopt;
  }
  const auto scratch = aggregate_register_lane_scratch(destination);
  if (!scratch.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  std::size_t covered_bytes = 0;
  std::size_t lane_index = 0;
  std::size_t lane_offset = 0;
  const std::string scratch_x = abi::register_name(*scratch);
  for (const auto& store : stores) {
    if (store.source_offset > covered_bytes) {
      return std::nullopt;
    }
    std::size_t store_offset = covered_bytes - store.source_offset;
    while (covered_bytes < size_bytes && store_offset < store.size_bytes) {
      const std::size_t lane_remaining = 8 - lane_offset;
      const std::size_t store_remaining = store.size_bytes - store_offset;
      const std::size_t total_remaining = size_bytes - covered_bytes;
      const std::size_t max_chunk =
          std::min({std::size_t{8}, lane_remaining, store_remaining, total_remaining});
      const auto lane_register =
          aggregate_register_lane_destination(destination, lane_index);
      if (!lane_register.has_value()) {
        return std::nullopt;
      }
      std::optional<MemoryOperand> source_memory;
      bool source_memory_needs_materialized_address = false;
      std::size_t chunk_width = 0;
      for (const std::size_t candidate : {std::size_t{8},
                                          std::size_t{4},
                                          std::size_t{2},
                                          std::size_t{1}}) {
        if (candidate > max_chunk) {
          continue;
        }
        auto candidate_memory =
            aggregate_lane_store_memory(context,
                                        argument,
                                        source_home,
                                        store,
                                        store_offset,
                                        candidate,
                                        instruction_index);
        if (candidate_memory.has_value() &&
            !aggregate_register_lane_load_mnemonic(candidate).empty()) {
          const bool printable =
              aggregate_register_lane_memory_is_printable(*candidate_memory, candidate);
          const bool materializable =
              !printable &&
              candidate_memory->base_kind == MemoryBaseKind::FrameSlot &&
              !materialize_call_boundary_frame_slot_address_lines(
                   *scratch, *candidate_memory).empty();
          if (!printable && !materializable) {
            continue;
          }
          source_memory = std::move(candidate_memory);
          source_memory_needs_materialized_address = materializable;
          chunk_width = candidate;
          break;
        }
      }
      if (!source_memory.has_value() || chunk_width == 0) {
        return std::nullopt;
      }
      const auto mnemonic = aggregate_register_lane_load_mnemonic(chunk_width);
      const bool first_chunk = lane_offset == 0;
      const auto load_register =
          first_chunk
              ? aggregate_register_lane_load_register(*lane_register, chunk_width)
              : aggregate_register_lane_load_register(*scratch, chunk_width);
      if (source_memory_needs_materialized_address) {
        auto address_lines =
            materialize_call_boundary_frame_slot_address_lines(*scratch, *source_memory);
        lines.insert(lines.end(), address_lines.begin(), address_lines.end());
      }
      lines.push_back(std::string{mnemonic} + " " +
                      std::string{abi::register_name(load_register)} + ", " +
                      (source_memory_needs_materialized_address
                           ? ("[" + scratch_x + "]")
                           : memory_address(*source_memory)));
      source_operands.push_back(*source_memory);
      if (!first_chunk) {
        lines.push_back("orr " + std::string{abi::register_name(*lane_register)} +
                        ", " + std::string{abi::register_name(*lane_register)} +
                        ", " + scratch_x + ", lsl #" +
                        std::to_string(lane_offset * 8));
      }
      covered_bytes += chunk_width;
      store_offset += chunk_width;
      lane_offset += chunk_width;
      if (lane_offset == 8) {
        lane_offset = 0;
        ++lane_index;
      }
    }
    if (covered_bytes >= size_bytes) {
      break;
    }
  }
  if (covered_bytes == 0) {
    return std::nullopt;
  }
  return lines;
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_fragmented_aggregate_register_lane_publication_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedMoveResolution& move,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    const RegisterOperand& destination,
    std::size_t size_bytes,
    std::size_t source_instruction_index,
    std::size_t instruction_index) {
  std::vector<MemoryOperand> source_operands;
  const auto lines = fragmented_aggregate_register_lane_publication_lines(
      context,
      argument,
      source_home,
      destination,
      size_bytes,
      source_instruction_index,
      instruction_index,
      source_operands);
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

  std::vector<OperandRecord> operands;
  std::vector<MachineEffectResource> uses;
  const auto destination_operand = make_register_operand(destination);
  operands.push_back(destination_operand);
  for (const auto& source : source_operands) {
    const auto source_operand = make_memory_operand(source);
    operands.push_back(source_operand);
    uses.push_back(effect_from_operand(source_operand));
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = bundle.block_index,
      .instruction_index = bundle.instruction_index,
      .operands = operands,
      .defs = {effect_from_operand(destination_operand)},
      .uses = uses,
      .side_effects = {MachineSideEffectKind::MemoryRead,
                       MachineSideEffectKind::InlineAssembly},
      .payload =
          AssemblerInstructionRecord{
              .operands = std::move(operands),
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  (void)move;
  return make_call_boundary_machine_instruction(context, instruction_index,
                                                std::move(target));
}

[[nodiscard]] std::optional<std::string_view> value_move_load_mnemonic(
    std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return std::string_view{"ldrb"};
    case 2:
      return std::string_view{"ldrh"};
    case 4:
    case 8:
      return std::string_view{"ldr"};
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string_view> value_move_store_mnemonic(
    std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return std::string_view{"strb"};
    case 2:
      return std::string_view{"strh"};
    case 4:
    case 8:
      return std::string_view{"str"};
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<abi::RegisterReference> value_move_scratch_register(
    std::size_t width_bytes) {
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.empty()) {
    return std::nullopt;
  }
  if (width_bytes == 1 || width_bytes == 2 || width_bytes == 4) {
    return abi::w_register(scratches.front().index);
  }
  if (width_bytes == 8) {
    return abi::x_register(scratches.front().index);
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string> value_move_frame_slot_address(
    const prepare::PreparedValueHome& home) {
  if (home.kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home.offset_bytes.has_value()) {
    return std::nullopt;
  }
  std::ostringstream out;
  out << "[sp";
  if (*home.offset_bytes != 0) {
    out << ", #" << *home.offset_bytes;
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_value_stack_move_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedMoveResolution& move,
    const prepare::PreparedValueHome& destination_home,
    const prepare::PreparedValueHome* source_home,
    std::size_t instruction_index) {
  const auto width_bytes =
      destination_home.size_bytes.value_or(source_home != nullptr
                                               ? source_home->size_bytes.value_or(4)
                                               : 4);
  const auto store_mnemonic = value_move_store_mnemonic(width_bytes);
  const auto destination = value_move_frame_slot_address(destination_home);
  if (!store_mnemonic.has_value() || !destination.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  std::string value_register;
  if (move.source_immediate_i32.has_value()) {
    const auto scratch = value_move_scratch_register(width_bytes);
    if (!scratch.has_value()) {
      return std::nullopt;
    }
    value_register = std::string{abi::register_name(*scratch)};
    auto materialized = materialize_integer_constant_lines(
        *scratch,
        static_cast<std::uint64_t>(
            static_cast<std::uint32_t>(*move.source_immediate_i32)),
        width_bytes == 8 ? 64U : 32U);
    if (materialized.empty()) {
      return std::nullopt;
    }
    lines.insert(lines.end(), materialized.begin(), materialized.end());
  } else if (source_home != nullptr &&
             source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
             source_home->offset_bytes.has_value()) {
    const auto scratch = value_move_scratch_register(width_bytes);
    const auto load_mnemonic = value_move_load_mnemonic(width_bytes);
    const auto source = value_move_frame_slot_address(*source_home);
    if (!scratch.has_value() || !load_mnemonic.has_value() || !source.has_value()) {
      return std::nullopt;
    }
    value_register = std::string{abi::register_name(*scratch)};
    lines.push_back(std::string{*load_mnemonic} + " " + value_register + ", " +
                    *source);
  } else if (source_home != nullptr &&
             source_home->kind == prepare::PreparedValueHomeKind::Register &&
             source_home->register_name.has_value()) {
    const auto parsed = abi::parse_aarch64_register_name(*source_home->register_name);
    if (!parsed.has_value() ||
        (parsed->bank != abi::RegisterBank::GeneralPurpose &&
         parsed->bank != abi::RegisterBank::FpSimd)) {
      return std::nullopt;
    }
    std::optional<abi::RegisterReference> source_reg;
    if (parsed->bank == abi::RegisterBank::FpSimd) {
      source_reg = abi::fp_simd_register(
          parsed->index,
          width_bytes == 8 ? abi::RegisterView::D : abi::RegisterView::S);
    } else {
      source_reg = width_bytes == 8 ? abi::x_register(parsed->index)
                                    : abi::w_register(parsed->index);
    }
    if (!source_reg.has_value()) {
      return std::nullopt;
    }
    value_register = std::string{abi::register_name(*source_reg)};
  } else {
    return std::nullopt;
  }
  lines.push_back(std::string{*store_mnemonic} + " " + value_register + ", " +
                  *destination);

  std::string asm_text;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (index != 0) {
      asm_text += '\n';
    }
    asm_text += lines[index];
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .defs = {MachineEffectResource{
          .kind = MachineEffectResourceKind::PreparedValue,
          .value_id = destination_home.value_id,
          .value_name = destination_home.value_name,
      }},
      .uses = source_home != nullptr
                  ? std::vector<MachineEffectResource>{MachineEffectResource{
                        .kind = MachineEffectResourceKind::PreparedValue,
                        .value_id = source_home->value_id,
                        .value_name = source_home->value_name,
                    }}
                  : std::vector<MachineEffectResource>{},
      .side_effects = {MachineSideEffectKind::MemoryWrite,
                       MachineSideEffectKind::InlineAssembly},
      .payload =
          AssemblerInstructionRecord{
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  return make_call_boundary_machine_instruction(context, instruction_index,
                                                std::move(target));
}

[[nodiscard]] module::MachineInstruction make_aggregate_stack_copy_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const MemoryOperand& source,
    const MemoryOperand& destination) {
  const auto source_base = abi::register_name(source.base_register->reg);
  const std::string destination_base =
      destination.base_kind == MemoryBaseKind::Register && destination.base_register.has_value()
          ? std::string{abi::register_name(destination.base_register->reg)}
          : std::string{"sp"};
  std::string asm_text;
  std::size_t offset = 0;
  for (const auto width : aggregate_stack_copy_chunks(source.size_bytes)) {
    const auto scratch = aggregate_stack_copy_scratch(width);
    const auto load_mnemonic = aggregate_stack_copy_load_mnemonic(width);
    const auto store_mnemonic = aggregate_stack_copy_store_mnemonic(width);
    if (!scratch.has_value() || load_mnemonic.empty() || store_mnemonic.empty()) {
      continue;
    }
    if (!asm_text.empty()) {
      asm_text += '\n';
    }
    asm_text += std::string{load_mnemonic};
    asm_text += " ";
    asm_text += std::string{abi::register_name(*scratch)};
    asm_text += ", ";
    asm_text += stack_copy_address(
        source_base, source.byte_offset + static_cast<std::int64_t>(offset));
    asm_text += '\n';
    asm_text += std::string{store_mnemonic};
    asm_text += " ";
    asm_text += std::string{abi::register_name(*scratch)};
    asm_text += ", ";
    asm_text += stack_copy_address(
        destination_base, destination.byte_offset + static_cast<std::int64_t>(offset));
    offset += width;
  }

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
      .operands = {make_memory_operand(source), make_memory_operand(destination)},
      .defs = {effect_from_operand(make_memory_operand(destination))},
      .uses = {effect_from_operand(make_memory_operand(source))},
      .side_effects = {MachineSideEffectKind::MemoryRead,
                       MachineSideEffectKind::MemoryWrite,
                       MachineSideEffectKind::InlineAssembly},
      .payload =
          AssemblerInstructionRecord{
              .operands = {make_memory_operand(source), make_memory_operand(destination)},
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  return make_call_boundary_machine_instruction(context, instruction_index,
                                                std::move(target));
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_byval_register_lane_stack_publication_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const MemoryOperand& source,
    const MemoryOperand& destination) {
  if (source.size_bytes == 0 || source.size_bytes != destination.size_bytes) {
    return std::nullopt;
  }

  auto chunks = aggregate_stack_copy_chunks(source.size_bytes);
  bool printable_chunks = true;
  std::size_t printable_offset = 0;
  for (const auto width : chunks) {
    auto source_chunk =
        aggregate_register_lane_memory(source, printable_offset, width);
    auto destination_chunk =
        aggregate_register_lane_memory(destination, printable_offset, width);
    if (!aggregate_register_lane_memory_is_printable(source_chunk, width) ||
        !aggregate_register_lane_memory_is_printable(destination_chunk, width)) {
      printable_chunks = false;
      break;
    }
    printable_offset += width;
  }
  if (!printable_chunks) {
    chunks.assign(source.size_bytes, std::size_t{1});
  }

  std::string asm_text;
  std::size_t offset = 0;
  for (const auto width : chunks) {
    auto source_chunk = aggregate_register_lane_memory(source, offset, width);
    auto destination_chunk =
        aggregate_register_lane_memory(destination, offset, width);
    if (!aggregate_register_lane_memory_is_printable(source_chunk, width) ||
        !aggregate_register_lane_memory_is_printable(destination_chunk, width)) {
      return std::nullopt;
    }
    const auto scratch = aggregate_stack_copy_scratch(width);
    const auto load_mnemonic = aggregate_stack_copy_load_mnemonic(width);
    const auto store_mnemonic = aggregate_stack_copy_store_mnemonic(width);
    if (!scratch.has_value() || load_mnemonic.empty() || store_mnemonic.empty()) {
      return std::nullopt;
    }
    if (!asm_text.empty()) {
      asm_text += '\n';
    }
    asm_text += std::string{load_mnemonic};
    asm_text += " ";
    asm_text += std::string{abi::register_name(*scratch)};
    asm_text += ", ";
    asm_text += memory_address(source_chunk);
    asm_text += '\n';
    asm_text += std::string{store_mnemonic};
    asm_text += " ";
    asm_text += std::string{abi::register_name(*scratch)};
    asm_text += ", ";
    asm_text += memory_address(destination_chunk);
    offset += width;
  }
  if (asm_text.empty()) {
    return std::nullopt;
  }

  const auto source_operand = make_memory_operand(source);
  const auto destination_operand = make_memory_operand(destination);
  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .operands = {source_operand, destination_operand},
      .defs = {effect_from_operand(destination_operand)},
      .uses = {effect_from_operand(source_operand)},
      .side_effects = {MachineSideEffectKind::MemoryRead,
                       MachineSideEffectKind::MemoryWrite,
                       MachineSideEffectKind::InlineAssembly},
      .payload =
          AssemblerInstructionRecord{
              .operands = {source_operand, destination_operand},
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  return make_call_boundary_machine_instruction(context, instruction_index,
                                                std::move(target));
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_fragmented_byval_register_lane_stack_publication_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    std::size_t size_bytes,
    std::size_t source_instruction_index,
    std::size_t instruction_index,
    const MemoryOperand& destination) {
  const auto stores =
      collect_byval_register_lane_stores(context, source_home, source_instruction_index);
  if (stores.empty() || stores.front().source_offset != 0) {
    return std::nullopt;
  }

  std::string asm_text;
  std::vector<OperandRecord> operands;
  std::vector<MachineEffectResource> uses;
  std::size_t covered_bytes = 0;
  for (const auto& store : stores) {
    if (store.source_offset > covered_bytes) {
      return std::nullopt;
    }
    std::size_t store_offset = covered_bytes - store.source_offset;
    while (covered_bytes < size_bytes && store_offset < store.size_bytes) {
      const std::size_t store_remaining = store.size_bytes - store_offset;
      const std::size_t total_remaining = size_bytes - covered_bytes;
      const std::size_t max_chunk = std::min<std::size_t>(store_remaining, total_remaining);
      std::optional<MemoryOperand> source_memory;
      bool source_memory_needs_materialized_address = false;
      std::size_t chunk_width = 0;
      for (const std::size_t candidate : {std::size_t{8},
                                          std::size_t{4},
                                          std::size_t{2},
                                          std::size_t{1}}) {
        if (candidate > max_chunk) {
          continue;
        }
        auto candidate_source = aggregate_lane_store_memory(context,
                                                           argument,
                                                           source_home,
                                                           store,
                                                           store_offset,
                                                           candidate,
                                                           instruction_index);
        auto candidate_destination =
            aggregate_register_lane_memory(destination, covered_bytes, candidate);
        if (!candidate_source.has_value() ||
            !aggregate_register_lane_memory_is_printable(candidate_destination, candidate)) {
          continue;
        }
        const bool printable =
            aggregate_register_lane_memory_is_printable(*candidate_source, candidate);
        const auto scratch = aggregate_stack_copy_scratch(candidate);
        const bool materializable =
            !printable && scratch.has_value() &&
            candidate_source->base_kind == MemoryBaseKind::FrameSlot &&
            !materialize_call_boundary_frame_slot_address_lines(
                 abi::x_register(scratch->index), *candidate_source).empty();
        if ((!printable && !materializable) || !scratch.has_value() ||
            aggregate_stack_copy_load_mnemonic(candidate).empty() ||
            aggregate_stack_copy_store_mnemonic(candidate).empty()) {
          continue;
        }
        source_memory = std::move(candidate_source);
        source_memory_needs_materialized_address = materializable;
        chunk_width = candidate;
        break;
      }
      if (!source_memory.has_value() || chunk_width == 0) {
        return std::nullopt;
      }

      const auto scratch = aggregate_stack_copy_scratch(chunk_width);
      const auto load_mnemonic = aggregate_stack_copy_load_mnemonic(chunk_width);
      const auto store_mnemonic = aggregate_stack_copy_store_mnemonic(chunk_width);
      if (!scratch.has_value() || load_mnemonic.empty() || store_mnemonic.empty()) {
        return std::nullopt;
      }
      const auto destination_chunk =
          aggregate_register_lane_memory(destination, covered_bytes, chunk_width);
      if (!asm_text.empty()) {
        asm_text += '\n';
      }
      if (source_memory_needs_materialized_address) {
        auto address_lines = materialize_call_boundary_frame_slot_address_lines(
            abi::x_register(scratch->index), *source_memory);
        for (const auto& line : address_lines) {
          asm_text += line;
          asm_text += '\n';
        }
      }
      asm_text += std::string{load_mnemonic} + " " +
                  std::string{abi::register_name(*scratch)} + ", " +
                  (source_memory_needs_materialized_address
                       ? ("[" + std::string{abi::register_name(abi::x_register(scratch->index))} +
                          "]")
                       : memory_address(*source_memory));
      asm_text += '\n';
      asm_text += std::string{store_mnemonic} + " " +
                  std::string{abi::register_name(*scratch)} + ", " +
                  memory_address(destination_chunk);
      const auto source_operand = make_memory_operand(*source_memory);
      operands.push_back(source_operand);
      uses.push_back(effect_from_operand(source_operand));
      covered_bytes += chunk_width;
      store_offset += chunk_width;
    }
  }
  if (covered_bytes == 0 || asm_text.empty()) {
    return std::nullopt;
  }

  const auto destination_operand = make_memory_operand(destination);
  operands.push_back(destination_operand);
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
      .operands = operands,
      .defs = {effect_from_operand(destination_operand)},
      .uses = uses,
      .side_effects = {MachineSideEffectKind::MemoryRead,
                       MachineSideEffectKind::MemoryWrite,
                       MachineSideEffectKind::InlineAssembly},
      .payload =
          AssemblerInstructionRecord{
              .operands = std::move(operands),
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  return make_call_boundary_machine_instruction(context, instruction_index, std::move(target));
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

[[nodiscard]] std::optional<abi::RegisterView> scalar_fp_register_view(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::F32:
      return abi::RegisterView::S;
    case bir::TypeKind::F64:
      return abi::RegisterView::D;
    default:
      return std::nullopt;
  }
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
      .defs = {effect_from_operand(destination_operand)},
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

[[nodiscard]] std::optional<module::MachineInstruction> lower_before_call_move(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedMoveResolution& move,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* source_home =
      context.function.value_locations == nullptr
          ? nullptr
          : find_value_home(context, move.from_value_id);
  const auto* argument = find_prepared_argument_plan(call_plan, move);
  const auto* binding = find_prepared_argument_binding(bundle, move);
  const auto* f128_carriers =
      context.function.prepared == nullptr
          ? nullptr
          : prepare::find_prepared_f128_carriers(
                *context.function.prepared,
                context.function.control_flow != nullptr
                    ? context.function.control_flow->function_name
                    : c4c::kInvalidFunctionName);
  const auto* source_f128_carrier =
      f128_carriers != nullptr && source_home != nullptr
          ? prepare::find_prepared_f128_carrier(*f128_carriers, source_home->value_name)
          : nullptr;
  if (source_f128_carrier == nullptr && f128_carriers != nullptr && argument != nullptr &&
      argument->source_value_id.has_value()) {
    source_f128_carrier =
        prepare::find_prepared_f128_carrier(*f128_carriers, *argument->source_value_id);
  }
  if (source_f128_carrier == nullptr && context.function.prepared != nullptr &&
      argument != nullptr && argument->source_value_id.has_value()) {
    source_f128_carrier =
        find_prepared_f128_carrier_in_module(*context.function.prepared,
                                             *argument->source_value_id);
  }
  const bool structured_f128_register_argument_move =
      argument != nullptr &&
      (argument->source_register_bank == prepare::PreparedRegisterBank::Vreg ||
       argument->source_register_bank == prepare::PreparedRegisterBank::Fpr) &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Vreg &&
      complete_full_width_f128_carrier(source_f128_carrier);

  CallBoundaryMoveInstructionRecord move_record{
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .phase = bundle.phase,
      .authority_kind = bundle.authority_kind,
      .block_index = bundle.block_index,
      .instruction_index = bundle.instruction_index,
      .source_parallel_copy_predecessor_label =
          bundle.source_parallel_copy_predecessor_label,
      .source_parallel_copy_successor_label =
          bundle.source_parallel_copy_successor_label,
      .move = move,
      .source_bundle = &bundle,
      .source_move = &move,
  };
  const auto aggregate_lane_size =
      byval_register_lane_size_bytes(context, move, call_plan.instruction_index);

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      argument != nullptr) {
    const bool frame_slot_address_argument =
        source_home != nullptr &&
        argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
        (make_sret_memory_return_address_source(
             context, call_plan, *argument, instruction_index)
             .has_value() ||
         make_frame_slot_call_argument_address_source(
             context, *argument, *source_home, instruction_index)
             .has_value());
    const bool register_byval_argument =
        aarch64_register_byval_argument_size_bytes(
            context, *argument, call_plan.instruction_index)
            .has_value();
    if (!frame_slot_address_argument && !structured_f128_register_argument_move &&
        !register_byval_argument) {
      auto preserved_source = make_prior_preserved_call_argument_source(
          context,
          call_plan,
          *argument,
          move,
          source_home,
          instruction_index,
          diagnostics);
      if (preserved_source.has_value()) {
        auto destination = make_register_operand_from_prepared_authority(
            binding != nullptr && binding->destination_register_name.has_value()
                ? binding->destination_register_name
                : move.destination_register_name,
            binding != nullptr && binding->destination_register_placement.has_value()
                ? binding->destination_register_placement
                : move.destination_register_placement,
            argument->destination_register_bank,
            RegisterOperandRole::CallAbi,
            move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                                  : argument->source_value_id,
            preserved_source->preserved != nullptr
                ? preserved_source->preserved->value_name
                : c4c::kInvalidValueName,
            binding != nullptr ? binding->destination_contiguous_width
                               : move.destination_contiguous_width,
            binding != nullptr ? binding->destination_occupied_register_names
                               : move.destination_occupied_register_names,
            preserved_source->source_memory.has_value()
                ? scalar_integer_register_view_from_size(
                      preserved_source->source_memory->size_bytes)
                : (source_home != nullptr && source_home->size_bytes.has_value()
                       ? scalar_integer_register_view_from_size(*source_home->size_bytes)
                       : scalar_view_from_register_name(move.destination_register_name)),
            diagnostics,
            context,
            instruction_index);
        if (destination.has_value()) {
          move_record.source_register = preserved_source->source_register;
          move_record.source_memory = preserved_source->source_memory;
          move_record.destination_register = *destination;
          return make_call_boundary_machine_instruction(
              context,
              instruction_index,
              make_call_boundary_move_instruction(std::move(move_record)));
        }
      }
    }
  }

  const bool selected_gpr_argument_move =
      argument != nullptr &&
      (argument->source_encoding == prepare::PreparedStorageEncodingKind::Register ||
       argument->source_encoding == prepare::PreparedStorageEncodingKind::SymbolAddress) &&
      argument->source_register_name.has_value() &&
      argument->source_register_bank == prepare::PreparedRegisterBank::Gpr &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Gpr;
  const bool selected_f128_argument_move =
      structured_f128_register_argument_move;
  const bool selected_scalar_fpr_argument_move =
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::Register &&
      argument->source_register_bank == prepare::PreparedRegisterBank::Fpr &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Fpr &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register) &&
      scalar_fp_view_from_register_name(
          binding != nullptr && binding->destination_register_name.has_value()
              ? binding->destination_register_name
              : move.destination_register_name).has_value();
  const bool selected_f128_constant_argument_move =
      argument != nullptr &&
      argument->value_bank == prepare::PreparedRegisterBank::Vreg &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::Immediate &&
      argument->source_literal.has_value() &&
      argument->source_literal->type == bir::TypeKind::F128 &&
      argument->source_literal->f128_payload.has_value() &&
      argument->source_value_id.has_value() &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Vreg &&
      complete_f128_constant_carrier(source_f128_carrier) &&
      source_f128_carrier->constant_payload->low_bits ==
          argument->source_literal->f128_payload->low_bits &&
      source_f128_carrier->constant_payload->high_bits ==
          argument->source_literal->f128_payload->high_bits &&
      binding != nullptr &&
      binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register;

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::Immediate &&
      argument->value_bank == prepare::PreparedRegisterBank::Vreg &&
      !selected_f128_constant_argument_move) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        "AArch64 binary128 constant argument move requires a complete structured full-width constant carrier");
    return std::nullopt;
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::Register &&
      source_home->register_name.has_value() && argument != nullptr &&
      (selected_gpr_argument_move || selected_scalar_fpr_argument_move ||
       selected_f128_argument_move) &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register) &&
      !move_record.destination_register.has_value()) {
    if (argument->source_register_name.has_value() &&
        *argument->source_register_name != *source_home->register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 call-boundary move source register disagrees with prepared value home");
      return std::nullopt;
    }
    if (selected_f128_argument_move &&
        source_f128_carrier->register_name != source_home->register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 binary128 call-boundary move source register disagrees with prepared f128 carrier");
      return std::nullopt;
    }
    const auto expected_view =
        selected_f128_argument_move ? std::optional<abi::RegisterView>{abi::RegisterView::Q}
        : selected_scalar_fpr_argument_move
            ? scalar_fp_view_from_register_name(
                  binding != nullptr && binding->destination_register_name.has_value()
                      ? binding->destination_register_name
                      : move.destination_register_name)
            : std::nullopt;
    const auto source_register_name =
        selected_scalar_fpr_argument_move &&
                argument->source_register_placement.has_value()
            ? std::optional<std::string>{}
            : source_home->register_name;
    auto destination = make_register_operand_from_prepared_authority(
        binding != nullptr && binding->destination_register_name.has_value()
            ? binding->destination_register_name
            : move.destination_register_name,
        binding != nullptr && binding->destination_register_placement.has_value()
            ? binding->destination_register_placement
            : (move.destination_register_placement.has_value()
                   ? move.destination_register_placement
                   : argument->destination_register_placement),
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : std::nullopt,
        source_home->value_name,
        binding != nullptr ? binding->destination_contiguous_width
                           : move.destination_contiguous_width,
        binding != nullptr ? binding->destination_occupied_register_names
                           : move.destination_occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    if (!destination.has_value()) {
      return std::nullopt;
    }
    move_record.destination_register = *destination;
    if (selected_f128_argument_move) {
      auto source = make_f128_q_register_operand_from_carrier(
          *source_f128_carrier,
          RegisterOperandRole::CallAbi,
          source_home->value_id,
          source_home->value_name,
          diagnostics,
          context,
          instruction_index);
      if (!source.has_value()) {
        return std::nullopt;
      }
      move_record.source_register = *source;
      move_record.source_f128_carrier = source_f128_carrier;
    } else {
      auto source = make_register_operand_from_prepared_authority(
          source_register_name,
          argument->source_register_placement,
          argument->source_register_bank,
          RegisterOperandRole::CallAbi,
          source_home->value_id,
          source_home->value_name,
          1,
          std::vector<std::string>{},
          expected_view,
          diagnostics,
          context,
          instruction_index);
      if (!source.has_value()) {
        return std::nullopt;
      }
      move_record.source_register = *source;
    }
    }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::Register &&
      argument != nullptr &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Gpr &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register)) {
    const auto register_byval_size = aarch64_register_byval_argument_size_bytes(
        context, *argument, call_plan.instruction_index);
    std::optional<MemoryOperand> source;
    std::optional<MemoryOperand> address_source;
    if (register_byval_size.has_value()) {
      source = make_byval_register_lane_prepared_source(
          context, *argument, *source_home, *register_byval_size, call_plan.instruction_index);
      if (!source.has_value() && source_home->register_name.has_value()) {
        auto source_register = make_register_operand_from_prepared_authority(
            source_home->register_name,
            argument->source_register_placement,
            argument->source_register_bank.has_value()
                ? argument->source_register_bank
                : std::optional<prepare::PreparedRegisterBank>{
                      prepare::PreparedRegisterBank::Gpr},
            RegisterOperandRole::CallAbi,
            source_home->value_id,
            source_home->value_name,
            1,
            {},
            abi::RegisterView::X,
            diagnostics,
            context,
            instruction_index);
        if (source_register.has_value()) {
          source = make_aggregate_call_argument_source(
              context,
              *argument,
              *source_home,
              *source_register,
              *register_byval_size,
              source_home->pointer_byte_delta.value_or(0),
              instruction_index);
        }
      }
    } else {
      address_source =
          make_local_frame_address_call_argument_source(
              context, *argument, *source_home, instruction_index);
      source = address_source;
    }
    if (source.has_value()) {
      const auto destination_register_placement =
          binding != nullptr && binding->destination_register_placement.has_value()
              ? binding->destination_register_placement
              : (move.destination_register_placement.has_value()
                     ? move.destination_register_placement
                     : argument->destination_register_placement);
      const auto destination_register_name =
          destination_register_placement.has_value()
              ? std::optional<std::string>{}
              : (binding != nullptr && binding->destination_register_name.has_value()
                     ? binding->destination_register_name
                     : move.destination_register_name);
      auto destination = make_register_operand_from_prepared_authority(
          destination_register_name,
          destination_register_placement,
          argument->destination_register_bank,
          RegisterOperandRole::CallAbi,
          move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                                : argument->source_value_id,
          source_home->value_name,
          binding != nullptr ? binding->destination_contiguous_width
                             : move.destination_contiguous_width,
          binding != nullptr ? binding->destination_occupied_register_names
                             : move.destination_occupied_register_names,
          abi::RegisterView::X,
          diagnostics,
          context,
          instruction_index);
      if (!destination.has_value()) {
        return std::nullopt;
      }
      move_record.source_register.reset();
      move_record.source_memory = *source;
      move_record.source_memory_materializes_address = true;
      move_record.destination_register = *destination;
      if (register_byval_size.has_value()) {
        move_record.source_memory_materializes_address = false;
        move_record.move.reason = "call_arg_byval_aggregate_register_lanes";
      }
    }
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::Register &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::Register &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      move_record.destination_register.has_value()) {
    if (auto cast_publication =
            make_immediate_cast_call_argument_publication_instruction(
                context, *source_home, *move_record.destination_register, instruction_index)) {
      return cast_publication;
    }
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::Register &&
      source_home->register_name.has_value() &&
      argument != nullptr &&
      (argument->source_encoding == prepare::PreparedStorageEncodingKind::Register ||
       argument->source_encoding == prepare::PreparedStorageEncodingKind::ComputedAddress ||
       argument->source_encoding == prepare::PreparedStorageEncodingKind::SymbolAddress) &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Gpr &&
      is_aarch64_byval_register_lane_move(move) &&
      (argument->source_register_bank == prepare::PreparedRegisterBank::AggregateAddress ||
       argument->source_register_bank == prepare::PreparedRegisterBank::Gpr) &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register)) {
    const auto lane_size =
        aggregate_lane_size.has_value() ? aggregate_lane_size : source_home->size_bytes;
    if (!lane_size.has_value() || *lane_size == 0 || *lane_size > 16) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate register-lane call-argument publication requires a small ABI byval size");
      return std::nullopt;
    }
    if (argument->source_register_name.has_value() &&
        *argument->source_register_name != *source_home->register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 aggregate register-lane call-argument source register disagrees with prepared value home");
      return std::nullopt;
    }
    auto source = make_byval_register_lane_prepared_source(
        context, *argument, *source_home, *lane_size, call_plan.instruction_index);
    if (!source.has_value() && source_home->size_bytes.has_value()) {
      auto source_register = make_register_operand_from_prepared_authority(
          source_home->register_name,
          argument->source_register_placement,
          argument->source_register_bank.has_value()
              ? argument->source_register_bank
              : std::optional<prepare::PreparedRegisterBank>{
                    prepare::PreparedRegisterBank::Gpr},
          RegisterOperandRole::CallAbi,
          source_home->value_id,
          source_home->value_name,
          1,
          {},
          abi::RegisterView::X,
          diagnostics,
          context,
          instruction_index);
      if (source_register.has_value()) {
        source = make_aggregate_call_argument_source(
            context,
            *argument,
            *source_home,
            *source_register,
            *lane_size,
            source_home->pointer_byte_delta.value_or(0),
            instruction_index);
      }
    }
    const auto destination_register_placement =
        move.destination_register_placement.has_value()
            ? move.destination_register_placement
            : argument->destination_register_placement;
    const auto destination_register_name =
        destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : move.destination_register_name;
    auto destination = make_register_operand_from_prepared_authority(
        destination_register_name,
        destination_register_placement,
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_home->value_name,
        std::max(move.destination_contiguous_width,
                 argument->destination_contiguous_width),
        !move.destination_occupied_register_names.empty()
            ? move.destination_occupied_register_names
            : argument->destination_occupied_register_names,
        abi::RegisterView::X,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value() || !destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate register-lane call-argument publication requires prepared source bytes and destination register");
      return std::nullopt;
      }
    move_record.source_register.reset();
      move_record.source_memory = *source;
      move_record.destination_register = *destination;
    move_record.move.reason = "call_arg_byval_aggregate_register_lanes";
    }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Gpr &&
      is_aarch64_byval_register_lane_move(move) &&
      (argument->source_register_bank == prepare::PreparedRegisterBank::AggregateAddress ||
       argument->source_register_bank == prepare::PreparedRegisterBank::Gpr) &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register)) {
    const auto lane_size =
        aggregate_lane_size.has_value() ? aggregate_lane_size : source_home->size_bytes;
    if (!lane_size.has_value() || *lane_size == 0 || *lane_size > 16) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate register-lane call-argument publication requires a small ABI byval size");
      return std::nullopt;
    }
    auto source = make_byval_register_lane_prepared_source(
        context, *argument, *source_home, *lane_size, call_plan.instruction_index);
    const auto destination_register_placement =
        move.destination_register_placement.has_value()
            ? move.destination_register_placement
            : argument->destination_register_placement;
    const auto destination_register_name =
        destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : move.destination_register_name;
    auto destination = make_register_operand_from_prepared_authority(
        destination_register_name,
        destination_register_placement,
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_home->value_name,
        std::max(move.destination_contiguous_width,
                 argument->destination_contiguous_width),
        !move.destination_occupied_register_names.empty()
            ? move.destination_occupied_register_names
            : argument->destination_occupied_register_names,
        abi::RegisterView::X,
        diagnostics,
        context,
        instruction_index);
    if (!destination.has_value()) {
      return std::nullopt;
      }
    if (!source.has_value()) {
      if (auto fragmented =
              make_fragmented_aggregate_register_lane_publication_instruction(
                  context,
                  bundle,
                  move,
                  *argument,
                  *source_home,
                  *destination,
                  *lane_size,
                  call_plan.instruction_index,
                  instruction_index)) {
        return fragmented;
      }
    }
    if (!source.has_value() && source_home->size_bytes.has_value()) {
      source =
          make_frame_slot_call_argument_source(context, *argument, *source_home, instruction_index);
      if (source.has_value()) {
        source->size_bytes = *lane_size;
      }
    }
    if (!source.has_value() || source->size_bytes == 0 || source->size_bytes > 16) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate register-lane call-argument publication requires a small prepared frame-slot source");
      return std::nullopt;
    }
    move_record.source_register.reset();
      move_record.source_memory = *source;
      move_record.destination_register = *destination;
    move_record.move.reason = "call_arg_byval_aggregate_register_lanes";
    }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      !is_aarch64_byval_register_lane_move(move) &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->source_register_bank == prepare::PreparedRegisterBank::Fpr &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Fpr &&
      binding != nullptr &&
      binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      scalar_fp_view_from_register_name(binding->destination_register_name).has_value()) {
    auto source =
        make_frame_slot_call_argument_source(context, *argument, *source_home, instruction_index);
    const auto destination_register_placement =
        binding->destination_register_placement.has_value()
            ? binding->destination_register_placement
            : (move.destination_register_placement.has_value()
                   ? move.destination_register_placement
                   : argument->destination_register_placement);
    const auto destination_register_name =
        destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : (binding->destination_register_name.has_value()
                   ? binding->destination_register_name
                   : move.destination_register_name);
    auto destination = make_register_operand_from_prepared_authority(
        destination_register_name,
        destination_register_placement,
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_home->value_name,
        binding->destination_contiguous_width,
        binding->destination_occupied_register_names,
        scalar_fp_view_from_register_name(binding->destination_register_name),
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value() || !destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 scalar FPR frame-slot call-argument move requires a prepared load access for the source value");
      return std::nullopt;
    }
    move_record.source_memory = *source;
    move_record.destination_register = *destination;
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      !is_aarch64_byval_register_lane_move(move) &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      source_home->size_bytes == std::optional<std::size_t>{16} &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Vreg &&
      binding != nullptr &&
      binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register) {
    auto source =
        make_frame_slot_call_argument_source(context, *argument, *source_home, instruction_index);
    const auto destination_register_placement =
        binding->destination_register_placement.has_value()
            ? binding->destination_register_placement
            : (move.destination_register_placement.has_value()
                   ? move.destination_register_placement
                   : argument->destination_register_placement);
    const auto destination_register_name =
        destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : (binding->destination_register_name.has_value()
                   ? binding->destination_register_name
                   : move.destination_register_name);
    auto destination = make_register_operand_from_prepared_authority(
        destination_register_name,
        destination_register_placement,
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_home->value_name,
        binding->destination_contiguous_width,
        binding->destination_occupied_register_names,
        abi::RegisterView::Q,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value() || !destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 binary128 frame-slot call-argument move requires a prepared frame-slot source and q-register destination");
      return std::nullopt;
    }
    move_record.source_memory = *source;
    move_record.destination_register = *destination;
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      !is_aarch64_byval_register_lane_move(move) &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      (argument->source_register_bank == prepare::PreparedRegisterBank::Gpr ||
       argument->source_register_bank == prepare::PreparedRegisterBank::AggregateAddress) &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Gpr &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register)) {
    const auto register_byval_size = aarch64_register_byval_argument_size_bytes(
        context, *argument, call_plan.instruction_index);
    std::optional<MemoryOperand> address_source;
    std::optional<MemoryOperand> source;
    if (register_byval_size.has_value()) {
      source = make_byval_register_lane_prepared_source(
          context, *argument, *source_home, *register_byval_size, call_plan.instruction_index);
      if (!source.has_value()) {
        source = make_frame_slot_call_argument_source(
            context, *argument, *source_home, instruction_index);
        if (source.has_value()) {
          source->size_bytes = *register_byval_size;
        }
      }
    } else {
      address_source = make_sret_memory_return_address_source(
          context, call_plan, *argument, instruction_index);
      if (!address_source.has_value()) {
        address_source = make_frame_slot_call_argument_address_source(
            context, *argument, *source_home, instruction_index);
      }
      if (!address_source.has_value()) {
        if (const auto byval_size = aarch64_indirect_byval_argument_size_bytes(
                context, *argument, call_plan.instruction_index);
            byval_size.has_value()) {
          address_source = make_byval_register_lane_prepared_source(
              context, *argument, *source_home, *byval_size, call_plan.instruction_index);
        }
      }
      source =
          address_source.has_value()
              ? address_source
              : make_frame_slot_call_argument_source(
                    context, *argument, *source_home, instruction_index);
    }
    const auto destination_register_placement =
        binding != nullptr && binding->destination_register_placement.has_value()
            ? binding->destination_register_placement
            : (move.destination_register_placement.has_value()
                   ? move.destination_register_placement
                   : argument->destination_register_placement);
    const auto destination_register_name =
        destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : (binding != nullptr && binding->destination_register_name.has_value()
                   ? binding->destination_register_name
                   : move.destination_register_name);
    auto destination = make_register_operand_from_prepared_authority(
        destination_register_name,
        destination_register_placement,
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_home->value_name,
        binding != nullptr ? binding->destination_contiguous_width
                           : move.destination_contiguous_width,
        binding != nullptr ? binding->destination_occupied_register_names
                           : move.destination_occupied_register_names,
        register_byval_size.has_value() || address_source.has_value()
            ? std::optional<abi::RegisterView>{abi::RegisterView::X}
            : source.has_value()
            ? scalar_integer_register_view_from_size(source->size_bytes)
            : std::nullopt,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value() || !destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 frame-slot call-argument move requires a prepared load access for the source value");
      return std::nullopt;
    }
    move_record.source_memory = *source;
    move_record.source_memory_materializes_address = address_source.has_value();
    move_record.destination_register = *destination;
    if (register_byval_size.has_value()) {
      move_record.move.reason = "call_arg_byval_aggregate_register_lanes";
    }
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      (argument->value_bank == prepare::PreparedRegisterBank::Gpr ||
       argument->destination_register_bank == prepare::PreparedRegisterBank::Gpr) &&
      is_aarch64_byval_register_lane_move(move) &&
      (!argument->source_register_bank.has_value() ||
       argument->source_register_bank == prepare::PreparedRegisterBank::AggregateAddress ||
       argument->source_register_bank == prepare::PreparedRegisterBank::Gpr) &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot)) {
    const auto lane_size =
        aggregate_lane_size.has_value() ? aggregate_lane_size : source_home->size_bytes;
    if (!lane_size.has_value() || *lane_size == 0 || *lane_size > 16) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack-lane call-argument publication requires a small ABI byval size");
      return std::nullopt;
    }
    auto source = make_byval_register_lane_prepared_source(
        context, *argument, *source_home, *lane_size, call_plan.instruction_index);
    if (!source.has_value() && source_home->size_bytes.has_value()) {
      source =
          make_frame_slot_call_argument_source(context, *argument, *source_home, instruction_index);
      if (source.has_value()) {
        source->size_bytes = *lane_size;
      }
    }
    if (!source.has_value() || source->size_bytes == 0 || source->size_bytes > 16) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack-lane call-argument publication requires prepared source bytes");
      return std::nullopt;
    }
    const auto destination = make_stack_call_argument_destination(
        context, *argument, *source_home, move, binding, *source, instruction_index);
    if (!destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack-lane call-argument publication requires a prepared destination stack offset");
      return std::nullopt;
    }
    if (auto fragmented =
            make_fragmented_byval_register_lane_stack_publication_instruction(
                context,
                *argument,
                *source_home,
                *lane_size,
                call_plan.instruction_index,
                instruction_index,
                *destination)) {
      return fragmented;
    }
    auto lowered = make_byval_register_lane_stack_publication_instruction(
        context, instruction_index, *source, *destination);
    if (!lowered.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 aggregate stack-lane call-argument publication requires printable prepared source and destination stack slots");
      return std::nullopt;
    }
    return lowered;
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      argument != nullptr &&
      (argument->source_encoding == prepare::PreparedStorageEncodingKind::Register ||
       argument->source_encoding == prepare::PreparedStorageEncodingKind::ComputedAddress) &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->value_bank == prepare::PreparedRegisterBank::AggregateAddress &&
      (!argument->source_register_bank.has_value() ||
       argument->source_register_bank == prepare::PreparedRegisterBank::Gpr ||
       argument->source_register_bank == prepare::PreparedRegisterBank::AggregateAddress) &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot)) {
    const auto* source_register_home = source_home;
    if (source_home->kind == prepare::PreparedValueHomeKind::PointerBasePlusOffset &&
        argument->source_base_value_id.has_value() &&
        context.function.value_locations != nullptr) {
      source_register_home = prepare::find_prepared_value_home(
          *context.function.value_locations, *argument->source_base_value_id);
    }
    if (source_register_home == nullptr ||
        source_register_home->kind != prepare::PreparedValueHomeKind::Register ||
        !source_register_home->register_name.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack call-argument copy requires a prepared aggregate address register");
      return std::nullopt;
    }
    if (argument->source_register_name.has_value() &&
        *argument->source_register_name != *source_register_home->register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack call-argument source register disagrees with prepared value home");
      return std::nullopt;
    }
    const auto size_bytes =
        source_home->size_bytes.has_value()
            ? source_home->size_bytes
            : aarch64_stack_byval_argument_size_bytes(
                  context, *argument, call_plan.instruction_index);
    if (!size_bytes.has_value() || *size_bytes == 0) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack call-argument copy requires a prepared aggregate size");
      return std::nullopt;
    }
    auto source_register = make_register_operand_from_prepared_authority(
        source_register_home->register_name,
        argument->source_register_placement,
        argument->source_register_bank.has_value()
            ? argument->source_register_bank
            : std::optional<prepare::PreparedRegisterBank>{
                  prepare::PreparedRegisterBank::Gpr},
        RegisterOperandRole::CallAbi,
        source_register_home->value_id,
        source_register_home->value_name,
        1,
        {},
        abi::RegisterView::X,
        diagnostics,
        context,
        instruction_index);
    if (!source_register.has_value()) {
      return std::nullopt;
    }
    const auto source_byte_offset = source_home->pointer_byte_delta.value_or(0);
    const auto source = make_aggregate_call_argument_source(
        context,
        *argument,
        *source_home,
        *source_register,
        *size_bytes,
        source_byte_offset,
        instruction_index);
    if (!source.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack call-argument copy requires a prepared aggregate address source");
      return std::nullopt;
    }
    const auto destination = make_stack_call_argument_destination(
        context, *argument, *source_home, move, binding, *source, instruction_index);
    if (!destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack call-argument copy requires a prepared destination stack offset");
      return std::nullopt;
    }
    return make_aggregate_stack_copy_instruction(
        context, instruction_index, *source, *destination);
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      source_home->size_bytes == std::optional<std::size_t>{16} &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->value_bank == prepare::PreparedRegisterBank::Vreg &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot)) {
    const auto source = make_frame_slot_call_argument_source(
        context, *argument, *source_home, instruction_index);
    if (!source.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 binary128 stack call-argument move requires a prepared frame-slot source");
      return std::nullopt;
    }
    const auto destination = make_stack_call_argument_destination(
        context, *argument, *source_home, move, binding, *source, instruction_index);
    if (!destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 binary128 stack call-argument move requires a prepared destination stack offset");
      return std::nullopt;
    }
    if (f128_carriers == nullptr) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 binary128 stack call-argument move requires prepared f128 carrier facts");
      return std::nullopt;
    }
    auto prepared = make_prepared_f128_carrier_transport_record(
        *f128_carriers,
        source_home->value_name,
        F128TransportKind::StoreToMemory,
        *destination);
    if (!prepared.record.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 binary128 stack call-argument move requires a complete source carrier");
      return std::nullopt;
    }
    return make_call_boundary_machine_instruction(
        context,
        instruction_index,
        make_f128_transport_instruction(std::move(*prepared.record)));
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot)) {
    const auto source = make_frame_slot_call_argument_source(
        context, *argument, *source_home, instruction_index);
    if (!source.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 stack call-argument move requires a prepared frame-slot source");
      return std::nullopt;
    }
    const auto value_type = scalar_integer_type_from_size(source->size_bytes);
    if (!value_type.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 stack call-argument move lowering requires a 1, 4, or 8 byte prepared stack slot");
      return std::nullopt;
    }
    const auto destination = make_stack_call_argument_destination(
        context, *argument, *source_home, move, binding, *source, instruction_index);
    if (!destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 stack call-argument move requires a prepared destination stack offset");
      return std::nullopt;
    }
    return make_call_boundary_machine_instruction(
        context,
        instruction_index,
        make_memory_instruction(MemoryInstructionRecord{
            .memory_kind = MemoryInstructionKind::Store,
            .address = *destination,
            .value = make_memory_operand(*source),
            .value_type = *value_type,
        }));
  }

  if (selected_f128_constant_argument_move) {
    auto destination = make_register_operand_from_prepared_authority(
        binding->destination_register_name,
        binding->destination_register_placement,
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_f128_carrier->value_name,
        binding->destination_contiguous_width,
        binding->destination_occupied_register_names,
        abi::RegisterView::Q,
        diagnostics,
        context,
        instruction_index);
    if (!destination.has_value()) {
      return std::nullopt;
    }
    move_record.destination_register = *destination;
    move_record.source_f128_carrier = source_f128_carrier;
    move_record.source_f128_constant_payload = source_f128_carrier->constant_payload;
  }

  const bool symbol_address_argument_materialized_at_call_site =
      bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::SymbolAddress &&
      source_home != nullptr &&
      source_home->kind != prepare::PreparedValueHomeKind::Register &&
      context.function.prepared != nullptr &&
      context.function.control_flow != nullptr &&
      context.control_flow_block != nullptr &&
      [&]() {
        const auto* addressing = prepare::find_prepared_addressing(
            *context.function.prepared, context.function.control_flow->function_name);
        if (addressing == nullptr) {
          return false;
        }
        for (const auto& materialization : addressing->address_materializations) {
          if (materialization.block_label == context.control_flow_block->block_label &&
              materialization.inst_index <= instruction_index &&
              materialization.result_value_name == source_home->value_name) {
            return true;
          }
        }
        return false;
      }();
  if (symbol_address_argument_materialized_at_call_site) {
    return std::nullopt;
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      !selected_f128_constant_argument_move &&
      ((!move_record.source_register.has_value() &&
        !move_record.source_memory.has_value()) ||
       !move_record.destination_register.has_value())) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        "AArch64 call-argument move lowering requires selected prepared register source and destination");
    return std::nullopt;
  }

  return make_call_boundary_machine_instruction(
      context,
      instruction_index,
      make_call_boundary_move_instruction(std::move(move_record)));
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_before_call_immediate_binding(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedAbiBinding& binding,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (bundle.phase != prepare::PreparedMovePhase::BeforeCall ||
      binding.destination_kind != prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      !binding.destination_abi_index.has_value()) {
    return std::nullopt;
  }

  const auto* argument = [&]() -> const prepare::PreparedCallArgumentPlan* {
    for (const auto& candidate : call_plan.arguments) {
      if (candidate.arg_index == *binding.destination_abi_index &&
          candidate.source_encoding == prepare::PreparedStorageEncodingKind::Immediate &&
          candidate.source_literal.has_value() &&
          (binding.destination_storage_kind ==
               prepare::PreparedMoveStorageKind::StackSlot ||
           candidate.destination_register_bank == prepare::PreparedRegisterBank::Gpr ||
           candidate.destination_register_bank == prepare::PreparedRegisterBank::Fpr)) {
        return &candidate;
      }
    }
    return nullptr;
  }();
  if (argument == nullptr) {
    return std::nullopt;
  }

  const auto source_immediate =
      make_scalar_call_argument_immediate(*argument->source_literal,
                                          argument->source_value_id);
  const auto expected_view =
      argument->destination_register_bank == prepare::PreparedRegisterBank::Fpr
          ? scalar_fp_register_view(argument->source_literal->type)
          : scalar_integer_register_view(argument->source_literal->type);
  if (!source_immediate.has_value() || !expected_view.has_value()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        "AArch64 immediate call-argument move requires a scalar integer or FP literal");
    return std::nullopt;
  }

  if (binding.destination_storage_kind ==
      prepare::PreparedMoveStorageKind::StackSlot) {
    if (argument->destination_register_bank == prepare::PreparedRegisterBank::Fpr) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 immediate stack call-argument move requires a scalar integer literal");
      return std::nullopt;
    }
    if (!binding.destination_stack_offset_bytes.has_value()) {
      return std::nullopt;
    }
    const auto value_type = scalar_integer_type_from_size(
        scalar_size_from_register_view(expected_view));
    if (!value_type.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 immediate stack call-argument move requires a 4 or 8 byte scalar integer literal");
      return std::nullopt;
    }
    MemoryOperand destination{
        .surface = RecordSurfaceKind::MachineInstructionNode,
        .support = MemoryOperandSupportKind::Prepared,
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_index = instruction_index,
        .stored_value_id = argument->source_value_id,
        .stored_value_name = c4c::kInvalidValueName,
        .base_kind = MemoryBaseKind::Register,
        .base_register = RegisterOperand{
            .reg = outgoing_stack_argument_base_register(),
            .role = RegisterOperandRole::Physical,
            .expected_view = abi::RegisterView::X,
        },
        .byte_offset =
            static_cast<std::int64_t>(*binding.destination_stack_offset_bytes),
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = scalar_size_from_register_view(expected_view),
        .align_bytes = scalar_size_from_register_view(expected_view),
        .can_use_base_plus_offset = true,
    };
    return make_call_boundary_machine_instruction(
        context,
        instruction_index,
        make_memory_instruction(MemoryInstructionRecord{
            .memory_kind = MemoryInstructionKind::Store,
            .address = std::move(destination),
            .value = make_immediate_operand(*source_immediate),
            .value_type = *value_type,
        }));
  }

  if (binding.destination_storage_kind !=
          prepare::PreparedMoveStorageKind::Register ||
      !binding.destination_register_name.has_value()) {
    return std::nullopt;
  }

  auto destination = make_register_operand_from_prepared_authority(
      binding.destination_register_placement.has_value()
          ? std::optional<std::string>{}
          : binding.destination_register_name,
      binding.destination_register_placement.has_value()
          ? binding.destination_register_placement
          : argument->destination_register_placement,
      argument->destination_register_bank,
      RegisterOperandRole::CallAbi,
      argument->source_value_id,
      c4c::kInvalidValueName,
      binding.destination_contiguous_width,
      binding.destination_occupied_register_names.empty()
          ? argument->destination_occupied_register_names
          : binding.destination_occupied_register_names,
      expected_view,
      diagnostics,
      context,
      instruction_index);
  if (!destination.has_value()) {
    return std::nullopt;
  }

  prepare::PreparedMoveResolution synthetic_move{
      .from_value_id = argument->source_value_id.value_or(prepare::PreparedValueId{0}),
      .to_value_id = argument->source_value_id.value_or(prepare::PreparedValueId{0}),
      .destination_kind = binding.destination_kind,
      .destination_storage_kind = binding.destination_storage_kind,
      .destination_abi_index = binding.destination_abi_index,
      .destination_register_name = binding.destination_register_name,
      .destination_contiguous_width = binding.destination_contiguous_width,
      .destination_occupied_register_names = binding.destination_occupied_register_names,
      .block_index = bundle.block_index,
      .instruction_index = bundle.instruction_index,
      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
      .reason = "call_arg_immediate_to_register",
      .destination_register_placement = binding.destination_register_placement,
  };

  CallBoundaryMoveInstructionRecord move_record{
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .phase = bundle.phase,
      .authority_kind = bundle.authority_kind,
      .block_index = bundle.block_index,
      .instruction_index = bundle.instruction_index,
      .source_parallel_copy_predecessor_label =
          bundle.source_parallel_copy_predecessor_label,
      .source_parallel_copy_successor_label =
          bundle.source_parallel_copy_successor_label,
      .move = std::move(synthetic_move),
      .source_immediate = source_immediate,
      .destination_register = *destination,
      .source_bundle = &bundle,
  };
  return make_call_boundary_machine_instruction(
      context,
      instruction_index,
      make_call_boundary_move_instruction(std::move(move_record)));
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_after_call_move(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedMoveResolution& move,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* destination_home =
      context.function.value_locations == nullptr
          ? nullptr
          : find_value_home(context, move.to_value_id);
  const auto* result_plan = call_plan.result.has_value() ? &*call_plan.result : nullptr;
  const auto* binding = find_prepared_result_binding(bundle, move);
  const auto* f128_carriers =
      context.function.prepared == nullptr
          ? nullptr
          : prepare::find_prepared_f128_carriers(
                *context.function.prepared,
                context.function.control_flow != nullptr
                    ? context.function.control_flow->function_name
                    : c4c::kInvalidFunctionName);
  const auto* destination_f128_carrier =
      f128_carriers != nullptr && destination_home != nullptr
          ? prepare::find_prepared_f128_carrier(*f128_carriers, destination_home->value_name)
          : nullptr;

  CallBoundaryMoveInstructionRecord move_record{
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .phase = bundle.phase,
      .authority_kind = bundle.authority_kind,
      .block_index = bundle.block_index,
      .instruction_index = bundle.instruction_index,
      .source_parallel_copy_predecessor_label =
          bundle.source_parallel_copy_predecessor_label,
      .source_parallel_copy_successor_label =
          bundle.source_parallel_copy_successor_label,
      .move = move,
      .source_bundle = &bundle,
      .source_move = &move,
  };

  const bool selected_gpr_result_move =
      result_plan != nullptr &&
      result_plan->source_register_bank == prepare::PreparedRegisterBank::Gpr &&
      result_plan->destination_register_bank == prepare::PreparedRegisterBank::Gpr;
  const bool selected_scalar_fpr_result_move =
      result_plan != nullptr &&
      result_plan->source_register_bank == prepare::PreparedRegisterBank::Fpr &&
      result_plan->destination_register_bank == prepare::PreparedRegisterBank::Fpr &&
      result_plan->source_contiguous_width == 1 &&
      result_plan->destination_contiguous_width == 1;
  const bool selected_f128_result_move =
      result_plan != nullptr &&
      result_plan->source_register_bank == prepare::PreparedRegisterBank::Vreg &&
      result_plan->destination_register_bank == prepare::PreparedRegisterBank::Vreg &&
      complete_full_width_f128_carrier(destination_f128_carrier);

  if (bundle.phase == prepare::PreparedMovePhase::AfterCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallResultAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      !move.destination_abi_index.has_value() &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      destination_home != nullptr &&
      destination_home->kind == prepare::PreparedValueHomeKind::Register &&
      destination_home->register_name.has_value() &&
      result_plan != nullptr &&
      result_plan->instruction_index == instruction_index &&
      result_plan->destination_value_id == std::optional<prepare::PreparedValueId>{move.to_value_id} &&
      result_plan->source_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      result_plan->destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      result_plan->source_register_name.has_value() &&
      result_plan->destination_register_name.has_value() &&
      (selected_gpr_result_move || selected_scalar_fpr_result_move ||
       selected_f128_result_move) &&
      binding != nullptr &&
      binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      binding->destination_register_name.has_value()) {
    if (*result_plan->source_register_name != *binding->destination_register_name ||
        *result_plan->source_register_name != *move.destination_register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 call-result move source register disagrees with prepared ABI binding");
      return std::nullopt;
    }
    if (*result_plan->destination_register_name != *destination_home->register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 call-result move destination register disagrees with prepared value home");
      return std::nullopt;
    }
    if (selected_f128_result_move &&
        destination_f128_carrier->register_name != destination_home->register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 binary128 call-result move destination register disagrees with prepared f128 carrier");
      return std::nullopt;
    }
    const auto expected_view =
        selected_f128_result_move ? std::optional<abi::RegisterView>{abi::RegisterView::Q}
        : selected_scalar_fpr_result_move
            ? scalar_fp_view_from_register_name(binding->destination_register_name)
        : std::nullopt;
    auto source = make_register_operand_from_prepared_authority(
        binding->destination_register_name,
        result_plan->source_register_placement.has_value()
            ? result_plan->source_register_placement
            : binding->destination_register_placement,
        result_plan->source_register_bank,
        RegisterOperandRole::CallAbi,
        move.from_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.from_value_id}
                                : std::nullopt,
        destination_home->value_name,
        result_plan->source_contiguous_width,
        result_plan->source_occupied_register_names.empty()
            ? binding->destination_occupied_register_names
            : result_plan->source_occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    auto destination = make_register_operand_from_prepared_authority(
        selected_scalar_fpr_result_move &&
                result_plan->destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : destination_home->register_name,
        result_plan->destination_register_placement,
        result_plan->destination_register_bank,
        RegisterOperandRole::CallAbi,
        destination_home->value_id,
        destination_home->value_name,
        result_plan->destination_contiguous_width,
        result_plan->destination_occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value() || !destination.has_value()) {
      return std::nullopt;
    }
    move_record.source_register = *source;
    move_record.destination_register = *destination;
    move_record.destination_f128_carrier =
        selected_f128_result_move ? destination_f128_carrier : nullptr;
  }

  if (bundle.phase == prepare::PreparedMovePhase::AfterCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallResultAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      result_plan != nullptr &&
      result_plan->destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot) {
    if (destination_home == nullptr ||
        destination_home->kind != prepare::PreparedValueHomeKind::StackSlot ||
        !destination_home->offset_bytes.has_value() ||
        result_plan->instruction_index != instruction_index ||
        result_plan->destination_value_id !=
            std::optional<prepare::PreparedValueId>{move.to_value_id} ||
        result_plan->source_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        !result_plan->source_register_name.has_value() ||
        result_plan->source_register_bank != prepare::PreparedRegisterBank::Gpr) {
      return std::nullopt;
    }
    if ((binding != nullptr &&
         binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
         binding->destination_register_name.has_value() &&
         *result_plan->source_register_name != *binding->destination_register_name) ||
        (move.destination_register_name.has_value() &&
         *result_plan->source_register_name != *move.destination_register_name)) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 stack call-result publication source register disagrees with prepared ABI binding");
      return std::nullopt;
    }
    const auto width_bytes = destination_home->size_bytes.value_or(
        scalar_size_from_register_view(
            scalar_view_from_register_name(result_plan->source_register_name)));
    const auto expected_view = scalar_integer_register_view_from_size(width_bytes);
    const auto value_type = scalar_integer_type_from_size(width_bytes);
    if (!expected_view.has_value() || !value_type.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 stack call-result publication requires a 1, 2, 4, or 8 byte scalar GPR result");
      return std::nullopt;
    }
    const auto source_register_name =
        register_name_with_expected_view(result_plan->source_register_name, expected_view);
    auto source = make_register_operand_from_prepared_authority(
        source_register_name,
        result_plan->source_register_placement,
        result_plan->source_register_bank,
        RegisterOperandRole::CallAbi,
        move.from_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.from_value_id}
                                : std::nullopt,
        destination_home->value_name,
        result_plan->source_contiguous_width,
        result_plan->source_occupied_register_names.empty()
            ? (binding != nullptr ? binding->destination_occupied_register_names
                                  : move.destination_occupied_register_names)
            : result_plan->source_occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value()) {
      return std::nullopt;
    }
    MemoryOperand destination{
        .surface = RecordSurfaceKind::MachineInstructionNode,
        .support = MemoryOperandSupportKind::Prepared,
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_index = instruction_index,
        .stored_value_id = destination_home->value_id,
        .stored_value_name = destination_home->value_name,
        .base_kind = MemoryBaseKind::FrameSlot,
        .frame_slot_id = destination_home->slot_id,
        .byte_offset = static_cast<std::int64_t>(*destination_home->offset_bytes),
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = width_bytes,
        .align_bytes = destination_home->align_bytes.value_or(width_bytes),
        .can_use_base_plus_offset = true,
    };
    return make_call_boundary_machine_instruction(
        context,
        instruction_index,
        make_memory_instruction(MemoryInstructionRecord{
            .memory_kind = MemoryInstructionKind::Store,
            .address = std::move(destination),
            .value = make_register_operand(*source),
            .value_type = *value_type,
        }));
  }

  if (bundle.phase == prepare::PreparedMovePhase::AfterCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallResultAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      destination_home != nullptr &&
      destination_home->kind == prepare::PreparedValueHomeKind::Register &&
      destination_home->register_name.has_value() &&
      binding != nullptr &&
      binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      binding->destination_register_name.has_value() &&
      binding->destination_register_placement.has_value() &&
      binding->destination_register_placement->bank == prepare::PreparedRegisterBank::Fpr &&
      (!move_record.source_register.has_value() ||
       !move_record.destination_register.has_value())) {
    const auto expected_view =
        scalar_fp_view_from_register_name(binding->destination_register_name);
    auto source = make_register_operand_from_prepared_authority(
        binding->destination_register_name,
        binding->destination_register_placement,
        prepare::PreparedRegisterBank::Fpr,
        RegisterOperandRole::CallAbi,
        move.from_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.from_value_id}
                                : std::nullopt,
        destination_home->value_name,
        1,
        binding->destination_occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    auto destination = make_register_operand_from_prepared_authority(
        destination_home->register_name,
        std::nullopt,
        prepare::PreparedRegisterBank::Fpr,
        RegisterOperandRole::CallAbi,
        destination_home->value_id,
        destination_home->value_name,
        1,
        {},
        expected_view,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value() || !destination.has_value()) {
      return std::nullopt;
    }
    move_record.source_register = *source;
    move_record.destination_register = *destination;
  }

  if (bundle.phase == prepare::PreparedMovePhase::AfterCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallResultAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      (!move_record.source_register.has_value() ||
       !move_record.destination_register.has_value())) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        "AArch64 call-result move lowering requires selected prepared register source and destination");
    return std::nullopt;
  }

  return make_call_boundary_machine_instruction(
      context,
      instruction_index,
      make_call_boundary_move_instruction(std::move(move_record)));
}


}  // namespace

[[nodiscard]] bool prepared_argument_is_small_byval_stack_lane(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t instruction_index) {
  if (context.bir_block == nullptr ||
      instruction_index >= context.bir_block->insts.size() ||
      argument.source_encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      !argument.source_value_id.has_value() ||
      !argument.destination_stack_offset_bytes.has_value()) {
    return false;
  }
  const auto* call =
      std::get_if<bir::CallInst>(&context.bir_block->insts[instruction_index]);
  if (call == nullptr || argument.arg_index >= call->arg_abi.size()) {
    return false;
  }
  const auto& abi = call->arg_abi[argument.arg_index];
  return abi.type == bir::TypeKind::Ptr &&
         abi.byval_copy &&
         !abi.sret_pointer &&
         abi.primary_class == bir::AbiValueClass::Integer &&
         abi.size_bytes > 0 &&
         abi.size_bytes <= 16;
}

std::vector<module::MachineInstruction> lower_before_call_moves(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.value_locations == nullptr) {
    return lowered;
  }
  const auto* bundle = find_move_bundle(context,
                                        prepare::PreparedMovePhase::BeforeCall,
                                        context.block_index,
                                        instruction_index);
  const prepare::PreparedMoveBundle synthetic_bundle{
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
  };
  if (bundle == nullptr) {
    bundle = &synthetic_bundle;
  }
  if (context.bir_block != nullptr && instruction_index < context.bir_block->insts.size()) {
    if (const auto* call =
            std::get_if<bir::CallInst>(&context.bir_block->insts[instruction_index]);
        call != nullptr) {
      const std::size_t outgoing_bytes =
          outgoing_stack_argument_bytes(*call, call_plan);
      if (outgoing_bytes > 0) {
        lowered.push_back(
            make_outgoing_stack_base_instruction(context, instruction_index, outgoing_bytes));
      }
    }
  }
  for (const auto& preserved : call_plan.preserved_values) {
    if (auto instruction = make_callee_saved_preservation_home_population(
            context, call_plan, *bundle, preserved, instruction_index, diagnostics)) {
      lowered.push_back(std::move(*instruction));
    }
  }
  std::vector<std::size_t> lowered_stack_byval_args;
  for (const auto& move : bundle->moves) {
    if (auto instruction =
            lower_before_call_move(context, call_plan, *bundle, move, instruction_index, diagnostics)) {
      if (move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
          move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
          move.reason == "call_arg_byval_aggregate_register_lanes" &&
          move.destination_abi_index.has_value()) {
        lowered_stack_byval_args.push_back(*move.destination_abi_index);
      }
      lowered.push_back(std::move(*instruction));
    }
  }
  for (const auto& argument : call_plan.arguments) {
    if (!prepared_argument_is_small_byval_stack_lane(context, argument, instruction_index) ||
        std::find(lowered_stack_byval_args.begin(),
                  lowered_stack_byval_args.end(),
                  argument.arg_index) != lowered_stack_byval_args.end()) {
      continue;
    }
    prepare::PreparedMoveResolution move{
        .from_value_id = *argument.source_value_id,
        .to_value_id = *argument.source_value_id,
        .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
        .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot,
        .destination_abi_index = argument.arg_index,
        .destination_stack_offset_bytes = argument.destination_stack_offset_bytes,
        .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
        .reason = "call_arg_byval_aggregate_register_lanes",
    };
    if (auto instruction =
            lower_before_call_move(context, call_plan, *bundle, move, instruction_index, diagnostics)) {
      lowered.push_back(std::move(*instruction));
    }
  }
  for (const auto& binding : bundle->abi_bindings) {
    if (auto instruction =
            lower_before_call_immediate_binding(
                context, call_plan, *bundle, binding, instruction_index, diagnostics)) {
      lowered.push_back(std::move(*instruction));
    }
  }
  return lowered;
}

std::vector<module::MachineInstruction> lower_after_call_moves(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.value_locations == nullptr) {
    return lowered;
  }
  const auto* bundle = find_move_bundle(context,
                                        prepare::PreparedMovePhase::AfterCall,
                                        context.block_index,
                                        instruction_index);
  const prepare::PreparedMoveBundle synthetic_bundle{
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .phase = prepare::PreparedMovePhase::BeforeInstruction,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
  };
  if (bundle != nullptr) {
    for (const auto& move : bundle->moves) {
      if (auto instruction =
              lower_after_call_move(context, call_plan, *bundle, move, instruction_index, diagnostics)) {
        lowered.push_back(std::move(*instruction));
      }
    }
  }

  const auto& republication_bundle =
      bundle != nullptr ? *bundle : synthetic_bundle;
  for (const auto& preserved : call_plan.preserved_values) {
    if (auto instruction = make_callee_saved_preservation_home_republication(
            context,
            call_plan,
            republication_bundle,
            preserved,
            instruction_index,
            diagnostics)) {
      lowered.push_back(std::move(*instruction));
    }
  }
  return lowered;
}

std::vector<module::MachineInstruction> lower_before_return_moves(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.value_locations == nullptr ||
      context.function.control_flow == nullptr) {
    return lowered;
  }
  const auto* bundle = find_move_bundle(context,
                                        prepare::PreparedMovePhase::BeforeReturn,
                                        context.block_index,
                                        instruction_index);
  if (bundle == nullptr) {
    return lowered;
  }

  for (const auto& move : bundle->moves) {
    if (move.destination_kind != prepare::PreparedMoveDestinationKind::FunctionReturnAbi ||
        move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        move.op_kind != prepare::PreparedMoveResolutionOpKind::Move) {
      continue;
    }
    const auto* source_home =
        find_value_home(context, move.from_value_id);
    if (source_home == nullptr || !move.destination_register_name.has_value()) {
      continue;
    }

    const auto expected_view = scalar_view_from_register_name(move.destination_register_name);
    const auto destination_bank =
        move.destination_register_placement.has_value()
            ? std::optional<prepare::PreparedRegisterBank>{
                  move.destination_register_placement->bank}
            : std::nullopt;
    auto destination = make_register_operand_from_prepared_authority(
        move.destination_register_name,
        move.destination_register_placement,
        destination_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : std::nullopt,
        source_home->value_name,
        move.destination_contiguous_width,
        move.destination_occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    if (!destination.has_value()) {
      continue;
    }

    CallBoundaryMoveInstructionRecord move_record{
        .function_name = context.function.control_flow->function_name,
        .phase = bundle->phase,
        .authority_kind = bundle->authority_kind,
        .block_index = bundle->block_index,
        .instruction_index = bundle->instruction_index,
        .source_parallel_copy_predecessor_label =
            bundle->source_parallel_copy_predecessor_label,
        .source_parallel_copy_successor_label =
            bundle->source_parallel_copy_successor_label,
        .move = move,
        .destination_register = *destination,
        .source_bundle = bundle,
        .source_move = &move,
    };
    if (source_home->kind == prepare::PreparedValueHomeKind::Register &&
        source_home->register_name.has_value()) {
      const auto source_register_name =
          register_name_with_expected_view(source_home->register_name, expected_view);
      auto source = make_register_operand_from_prepared_authority(
          source_register_name,
          std::nullopt,
          destination_bank,
          RegisterOperandRole::CallAbi,
          source_home->value_id,
          source_home->value_name,
          1,
          {},
          expected_view,
          diagnostics,
          context,
          instruction_index);
      if (!source.has_value()) {
        continue;
      }
      move_record.source_register = *source;
    } else if (source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
               source_home->offset_bytes.has_value()) {
      move_record.source_memory = MemoryOperand{
          .surface = RecordSurfaceKind::MachineInstructionNode,
          .support = MemoryOperandSupportKind::Prepared,
          .function_name = context.function.control_flow->function_name,
          .block_label = context.control_flow_block != nullptr
                             ? context.control_flow_block->block_label
                             : c4c::kInvalidBlockLabel,
          .instruction_index = instruction_index,
          .result_value_id = source_home->value_id,
          .result_value_name = source_home->value_name,
          .base_kind = MemoryBaseKind::FrameSlot,
          .frame_slot_id = source_home->slot_id,
          .byte_offset = static_cast<std::int64_t>(*source_home->offset_bytes),
          .byte_offset_is_prepared_snapshot = true,
          .size_bytes = source_home->size_bytes.value_or(
              scalar_size_from_register_view(expected_view)),
          .align_bytes = source_home->align_bytes.value_or(
              scalar_size_from_register_view(expected_view)),
          .can_use_base_plus_offset = true,
      };
    } else {
      continue;
    }
    lowered.push_back(make_call_boundary_machine_instruction(
        context,
        instruction_index,
        make_call_boundary_move_instruction(std::move(move_record))));
  }
  return lowered;
}

std::vector<module::MachineInstruction> lower_value_moves(
    const module::BlockLoweringContext& context,
    prepare::PreparedMovePhase phase,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.value_locations == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      (phase != prepare::PreparedMovePhase::BlockEntry &&
       phase != prepare::PreparedMovePhase::BeforeInstruction)) {
    return lowered;
  }
  const auto* bundle = find_move_bundle(context, phase, context.block_index, instruction_index);
  const prepare::PreparedMoveBundle synthetic_bundle{
      .function_name = context.function.control_flow->function_name,
      .phase = phase,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
  };

  if (bundle != nullptr) {
    for (const auto& move : bundle->moves) {
    if (move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
        move.op_kind != prepare::PreparedMoveResolutionOpKind::Move) {
      continue;
    }
    const auto* destination_home =
        find_value_home(context, move.to_value_id);
    if (destination_home == nullptr) {
      continue;
    }
    const auto* source_home =
        find_value_home(context, move.from_value_id);
    if (move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
        destination_home->kind == prepare::PreparedValueHomeKind::StackSlot) {
      if (auto stack_move = make_value_stack_move_instruction(
              context,
              *bundle,
              move,
              *destination_home,
              source_home,
              instruction_index)) {
        lowered.push_back(std::move(*stack_move));
      }
      continue;
    }
    if (move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        destination_home->kind != prepare::PreparedValueHomeKind::Register) {
      continue;
    }
    const auto destination_register_view =
        scalar_view_from_register_name(destination_home->register_name);
    const auto expected_view =
        destination_register_view.has_value()
            ? destination_register_view
            : (destination_home->size_bytes.has_value()
                   ? scalar_integer_register_view_from_size(*destination_home->size_bytes)
                   : std::nullopt);
    const auto destination_bank =
        expected_view == std::optional<abi::RegisterView>{abi::RegisterView::S} ||
                expected_view == std::optional<abi::RegisterView>{abi::RegisterView::D}
            ? prepare::PreparedRegisterBank::Fpr
            : prepare::PreparedRegisterBank::Gpr;
    auto destination = make_register_operand_from_prepared_authority(
        destination_home->register_name,
        move.destination_register_placement,
        destination_bank,
        RegisterOperandRole::StoragePlan,
        destination_home->value_id,
        destination_home->value_name,
        move.destination_contiguous_width,
        move.destination_occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    if (!destination.has_value()) {
      continue;
    }

    CallBoundaryMoveInstructionRecord move_record{
        .function_name = context.function.control_flow->function_name,
        .phase = phase,
        .authority_kind = bundle->authority_kind,
        .block_index = bundle->block_index,
        .instruction_index = bundle->instruction_index,
        .source_parallel_copy_predecessor_label =
            bundle->source_parallel_copy_predecessor_label,
        .source_parallel_copy_successor_label =
            bundle->source_parallel_copy_successor_label,
        .move = move,
        .destination_register = *destination,
        .source_bundle = bundle,
        .source_move = &move,
    };
    if (move.source_immediate_i32.has_value()) {
      const auto immediate = make_scalar_call_argument_immediate(
          bir::Value::immediate_i32(static_cast<std::int32_t>(*move.source_immediate_i32)),
          move.from_value_id);
      if (!immediate.has_value()) {
        continue;
      }
      move_record.source_immediate = immediate;
    } else if (const auto* prior_stack_preserved =
                   phase == prepare::PreparedMovePhase::BeforeInstruction
                       ? find_prior_stack_preserved_value_before_instruction(
                             context, move.from_value_id, instruction_index)
                       : nullptr;
               prior_stack_preserved != nullptr) {
      move_record.source_memory = MemoryOperand{
          .surface = RecordSurfaceKind::MachineInstructionNode,
          .support = MemoryOperandSupportKind::Prepared,
          .function_name = context.function.control_flow->function_name,
          .block_label = context.control_flow_block->block_label,
          .instruction_index = instruction_index,
          .result_value_id = prior_stack_preserved->value_id,
          .result_value_name = prior_stack_preserved->value_name,
          .base_kind = MemoryBaseKind::FrameSlot,
          .frame_slot_id = prior_stack_preserved->slot_id,
          .byte_offset =
              static_cast<std::int64_t>(*prior_stack_preserved->stack_offset_bytes),
          .byte_offset_is_prepared_snapshot = true,
          .size_bytes = *prior_stack_preserved->stack_size_bytes,
          .align_bytes = prior_stack_preserved->stack_align_bytes.value_or(
              *prior_stack_preserved->stack_size_bytes),
          .can_use_base_plus_offset = true,
      };
    } else if (source_home != nullptr &&
               source_home->kind == prepare::PreparedValueHomeKind::Register &&
               source_home->register_name.has_value()) {
      auto source = make_register_operand_from_prepared_authority(
          source_home->register_name,
          std::nullopt,
          destination_bank,
          RegisterOperandRole::StoragePlan,
          source_home->value_id,
          source_home->value_name,
          1,
          {},
          expected_view,
          diagnostics,
          context,
          instruction_index);
      if (!source.has_value()) {
        continue;
      }
      move_record.source_register = *source;
    } else if (source_home != nullptr &&
               source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
               source_home->offset_bytes.has_value()) {
      move_record.source_memory = MemoryOperand{
          .surface = RecordSurfaceKind::MachineInstructionNode,
          .support = MemoryOperandSupportKind::Prepared,
          .function_name = context.function.control_flow->function_name,
          .block_label = context.control_flow_block->block_label,
          .instruction_index = instruction_index,
          .result_value_id = source_home->value_id,
          .result_value_name = source_home->value_name,
          .base_kind = MemoryBaseKind::FrameSlot,
          .frame_slot_id = source_home->slot_id,
          .byte_offset = static_cast<std::int64_t>(*source_home->offset_bytes),
          .byte_offset_is_prepared_snapshot = true,
          .size_bytes = source_home->size_bytes.value_or(4),
          .align_bytes = source_home->align_bytes.value_or(4),
          .can_use_base_plus_offset = true,
      };
    } else {
      continue;
    }

    lowered.push_back(make_call_boundary_machine_instruction(
        context,
        instruction_index,
        make_call_boundary_move_instruction(std::move(move_record))));
    }
  }

  if (phase != prepare::PreparedMovePhase::BlockEntry) {
    return lowered;
  }

  const auto* call_plans =
      context.function.call_plans != nullptr
          ? context.function.call_plans
          : (context.function.prepared != nullptr && context.function.control_flow != nullptr
                 ? prepare::find_prepared_call_plans(
                       *context.function.prepared, context.function.control_flow->function_name)
                 : nullptr);
  if (call_plans == nullptr) {
    return lowered;
  }

  std::vector<const prepare::PreparedCallPreservedValue*> selected_preserved;
  for (const auto& call : call_plans->calls) {
    if (call.block_index >= context.block_index) {
      continue;
    }
    for (const auto& preserved : call.preserved_values) {
      if (preserved.route !=
          prepare::PreparedCallPreservationRoute::CalleeSavedRegister) {
        continue;
      }
      auto existing = std::find_if(
          selected_preserved.begin(),
          selected_preserved.end(),
          [&](const prepare::PreparedCallPreservedValue* candidate) {
            return candidate->value_id == preserved.value_id;
          });
      if (existing == selected_preserved.end()) {
        selected_preserved.push_back(&preserved);
      } else {
        *existing = &preserved;
      }
    }
  }

  const auto& republication_bundle = bundle != nullptr ? *bundle : synthetic_bundle;
  for (const auto* preserved : selected_preserved) {
    if (preserved == nullptr ||
        !preserved_value_has_block_entry_non_call_use(context, *preserved)) {
      continue;
    }
    if (auto instruction =
            make_callee_saved_preservation_home_republication_instruction(
                context,
                republication_bundle,
                *preserved,
                prepare::PreparedMovePhase::BlockEntry,
                context.block_index,
                instruction_index,
                "callee_saved_preservation_home_block_entry_republication",
                diagnostics)) {
      lowered.push_back(std::move(*instruction));
    }
  }
  return lowered;
}
}  // namespace c4c::backend::aarch64::codegen
