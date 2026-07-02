#include "bir.hpp"

#include <algorithm>
#include <type_traits>

namespace c4c::backend::bir {

Route3MemoryAccessNodeKind route3_memory_access_node_kind(const Inst& inst) {
  return std::visit(
      [](const auto& candidate) -> Route3MemoryAccessNodeKind {
        using T = std::decay_t<decltype(candidate)>;
        if constexpr (std::is_same_v<T, LoadLocalInst>) {
          return Route3MemoryAccessNodeKind::LoadLocal;
        } else if constexpr (std::is_same_v<T, LoadGlobalInst>) {
          return Route3MemoryAccessNodeKind::LoadGlobal;
        } else if constexpr (std::is_same_v<T, StoreLocalInst>) {
          return Route3MemoryAccessNodeKind::StoreLocal;
        } else if constexpr (std::is_same_v<T, StoreGlobalInst>) {
          return Route3MemoryAccessNodeKind::StoreGlobal;
        } else {
          return Route3MemoryAccessNodeKind::Unknown;
        }
      },
      inst);
}

Route3MemoryAccessBaseKind route3_memory_access_base_kind(
    const MemoryAddress& address) {
  switch (address.base_kind) {
    case MemoryAddress::BaseKind::None:
      return Route3MemoryAccessBaseKind::None;
    case MemoryAddress::BaseKind::LocalSlot:
      return Route3MemoryAccessBaseKind::LocalSlot;
    case MemoryAddress::BaseKind::GlobalSymbol:
      return Route3MemoryAccessBaseKind::GlobalSymbol;
    case MemoryAddress::BaseKind::PointerValue:
      return Route3MemoryAccessBaseKind::PointerValue;
    case MemoryAddress::BaseKind::StringConstant:
      return Route3MemoryAccessBaseKind::StringConstant;
    case MemoryAddress::BaseKind::Label:
      return Route3MemoryAccessBaseKind::None;
  }
  return Route3MemoryAccessBaseKind::None;
}

namespace {

[[nodiscard]] const Value* route3_result_value(const Inst& inst) {
  return std::visit(
      [](const auto& candidate) -> const Value* {
        using T = std::decay_t<decltype(candidate)>;
        if constexpr (std::is_same_v<T, LoadLocalInst> ||
                      std::is_same_v<T, LoadGlobalInst>) {
          return &candidate.result;
        } else {
          return nullptr;
        }
      },
      inst);
}

[[nodiscard]] const Value* route3_stored_value(const Inst& inst) {
  return std::visit(
      [](const auto& candidate) -> const Value* {
        using T = std::decay_t<decltype(candidate)>;
        if constexpr (std::is_same_v<T, StoreLocalInst> ||
                      std::is_same_v<T, StoreGlobalInst>) {
          return &candidate.value;
        } else {
          return nullptr;
        }
      },
      inst);
}

[[nodiscard]] const MemoryAddress* route3_memory_address(const Inst& inst) {
  return std::visit(
      [](const auto& candidate) -> const MemoryAddress* {
        using T = std::decay_t<decltype(candidate)>;
        if constexpr (std::is_same_v<T, LoadLocalInst> ||
                      std::is_same_v<T, LoadGlobalInst> ||
                      std::is_same_v<T, StoreLocalInst> ||
                      std::is_same_v<T, StoreGlobalInst>) {
          return candidate.address.has_value() ? &*candidate.address : nullptr;
        } else {
          return nullptr;
        }
      },
      inst);
}

[[nodiscard]] bool route3_same_local_slot(
    const Route3MemoryAccessRecord& lhs,
    const Route3MemoryAccessRecord& rhs) {
  if (!lhs ||
      !rhs ||
      lhs.base_kind != Route3MemoryAccessBaseKind::LocalSlot ||
      rhs.base_kind != Route3MemoryAccessBaseKind::LocalSlot) {
    return false;
  }
  if (lhs.local_slot_id != kInvalidSlotName &&
      rhs.local_slot_id != kInvalidSlotName) {
    return lhs.local_slot_id == rhs.local_slot_id;
  }
  return !lhs.local_slot_name.empty() &&
         lhs.local_slot_name == rhs.local_slot_name;
}

[[nodiscard]] bool route3_same_local_range(
    const Route3MemoryAccessRecord& lhs,
    const Route3MemoryAccessRecord& rhs) {
  return route3_same_local_slot(lhs, rhs) &&
         lhs.byte_offset == rhs.byte_offset &&
         lhs.size_bytes != 0 &&
         lhs.size_bytes == rhs.size_bytes;
}

[[nodiscard]] bool route3_local_ranges_overlap(
    const Route3MemoryAccessRecord& lhs,
    const Route3MemoryAccessRecord& rhs) {
  if (!route3_same_local_slot(lhs, rhs) ||
      lhs.size_bytes == 0 ||
      rhs.size_bytes == 0) {
    return false;
  }
  const auto lhs_begin = lhs.byte_offset;
  const auto rhs_begin = rhs.byte_offset;
  const auto lhs_end = lhs_begin + static_cast<std::int64_t>(lhs.size_bytes);
  const auto rhs_end = rhs_begin + static_cast<std::int64_t>(rhs.size_bytes);
  return lhs_begin < rhs_end && rhs_begin < lhs_end;
}

}  // namespace

Route3MemoryAccessRecord route3_memory_access_record(
    const Block& block,
    std::size_t instruction_index) {
  if (instruction_index >= block.insts.size()) {
    return {};
  }
  const auto& inst = block.insts[instruction_index];
  const auto node_kind = route3_memory_access_node_kind(inst);
  if (node_kind == Route3MemoryAccessNodeKind::Unknown) {
    return {};
  }
  const auto* address = route3_memory_address(inst);
  if (address == nullptr) {
    return {};
  }
  const auto base_kind = route3_memory_access_base_kind(*address);
  if (address->base_kind == MemoryAddress::BaseKind::Label) {
    return {};
  }
  Route3MemoryAccessRecord record{
      .available = true,
      .instruction = &inst,
      .instruction_index = instruction_index,
      .node_kind = node_kind,
      .block_label = block.label,
      .block_label_id = block.label_id,
      .address_space = address->address_space,
      .is_volatile = address->is_volatile,
      .base_kind = base_kind,
      .byte_offset = address->byte_offset,
      .size_bytes = address->size_bytes,
      .align_bytes = address->align_bytes,
      .provenance = address->provenance,
  };
  if (const auto* result = route3_result_value(inst); result != nullptr) {
    record.result_value = route1_source_value_identity(*result);
  }
  if (const auto* stored = route3_stored_value(inst); stored != nullptr) {
    record.stored_value = route1_source_value_identity(*stored);
  }
  switch (base_kind) {
    case Route3MemoryAccessBaseKind::LocalSlot:
      record.local_slot_name = address->base_name;
      record.local_slot_id = address->base_slot_id;
      break;
    case Route3MemoryAccessBaseKind::GlobalSymbol:
      record.global_name = address->base_name;
      record.global_name_id = address->base_link_name_id;
      break;
    case Route3MemoryAccessBaseKind::PointerValue:
      record.pointer_value = route1_source_value_identity(address->base_value);
      break;
    case Route3MemoryAccessBaseKind::StringConstant:
      record.string_constant_name = address->base_name;
      record.string_constant_name_id = address->base_link_name_id;
      break;
    case Route3MemoryAccessBaseKind::None:
      break;
  }
  return record;
}

Route3MemoryAccessValueRecord route3_memory_access_result_value_record(
    const Block& block,
    std::size_t instruction_index) {
  const auto access = route3_memory_access_record(block, instruction_index);
  if (!access || !access.result_value) {
    return {};
  }
  return Route3MemoryAccessValueRecord{
      .available = true,
      .role = Route3MemoryAccessValueRole::Result,
      .value = access.result_value,
      .access_instruction_index = access.instruction_index,
      .node_kind = access.node_kind,
      .access = access,
  };
}

Route3MemoryAccessValueRecord route3_memory_access_stored_value_record(
    const Block& block,
    std::size_t instruction_index) {
  const auto access = route3_memory_access_record(block, instruction_index);
  if (!access || !access.stored_value) {
    return {};
  }
  return Route3MemoryAccessValueRecord{
      .available = true,
      .role = Route3MemoryAccessValueRole::Stored,
      .value = access.stored_value,
      .access_instruction_index = access.instruction_index,
      .node_kind = access.node_kind,
      .access = access,
  };
}

Route3MemoryAccessIndex route3_build_memory_access_index(const Block& block) {
  Route3MemoryAccessIndex index{
      .block = &block,
  };
  index.records.reserve(block.insts.size());
  for (std::size_t instruction_index = 0; instruction_index < block.insts.size();
       ++instruction_index) {
    const auto record = route3_memory_access_record(block, instruction_index);
    if (record) {
      index.records.push_back(record);
    }
  }
  return index;
}

const Route3MemoryAccessRecord* route3_find_memory_access_record(
    const Route3MemoryAccessIndex& index,
    std::size_t instruction_index,
    Route3MemoryAccessNodeKind node_kind) {
  if (!index) {
    return nullptr;
  }
  for (const auto& record : index.records) {
    if (!record ||
        record.instruction_index != instruction_index ||
        record.node_kind != node_kind) {
      continue;
    }
    return &record;
  }
  return nullptr;
}

Route3SameBlockGlobalLoadAccessRecord
route3_same_block_global_load_access_record(
    Route1SameBlockProducerQuery query,
    const Value& value) {
  Route3SameBlockGlobalLoadAccessRecord record{
      .available = value.kind == Value::Kind::Named && !value.name.empty(),
      .root_value = route1_source_value_identity(value),
  };
  if (!query || !record.available || query.index->block == nullptr) {
    return {};
  }
  const auto producer = route1_find_same_block_scalar_producer(query, value);
  if (!producer.has_value() ||
      producer->record == nullptr ||
      producer->record->kind != Route1ProducerKind::LoadGlobal) {
    return record;
  }
  record.load_instruction_index = producer->instruction_index;
  record.load_access =
      route3_memory_access_record(*query.index->block, producer->instruction_index);
  record.access_available =
      record.load_access &&
      record.load_access.node_kind == Route3MemoryAccessNodeKind::LoadGlobal &&
      record.load_access.base_kind == Route3MemoryAccessBaseKind::GlobalSymbol;
  return record;
}

Route3SameBlockLoadLocalSourceRecord
route3_same_block_load_local_source_record(
    Route1SameBlockProducerQuery query,
    const Value& value) {
  Route3SameBlockLoadLocalSourceRecord record{
      .available = value.kind == Value::Kind::Named && !value.name.empty(),
      .root_value = route1_source_value_identity(value),
  };
  if (!query || !record.available || query.index->block == nullptr) {
    return {};
  }
  const auto producer = route1_find_same_block_scalar_producer(query, value);
  if (!producer.has_value() ||
      producer->record == nullptr ||
      producer->record->kind != Route1ProducerKind::LoadLocal) {
    return record;
  }
  const auto& block = *query.index->block;
  record.load_instruction_index = producer->instruction_index;
  record.load_access =
      route3_memory_access_record(block, producer->instruction_index);
  if (!record.load_access ||
      record.load_access.node_kind != Route3MemoryAccessNodeKind::LoadLocal ||
      record.load_access.base_kind != Route3MemoryAccessBaseKind::LocalSlot) {
    return record;
  }
  const auto limit = std::min(query.before_instruction_index, block.insts.size());
  for (std::size_t index = producer->instruction_index + 1U; index < limit;
       ++index) {
    const auto store_access = route3_memory_access_record(block, index);
    if (store_access.node_kind == Route3MemoryAccessNodeKind::StoreLocal &&
        route3_same_local_slot(record.load_access, store_access)) {
      record.invalidating_store_instruction_index = index;
      record.invalidating_store_access = store_access;
      return record;
    }
  }
  record.source_available = true;
  return record;
}

Route3SameBlockGlobalLoadAccessRecord
route3_find_same_block_global_load_access(
    Route3MemoryAccessQuery query,
    const Value& value) {
  Route3SameBlockGlobalLoadAccessRecord record{
      .available = value.kind == Value::Kind::Named && !value.name.empty(),
      .root_value = route1_source_value_identity(value),
  };
  if (!query || !record.available) {
    return {};
  }
  for (auto it = query.index->records.rbegin();
       it != query.index->records.rend();
       ++it) {
    const auto& candidate = *it;
    if (!candidate ||
        candidate.node_kind != Route3MemoryAccessNodeKind::LoadGlobal ||
        candidate.base_kind != Route3MemoryAccessBaseKind::GlobalSymbol ||
        candidate.instruction_index >= query.before_instruction_index ||
        candidate.result_value.value_kind != Value::Kind::Named ||
        candidate.result_value.name != value.name ||
        candidate.result_value.type != value.type) {
      continue;
    }
    record.load_instruction_index = candidate.instruction_index;
    record.load_access = candidate;
    record.access_available = true;
    return record;
  }
  return record;
}

Route3SameBlockLoadLocalSourceRecord
route3_find_same_block_load_local_source(
    Route3MemoryAccessQuery query,
    const Value& value) {
  Route3SameBlockLoadLocalSourceRecord record{
      .available = value.kind == Value::Kind::Named && !value.name.empty(),
      .root_value = route1_source_value_identity(value),
  };
  if (!query || !record.available) {
    return {};
  }
  const Route3MemoryAccessRecord* load_access = nullptr;
  for (auto it = query.index->records.rbegin();
       it != query.index->records.rend();
       ++it) {
    const auto& candidate = *it;
    if (!candidate ||
        candidate.node_kind != Route3MemoryAccessNodeKind::LoadLocal ||
        candidate.base_kind != Route3MemoryAccessBaseKind::LocalSlot ||
        candidate.instruction_index >= query.before_instruction_index ||
        candidate.result_value.value_kind != Value::Kind::Named ||
        candidate.result_value.name != value.name ||
        candidate.result_value.type != value.type) {
      continue;
    }
    load_access = &candidate;
    break;
  }
  if (load_access == nullptr) {
    return record;
  }
  record.load_instruction_index = load_access->instruction_index;
  record.load_access = *load_access;
  for (const auto& candidate : query.index->records) {
    if (!candidate ||
        candidate.node_kind != Route3MemoryAccessNodeKind::StoreLocal ||
        candidate.instruction_index <= load_access->instruction_index ||
        candidate.instruction_index >= query.before_instruction_index ||
        !route3_same_local_slot(*load_access, candidate)) {
      continue;
    }
    record.invalidating_store_instruction_index = candidate.instruction_index;
    record.invalidating_store_access = candidate;
    return record;
  }
  record.source_available = true;
  return record;
}

Route3SameBlockLoadLocalStoredValueSourceRecord
route3_find_same_block_load_local_stored_value_source(
    Route3MemoryAccessQuery query,
    const Value& value) {
  Route3SameBlockLoadLocalStoredValueSourceRecord record{
      .available = value.kind == Value::Kind::Named && !value.name.empty(),
      .root_value = route1_source_value_identity(value),
  };
  if (!query || !record.available) {
    return {};
  }
  const Route3MemoryAccessRecord* load_access = nullptr;
  for (auto it = query.index->records.rbegin();
       it != query.index->records.rend();
       ++it) {
    const auto& candidate = *it;
    if (!candidate ||
        candidate.node_kind != Route3MemoryAccessNodeKind::LoadLocal ||
        candidate.base_kind != Route3MemoryAccessBaseKind::LocalSlot ||
        candidate.instruction_index >= query.before_instruction_index ||
        candidate.result_value.value_kind != Value::Kind::Named ||
        candidate.result_value.name != value.name ||
        candidate.result_value.type != value.type) {
      continue;
    }
    load_access = &candidate;
    break;
  }
  if (load_access == nullptr) {
    return record;
  }
  record.load_instruction_index = load_access->instruction_index;
  record.load_access = *load_access;

  for (auto it = query.index->records.rbegin();
       it != query.index->records.rend();
       ++it) {
    const auto& candidate = *it;
    if (!candidate ||
        candidate.node_kind != Route3MemoryAccessNodeKind::StoreLocal ||
        candidate.base_kind != Route3MemoryAccessBaseKind::LocalSlot ||
        candidate.instruction_index >= load_access->instruction_index ||
        !route3_same_local_slot(*load_access, candidate)) {
      continue;
    }
    if (!route3_same_local_range(*load_access, candidate)) {
      if (route3_local_ranges_overlap(*load_access, candidate)) {
        return record;
      }
      continue;
    }
    if (!candidate.stored_value) {
      return record;
    }
    record.store_instruction_index = candidate.instruction_index;
    record.store_access = candidate;
    record.stored_value = candidate.stored_value;
    record.source_available = true;
    return record;
  }

  return record;
}

}  // namespace c4c::backend::bir
