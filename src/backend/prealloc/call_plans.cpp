#include "call_plans.hpp"
#include "regalloc/call_return_abi.hpp"
#include "target_register_profile.hpp"

#include <algorithm>
#include <charconv>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

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

[[nodiscard]] std::size_t aarch64_byval_register_lane_width(
    const c4c::TargetProfile& target_profile,
    const bir::CallArgAbiInfo& abi) {
  if (target_profile.arch == c4c::TargetArch::Aarch64 &&
      abi.type == bir::TypeKind::Ptr &&
      abi.byval_copy &&
      abi.passed_in_register &&
      !abi.passed_on_stack &&
      abi.primary_class == bir::AbiValueClass::Integer &&
      abi.size_bytes > 0 &&
      abi.size_bytes <= 16) {
    return std::max<std::size_t>((abi.size_bytes + 7) / 8, 1);
  }
  return 1;
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

[[nodiscard]] const bir::Function* find_module_function(const bir::Module& module,
                                                        std::string_view function_name) {
  for (const auto& function : module.functions) {
    if (function.name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

[[nodiscard]] const bir::Function* find_module_function_by_link_name_id(
    const bir::Module& module,
    LinkNameId link_name_id) {
  if (link_name_id == kInvalidLinkName) {
    return nullptr;
  }
  for (const auto& function : module.functions) {
    if (function.link_name_id == link_name_id) {
      return &function;
    }
  }
  return nullptr;
}

[[nodiscard]] bool module_has_symbol_link_name_id(const bir::Module& module,
                                                  LinkNameId link_name_id) {
  if (link_name_id == kInvalidLinkName) {
    return false;
  }
  for (const auto& global : module.globals) {
    if (global.link_name_id == link_name_id) {
      return true;
    }
  }
  return find_module_function_by_link_name_id(module, link_name_id) != nullptr;
}

[[nodiscard]] std::optional<std::string_view> resolve_symbol_pointer_name(
    const bir::Module& module,
    const bir::Value& value) {
  if (value.pointer_symbol_link_name_id == kInvalidLinkName) {
    if (value.kind == bir::Value::Kind::Named && !value.name.empty() && value.name.front() == '@') {
      return value.name;
    }
    return std::nullopt;
  }

  const std::string_view semantic_name =
      module.names.link_names.spelling(value.pointer_symbol_link_name_id);
  if (semantic_name.empty() || !module_has_symbol_link_name_id(module, value.pointer_symbol_link_name_id)) {
    return std::nullopt;
  }
  std::string_view raw_name = value.name;
  if (!raw_name.empty() && raw_name.front() == '@') {
    raw_name.remove_prefix(1);
  }
  if (!raw_name.empty() && raw_name != semantic_name) {
    return std::nullopt;
  }
  return semantic_name;
}

[[nodiscard]] std::string display_symbol_pointer_name(std::string_view semantic_name) {
  if (!semantic_name.empty() && semantic_name.front() == '@') {
    return std::string(semantic_name);
  }
  return "@" + std::string(semantic_name);
}

struct DirectCalleeResolution {
  std::optional<std::string_view> name;
  const bir::Function* function = nullptr;
};

struct PreparedValueHomeLookup {
  std::vector<std::pair<PreparedValueId, const PreparedValueHome*>> by_value_id;
};

struct CallPreservationCandidates {
  std::vector<const PreparedRegallocValue*> by_start_point;
};

struct CallArgumentDestinationPlan {
  std::optional<std::string> register_name;
  std::size_t contiguous_width = 1;
  std::vector<std::string> occupied_register_names;
  std::optional<PreparedRegisterBank> register_bank;
  std::optional<std::size_t> stack_offset_bytes;
  std::optional<PreparedRegisterPlacement> register_placement;
};

struct CallArgumentSourcePlan {
  PreparedStorageEncodingKind encoding = PreparedStorageEncodingKind::None;
  std::optional<PreparedValueId> value_id;
  std::optional<PreparedValueId> base_value_id;
  std::optional<bir::Value> literal;
  std::optional<std::string> symbol_name;
  std::optional<LinkNameId> symbol_name_id;
  std::optional<std::string> register_name;
  std::optional<PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  std::optional<PreparedRegisterBank> register_bank;
  std::optional<ValueNameId> base_value_name;
  std::optional<std::int64_t> pointer_byte_delta;
  std::optional<PreparedRegisterPlacement> register_placement;
};

[[nodiscard]] PreparedValueHomeLookup make_prepared_value_home_lookup(
    const PreparedValueLocationFunction* value_locations) {
  PreparedValueHomeLookup lookup;
  if (value_locations == nullptr) {
    return lookup;
  }
  lookup.by_value_id.reserve(value_locations->value_homes.size());
  for (const auto& home : value_locations->value_homes) {
    lookup.by_value_id.push_back({home.value_id, &home});
  }
  std::sort(lookup.by_value_id.begin(),
            lookup.by_value_id.end(),
            [](const auto& lhs, const auto& rhs) {
              return lhs.first < rhs.first;
            });
  return lookup;
}

[[nodiscard]] CallPreservationCandidates make_call_preservation_candidates(
    const PreparedRegallocFunction* regalloc_function) {
  CallPreservationCandidates candidates;
  if (regalloc_function == nullptr) {
    return candidates;
  }
  candidates.by_start_point.reserve(regalloc_function->values.size());
  for (const auto& value : regalloc_function->values) {
    if (!value.crosses_call || !value.live_interval.has_value()) {
      continue;
    }
    candidates.by_start_point.push_back(&value);
  }
  std::sort(candidates.by_start_point.begin(),
            candidates.by_start_point.end(),
            [](const PreparedRegallocValue* lhs, const PreparedRegallocValue* rhs) {
              if (lhs->live_interval->start_point != rhs->live_interval->start_point) {
                return lhs->live_interval->start_point < rhs->live_interval->start_point;
              }
              return lhs->value_id < rhs->value_id;
            });
  return candidates;
}

[[nodiscard]] const PreparedValueHome* find_prepared_value_home(
    const PreparedValueHomeLookup& lookup,
    PreparedValueId value_id) {
  const auto it = std::lower_bound(
      lookup.by_value_id.begin(),
      lookup.by_value_id.end(),
      value_id,
      [](const auto& entry, PreparedValueId needle) {
        return entry.first < needle;
      });
  if (it == lookup.by_value_id.end() || it->first != value_id) {
    return nullptr;
  }
  return it->second;
}

[[nodiscard]] DirectCalleeResolution resolve_direct_callee(const bir::Module& module,
                                                           const bir::CallInst& call) {
  if (call.is_indirect) {
    return {};
  }

  if (call.callee_link_name_id != kInvalidLinkName) {
    const std::string_view semantic_name =
        module.names.link_names.spelling(call.callee_link_name_id);
    if (semantic_name.empty()) {
      return {};
    }
    if (!call.callee.empty() && call.callee != semantic_name) {
      return {};
    }
    return DirectCalleeResolution{
        .name = semantic_name,
        .function = find_module_function_by_link_name_id(module, call.callee_link_name_id),
    };
  }

  if (call.callee.empty()) {
    return {};
  }
  return DirectCalleeResolution{
      .name = call.callee,
      .function = find_module_function(module, call.callee),
  };
}

[[nodiscard]] PreparedCallWrapperKind classify_call_wrapper_kind(
    const bir::CallInst& call,
    const DirectCalleeResolution& direct_callee) {
  if (call.is_indirect) {
    return PreparedCallWrapperKind::Indirect;
  }
  if (!direct_callee.name.has_value()) {
    return PreparedCallWrapperKind::Indirect;
  }

  if (const auto* callee = direct_callee.function; callee != nullptr) {
    if (!callee->is_declaration) {
      return PreparedCallWrapperKind::SameModule;
    }
    if (callee->is_variadic || call.is_variadic) {
      return PreparedCallWrapperKind::DirectExternVariadic;
    }
    return PreparedCallWrapperKind::DirectExternFixedArity;
  }

  return call.is_variadic ? PreparedCallWrapperKind::DirectExternVariadic
                          : PreparedCallWrapperKind::DirectExternFixedArity;
}

[[nodiscard]] std::size_t variadic_fpr_arg_register_count(const bir::CallInst& call) {
  if (!call.is_variadic) {
    return 0;
  }

  std::size_t count = 0;
  for (const auto& arg_abi : call.arg_abi) {
    if (arg_abi.passed_in_register && arg_abi.primary_class == bir::AbiValueClass::Sse) {
      ++count;
    }
  }
  return count;
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

[[nodiscard]] PreparedStorageEncodingKind storage_encoding_from_move_storage_kind(
    PreparedMoveStorageKind kind) {
  switch (kind) {
    case PreparedMoveStorageKind::Register:
      return PreparedStorageEncodingKind::Register;
    case PreparedMoveStorageKind::StackSlot:
      return PreparedStorageEncodingKind::FrameSlot;
    case PreparedMoveStorageKind::None:
      return PreparedStorageEncodingKind::None;
  }
  return PreparedStorageEncodingKind::None;
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

[[nodiscard]] PreparedMoveStorageKind move_storage_kind_from_storage_encoding(
    PreparedStorageEncodingKind encoding) {
  switch (encoding) {
    case PreparedStorageEncodingKind::Register:
      return PreparedMoveStorageKind::Register;
    case PreparedStorageEncodingKind::FrameSlot:
      return PreparedMoveStorageKind::StackSlot;
    case PreparedStorageEncodingKind::None:
    case PreparedStorageEncodingKind::Immediate:
    case PreparedStorageEncodingKind::ComputedAddress:
    case PreparedStorageEncodingKind::SymbolAddress:
      return PreparedMoveStorageKind::None;
  }
  return PreparedMoveStorageKind::None;
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

[[nodiscard]] std::optional<PreparedIndirectCalleePlan> build_indirect_callee_plan(
    const PreparedNameTables& names,
    const c4c::TargetProfile& target_profile,
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueLocationFunction* value_locations,
    const bir::CallInst& call) {
  if (!call.is_indirect || !call.callee_value.has_value()) {
    return std::nullopt;
  }

  const auto value_name_id = maybe_named_value_id(names, *call.callee_value);
  if (!value_name_id.has_value()) {
    return std::nullopt;
  }

  PreparedIndirectCalleePlan plan{
      .value_name = *value_name_id,
      .value_id = std::nullopt,
      .encoding = PreparedStorageEncodingKind::None,
      .bank = published_bank_for_value(regalloc_function, *value_name_id, call.callee_value->type),
      .register_name = std::nullopt,
      .slot_id = std::nullopt,
      .stack_offset_bytes = std::nullopt,
      .immediate_i32 = std::nullopt,
      .pointer_base_value_name = std::nullopt,
      .pointer_byte_delta = std::nullopt,
      .register_placement = std::nullopt,
  };

  if (value_locations == nullptr) {
    return plan;
  }

  if (const auto* home = find_prepared_value_home(*value_locations, *value_name_id); home != nullptr) {
    plan.value_id = home->value_id;
    plan.encoding = storage_encoding_from_home(*home);
    plan.register_name = home->register_name;
    plan.slot_id = home->slot_id;
    plan.stack_offset_bytes = home->offset_bytes;
    plan.immediate_i32 = home->immediate_i32;
    plan.pointer_base_value_name = home->pointer_base_value_name;
    plan.pointer_byte_delta = home->pointer_byte_delta;
    if (home->register_name.has_value() && regalloc_function != nullptr) {
      if (const auto* regalloc_value =
              find_regalloc_value_by_name(*regalloc_function, *value_name_id);
          regalloc_value != nullptr && regalloc_value->assigned_register.has_value() &&
          home->register_name ==
              std::optional<std::string>{regalloc_value->assigned_register->register_name}) {
        plan.register_placement =
            assignment_register_placement(target_profile, *regalloc_value->assigned_register);
      }
    }
  }

  return plan;
}

[[nodiscard]] std::optional<PreparedMemoryReturnPlan> build_memory_return_plan(
    const PreparedNameTables& names,
    const bir::NameTables& bir_names,
    const PreparedStackLayout& stack_layout,
    FunctionNameId function_name,
    const bir::CallInst& call) {
  if (!call.result_abi.has_value() || !call.result_abi->returned_in_memory ||
      !call.sret_storage_name.has_value()) {
    return std::nullopt;
  }

  PreparedMemoryReturnPlan plan{
      .sret_arg_index = std::nullopt,
      .storage_slot_name = kInvalidSlotName,
      .encoding = PreparedStorageEncodingKind::None,
      .slot_id = std::nullopt,
      .stack_offset_bytes = std::nullopt,
      .size_bytes = 0,
      .align_bytes = 0,
  };

  for (std::size_t arg_index = 0; arg_index < call.arg_abi.size(); ++arg_index) {
    if (call.arg_abi[arg_index].sret_pointer) {
      plan.sret_arg_index = arg_index;
      plan.size_bytes = call.arg_abi[arg_index].size_bytes;
      plan.align_bytes = call.arg_abi[arg_index].align_bytes;
      break;
    }
  }

  const std::string_view sret_storage_spelling =
      call.sret_storage_name_id == kInvalidSlotName
          ? std::string_view(*call.sret_storage_name)
          : bir_names.slot_names.spelling(call.sret_storage_name_id);
  const SlotNameId slot_name_id = names.slot_names.find(sret_storage_spelling);
  if (slot_name_id == kInvalidSlotName) {
    return plan;
  }
  plan.storage_slot_name = slot_name_id;

  const auto storage_spelling = names.slot_names.spelling(slot_name_id);
  const auto aggregate_slot_matches_storage =
      [&](SlotNameId candidate_slot_name) -> std::optional<std::size_t> {
    const auto candidate = names.slot_names.spelling(candidate_slot_name);
    if (storage_spelling.empty() ||
        candidate.size() <= storage_spelling.size() + 1 ||
        candidate.compare(0, storage_spelling.size(), storage_spelling) != 0 ||
        candidate[storage_spelling.size()] != '.') {
      return std::nullopt;
    }
    std::size_t byte_offset = 0;
    const auto suffix = candidate.substr(storage_spelling.size() + 1);
    const auto* begin = suffix.data();
    const auto* end = begin + suffix.size();
    const auto parsed = std::from_chars(begin, end, byte_offset);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
      return std::nullopt;
    }
    return byte_offset;
  };

  for (const auto& object : stack_layout.objects) {
    if (object.function_name != function_name || object.slot_name != slot_name_id) {
      continue;
    }
    if (plan.size_bytes == 0) {
      plan.size_bytes = object.size_bytes;
    }
    if (plan.align_bytes == 0) {
      plan.align_bytes = object.align_bytes;
    }
    if (const auto* frame_slot = find_prepared_frame_slot(stack_layout, object.object_id);
        frame_slot != nullptr) {
      plan.encoding = PreparedStorageEncodingKind::FrameSlot;
      plan.slot_id = frame_slot->slot_id;
      plan.stack_offset_bytes = frame_slot->offset_bytes;
    }
    break;
  }
  if (plan.encoding == PreparedStorageEncodingKind::None) {
    const PreparedFrameSlot* selected_slot = nullptr;
    std::optional<std::size_t> selected_byte_offset;
    for (const auto& object : stack_layout.objects) {
      if (object.function_name != function_name || !object.slot_name.has_value()) {
        continue;
      }
      const auto byte_offset = aggregate_slot_matches_storage(*object.slot_name);
      if (!byte_offset.has_value() ||
          (selected_byte_offset.has_value() && *byte_offset >= *selected_byte_offset)) {
        continue;
      }
      if (const auto* frame_slot = find_prepared_frame_slot(stack_layout, object.object_id);
          frame_slot != nullptr) {
        selected_slot = frame_slot;
        selected_byte_offset = byte_offset;
      }
    }
    if (selected_slot != nullptr) {
      plan.encoding = PreparedStorageEncodingKind::FrameSlot;
      plan.slot_id = selected_slot->slot_id;
      plan.stack_offset_bytes = selected_slot->offset_bytes;
    }
  }

  return plan;
}

[[nodiscard]] const PreparedAbiBinding* find_call_abi_binding(
    const PreparedMoveBundle* move_bundle,
    PreparedMoveDestinationKind destination_kind,
    std::optional<std::size_t> destination_abi_index) {
  if (move_bundle == nullptr) {
    return nullptr;
  }
  const PreparedAbiBinding* preferred = nullptr;
  for (const auto& binding : move_bundle->abi_bindings) {
    if (binding.destination_kind != destination_kind ||
        binding.destination_abi_index != destination_abi_index) {
      continue;
    }
    if (binding.destination_storage_kind == PreparedMoveStorageKind::StackSlot) {
      return &binding;
    }
    if (preferred == nullptr) {
      preferred = &binding;
    }
  }
  return preferred;
}

[[nodiscard]] std::optional<PreparedCallResultPlan> build_call_result_plan(
    const PreparedNameTables& names,
    const c4c::TargetProfile& target_profile,
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueLocationFunction* value_locations,
    const PreparedMoveBundle* after_call_bundle,
    std::size_t instruction_index,
    const bir::CallInst& call) {
  if (!call.result.has_value() || call.result->kind != bir::Value::Kind::Named ||
      value_locations == nullptr) {
    return std::nullopt;
  }

  PreparedCallResultPlan result_plan{
      .instruction_index = instruction_index,
      .value_bank = call.result_abi.has_value()
                        ? register_bank_from_result_abi(*call.result_abi)
                        : register_bank_from_type(call.result->type),
      .source_storage_kind = PreparedMoveStorageKind::None,
      .destination_storage_kind = PreparedMoveStorageKind::None,
      .destination_value_id = std::nullopt,
      .source_register_name = std::nullopt,
      .source_contiguous_width = 1,
      .source_occupied_register_names = {},
      .source_register_bank = std::nullopt,
      .source_stack_offset_bytes = std::nullopt,
      .destination_register_name = std::nullopt,
      .destination_contiguous_width = 1,
      .destination_occupied_register_names = {},
      .destination_register_bank = std::nullopt,
      .destination_slot_id = std::nullopt,
      .destination_stack_offset_bytes = std::nullopt,
      .source_register_placement = std::nullopt,
      .destination_register_placement = std::nullopt,
      .destination_spill_slot_placement = std::nullopt,
  };

  if (const auto* binding = find_call_abi_binding(after_call_bundle,
                                                  PreparedMoveDestinationKind::CallResultAbi,
                                                  std::nullopt);
      binding != nullptr) {
    result_plan.source_storage_kind = binding->destination_storage_kind;
    result_plan.source_register_name = binding->destination_register_name;
    result_plan.source_contiguous_width = binding->destination_contiguous_width;
    result_plan.source_occupied_register_names = binding->destination_occupied_register_names;
    result_plan.source_stack_offset_bytes = binding->destination_stack_offset_bytes;
    if (binding->destination_register_name.has_value()) {
      result_plan.source_register_bank = result_plan.value_bank;
      result_plan.source_register_placement = binding->destination_register_placement;
      if (!result_plan.source_register_placement.has_value() &&
          call.result_abi.has_value()) {
        result_plan.source_register_placement =
            call_result_destination_register_placement(target_profile,
                                                       *call.result_abi,
                                                       result_plan.source_contiguous_width);
      }
    }
  }

  if (const auto value_name_id = maybe_named_value_id(names, *call.result);
      value_name_id.has_value()) {
    if (const auto* home = find_prepared_value_home(*value_locations, *value_name_id);
        home != nullptr) {
      result_plan.destination_storage_kind = move_storage_kind_from_home(*home);
      result_plan.destination_register_name = home->register_name;
      result_plan.destination_slot_id = home->slot_id;
      result_plan.destination_stack_offset_bytes = home->offset_bytes;
      if (home->register_name.has_value()) {
        result_plan.destination_register_bank = result_plan.value_bank;
      }
      result_plan.destination_spill_slot_placement =
          make_spill_slot_placement(home->slot_id, home->offset_bytes);
    }
    if (regalloc_function != nullptr) {
      if (const auto* regalloc_value =
              find_regalloc_value_by_name(*regalloc_function, *value_name_id);
          regalloc_value != nullptr) {
        result_plan.destination_value_id = regalloc_value->value_id;
        if (regalloc_value->assigned_register.has_value() &&
            result_plan.destination_register_name ==
                std::optional<std::string>{regalloc_value->assigned_register->register_name}) {
          result_plan.destination_register_placement =
              assignment_register_placement(target_profile,
                                            *regalloc_value->assigned_register);
        }
      }
    }
  }

  return result_plan;
}

[[nodiscard]] const PreparedAbiBinding* find_call_register_abi_binding(
    const PreparedMoveBundle* move_bundle,
    PreparedMoveDestinationKind destination_kind,
    std::optional<std::size_t> destination_abi_index) {
  if (move_bundle == nullptr) {
    return nullptr;
  }
  for (const auto& binding : move_bundle->abi_bindings) {
    if (binding.destination_kind == destination_kind &&
        binding.destination_storage_kind == PreparedMoveStorageKind::Register &&
        binding.destination_abi_index == destination_abi_index &&
        binding.destination_register_name.has_value()) {
      return &binding;
    }
  }
  return nullptr;
}

[[nodiscard]] PreparedRegisterBank call_argument_destination_register_bank(
    const bir::CallInst& call,
    std::size_t arg_index,
    PreparedRegisterBank value_bank) {
  return arg_index < call.arg_abi.size() &&
                 call.arg_abi[arg_index].type == bir::TypeKind::Ptr &&
                 call.arg_abi[arg_index].sret_pointer
             ? PreparedRegisterBank::Gpr
             : value_bank;
}

[[nodiscard]] CallArgumentDestinationPlan plan_call_argument_destination(
    const c4c::TargetProfile& target_profile,
    const bir::CallInst& call,
    const PreparedMoveBundle* before_call_bundle,
    std::size_t arg_index,
    PreparedRegisterBank value_bank) {
  CallArgumentDestinationPlan destination;
  const auto abi_register_index =
      regalloc_detail::call_arg_abi_register_index(target_profile, call, arg_index);

  auto apply_register_binding = [&](const PreparedAbiBinding& binding) {
    destination.register_name = binding.destination_register_name;
    destination.contiguous_width = binding.destination_contiguous_width;
    destination.occupied_register_names = binding.destination_occupied_register_names;
    destination.register_bank = call_argument_destination_register_bank(call, arg_index, value_bank);
    destination.register_placement = binding.destination_register_placement;
    if (!destination.register_placement.has_value() &&
        arg_index < call.arg_abi.size() && abi_register_index.has_value()) {
      destination.register_placement =
          call_arg_destination_register_placement(target_profile,
                                                  call.arg_abi[arg_index],
                                                  *abi_register_index,
                                                  destination.contiguous_width);
    }
  };

  if (const auto* binding = find_call_abi_binding(before_call_bundle,
                                                  PreparedMoveDestinationKind::CallArgumentAbi,
                                                  arg_index);
      binding != nullptr) {
    destination.register_name = binding->destination_register_name;
    destination.contiguous_width = binding->destination_contiguous_width;
    destination.occupied_register_names = binding->destination_occupied_register_names;
    destination.stack_offset_bytes = binding->destination_stack_offset_bytes;
    if (binding->destination_register_name.has_value()) {
      apply_register_binding(*binding);
    }
  }
  if (!destination.register_name.has_value()) {
    if (const auto* register_binding =
            find_call_register_abi_binding(before_call_bundle,
                                           PreparedMoveDestinationKind::CallArgumentAbi,
                                           arg_index);
        register_binding != nullptr) {
      apply_register_binding(*register_binding);
    }
  }

  if (arg_index < call.arg_abi.size() &&
      destination.register_name.has_value() && abi_register_index.has_value()) {
    const std::size_t byval_lane_width =
        aarch64_byval_register_lane_width(target_profile, call.arg_abi[arg_index]);
    if (byval_lane_width > destination.contiguous_width) {
      auto occupied_register_names =
          regalloc_detail::call_arg_destination_register_names(
              target_profile,
              PreparedRegisterClass::General,
              *abi_register_index,
              *destination.register_name,
              byval_lane_width);
      if (occupied_register_names.empty()) {
        destination.register_name = std::nullopt;
        destination.contiguous_width = 1;
        destination.occupied_register_names.clear();
        destination.register_bank = std::nullopt;
        destination.register_placement = std::nullopt;
        destination.stack_offset_bytes =
            regalloc_detail::call_arg_destination_stack_offset_bytes(
                target_profile, call, arg_index);
      } else {
        destination.contiguous_width = byval_lane_width;
        destination.occupied_register_names = std::move(occupied_register_names);
        destination.register_placement =
            call_arg_destination_register_placement(target_profile,
                                                    call.arg_abi[arg_index],
                                                    *abi_register_index,
                                                    byval_lane_width);
      }
    }
  }

  return destination;
}

[[nodiscard]] CallArgumentSourcePlan plan_call_argument_source(
    const bir::Module& module,
    PreparedNameTables& names,
    const c4c::TargetProfile& target_profile,
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueLocationFunction* value_locations,
    const bir::Value& argument) {
  CallArgumentSourcePlan source;

  if (const auto value_name_id = maybe_named_value_id(names, argument);
      value_name_id.has_value() && value_locations != nullptr) {
    if (const auto* home = find_prepared_value_home(*value_locations, *value_name_id);
        home != nullptr) {
      source.encoding = storage_encoding_from_home(*home);
      source.register_name = home->register_name;
      source.slot_id = home->slot_id;
      source.stack_offset_bytes = home->offset_bytes;
      if (home->immediate_i32.has_value()) {
        source.literal = bir::Value::immediate_i32(*home->immediate_i32);
      }
      source.base_value_name = home->pointer_base_value_name;
      source.pointer_byte_delta = home->pointer_byte_delta;
      if (home->pointer_base_value_name.has_value()) {
        if (const auto* base_home =
                find_prepared_value_home(*value_locations, *home->pointer_base_value_name);
            base_home != nullptr) {
          source.base_value_id = base_home->value_id;
        } else if (regalloc_function != nullptr) {
          if (const auto* base_regalloc_value =
                  find_regalloc_value_by_name(*regalloc_function, *home->pointer_base_value_name);
              base_regalloc_value != nullptr) {
            source.base_value_id = base_regalloc_value->value_id;
          }
        }
      }
    }
    if (regalloc_function != nullptr) {
      if (const auto* regalloc_value =
              find_regalloc_value_by_name(*regalloc_function, *value_name_id);
          regalloc_value != nullptr) {
        source.value_id = regalloc_value->value_id;
        source.register_bank = register_bank_from_class(regalloc_value->register_class);
        if (regalloc_value->assigned_register.has_value() &&
            source.register_name ==
                std::optional<std::string>{regalloc_value->assigned_register->register_name}) {
          source.register_placement =
              assignment_register_placement(target_profile, *regalloc_value->assigned_register);
        }
      }
    }
    const auto symbol_name = resolve_symbol_pointer_name(module, argument);
    if (symbol_name.has_value()) {
      source.encoding = PreparedStorageEncodingKind::SymbolAddress;
      if (argument.pointer_symbol_link_name_id != kInvalidLinkName) {
        source.symbol_name_id = names.link_names.intern(*symbol_name);
        source.symbol_name = display_symbol_pointer_name(*symbol_name);
      } else {
        source.symbol_name = std::string(*symbol_name);
      }
    }
  } else if (const auto symbol_name = resolve_symbol_pointer_name(module, argument);
             symbol_name.has_value()) {
    source.encoding = PreparedStorageEncodingKind::SymbolAddress;
    if (argument.pointer_symbol_link_name_id != kInvalidLinkName) {
      source.symbol_name_id = names.link_names.intern(*symbol_name);
      source.symbol_name = display_symbol_pointer_name(*symbol_name);
    } else {
      source.symbol_name = std::string(*symbol_name);
    }
  } else if (argument.kind != bir::Value::Kind::Named) {
    source.encoding = PreparedStorageEncodingKind::Immediate;
    source.literal = argument;
    if (regalloc_function != nullptr) {
      if (const auto* constant_value =
              find_f128_constant_regalloc_value(*regalloc_function, argument);
          constant_value != nullptr) {
        source.value_id = constant_value->value_id;
        source.register_bank = register_bank_from_class(constant_value->register_class);
      }
    }
  }

  return source;
}

void append_call_clobbered_register_spans(
    std::vector<PreparedClobberedRegister>& clobbers,
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class,
    std::size_t contiguous_width) {
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
}

[[nodiscard]] std::vector<PreparedClobberedRegister> build_call_clobber_set(
    const c4c::TargetProfile& target_profile,
    const PreparedRegallocFunction* regalloc_function) {
  std::vector<PreparedClobberedRegister> clobbers;
  for (PreparedRegisterClass reg_class :
       {PreparedRegisterClass::General, PreparedRegisterClass::Float, PreparedRegisterClass::Vector}) {
    append_call_clobbered_register_spans(clobbers, target_profile, reg_class, 1);
    if (regalloc_function == nullptr) {
      continue;
    }
    for (const auto& value : regalloc_function->values) {
      if (value.register_class != reg_class || value.register_group_width <= 1) {
        continue;
      }
      append_call_clobbered_register_spans(
          clobbers, target_profile, reg_class, value.register_group_width);
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

[[nodiscard]] std::vector<PreparedCallPreservedValue> build_call_preserved_values(
    const PreparedBirModule& prepared,
    const PreparedFramePlanFunction* frame_plan,
    const PreparedLivenessFunction* liveness_function,
    const CallPreservationCandidates& candidates,
    const PreparedValueHomeLookup& value_home_lookup,
    std::size_t program_point) {
  std::vector<PreparedCallPreservedValue> preserved_values;
  if (liveness_function == nullptr) {
    return preserved_values;
  }
  preserved_values.reserve(candidates.by_start_point.size());

  for (const auto* value_ptr : candidates.by_start_point) {
    const auto& value = *value_ptr;
    if (program_point <= value.live_interval->start_point) {
      break;
    }
    if (program_point >= value.live_interval->end_point) {
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
        find_prepared_value_home(value_home_lookup, value.value_id);
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

    if (preserved.route != PreparedCallPreservationRoute::Unknown) {
      preserved_values.push_back(std::move(preserved));
    }
  }

  const auto by_value_id =
      [](const PreparedCallPreservedValue& lhs,
         const PreparedCallPreservedValue& rhs) {
        return lhs.value_id < rhs.value_id;
      };
  if (!std::is_sorted(preserved_values.begin(), preserved_values.end(), by_value_id)) {
    std::sort(preserved_values.begin(), preserved_values.end(), by_value_id);
  }
  return preserved_values;
}

[[nodiscard]] std::vector<PreparedCallPreservedValue> build_call_preserved_values(
    const PreparedBirModule& prepared,
    const PreparedFramePlanFunction* frame_plan,
    const PreparedLivenessFunction* liveness_function,
    const CallPreservationCandidates& candidates,
    const PreparedValueHomeLookup& value_home_lookup,
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
                                     candidates,
                                     value_home_lookup,
                                     *call_point);
}

}  // namespace

[[nodiscard]] const PreparedCallArgumentPlan* find_call_boundary_argument_plan(
    const PreparedCallPlan& call_plan,
    const PreparedMoveResolution& move) {
  if (!move.destination_abi_index.has_value()) {
    return nullptr;
  }
  for (const auto& argument : call_plan.arguments) {
    if (argument.arg_index == *move.destination_abi_index &&
        ((!argument.source_value_id.has_value() &&
          argument.source_encoding == PreparedStorageEncodingKind::Immediate) ||
         argument.source_value_id == std::optional<PreparedValueId>{move.from_value_id})) {
      return &argument;
    }
  }

  for (const auto& argument : call_plan.arguments) {
    if (argument.arg_index != *move.destination_abi_index ||
        argument.source_encoding != PreparedStorageEncodingKind::FrameSlot ||
        move.destination_storage_kind != PreparedMoveStorageKind::StackSlot ||
        argument.destination_stack_offset_bytes != move.destination_stack_offset_bytes) {
      continue;
    }
    const bool no_register_destination =
        !argument.destination_register_name.has_value() &&
        !argument.destination_register_bank.has_value() &&
        !argument.destination_register_placement.has_value() &&
        !move.destination_register_name.has_value() &&
        !move.destination_register_placement.has_value();
    if (no_register_destination) {
      return &argument;
    }
  }
  return nullptr;
}

[[nodiscard]] bool call_boundary_binding_matches_move(
    const PreparedAbiBinding& binding,
    const PreparedMoveResolution& move) {
  return binding.destination_kind == move.destination_kind &&
         binding.destination_storage_kind == move.destination_storage_kind &&
         binding.destination_abi_index == move.destination_abi_index &&
         binding.destination_register_name == move.destination_register_name &&
         binding.destination_register_placement == move.destination_register_placement;
}

[[nodiscard]] const PreparedAbiBinding* find_call_boundary_abi_binding(
    const PreparedMoveBundle& bundle,
    const PreparedMoveResolution& move) {
  if (move.destination_abi_index.has_value() &&
      *move.destination_abi_index < bundle.abi_bindings.size()) {
    const auto& indexed_binding = bundle.abi_bindings[*move.destination_abi_index];
    if (call_boundary_binding_matches_move(indexed_binding, move)) {
      return &indexed_binding;
    }
  }
  for (const auto& binding : bundle.abi_bindings) {
    if (call_boundary_binding_matches_move(binding, move)) {
      return &binding;
    }
  }
  return nullptr;
}

[[nodiscard]] bool call_boundary_result_plan_matches_move(
    const PreparedCallPlan& call_plan,
    const PreparedCallResultPlan& result_plan,
    const PreparedMoveResolution& move) {
  return result_plan.instruction_index == call_plan.instruction_index &&
         (!result_plan.destination_value_id.has_value() ||
          result_plan.destination_value_id ==
              std::optional<PreparedValueId>{move.to_value_id});
}

void populate_call_plans(PreparedBirModule& prepared) {
  prepared.call_plans.functions.clear();

  for (const auto& function : prepared.module.functions) {
    if (function.is_declaration) {
      continue;
    }
    const FunctionNameId function_name_id = prepared.names.function_names.find(function.name);
    if (function_name_id == kInvalidFunctionName) {
      continue;
    }

    PreparedCallPlansFunction function_plan{
        .function_name = function_name_id,
        .calls = {},
    };
    const auto* frame_plan = find_prepared_frame_plan(prepared, function_name_id);
    const auto* regalloc_function = find_regalloc_function(prepared.regalloc, function_name_id);
    const auto* liveness_function = find_liveness_function(prepared.liveness, function_name_id);
    const auto* value_locations = find_prepared_value_location_function(prepared, function_name_id);
    const PreparedValueHomeLookup value_home_lookup =
        make_prepared_value_home_lookup(value_locations);
    const CallPreservationCandidates call_preservation_candidates =
        make_call_preservation_candidates(regalloc_function);
    const std::vector<PreparedClobberedRegister> call_clobbers =
        build_call_clobber_set(prepared.target_profile, regalloc_function);

    for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
      const auto& block = function.blocks[block_index];
      for (std::size_t instruction_index = 0; instruction_index < block.insts.size(); ++instruction_index) {
        const auto* call = std::get_if<bir::CallInst>(&block.insts[instruction_index]);
        if (call == nullptr) {
          continue;
        }

        const DirectCalleeResolution direct_callee = resolve_direct_callee(prepared.module, *call);
        PreparedCallPlan call_plan{
            .block_index = block_index,
            .instruction_index = instruction_index,
            .wrapper_kind = classify_call_wrapper_kind(*call, direct_callee),
            .variadic_fpr_arg_register_count = variadic_fpr_arg_register_count(*call),
            .is_indirect = call->is_indirect,
            .direct_callee_name =
                direct_callee.name.has_value()
                    ? std::optional<std::string>{std::string{*direct_callee.name}}
                    : std::nullopt,
            .indirect_callee =
                build_indirect_callee_plan(prepared.names,
                                           prepared.target_profile,
                                           regalloc_function,
                                           value_locations,
                                           *call),
            .memory_return =
                build_memory_return_plan(prepared.names,
                                         prepared.module.names,
                                         prepared.stack_layout,
                                         function_name_id,
                                         *call),
            .arguments = {},
            .result = std::nullopt,
            .preserved_values = build_call_preserved_values(
                prepared,
                frame_plan,
                liveness_function,
                call_preservation_candidates,
                value_home_lookup,
                block_index,
                instruction_index),
            .clobbered_registers = call_clobbers,
        };

        const PreparedMoveBundle* before_call_bundle =
            value_locations == nullptr
                ? nullptr
                : find_prepared_move_bundle(
                      *value_locations, PreparedMovePhase::BeforeCall, block_index, instruction_index);
        const PreparedMoveBundle* after_call_bundle =
            value_locations == nullptr
                ? nullptr
                : find_prepared_move_bundle(
                      *value_locations, PreparedMovePhase::AfterCall, block_index, instruction_index);

        for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
          PreparedCallArgumentPlan arg_plan{
              .instruction_index = instruction_index,
              .arg_index = arg_index,
              .value_bank = arg_index < call->arg_abi.size()
                                ? register_bank_from_arg_abi(call->arg_abi[arg_index])
                                : register_bank_from_type(call->arg_types[arg_index]),
              .source_encoding = PreparedStorageEncodingKind::None,
              .source_value_id = std::nullopt,
              .source_base_value_id = std::nullopt,
              .source_literal = std::nullopt,
              .source_symbol_name = std::nullopt,
              .source_symbol_name_id = std::nullopt,
              .source_register_name = std::nullopt,
              .source_slot_id = std::nullopt,
              .source_stack_offset_bytes = std::nullopt,
              .source_register_bank = std::nullopt,
              .source_base_value_name = std::nullopt,
              .source_pointer_byte_delta = std::nullopt,
              .destination_register_name = std::nullopt,
              .destination_contiguous_width = 1,
              .destination_occupied_register_names = {},
              .destination_register_bank = std::nullopt,
              .destination_stack_offset_bytes = std::nullopt,
              .source_register_placement = std::nullopt,
              .destination_register_placement = std::nullopt,
          };
          const CallArgumentDestinationPlan destination =
              plan_call_argument_destination(prepared.target_profile,
                                             *call,
                                             before_call_bundle,
                                             arg_index,
                                             arg_plan.value_bank);
          arg_plan.destination_register_name = destination.register_name;
          arg_plan.destination_contiguous_width = destination.contiguous_width;
          arg_plan.destination_occupied_register_names =
              destination.occupied_register_names;
          arg_plan.destination_register_bank = destination.register_bank;
          arg_plan.destination_stack_offset_bytes = destination.stack_offset_bytes;
          arg_plan.destination_register_placement = destination.register_placement;

          const CallArgumentSourcePlan source =
              plan_call_argument_source(prepared.module,
                                        prepared.names,
                                        prepared.target_profile,
                                        regalloc_function,
                                        value_locations,
                                        call->args[arg_index]);
          arg_plan.source_encoding = source.encoding;
          arg_plan.source_value_id = source.value_id;
          arg_plan.source_base_value_id = source.base_value_id;
          arg_plan.source_literal = source.literal;
          arg_plan.source_symbol_name = source.symbol_name;
          arg_plan.source_symbol_name_id = source.symbol_name_id;
          arg_plan.source_register_name = source.register_name;
          arg_plan.source_slot_id = source.slot_id;
          arg_plan.source_stack_offset_bytes = source.stack_offset_bytes;
          arg_plan.source_register_bank = source.register_bank;
          arg_plan.source_base_value_name = source.base_value_name;
          arg_plan.source_pointer_byte_delta = source.pointer_byte_delta;
          arg_plan.source_register_placement = source.register_placement;

          call_plan.arguments.push_back(std::move(arg_plan));
        }

        call_plan.result = build_call_result_plan(prepared.names,
                                                  prepared.target_profile,
                                                  regalloc_function,
                                                  value_locations,
                                                  after_call_bundle,
                                                  instruction_index,
                                                  *call);

        function_plan.calls.push_back(std::move(call_plan));
      }
    }
    if (!function_plan.calls.empty()) {
      prepared.call_plans.functions.push_back(std::move(function_plan));
    }
  }
}

bool prepared_call_boundary_move_classification_available(
    const PreparedCallBoundaryMoveClassification& classification) {
  return classification.status == PreparedCallBoundaryMoveClassificationStatus::Available;
}

PreparedCallBoundaryMoveClassification classify_prepared_call_boundary_move(
    const PreparedCallPlan& call_plan,
    const PreparedMoveBundle& bundle,
    const PreparedMoveResolution& move) {
  PreparedCallBoundaryMoveClassification classification{
      .call_plan = &call_plan,
      .bundle = &bundle,
      .move = &move,
      .phase = bundle.phase,
      .destination_kind = move.destination_kind,
      .storage_kind = move.destination_storage_kind,
      .abi_index = move.destination_abi_index,
  };

  if (move.op_kind != PreparedMoveResolutionOpKind::Move) {
    classification.status =
        PreparedCallBoundaryMoveClassificationStatus::UnsupportedOpKind;
    return classification;
  }

  switch (move.destination_kind) {
    case PreparedMoveDestinationKind::CallArgumentAbi:
      if (!move.destination_abi_index.has_value()) {
        classification.status =
            PreparedCallBoundaryMoveClassificationStatus::MissingAbiIndex;
        return classification;
      }
      classification.argument_plan =
          find_call_boundary_argument_plan(call_plan, move);
      if (classification.argument_plan == nullptr) {
        classification.status =
            PreparedCallBoundaryMoveClassificationStatus::MissingCallArgumentPlan;
        return classification;
      }
      classification.abi_binding = find_call_boundary_abi_binding(bundle, move);
      if (classification.abi_binding == nullptr) {
        classification.status =
            PreparedCallBoundaryMoveClassificationStatus::MissingAbiBinding;
        return classification;
      }
      return classification;

    case PreparedMoveDestinationKind::CallResultAbi:
      classification.abi_binding = find_call_boundary_abi_binding(bundle, move);
      classification.result_plan =
          call_plan.result.has_value() ? &*call_plan.result : nullptr;
      if (classification.result_plan == nullptr) {
        classification.status =
            PreparedCallBoundaryMoveClassificationStatus::MissingCallResultPlan;
        return classification;
      }
      if (!call_boundary_result_plan_matches_move(
              call_plan, *classification.result_plan, move)) {
        classification.status =
            PreparedCallBoundaryMoveClassificationStatus::MismatchedCallResultPlan;
        return classification;
      }
      if (classification.abi_binding == nullptr) {
        classification.status =
            PreparedCallBoundaryMoveClassificationStatus::MissingAbiBinding;
        return classification;
      }
      return classification;

    case PreparedMoveDestinationKind::FunctionReturnAbi:
    case PreparedMoveDestinationKind::Value:
      return classification;
  }
  return classification;
}

namespace {

[[nodiscard]] PreparedCallBoundaryEffectEndpoint make_value_effect_endpoint(
    std::optional<PreparedValueId> value_id,
    ValueNameId value_name = kInvalidValueName) {
  return PreparedCallBoundaryEffectEndpoint{
      .value_id = value_id,
      .value_name = value_name,
  };
}

[[nodiscard]] PreparedCallBoundaryEffectEndpoint make_argument_source_endpoint(
    const PreparedCallArgumentPlan& argument) {
  return PreparedCallBoundaryEffectEndpoint{
      .encoding = argument.source_encoding,
      .storage_kind = move_storage_kind_from_storage_encoding(argument.source_encoding),
      .value_id = argument.source_value_id,
      .value_name = argument.source_base_value_name.value_or(kInvalidValueName),
      .register_bank = argument.source_register_bank,
      .contiguous_width = 1,
      .slot_id = argument.source_slot_id,
      .stack_offset_bytes = argument.source_stack_offset_bytes,
  };
}

[[nodiscard]] PreparedCallBoundaryEffectEndpoint make_argument_destination_endpoint(
    const PreparedCallArgumentPlan& argument,
    const PreparedMoveResolution& move) {
  return PreparedCallBoundaryEffectEndpoint{
      .encoding = storage_encoding_from_move_storage_kind(move.destination_storage_kind),
      .storage_kind = move.destination_storage_kind,
      .register_bank = argument.destination_register_bank,
      .contiguous_width = argument.destination_contiguous_width,
      .stack_offset_bytes = argument.destination_stack_offset_bytes.has_value()
                                ? argument.destination_stack_offset_bytes
                                : move.destination_stack_offset_bytes,
  };
}

[[nodiscard]] PreparedCallBoundaryEffectEndpoint make_result_source_endpoint(
    const PreparedCallResultPlan& result) {
  return PreparedCallBoundaryEffectEndpoint{
      .encoding = storage_encoding_from_move_storage_kind(result.source_storage_kind),
      .storage_kind = result.source_storage_kind,
      .register_bank = result.source_register_bank,
      .contiguous_width = result.source_contiguous_width,
      .stack_offset_bytes = result.source_stack_offset_bytes,
  };
}

[[nodiscard]] PreparedCallBoundaryEffectEndpoint make_result_destination_endpoint(
    const PreparedCallResultPlan& result) {
  return PreparedCallBoundaryEffectEndpoint{
      .encoding = storage_encoding_from_move_storage_kind(result.destination_storage_kind),
      .storage_kind = result.destination_storage_kind,
      .value_id = result.destination_value_id,
      .register_bank = result.destination_register_bank,
      .contiguous_width = result.destination_contiguous_width,
      .slot_id = result.destination_slot_id,
      .stack_offset_bytes = result.destination_stack_offset_bytes,
  };
}

[[nodiscard]] PreparedCallBoundaryEffectEndpoint make_preserved_storage_endpoint(
    const PreparedCallPreservedValue& preserved) {
  const PreparedMoveStorageKind storage_kind =
      preserved.route == PreparedCallPreservationRoute::CalleeSavedRegister
          ? PreparedMoveStorageKind::Register
          : preserved.route == PreparedCallPreservationRoute::StackSlot
                ? PreparedMoveStorageKind::StackSlot
                : PreparedMoveStorageKind::None;
  return PreparedCallBoundaryEffectEndpoint{
      .encoding = storage_encoding_from_move_storage_kind(storage_kind),
      .storage_kind = storage_kind,
      .value_id = preserved.value_id,
      .value_name = preserved.value_name,
      .register_name = preserved.register_name,
      .register_bank = preserved.register_bank,
      .contiguous_width = preserved.contiguous_width,
      .occupied_register_names = preserved.occupied_register_names,
      .slot_id = preserved.slot_id,
      .stack_offset_bytes = preserved.stack_offset_bytes,
      .stack_size_bytes = preserved.stack_size_bytes,
      .stack_align_bytes = preserved.stack_align_bytes,
      .callee_saved_save_index = preserved.callee_saved_save_index,
      .register_placement = preserved.register_placement,
      .spill_slot_placement = preserved.spill_slot_placement,
  };
}

void append_explicit_call_boundary_effects(
    std::vector<PreparedCallBoundaryEffectPlan>& effects,
    const PreparedCallPlan& call_plan,
    const PreparedMoveBundle* bundle) {
  if (bundle == nullptr) {
    return;
  }
  for (const auto& move : bundle->moves) {
    const auto classification =
        classify_prepared_call_boundary_move(call_plan, *bundle, move);
    PreparedCallBoundaryEffectPlan effect{
        .effect_kind = PreparedCallBoundaryEffectKind::ExplicitMove,
        .phase = bundle->phase,
        .block_index = call_plan.block_index,
        .instruction_index = call_plan.instruction_index,
        .order_index = effects.size(),
        .classification_status = classification.status,
        .destination_kind = move.destination_kind,
        .storage_kind = move.destination_storage_kind,
        .abi_index = move.destination_abi_index,
        .source = make_value_effect_endpoint(move.from_value_id),
        .destination = make_value_effect_endpoint(move.to_value_id),
        .reason = move.reason,
    };
    effect.destination.encoding =
        storage_encoding_from_move_storage_kind(move.destination_storage_kind);
    effect.destination.storage_kind = move.destination_storage_kind;
    effect.destination.stack_offset_bytes = move.destination_stack_offset_bytes;
    if (classification.argument_plan != nullptr) {
      effect.source = make_argument_source_endpoint(*classification.argument_plan);
      effect.destination =
          make_argument_destination_endpoint(*classification.argument_plan, move);
    } else if (classification.result_plan != nullptr) {
      effect.source = make_result_source_endpoint(*classification.result_plan);
      effect.destination = make_result_destination_endpoint(*classification.result_plan);
    }
    effects.push_back(std::move(effect));
  }
}

void append_preservation_call_boundary_effects(
    std::vector<PreparedCallBoundaryEffectPlan>& effects,
    const PreparedCallPlan& call_plan,
    PreparedMovePhase phase,
    PreparedCallBoundaryEffectKind effect_kind,
    std::string reason) {
  for (const auto& preserved : call_plan.preserved_values) {
    if (preserved.route == PreparedCallPreservationRoute::Unknown) {
      continue;
    }
    const auto value_endpoint =
        make_value_effect_endpoint(preserved.value_id, preserved.value_name);
    const auto storage_endpoint = make_preserved_storage_endpoint(preserved);
    effects.push_back(PreparedCallBoundaryEffectPlan{
        .effect_kind = effect_kind,
        .phase = phase,
        .block_index = call_plan.block_index,
        .instruction_index = call_plan.instruction_index,
        .order_index = effects.size(),
        .classification_status =
            PreparedCallBoundaryMoveClassificationStatus::Available,
        .destination_kind = PreparedMoveDestinationKind::Value,
        .storage_kind = storage_endpoint.storage_kind,
        .source = effect_kind ==
                      PreparedCallBoundaryEffectKind::PreservationHomePopulation
                      ? value_endpoint
                      : storage_endpoint,
        .destination = effect_kind ==
                           PreparedCallBoundaryEffectKind::PreservationHomePopulation
                           ? storage_endpoint
                           : value_endpoint,
        .preservation_route = preserved.route,
        .reason = reason,
    });
  }
}

}  // namespace

std::vector<PreparedCallBoundaryEffectPlan> plan_prepared_call_boundary_effects(
    const PreparedCallPlan& call_plan,
    const PreparedMoveBundle* before_call_bundle,
    const PreparedMoveBundle* after_call_bundle) {
  std::vector<PreparedCallBoundaryEffectPlan> effects;
  const std::size_t before_move_count =
      before_call_bundle == nullptr ? 0 : before_call_bundle->moves.size();
  const std::size_t after_move_count =
      after_call_bundle == nullptr ? 0 : after_call_bundle->moves.size();
  effects.reserve(before_move_count + after_move_count +
                  (call_plan.preserved_values.size() * 2U));

  append_explicit_call_boundary_effects(effects, call_plan, before_call_bundle);
  append_preservation_call_boundary_effects(
      effects,
      call_plan,
      PreparedMovePhase::BeforeCall,
      PreparedCallBoundaryEffectKind::PreservationHomePopulation,
      "preservation_home_population");
  append_explicit_call_boundary_effects(effects, call_plan, after_call_bundle);
  append_preservation_call_boundary_effects(
      effects,
      call_plan,
      PreparedMovePhase::AfterCall,
      PreparedCallBoundaryEffectKind::PreservationRepublication,
      "preservation_republication");

  return effects;
}

}  // namespace c4c::backend::prepare
