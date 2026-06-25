#include "variadic_entry_plans.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::prepare {

namespace {

void append_missing_variadic_entry_fact(PreparedVariadicEntryPlanFunction& function_plan,
                                        std::string fact) {
  auto& facts = function_plan.missing_required_facts;
  if (std::find(facts.begin(), facts.end(), fact) == facts.end()) {
    facts.push_back(std::move(fact));
  }
}

void remove_missing_variadic_entry_fact(PreparedVariadicEntryPlanFunction& function_plan,
                                        std::string_view fact) {
  auto& facts = function_plan.missing_required_facts;
  facts.erase(std::remove(facts.begin(), facts.end(), fact), facts.end());
}

[[nodiscard]] std::size_t align_prepared_offset(std::size_t value,
                                                std::size_t alignment) {
  if (alignment <= 1) {
    return value;
  }
  const std::size_t remainder = value % alignment;
  return remainder == 0 ? value : value + (alignment - remainder);
}

[[nodiscard]] std::size_t aapcs64_hfa_overflow_stride_bytes(std::size_t payload_size_bytes,
                                                            std::size_t lane_size_bytes) {
  const std::size_t stack_alignment =
      std::min<std::size_t>(std::max<std::size_t>(lane_size_bytes, 8), 16);
  return align_prepared_offset(payload_size_bytes, stack_alignment);
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

[[nodiscard]] std::optional<PreparedVariadicVaListField> find_variadic_va_list_field(
    const PreparedVariadicVaListLayout& layout,
    PreparedVariadicVaListFieldKind kind) {
  for (const auto& field : layout.fields) {
    if (field.kind == kind) {
      return field;
    }
  }
  return std::nullopt;
}

struct Aapcs64HfaVaArgLaneHomes {
  std::vector<PreparedValueHome> lane_destination_homes;
};

[[nodiscard]] std::size_t type_kind_size_bytes(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::F32:
      return 4;
    case bir::TypeKind::F64:
      return 8;
    case bir::TypeKind::F128:
      return 16;
    default:
      return 0;
  }
}

[[nodiscard]] std::optional<std::size_t> aggregate_slot_suffix_offset(
    std::string_view slot_name,
    std::string_view aggregate_name) {
  if (aggregate_name.empty() ||
      slot_name.size() <= aggregate_name.size() + 1 ||
      slot_name.compare(0, aggregate_name.size(), aggregate_name) != 0 ||
      slot_name[aggregate_name.size()] != '.') {
    return std::nullopt;
  }
  std::size_t value = 0;
  for (std::size_t index = aggregate_name.size() + 1; index < slot_name.size();
       ++index) {
    const char ch = slot_name[index];
    if (ch < '0' || ch > '9') {
      return std::nullopt;
    }
    value = value * 10 + static_cast<std::size_t>(ch - '0');
  }
  return value;
}

[[nodiscard]] std::optional<Aapcs64HfaVaArgLaneHomes>
find_aapcs64_hfa_va_arg_lane_destination_homes(
    const PreparedBirModule& prepared,
    const PreparedVariadicEntryPlanFunction& function_plan,
    const bir::Block& block,
    std::size_t instruction_index,
    const bir::CallInst& call,
    std::size_t expected_lane_count,
    std::size_t expected_lane_size_bytes) {
  if (call.args.empty() ||
      call.args.front().kind != bir::Value::Kind::Named ||
      call.args.front().name.empty() ||
      expected_lane_count == 0 ||
      expected_lane_size_bytes == 0) {
    return std::nullopt;
  }
  const auto* function_addressing =
      find_prepared_addressing_function(prepared.addressing,
                                        function_plan.function_name);
  if (function_addressing == nullptr) {
    return std::nullopt;
  }
  std::vector<std::size_t> lane_offsets;
  std::vector<PreparedValueHome> lane_destination_homes;
  for (std::size_t index = instruction_index + 1; index < block.insts.size(); ++index) {
    const auto* load = std::get_if<bir::LoadLocalInst>(&block.insts[index]);
    if (load == nullptr) {
      continue;
    }
    const auto offset =
        aggregate_slot_suffix_offset(load->slot_name, call.args.front().name);
    if (!offset.has_value()) {
      continue;
    }
    const std::size_t current_lane_size = type_kind_size_bytes(load->result.type);
    if (current_lane_size != expected_lane_size_bytes) {
      return std::nullopt;
    }
    if (*offset != lane_offsets.size() * expected_lane_size_bytes) {
      return std::nullopt;
    }
    const auto* access =
        find_prepared_memory_access(*function_addressing, block.label_id, index);
    if (access == nullptr ||
        !access->result_value_name.has_value() ||
        access->address.base_kind != PreparedAddressBaseKind::FrameSlot ||
        !access->address.frame_slot_id.has_value() ||
        access->address.byte_offset < 0) {
      return std::nullopt;
    }
    const PreparedFrameSlot* frame_slot = nullptr;
    for (const auto& slot : prepared.stack_layout.frame_slots) {
      if (slot.slot_id == *access->address.frame_slot_id) {
        frame_slot = &slot;
        break;
      }
    }
    if (frame_slot == nullptr) {
      return std::nullopt;
    }
    lane_destination_homes.push_back(PreparedValueHome{
        .function_name = function_plan.function_name,
        .value_name = *access->result_value_name,
        .kind = PreparedValueHomeKind::StackSlot,
        .slot_id = access->address.frame_slot_id,
        .offset_bytes =
            frame_slot->offset_bytes +
            static_cast<std::size_t>(access->address.byte_offset),
        .size_bytes = access->address.size_bytes,
        .align_bytes = access->address.align_bytes,
    });
    lane_offsets.push_back(*offset);
  }
  if (lane_offsets.size() != expected_lane_count) {
    return std::nullopt;
  }
  if (lane_destination_homes.size() != lane_offsets.size()) {
    return std::nullopt;
  }
  return Aapcs64HfaVaArgLaneHomes{.lane_destination_homes =
                                      std::move(lane_destination_homes)};
}

[[nodiscard]] PreparedObjectId next_prepared_object_id(
    const PreparedStackLayout& stack_layout) {
  PreparedObjectId next = 0;
  for (const auto& object : stack_layout.objects) {
    next = std::max(next, object.object_id + 1);
  }
  return next;
}

[[nodiscard]] PreparedFrameSlotId next_prepared_frame_slot_id(
    const PreparedStackLayout& stack_layout) {
  PreparedFrameSlotId next = 0;
  for (const auto& slot : stack_layout.frame_slots) {
    next = std::max(next, slot.slot_id + 1);
  }
  return next;
}

[[nodiscard]] std::size_t function_frame_end_offset(
    const PreparedStackLayout& stack_layout,
    FunctionNameId function_name) {
  std::size_t end_offset = 0;
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.function_name != function_name) {
      continue;
    }
    end_offset = std::max(end_offset, slot.offset_bytes + slot.size_bytes);
  }
  return end_offset;
}

[[nodiscard]] const PreparedFrameSlot* find_variadic_storage_slot(
    const PreparedBirModule& prepared,
    FunctionNameId function_name,
    std::string_view source_kind) {
  for (const auto& object : prepared.stack_layout.objects) {
    if (object.function_name != function_name ||
        object.source_kind != source_kind) {
      continue;
    }
    return find_prepared_frame_slot(prepared.stack_layout, object.object_id);
  }
  return nullptr;
}

[[nodiscard]] std::string variadic_va_start_destination_source_kind(
    const PreparedValueHome& destination) {
  std::string source_kind = "aapcs64_variadic_va_start_destination.";
  source_kind += std::to_string(destination.value_name);
  return source_kind;
}

[[nodiscard]] std::string variadic_va_start_destination_slot_name(
    const PreparedValueHome& destination) {
  std::string slot_name = "__aapcs64_variadic_va_start_destination_";
  slot_name += std::to_string(destination.value_name);
  return slot_name;
}

PreparedFrameSlot& append_variadic_storage_slot(PreparedBirModule& prepared,
                                                FunctionNameId function_name,
                                                std::string_view slot_name,
                                                std::string_view source_kind,
                                                std::size_t size_bytes,
                                                std::size_t align_bytes) {
  const PreparedObjectId object_id = next_prepared_object_id(prepared.stack_layout);
  const PreparedFrameSlotId slot_id = next_prepared_frame_slot_id(prepared.stack_layout);
  const std::size_t offset_bytes =
      align_prepared_offset(function_frame_end_offset(prepared.stack_layout, function_name),
                            align_bytes);

  prepared.stack_layout.objects.push_back(PreparedStackObject{
      .object_id = object_id,
      .function_name = function_name,
      .slot_name = prepared.names.slot_names.intern(slot_name),
      .source_kind = std::string(source_kind),
      .type = bir::TypeKind::Void,
      .size_bytes = size_bytes,
      .align_bytes = align_bytes,
      .address_exposed = true,
      .requires_home_slot = true,
      .permanent_home_slot = true,
  });
  prepared.stack_layout.frame_slots.push_back(PreparedFrameSlot{
      .slot_id = slot_id,
      .object_id = object_id,
      .function_name = function_name,
      .offset_bytes = offset_bytes,
      .size_bytes = size_bytes,
      .align_bytes = align_bytes,
      .fixed_location = true,
  });
  prepared.stack_layout.frame_size_bytes =
      std::max(prepared.stack_layout.frame_size_bytes, offset_bytes + size_bytes);
  prepared.stack_layout.frame_alignment_bytes =
      std::max(prepared.stack_layout.frame_alignment_bytes, align_bytes);
  return prepared.stack_layout.frame_slots.back();
}

[[nodiscard]] std::optional<PreparedValueHome> materialize_va_start_destination_home(
    PreparedBirModule& prepared,
    const PreparedVariadicEntryPlanFunction& function_plan,
    const PreparedValueHome& destination) {
  if (!function_plan.va_list_layout.size_bytes.has_value() ||
      !function_plan.va_list_layout.align_bytes.has_value()) {
    return std::nullopt;
  }

  const std::string source_kind =
      variadic_va_start_destination_source_kind(destination);
  const PreparedFrameSlot* slot =
      find_variadic_storage_slot(prepared, function_plan.function_name, source_kind);
  if (slot == nullptr) {
    const std::string slot_name =
        variadic_va_start_destination_slot_name(destination);
    slot = &append_variadic_storage_slot(prepared,
                                         function_plan.function_name,
                                         slot_name,
                                         source_kind,
                                         *function_plan.va_list_layout.size_bytes,
                                         *function_plan.va_list_layout.align_bytes);
  }

  return PreparedValueHome{
      .value_id = destination.value_id,
      .function_name = destination.function_name,
      .value_name = destination.value_name,
      .kind = PreparedValueHomeKind::StackSlot,
      .slot_id = slot->slot_id,
      .offset_bytes = slot->offset_bytes,
      .size_bytes = slot->size_bytes,
      .align_bytes = slot->align_bytes,
  };
}

void populate_aapcs64_variadic_entry_abi_facts(
    PreparedVariadicEntryPlanFunction& function_plan) {
  if (function_plan.helper_resources.required_helpers.empty()) {
    return;
  }

  constexpr std::size_t kAapcs64GpArgumentRegisters = 8;
  constexpr std::size_t kAapcs64FpArgumentRegisters = 8;
  constexpr std::size_t kAapcs64GpSlotBytes = 8;
  constexpr std::size_t kAapcs64FpSlotBytes = 16;
  constexpr std::size_t kAapcs64GpSaveAreaBytes =
      kAapcs64GpArgumentRegisters * kAapcs64GpSlotBytes;
  constexpr std::size_t kAapcs64FpSaveAreaBytes =
      kAapcs64FpArgumentRegisters * kAapcs64FpSlotBytes;

  function_plan.register_save_area.required = true;
  function_plan.register_save_area.size_bytes =
      kAapcs64GpSaveAreaBytes + kAapcs64FpSaveAreaBytes;
  function_plan.register_save_area.align_bytes = kAapcs64FpSlotBytes;
  function_plan.register_save_area.gp_offset_bytes = 0;
  function_plan.register_save_area.fp_offset_bytes = kAapcs64GpSaveAreaBytes;
  function_plan.register_save_area.gp_slot_size_bytes = kAapcs64GpSlotBytes;
  function_plan.register_save_area.fp_slot_size_bytes = kAapcs64FpSlotBytes;

  function_plan.overflow_area.required = true;
  function_plan.overflow_area.align_bytes = kAapcs64GpSlotBytes;

  function_plan.va_list_layout.required = true;
  function_plan.va_list_layout.size_bytes = 32;
  function_plan.va_list_layout.align_bytes = 8;
  function_plan.va_list_layout.fields = {
      PreparedVariadicVaListField{
          .kind = PreparedVariadicVaListFieldKind::OverflowArgArea,
          .offset_bytes = 0,
          .size_bytes = 8,
      },
      PreparedVariadicVaListField{
          .kind = PreparedVariadicVaListFieldKind::GpRegisterSaveArea,
          .offset_bytes = 8,
          .size_bytes = 8,
      },
      PreparedVariadicVaListField{
          .kind = PreparedVariadicVaListFieldKind::FpRegisterSaveArea,
          .offset_bytes = 16,
          .size_bytes = 8,
      },
      PreparedVariadicVaListField{
          .kind = PreparedVariadicVaListFieldKind::GpOffset,
          .offset_bytes = 24,
          .size_bytes = 4,
      },
      PreparedVariadicVaListField{
          .kind = PreparedVariadicVaListFieldKind::FpOffset,
          .offset_bytes = 28,
          .size_bytes = 4,
      },
  };

  if (!function_plan.named_register_counts.gp.has_value()) {
    append_missing_variadic_entry_fact(function_plan, "named_register_counts.gp");
  } else {
    const std::size_t named_gp =
        std::min(*function_plan.named_register_counts.gp, kAapcs64GpArgumentRegisters);
    const std::size_t saved_gp = kAapcs64GpArgumentRegisters - named_gp;
    function_plan.register_save_area.saved_gp_register_count = saved_gp;
    function_plan.register_save_area.initial_gp_offset_bytes =
        saved_gp == 0 ? 0 : -static_cast<std::ptrdiff_t>(saved_gp * kAapcs64GpSlotBytes);
  }

  if (!function_plan.named_register_counts.fp.has_value()) {
    append_missing_variadic_entry_fact(function_plan, "named_register_counts.fp");
  } else {
    const std::size_t named_fp =
        std::min(*function_plan.named_register_counts.fp, kAapcs64FpArgumentRegisters);
    const std::size_t saved_fp = kAapcs64FpArgumentRegisters - named_fp;
    function_plan.register_save_area.saved_fp_register_count = saved_fp;
    function_plan.register_save_area.initial_fp_offset_bytes =
        saved_fp == 0 ? 0 : -static_cast<std::ptrdiff_t>(saved_fp * kAapcs64FpSlotBytes);
  }

  append_missing_variadic_entry_fact(function_plan, "register_save_area.slot_id");
  append_missing_variadic_entry_fact(function_plan, "register_save_area.stack_offset_bytes");
  append_missing_variadic_entry_fact(function_plan, "overflow_area.base_slot_id");
  append_missing_variadic_entry_fact(function_plan, "overflow_area.base_stack_offset_bytes");
}

struct VariadicHelperScratchRequirement {
  std::size_t scratch_register_count = 0;
  std::size_t scratch_stack_bytes = 0;
};

[[nodiscard]] constexpr VariadicHelperScratchRequirement
aapcs64_variadic_helper_scratch_requirement(PreparedVariadicEntryHelperKind kind) {
  switch (kind) {
    case PreparedVariadicEntryHelperKind::VaStart:
      return VariadicHelperScratchRequirement{
          .scratch_register_count = 1,
      };
    case PreparedVariadicEntryHelperKind::VaArg:
    case PreparedVariadicEntryHelperKind::VaArgAggregate:
      return VariadicHelperScratchRequirement{
          .scratch_register_count = 2,
      };
    case PreparedVariadicEntryHelperKind::VaCopy:
      return VariadicHelperScratchRequirement{
          .scratch_register_count = 1,
      };
  }
  return VariadicHelperScratchRequirement{};
}

void populate_aapcs64_variadic_entry_helper_resource_authority(
    PreparedVariadicEntryPlanFunction& function_plan) {
  if (function_plan.helper_resources.required_helpers.empty()) {
    return;
  }

  std::size_t scratch_register_count = 0;
  std::size_t scratch_stack_bytes = 0;
  for (const auto helper : function_plan.helper_resources.required_helpers) {
    const auto requirement = aapcs64_variadic_helper_scratch_requirement(helper);
    scratch_register_count =
        std::max(scratch_register_count, requirement.scratch_register_count);
    scratch_stack_bytes =
        std::max(scratch_stack_bytes, requirement.scratch_stack_bytes);
  }
  function_plan.helper_resources.scratch_register_count = scratch_register_count;
  function_plan.helper_resources.scratch_stack_bytes = scratch_stack_bytes;
  remove_missing_variadic_entry_fact(function_plan,
                                     "helper_resources.scratch_register_count");
  remove_missing_variadic_entry_fact(function_plan, "helper_resources.scratch_stack_bytes");
}

void publish_missing_non_aapcs64_variadic_entry_contract(
    PreparedVariadicEntryPlanFunction& function_plan) {
  append_missing_variadic_entry_fact(function_plan,
                                     "target_abi.variadic_entry_state");
  append_missing_variadic_entry_fact(function_plan, "target_abi.va_list_layout");

  if (function_plan.helper_resources.required_helpers.empty()) {
    return;
  }

  append_missing_variadic_entry_fact(function_plan,
                                     "helper_resources.scratch_register_count");
  append_missing_variadic_entry_fact(function_plan,
                                     "helper_resources.scratch_stack_bytes");

  for (const auto helper : function_plan.helper_resources.required_helpers) {
    switch (helper) {
      case PreparedVariadicEntryHelperKind::VaStart:
        append_missing_variadic_entry_fact(
            function_plan,
            "helper_operand_homes.va_start.destination_va_list");
        append_missing_variadic_entry_fact(
            function_plan,
            "helper_operand_homes.va_start.destination_va_list_address");
        break;
      case PreparedVariadicEntryHelperKind::VaArg:
        append_missing_variadic_entry_fact(
            function_plan,
            "helper_operand_homes.va_arg.source_va_list");
        append_missing_variadic_entry_fact(
            function_plan,
            "helper_operand_homes.va_arg.scalar_result");
        append_missing_variadic_entry_fact(
            function_plan,
            "helper_operand_homes.va_arg.scalar_access_plan");
        break;
      case PreparedVariadicEntryHelperKind::VaArgAggregate:
        append_missing_variadic_entry_fact(
            function_plan,
            "helper_operand_homes.va_arg_aggregate.source_va_list");
        append_missing_variadic_entry_fact(
            function_plan,
            "helper_operand_homes.va_arg_aggregate.aggregate_destination_payload");
        append_missing_variadic_entry_fact(
            function_plan,
            "helper_operand_homes.va_arg_aggregate.aggregate_access_plan");
        break;
      case PreparedVariadicEntryHelperKind::VaCopy:
        append_missing_variadic_entry_fact(
            function_plan,
            "helper_operand_homes.va_copy.destination_va_list");
        append_missing_variadic_entry_fact(
            function_plan,
            "helper_operand_homes.va_copy.source_va_list");
        break;
    }
  }
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

[[nodiscard]] std::optional<PreparedVariadicScalarVaArgAccessPlan>
make_aapcs64_scalar_va_arg_access_plan(
    const PreparedVariadicEntryPlanFunction& function_plan,
    const PreparedVariadicEntryHelperOperandHomes& homes,
    const bir::CallInst& call) {
  if (!homes.scalar_result.has_value() || !homes.source_va_list.has_value()) {
    return std::nullopt;
  }
  if (!call.va_arg_payload_abi.has_value()) {
    return std::nullopt;
  }
  const auto& abi = *call.va_arg_payload_abi;

  PreparedVariadicScalarVaArgAccessPlan plan{
      .value_type = abi.type,
      .value_size_bytes = abi.size_bytes,
      .value_align_bytes = abi.align_bytes,
      .result_home = homes.scalar_result,
      .overflow_source_field = PreparedVariadicVaListFieldKind::OverflowArgArea,
      .overflow_stride_bytes =
          align_prepared_offset(std::max<std::size_t>(abi.size_bytes, 1),
                                std::max<std::size_t>(abi.align_bytes, 1)),
  };

  if (const auto overflow_field = find_variadic_va_list_field(
          function_plan.va_list_layout, PreparedVariadicVaListFieldKind::OverflowArgArea);
      overflow_field.has_value()) {
    plan.overflow_source_field_offset_bytes = overflow_field->offset_bytes;
  }

  switch (abi.primary_class) {
    case bir::AbiValueClass::Integer:
      plan.source_class = PreparedVariadicScalarVaArgSourceClass::GpRegisterSaveArea;
      plan.source_field = PreparedVariadicVaListFieldKind::GpRegisterSaveArea;
      plan.progression_field = PreparedVariadicVaListFieldKind::GpOffset;
      plan.source_slot_size_bytes = function_plan.register_save_area.gp_slot_size_bytes;
      plan.progression_stride_bytes = function_plan.register_save_area.gp_slot_size_bytes;
      break;
    case bir::AbiValueClass::Sse:
      plan.source_class = PreparedVariadicScalarVaArgSourceClass::FpRegisterSaveArea;
      plan.source_field = PreparedVariadicVaListFieldKind::FpRegisterSaveArea;
      plan.progression_field = PreparedVariadicVaListFieldKind::FpOffset;
      plan.source_slot_size_bytes = function_plan.register_save_area.fp_slot_size_bytes;
      plan.progression_stride_bytes = function_plan.register_save_area.fp_slot_size_bytes;
      break;
    case bir::AbiValueClass::Memory:
      plan.source_class = PreparedVariadicScalarVaArgSourceClass::OverflowArgArea;
      plan.source_field = PreparedVariadicVaListFieldKind::OverflowArgArea;
      plan.progression_field = PreparedVariadicVaListFieldKind::OverflowArgArea;
      plan.source_slot_size_bytes = plan.overflow_stride_bytes;
      plan.progression_stride_bytes = plan.overflow_stride_bytes;
      break;
    case bir::AbiValueClass::None:
    case bir::AbiValueClass::X87:
      return std::nullopt;
  }

  if (plan.source_field.has_value()) {
    if (const auto source_field = find_variadic_va_list_field(
            function_plan.va_list_layout, *plan.source_field);
        source_field.has_value()) {
      plan.source_field_offset_bytes = source_field->offset_bytes;
    }
  }
  if (plan.progression_field.has_value()) {
    if (const auto progression_field = find_variadic_va_list_field(
            function_plan.va_list_layout, *plan.progression_field);
        progression_field.has_value()) {
      plan.progression_field_offset_bytes = progression_field->offset_bytes;
    }
  }

  return plan;
}

[[nodiscard]] std::optional<PreparedVariadicAggregateVaArgAccessPlan>
make_aapcs64_aggregate_va_arg_access_plan(
    const PreparedBirModule& prepared,
    const PreparedVariadicEntryPlanFunction& function_plan,
    const PreparedVariadicEntryHelperOperandHomes& homes,
    const bir::Block& block,
    std::size_t instruction_index,
    const bir::CallInst& call) {
  if (!homes.aggregate_destination_payload.has_value() ||
      !homes.source_va_list.has_value() ||
      !call.va_arg_payload_abi.has_value()) {
    return std::nullopt;
  }

  const auto& payload_abi = *call.va_arg_payload_abi;
  if (payload_abi.type != bir::TypeKind::Ptr ||
      !payload_abi.sret_pointer ||
      payload_abi.primary_class != bir::AbiValueClass::Memory ||
      payload_abi.size_bytes == 0 ||
      payload_abi.align_bytes == 0) {
    return std::nullopt;
  }

  const std::size_t payload_stride_bytes =
      align_prepared_offset(payload_abi.size_bytes, payload_abi.align_bytes);
  PreparedVariadicAggregateVaArgAccessPlan plan{
      .source_class = PreparedVariadicAggregateVaArgSourceClass::OverflowArgArea,
      .block_index = homes.block_index,
      .instruction_index = homes.instruction_index,
      .payload_size_bytes = payload_abi.size_bytes,
      .payload_align_bytes = payload_abi.align_bytes,
      .destination_payload_home = homes.aggregate_destination_payload,
      .source_field = PreparedVariadicVaListFieldKind::OverflowArgArea,
      .source_payload_offset_bytes = std::size_t{0},
      .source_slot_size_bytes = payload_stride_bytes,
      .copy_size_bytes = payload_abi.size_bytes,
      .copy_align_bytes = payload_abi.align_bytes,
      .progression_field = PreparedVariadicVaListFieldKind::OverflowArgArea,
      .progression_stride_bytes = payload_stride_bytes,
      .overflow_stride_bytes = payload_stride_bytes,
  };

  if (call.va_arg_hfa_lane_count > 0 &&
      call.va_arg_hfa_lane_size_bytes > 0 &&
      function_plan.register_save_area.fp_slot_size_bytes.has_value()) {
    const auto lane_homes = find_aapcs64_hfa_va_arg_lane_destination_homes(
        prepared,
        function_plan,
        block,
        instruction_index,
        call,
        call.va_arg_hfa_lane_count,
        call.va_arg_hfa_lane_size_bytes);
    if (!lane_homes.has_value()) {
      return std::nullopt;
    }
    plan.source_class = PreparedVariadicAggregateVaArgSourceClass::RegisterSaveArea;
    plan.source_field = PreparedVariadicVaListFieldKind::FpRegisterSaveArea;
    plan.source_slot_size_bytes = function_plan.register_save_area.fp_slot_size_bytes;
    plan.copy_size_bytes = payload_abi.size_bytes;
    plan.copy_align_bytes = payload_abi.align_bytes;
    plan.progression_field = PreparedVariadicVaListFieldKind::FpOffset;
    plan.progression_stride_bytes =
        call.va_arg_hfa_lane_count * *function_plan.register_save_area.fp_slot_size_bytes;
    plan.register_save_lane_count = call.va_arg_hfa_lane_count;
    plan.register_save_lane_size_bytes = call.va_arg_hfa_lane_size_bytes;
    plan.register_save_lane_destination_homes =
        std::move(lane_homes->lane_destination_homes);
    plan.overflow_stride_bytes =
        aapcs64_hfa_overflow_stride_bytes(payload_abi.size_bytes,
                                          call.va_arg_hfa_lane_size_bytes);
  }

  if (const auto overflow_field = find_variadic_va_list_field(
          function_plan.va_list_layout, PreparedVariadicVaListFieldKind::OverflowArgArea);
      overflow_field.has_value()) {
    plan.overflow_source_field_offset_bytes = overflow_field->offset_bytes;
    if (plan.source_field == PreparedVariadicVaListFieldKind::OverflowArgArea) {
      plan.source_field_offset_bytes = overflow_field->offset_bytes;
    }
    if (plan.progression_field ==
        PreparedVariadicVaListFieldKind::OverflowArgArea) {
      plan.progression_field_offset_bytes = overflow_field->offset_bytes;
    }
  }
  if (const auto source_field = find_variadic_va_list_field(
          function_plan.va_list_layout, *plan.source_field);
      source_field.has_value()) {
    plan.source_field_offset_bytes = source_field->offset_bytes;
  }
  if (const auto progression_field = find_variadic_va_list_field(
          function_plan.va_list_layout, *plan.progression_field);
      progression_field.has_value()) {
    plan.progression_field_offset_bytes = progression_field->offset_bytes;
  }

  return plan;
}

void require_variadic_helper_operand_home(
    PreparedVariadicEntryPlanFunction& function_plan,
    const PreparedVariadicEntryHelperOperandHomes& homes,
    const std::optional<PreparedValueHome>& home,
    std::string_view fact_suffix) {
  if (home.has_value()) {
    return;
  }
  std::string fact = "helper_operand_homes.";
  fact += prepared_variadic_entry_helper_kind_name(homes.helper);
  fact += ".";
  fact += fact_suffix;
  append_missing_variadic_entry_fact(function_plan, fact);
}

void populate_aapcs64_variadic_entry_helper_operand_home_authority(
    PreparedBirModule& prepared,
    const bir::Function& function,
    PreparedVariadicEntryPlanFunction& function_plan) {
  function_plan.helper_operand_homes.clear();
  const auto* value_locations =
      find_prepared_value_location_function(prepared, function_plan.function_name);

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (std::size_t instruction_index = 0; instruction_index < block.insts.size();
         ++instruction_index) {
      const auto* call = std::get_if<bir::CallInst>(&block.insts[instruction_index]);
      if (call == nullptr) {
        continue;
      }
      const auto helper_kind = prepared_variadic_entry_helper_kind_for_call(*call);
      if (!helper_kind.has_value()) {
        continue;
      }

      PreparedVariadicEntryHelperOperandHomes homes{
          .helper = *helper_kind,
          .block_index = block_index,
          .instruction_index = instruction_index,
      };

      switch (*helper_kind) {
        case PreparedVariadicEntryHelperKind::VaStart:
          if (!call->args.empty()) {
            homes.destination_va_list =
                prepared_home_for_named_value(prepared.names, value_locations, call->args[0]);
            if (homes.destination_va_list.has_value()) {
              homes.destination_va_list_address =
                  materialize_va_start_destination_home(
                      prepared, function_plan, *homes.destination_va_list);
            }
          }
          require_variadic_helper_operand_home(function_plan,
                                               homes,
                                               homes.destination_va_list,
                                               "destination_va_list");
          require_variadic_helper_operand_home(function_plan,
                                               homes,
                                               homes.destination_va_list_address,
                                               "destination_va_list_address");
          break;
        case PreparedVariadicEntryHelperKind::VaArg:
          if (call->result.has_value()) {
            homes.scalar_result =
                prepared_home_for_named_value(prepared.names, value_locations, *call->result);
          }
          if (!call->args.empty()) {
            homes.source_va_list =
                prepared_home_for_named_value(prepared.names, value_locations, call->args[0]);
          }
          require_variadic_helper_operand_home(
              function_plan, homes, homes.scalar_result, "scalar_result");
          require_variadic_helper_operand_home(
              function_plan, homes, homes.source_va_list, "source_va_list");
          homes.scalar_access_plan = make_aapcs64_scalar_va_arg_access_plan(
              function_plan, homes, *call);
          if (!homes.scalar_access_plan.has_value()) {
            append_missing_variadic_entry_fact(
                function_plan, "helper_operand_homes.va_arg.scalar_access_plan");
          }
          break;
        case PreparedVariadicEntryHelperKind::VaArgAggregate:
          if (call->args.size() > 1) {
            homes.aggregate_destination_payload =
                prepared_home_for_named_value(prepared.names, value_locations, call->args[0]);
            homes.source_va_list =
                prepared_home_for_named_value(prepared.names, value_locations, call->args[1]);
          } else {
            if (call->result.has_value()) {
              homes.aggregate_destination_payload =
                  prepared_home_for_named_value(prepared.names, value_locations, *call->result);
            }
            if (!call->args.empty()) {
              homes.source_va_list =
                  prepared_home_for_named_value(prepared.names, value_locations, call->args[0]);
            }
          }
          require_variadic_helper_operand_home(function_plan,
                                               homes,
                                               homes.aggregate_destination_payload,
                                               "aggregate_destination_payload");
          require_variadic_helper_operand_home(
              function_plan, homes, homes.source_va_list, "source_va_list");
          homes.aggregate_access_plan =
              make_aapcs64_aggregate_va_arg_access_plan(
                  prepared, function_plan, homes, block, instruction_index, *call);
          if (!has_complete_prepared_variadic_aggregate_va_arg_access_plan(homes)) {
            append_missing_variadic_entry_fact(
                function_plan,
                "helper_operand_homes.va_arg_aggregate.aggregate_access_plan");
          }
          break;
        case PreparedVariadicEntryHelperKind::VaCopy:
          if (!call->args.empty()) {
            homes.destination_va_list =
                prepared_home_for_named_value(prepared.names, value_locations, call->args[0]);
          }
          if (call->args.size() > 1) {
            homes.source_va_list =
                prepared_home_for_named_value(prepared.names, value_locations, call->args[1]);
          }
          require_variadic_helper_operand_home(function_plan,
                                               homes,
                                               homes.destination_va_list,
                                               "destination_va_list");
          require_variadic_helper_operand_home(
              function_plan, homes, homes.source_va_list, "source_va_list");
          break;
      }

      function_plan.helper_operand_homes.push_back(std::move(homes));
    }
  }
}

void attach_aapcs64_variadic_entry_storage_authority(
    PreparedBirModule& prepared,
    PreparedVariadicEntryPlanFunction& function_plan) {
  if (function_plan.helper_resources.required_helpers.empty() ||
      !function_plan.register_save_area.required ||
      !function_plan.overflow_area.required ||
      !function_plan.register_save_area.size_bytes.has_value() ||
      !function_plan.register_save_area.align_bytes.has_value() ||
      !function_plan.overflow_area.align_bytes.has_value()) {
    return;
  }

  const PreparedFrameSlot* register_save_slot =
      find_variadic_storage_slot(prepared,
                                 function_plan.function_name,
                                 "aapcs64_variadic_register_save_area");
  if (register_save_slot == nullptr) {
    register_save_slot = &append_variadic_storage_slot(
        prepared,
        function_plan.function_name,
        "__aapcs64_variadic_register_save_area",
        "aapcs64_variadic_register_save_area",
        *function_plan.register_save_area.size_bytes,
        *function_plan.register_save_area.align_bytes);
  }
  function_plan.register_save_area.slot_id = register_save_slot->slot_id;
  function_plan.register_save_area.stack_offset_bytes = register_save_slot->offset_bytes;
  remove_missing_variadic_entry_fact(function_plan, "register_save_area.slot_id");
  remove_missing_variadic_entry_fact(function_plan, "register_save_area.stack_offset_bytes");

  const PreparedFrameSlot* overflow_base_slot =
      find_variadic_storage_slot(prepared,
                                 function_plan.function_name,
                                 "aapcs64_variadic_overflow_area_base");
  if (overflow_base_slot == nullptr) {
    overflow_base_slot = &append_variadic_storage_slot(
        prepared,
        function_plan.function_name,
        "__aapcs64_variadic_overflow_area_base",
        "aapcs64_variadic_overflow_area_base",
        0,
        *function_plan.overflow_area.align_bytes);
  }
  function_plan.overflow_area.base_slot_id = overflow_base_slot->slot_id;
  function_plan.overflow_area.base_stack_offset_bytes = overflow_base_slot->offset_bytes;
  remove_missing_variadic_entry_fact(function_plan, "overflow_area.base_slot_id");
  remove_missing_variadic_entry_fact(function_plan, "overflow_area.base_stack_offset_bytes");
}

}  // namespace

void populate_variadic_entry_plans(PreparedBirModule& prepared) {
  prepared.variadic_entry_plans.functions.clear();

  for (const auto& function : prepared.module.functions) {
    if (function.is_declaration || !function.is_variadic) {
      continue;
    }
    const FunctionNameId function_name_id = prepared.names.function_names.find(function.name);
    if (function_name_id == kInvalidFunctionName) {
      continue;
    }

    PreparedVariadicEntryPlanFunction function_plan{
        .function_name = function_name_id,
        .named_parameter_count = 0,
        .named_register_counts = {},
        .register_save_area = {},
        .overflow_area = {},
        .va_list_layout = {},
        .helper_resources = {},
    };

    std::size_t named_gp_register_count = 0;
    std::size_t named_fp_register_count = 0;
    bool has_named_gp_register_count = true;
    bool has_named_fp_register_count = true;
    for (const auto& param : function.params) {
      if (param.is_varargs) {
        continue;
      }
      ++function_plan.named_parameter_count;
      if (!param.abi.has_value()) {
        has_named_gp_register_count = false;
        has_named_fp_register_count = false;
        continue;
      }
      if (!param.abi->passed_in_register) {
        continue;
      }
      switch (param.abi->primary_class) {
        case bir::AbiValueClass::Integer:
          ++named_gp_register_count;
          break;
        case bir::AbiValueClass::Sse:
          ++named_fp_register_count;
          break;
        case bir::AbiValueClass::X87:
        case bir::AbiValueClass::Memory:
        case bir::AbiValueClass::None:
          break;
      }
    }
    if (has_named_gp_register_count) {
      function_plan.named_register_counts.gp = named_gp_register_count;
    }
    if (has_named_fp_register_count) {
      function_plan.named_register_counts.fp = named_fp_register_count;
    }

    for (const auto& block : function.blocks) {
      for (const auto& inst : block.insts) {
        const auto* call = std::get_if<bir::CallInst>(&inst);
        if (call == nullptr) {
          continue;
        }
        const auto helper_kind = prepared_variadic_entry_helper_kind_for_call(*call);
        if (!helper_kind.has_value()) {
          continue;
        }
        auto& helpers = function_plan.helper_resources.required_helpers;
        if (std::find(helpers.begin(), helpers.end(), *helper_kind) == helpers.end()) {
          helpers.push_back(*helper_kind);
        }
      }
    }

    if (prepared.target_profile.backend_abi == c4c::BackendAbiKind::Aapcs64) {
      populate_aapcs64_variadic_entry_abi_facts(function_plan);
      populate_aapcs64_variadic_entry_helper_resource_authority(function_plan);
      attach_aapcs64_variadic_entry_storage_authority(prepared, function_plan);
      populate_aapcs64_variadic_entry_helper_operand_home_authority(
          prepared, function, function_plan);
    } else {
      publish_missing_non_aapcs64_variadic_entry_contract(function_plan);
    }

    prepared.variadic_entry_plans.functions.push_back(std::move(function_plan));
  }
}

}  // namespace c4c::backend::prepare
