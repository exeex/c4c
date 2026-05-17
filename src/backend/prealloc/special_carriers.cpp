#include "special_carriers.hpp"

#include "module.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace c4c::backend::prepare {

namespace {

[[nodiscard]] PreparedRegisterBank register_bank_from_class(PreparedRegisterClass reg_class) {
  switch (reg_class) {
    case PreparedRegisterClass::General:
      return PreparedRegisterBank::Gpr;
    case PreparedRegisterClass::Float:
      return PreparedRegisterBank::Fpr;
    case PreparedRegisterClass::Vector:
      return PreparedRegisterBank::Vreg;
    case PreparedRegisterClass::AggregateAddress:
      return PreparedRegisterBank::AggregateAddress;
    case PreparedRegisterClass::None:
      return PreparedRegisterBank::None;
  }
  return PreparedRegisterBank::None;
}

[[nodiscard]] const PreparedStoragePlanValue* find_storage_plan_value(
    const PreparedStoragePlanFunction& function_plan,
    PreparedValueId value_id) {
  for (const auto& value : function_plan.values) {
    if (value.value_id == value_id) {
      return &value;
    }
  }
  return nullptr;
}

void append_i128_missing_fact(PreparedI128CarrierFunction& function_carriers,
                              PreparedI128Carrier& carrier,
                              std::string fact) {
  carrier.missing_required_facts.push_back(fact);
  function_carriers.missing_required_facts.push_back(
      "value#" + std::to_string(carrier.value_id) + ":" + std::move(fact));
}

void append_f128_missing_fact(PreparedF128CarrierFunction& function_carriers,
                              PreparedF128Carrier& carrier,
                              std::string fact) {
  carrier.missing_required_facts.push_back(fact);
  function_carriers.missing_required_facts.push_back(
      "value#" + std::to_string(carrier.value_id) + ":" + std::move(fact));
}

[[nodiscard]] PreparedI128Carrier build_i128_carrier(
    PreparedI128CarrierFunction& function_carriers,
    const PreparedRegallocValue& value,
    const PreparedStoragePlanValue* storage) {
  constexpr std::size_t kLaneWidthBytes = 8;
  constexpr std::size_t kTotalSizeBytes = 16;
  PreparedI128Carrier carrier{
      .function_name = value.function_name,
      .value_id = value.value_id,
      .value_name = value.value_name,
      .source_type = bir::TypeKind::I128,
      .kind = PreparedI128CarrierKind::Missing,
      .lane_width_bytes = kLaneWidthBytes,
      .total_size_bytes = kTotalSizeBytes,
      .total_align_bytes = kTotalSizeBytes,
      .register_bank = register_bank_from_class(value.register_class),
      .register_class = value.register_class,
      .contiguous_width = std::max<std::size_t>(value.register_group_width, 1),
  };

  if (storage == nullptr) {
    if (!value.assigned_stack_slot.has_value()) {
      append_i128_missing_fact(function_carriers, carrier, "missing_storage_plan_value");
      return carrier;
    }
    carrier.kind = PreparedI128CarrierKind::MemoryBacked;
    carrier.slot_id = value.assigned_stack_slot->slot_id;
    carrier.stack_offset_bytes = value.assigned_stack_slot->offset_bytes;
    carrier.low_lane.slot_id = carrier.slot_id;
    carrier.low_lane.stack_offset_bytes = carrier.stack_offset_bytes;
    carrier.high_lane.slot_id = carrier.slot_id;
    carrier.high_lane.stack_offset_bytes = *carrier.stack_offset_bytes + kLaneWidthBytes;
    return carrier;
  }

  carrier.register_bank = storage->bank;
  carrier.contiguous_width = storage->contiguous_width;
  carrier.occupied_register_names = storage->occupied_register_names;
  carrier.register_placement = storage->register_placement;
  carrier.slot_id = storage->slot_id;
  carrier.stack_offset_bytes = storage->stack_offset_bytes;

  if (storage->encoding == PreparedStorageEncodingKind::Register) {
    const bool has_pair_occupancy =
        storage->contiguous_width == 2 && storage->occupied_register_names.size() == 2;
    if (!has_pair_occupancy) {
      append_i128_missing_fact(
          function_carriers,
          carrier,
          "register_pair_requires_width_2_and_two_structured_occupied_registers");
      return carrier;
    }
    if (storage->bank == PreparedRegisterBank::None ||
        value.register_class == PreparedRegisterClass::None) {
      append_i128_missing_fact(
          function_carriers,
          carrier,
          "register_pair_requires_explicit_register_bank_and_class");
      return carrier;
    }
    carrier.kind = PreparedI128CarrierKind::RegisterPair;
    carrier.low_lane.register_name = storage->occupied_register_names[0];
    carrier.high_lane.register_name = storage->occupied_register_names[1];
    return carrier;
  }

  if (storage->encoding == PreparedStorageEncodingKind::FrameSlot) {
    if (!storage->slot_id.has_value() || !storage->stack_offset_bytes.has_value()) {
      append_i128_missing_fact(
          function_carriers,
          carrier,
          "memory_backed_i128_requires_slot_id_and_stack_offset");
      return carrier;
    }
    carrier.kind = PreparedI128CarrierKind::MemoryBacked;
    carrier.low_lane.slot_id = storage->slot_id;
    carrier.low_lane.stack_offset_bytes = storage->stack_offset_bytes;
    carrier.high_lane.slot_id = storage->slot_id;
    carrier.high_lane.stack_offset_bytes = *storage->stack_offset_bytes + kLaneWidthBytes;
    return carrier;
  }

  if (storage->encoding == PreparedStorageEncodingKind::None &&
      value.assigned_stack_slot.has_value()) {
    carrier.kind = PreparedI128CarrierKind::MemoryBacked;
    carrier.slot_id = value.assigned_stack_slot->slot_id;
    carrier.stack_offset_bytes = value.assigned_stack_slot->offset_bytes;
    carrier.low_lane.slot_id = carrier.slot_id;
    carrier.low_lane.stack_offset_bytes = carrier.stack_offset_bytes;
    carrier.high_lane.slot_id = carrier.slot_id;
    carrier.high_lane.stack_offset_bytes = *carrier.stack_offset_bytes + kLaneWidthBytes;
    return carrier;
  }

  append_i128_missing_fact(function_carriers, carrier, "unsupported_i128_storage_encoding");
  return carrier;
}

[[nodiscard]] PreparedF128Carrier build_f128_carrier(
    PreparedF128CarrierFunction& function_carriers,
    const PreparedRegallocValue& value,
    const PreparedStoragePlanValue* storage) {
  constexpr std::size_t kTotalSizeBytes = 16;
  PreparedF128Carrier carrier{
      .function_name = value.function_name,
      .value_id = value.value_id,
      .value_name = value.value_name,
      .source_type = bir::TypeKind::F128,
      .kind = PreparedF128CarrierKind::Missing,
      .total_size_bytes = kTotalSizeBytes,
      .total_align_bytes = kTotalSizeBytes,
      .register_bank = register_bank_from_class(value.register_class),
      .register_class = value.register_class,
      .contiguous_width = std::max<std::size_t>(value.register_group_width, 1),
      .constant_payload = value.constant_f128_payload,
  };

  if (storage == nullptr) {
    if (!value.assigned_stack_slot.has_value()) {
      append_f128_missing_fact(function_carriers, carrier, "missing_storage_plan_value");
      return carrier;
    }
    carrier.kind = PreparedF128CarrierKind::MemoryBacked;
    carrier.slot_id = value.assigned_stack_slot->slot_id;
    carrier.stack_offset_bytes = value.assigned_stack_slot->offset_bytes;
    return carrier;
  }

  carrier.register_bank = storage->bank;
  carrier.contiguous_width = storage->contiguous_width;
  carrier.register_name = storage->register_name;
  carrier.occupied_register_names = storage->occupied_register_names;
  carrier.register_placement = storage->register_placement;
  carrier.slot_id = storage->slot_id;
  carrier.stack_offset_bytes = storage->stack_offset_bytes;
  if (storage->immediate_f128.has_value()) {
    carrier.constant_payload = storage->immediate_f128;
  }

  if (storage->encoding == PreparedStorageEncodingKind::Immediate) {
    if (!storage->immediate_f128.has_value()) {
      append_f128_missing_fact(
          function_carriers,
          carrier,
          "immediate_f128_constant_requires_full_width_payload");
      return carrier;
    }
    carrier.kind = PreparedF128CarrierKind::Missing;
    return carrier;
  }

  if (storage->encoding == PreparedStorageEncodingKind::Register) {
    const bool has_full_width_register =
        storage->contiguous_width == 1 &&
        storage->register_name.has_value() &&
        storage->occupied_register_names.size() == 1;
    if (!has_full_width_register) {
      append_f128_missing_fact(
          function_carriers,
          carrier,
          "full_width_f128_register_requires_single_structured_16_byte_register");
      return carrier;
    }
    if (storage->bank != PreparedRegisterBank::Fpr &&
        storage->bank != PreparedRegisterBank::Vreg) {
      append_f128_missing_fact(
          function_carriers,
          carrier,
          "full_width_f128_register_requires_fp_simd_register_bank");
      return carrier;
    }
    carrier.kind = PreparedF128CarrierKind::FullWidthRegister;
    carrier.register_bank = PreparedRegisterBank::Vreg;
    carrier.register_class = PreparedRegisterClass::Vector;
    return carrier;
  }

  if (storage->encoding == PreparedStorageEncodingKind::FrameSlot) {
    if (!storage->slot_id.has_value() || !storage->stack_offset_bytes.has_value()) {
      append_f128_missing_fact(
          function_carriers,
          carrier,
          "memory_backed_f128_requires_slot_id_and_stack_offset");
      return carrier;
    }
    carrier.kind = PreparedF128CarrierKind::MemoryBacked;
    return carrier;
  }

  if (storage->encoding == PreparedStorageEncodingKind::None &&
      value.assigned_stack_slot.has_value()) {
    carrier.kind = PreparedF128CarrierKind::MemoryBacked;
    carrier.slot_id = value.assigned_stack_slot->slot_id;
    carrier.stack_offset_bytes = value.assigned_stack_slot->offset_bytes;
    return carrier;
  }

  append_f128_missing_fact(function_carriers, carrier, "unsupported_f128_storage_encoding");
  return carrier;
}

}  // namespace

void populate_i128_carriers(PreparedBirModule& prepared) {
  prepared.i128_carriers.functions.clear();

  for (const auto& regalloc_function : prepared.regalloc.functions) {
    PreparedI128CarrierFunction function_carriers{
        .function_name = regalloc_function.function_name,
        .carriers = {},
        .missing_required_facts = {},
    };
    const auto* storage_plan =
        find_prepared_storage_plan(prepared.storage_plans, regalloc_function.function_name);
    for (const auto& value : regalloc_function.values) {
      if (value.type != bir::TypeKind::I128) {
        continue;
      }
      const auto* storage =
          storage_plan == nullptr ? nullptr : find_storage_plan_value(*storage_plan, value.value_id);
      function_carriers.carriers.push_back(build_i128_carrier(function_carriers, value, storage));
    }
    if (!function_carriers.carriers.empty() || !function_carriers.missing_required_facts.empty()) {
      prepared.i128_carriers.functions.push_back(std::move(function_carriers));
    }
  }
}

void populate_f128_carriers(PreparedBirModule& prepared) {
  prepared.f128_carriers.functions.clear();

  for (const auto& regalloc_function : prepared.regalloc.functions) {
    PreparedF128CarrierFunction function_carriers{
        .function_name = regalloc_function.function_name,
        .carriers = {},
        .missing_required_facts = {},
    };
    const auto* storage_plan =
        find_prepared_storage_plan(prepared.storage_plans, regalloc_function.function_name);
    for (const auto& value : regalloc_function.values) {
      if (value.type != bir::TypeKind::F128) {
        continue;
      }
      const auto* storage =
          storage_plan == nullptr ? nullptr : find_storage_plan_value(*storage_plan, value.value_id);
      function_carriers.carriers.push_back(build_f128_carrier(function_carriers, value, storage));
    }
    if (!function_carriers.carriers.empty() || !function_carriers.missing_required_facts.empty()) {
      prepared.f128_carriers.functions.push_back(std::move(function_carriers));
    }
  }
}

}  // namespace c4c::backend::prepare
