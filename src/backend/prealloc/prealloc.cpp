#include "prealloc.hpp"
#include "atomics.hpp"
#include "call_plans.hpp"
#include "dynamic_stack.hpp"
#include "inline_asm.hpp"
#include "intrinsics.hpp"
#include "label_identity.hpp"
#include "regalloc_placement_identity.hpp"
#include "storage_plans.hpp"
#include "target_register_profile.hpp"
#include "variadic_entry_plans.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <unordered_set>

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

[[nodiscard]] PreparedRegisterBank register_bank_from_type(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return PreparedRegisterBank::Gpr;
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
      return PreparedRegisterBank::Fpr;
    case bir::TypeKind::I128:
    case bir::TypeKind::F128:
      return PreparedRegisterBank::Vreg;
    case bir::TypeKind::Void:
      return PreparedRegisterBank::None;
  }
  return PreparedRegisterBank::None;
}

[[nodiscard]] std::size_t type_size_bytes(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return 1;
    case bir::TypeKind::I16:
      return 2;
    case bir::TypeKind::I32:
    case bir::TypeKind::F32:
      return 4;
    case bir::TypeKind::I64:
    case bir::TypeKind::F64:
    case bir::TypeKind::Ptr:
      return 8;
    case bir::TypeKind::I128:
    case bir::TypeKind::F128:
      return 16;
    case bir::TypeKind::Void:
      return 0;
  }
  return 0;
}

[[nodiscard]] PreparedRegisterBank register_bank_from_arg_abi(const bir::CallArgAbiInfo& abi) {
  switch (abi.primary_class) {
    case bir::AbiValueClass::Sse:
      if (abi.type == bir::TypeKind::F128) {
        return PreparedRegisterBank::Vreg;
      }
      return PreparedRegisterBank::Fpr;
    case bir::AbiValueClass::Memory:
      return abi.type == bir::TypeKind::Ptr ? PreparedRegisterBank::AggregateAddress
                                            : register_bank_from_type(abi.type);
    case bir::AbiValueClass::Integer:
      return register_bank_from_type(abi.type);
  }
  return PreparedRegisterBank::None;
}

[[nodiscard]] PreparedRegisterClass register_class_from_bank(PreparedRegisterBank bank) {
  switch (bank) {
    case PreparedRegisterBank::Gpr:
      return PreparedRegisterClass::General;
    case PreparedRegisterBank::Fpr:
      return PreparedRegisterClass::Float;
    case PreparedRegisterBank::Vreg:
      return PreparedRegisterClass::Vector;
    case PreparedRegisterBank::AggregateAddress:
      return PreparedRegisterClass::AggregateAddress;
    case PreparedRegisterBank::None:
      return PreparedRegisterClass::None;
  }
  return PreparedRegisterClass::None;
}

[[nodiscard]] PreparedRegisterBank register_bank_from_result_abi(
    const bir::CallResultAbiInfo& abi) {
  switch (abi.primary_class) {
    case bir::AbiValueClass::Sse:
      return PreparedRegisterBank::Fpr;
    case bir::AbiValueClass::Memory:
      return abi.type == bir::TypeKind::Ptr ? PreparedRegisterBank::AggregateAddress
                                            : register_bank_from_type(abi.type);
    case bir::AbiValueClass::Integer:
      return register_bank_from_type(abi.type);
  }
  return PreparedRegisterBank::None;
}

[[nodiscard]] std::optional<PreparedSpillSlotPlacement> make_spill_slot_placement(
    std::optional<PreparedFrameSlotId> slot_id,
    std::optional<std::size_t> offset_bytes) {
  if (!slot_id.has_value() || !offset_bytes.has_value()) {
    return std::nullopt;
  }
  return PreparedSpillSlotPlacement{
      .slot_id = *slot_id,
      .offset_bytes = *offset_bytes,
  };
}

[[nodiscard]] std::optional<PreparedRegisterPlacement> find_register_pool_placement(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class,
    std::size_t contiguous_width,
    const std::vector<std::string>& occupied_register_names,
    bool caller_pool) {
  if (occupied_register_names.empty()) {
    return std::nullopt;
  }
  const auto spans = caller_pool ? caller_saved_register_spans(target_profile, reg_class, contiguous_width)
                                 : callee_saved_register_spans(target_profile, reg_class, contiguous_width);
  for (const auto& span : spans) {
    if (span.occupied_register_names == occupied_register_names) {
      return span.placement;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<PreparedRegisterPlacement> find_register_placement(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class,
    std::size_t contiguous_width,
    const std::vector<std::string>& occupied_register_names) {
  if (const auto callee_saved =
          find_register_pool_placement(target_profile, reg_class, contiguous_width, occupied_register_names, false);
      callee_saved.has_value()) {
    return callee_saved;
  }
  return find_register_pool_placement(target_profile, reg_class, contiguous_width, occupied_register_names, true);
}

[[nodiscard]] std::optional<PreparedRegisterPlacement> assignment_register_placement(
    const c4c::TargetProfile& target_profile,
    const PreparedPhysicalRegisterAssignment& assignment) {
  if (assignment.placement.has_value()) {
    return assignment.placement;
  }
  return find_register_placement(target_profile,
                                 assignment.reg_class,
                                 assignment.contiguous_width,
                                 assignment.occupied_register_names);
}

[[nodiscard]] std::optional<PreparedRegisterPlacement> as_reserved_scratch_placement(
    std::optional<PreparedRegisterPlacement> placement) {
  if (!placement.has_value()) {
    return std::nullopt;
  }
  placement->pool = PreparedRegisterSlotPool::ReservedScratch;
  return placement;
}

[[nodiscard]] std::optional<ValueNameId> maybe_named_value_id(const PreparedNameTables& names,
                                                              const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return std::nullopt;
  }
  const ValueNameId id = names.value_names.find(value.name);
  if (id == kInvalidValueName) {
    return std::nullopt;
  }
  return id;
}

[[nodiscard]] std::size_t align_prepared_offset(std::size_t value,
                                                std::size_t alignment) {
  if (alignment <= 1) {
    return value;
  }
  const std::size_t remainder = value % alignment;
  return remainder == 0 ? value : value + (alignment - remainder);
}

[[nodiscard]] PreparedFrameSlotId next_prepared_frame_slot_id(
    const PreparedStackLayout& stack_layout) {
  PreparedFrameSlotId next = 0;
  for (const auto& slot : stack_layout.frame_slots) {
    next = std::max(next, slot.slot_id + 1);
  }
  return next;
}

[[nodiscard]] std::size_t saved_register_slot_unit_size(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterBank bank) {
  switch (bank) {
    case PreparedRegisterBank::Gpr:
    case PreparedRegisterBank::AggregateAddress:
      return target_profile.arch == c4c::TargetArch::I686 ? 4U : 8U;
    case PreparedRegisterBank::Fpr:
      return 8U;
    case PreparedRegisterBank::Vreg:
      return 16U;
    case PreparedRegisterBank::None:
      return 0U;
  }
  return 0U;
}

[[nodiscard]] PreparedSavedRegisterSlotPlacement make_saved_register_slot_placement(
    const PreparedSavedRegister& saved,
    PreparedFrameSlotId slot_id,
    std::size_t offset_bytes,
    std::size_t size_bytes,
    std::size_t align_bytes) {
  return PreparedSavedRegisterSlotPlacement{
      .bank = saved.bank,
      .register_name = saved.register_name,
      .contiguous_width = saved.contiguous_width,
      .occupied_register_names = saved.occupied_register_names,
      .save_index = saved.save_index,
      .register_placement = saved.placement,
      .slot_id = slot_id,
      .stack_offset_bytes = offset_bytes,
      .size_bytes = size_bytes,
      .align_bytes = align_bytes,
      .fixed_location = true,
  };
}

[[nodiscard]] std::optional<PreparedValueHome> prepared_home_for_named_value(
    PreparedNameTables& names,
    const PreparedValueLocationFunction* value_locations,
    const bir::Value& value) {
  if (value_locations == nullptr) {
    return std::nullopt;
  }
  const auto value_name = maybe_named_value_id(names, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  const auto* home = find_prepared_value_home(*value_locations, *value_name);
  if (home == nullptr) {
    return std::nullopt;
  }
  return *home;
}

[[nodiscard]] const PreparedRegallocFunction* find_regalloc_function(
    const PreparedRegalloc& regalloc,
    FunctionNameId function_name) {
  for (const auto& function : regalloc.functions) {
    if (function.function_name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

[[nodiscard]] const PreparedRegallocValue* find_regalloc_value_by_name(
    const PreparedRegallocFunction& function,
    ValueNameId value_name) {
  for (const auto& value : function.values) {
    if (value.value_name == value_name) {
      return &value;
    }
  }
  return nullptr;
}

[[nodiscard]] const PreparedRegallocValue* find_f128_constant_regalloc_value(
    const PreparedRegallocFunction& function,
    const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Immediate ||
      value.type != bir::TypeKind::F128 ||
      !value.f128_payload.has_value()) {
    return nullptr;
  }
  for (const auto& regalloc_value : function.values) {
    if (regalloc_value.type == bir::TypeKind::F128 &&
        regalloc_value.constant_f128_payload == value.f128_payload) {
      return &regalloc_value;
    }
  }
  return nullptr;
}

[[nodiscard]] const PreparedLivenessFunction* find_liveness_function(
    const PreparedLiveness& liveness,
    FunctionNameId function_name) {
  for (const auto& function : liveness.functions) {
    if (function.function_name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<std::size_t> find_call_program_point(
    const PreparedLivenessFunction& function,
    std::size_t block_index,
    std::size_t instruction_index) {
  for (const auto& block : function.blocks) {
    if (block.block_index != block_index) {
      continue;
    }
    return block.start_point + instruction_index;
  }
  return std::nullopt;
}

[[nodiscard]] PreparedMoveStorageKind move_storage_kind_from_home(
    const PreparedValueHome& home) {
  switch (home.kind) {
    case PreparedValueHomeKind::Register:
      return PreparedMoveStorageKind::Register;
    case PreparedValueHomeKind::StackSlot:
      return PreparedMoveStorageKind::StackSlot;
    case PreparedValueHomeKind::None:
    case PreparedValueHomeKind::RematerializableImmediate:
    case PreparedValueHomeKind::PointerBasePlusOffset:
      return PreparedMoveStorageKind::None;
  }
  return PreparedMoveStorageKind::None;
}

[[nodiscard]] PreparedStorageEncodingKind storage_encoding_from_home(
    const PreparedValueHome& home) {
  switch (home.kind) {
    case PreparedValueHomeKind::Register:
      return PreparedStorageEncodingKind::Register;
    case PreparedValueHomeKind::StackSlot:
      return PreparedStorageEncodingKind::FrameSlot;
    case PreparedValueHomeKind::RematerializableImmediate:
      return PreparedStorageEncodingKind::Immediate;
    case PreparedValueHomeKind::PointerBasePlusOffset:
      return PreparedStorageEncodingKind::ComputedAddress;
    case PreparedValueHomeKind::None:
      return PreparedStorageEncodingKind::None;
  }
  return PreparedStorageEncodingKind::None;
}

[[nodiscard]] PreparedRegisterBank published_bank_for_value(
    const PreparedRegallocFunction* regalloc_function,
    ValueNameId value_name,
    bir::TypeKind type) {
  if (regalloc_function != nullptr) {
    if (const auto* regalloc_value = find_regalloc_value_by_name(*regalloc_function, value_name);
        regalloc_value != nullptr) {
      return register_bank_from_class(regalloc_value->register_class);
    }
  }
  return register_bank_from_type(type);
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

void append_f128_runtime_helper_fact(PreparedF128RuntimeHelper& helper,
                                     std::string fact) {
  if (std::find(helper.missing_required_facts.begin(),
                helper.missing_required_facts.end(),
                fact) == helper.missing_required_facts.end()) {
    helper.missing_required_facts.push_back(std::move(fact));
  }
}

[[nodiscard]] PreparedF128RuntimeHelper::CarrierBinding make_f128_helper_carrier_binding(
    const PreparedF128Carrier& carrier) {
  return PreparedF128RuntimeHelper::CarrierBinding{
      .value_id = carrier.value_id,
      .value_name = carrier.value_name,
      .carrier_kind = carrier.kind,
      .width_bytes = carrier.total_size_bytes,
      .align_bytes = carrier.total_align_bytes,
      .register_bank = carrier.register_bank,
      .register_class = carrier.register_class,
      .register_name = carrier.register_name,
      .slot_id = carrier.slot_id,
      .stack_offset_bytes = carrier.stack_offset_bytes,
  };
}

void populate_f128_helper_carrier_from_fact(
    const PreparedF128CarrierFunction* function_carriers,
    PreparedF128RuntimeHelper& helper,
    PreparedValueId value_id,
    std::optional<PreparedF128RuntimeHelper::CarrierBinding>& output,
    std::string_view fact_prefix) {
  output.reset();
  if (function_carriers == nullptr) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_missing_prepared_f128_carriers");
    return;
  }
  const auto* carrier = find_prepared_f128_carrier(*function_carriers, value_id);
  if (carrier == nullptr) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_missing_prepared_f128_carrier");
    return;
  }
  output = make_f128_helper_carrier_binding(*carrier);
  if (carrier->kind == PreparedF128CarrierKind::Missing) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_requires_full_width_f128_carrier");
  }
  if (carrier->total_size_bytes != 16 || carrier->total_align_bytes != 16) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_requires_16_byte_f128_carrier");
  }
  for (const auto& fact : carrier->missing_required_facts) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_carrier_fact:" + fact);
  }
}

[[nodiscard]] PreparedRegisterBank f128_helper_abi_bank_for_register(
    const std::string& register_name) {
  return !register_name.empty() && register_name.front() == 'q'
             ? PreparedRegisterBank::Vreg
             : PreparedRegisterBank::Fpr;
}

[[nodiscard]] std::optional<PreparedF128RuntimeHelper::AbiRegisterBinding>
make_f128_helper_abi_register_binding(
    const c4c::TargetProfile& target_profile,
    PreparedValueId value_id,
    ValueNameId value_name,
    std::optional<std::size_t> helper_argument_index,
    std::size_t abi_register_index) {
  std::optional<std::string> register_name;
  std::optional<PreparedRegisterPlacement> placement;

  if (helper_argument_index.has_value()) {
    const auto arg_abi = infer_call_arg_abi(target_profile, bir::TypeKind::F128);
    if (!arg_abi.has_value() || !arg_abi->passed_in_register) {
      return std::nullopt;
    }
    register_name =
        call_arg_destination_register_name(target_profile, *arg_abi, abi_register_index);
    placement =
        call_arg_destination_register_placement(target_profile, *arg_abi, abi_register_index);
  } else {
    const bir::CallResultAbiInfo result_abi{
        .type = bir::TypeKind::F128,
        .primary_class = target_profile.has_float_return_registers
                             ? bir::AbiValueClass::Sse
                             : bir::AbiValueClass::Integer,
    };
    register_name = call_result_destination_register_name(target_profile, result_abi);
    placement = call_result_destination_register_placement(target_profile, result_abi);
  }

  if (!register_name.has_value() || register_name->empty() || !placement.has_value()) {
    return std::nullopt;
  }

  const PreparedRegisterBank bank = f128_helper_abi_bank_for_register(*register_name);
  PreparedRegisterPlacement normalized_placement = *placement;
  normalized_placement.bank = bank;
  normalized_placement.contiguous_width = 1;

  return PreparedF128RuntimeHelper::AbiRegisterBinding{
      .value_id = value_id,
      .value_name = value_name,
      .helper_argument_index = helper_argument_index,
      .abi_register_index = abi_register_index,
      .width_bytes = 16,
      .register_bank = bank,
      .register_class = register_class_from_bank(bank),
      .register_name = *register_name,
      .contiguous_width = 1,
      .occupied_register_names = {*register_name},
      .register_placement = normalized_placement,
  };
}

[[nodiscard]] std::optional<PreparedF128RuntimeHelper::ScalarResultOwnership>
make_f128_helper_scalar_ownership(
    PreparedF128RuntimeHelper& helper,
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueLocationFunction* value_locations,
    PreparedValueId value_id,
    ValueNameId value_name,
    bir::TypeKind type,
    PreparedRegisterBank expected_bank,
    std::string_view fact_prefix) {
  const std::size_t width_bytes = type_size_bytes(type);
  if (value_name == kInvalidValueName) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_value_identity");
    return std::nullopt;
  }
  if (value_locations == nullptr) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_value_locations");
    return std::nullopt;
  }
  const auto* home = find_prepared_value_home(*value_locations, value_name);
  if (home == nullptr || home->value_id != value_id) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_value_home");
    return std::nullopt;
  }
  const PreparedRegisterBank bank =
      published_bank_for_value(regalloc_function, value_name, type);
  if (bank != expected_bank) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_expected_bank");
  }
  if (width_bytes == 0) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_width");
  }
  if (home->kind != PreparedValueHomeKind::Register &&
      home->kind != PreparedValueHomeKind::StackSlot) {
    append_f128_runtime_helper_fact(
        helper,
        std::string(fact_prefix) + "_scalar_ownership_requires_register_or_stack_home");
  }
  return PreparedF128RuntimeHelper::ScalarResultOwnership{
      .value_id = value_id,
      .value_name = value_name,
      .type = type,
      .width_bytes = width_bytes,
      .register_bank = bank,
      .home_kind = home->kind,
      .register_name = home->register_name,
      .slot_id = home->slot_id,
      .stack_offset_bytes = home->offset_bytes,
  };
}

[[nodiscard]] std::optional<PreparedF128RuntimeHelper::AbiRegisterBinding>
make_f128_helper_scalar_abi_register_binding(
    const c4c::TargetProfile& target_profile,
    const PreparedF128RuntimeHelper::ScalarResultOwnership& scalar,
    std::optional<std::size_t> helper_argument_index,
    std::size_t abi_register_index) {
  std::optional<std::string> register_name;
  std::optional<PreparedRegisterPlacement> placement;
  if (helper_argument_index.has_value()) {
    const auto arg_abi = infer_call_arg_abi(target_profile, scalar.type);
    if (!arg_abi.has_value() || !arg_abi->passed_in_register) {
      return std::nullopt;
    }
    register_name =
        call_arg_destination_register_name(target_profile, *arg_abi, abi_register_index);
    placement =
        call_arg_destination_register_placement(target_profile, *arg_abi, abi_register_index);
  } else {
    const bir::CallResultAbiInfo result_abi{
        .type = scalar.type,
        .primary_class = scalar.register_bank == PreparedRegisterBank::Gpr
                             ? bir::AbiValueClass::Integer
                             : bir::AbiValueClass::Sse,
    };
    register_name = call_result_destination_register_name(target_profile, result_abi);
    placement = call_result_destination_register_placement(target_profile, result_abi);
  }
  if (!register_name.has_value() || register_name->empty() || !placement.has_value()) {
    return std::nullopt;
  }
  return PreparedF128RuntimeHelper::AbiRegisterBinding{
      .value_id = scalar.value_id,
      .value_name = scalar.value_name,
      .helper_argument_index = helper_argument_index,
      .abi_register_index = abi_register_index,
      .width_bytes = scalar.width_bytes,
      .register_bank = scalar.register_bank,
      .register_class = register_class_from_bank(scalar.register_bank),
      .register_name = *register_name,
      .contiguous_width = 1,
      .occupied_register_names = {*register_name},
      .register_placement = *placement,
  };
}

void populate_f128_runtime_helper_abi_bindings(
    const c4c::TargetProfile& target_profile,
    PreparedF128RuntimeHelper& helper) {
  helper.lhs_abi_argument.reset();
  helper.rhs_abi_argument.reset();
  helper.result_abi_result.reset();
  helper.scalar_operand_abi_argument.reset();

  if (helper.callee_name.empty()) {
    append_f128_runtime_helper_fact(helper, "f128_helper_abi_bindings_require_callee_identity");
    return;
  }

  if (helper.helper_family == PreparedF128RuntimeHelperFamily::Cast) {
    const bool scalar_to_f128 = helper.result_type == bir::TypeKind::F128;
    if (scalar_to_f128) {
      helper.scalar_operand_abi_argument =
          helper.scalar_operand.has_value()
              ? make_f128_helper_scalar_abi_register_binding(
                    target_profile, *helper.scalar_operand, std::size_t{0}, 0)
              : std::optional<PreparedF128RuntimeHelper::AbiRegisterBinding>{};
      helper.result_abi_result = make_f128_helper_abi_register_binding(
          target_profile, helper.result_value_id, helper.result_value_name, std::nullopt, 0);
    } else {
      helper.lhs_abi_argument = make_f128_helper_abi_register_binding(
          target_profile, helper.operand_value_id, helper.operand_value_name, std::size_t{0}, 0);
      helper.result_abi_result =
          helper.scalar_result.has_value()
              ? make_f128_helper_scalar_abi_register_binding(
                    target_profile, *helper.scalar_result, std::nullopt, 0)
              : std::optional<PreparedF128RuntimeHelper::AbiRegisterBinding>{};
    }
    helper.abi_policy = PreparedF128RuntimeHelper::AbiPolicy{
        .transition = scalar_to_f128
                          ? PreparedF128RuntimeHelperAbiTransition::
                                DirectScalarArgumentAndF128Result
                          : PreparedF128RuntimeHelperAbiTransition::
                                DirectF128ArgumentAndScalarResult,
        .argument_bank = scalar_to_f128 ? PreparedRegisterBank::Fpr : PreparedRegisterBank::Vreg,
        .result_bank = scalar_to_f128 ? PreparedRegisterBank::Vreg : PreparedRegisterBank::Fpr,
        .argument_count = 1,
        .result_count = 1,
        .width_bytes = scalar_to_f128 ? std::size_t{16} : type_size_bytes(helper.result_type),
    };
  } else {
    helper.lhs_abi_argument =
        make_f128_helper_abi_register_binding(target_profile,
                                              helper.lhs_value_id,
                                              helper.lhs_value_name,
                                              std::size_t{0},
                                              0);
    helper.rhs_abi_argument =
        make_f128_helper_abi_register_binding(target_profile,
                                              helper.rhs_value_id,
                                              helper.rhs_value_name,
                                              std::size_t{1},
                                              1);
    helper.result_abi_result =
        helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison
            ? (helper.scalar_result.has_value()
                   ? make_f128_helper_scalar_abi_register_binding(
                         target_profile, *helper.scalar_result, std::nullopt, 0)
                   : std::optional<PreparedF128RuntimeHelper::AbiRegisterBinding>{})
            : make_f128_helper_abi_register_binding(target_profile,
                                                    helper.result_value_id,
                                                    helper.result_value_name,
                                                    std::nullopt,
                                                    0);
    helper.abi_policy = PreparedF128RuntimeHelper::AbiPolicy{
        .transition =
            helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison
                ? PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentsAndCmpResult
                : PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentsAndResult,
        .argument_bank = PreparedRegisterBank::Vreg,
        .result_bank = helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison
                           ? PreparedRegisterBank::Gpr
                           : PreparedRegisterBank::Vreg,
        .argument_count = 2,
        .result_count = 1,
        .width_bytes =
            helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison
                ? std::size_t{4}
                : std::size_t{16},
    };
  }

  if (!helper.result_abi_result.has_value() ||
      (helper.helper_family != PreparedF128RuntimeHelperFamily::Cast &&
       (!helper.lhs_abi_argument.has_value() || !helper.rhs_abi_argument.has_value())) ||
      (helper.helper_family == PreparedF128RuntimeHelperFamily::Cast &&
       helper.result_type == bir::TypeKind::F128 && !helper.scalar_operand_abi_argument.has_value()) ||
      (helper.helper_family == PreparedF128RuntimeHelperFamily::Cast &&
       helper.source_type == bir::TypeKind::F128 && !helper.lhs_abi_argument.has_value())) {
    append_f128_runtime_helper_fact(
        helper, "f128_helper_abi_bindings_require_supported_registers");
  }
}

[[nodiscard]] std::optional<PreparedF128RuntimeHelper::MarshalingMove>
make_f128_helper_marshaling_move(
    PreparedF128RuntimeHelper& helper,
    const PreparedF128RuntimeHelper::CarrierBinding& carrier,
    const PreparedF128RuntimeHelper::AbiRegisterBinding& abi_register,
    PreparedF128RuntimeHelperMarshalDirection direction,
    std::string_view fact_prefix) {
  if (carrier.carrier_kind != PreparedF128CarrierKind::FullWidthRegister) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_marshaling_requires_full_width_register_carrier");
  }
  if (carrier.width_bytes != 16 || abi_register.width_bytes != 16) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_marshaling_requires_16_byte_f128_values");
  }
  if (carrier.value_id != abi_register.value_id ||
      carrier.value_name != abi_register.value_name) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_marshaling_requires_matching_value_identity");
  }
  if (abi_register.register_bank != PreparedRegisterBank::Vreg ||
      abi_register.register_class != PreparedRegisterClass::Vector ||
      abi_register.register_name.empty() ||
      abi_register.occupied_register_names.empty() ||
      abi_register.contiguous_width != 1 ||
      !abi_register.register_placement.has_value()) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_marshaling_requires_q_register_binding");
  }
  return PreparedF128RuntimeHelper::MarshalingMove{
      .direction = direction,
      .carrier = carrier,
      .abi_register = abi_register,
  };
}

void populate_f128_runtime_helper_marshaling(PreparedF128RuntimeHelper& helper) {
  helper.lhs_argument_move.reset();
  helper.rhs_argument_move.reset();
  helper.result_unmarshal_move.reset();
  helper.scalar_result_unmarshal_move.reset();
  helper.scalar_operand_argument_move.reset();

  auto populate_move =
      [&](const std::optional<PreparedF128RuntimeHelper::CarrierBinding>& carrier,
          const std::optional<PreparedF128RuntimeHelper::AbiRegisterBinding>& abi_register,
          PreparedF128RuntimeHelperMarshalDirection direction,
          std::optional<PreparedF128RuntimeHelper::MarshalingMove>& output,
          std::string_view fact_prefix) {
        if (!carrier.has_value()) {
          append_f128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_carrier");
          return;
        }
        if (!abi_register.has_value()) {
          append_f128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_abi_binding");
          return;
        }
        output = make_f128_helper_marshaling_move(helper,
                                                  *carrier,
                                                  *abi_register,
                                                  direction,
                                                  fact_prefix);
      };

  if (helper.helper_family != PreparedF128RuntimeHelperFamily::Cast ||
      helper.source_type == bir::TypeKind::F128) {
    populate_move(helper.lhs_carrier,
                  helper.lhs_abi_argument,
                  PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument,
                  helper.lhs_argument_move,
                  "lhs");
  }
  if (helper.helper_family != PreparedF128RuntimeHelperFamily::Cast) {
    populate_move(helper.rhs_carrier,
                  helper.rhs_abi_argument,
                  PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument,
                  helper.rhs_argument_move,
                  "rhs");
  }
  if (helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison) {
    if (!helper.scalar_result.has_value()) {
      append_f128_runtime_helper_fact(
          helper, "cmp_result_marshaling_requires_scalar_result_ownership");
    } else if (!helper.result_abi_result.has_value()) {
      append_f128_runtime_helper_fact(
          helper, "cmp_result_marshaling_requires_abi_binding");
    } else if (helper.result_abi_result->register_bank != PreparedRegisterBank::Gpr ||
               helper.result_abi_result->width_bytes != 4) {
      append_f128_runtime_helper_fact(
          helper, "cmp_result_marshaling_requires_gpr_cmp_result_binding");
    } else {
      helper.scalar_result_unmarshal_move =
          PreparedF128RuntimeHelper::ScalarMarshalingMove{
              .direction =
                  PreparedF128RuntimeHelperMarshalDirection::AbiCmpResultToScalar,
              .scalar_result = *helper.scalar_result,
              .abi_register = *helper.result_abi_result,
          };
    }
  } else if (helper.helper_family == PreparedF128RuntimeHelperFamily::Cast) {
    const bool scalar_to_f128 = helper.result_type == bir::TypeKind::F128;
    if (scalar_to_f128) {
      if (!helper.scalar_operand.has_value()) {
        append_f128_runtime_helper_fact(
            helper, "cast_scalar_argument_marshaling_requires_scalar_operand_ownership");
      } else if (!helper.scalar_operand_abi_argument.has_value()) {
        append_f128_runtime_helper_fact(
            helper, "cast_scalar_argument_marshaling_requires_abi_binding");
      } else if (helper.scalar_operand_abi_argument->register_bank != PreparedRegisterBank::Fpr) {
        append_f128_runtime_helper_fact(
            helper, "cast_scalar_argument_marshaling_requires_fpr_binding");
      } else {
        helper.scalar_operand_argument_move =
            PreparedF128RuntimeHelper::ScalarMarshalingMove{
                .direction =
                    PreparedF128RuntimeHelperMarshalDirection::ScalarToAbiArgument,
                .scalar_result = *helper.scalar_operand,
                .abi_register = *helper.scalar_operand_abi_argument,
            };
      }
      populate_move(helper.result_carrier,
                    helper.result_abi_result,
                    PreparedF128RuntimeHelperMarshalDirection::AbiResultToCarrier,
                    helper.result_unmarshal_move,
                    "result");
    } else {
      if (!helper.scalar_result.has_value()) {
        append_f128_runtime_helper_fact(
            helper, "cast_scalar_result_marshaling_requires_scalar_result_ownership");
      } else if (!helper.result_abi_result.has_value()) {
        append_f128_runtime_helper_fact(
            helper, "cast_scalar_result_marshaling_requires_abi_binding");
      } else if (helper.result_abi_result->register_bank != PreparedRegisterBank::Fpr) {
        append_f128_runtime_helper_fact(
            helper, "cast_scalar_result_marshaling_requires_fpr_binding");
      } else {
        helper.scalar_result_unmarshal_move =
            PreparedF128RuntimeHelper::ScalarMarshalingMove{
                .direction =
                    PreparedF128RuntimeHelperMarshalDirection::AbiResultToScalar,
                .scalar_result = *helper.scalar_result,
                .abi_register = *helper.result_abi_result,
            };
      }
    }
  } else {
    populate_move(helper.result_carrier,
                  helper.result_abi_result,
                  PreparedF128RuntimeHelperMarshalDirection::AbiResultToCarrier,
                  helper.result_unmarshal_move,
                  "result");
  }
}

[[nodiscard]] std::vector<PreparedClobberedRegister> build_call_clobber_set(
    const c4c::TargetProfile& target_profile,
    const PreparedRegallocFunction* regalloc_function);

[[nodiscard]] std::vector<PreparedCallPreservedValue> build_call_preserved_values(
    const PreparedBirModule& prepared,
    const PreparedFramePlanFunction* frame_plan,
    const PreparedLivenessFunction* liveness_function,
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueLocationFunction* value_locations,
    std::size_t program_point,
    bool require_call_crossing_value);

[[nodiscard]] bool preserved_value_has_complete_route(
    const PreparedCallPreservedValue& preserved);

void populate_f128_runtime_helper_boundary_policy(
    const c4c::TargetProfile& target_profile,
    const PreparedRegallocFunction* regalloc_function,
    PreparedF128RuntimeHelper& helper) {
  helper.resource_policy = PreparedF128RuntimeHelper::ResourcePolicy{
      .call_boundary = true,
      .runtime_helper_callee = !helper.callee_name.empty(),
      .caller_saved_clobbers = true,
      .preserves_source_operation_identity = true,
  };
  helper.abi_policy = PreparedF128RuntimeHelper::AbiPolicy{
      .transition = PreparedF128RuntimeHelperAbiTransition::Missing,
      .argument_bank = PreparedRegisterBank::None,
      .result_bank = PreparedRegisterBank::None,
      .argument_count = 2,
      .result_count = 1,
      .width_bytes = 16,
  };
  helper.live_preservation_policy = PreparedF128RuntimeHelper::LivePreservationPolicy{
      .evaluated = false,
      .caller_saved_clobbers_modeled = false,
      .no_additional_live_preservation_required = false,
      .preserved_values = {},
  };
  helper.clobbered_registers = build_call_clobber_set(target_profile, regalloc_function);

  if (helper.callee_name.empty()) {
    append_f128_runtime_helper_fact(helper, "f128_helper_boundary_requires_callee_identity");
  }
  if (helper.clobbered_registers.empty()) {
    append_f128_runtime_helper_fact(
        helper, "f128_helper_boundary_requires_caller_saved_clobbers");
  }
}

void populate_f128_runtime_helper_call_ownership(
    const PreparedBirModule& prepared,
    const PreparedFramePlanFunction* frame_plan,
    const PreparedLivenessFunction* liveness_function,
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueLocationFunction* value_locations,
    PreparedF128RuntimeHelper& helper) {
  const bool has_clobber_policy =
      helper.resource_policy.caller_saved_clobbers && !helper.clobbered_registers.empty();
  const auto helper_point =
      liveness_function == nullptr
          ? std::optional<std::size_t>{}
          : find_call_program_point(*liveness_function,
                                    helper.block_index,
                                    helper.instruction_index);
  std::vector<PreparedCallPreservedValue> preserved_values =
      helper_point.has_value()
          ? build_call_preserved_values(prepared,
                                        frame_plan,
                                        liveness_function,
                                        regalloc_function,
                                        value_locations,
                                        *helper_point,
                                        false)
          : std::vector<PreparedCallPreservedValue>{};
  const bool preserved_values_complete =
      std::all_of(preserved_values.begin(),
                  preserved_values.end(),
                  preserved_value_has_complete_route);
  if (liveness_function == nullptr || regalloc_function == nullptr ||
      !helper_point.has_value()) {
    append_f128_runtime_helper_fact(
        helper, "live_preservation_requires_structured_live_across_helper_facts");
  } else if (!preserved_values_complete) {
    append_f128_runtime_helper_fact(
        helper, "live_preservation_requires_complete_preserved_value_routes");
  }
  helper.live_preservation_policy = PreparedF128RuntimeHelper::LivePreservationPolicy{
      .evaluated = true,
      .caller_saved_clobbers_modeled = has_clobber_policy,
      .no_additional_live_preservation_required =
          helper_point.has_value() && preserved_values_complete,
      .preserved_values = std::move(preserved_values),
  };
}

void populate_f128_runtime_helper_ownership(PreparedF128RuntimeHelper& helper) {
  const bool scalar_to_f128_cast =
      helper.helper_family == PreparedF128RuntimeHelperFamily::Cast &&
      helper.result_type == bir::TypeKind::F128;
  const bool f128_to_scalar_cast =
      helper.helper_family == PreparedF128RuntimeHelperFamily::Cast &&
      helper.source_type == bir::TypeKind::F128;
  const bool has_carriers =
      scalar_to_f128_cast
          ? (helper.scalar_operand.has_value() && helper.result_carrier.has_value())
          : (f128_to_scalar_cast
                 ? (helper.lhs_carrier.has_value() && helper.scalar_result.has_value())
                 : (helper.lhs_carrier.has_value() &&
                    helper.rhs_carrier.has_value() &&
                    (helper.result_carrier.has_value() ||
                     (helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison &&
                      helper.scalar_result.has_value()))));
  const bool has_resource_policy =
      helper.resource_policy.call_boundary &&
      helper.resource_policy.runtime_helper_callee &&
      helper.resource_policy.preserves_source_operation_identity;
  const bool has_clobber_policy =
      helper.resource_policy.caller_saved_clobbers && !helper.clobbered_registers.empty();
  const bool has_abi_bindings =
      (((helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison &&
         helper.abi_policy.transition ==
             PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentsAndCmpResult &&
         helper.abi_policy.argument_bank == PreparedRegisterBank::Vreg &&
         helper.abi_policy.result_bank == PreparedRegisterBank::Gpr &&
         helper.abi_policy.argument_count == 2 &&
         helper.abi_policy.width_bytes == 4) ||
        (helper.helper_family == PreparedF128RuntimeHelperFamily::Arithmetic &&
         helper.abi_policy.transition ==
             PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentsAndResult &&
         helper.abi_policy.argument_bank == PreparedRegisterBank::Vreg &&
         helper.abi_policy.result_bank == PreparedRegisterBank::Vreg &&
         helper.abi_policy.argument_count == 2 &&
         helper.abi_policy.width_bytes == 16) ||
        (scalar_to_f128_cast &&
         helper.abi_policy.transition ==
             PreparedF128RuntimeHelperAbiTransition::DirectScalarArgumentAndF128Result &&
         helper.abi_policy.argument_bank == PreparedRegisterBank::Fpr &&
         helper.abi_policy.result_bank == PreparedRegisterBank::Vreg &&
         helper.abi_policy.argument_count == 1 &&
         helper.abi_policy.width_bytes == 16) ||
        (f128_to_scalar_cast &&
         helper.abi_policy.transition ==
             PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentAndScalarResult &&
         helper.abi_policy.argument_bank == PreparedRegisterBank::Vreg &&
         helper.abi_policy.result_bank == PreparedRegisterBank::Fpr &&
         helper.abi_policy.argument_count == 1 &&
         (helper.abi_policy.width_bytes == 4 || helper.abi_policy.width_bytes == 8))) &&
       helper.abi_policy.result_count == 1);
  const bool has_marshaling =
      scalar_to_f128_cast
          ? (helper.scalar_operand_argument_move.has_value() &&
             helper.result_unmarshal_move.has_value())
          : (f128_to_scalar_cast
                 ? (helper.lhs_argument_move.has_value() &&
                    helper.scalar_result_unmarshal_move.has_value())
                 : (helper.lhs_argument_move.has_value() &&
                    helper.rhs_argument_move.has_value() &&
                    (helper.result_unmarshal_move.has_value() ||
                     (helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison &&
                      helper.scalar_result_unmarshal_move.has_value()))));
  const bool has_cmp_result_consumption =
      helper.helper_family != PreparedF128RuntimeHelperFamily::Comparison ||
      (helper.scalar_cmp_result_consumption.has_value() &&
       helper.scalar_cmp_result_consumption->cmp_type == bir::TypeKind::I32 &&
       helper.scalar_cmp_result_consumption->bir_result_type == bir::TypeKind::I1 &&
       helper.scalar_cmp_result_consumption->zero_test !=
           PreparedF128CmpResultZeroTest::Missing &&
       helper.scalar_cmp_result_consumption->consumes_helper_cmp_result &&
       helper.scalar_cmp_result_consumption->owns_bir_i1_result);
  const bool has_live_preservation =
      helper.live_preservation_policy.evaluated &&
      helper.live_preservation_policy.caller_saved_clobbers_modeled &&
      helper.live_preservation_policy.no_additional_live_preservation_required;

  helper.selected_call_ownership =
      PreparedF128RuntimeHelper::SelectedCallOwnershipPolicy{
          .owns_terminal_call =
              !helper.callee_name.empty() && has_carriers && has_resource_policy &&
              has_clobber_policy && has_abi_bindings && has_marshaling &&
              has_cmp_result_consumption && has_live_preservation,
          .has_callee_identity = !helper.callee_name.empty(),
          .has_resource_policy = has_resource_policy,
          .has_clobber_policy = has_clobber_policy,
          .has_abi_bindings = has_abi_bindings,
          .has_marshaling = has_marshaling && has_cmp_result_consumption,
          .has_live_preservation = has_live_preservation,
      };

  if (!has_carriers) {
    append_f128_runtime_helper_fact(helper, "selected_call_ownership_requires_f128_carriers");
  }
  if (!helper.selected_call_ownership.has_resource_policy) {
    append_f128_runtime_helper_fact(helper, "selected_call_ownership_requires_resource_policy");
  }
  if (!helper.selected_call_ownership.has_clobber_policy) {
    append_f128_runtime_helper_fact(helper, "selected_call_ownership_requires_clobber_policy");
  }
  if (!helper.selected_call_ownership.has_abi_bindings) {
    append_f128_runtime_helper_fact(helper, "selected_call_ownership_requires_abi_bindings");
  }
  if (!helper.selected_call_ownership.has_marshaling) {
    append_f128_runtime_helper_fact(helper, "selected_call_ownership_requires_marshaling");
  }
  if (!helper.selected_call_ownership.has_live_preservation) {
    append_f128_runtime_helper_fact(
        helper, "selected_call_ownership_requires_live_preservation_policy");
  }
}

void populate_f128_runtime_helper_facts(PreparedBirModule& prepared) {
  for (auto& function_helpers : prepared.f128_runtime_helpers.functions) {
    const auto* function_carriers =
        find_prepared_f128_carriers(prepared.f128_carriers, function_helpers.function_name);
    const auto* regalloc_function =
        find_regalloc_function(prepared.regalloc, function_helpers.function_name);
    const auto* frame_plan = find_prepared_frame_plan(prepared, function_helpers.function_name);
    const auto* liveness_function =
        find_liveness_function(prepared.liveness, function_helpers.function_name);
    const auto* value_locations =
        find_prepared_value_location_function(prepared, function_helpers.function_name);
    for (auto& helper : function_helpers.helpers) {
      helper.lhs_carrier.reset();
      helper.rhs_carrier.reset();
      helper.result_carrier.reset();
      helper.scalar_operand.reset();
      helper.scalar_result.reset();
      helper.missing_required_facts.clear();
      populate_f128_runtime_helper_boundary_policy(
          prepared.target_profile, regalloc_function, helper);
      if (helper.helper_family == PreparedF128RuntimeHelperFamily::Cast) {
        if (helper.result_type == bir::TypeKind::F128) {
          helper.scalar_operand =
              make_f128_helper_scalar_ownership(helper,
                                                regalloc_function,
                                                value_locations,
                                                helper.operand_value_id,
                                                helper.operand_value_name,
                                                helper.source_type,
                                                PreparedRegisterBank::Fpr,
                                                "cast_operand");
          populate_f128_helper_carrier_from_fact(
              function_carriers, helper, helper.result_value_id, helper.result_carrier, "result");
        } else {
          populate_f128_helper_carrier_from_fact(
              function_carriers, helper, helper.operand_value_id, helper.lhs_carrier, "lhs");
          helper.scalar_result =
              make_f128_helper_scalar_ownership(helper,
                                                regalloc_function,
                                                value_locations,
                                                helper.result_value_id,
                                                helper.result_value_name,
                                                helper.result_type,
                                                PreparedRegisterBank::Fpr,
                                                "cast_result");
        }
      } else {
        populate_f128_helper_carrier_from_fact(
            function_carriers, helper, helper.lhs_value_id, helper.lhs_carrier, "lhs");
        populate_f128_helper_carrier_from_fact(
            function_carriers, helper, helper.rhs_value_id, helper.rhs_carrier, "rhs");
      }
      if (helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison) {
        helper.scalar_result =
            make_f128_helper_scalar_ownership(helper,
                                              regalloc_function,
                                              value_locations,
                                              helper.result_value_id,
                                              helper.result_value_name,
                                              bir::TypeKind::I32,
                                              PreparedRegisterBank::Gpr,
                                              "cmp_result");
      } else if (helper.helper_family == PreparedF128RuntimeHelperFamily::Arithmetic) {
        populate_f128_helper_carrier_from_fact(
            function_carriers, helper, helper.result_value_id, helper.result_carrier, "result");
      }
      populate_f128_runtime_helper_abi_bindings(prepared.target_profile, helper);
      populate_f128_runtime_helper_marshaling(helper);
      if (helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison &&
          helper.result_ownership !=
              PreparedF128RuntimeHelperResultOwnership::ScalarCmpResult) {
        append_f128_runtime_helper_fact(
            helper, "f128_compare_helper_result_requires_scalar_cmp_ownership");
      } else if (helper.helper_family == PreparedF128RuntimeHelperFamily::Arithmetic &&
                 helper.result_ownership !=
                     PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier) {
        append_f128_runtime_helper_fact(
            helper, "f128_helper_result_requires_full_width_carrier_ownership");
      } else if (helper.helper_family == PreparedF128RuntimeHelperFamily::Cast &&
                 helper.result_type == bir::TypeKind::F128 &&
                 helper.result_ownership !=
                     PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier) {
        append_f128_runtime_helper_fact(
            helper, "f128_cast_helper_result_requires_full_width_carrier_ownership");
      } else if (helper.helper_family == PreparedF128RuntimeHelperFamily::Cast &&
                 helper.source_type == bir::TypeKind::F128 &&
                 helper.result_ownership !=
                     PreparedF128RuntimeHelperResultOwnership::ScalarValue) {
        append_f128_runtime_helper_fact(
            helper, "f128_cast_helper_result_requires_scalar_ownership");
      }
      populate_f128_runtime_helper_call_ownership(prepared,
                                                  frame_plan,
                                                  liveness_function,
                                                  regalloc_function,
                                                  value_locations,
                                                  helper);
      populate_f128_runtime_helper_ownership(helper);
    }
  }
}

void append_i128_runtime_helper_fact(PreparedI128RuntimeHelper& helper,
                                     std::string fact) {
  if (std::find(helper.missing_required_facts.begin(),
                helper.missing_required_facts.end(),
                fact) == helper.missing_required_facts.end()) {
    helper.missing_required_facts.push_back(std::move(fact));
  }
}

[[nodiscard]] PreparedI128RuntimeHelper::LaneBinding make_i128_helper_lane_binding(
    const PreparedI128Carrier& carrier,
    const PreparedI128LaneCarrier& lane) {
  return PreparedI128RuntimeHelper::LaneBinding{
      .value_id = carrier.value_id,
      .value_name = carrier.value_name,
      .carrier_kind = carrier.kind,
      .role = lane.role,
      .lane_index = lane.lane_index,
      .width_bytes = lane.width_bytes,
      .register_name = lane.register_name,
      .slot_id = lane.slot_id,
      .stack_offset_bytes = lane.stack_offset_bytes,
  };
}

[[nodiscard]] std::optional<std::string> i128_helper_abi_register_name(
    const c4c::TargetProfile& target_profile,
    bool result,
    std::size_t abi_register_index) {
  switch (target_profile.arch) {
    case c4c::TargetArch::X86_64:
      if (result) {
        constexpr std::array<std::string_view, 2> kResultRegisters = {"rax", "rdx"};
        if (abi_register_index < kResultRegisters.size()) {
          return std::string(kResultRegisters[abi_register_index]);
        }
        return std::nullopt;
      }
      break;
    case c4c::TargetArch::Aarch64:
    case c4c::TargetArch::Riscv64:
      if (result) {
        const bir::CallResultAbiInfo result_abi{
            .type = bir::TypeKind::I64,
            .primary_class = bir::AbiValueClass::Integer,
        };
        const auto low = call_result_destination_register_name(target_profile, result_abi);
        if (!low.has_value()) {
          return std::nullopt;
        }
        if (abi_register_index == 0) {
          return low;
        }
        if (abi_register_index == 1) {
          if (target_profile.arch == c4c::TargetArch::Aarch64) {
            return std::string("x1");
          }
          return std::string("a1");
        }
        return std::nullopt;
      }
      break;
    case c4c::TargetArch::I686:
    case c4c::TargetArch::Unknown:
      return std::nullopt;
  }

  const bir::CallArgAbiInfo arg_abi{
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
      .primary_class = bir::AbiValueClass::Integer,
      .passed_in_register = true,
  };
  return call_arg_destination_register_name(target_profile, arg_abi, abi_register_index);
}

[[nodiscard]] std::optional<PreparedI128RuntimeHelper::AbiRegisterBinding>
make_i128_helper_abi_register_binding(
    const c4c::TargetProfile& target_profile,
    PreparedValueId value_id,
    ValueNameId value_name,
    PreparedI128LaneRole role,
    std::size_t lane_index,
    std::optional<std::size_t> helper_argument_index,
    std::size_t abi_register_index) {
  const auto register_name =
      i128_helper_abi_register_name(target_profile,
                                    !helper_argument_index.has_value(),
                                    abi_register_index);
  if (!register_name.has_value()) {
    return std::nullopt;
  }
  return PreparedI128RuntimeHelper::AbiRegisterBinding{
      .value_id = value_id,
      .value_name = value_name,
      .role = role,
      .lane_index = lane_index,
      .width_bytes = 8,
      .helper_argument_index = helper_argument_index,
      .abi_register_index = abi_register_index,
      .register_bank = PreparedRegisterBank::Gpr,
      .register_class = PreparedRegisterClass::General,
      .register_name = *register_name,
      .contiguous_width = 1,
      .occupied_register_names = {*register_name},
      .register_placement =
          PreparedRegisterPlacement{
              .bank = PreparedRegisterBank::Gpr,
              .pool = helper_argument_index.has_value()
                          ? PreparedRegisterSlotPool::CallArgument
                          : PreparedRegisterSlotPool::CallResult,
              .slot_index = abi_register_index,
              .contiguous_width = 1,
          },
  };
}

[[nodiscard]] std::optional<PreparedI128RuntimeHelper::AbiRegisterBinding>
make_i128_helper_scalar_abi_register_binding(
    const c4c::TargetProfile& target_profile,
    const PreparedI128RuntimeHelper::ScalarValueOwnership& scalar,
    std::optional<std::size_t> helper_argument_index,
    std::size_t abi_register_index) {
  std::optional<std::string> register_name;
  std::optional<PreparedRegisterPlacement> placement;
  PreparedRegisterBank bank = PreparedRegisterBank::None;

  if (helper_argument_index.has_value()) {
    const auto arg_abi = infer_call_arg_abi(target_profile, scalar.type);
    if (!arg_abi.has_value() || !arg_abi->passed_in_register) {
      return std::nullopt;
    }
    register_name =
        call_arg_destination_register_name(target_profile, *arg_abi, abi_register_index);
    placement =
        call_arg_destination_register_placement(target_profile, *arg_abi, abi_register_index);
    bank = register_bank_from_arg_abi(*arg_abi);
  } else {
    const bir::CallResultAbiInfo result_abi{
        .type = scalar.type,
        .primary_class = scalar.register_bank == PreparedRegisterBank::Fpr
                             ? bir::AbiValueClass::Sse
                             : bir::AbiValueClass::Integer,
    };
    register_name = call_result_destination_register_name(target_profile, result_abi);
    placement = call_result_destination_register_placement(target_profile, result_abi);
    bank = register_bank_from_result_abi(result_abi);
  }

  if (!register_name.has_value() || register_name->empty() ||
      !placement.has_value() || bank == PreparedRegisterBank::None ||
      bank != scalar.register_bank) {
    return std::nullopt;
  }

  return PreparedI128RuntimeHelper::AbiRegisterBinding{
      .value_id = scalar.value_id,
      .value_name = scalar.value_name,
      .role = PreparedI128LaneRole::Low,
      .lane_index = 0,
      .width_bytes = scalar.width_bytes,
      .helper_argument_index = helper_argument_index,
      .abi_register_index = abi_register_index,
      .register_bank = bank,
      .register_class = register_class_from_bank(bank),
      .register_name = *register_name,
      .contiguous_width = 1,
      .occupied_register_names = {*register_name},
      .register_placement = placement,
  };
}

void populate_i128_helper_lanes_from_carrier(
    const PreparedI128CarrierFunction* function_carriers,
    PreparedI128RuntimeHelper& helper,
    PreparedValueId value_id,
    std::optional<PreparedI128RuntimeHelper::LaneBinding>& low,
    std::optional<PreparedI128RuntimeHelper::LaneBinding>& high,
    std::string_view fact_prefix) {
  low.reset();
  high.reset();
  if (function_carriers == nullptr) {
    append_i128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_missing_prepared_i128_carriers");
    return;
  }

  const auto* carrier = find_prepared_i128_carrier(*function_carriers, value_id);
  if (carrier == nullptr) {
    append_i128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_missing_prepared_i128_carrier");
    return;
  }

  low = make_i128_helper_lane_binding(*carrier, carrier->low_lane);
  high = make_i128_helper_lane_binding(*carrier, carrier->high_lane);

  if (carrier->kind != PreparedI128CarrierKind::RegisterPair) {
    append_i128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_requires_register_pair_carrier");
  }
  for (const auto& fact : carrier->missing_required_facts) {
    append_i128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_carrier_fact:" + fact);
  }
}

[[nodiscard]] std::optional<PreparedI128RuntimeHelper::ScalarValueOwnership>
make_i128_helper_scalar_ownership(
    PreparedI128RuntimeHelper& helper,
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueLocationFunction* value_locations,
    PreparedValueId value_id,
    ValueNameId value_name,
    bir::TypeKind type,
    std::size_t width_bytes,
    PreparedRegisterBank expected_bank,
    std::string_view fact_prefix) {
  if (value_name == kInvalidValueName) {
    append_i128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_value_identity");
    return std::nullopt;
  }
  if (value_locations == nullptr) {
    append_i128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_value_locations");
    return std::nullopt;
  }
  const auto* home = find_prepared_value_home(*value_locations, value_name);
  if (home == nullptr || home->value_id != value_id) {
    append_i128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_value_home");
    return std::nullopt;
  }
  const PreparedRegisterBank bank =
      published_bank_for_value(regalloc_function, value_name, type);
  if (bank != expected_bank) {
    append_i128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_expected_bank");
  }
  if (width_bytes == 0) {
    append_i128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_width");
  }
  if (home->kind != PreparedValueHomeKind::Register &&
      home->kind != PreparedValueHomeKind::StackSlot) {
    append_i128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_register_or_stack_home");
  }
  return PreparedI128RuntimeHelper::ScalarValueOwnership{
      .value_id = value_id,
      .value_name = value_name,
      .type = type,
      .width_bytes = width_bytes,
      .register_bank = bank,
      .home_kind = home->kind,
      .register_name = home->register_name,
      .slot_id = home->slot_id,
      .stack_offset_bytes = home->offset_bytes,
  };
}

void populate_i128_runtime_helper_abi_bindings(
    const c4c::TargetProfile& target_profile,
    PreparedI128RuntimeHelper& helper) {
  helper.lhs_low_abi_argument.reset();
  helper.lhs_high_abi_argument.reset();
  helper.rhs_low_abi_argument.reset();
  helper.rhs_high_abi_argument.reset();
  helper.result_low_abi_result.reset();
  helper.result_high_abi_result.reset();
  helper.scalar_operand_abi_argument.reset();
  helper.scalar_result_abi_result.reset();

  if (helper.callee_name.empty()) {
    append_i128_runtime_helper_fact(helper, "i128_helper_abi_bindings_require_callee_identity");
    return;
  }

  if (helper.helper_family == PreparedI128RuntimeHelperFamily::FloatIntegerConversion) {
    const bool operand_is_i128 = helper.source_type == bir::TypeKind::I128;
    const bool result_is_i128 = helper.result_type == bir::TypeKind::I128;
    if (operand_is_i128 == result_is_i128) {
      append_i128_runtime_helper_fact(
          helper, "i128_conversion_abi_bindings_require_one_i128_side");
      return;
    }

    if (operand_is_i128) {
      helper.lhs_low_abi_argument =
          make_i128_helper_abi_register_binding(target_profile,
                                                helper.operand_value_id,
                                                helper.operand_value_name,
                                                PreparedI128LaneRole::Low,
                                                0,
                                                std::size_t{0},
                                                0);
      helper.lhs_high_abi_argument =
          make_i128_helper_abi_register_binding(target_profile,
                                                helper.operand_value_id,
                                                helper.operand_value_name,
                                                PreparedI128LaneRole::High,
                                                1,
                                                std::size_t{0},
                                                1);
      if (!helper.scalar_result.has_value()) {
        append_i128_runtime_helper_fact(
            helper, "i128_conversion_abi_bindings_require_scalar_result_ownership");
      } else {
        helper.scalar_result_abi_result =
            make_i128_helper_scalar_abi_register_binding(
                target_profile, *helper.scalar_result, std::nullopt, 0);
      }
      helper.abi_policy = PreparedI128RuntimeHelper::AbiPolicy{
          .transition =
              PreparedI128RuntimeHelperAbiTransition::RegisterPairArgumentAndScalarResult,
          .argument_bank = PreparedRegisterBank::Gpr,
          .result_bank = PreparedRegisterBank::Fpr,
          .argument_count = 1,
          .lanes_per_argument = 2,
          .result_lane_count = 1,
          .lane_width_bytes = 8,
      };
      if (!helper.lhs_low_abi_argument.has_value() ||
          !helper.lhs_high_abi_argument.has_value() ||
          !helper.scalar_result_abi_result.has_value()) {
        append_i128_runtime_helper_fact(
            helper, "i128_conversion_abi_bindings_require_supported_target_registers");
      }
      return;
    }

    if (!helper.scalar_operand.has_value()) {
      append_i128_runtime_helper_fact(
          helper, "i128_conversion_abi_bindings_require_scalar_operand_ownership");
    } else {
      helper.scalar_operand_abi_argument =
          make_i128_helper_scalar_abi_register_binding(
              target_profile, *helper.scalar_operand, std::size_t{0}, 0);
    }
    helper.result_low_abi_result =
        make_i128_helper_abi_register_binding(target_profile,
                                              helper.result_value_id,
                                              helper.result_value_name,
                                              PreparedI128LaneRole::Low,
                                              0,
                                              std::nullopt,
                                              0);
    helper.result_high_abi_result =
        make_i128_helper_abi_register_binding(target_profile,
                                              helper.result_value_id,
                                              helper.result_value_name,
                                              PreparedI128LaneRole::High,
                                              1,
                                              std::nullopt,
                                              1);
    helper.abi_policy = PreparedI128RuntimeHelper::AbiPolicy{
        .transition =
            PreparedI128RuntimeHelperAbiTransition::ScalarArgumentAndRegisterPairResult,
        .argument_bank = PreparedRegisterBank::Fpr,
        .result_bank = PreparedRegisterBank::Gpr,
        .argument_count = 1,
        .lanes_per_argument = 1,
        .result_lane_count = 2,
        .lane_width_bytes = 8,
    };
    if (!helper.scalar_operand_abi_argument.has_value() ||
        !helper.result_low_abi_result.has_value() ||
        !helper.result_high_abi_result.has_value()) {
      append_i128_runtime_helper_fact(
          helper, "i128_conversion_abi_bindings_require_supported_target_registers");
    }
    return;
  }

  if (helper.helper_family != PreparedI128RuntimeHelperFamily::DivRem) {
    append_i128_runtime_helper_fact(helper, "i128_helper_abi_bindings_deferred_for_family");
    return;
  }
  if (helper.result_ownership !=
      PreparedI128RuntimeHelperResultOwnership::DirectLowHighLanes) {
    append_i128_runtime_helper_fact(
        helper, "i128_helper_abi_bindings_require_direct_low_high_result");
    return;
  }

  helper.lhs_low_abi_argument =
      make_i128_helper_abi_register_binding(target_profile,
                                            helper.lhs_value_id,
                                            helper.lhs_value_name,
                                            PreparedI128LaneRole::Low,
                                            0,
                                            std::size_t{0},
                                            0);
  helper.lhs_high_abi_argument =
      make_i128_helper_abi_register_binding(target_profile,
                                            helper.lhs_value_id,
                                            helper.lhs_value_name,
                                            PreparedI128LaneRole::High,
                                            1,
                                            std::size_t{0},
                                            1);
  helper.rhs_low_abi_argument =
      make_i128_helper_abi_register_binding(target_profile,
                                            helper.rhs_value_id,
                                            helper.rhs_value_name,
                                            PreparedI128LaneRole::Low,
                                            0,
                                            std::size_t{1},
                                            2);
  helper.rhs_high_abi_argument =
      make_i128_helper_abi_register_binding(target_profile,
                                            helper.rhs_value_id,
                                            helper.rhs_value_name,
                                            PreparedI128LaneRole::High,
                                            1,
                                            std::size_t{1},
                                            3);
  helper.result_low_abi_result =
      make_i128_helper_abi_register_binding(target_profile,
                                            helper.result_value_id,
                                            helper.result_value_name,
                                            PreparedI128LaneRole::Low,
                                            0,
                                            std::nullopt,
                                            0);
  helper.result_high_abi_result =
      make_i128_helper_abi_register_binding(target_profile,
                                            helper.result_value_id,
                                            helper.result_value_name,
                                            PreparedI128LaneRole::High,
                                            1,
                                            std::nullopt,
                                            1);

  if (!helper.lhs_low_abi_argument.has_value() || !helper.lhs_high_abi_argument.has_value() ||
      !helper.rhs_low_abi_argument.has_value() || !helper.rhs_high_abi_argument.has_value() ||
      !helper.result_low_abi_result.has_value() ||
      !helper.result_high_abi_result.has_value()) {
    append_i128_runtime_helper_fact(
        helper, "i128_helper_abi_bindings_require_supported_target_registers");
  }
}

[[nodiscard]] std::optional<PreparedI128RuntimeHelper::MarshalingMove>
make_i128_helper_marshaling_move(
    const PreparedI128RuntimeHelper::LaneBinding& carrier_lane,
    const PreparedI128RuntimeHelper::AbiRegisterBinding& abi_register,
    PreparedI128RuntimeHelperMarshalDirection direction) {
  return PreparedI128RuntimeHelper::MarshalingMove{
      .direction = direction,
      .phase = direction ==
                       PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument
                   ? PreparedMovePhase::BeforeCall
                   : PreparedMovePhase::AfterCall,
      .op_kind = PreparedMoveResolutionOpKind::Move,
      .carrier_lane = carrier_lane,
      .abi_register = abi_register,
  };
}

[[nodiscard]] std::optional<PreparedI128RuntimeHelper::ScalarMarshalingMove>
make_i128_helper_scalar_marshaling_move(
    const PreparedI128RuntimeHelper::ScalarValueOwnership& scalar_value,
    const PreparedI128RuntimeHelper::AbiRegisterBinding& abi_register,
    PreparedI128RuntimeHelperMarshalDirection direction) {
  return PreparedI128RuntimeHelper::ScalarMarshalingMove{
      .direction = direction,
      .phase = direction ==
                       PreparedI128RuntimeHelperMarshalDirection::ScalarValueToAbiArgument
                   ? PreparedMovePhase::BeforeCall
                   : PreparedMovePhase::AfterCall,
      .op_kind = PreparedMoveResolutionOpKind::Move,
      .scalar_value = scalar_value,
      .abi_register = abi_register,
  };
}

[[nodiscard]] std::vector<PreparedCallPreservedValue> build_call_preserved_values(
    const PreparedBirModule& prepared,
    const PreparedFramePlanFunction* frame_plan,
    const PreparedLivenessFunction* liveness_function,
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueLocationFunction* value_locations,
    std::size_t program_point,
    bool require_call_crossing_value);

[[nodiscard]] bool preserved_value_has_complete_route(
    const PreparedCallPreservedValue& preserved);

void populate_i128_runtime_helper_marshaling(
    PreparedI128RuntimeHelper& helper) {
  helper.lhs_low_argument_move.reset();
  helper.lhs_high_argument_move.reset();
  helper.rhs_low_argument_move.reset();
  helper.rhs_high_argument_move.reset();
  helper.result_low_unmarshal_move.reset();
  helper.result_high_unmarshal_move.reset();
  helper.scalar_operand_argument_move.reset();
  helper.scalar_result_unmarshal_move.reset();

  auto bind_move =
      [&](std::string_view fact_prefix,
          const std::optional<PreparedI128RuntimeHelper::LaneBinding>& carrier_lane,
          const std::optional<PreparedI128RuntimeHelper::AbiRegisterBinding>& abi_register,
          PreparedI128RuntimeHelperMarshalDirection direction,
          std::optional<PreparedI128RuntimeHelper::MarshalingMove>& output) {
        output.reset();
        if (!carrier_lane.has_value()) {
          append_i128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_carrier_lane");
          return;
        }
        if (!abi_register.has_value()) {
          append_i128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_abi_binding");
          return;
        }
        const bool has_register_lane = carrier_lane->register_name.has_value();
        const bool has_memory_lane =
            carrier_lane->slot_id.has_value() &&
            carrier_lane->stack_offset_bytes.has_value();
        if (carrier_lane->carrier_kind == PreparedI128CarrierKind::Missing ||
            (!has_register_lane && !has_memory_lane)) {
          append_i128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_complete_carrier_lane");
          return;
        }
        if (abi_register->register_bank != PreparedRegisterBank::Gpr ||
            abi_register->register_class != PreparedRegisterClass::General ||
            abi_register->register_name.empty() ||
            abi_register->occupied_register_names.empty() ||
            abi_register->contiguous_width == 0 ||
            !abi_register->register_placement.has_value()) {
          append_i128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_register_binding");
          return;
        }
        if (carrier_lane->value_id != abi_register->value_id ||
            carrier_lane->value_name != abi_register->value_name ||
            carrier_lane->role != abi_register->role ||
            carrier_lane->lane_index != abi_register->lane_index ||
            carrier_lane->width_bytes != abi_register->width_bytes) {
          append_i128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_matching_lane_identity");
          return;
        }
        output = make_i128_helper_marshaling_move(*carrier_lane, *abi_register, direction);
      };

  auto bind_scalar_move =
      [&](std::string_view fact_prefix,
          const std::optional<PreparedI128RuntimeHelper::ScalarValueOwnership>& scalar_value,
          const std::optional<PreparedI128RuntimeHelper::AbiRegisterBinding>& abi_register,
          PreparedI128RuntimeHelperMarshalDirection direction,
          std::optional<PreparedI128RuntimeHelper::ScalarMarshalingMove>& output) {
        output.reset();
        if (!scalar_value.has_value()) {
          append_i128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_scalar_ownership");
          return;
        }
        if (!abi_register.has_value()) {
          append_i128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_abi_binding");
          return;
        }
        const bool has_register_home = scalar_value->register_name.has_value();
        const bool has_memory_home =
            scalar_value->slot_id.has_value() &&
            scalar_value->stack_offset_bytes.has_value();
        if (scalar_value->home_kind == PreparedValueHomeKind::None ||
            (!has_register_home && !has_memory_home)) {
          append_i128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_complete_scalar_home");
          return;
        }
        if (abi_register->register_bank != scalar_value->register_bank ||
            abi_register->register_class == PreparedRegisterClass::None ||
            abi_register->register_name.empty() ||
            abi_register->occupied_register_names.empty() ||
            abi_register->contiguous_width == 0 ||
            !abi_register->register_placement.has_value()) {
          append_i128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_register_binding");
          return;
        }
        if (scalar_value->value_id != abi_register->value_id ||
            scalar_value->value_name != abi_register->value_name ||
            scalar_value->width_bytes != abi_register->width_bytes) {
          append_i128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_matching_scalar_identity");
          return;
        }
        output =
            make_i128_helper_scalar_marshaling_move(*scalar_value, *abi_register, direction);
      };

  if (helper.helper_family == PreparedI128RuntimeHelperFamily::FloatIntegerConversion) {
    const bool operand_is_i128 = helper.source_type == bir::TypeKind::I128;
    const bool result_is_i128 = helper.result_type == bir::TypeKind::I128;
    if (operand_is_i128) {
      bind_move("operand_low",
                helper.lhs_low_lane,
                helper.lhs_low_abi_argument,
                PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument,
                helper.lhs_low_argument_move);
      bind_move("operand_high",
                helper.lhs_high_lane,
                helper.lhs_high_abi_argument,
                PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument,
                helper.lhs_high_argument_move);
    } else {
      bind_scalar_move("operand",
                       helper.scalar_operand,
                       helper.scalar_operand_abi_argument,
                       PreparedI128RuntimeHelperMarshalDirection::ScalarValueToAbiArgument,
                       helper.scalar_operand_argument_move);
    }
    if (result_is_i128) {
      bind_move("result_low",
                helper.result_low_lane,
                helper.result_low_abi_result,
                PreparedI128RuntimeHelperMarshalDirection::AbiResultToCarrierLane,
                helper.result_low_unmarshal_move);
      bind_move("result_high",
                helper.result_high_lane,
                helper.result_high_abi_result,
                PreparedI128RuntimeHelperMarshalDirection::AbiResultToCarrierLane,
                helper.result_high_unmarshal_move);
    } else {
      bind_scalar_move("result",
                       helper.scalar_result,
                       helper.scalar_result_abi_result,
                       PreparedI128RuntimeHelperMarshalDirection::AbiResultToScalarValue,
                       helper.scalar_result_unmarshal_move);
    }
    return;
  }

  bind_move("lhs_low",
            helper.lhs_low_lane,
            helper.lhs_low_abi_argument,
            PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument,
            helper.lhs_low_argument_move);
  bind_move("lhs_high",
            helper.lhs_high_lane,
            helper.lhs_high_abi_argument,
            PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument,
            helper.lhs_high_argument_move);
  bind_move("rhs_low",
            helper.rhs_low_lane,
            helper.rhs_low_abi_argument,
            PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument,
            helper.rhs_low_argument_move);
  bind_move("rhs_high",
            helper.rhs_high_lane,
            helper.rhs_high_abi_argument,
            PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument,
            helper.rhs_high_argument_move);
  bind_move("result_low",
            helper.result_low_lane,
            helper.result_low_abi_result,
            PreparedI128RuntimeHelperMarshalDirection::AbiResultToCarrierLane,
            helper.result_low_unmarshal_move);
  bind_move("result_high",
            helper.result_high_lane,
            helper.result_high_abi_result,
            PreparedI128RuntimeHelperMarshalDirection::AbiResultToCarrierLane,
            helper.result_high_unmarshal_move);
}

void populate_i128_runtime_helper_call_ownership(
    const PreparedBirModule& prepared,
    const PreparedFramePlanFunction* frame_plan,
    const PreparedLivenessFunction* liveness_function,
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueLocationFunction* value_locations,
    PreparedI128RuntimeHelper& helper) {
  helper.live_preservation_policy = PreparedI128RuntimeHelper::LivePreservationPolicy{};
  helper.selected_call_ownership = PreparedI128RuntimeHelper::SelectedCallOwnershipPolicy{};

  const bool has_clobber_policy =
      helper.resource_policy.caller_saved_clobbers && !helper.clobbered_registers.empty();
  const auto helper_point =
      liveness_function == nullptr
          ? std::optional<std::size_t>{}
          : find_call_program_point(*liveness_function,
                                    helper.block_index,
                                    helper.instruction_index);
  std::vector<PreparedCallPreservedValue> preserved_values =
      helper_point.has_value()
          ? build_call_preserved_values(prepared,
                                        frame_plan,
                                        liveness_function,
                                        regalloc_function,
                                        value_locations,
                                        *helper_point,
                                        false)
          : std::vector<PreparedCallPreservedValue>{};
  const bool preserved_values_complete =
      std::all_of(preserved_values.begin(),
                  preserved_values.end(),
                  preserved_value_has_complete_route);
  if (liveness_function == nullptr || regalloc_function == nullptr ||
      !helper_point.has_value()) {
    append_i128_runtime_helper_fact(
        helper, "live_preservation_requires_structured_live_across_helper_facts");
  } else if (!preserved_values_complete) {
    append_i128_runtime_helper_fact(
        helper, "live_preservation_requires_complete_preserved_value_routes");
  }
  helper.live_preservation_policy = PreparedI128RuntimeHelper::LivePreservationPolicy{
      .evaluated = true,
      .caller_saved_clobbers_modeled = has_clobber_policy,
      .no_additional_live_preservation_required =
          helper_point.has_value() && preserved_values_complete,
      .preserved_values = std::move(preserved_values),
  };

  const bool has_resource_policy =
      helper.resource_policy.call_boundary &&
      helper.resource_policy.runtime_helper_callee &&
      helper.resource_policy.caller_saved_clobbers &&
      helper.resource_policy.preserves_source_operation_identity;
  bool has_abi_bindings = false;
  bool has_marshaling = false;
  if (helper.helper_family == PreparedI128RuntimeHelperFamily::FloatIntegerConversion) {
    const bool operand_is_i128 = helper.source_type == bir::TypeKind::I128;
    const bool result_is_i128 = helper.result_type == bir::TypeKind::I128;
    if (operand_is_i128 && !result_is_i128) {
      has_abi_bindings =
          helper.lhs_low_abi_argument.has_value() &&
          helper.lhs_high_abi_argument.has_value() &&
          helper.scalar_result_abi_result.has_value();
      has_marshaling =
          helper.lhs_low_argument_move.has_value() &&
          helper.lhs_high_argument_move.has_value() &&
          helper.scalar_result_unmarshal_move.has_value();
    } else if (!operand_is_i128 && result_is_i128) {
      has_abi_bindings =
          helper.scalar_operand_abi_argument.has_value() &&
          helper.result_low_abi_result.has_value() &&
          helper.result_high_abi_result.has_value();
      has_marshaling =
          helper.scalar_operand_argument_move.has_value() &&
          helper.result_low_unmarshal_move.has_value() &&
          helper.result_high_unmarshal_move.has_value();
    }
  } else if (helper.helper_family == PreparedI128RuntimeHelperFamily::DivRem) {
    has_abi_bindings =
        helper.lhs_low_abi_argument.has_value() &&
        helper.lhs_high_abi_argument.has_value() &&
        helper.rhs_low_abi_argument.has_value() &&
        helper.rhs_high_abi_argument.has_value() &&
        helper.result_low_abi_result.has_value() &&
        helper.result_high_abi_result.has_value();
    has_marshaling =
        helper.lhs_low_argument_move.has_value() &&
        helper.lhs_high_argument_move.has_value() &&
        helper.rhs_low_argument_move.has_value() &&
        helper.rhs_high_argument_move.has_value() &&
        helper.result_low_unmarshal_move.has_value() &&
        helper.result_high_unmarshal_move.has_value();
  }
  const bool has_live_preservation =
      helper.live_preservation_policy.evaluated &&
      helper.live_preservation_policy.caller_saved_clobbers_modeled &&
      helper.live_preservation_policy.no_additional_live_preservation_required;

  helper.selected_call_ownership =
      PreparedI128RuntimeHelper::SelectedCallOwnershipPolicy{
          .owns_terminal_call =
              !helper.callee_name.empty() && has_resource_policy &&
              has_clobber_policy && has_abi_bindings && has_marshaling &&
              has_live_preservation,
          .has_callee_identity = !helper.callee_name.empty(),
          .has_resource_policy = has_resource_policy,
          .has_clobber_policy = has_clobber_policy,
          .has_abi_bindings = has_abi_bindings,
          .has_marshaling = has_marshaling,
          .has_live_preservation = has_live_preservation,
      };

  if (!helper.selected_call_ownership.has_callee_identity) {
    append_i128_runtime_helper_fact(helper, "selected_call_ownership_requires_callee_identity");
  }
  if (!helper.selected_call_ownership.has_resource_policy) {
    append_i128_runtime_helper_fact(helper, "selected_call_ownership_requires_resource_policy");
  }
  if (!helper.selected_call_ownership.has_clobber_policy) {
    append_i128_runtime_helper_fact(helper, "selected_call_ownership_requires_clobber_policy");
  }
  if (!helper.selected_call_ownership.has_abi_bindings) {
    append_i128_runtime_helper_fact(helper, "selected_call_ownership_requires_abi_bindings");
  }
  if (!helper.selected_call_ownership.has_marshaling) {
    append_i128_runtime_helper_fact(helper, "selected_call_ownership_requires_marshaling");
  }
  if (!helper.selected_call_ownership.has_live_preservation) {
    append_i128_runtime_helper_fact(
        helper, "selected_call_ownership_requires_live_preservation_policy");
  }
}

void populate_i128_runtime_helper_boundary_policy(
    const c4c::TargetProfile& target_profile,
    const PreparedRegallocFunction* regalloc_function,
    PreparedI128RuntimeHelper& helper) {
  helper.resource_policy = PreparedI128RuntimeHelper::ResourcePolicy{};
  helper.abi_policy = PreparedI128RuntimeHelper::AbiPolicy{};
  helper.clobbered_registers.clear();

  const bool supported_family =
      helper.helper_family == PreparedI128RuntimeHelperFamily::DivRem ||
      helper.helper_family == PreparedI128RuntimeHelperFamily::FloatIntegerConversion;
  if (!supported_family) {
    append_i128_runtime_helper_fact(helper, "i128_helper_boundary_policy_deferred_for_family");
    return;
  }

  helper.resource_policy = PreparedI128RuntimeHelper::ResourcePolicy{
      .call_boundary = true,
      .runtime_helper_callee = true,
      .caller_saved_clobbers = true,
      .preserves_source_operation_identity = true,
  };
  helper.clobbered_registers = build_call_clobber_set(target_profile, regalloc_function);

  if (helper.helper_family == PreparedI128RuntimeHelperFamily::DivRem) {
    helper.abi_policy = PreparedI128RuntimeHelper::AbiPolicy{
        .transition = PreparedI128RuntimeHelperAbiTransition::DirectRegisterPairArgumentsAndResult,
        .argument_bank = PreparedRegisterBank::Gpr,
        .result_bank = PreparedRegisterBank::Gpr,
        .argument_count = 2,
        .lanes_per_argument = 2,
        .result_lane_count = 2,
        .lane_width_bytes = 8,
    };
  }

  if (helper.callee_name.empty()) {
    append_i128_runtime_helper_fact(helper, "i128_helper_boundary_requires_callee_identity");
  }
  if (helper.helper_family == PreparedI128RuntimeHelperFamily::DivRem &&
      (helper.abi_policy.argument_bank == PreparedRegisterBank::None ||
       helper.abi_policy.result_bank == PreparedRegisterBank::None ||
       helper.abi_policy.argument_count != 2 ||
       helper.abi_policy.lanes_per_argument != 2 ||
       helper.abi_policy.result_lane_count != 2 ||
       helper.abi_policy.lane_width_bytes != 8)) {
    append_i128_runtime_helper_fact(helper, "i128_helper_boundary_requires_direct_gpr_pair_abi");
  }
  if (!helper.resource_policy.call_boundary ||
      !helper.resource_policy.runtime_helper_callee ||
      !helper.resource_policy.caller_saved_clobbers ||
      !helper.resource_policy.preserves_source_operation_identity) {
    append_i128_runtime_helper_fact(helper, "i128_helper_boundary_requires_resource_policy");
  }
  if (helper.clobbered_registers.empty()) {
    append_i128_runtime_helper_fact(helper, "i128_helper_boundary_requires_caller_saved_clobbers");
  }
}

void populate_i128_runtime_helper_lanes(PreparedBirModule& prepared) {
  for (auto& function_helpers : prepared.i128_runtime_helpers.functions) {
    const auto* function_carriers =
        find_prepared_i128_carriers(prepared.i128_carriers, function_helpers.function_name);
    const auto* regalloc_function =
        find_regalloc_function(prepared.regalloc, function_helpers.function_name);
    const auto* frame_plan = find_prepared_frame_plan(prepared, function_helpers.function_name);
    const auto* liveness_function =
        find_liveness_function(prepared.liveness, function_helpers.function_name);
    const auto* value_locations =
        find_prepared_value_location_function(prepared, function_helpers.function_name);
    for (auto& helper : function_helpers.helpers) {
      helper.lhs_low_lane.reset();
      helper.lhs_high_lane.reset();
      helper.rhs_low_lane.reset();
      helper.rhs_high_lane.reset();
      helper.result_low_lane.reset();
      helper.result_high_lane.reset();
      helper.lhs_low_argument_move.reset();
      helper.lhs_high_argument_move.reset();
      helper.rhs_low_argument_move.reset();
      helper.rhs_high_argument_move.reset();
      helper.result_low_unmarshal_move.reset();
      helper.result_high_unmarshal_move.reset();
      helper.scalar_operand_argument_move.reset();
      helper.scalar_result_unmarshal_move.reset();
      helper.memory_return.reset();
      helper.scalar_operand.reset();
      helper.scalar_result.reset();
      helper.missing_required_facts.clear();
      populate_i128_runtime_helper_boundary_policy(
          prepared.target_profile, regalloc_function, helper);

      if (helper.helper_family == PreparedI128RuntimeHelperFamily::DivRem) {
        populate_i128_helper_lanes_from_carrier(function_carriers,
                                               helper,
                                               helper.lhs_value_id,
                                               helper.lhs_low_lane,
                                               helper.lhs_high_lane,
                                               "lhs");
        populate_i128_helper_lanes_from_carrier(function_carriers,
                                               helper,
                                               helper.rhs_value_id,
                                               helper.rhs_low_lane,
                                               helper.rhs_high_lane,
                                               "rhs");
        populate_i128_helper_lanes_from_carrier(function_carriers,
                                               helper,
                                               helper.result_value_id,
                                               helper.result_low_lane,
                                               helper.result_high_lane,
                                               "result");
      } else if (helper.helper_family ==
                 PreparedI128RuntimeHelperFamily::FloatIntegerConversion) {
        const bool operand_is_i128 = helper.source_type == bir::TypeKind::I128;
        const bool result_is_i128 = helper.result_type == bir::TypeKind::I128;
        if (operand_is_i128) {
          populate_i128_helper_lanes_from_carrier(function_carriers,
                                                 helper,
                                                 helper.operand_value_id,
                                                 helper.lhs_low_lane,
                                                 helper.lhs_high_lane,
                                                 "operand");
        } else {
          helper.scalar_operand =
              make_i128_helper_scalar_ownership(helper,
                                                regalloc_function,
                                                value_locations,
                                                helper.operand_value_id,
                                                helper.operand_value_name,
                                                helper.source_type,
                                                helper.source_width_bytes,
                                                PreparedRegisterBank::Fpr,
                                                "operand");
        }
        if (result_is_i128) {
          populate_i128_helper_lanes_from_carrier(function_carriers,
                                                 helper,
                                                 helper.result_value_id,
                                                 helper.result_low_lane,
                                                 helper.result_high_lane,
                                                 "result");
        } else {
          helper.scalar_result =
              make_i128_helper_scalar_ownership(helper,
                                                regalloc_function,
                                                value_locations,
                                                helper.result_value_id,
                                                helper.result_value_name,
                                                helper.result_type,
                                                helper.result_width_bytes,
                                                PreparedRegisterBank::Fpr,
                                                "result");
        }
      }
      populate_i128_runtime_helper_abi_bindings(prepared.target_profile, helper);
      switch (helper.result_ownership) {
        case PreparedI128RuntimeHelperResultOwnership::DirectLowHighLanes:
          if (!helper.result_low_lane.has_value() || !helper.result_high_lane.has_value()) {
            append_i128_runtime_helper_fact(
                helper, "direct_result_requires_low_high_result_lanes");
          }
          break;
        case PreparedI128RuntimeHelperResultOwnership::ScalarValue:
          if (!helper.scalar_result.has_value()) {
            append_i128_runtime_helper_fact(
                helper, "scalar_result_requires_scalar_ownership");
          }
          break;
        case PreparedI128RuntimeHelperResultOwnership::MemoryReturn:
          append_i128_runtime_helper_fact(
              helper, "memory_return_ownership_requires_explicit_destination");
          break;
        case PreparedI128RuntimeHelperResultOwnership::Missing:
          append_i128_runtime_helper_fact(
              helper, "missing_i128_helper_result_ownership_policy");
          break;
      }
      populate_i128_runtime_helper_marshaling(helper);
      populate_i128_runtime_helper_call_ownership(prepared,
                                                  frame_plan,
                                                  liveness_function,
                                                  regalloc_function,
                                                  value_locations,
                                                  helper);
    }
  }
}

[[nodiscard]] std::vector<PreparedClobberedRegister> build_call_clobber_set(
    const c4c::TargetProfile& target_profile,
    const PreparedRegallocFunction* regalloc_function) {
  std::vector<PreparedClobberedRegister> clobbers;
  auto append_spans_for_width = [&](PreparedRegisterClass reg_class, std::size_t contiguous_width) {
    const PreparedRegisterBank bank = register_bank_from_class(reg_class);
    for (const auto& register_span :
         caller_saved_register_spans(target_profile, reg_class, contiguous_width)) {
      const auto duplicate = std::find_if(
          clobbers.begin(),
          clobbers.end(),
          [&](const PreparedClobberedRegister& clobber) {
            return clobber.bank == bank &&
                   clobber.occupied_register_names == register_span.occupied_register_names;
          });
      if (duplicate != clobbers.end()) {
        continue;
      }
      clobbers.push_back(PreparedClobberedRegister{
          .bank = bank,
          .register_name = register_span.register_name,
          .contiguous_width = register_span.contiguous_width,
          .occupied_register_names = register_span.occupied_register_names,
          .placement = as_reserved_scratch_placement(register_span.placement),
      });
    }
  };
  for (PreparedRegisterClass reg_class :
       {PreparedRegisterClass::General, PreparedRegisterClass::Float, PreparedRegisterClass::Vector}) {
    append_spans_for_width(reg_class, 1);
    if (regalloc_function == nullptr) {
      continue;
    }
    for (const auto& value : regalloc_function->values) {
      if (value.register_class != reg_class || value.register_group_width <= 1) {
        continue;
      }
      append_spans_for_width(reg_class, value.register_group_width);
    }
  }
  return clobbers;
}

[[nodiscard]] bool is_callee_saved_register_assignment(const c4c::TargetProfile& target_profile,
                                                       const PreparedRegallocValue& value) {
  if (!value.assigned_register.has_value()) {
    return false;
  }
  for (const auto& span :
       callee_saved_register_spans(target_profile, value.register_class, value.register_group_width)) {
    if (span.occupied_register_names == value.assigned_register->occupied_register_names) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::optional<std::size_t> find_saved_callee_save_index(
    const PreparedFramePlanFunction* frame_plan,
    const PreparedRegallocValue& value) {
  if (frame_plan == nullptr || !value.assigned_register.has_value()) {
    return std::nullopt;
  }
  for (const auto& saved : frame_plan->saved_callee_registers) {
    if (saved.occupied_register_names == value.assigned_register->occupied_register_names &&
        !saved.occupied_register_names.empty()) {
      return saved.save_index;
    }
    if (saved.register_name == value.assigned_register->register_name) {
      return saved.save_index;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::size_t> callee_saved_span_save_index(
    const c4c::TargetProfile& target_profile,
    const PreparedRegallocValue& value) {
  if (!value.assigned_register.has_value()) {
    return std::nullopt;
  }
  const auto callee_saved =
      callee_saved_register_spans(target_profile, value.register_class, value.register_group_width);
  for (std::size_t save_index = 0; save_index < callee_saved.size(); ++save_index) {
    if (callee_saved[save_index].occupied_register_names ==
        value.assigned_register->occupied_register_names) {
      return save_index;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::vector<PreparedCallPreservedValue> build_call_preserved_values(
    const PreparedBirModule& prepared,
    const PreparedFramePlanFunction* frame_plan,
    const PreparedLivenessFunction* liveness_function,
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueLocationFunction* value_locations,
    std::size_t program_point,
    bool require_call_crossing_value) {
  std::vector<PreparedCallPreservedValue> preserved_values;
  if (liveness_function == nullptr || regalloc_function == nullptr) {
    return preserved_values;
  }

  for (const auto& value : regalloc_function->values) {
    if ((require_call_crossing_value && !value.crosses_call) ||
        !value.live_interval.has_value()) {
      continue;
    }
    if (!(program_point > value.live_interval->start_point &&
          program_point < value.live_interval->end_point)) {
      continue;
    }

    PreparedCallPreservedValue preserved{
        .value_id = value.value_id,
        .value_name = value.value_name,
        .route = PreparedCallPreservationRoute::Unknown,
        .callee_saved_save_index = std::nullopt,
        .contiguous_width = std::max<std::size_t>(value.register_group_width, 1),
        .register_name = std::nullopt,
        .register_bank = std::nullopt,
        .occupied_register_names = {},
        .slot_id = std::nullopt,
        .stack_offset_bytes = std::nullopt,
        .stack_size_bytes = std::nullopt,
        .stack_align_bytes = std::nullopt,
        .register_placement = std::nullopt,
        .spill_slot_placement = std::nullopt,
    };

    const PreparedValueHome* value_home =
        value_locations == nullptr ? nullptr : find_prepared_value_home(*value_locations, value.value_id);
    if (value_home != nullptr && value_home->kind == PreparedValueHomeKind::StackSlot) {
      preserved.route = PreparedCallPreservationRoute::StackSlot;
      preserved.slot_id = value_home->slot_id;
      preserved.stack_offset_bytes = value_home->offset_bytes;
      preserved.stack_size_bytes = value_home->size_bytes;
      preserved.stack_align_bytes = value_home->align_bytes;
      preserved.spill_slot_placement =
          make_spill_slot_placement(value_home->slot_id, value_home->offset_bytes);
    } else if (value.stack_object_id.has_value()) {
      if (const auto* frame_slot =
              find_prepared_frame_slot(prepared.stack_layout, *value.stack_object_id);
          frame_slot != nullptr) {
        preserved.route = PreparedCallPreservationRoute::StackSlot;
        preserved.slot_id = frame_slot->slot_id;
        preserved.stack_offset_bytes = frame_slot->offset_bytes;
        preserved.stack_size_bytes = frame_slot->size_bytes;
        preserved.stack_align_bytes = frame_slot->align_bytes;
        preserved.spill_slot_placement = PreparedSpillSlotPlacement{
            .slot_id = frame_slot->slot_id,
            .offset_bytes = frame_slot->offset_bytes,
        };
      }
    } else if (value.assigned_stack_slot.has_value()) {
      preserved.route = PreparedCallPreservationRoute::StackSlot;
      preserved.slot_id = value.assigned_stack_slot->slot_id;
      preserved.stack_offset_bytes = value.assigned_stack_slot->offset_bytes;
      preserved.stack_size_bytes = value.assigned_stack_slot->size_bytes;
      preserved.stack_align_bytes = value.assigned_stack_slot->align_bytes;
      preserved.spill_slot_placement =
          make_spill_slot_placement(preserved.slot_id, preserved.stack_offset_bytes);
    } else if (value.assigned_register.has_value()) {
      preserved.register_name = value.assigned_register->register_name;
      preserved.register_bank = register_bank_from_class(value.register_class);
      preserved.contiguous_width = value.assigned_register->contiguous_width;
      preserved.occupied_register_names = value.assigned_register->occupied_register_names;
      preserved.register_placement =
          assignment_register_placement(prepared.target_profile, *value.assigned_register);
      if (is_callee_saved_register_assignment(prepared.target_profile, value)) {
        preserved.route = PreparedCallPreservationRoute::CalleeSavedRegister;
        preserved.callee_saved_save_index = find_saved_callee_save_index(frame_plan, value);
      }
    }

    preserved_values.push_back(std::move(preserved));
  }

  std::sort(preserved_values.begin(),
            preserved_values.end(),
            [](const PreparedCallPreservedValue& lhs, const PreparedCallPreservedValue& rhs) {
              return lhs.value_id < rhs.value_id;
            });
  return preserved_values;
}

[[nodiscard]] std::vector<PreparedCallPreservedValue> build_call_preserved_values(
    const PreparedBirModule& prepared,
    const PreparedFramePlanFunction* frame_plan,
    const PreparedLivenessFunction* liveness_function,
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueLocationFunction* value_locations,
    std::size_t block_index,
    std::size_t instruction_index) {
  if (liveness_function == nullptr) {
    return {};
  }
  const auto call_point =
      find_call_program_point(*liveness_function, block_index, instruction_index);
  if (!call_point.has_value()) {
    return {};
  }
  return build_call_preserved_values(prepared,
                                     frame_plan,
                                     liveness_function,
                                     regalloc_function,
                                     value_locations,
                                     *call_point,
                                     true);
}

[[nodiscard]] bool preserved_value_has_complete_route(
    const PreparedCallPreservedValue& preserved) {
  switch (preserved.route) {
    case PreparedCallPreservationRoute::Unknown:
      return false;
    case PreparedCallPreservationRoute::CalleeSavedRegister:
      return preserved.register_name.has_value() &&
             preserved.register_bank.has_value() &&
             !preserved.occupied_register_names.empty() &&
             preserved.register_placement.has_value() &&
             preserved.callee_saved_save_index.has_value();
    case PreparedCallPreservationRoute::StackSlot:
      return preserved.slot_id.has_value() &&
             preserved.stack_offset_bytes.has_value() &&
             preserved.stack_size_bytes.has_value() &&
             *preserved.stack_size_bytes > 0 &&
             preserved.stack_align_bytes.has_value() &&
             *preserved.stack_align_bytes > 0 &&
             preserved.spill_slot_placement.has_value();
  }
  return false;
}

void populate_frame_plan(PreparedBirModule& prepared) {
  prepared.frame_plan.functions.clear();

  for (const auto& function : prepared.module.functions) {
    if (function.is_declaration) {
      continue;
    }
    const FunctionNameId function_name_id = prepared.names.function_names.find(function.name);
    if (function_name_id == kInvalidFunctionName) {
      continue;
    }

    PreparedFramePlanFunction plan{
        .function_name = function_name_id,
        .frame_size_bytes = 0,
        .frame_alignment_bytes = 1,
        .saved_callee_registers = {},
        .frame_slot_order = {},
        .has_dynamic_stack = false,
        .uses_frame_pointer_for_fixed_slots = false,
    };

    if (const auto* function_addressing =
            find_prepared_addressing_function(prepared.addressing, function_name_id);
        function_addressing != nullptr) {
      plan.frame_size_bytes = function_addressing->frame_size_bytes;
      plan.frame_alignment_bytes = function_addressing->frame_alignment_bytes;
    }

    for (const auto& slot : prepared.stack_layout.frame_slots) {
      if (slot.function_name == function_name_id) {
        plan.frame_slot_order.push_back(slot.slot_id);
        plan.frame_size_bytes =
            std::max(plan.frame_size_bytes, slot.offset_bytes + slot.size_bytes);
        plan.frame_alignment_bytes =
            std::max(plan.frame_alignment_bytes, slot.align_bytes);
      }
    }

    std::vector<PreparedSavedRegister> saved_registers;
    if (const auto* regalloc_function = find_regalloc_function(prepared.regalloc, function_name_id);
        regalloc_function != nullptr) {
      for (const auto& value : regalloc_function->values) {
        if (!value.assigned_register.has_value() ||
            !is_callee_saved_register_assignment(prepared.target_profile, value)) {
          continue;
        }
        const auto save_index = callee_saved_span_save_index(prepared.target_profile, value);
        if (!save_index.has_value()) {
          continue;
        }
        const auto already_published =
            std::find_if(saved_registers.begin(),
                         saved_registers.end(),
                         [&](const PreparedSavedRegister& saved) {
                           return saved.bank == register_bank_from_class(value.register_class) &&
                                  saved.occupied_register_names ==
                                      value.assigned_register->occupied_register_names;
                         });
        if (already_published != saved_registers.end()) {
          continue;
        }
        saved_registers.push_back(PreparedSavedRegister{
            .bank = register_bank_from_class(value.register_class),
            .register_name = value.assigned_register->register_name,
            .contiguous_width = value.assigned_register->contiguous_width,
            .occupied_register_names = value.assigned_register->occupied_register_names,
            .save_index = *save_index,
            .placement = assignment_register_placement(prepared.target_profile, *value.assigned_register),
        });
      }
    }
    std::sort(saved_registers.begin(),
              saved_registers.end(),
              [](const PreparedSavedRegister& lhs, const PreparedSavedRegister& rhs) {
                if (lhs.bank != rhs.bank) {
                  return static_cast<int>(lhs.bank) < static_cast<int>(rhs.bank);
                }
                if (lhs.save_index != rhs.save_index) {
                  return lhs.save_index < rhs.save_index;
                }
                if (lhs.contiguous_width != rhs.contiguous_width) {
                  return lhs.contiguous_width < rhs.contiguous_width;
                }
                return lhs.register_name < rhs.register_name;
              });
    for (const auto& block : function.blocks) {
      for (const auto& inst : block.insts) {
        const auto* call = std::get_if<bir::CallInst>(&inst);
        if (call == nullptr) {
          continue;
        }
        if (call->callee == "llvm.stacksave" || call->callee == "llvm.stackrestore" ||
            is_dynamic_alloca_call(call->callee)) {
          plan.has_dynamic_stack = true;
        }
      }
    }
    if (!plan.has_dynamic_stack) {
      PreparedFrameSlotId next_saved_slot_id =
          next_prepared_frame_slot_id(prepared.stack_layout);
      std::size_t next_saved_offset = plan.frame_size_bytes;
      for (auto& saved : saved_registers) {
        const std::size_t unit_size =
            saved_register_slot_unit_size(prepared.target_profile, saved.bank);
        if (unit_size == 0U || !saved.placement.has_value()) {
          continue;
        }
        const std::size_t align_bytes = unit_size;
        const std::size_t size_bytes =
            unit_size * std::max<std::size_t>(saved.contiguous_width, 1);
        next_saved_offset = align_prepared_offset(next_saved_offset, align_bytes);
        saved.slot_placement = make_saved_register_slot_placement(saved,
                                                                   next_saved_slot_id++,
                                                                   next_saved_offset,
                                                                   size_bytes,
                                                                   align_bytes);
        next_saved_offset += size_bytes;
      }
    }
    std::sort(plan.frame_slot_order.begin(), plan.frame_slot_order.end(), [&prepared](auto lhs, auto rhs) {
      const auto* lhs_slot = find_prepared_frame_slot(prepared.stack_layout, lhs);
      const auto* rhs_slot = find_prepared_frame_slot(prepared.stack_layout, rhs);
      if (lhs_slot == nullptr || rhs_slot == nullptr) {
        return lhs < rhs;
      }
      if (lhs_slot->offset_bytes != rhs_slot->offset_bytes) {
        return lhs_slot->offset_bytes < rhs_slot->offset_bytes;
      }
      return lhs < rhs;
    });
    plan.saved_callee_registers = std::move(saved_registers);

    plan.uses_frame_pointer_for_fixed_slots =
        plan.has_dynamic_stack && !plan.frame_slot_order.empty();

    prepared.frame_plan.functions.push_back(std::move(plan));
  }
}

}  // namespace

void BirPreAlloc::note(std::string_view message) {
  prepared_.notes.push_back(PrepareNote{
      .phase = "prealloc",
      .message = std::string(message),
  });
}

PreparedBirModule BirPreAlloc::run() {
  note("prealloc owns the shared semantic-BIR to prealloc-BIR route before MIR lowering");
  run_legalize();
  run_stack_layout();
  run_liveness();
  run_out_of_ssa();
  run_regalloc();
  publish_contract_plans();
  return std::move(prepared_);
}

void BirPreAlloc::publish_contract_plans() {
  publish_prepared_bir_label_identity(prepared_);
  populate_regalloc_placement_identity(prepared_);
  populate_frame_plan(prepared_);
  populate_dynamic_stack_plan(prepared_);
  populate_call_plans(prepared_);
  populate_variadic_entry_plans(prepared_);
  populate_frame_plan(prepared_);
  populate_storage_plans(prepared_);
  populate_i128_carriers(prepared_);
  populate_f128_carriers(prepared_);
  populate_atomic_operations(prepared_);
  populate_intrinsic_carriers(prepared_);
  populate_inline_asm_carriers(prepared_);
  populate_f128_runtime_helper_facts(prepared_);
  populate_i128_runtime_helper_lanes(prepared_);
}

PreparedBirModule prepare_semantic_bir_module_with_options(
    const c4c::backend::bir::Module& module,
    const c4c::TargetProfile& target_profile,
    const PrepareOptions& options) {
  return BirPreAlloc(module, target_profile, options).run();
}

PreparedBirModule prepare_bir_module_with_options(
    const c4c::backend::bir::Module& module,
    const c4c::TargetProfile& target_profile,
    const PrepareOptions& options) {
  return prepare_semantic_bir_module_with_options(module, target_profile, options);
}

}  // namespace c4c::backend::prepare
