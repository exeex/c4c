#include "calls.hpp"
#include "dispatch.hpp"
#include "dispatch_lookup.hpp"
#include "memory.hpp"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;
namespace abi = c4c::backend::aarch64::abi;

namespace {

[[nodiscard]] const prepare::PreparedFrameSlot* find_frame_slot_by_id(
    const prepare::PreparedStackLayout& stack_layout,
    prepare::PreparedFrameSlotId slot_id) {
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.slot_id == slot_id) {
      return &slot;
    }
  }
  return nullptr;
}

[[nodiscard]] bool byval_same_gp_register_index(abi::RegisterReference lhs,
                                                abi::RegisterReference rhs) {
  return lhs.index == rhs.index && abi::is_gp_register(lhs) && abi::is_gp_register(rhs);
}

[[nodiscard]] bool aggregate_frame_slot_direct_offset_is_encodable(
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

}  // namespace

[[nodiscard]] std::string_view aggregate_stack_copy_load_mnemonic(
    std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return "ldrb";
    case 4:
    case 8:
      return "ldr";
  }
  return {};
}

[[nodiscard]] std::string_view aggregate_stack_copy_store_mnemonic(
    std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return "strb";
    case 4:
    case 8:
      return "str";
  }
  return {};
}

[[nodiscard]] std::optional<abi::RegisterReference> aggregate_stack_copy_scratch(
    std::size_t width_bytes) {
  const auto scratch = abi::reserved_mir_scratch_gp_registers().front();
  if (width_bytes == 1 || width_bytes == 4) {
    return abi::w_register(scratch.index);
  }
  if (width_bytes == 8) {
    return abi::x_register(scratch.index);
  }
  return std::nullopt;
}

[[nodiscard]] std::vector<std::size_t> aggregate_stack_copy_chunks(
    std::size_t size_bytes) {
  std::vector<std::size_t> chunks;
  std::size_t remaining = size_bytes;
  while (remaining >= 8) {
    chunks.push_back(8);
    remaining -= 8;
  }
  if (remaining >= 4) {
    chunks.push_back(4);
    remaining -= 4;
  }
  while (remaining > 0) {
    chunks.push_back(1);
    --remaining;
  }
  return chunks;
}

[[nodiscard]] std::string_view aggregate_register_lane_load_mnemonic(
    std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return "ldrb";
    case 2:
      return "ldrh";
    case 4:
    case 8:
      return "ldr";
  }
  return {};
}

[[nodiscard]] abi::RegisterReference aggregate_register_lane_load_register(
    abi::RegisterReference reg,
    std::size_t width_bytes) {
  return width_bytes == 8 ? abi::x_register(reg.index) : abi::w_register(reg.index);
}

[[nodiscard]] std::optional<abi::RegisterReference> aggregate_register_lane_scratch(
    const RegisterOperand& destination) {
  for (const auto scratch : abi::reserved_mir_scratch_gp_registers()) {
    if (byval_same_gp_register_index(scratch, destination.reg)) {
      continue;
    }
    bool aliases_occupied = false;
    for (const auto occupied : destination.occupied_register_references) {
      if (byval_same_gp_register_index(scratch, occupied)) {
        aliases_occupied = true;
        break;
      }
    }
    if (!aliases_occupied) {
      return abi::x_register(scratch.index);
    }
  }
  return std::nullopt;
}

[[nodiscard]] MemoryOperand aggregate_register_lane_memory(
    MemoryOperand memory,
    std::size_t byte_offset,
    std::size_t width_bytes) {
  memory.byte_offset += static_cast<std::int64_t>(byte_offset);
  memory.size_bytes = width_bytes;
  memory.align_bytes = std::min<std::size_t>(memory.align_bytes, width_bytes);
  return memory;
}

[[nodiscard]] bool aggregate_register_lane_memory_is_printable(
    const MemoryOperand& memory,
    std::size_t width_bytes) {
  if (memory.base_kind == MemoryBaseKind::PointerValue && memory.base_register.has_value()) {
    return !memory_address(memory).empty();
  }
  if (memory.base_kind == MemoryBaseKind::Register && memory.base_register.has_value()) {
    return !memory_address(memory).empty();
  }
  return memory.base_kind == MemoryBaseKind::FrameSlot &&
         aggregate_frame_slot_direct_offset_is_encodable(memory, width_bytes) &&
         !memory_address(memory).empty();
}

[[nodiscard]] std::optional<std::size_t> aggregate_register_lane_printable_chunk(
    const MemoryOperand& memory,
    std::size_t source_offset,
    std::size_t remaining) {
  static constexpr std::size_t kCandidateChunks[] = {8, 4, 2, 1};
  for (const std::size_t chunk_width : kCandidateChunks) {
    if (chunk_width > remaining) {
      continue;
    }
    const auto chunk_memory =
        aggregate_register_lane_memory(memory, source_offset, chunk_width);
    if (aggregate_register_lane_memory_is_printable(chunk_memory, chunk_width) &&
        !aggregate_register_lane_load_mnemonic(chunk_width).empty()) {
      return chunk_width;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<abi::RegisterReference> aggregate_register_lane_destination(
    const RegisterOperand& destination,
    std::size_t lane_index) {
  if (lane_index < destination.occupied_register_references.size()) {
    return abi::x_register(destination.occupied_register_references[lane_index].index);
  }
  if (destination.reg.index + lane_index > 30) {
    return std::nullopt;
  }
  return abi::x_register(static_cast<std::uint8_t>(destination.reg.index + lane_index));
}

[[nodiscard]] bool is_aggregate_register_lane_publication(
    const CallBoundaryMoveInstructionRecord& move) {
  return move.phase == prepare::PreparedMovePhase::BeforeCall &&
           move.move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
           move.move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
           move.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
         move.move.reason == "call_arg_byval_aggregate_register_lanes" &&
           move.source_memory.has_value() &&
         !move.source_memory_materializes_address &&
         move.source_memory->support == MemoryOperandSupportKind::Prepared &&
         move.source_memory->size_bytes > 0 &&
           move.source_memory->size_bytes <= 16 &&
           move.destination_register.has_value() &&
           move.destination_register->prepared_bank == prepare::PreparedRegisterBank::Gpr &&
         move.destination_register->expected_view == abi::RegisterView::X &&
           abi::is_gp_register(move.destination_register->reg);
}

[[nodiscard]] bool is_aarch64_byval_register_lane_move(
    const prepare::PreparedMoveResolution& move) {
  return move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
         (move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register ||
          move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot) &&
         move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
         move.reason == "call_arg_byval_aggregate_register_lanes";
}

namespace {

[[nodiscard]] std::optional<std::size_t> aggregate_slot_source_byte_offset(
    std::string_view slot_name,
    std::string_view source_name) {
  if (source_name.empty() || slot_name.size() <= source_name.size() + 1 ||
      slot_name.compare(0, source_name.size(), source_name) != 0 ||
      slot_name[source_name.size()] != '.') {
    return std::nullopt;
  }
  const auto suffix = slot_name.substr(source_name.size() + 1);
  std::size_t byte_offset = 0;
  const auto* begin = suffix.data();
  const auto* end = begin + suffix.size();
  const auto [ptr, ec] = std::from_chars(begin, end, byte_offset);
  if (ec != std::errc{} || ptr != end) {
    return std::nullopt;
  }
  return byte_offset;
}

void append_aggregate_lane_source_name_variants(std::vector<std::string>* names,
                                                std::string_view name) {
  if (name.empty()) {
    return;
  }
  auto append_unique = [&](std::string candidate) {
    if (std::find(names->begin(), names->end(), candidate) == names->end()) {
      names->push_back(std::move(candidate));
    }
  };
  append_unique(std::string{name});
  if (name.front() == '%') {
    append_unique(std::string{name.substr(1)});
  } else {
    append_unique("%" + std::string{name});
  }
}

struct AggregateRegisterLaneLoadSource {
  std::string value_name;
  std::string slot_name;
};

struct AggregateRegisterLanePartialStore {
  std::string slot_name;
  std::size_t byte_offset = 0;
  AggregateRegisterLaneStore store;
};

[[nodiscard]] std::optional<std::string_view> find_aggregate_lane_load_source_slot(
    const std::vector<AggregateRegisterLaneLoadSource>& load_sources,
    std::string_view value_name) {
  for (auto it = load_sources.rbegin(); it != load_sources.rend(); ++it) {
    if (it->value_name == value_name) {
      return std::string_view{it->slot_name};
    }
  }
  return std::nullopt;
}

[[nodiscard]] bool append_aggregate_lane_partial_source_stores(
    std::string_view slot_name,
    std::size_t source_offset,
    std::size_t source_size_bytes,
    std::optional<std::size_t> source_home_offset,
    const std::vector<AggregateRegisterLanePartialStore>& partial_stores,
    std::vector<AggregateRegisterLaneStore>* stores) {
  bool appended = false;
  for (const auto& partial : partial_stores) {
    if (partial.slot_name != slot_name || partial.byte_offset >= source_size_bytes) {
      continue;
    }
    auto adjusted = partial.store;
    adjusted.source_offset = source_offset + partial.byte_offset;
    adjusted.size_bytes = std::min(adjusted.size_bytes,
                                   source_size_bytes - partial.byte_offset);
    if (!source_home_offset.has_value() || adjusted.stack_offset < 0 ||
        static_cast<std::size_t>(adjusted.stack_offset) < *source_home_offset) {
      continue;
    }
    stores->push_back(std::move(adjusted));
    appended = true;
  }
  return appended;
}

}  // namespace

[[nodiscard]] std::vector<AggregateRegisterLaneStore>
collect_byval_register_lane_stores(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& source_home,
    std::size_t instruction_index) {
  std::vector<AggregateRegisterLaneStore> stores;
  std::vector<AggregateRegisterLaneLoadSource> load_sources;
  std::vector<AggregateRegisterLanePartialStore> partial_stores;
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      context.bir_block == nullptr ||
      source_home.value_name == c4c::kInvalidValueName) {
    return stores;
  }
  std::vector<std::string> source_names;
  const auto source_name =
      prepare::prepared_value_name(context.function.prepared->names,
                                   source_home.value_name);
  append_aggregate_lane_source_name_variants(&source_names, source_name);
  if (instruction_index < context.bir_block->insts.size()) {
    if (const auto* call =
            std::get_if<bir::CallInst>(&context.bir_block->insts[instruction_index]);
        call != nullptr && call->args.size() == 1 &&
        call->args.front().kind == bir::Value::Kind::Named) {
      append_aggregate_lane_source_name_variants(&source_names, call->args.front().name);
    }
  }
  if (source_names.empty()) {
    return stores;
  }
  const auto* addressing = prepare::find_prepared_addressing(
      *context.function.prepared, context.function.control_flow->function_name);
  if (addressing == nullptr) {
    return stores;
  }

  for (std::size_t index = 0;
       index < instruction_index && index < context.bir_block->insts.size();
       ++index) {
    if (const auto* load = std::get_if<bir::LoadLocalInst>(
            &context.bir_block->insts[index]);
        load != nullptr && load->result.kind == bir::Value::Kind::Named &&
        !load->slot_name.empty()) {
      load_sources.push_back(AggregateRegisterLaneLoadSource{
          .value_name = load->result.name,
          .slot_name = load->slot_name,
      });
      continue;
    }

    const auto* store = std::get_if<bir::StoreLocalInst>(
        &context.bir_block->insts[index]);
    if (store == nullptr) {
      continue;
    }

    std::string_view store_slot_name = store->slot_name;
    if (store_slot_name.empty() && store->address.has_value() &&
        store->address->base_kind == bir::MemoryAddress::BaseKind::LocalSlot &&
        !store->address->base_name.empty()) {
      store_slot_name = store->address->base_name;
    }
    std::optional<std::size_t> direct_source_offset;
    for (const auto& candidate_source_name : source_names) {
      direct_source_offset =
          aggregate_slot_source_byte_offset(store_slot_name, candidate_source_name);
      if (direct_source_offset.has_value()) {
        break;
      }
    }
    if (direct_source_offset.has_value()) {
      const prepare::PreparedFrameSlot* selected_slot = nullptr;
      for (const auto& object : context.function.prepared->stack_layout.objects) {
        if (object.function_name != context.function.control_flow->function_name ||
            object.source_kind != "local_slot" ||
            prepare::prepared_stack_object_name(
                context.function.prepared->names, object) != store_slot_name) {
          continue;
        }
        for (const auto& slot : context.function.prepared->stack_layout.frame_slots) {
          if (slot.object_id == object.object_id &&
              (selected_slot == nullptr ||
               slot.offset_bytes < selected_slot->offset_bytes)) {
            selected_slot = &slot;
          }
        }
      }
      if (selected_slot != nullptr) {
        stores.push_back(AggregateRegisterLaneStore{
            .source_offset = *direct_source_offset,
            .stack_offset = static_cast<std::int64_t>(selected_slot->offset_bytes),
            .size_bytes = store->address.has_value() && store->address->size_bytes > 0
                              ? store->address->size_bytes
                              : selected_slot->size_bytes,
            .align_bytes = store->address.has_value() && store->address->align_bytes > 0
                               ? store->address->align_bytes
                               : selected_slot->align_bytes,
            .frame_slot_id = selected_slot->slot_id,
        });
        continue;
      }
    }

    const auto* access =
        prepare::find_prepared_memory_access(*addressing,
                                             context.control_flow_block->block_label,
                                             index);
    if (access == nullptr ||
        access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
        !access->address.frame_slot_id.has_value() ||
        access->address.size_bytes == 0) {
      continue;
    }
    if (store->value.kind == bir::Value::Kind::Named) {
      const auto stored_value_name =
          context.function.prepared->names.value_names.find(store->value.name);
      if (stored_value_name != c4c::kInvalidValueName &&
          access->stored_value_name != std::optional<c4c::ValueNameId>{stored_value_name}) {
        continue;
      }
    }
    const auto* slot =
        find_frame_slot_by_id(context.function.prepared->stack_layout,
                              *access->address.frame_slot_id);
    if (slot == nullptr) {
      continue;
    }
    AggregateRegisterLaneStore lane_store{
        .stack_offset =
            static_cast<std::int64_t>(slot->offset_bytes) +
            access->address.byte_offset,
        .size_bytes = access->address.size_bytes,
        .align_bytes = access->address.align_bytes == 0
                           ? std::size_t{1}
                           : access->address.align_bytes,
        .frame_slot_id = access->address.frame_slot_id,
    };

    if (store->address.has_value() &&
        store->address->base_kind == bir::MemoryAddress::BaseKind::LocalSlot &&
        store->address->byte_offset >= 0 && !store->address->base_name.empty()) {
      auto partial_store = lane_store;
      if (context.function.value_locations != nullptr &&
          source_home.offset_bytes.has_value() &&
          store->value.kind == bir::Value::Kind::Named) {
        const auto stored_value_name =
            context.function.prepared->names.value_names.find(store->value.name);
        const auto* stored_home =
            stored_value_name == c4c::kInvalidValueName
                ? nullptr
                : find_value_home(context, stored_value_name);
        if (stored_home != nullptr &&
            stored_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
            stored_home->offset_bytes.has_value() && stored_home->slot_id.has_value() &&
            *stored_home->offset_bytes >= *source_home.offset_bytes) {
          partial_store.stack_offset = static_cast<std::int64_t>(*stored_home->offset_bytes);
          partial_store.size_bytes = stored_home->size_bytes.value_or(access->address.size_bytes);
          partial_store.align_bytes =
              stored_home->align_bytes.value_or(access->address.align_bytes == 0
                                                    ? std::size_t{1}
                                                    : access->address.align_bytes);
          partial_store.frame_slot_id = stored_home->slot_id;
        }
      }
      partial_stores.push_back(AggregateRegisterLanePartialStore{
          .slot_name = store->address->base_name,
          .byte_offset = static_cast<std::size_t>(store->address->byte_offset),
          .store = partial_store,
      });
    }

    std::optional<std::size_t> source_offset;
    for (const auto& candidate_source_name : source_names) {
      source_offset =
          aggregate_slot_source_byte_offset(store_slot_name, candidate_source_name);
      if (source_offset.has_value()) {
        break;
      }
    }
    if (!source_offset.has_value()) {
      continue;
    }
    lane_store.source_offset = *source_offset;

    if (store->value.kind == bir::Value::Kind::Named) {
      if (const auto load_source_slot =
              find_aggregate_lane_load_source_slot(load_sources, store->value.name);
          load_source_slot.has_value() &&
          append_aggregate_lane_partial_source_stores(*load_source_slot,
                                                      *source_offset,
                                                      access->address.size_bytes,
                                                      source_home.offset_bytes,
                                                      partial_stores,
                                                      &stores)) {
        continue;
      }
    }
    stores.push_back(std::move(lane_store));
  }
  std::sort(stores.begin(), stores.end(),
            [](const AggregateRegisterLaneStore& lhs,
               const AggregateRegisterLaneStore& rhs) {
              return lhs.source_offset < rhs.source_offset;
            });
  return stores;
}

[[nodiscard]] std::optional<MemoryOperand>
make_byval_register_lane_prepared_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    std::size_t size_bytes,
    std::size_t instruction_index) {
  if (size_bytes == 0) {
    return std::nullopt;
  }
  if (argument.source_selection.has_value() &&
      argument.source_selection->kind ==
          prepare::PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane &&
      argument.source_selection->byval_lane_extent_bytes ==
          std::optional<std::size_t>{size_bytes} &&
      argument.source_selection->source_slot_id.has_value() &&
      argument.source_selection->source_stack_offset_bytes.has_value() &&
      argument.source_selection->source_size_bytes.has_value() &&
      argument.source_selection->source_align_bytes.has_value() &&
      *argument.source_selection->source_size_bytes >= size_bytes) {
    return MemoryOperand{
        .surface = RecordSurfaceKind::MachineInstructionNode,
        .support = MemoryOperandSupportKind::Prepared,
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_index = instruction_index,
        .result_value_id = argument.source_value_id.has_value()
                               ? argument.source_value_id
                               : std::optional<prepare::PreparedValueId>{
                                     source_home.value_id},
        .result_value_name = std::nullopt,
        .base_kind = MemoryBaseKind::FrameSlot,
        .frame_slot_id = argument.source_selection->source_slot_id,
        .byte_offset = static_cast<std::int64_t>(
            *argument.source_selection->source_stack_offset_bytes),
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = size_bytes,
        .align_bytes = *argument.source_selection->source_align_bytes,
        .can_use_base_plus_offset = true,
    };
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<MemoryOperand> aggregate_lane_store_memory(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    const AggregateRegisterLaneStore& store,
    std::size_t byte_delta,
    std::size_t width_bytes,
    std::size_t instruction_index) {
  if (!store.frame_slot_id.has_value() ||
      store.stack_offset < 0 ||
      width_bytes == 0 ||
      byte_delta >
          static_cast<std::size_t>(
              std::numeric_limits<std::int64_t>::max() - store.stack_offset)) {
    return std::nullopt;
  }
  return MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .result_value_id = argument.source_value_id.has_value()
                             ? argument.source_value_id
                             : std::optional<prepare::PreparedValueId>{
                                   source_home.value_id},
      .result_value_name = source_home.value_name,
      .base_kind = MemoryBaseKind::FrameSlot,
      .frame_slot_id = store.frame_slot_id,
      .byte_offset = store.stack_offset + static_cast<std::int64_t>(byte_delta),
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = width_bytes,
      .align_bytes = std::min<std::size_t>(store.align_bytes, width_bytes),
      .can_use_base_plus_offset = true,
  };
}

}  // namespace c4c::backend::aarch64::codegen
