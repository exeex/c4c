#include "call_plans.hpp"
#include "calls.hpp"
#include "select_chain_lookups.hpp"
#include "regalloc/call_return_abi.hpp"
#include "target_register_profile.hpp"
#include "variadic.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
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
    case bir::TypeKind::I16:
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

[[nodiscard]] PreparedRegisterBank direct_bir_call_arg_bank_display(bir::TypeKind type) {
  return register_bank_from_type(type);
}

[[nodiscard]] PreparedRegisterBank direct_bir_call_result_bank_display(bir::TypeKind type) {
  return register_bank_from_type(type);
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

[[nodiscard]] std::size_t scalar_stack_argument_size_bytes(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return 1;
    case bir::TypeKind::I32:
    case bir::TypeKind::F32:
      return 4;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F64:
      return 8;
    case bir::TypeKind::I128:
    case bir::TypeKind::F128:
      return 16;
    case bir::TypeKind::Void:
      return 0;
  }
  return 0;
}

[[nodiscard]] std::size_t align_stack_argument_size_bytes(std::size_t value,
                                                          std::size_t alignment) {
  if (alignment == 0) {
    return value;
  }
  const std::size_t remainder = value % alignment;
  return remainder == 0 ? value : value + (alignment - remainder);
}

[[nodiscard]] std::size_t prepared_call_stack_argument_size_bytes(
    const bir::CallArgAbiInfo& abi) {
  if (abi.aarch64_hfa_lane_count > 0) {
    return std::max<std::size_t>(scalar_stack_argument_size_bytes(abi.type), 1);
  }
  return align_stack_argument_size_bytes(std::max<std::size_t>(abi.size_bytes, 8), 8);
}

[[nodiscard]] std::optional<PreparedOutgoingStackArgumentArea>
compute_outgoing_stack_argument_area(
    const std::vector<PreparedCallArgumentPlan>& arguments) {
  std::size_t size_bytes = 0;
  for (const auto& argument : arguments) {
    if (!argument.destination_stack_offset_bytes.has_value() ||
        !argument.destination_stack_size_bytes.has_value() ||
        *argument.destination_stack_size_bytes == 0) {
      continue;
    }
    size_bytes = std::max(size_bytes,
                          *argument.destination_stack_offset_bytes +
                              *argument.destination_stack_size_bytes);
  }
  if (size_bytes == 0) {
    return std::nullopt;
  }
  return PreparedOutgoingStackArgumentArea{.size_bytes = size_bytes};
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
  std::optional<std::size_t> stack_size_bytes;
  std::optional<PreparedRegisterPlacement> register_placement;
};

struct CallArgumentSourcePlan {
  PreparedStorageEncodingKind encoding = PreparedStorageEncodingKind::None;
  std::optional<PreparedValueId> value_id;
  std::optional<ValueNameId> value_name;
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

[[nodiscard]] const PreparedMemoryAccess* find_unique_memory_access_by_result_name(
    const PreparedAddressingFunction* addressing,
    ValueNameId result_value_name) {
  if (addressing == nullptr || result_value_name == kInvalidValueName) {
    return nullptr;
  }
  const PreparedMemoryAccess* matched = nullptr;
  for (const auto& access : addressing->accesses) {
    if (access.result_value_name != std::optional<ValueNameId>{result_value_name}) {
      continue;
    }
    if (matched != nullptr) {
      return nullptr;
    }
    matched = &access;
  }
  return matched;
}

[[nodiscard]] BlockLabelId prepared_block_label_for_index(
    const PreparedControlFlowFunction* control_flow,
    const bir::Function& function,
    std::size_t block_index) {
  if (control_flow != nullptr && block_index < control_flow->blocks.size()) {
    const BlockLabelId prepared_label =
        control_flow->blocks[block_index].block_label;
    if (prepared_label == kInvalidBlockLabel) {
      return kInvalidBlockLabel;
    }
    if (block_index >= function.blocks.size()) {
      return prepared_label;
    }
    const BlockLabelId bir_label = function.blocks[block_index].label_id;
    if (bir_label != kInvalidBlockLabel && bir_label == prepared_label) {
      return bir_label;
    }
    return prepared_label;
  }
  if (block_index < function.blocks.size()) {
    return function.blocks[block_index].label_id;
  }
  return kInvalidBlockLabel;
}

[[nodiscard]] bool local_frame_address_name_matches(std::string_view source_name,
                                                    std::string_view candidate_name) {
  return candidate_name == source_name ||
         (candidate_name.size() == source_name.size() + 2 &&
          candidate_name.compare(0, source_name.size(), source_name) == 0 &&
          candidate_name.substr(source_name.size()) == ".0");
}

struct LocalFrameAddressDerivedSource {
  ValueNameId base_value_name = kInvalidValueName;
  std::int64_t byte_delta = 0;
};

[[nodiscard]] std::optional<std::int64_t> immediate_pointer_byte_delta(
    const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  switch (value.type) {
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
      return value.immediate;
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<LocalFrameAddressDerivedSource>
find_same_block_local_frame_address_derived_source(const PreparedNameTables& names,
                                                   const bir::Function& function,
                                                   std::size_t block_index,
                                                   std::size_t instruction_index,
                                                   ValueNameId source_value_name) {
  if (source_value_name == kInvalidValueName ||
      block_index >= function.blocks.size()) {
    return std::nullopt;
  }
  const auto& block = function.blocks[block_index];
  const std::size_t end = std::min(instruction_index, block.insts.size());
  for (std::size_t inst_index = 0; inst_index < end; ++inst_index) {
    const auto* binary = std::get_if<bir::BinaryInst>(&block.insts[inst_index]);
    if (binary == nullptr || binary->result.type != bir::TypeKind::Ptr ||
        maybe_named_value_id(names, binary->result) !=
            std::optional<ValueNameId>{source_value_name}) {
      continue;
    }

    if (binary->lhs.type == bir::TypeKind::Ptr &&
        binary->lhs.kind == bir::Value::Kind::Named) {
      const auto base = maybe_named_value_id(names, binary->lhs);
      const auto delta = immediate_pointer_byte_delta(binary->rhs);
      if (base.has_value() && delta.has_value()) {
        if (binary->opcode == bir::BinaryOpcode::Add) {
          return LocalFrameAddressDerivedSource{.base_value_name = *base,
                                                .byte_delta = *delta};
        }
        if (binary->opcode == bir::BinaryOpcode::Sub) {
          return LocalFrameAddressDerivedSource{.base_value_name = *base,
                                                .byte_delta = -*delta};
        }
      }
    }

    if (binary->opcode == bir::BinaryOpcode::Add &&
        binary->rhs.type == bir::TypeKind::Ptr &&
        binary->rhs.kind == bir::Value::Kind::Named) {
      const auto base = maybe_named_value_id(names, binary->rhs);
      const auto delta = immediate_pointer_byte_delta(binary->lhs);
      if (base.has_value() && delta.has_value()) {
        return LocalFrameAddressDerivedSource{.base_value_name = *base,
                                              .byte_delta = *delta};
      }
    }
  }
  return std::nullopt;
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

[[nodiscard]] PreparedCallBoundaryEffectEndpoint make_preservation_value_source_endpoint(
    const c4c::TargetProfile& target_profile,
    const PreparedRegallocValue& value,
    const PreparedValueHome* value_home,
    bool prefer_value_home) {
  PreparedCallBoundaryEffectEndpoint endpoint{
      .value_id = value.value_id,
      .value_name = value.value_name,
      .contiguous_width = std::max<std::size_t>(value.register_group_width, 1),
  };
  if (prefer_value_home && value_home != nullptr) {
    endpoint.encoding = storage_encoding_from_home(*value_home);
    endpoint.storage_kind = move_storage_kind_from_home(*value_home);
    if (value_home->kind == PreparedValueHomeKind::Register &&
        value_home->register_name.has_value()) {
      endpoint.register_name = value_home->register_name;
      endpoint.register_bank = register_bank_from_class(value.register_class);
      endpoint.contiguous_width = 1;
      endpoint.occupied_register_names = {*value_home->register_name};
      endpoint.register_placement = find_register_placement(
          target_profile,
          value.register_class,
          endpoint.contiguous_width,
          endpoint.occupied_register_names);
      return endpoint;
    }
    if (value_home->kind == PreparedValueHomeKind::StackSlot) {
      endpoint.slot_id = value_home->slot_id;
      endpoint.stack_offset_bytes = value_home->offset_bytes;
      endpoint.stack_size_bytes = value_home->size_bytes;
      endpoint.stack_align_bytes = value_home->align_bytes;
      endpoint.spill_slot_placement =
          make_spill_slot_placement(value_home->slot_id, value_home->offset_bytes);
      return endpoint;
    }
  }
  if (value.assigned_register.has_value()) {
    endpoint.encoding = PreparedStorageEncodingKind::Register;
    endpoint.storage_kind = PreparedMoveStorageKind::Register;
    endpoint.register_name = value.assigned_register->register_name;
    endpoint.register_bank = register_bank_from_class(value.register_class);
    endpoint.contiguous_width = value.assigned_register->contiguous_width;
    endpoint.occupied_register_names =
        value.assigned_register->occupied_register_names;
    endpoint.register_placement = value.assigned_register->placement;
    return endpoint;
  }
  if (value_home == nullptr) {
    return endpoint;
  }

  endpoint.encoding = storage_encoding_from_home(*value_home);
  endpoint.storage_kind = move_storage_kind_from_home(*value_home);
  endpoint.slot_id = value_home->slot_id;
  endpoint.stack_offset_bytes = value_home->offset_bytes;
  endpoint.stack_size_bytes = value_home->size_bytes;
  endpoint.stack_align_bytes = value_home->align_bytes;
  return endpoint;
}

[[nodiscard]] PreparedCallBoundaryEffectEndpoint make_preservation_destination_endpoint(
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

[[nodiscard]] std::string make_preservation_reason(
    const PreparedCallPreservedValue& preserved) {
  if (preserved.route == PreparedCallPreservationRoute::CalleeSavedRegister) {
    return "callee_saved_register_preservation";
  }
  if (preserved.route == PreparedCallPreservationRoute::StackSlot &&
      preserved.preservation_source.storage_kind == PreparedMoveStorageKind::Register) {
    return "caller_saved_clobber_reuse_stack_preservation";
  }
  if (preserved.route == PreparedCallPreservationRoute::StackSlot) {
    return "stack_slot_preservation";
  }
  return {};
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
                        : direct_bir_call_result_bank_display(call.result->type),
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
  } else if (!call.result_lanes.empty() &&
             call.result_abi.has_value() &&
             call.result_lanes.front().kind == call.result->kind &&
             call.result_lanes.front().name == call.result->name &&
             call.result_lanes.front().type == call.result->type) {
    const auto source_register_name =
        call_result_destination_register_name(target_profile, *call.result_abi);
    if (source_register_name.has_value()) {
      result_plan.source_storage_kind = PreparedMoveStorageKind::Register;
      result_plan.source_register_name = source_register_name;
      result_plan.source_contiguous_width = 1;
      result_plan.source_occupied_register_names = {*source_register_name};
      result_plan.source_register_bank = result_plan.value_bank;
      result_plan.source_register_placement =
          call_result_destination_register_placement(target_profile,
                                                     *call.result_abi,
                                                     result_plan.source_contiguous_width);
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

[[nodiscard]] bir::TypeKind call_argument_type(const bir::CallInst& call,
                                               std::size_t arg_index) {
  if (arg_index < call.arg_types.size() &&
      call.arg_types[arg_index] != bir::TypeKind::Void) {
    return call.arg_types[arg_index];
  }
  if (arg_index < call.args.size()) {
    return call.args[arg_index].type;
  }
  return bir::TypeKind::Void;
}

[[nodiscard]] PreparedRegisterBank call_argument_value_bank(
    const bir::CallInst& call,
    std::size_t arg_index) {
  PreparedRegisterBank bank = PreparedRegisterBank::None;
  if (arg_index < call.arg_abi.size()) {
    bank = register_bank_from_arg_abi(call.arg_abi[arg_index]);
  } else {
    bank = direct_bir_call_arg_bank_display(call_argument_type(call, arg_index));
  }
  if (bank == PreparedRegisterBank::None &&
      call_argument_type(call, arg_index) == bir::TypeKind::I16) {
    return PreparedRegisterBank::Gpr;
  }
  return bank;
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

[[nodiscard]] bool call_argument_allows_local_aggregate_address_publication(
    const bir::CallInst& call,
    std::size_t arg_index) {
  if (arg_index >= call.args.size()) {
    return false;
  }
  if (call_is_runtime_intrinsic_placeholder(call)) {
    return false;
  }
  if (arg_index < call.arg_abi.size() &&
      call.arg_abi[arg_index].type == bir::TypeKind::Ptr &&
      call.arg_abi[arg_index].byval_copy) {
    return true;
  }
  if (arg_index < call.arg_types.size()) {
    return call.arg_types[arg_index] == bir::TypeKind::Ptr;
  }
  return call.args[arg_index].type == bir::TypeKind::Ptr;
}

[[nodiscard]] CallArgumentDestinationPlan plan_call_argument_destination(
    const c4c::TargetProfile& target_profile,
    const bir::CallInst& call,
    PreparedCallWrapperKind wrapper_kind,
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
    if (destination.register_placement.has_value() &&
        destination.register_placement->bank == PreparedRegisterBank::None &&
        destination.register_bank == PreparedRegisterBank::Gpr) {
      destination.register_placement->bank = PreparedRegisterBank::Gpr;
    }
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
    if (destination.stack_offset_bytes.has_value() && arg_index < call.arg_abi.size()) {
      destination.stack_size_bytes =
          prepared_call_stack_argument_size_bytes(call.arg_abi[arg_index]);
    }
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
  if (!destination.register_name.has_value() &&
      wrapper_kind == PreparedCallWrapperKind::SameModule &&
      value_bank == PreparedRegisterBank::Gpr &&
      call_argument_type(call, arg_index) == bir::TypeKind::I16 &&
      abi_register_index.has_value()) {
    if (const auto abi = infer_call_arg_abi(target_profile, bir::TypeKind::I16);
        abi.has_value()) {
      destination.register_name =
          call_arg_destination_register_name(target_profile, *abi, *abi_register_index);
      if (destination.register_name.has_value()) {
        destination.contiguous_width = 1;
        destination.occupied_register_names = {*destination.register_name};
        destination.register_bank = PreparedRegisterBank::Gpr;
        destination.register_placement =
            call_arg_destination_register_placement(target_profile,
                                                    *abi,
                                                    *abi_register_index,
                                                    destination.contiguous_width);
        if (destination.register_placement.has_value() &&
            destination.register_placement->bank == PreparedRegisterBank::None) {
          destination.register_placement->bank = PreparedRegisterBank::Gpr;
        }
      }
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
        if (destination.stack_offset_bytes.has_value()) {
          destination.stack_size_bytes =
              prepared_call_stack_argument_size_bytes(call.arg_abi[arg_index]);
        }
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
    source.value_name = value_name_id;
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

[[nodiscard]] PreparedCallArgumentDirectGlobalSelectChainDependency
plan_call_argument_direct_global_select_chain_dependency(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block& block,
    const bir::Value& argument,
    std::size_t before_instruction_index,
    std::optional<ValueNameId> source_value_name) {
  PreparedCallArgumentDirectGlobalSelectChainDependency dependency;
  if (!source_value_name.has_value() || *source_value_name == kInvalidValueName) {
    return dependency;
  }
  auto direct_global_dependency =
      find_prepared_direct_global_select_chain_dependency(names,
                                                         source_producers,
                                                         block_label,
                                                         &block,
                                                         argument,
                                                         before_instruction_index);
  if (!prepared_direct_global_select_chain_dependency_available(
          direct_global_dependency)) {
    return dependency;
  }
  dependency.available = true;
  dependency.source_value_name = *source_value_name;
  dependency.direct_global_dependency = direct_global_dependency;
  return dependency;
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
      preserved.preservation_source =
          make_preservation_value_source_endpoint(
              prepared.target_profile,
              value,
              value_home,
              preserved.route == PreparedCallPreservationRoute::CalleeSavedRegister);
      preserved.preservation_destination =
          make_preservation_destination_endpoint(preserved);
      preserved.preservation_reason = make_preservation_reason(preserved);
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

void copy_home_source_selection_fields(PreparedCallArgumentSourceSelection& selection,
                                       const PreparedValueHome* source_home) {
  if (source_home == nullptr) {
    return;
  }
  selection.source_value_id = source_home->value_id;
  selection.source_value_name = source_home->value_name;
  selection.source_home_kind = source_home->kind;
  selection.source_slot_id = source_home->slot_id;
  selection.source_stack_offset_bytes = source_home->offset_bytes;
  selection.source_size_bytes = source_home->size_bytes;
  selection.source_align_bytes = source_home->align_bytes;
  selection.source_pointer_byte_delta = source_home->pointer_byte_delta;
}

void copy_access_source_selection_fields(PreparedCallArgumentSourceSelection& selection,
                                         const PreparedMemoryAccess& access,
                                         const PreparedStackLayout& stack_layout) {
  if (access.address.base_kind != PreparedAddressBaseKind::FrameSlot ||
      !access.address.frame_slot_id.has_value()) {
    return;
  }
  const PreparedFrameSlot* slot = nullptr;
  for (const auto& frame_slot : stack_layout.frame_slots) {
    if (frame_slot.slot_id == *access.address.frame_slot_id) {
      slot = &frame_slot;
      break;
    }
  }
  selection.source_slot_id = access.address.frame_slot_id;
  if (slot != nullptr) {
    selection.source_stack_offset_bytes =
        static_cast<std::size_t>(static_cast<std::int64_t>(slot->offset_bytes) +
                                 access.address.byte_offset);
  }
  selection.source_size_bytes = access.address.size_bytes;
  selection.source_align_bytes = access.address.align_bytes;
}

[[nodiscard]] const PreparedFrameSlot* find_call_plan_frame_slot_by_id(
    const PreparedStackLayout& stack_layout,
    PreparedFrameSlotId slot_id) {
  for (const auto& frame_slot : stack_layout.frame_slots) {
    if (frame_slot.slot_id == slot_id) {
      return &frame_slot;
    }
  }
  return nullptr;
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

[[nodiscard]] std::optional<std::size_t> aggregate_result_suffix_offset(
    std::string_view result_name,
    std::string_view aggregate_name,
    std::string_view marker) {
  if (aggregate_name.empty() || marker.empty() ||
      result_name.size() <= aggregate_name.size() + marker.size() ||
      result_name.compare(0, aggregate_name.size(), aggregate_name) != 0 ||
      result_name.compare(aggregate_name.size(), marker.size(), marker) != 0) {
    return std::nullopt;
  }
  std::size_t value = 0;
  for (std::size_t index = aggregate_name.size() + marker.size();
       index < result_name.size();
       ++index) {
    const char ch = result_name[index];
    if (ch < '0' || ch > '9') {
      return std::nullopt;
    }
    value = value * 10 + static_cast<std::size_t>(ch - '0');
  }
  return value;
}

struct ByvalPayloadLaneStore {
  std::size_t source_offset = 0;
  std::size_t stack_offset = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  PreparedFrameSlotId slot_id = 0;
  std::optional<ValueNameId> source_value_name;
  std::optional<std::size_t> source_instruction_index;
};

[[nodiscard]] std::optional<ByvalPayloadLaneStore> byval_payload_lane_store(
    const PreparedStackLayout& stack_layout,
    const PreparedMemoryAccess& access,
    std::size_t source_offset,
    std::optional<ValueNameId> source_value_name = std::nullopt,
    std::optional<std::size_t> source_instruction_index = std::nullopt) {
  if (access.address.base_kind != PreparedAddressBaseKind::FrameSlot ||
      !access.address.frame_slot_id.has_value() ||
      access.address.size_bytes == 0) {
    return std::nullopt;
  }
  const auto* slot =
      find_call_plan_frame_slot_by_id(stack_layout, *access.address.frame_slot_id);
  if (slot == nullptr) {
    return std::nullopt;
  }
  const auto stack_offset = static_cast<std::int64_t>(slot->offset_bytes) +
                            access.address.byte_offset;
  if (stack_offset < 0) {
    return std::nullopt;
  }
  return ByvalPayloadLaneStore{
      .source_offset = source_offset,
      .stack_offset = static_cast<std::size_t>(stack_offset),
      .size_bytes = access.address.size_bytes,
      .align_bytes = access.address.align_bytes,
      .slot_id = *access.address.frame_slot_id,
      .source_value_name = source_value_name,
      .source_instruction_index = source_instruction_index,
  };
}

[[nodiscard]] std::optional<ByvalPayloadLaneStore> select_contiguous_byval_payload_lane(
    std::vector<ByvalPayloadLaneStore> stores,
    std::size_t payload_size_bytes) {
  if (stores.empty()) {
    return std::nullopt;
  }
  std::sort(stores.begin(),
            stores.end(),
            [](const ByvalPayloadLaneStore& lhs, const ByvalPayloadLaneStore& rhs) {
              if (lhs.source_offset != rhs.source_offset) {
                return lhs.source_offset < rhs.source_offset;
              }
              return lhs.stack_offset < rhs.stack_offset;
            });
  const auto& first = stores.front();
  if (first.source_offset != 0) {
    return std::nullopt;
  }
  std::size_t covered = 0;
  for (const auto& store : stores) {
    if (store.source_offset < covered) {
      continue;
    }
    if (store.source_offset != covered ||
        store.stack_offset != first.stack_offset + store.source_offset) {
      return std::nullopt;
    }
    covered = store.source_offset + store.size_bytes;
    if (covered >= payload_size_bytes) {
      return first;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::vector<PreparedAggregateTransportChunk>
collect_byval_stack_copy_store_chunks(const PreparedStackLayout& stack_layout,
                                      const bir::Function& function,
                                      const PreparedAddressingFunction* addressing,
                                      BlockLabelId block_label,
                                      std::size_t block_index,
                                      std::size_t call_instruction_index,
                                      std::string_view aggregate_name,
                                      std::size_t copy_size_bytes) {
  std::vector<PreparedAggregateTransportChunk> chunks;
  if (addressing == nullptr || block_label == kInvalidBlockLabel ||
      aggregate_name.empty() || copy_size_bytes == 0 ||
      block_index >= function.blocks.size()) {
    return chunks;
  }

  std::vector<bool> covered(copy_size_bytes, false);
  const auto& block = function.blocks[block_index];
  const auto end_index = std::min(call_instruction_index, block.insts.size());
  for (std::size_t index = 0; index < end_index; ++index) {
    const auto* store = std::get_if<bir::StoreLocalInst>(&block.insts[index]);
    if (store == nullptr) {
      continue;
    }
    const auto payload_offset =
        aggregate_slot_suffix_offset(store->slot_name, aggregate_name);
    if (!payload_offset.has_value() || *payload_offset >= copy_size_bytes) {
      continue;
    }
    const auto* access = find_prepared_memory_access(*addressing, block_label, index);
    if (access == nullptr) {
      continue;
    }
    const auto payload_store =
        byval_payload_lane_store(stack_layout, *access, *payload_offset);
    if (!payload_store.has_value() ||
        payload_store->size_bytes > copy_size_bytes - *payload_offset) {
      continue;
    }
    bool overlaps = false;
    for (std::size_t byte = 0; byte < payload_store->size_bytes; ++byte) {
      if (covered[*payload_offset + byte]) {
        overlaps = true;
        break;
      }
    }
    if (overlaps) {
      return {};
    }
    for (std::size_t byte = 0; byte < payload_store->size_bytes; ++byte) {
      covered[*payload_offset + byte] = true;
    }
    chunks.push_back(PreparedAggregateTransportChunk{
        .chunk_index = chunks.size(),
        .kind = PreparedAggregateTransportChunkKind::RequiredPayload,
        .payload_offset_bytes = *payload_offset,
        .source_offset_bytes = payload_store->stack_offset,
        .destination_offset_bytes = *payload_offset,
        .size_bytes = payload_store->size_bytes,
        .align_bytes = payload_store->align_bytes,
        .preferred_width_bytes = payload_store->size_bytes,
        .fallback_width_bytes = {},
    });
  }
  for (bool byte_covered : covered) {
    if (!byte_covered) {
      return {};
    }
  }
  std::sort(chunks.begin(),
            chunks.end(),
            [](const PreparedAggregateTransportChunk& lhs,
               const PreparedAggregateTransportChunk& rhs) {
              return lhs.payload_offset_bytes < rhs.payload_offset_bytes;
            });
  for (std::size_t index = 0; index < chunks.size(); ++index) {
    chunks[index].chunk_index = index;
  }
  return chunks;
}

[[nodiscard]] std::optional<std::size_t> byval_payload_load_source_offset(
    std::string_view result_name,
    std::string_view aggregate_name) {
  if (const auto source_offset = aggregate_result_suffix_offset(
          result_name, aggregate_name, ".array.aggregate.load.");
      source_offset.has_value()) {
    return source_offset;
  }
  return aggregate_result_suffix_offset(
      result_name, aggregate_name, ".global.aggregate.load.");
}

[[nodiscard]] std::optional<ByvalPayloadLaneStore> select_byval_payload_lane_load_source(
    const PreparedStackLayout& stack_layout,
    const bir::Block& block,
    const PreparedAddressingFunction& addressing,
    BlockLabelId block_label,
    std::size_t call_instruction_index,
    std::string_view aggregate_name,
    std::size_t payload_size_bytes) {
  std::vector<ByvalPayloadLaneStore> stores;
  const auto end_index = std::min(call_instruction_index, block.insts.size());
  for (std::size_t index = 0; index < end_index; ++index) {
    const auto* load = std::get_if<bir::LoadLocalInst>(&block.insts[index]);
    if (load == nullptr || load->result.name.empty()) {
      continue;
    }
    const auto source_offset =
        byval_payload_load_source_offset(load->result.name, aggregate_name);
    if (!source_offset.has_value()) {
      continue;
    }
    const auto* access = find_prepared_memory_access(addressing, block_label, index);
    if (access == nullptr) {
      continue;
    }
    auto payload_store =
        byval_payload_lane_store(stack_layout,
                                 *access,
                                 *source_offset,
                                 access->result_value_name,
                                 index);
    if (payload_store.has_value()) {
      stores.push_back(*payload_store);
    }
  }
  return select_contiguous_byval_payload_lane(std::move(stores), payload_size_bytes);
}

[[nodiscard]] std::optional<ByvalPayloadLaneStore> select_byval_payload_lane_source(
    const PreparedNameTables& names,
    const PreparedStackLayout& stack_layout,
    const bir::Function& function,
    const PreparedAddressingFunction* addressing,
    BlockLabelId block_label,
    std::size_t block_index,
    std::size_t call_instruction_index,
    ValueNameId source_value_name,
    std::size_t payload_size_bytes) {
  if (addressing == nullptr || block_label == kInvalidBlockLabel ||
      block_index >= function.blocks.size() || source_value_name == kInvalidValueName ||
      payload_size_bytes == 0) {
    return std::nullopt;
  }
  const auto aggregate_name = names.value_names.spelling(source_value_name);
  if (aggregate_name.empty()) {
    return std::nullopt;
  }

  const auto& block = function.blocks[block_index];
  if (const auto load_source =
          select_byval_payload_lane_load_source(stack_layout,
                                                block,
                                                *addressing,
                                                block_label,
                                                call_instruction_index,
                                                aggregate_name,
                                                payload_size_bytes);
      load_source.has_value()) {
    return load_source;
  }

  std::vector<ByvalPayloadLaneStore> stores;
  const auto end_index = std::min(call_instruction_index, block.insts.size());
  for (std::size_t index = 0; index < end_index; ++index) {
    const auto* store = std::get_if<bir::StoreLocalInst>(&block.insts[index]);
    if (store == nullptr) {
      continue;
    }
    const auto source_offset =
        aggregate_slot_suffix_offset(store->slot_name, aggregate_name);
    if (!source_offset.has_value()) {
      continue;
    }
    const auto* access = find_prepared_memory_access(*addressing, block_label, index);
    if (access == nullptr) {
      continue;
    }
    auto payload_store =
        byval_payload_lane_store(stack_layout, *access, *source_offset);
    if (!payload_store.has_value()) {
      continue;
    }
    stores.push_back(*payload_store);
  }
  return select_contiguous_byval_payload_lane(std::move(stores), payload_size_bytes);
}

[[nodiscard]] const PreparedAddressMaterialization*
find_latest_frame_slot_materialization(const PreparedAddressingFunction* addressing,
                                       const PreparedNameTables& names,
                                       BlockLabelId block_label,
                                       std::size_t instruction_index,
                                       ValueNameId source_value_name,
                                       bool allow_lane_name) {
  if (addressing == nullptr || block_label == kInvalidBlockLabel ||
      source_value_name == kInvalidValueName) {
    return nullptr;
  }
  const auto source_name = names.value_names.spelling(source_value_name);
  if (source_name.empty()) {
    return nullptr;
  }
  const PreparedAddressMaterialization* selected = nullptr;
  for (const auto& materialization : addressing->address_materializations) {
    if (materialization.block_label != block_label ||
        materialization.inst_index > instruction_index ||
        materialization.kind != PreparedAddressMaterializationKind::FrameSlot ||
        !materialization.frame_slot_id.has_value() ||
        !materialization.result_value_name.has_value()) {
      continue;
    }
    const auto materialized_name =
        names.value_names.spelling(*materialization.result_value_name);
    const bool matched = allow_lane_name
                             ? local_frame_address_name_matches(source_name, materialized_name)
                             : materialized_name == source_name;
    if (!matched) {
      continue;
    }
    if (selected != nullptr && selected->inst_index == materialization.inst_index) {
      if (selected->frame_slot_id != materialization.frame_slot_id ||
          selected->byte_offset != materialization.byte_offset ||
          selected->result_value_name != materialization.result_value_name) {
        return nullptr;
      }
      continue;
    }
    if (selected == nullptr || selected->inst_index < materialization.inst_index) {
      selected = &materialization;
    }
  }
  return selected;
}

void copy_materialization_source_selection_fields(
    PreparedCallArgumentSourceSelection& selection,
    const PreparedAddressMaterialization& materialization) {
  selection.address_materialization_block_label = materialization.block_label;
  selection.address_materialization_inst_index = materialization.inst_index;
  selection.address_materialization_frame_slot_id = materialization.frame_slot_id;
  selection.address_materialization_byte_offset = materialization.byte_offset;
  selection.source_slot_id = materialization.frame_slot_id;
  if (materialization.byte_offset >= 0) {
    selection.source_stack_offset_bytes =
        static_cast<std::size_t>(materialization.byte_offset);
  }
}

struct NoAddressingFrameAddressSource {
  const PreparedFrameSlot* slot = nullptr;
};

[[nodiscard]] NoAddressingFrameAddressSource
find_no_addressing_local_frame_address_source_compatibility(
    const PreparedNameTables& names,
    const PreparedStackLayout& stack_layout,
    FunctionNameId function_name,
    ValueNameId source_value_name) {
  const auto source_name = names.value_names.spelling(source_value_name);
  if (source_name.empty() || function_name == kInvalidFunctionName) {
    return {};
  }

  NoAddressingFrameAddressSource selected;
  for (const auto& object : stack_layout.objects) {
    if (object.function_name != function_name ||
        object.source_kind != "local_slot") {
      continue;
    }
    const auto object_name = prepared_stack_object_name(names, object);
    if (!local_frame_address_name_matches(source_name, object_name)) {
      continue;
    }
    const auto* slot = find_prepared_frame_slot(stack_layout, object.object_id);
    if (slot == nullptr) {
      continue;
    }
    if (selected.slot == nullptr || object_name == source_name ||
        slot->offset_bytes < selected.slot->offset_bytes) {
      selected.slot = slot;
      if (object_name == source_name) {
        break;
      }
    }
  }
  return selected;
}

[[nodiscard]] bool call_argument_is_source_pointer_operand(
    const PreparedNameTables& names,
    const bir::CallInst& call,
    const PreparedCallArgumentPlan& argument,
    const PreparedValueHome& source_home) {
  if (argument.arg_index >= call.args.size() ||
      call.args[argument.arg_index].type != bir::TypeKind::Ptr) {
    return false;
  }
  const auto source_name =
      maybe_named_value_id(names, call.args[argument.arg_index]);
  return source_name == std::optional<ValueNameId>{source_home.value_name};
}

[[nodiscard]] bool call_argument_uses_local_frame_address_operand(
    const PreparedNameTables& names,
    const bir::CallInst& call,
    const PreparedCallArgumentPlan& argument,
    const PreparedValueHome& source_home) {
  return call_argument_is_source_pointer_operand(names, call, argument, source_home) &&
         argument.source_value_id ==
             std::optional<PreparedValueId>{source_home.value_id} &&
         argument.destination_register_bank == PreparedRegisterBank::Gpr;
}

[[nodiscard]] bool copy_prior_preservation_source_selection_fields(
    PreparedCallArgumentSourceSelection& selection,
    const PreparedCallPreservedValue& preserved) {
  selection.source_value_id = preserved.value_id;
  selection.source_value_name = preserved.value_name;
  selection.preservation_route = preserved.route;
  selection.preserved_register_name = preserved.register_name;
  selection.preserved_register_bank = preserved.register_bank;
  selection.preserved_register_contiguous_width = preserved.contiguous_width;
  selection.preserved_occupied_register_names = preserved.occupied_register_names;
  selection.preserved_register_placement = preserved.register_placement;
  selection.preserved_stack_slot_id = preserved.slot_id;
  selection.preserved_stack_offset_bytes = preserved.stack_offset_bytes;
  selection.preserved_stack_size_bytes = preserved.stack_size_bytes;
  selection.preserved_stack_align_bytes = preserved.stack_align_bytes;
  selection.preserved_callee_saved_save_index = preserved.callee_saved_save_index;
  selection.preserved_spill_slot_placement = preserved.spill_slot_placement;

  if (preserved.route == PreparedCallPreservationRoute::CalleeSavedRegister) {
    return selection.preserved_register_name.has_value() &&
           selection.preserved_register_bank.has_value() &&
           selection.preserved_register_contiguous_width.has_value() &&
           *selection.preserved_register_contiguous_width != 0 &&
           !selection.preserved_occupied_register_names.empty() &&
           selection.preserved_register_placement.has_value();
  }
  return true;
}

[[nodiscard]] std::optional<std::size_t> prepared_byval_lane_extent_bytes(
    const PreparedCallPlan& call_plan,
    const PreparedCallArgumentPlan& argument,
    const PreparedMoveResolution& move,
    const PreparedValueHome* source_home,
    const bir::CallInst& call) {
  if (source_home == nullptr || argument.arg_index >= call.arg_abi.size()) {
    return std::nullopt;
  }
  const auto& abi = call.arg_abi[argument.arg_index];
  if (abi.type != bir::TypeKind::Ptr || !abi.byval_copy ||
      !abi.passed_in_register || abi.passed_on_stack ||
      abi.primary_class != bir::AbiValueClass::Integer ||
      abi.size_bytes == 0 || abi.size_bytes > 16) {
    return std::nullopt;
  }
  const std::size_t lane_count = std::max({
      std::size_t{1},
      move.destination_contiguous_width,
      argument.destination_contiguous_width,
      move.destination_occupied_register_names.size(),
      argument.destination_occupied_register_names.size(),
  });
  const bool explicit_byval_lane_move =
      move.reason == "call_arg_byval_aggregate_register_lanes";
  const bool direct_variadic_single_gpr_lane =
      !explicit_byval_lane_move &&
      call_plan.wrapper_kind == PreparedCallWrapperKind::DirectExternVariadic &&
      move.reason == "call_arg_register_to_register" &&
      move.destination_kind == PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == PreparedMoveStorageKind::Register &&
      move.destination_abi_index == std::optional<std::size_t>{argument.arg_index} &&
      argument.destination_register_bank ==
          std::optional<PreparedRegisterBank>{PreparedRegisterBank::Gpr} &&
      move.destination_register_name.has_value() &&
      argument.destination_register_name.has_value() &&
      lane_count == 1 && abi.size_bytes <= 8;
  if (!explicit_byval_lane_move && !direct_variadic_single_gpr_lane) {
    return std::nullopt;
  }
  std::size_t extent = source_home->size_bytes.value_or(abi.size_bytes);
  if (extent == 0) {
    extent = abi.size_bytes;
  }
  if (lane_count > 1 && extent <= 8) {
    extent *= lane_count;
  }
  extent = std::max(extent, abi.size_bytes);
  return extent <= 16 ? std::optional<std::size_t>{extent} : std::nullopt;
}

[[nodiscard]] const PreparedMoveResolution* find_before_call_argument_move(
    const PreparedMoveBundle* before_call_bundle,
    const PreparedCallArgumentPlan& argument) {
  if (before_call_bundle == nullptr) {
    return nullptr;
  }
  for (const auto& move : before_call_bundle->moves) {
    if (move.op_kind == PreparedMoveResolutionOpKind::Move &&
        move.destination_kind == PreparedMoveDestinationKind::CallArgumentAbi &&
        move.destination_abi_index == std::optional<std::size_t>{argument.arg_index}) {
      return &move;
    }
  }
  return nullptr;
}

[[nodiscard]] std::size_t count_call_instructions(const bir::Function& function) {
  std::size_t count = 0;
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (std::holds_alternative<bir::CallInst>(inst)) {
        ++count;
      }
    }
  }
  return count;
}

void append_incremental_prior_preserved_values(PreparedCallPlanLookups& lookups,
                                               const PreparedCallPlan& call_plan) {
  for (const auto& preserved : call_plan.preserved_values) {
    if (preserved.route == PreparedCallPreservationRoute::Unknown) {
      continue;
    }
    if (preserved.value_id >= lookups.prior_preserved_by_value.size()) {
      lookups.prior_preserved_by_value.resize(preserved.value_id + 1U);
    }
    lookups.prior_preserved_by_value[preserved.value_id].push_back(
        PreparedPriorPreservedValueEntry{
            .block_index = call_plan.block_index,
            .instruction_index = call_plan.instruction_index,
            .preserved = &preserved,
        });
  }
}

[[nodiscard]] bool call_has_preserved_value(const PreparedCallPlan& call_plan,
                                            PreparedValueId value_id) {
  return std::any_of(call_plan.preserved_values.begin(),
                     call_plan.preserved_values.end(),
                     [&](const PreparedCallPreservedValue& preserved) {
                       return preserved.value_id == value_id;
                     });
}

[[nodiscard]] bool preservation_can_seed_prior_call(
    const PreparedCallPreservedValue& preserved) {
  return preserved.route == PreparedCallPreservationRoute::CalleeSavedRegister &&
         preserved.preservation_source.storage_kind ==
             PreparedMoveStorageKind::Register &&
         preserved.preservation_source.register_bank.has_value() &&
         preserved.preservation_source.register_name.has_value() &&
         !preserved.preservation_source.register_name->empty() &&
         preserved.preservation_destination.storage_kind ==
             PreparedMoveStorageKind::Register &&
         preserved.preservation_destination.register_bank.has_value() &&
         preserved.preservation_destination.register_name.has_value() &&
         !preserved.preservation_destination.register_name->empty() &&
         preserved.contiguous_width == 1 &&
         preserved.occupied_register_names.size() == 1;
}

[[nodiscard]] bool call_argument_sources_preserved_value(
    const PreparedCallArgumentPlan& argument,
    const PreparedCallPreservedValue& preserved) {
  if (!argument.source_value_id.has_value() ||
      *argument.source_value_id != preserved.value_id ||
      argument.source_encoding != PreparedStorageEncodingKind::Register ||
      argument.source_register_bank != preserved.preservation_source.register_bank ||
      argument.source_register_name != preserved.preservation_source.register_name) {
    return false;
  }
  return true;
}

void seed_prior_call_preservation_from_later_dependency(
    PreparedCallPlansFunction& function_plan,
    const PreparedCallPreservedValue& preserved) {
  if (!preservation_can_seed_prior_call(preserved)) {
    return;
  }

  for (auto& prior_call : function_plan.calls) {
    if (call_has_preserved_value(prior_call, preserved.value_id)) {
      continue;
    }
    const auto argument_it =
        std::find_if(prior_call.arguments.begin(),
                     prior_call.arguments.end(),
                     [&](const PreparedCallArgumentPlan& argument) {
                       return call_argument_sources_preserved_value(argument, preserved);
                     });
    if (argument_it == prior_call.arguments.end()) {
      continue;
    }
    prior_call.preserved_values.push_back(preserved);
    std::sort(prior_call.preserved_values.begin(),
              prior_call.preserved_values.end(),
              [](const PreparedCallPreservedValue& lhs,
                 const PreparedCallPreservedValue& rhs) {
                return lhs.value_id < rhs.value_id;
              });
  }
}

[[nodiscard]] std::optional<PreparedCallArgumentSourceSelection>
select_prepared_call_argument_source(const PreparedBirModule& prepared,
                                     const PreparedNameTables& names,
                                     const PreparedCallPlan& call_plan,
                                     const PreparedCallPlanLookups& call_plan_lookups,
                                     const bir::Function& function,
                                     const bir::CallInst& call,
                                     const PreparedControlFlowFunction* control_flow,
                                     const PreparedAddressingFunction* addressing,
                                     const PreparedMoveBundle* before_call_bundle,
                                     const PreparedCallArgumentPlan& argument,
                                     const PreparedValueHome* source_home) {
  PreparedCallArgumentSourceSelection selection;
  selection.source_value_id = argument.source_value_id;
  selection.source_value_name = source_home != nullptr
                                    ? std::optional<ValueNameId>{source_home->value_name}
                                    : std::nullopt;
  selection.source_base_value_id = argument.source_base_value_id;
  selection.source_pointer_byte_delta = argument.source_pointer_byte_delta;
  copy_home_source_selection_fields(selection, source_home);

  const auto* move = find_before_call_argument_move(before_call_bundle, argument);
  if (move == nullptr) {
    return std::nullopt;
  }

  const auto block_label =
      prepared_block_label_for_index(control_flow, function, call_plan.block_index);
  const auto* source_access =
      selection.source_value_name.has_value()
          ? find_unique_memory_access_by_result_name(addressing, *selection.source_value_name)
          : nullptr;

  if (const auto byval_extent =
          prepared_byval_lane_extent_bytes(call_plan, argument, *move, source_home, call);
      byval_extent.has_value()) {
    selection.kind = PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane;
    selection.byval_lane_extent_bytes = byval_extent;
    selection.byval_lane_source_instruction_index = call_plan.instruction_index;
    const auto& abi = call.arg_abi[argument.arg_index];
    if (selection.source_value_name.has_value()) {
      const auto payload_source =
          select_byval_payload_lane_source(names,
                                           prepared.stack_layout,
                                           function,
                                           addressing,
                                           block_label,
                                           call_plan.block_index,
                                           call_plan.instruction_index,
                                           *selection.source_value_name,
                                           abi.size_bytes);
      if (payload_source.has_value()) {
        if (payload_source->source_value_name.has_value()) {
          selection.source_value_name = payload_source->source_value_name;
        }
        selection.source_slot_id = payload_source->slot_id;
        selection.source_stack_offset_bytes = payload_source->stack_offset;
        selection.source_size_bytes = byval_extent;
        selection.source_align_bytes = payload_source->align_bytes;
        if (payload_source->source_instruction_index.has_value()) {
          selection.byval_lane_source_instruction_index =
              payload_source->source_instruction_index;
        }
        return selection;
      }
    }
    if (source_access != nullptr) {
      copy_access_source_selection_fields(selection, *source_access, prepared.stack_layout);
    }
    return selection;
  }

  if (argument.source_encoding == PreparedStorageEncodingKind::FrameSlot) {
    if (call_plan.memory_return.has_value() &&
        call_plan.memory_return->sret_arg_index == std::optional<std::size_t>{argument.arg_index} &&
        call_plan.memory_return->slot_id.has_value() &&
        call_plan.memory_return->stack_offset_bytes.has_value()) {
      selection.kind = PreparedCallArgumentSourceSelectionKind::FrameSlotAddress;
      selection.source_slot_id = call_plan.memory_return->slot_id;
      selection.source_stack_offset_bytes = call_plan.memory_return->stack_offset_bytes;
      selection.source_size_bytes = call_plan.memory_return->size_bytes;
      selection.source_align_bytes = call_plan.memory_return->align_bytes;
      return selection;
    }
    if (source_home != nullptr) {
      const bool direct_local_frame_address_operand =
          call_argument_uses_local_frame_address_operand(
              names, call, argument, *source_home);
      if (const auto* materialization =
              find_latest_frame_slot_materialization(addressing,
                                                     names,
                                                     block_label,
                                                     call_plan.instruction_index,
                                                     source_home->value_name,
                                                     direct_local_frame_address_operand);
          materialization != nullptr) {
        selection.kind = PreparedCallArgumentSourceSelectionKind::FrameSlotAddress;
        copy_materialization_source_selection_fields(selection, *materialization);
        selection.source_size_bytes = source_home->size_bytes;
        selection.source_align_bytes = source_home->align_bytes;
        return selection;
      }
      if (addressing == nullptr &&
          direct_local_frame_address_operand) {
        const auto function_name =
            control_flow != nullptr ? control_flow->function_name
                                    : names.function_names.find(function.name);
        const auto local_source =
            find_no_addressing_local_frame_address_source_compatibility(
                names,
                prepared.stack_layout,
                function_name,
                source_home->value_name);
        if (local_source.slot != nullptr) {
          selection.kind = PreparedCallArgumentSourceSelectionKind::FrameSlotAddress;
          selection.source_slot_id = local_source.slot->slot_id;
          selection.source_stack_offset_bytes = local_source.slot->offset_bytes;
          selection.source_size_bytes =
              source_home->size_bytes.value_or(std::size_t{8});
          selection.source_align_bytes =
              source_home->align_bytes.value_or(std::size_t{8});
          return selection;
        }
      }
    }

    selection.kind = PreparedCallArgumentSourceSelectionKind::FrameSlotValue;
    if (source_access != nullptr) {
      copy_access_source_selection_fields(selection, *source_access, prepared.stack_layout);
    }
    if (!selection.source_slot_id.has_value()) {
      selection.source_slot_id = argument.source_slot_id;
    }
    if (!selection.source_stack_offset_bytes.has_value()) {
      selection.source_stack_offset_bytes = argument.source_stack_offset_bytes;
    }
    return selection.source_slot_id.has_value() ||
                   selection.source_stack_offset_bytes.has_value()
               ? std::optional<PreparedCallArgumentSourceSelection>{selection}
               : std::nullopt;
  }

  const bool byval_pointer_abi_argument =
      argument.arg_index < call.arg_abi.size() &&
      call.arg_abi[argument.arg_index].type == bir::TypeKind::Ptr &&
      call.arg_abi[argument.arg_index].byval_copy;

  if ((argument.allows_local_aggregate_address_publication ||
       byval_pointer_abi_argument) &&
      argument.source_encoding == PreparedStorageEncodingKind::Register &&
      source_home != nullptr &&
      source_home->kind == PreparedValueHomeKind::Register &&
      selection.source_value_name.has_value()) {
    const auto derived_source =
        find_same_block_local_frame_address_derived_source(names,
                                                           function,
                                                           call_plan.block_index,
                                                           call_plan.instruction_index,
                                                           *selection.source_value_name);
    const auto selected_source_value_name =
        derived_source.has_value() ? derived_source->base_value_name
                                   : *selection.source_value_name;
    const std::int64_t selected_source_delta =
        derived_source.has_value() ? derived_source->byte_delta : 0;
    if (const auto* materialization =
            find_latest_frame_slot_materialization(addressing,
                                                   names,
                                                   block_label,
                                                   call_plan.instruction_index,
                                                   selected_source_value_name,
                                                   true);
        materialization != nullptr) {
      selection.kind =
          PreparedCallArgumentSourceSelectionKind::LocalFrameAddressMaterialization;
      copy_materialization_source_selection_fields(selection, *materialization);
      selection.source_stack_offset_bytes = std::nullopt;
      selection.address_materialization_byte_offset =
          materialization->byte_offset + selected_source_delta;
      if (*selection.address_materialization_byte_offset >= 0) {
        selection.source_stack_offset_bytes = static_cast<std::size_t>(
            *selection.address_materialization_byte_offset);
      }
      selection.source_pointer_byte_delta = selected_source_delta;
      if (argument.arg_index < call.arg_abi.size() &&
          call.arg_abi[argument.arg_index].byval_copy &&
          call.arg_abi[argument.arg_index].size_bytes > 0) {
        selection.source_size_bytes = call.arg_abi[argument.arg_index].size_bytes;
        selection.source_align_bytes = call.arg_abi[argument.arg_index].align_bytes == 0
                                           ? source_home->align_bytes.value_or(std::size_t{8})
                                           : call.arg_abi[argument.arg_index].align_bytes;
      } else {
        selection.source_size_bytes = source_home->size_bytes.value_or(std::size_t{8});
        selection.source_align_bytes = source_home->align_bytes.value_or(std::size_t{8});
      }
      return selection.source_stack_offset_bytes.has_value()
                 ? std::optional<PreparedCallArgumentSourceSelection>{selection}
                 : std::nullopt;
    }
  }

  if ((argument.allows_local_aggregate_address_publication ||
       byval_pointer_abi_argument) &&
      argument.source_encoding == PreparedStorageEncodingKind::ComputedAddress &&
      source_home != nullptr &&
      source_home->kind == PreparedValueHomeKind::PointerBasePlusOffset &&
      source_home->pointer_base_value_name.has_value()) {
    const auto selected_source_value_name = *source_home->pointer_base_value_name;
    const auto selected_source_delta = source_home->pointer_byte_delta.value_or(0);
    if (const auto* materialization =
            find_latest_frame_slot_materialization(addressing,
                                                   names,
                                                   block_label,
                                                   call_plan.instruction_index,
                                                   selected_source_value_name,
                                                   true);
        materialization != nullptr) {
      selection.kind =
          PreparedCallArgumentSourceSelectionKind::LocalFrameAddressMaterialization;
      copy_materialization_source_selection_fields(selection, *materialization);
      selection.source_stack_offset_bytes = std::nullopt;
      selection.address_materialization_byte_offset =
          materialization->byte_offset + selected_source_delta;
      if (*selection.address_materialization_byte_offset >= 0) {
        selection.source_stack_offset_bytes =
            static_cast<std::size_t>(*selection.address_materialization_byte_offset);
      }
      selection.source_pointer_byte_delta = selected_source_delta;
      if (argument.arg_index < call.arg_abi.size() &&
          call.arg_abi[argument.arg_index].byval_copy &&
          call.arg_abi[argument.arg_index].size_bytes > 0) {
        selection.source_size_bytes = call.arg_abi[argument.arg_index].size_bytes;
        selection.source_align_bytes = call.arg_abi[argument.arg_index].align_bytes == 0
                                           ? source_home->align_bytes.value_or(std::size_t{8})
                                           : call.arg_abi[argument.arg_index].align_bytes;
      } else {
        selection.source_size_bytes = source_home->size_bytes.value_or(std::size_t{8});
        selection.source_align_bytes = source_home->align_bytes.value_or(std::size_t{8});
      }
      return selection.source_stack_offset_bytes.has_value()
                 ? std::optional<PreparedCallArgumentSourceSelection>{selection}
                 : std::nullopt;
    }
  }

  const auto value_id = argument.source_value_id.value_or(move->from_value_id);
  const auto preserved_lookup = find_unique_indexed_prior_preserved_value_source(
      call_plan_lookups, control_flow, call_plan, value_id);
  if (preserved_lookup.status == PreparedPriorPreservedValueLookupStatus::Found &&
      preserved_lookup.preserved != nullptr && preserved_lookup.entry != nullptr) {
    selection.kind = PreparedCallArgumentSourceSelectionKind::PriorPreservation;
    if (!copy_prior_preservation_source_selection_fields(selection,
                                                         *preserved_lookup.preserved)) {
      return std::nullopt;
    }
    selection.preserved_call_block_index = preserved_lookup.entry->block_index;
    selection.preserved_call_instruction_index = preserved_lookup.entry->instruction_index;
    return selection;
  }

  return std::nullopt;
}

[[nodiscard]] std::optional<PreparedAggregateTransportPlan>
plan_prepared_aggregate_transport(const PreparedNameTables& names,
                                  const PreparedStackLayout& stack_layout,
                                  const bir::Function& function,
                                  const PreparedCallPlan& call_plan,
                                  const PreparedAddressingFunction* addressing,
                                  BlockLabelId block_label,
                                  const bir::CallInst& call,
                                  const PreparedCallArgumentPlan& argument) {
  if (argument.arg_index >= call.arg_abi.size() ||
      !argument.source_selection.has_value()) {
    return std::nullopt;
  }
  const auto& abi = call.arg_abi[argument.arg_index];
  const auto& selection = *argument.source_selection;
  if (selection.kind ==
      PreparedCallArgumentSourceSelectionKind::LocalFrameAddressMaterialization) {
    const auto route = as_local_frame_address_materialization_route(selection);
    if (abi.type != bir::TypeKind::Ptr || !abi.byval_copy ||
        abi.primary_class != bir::AbiValueClass::Memory ||
        abi.size_bytes == 0 || abi.align_bytes == 0 ||
        !route.has_value() ||
        route->source_size_bytes < abi.size_bytes) {
      return std::nullopt;
    }

    PreparedAggregateTransportPlan plan{
        .kind = PreparedAggregateTransportKind::StackCopy,
        .payload_size_bytes = route->source_size_bytes,
        .payload_align_bytes = route->source_align_bytes,
        .copy_size_bytes = abi.size_bytes,
        .copy_align_bytes = abi.align_bytes,
        .source_slot_id = route->source_slot_id,
        .source_stack_offset_bytes = route->source_stack_offset_bytes,
        .destination_stack_offset_bytes = argument.destination_stack_offset_bytes,
        .destination_stack_size_bytes = argument.destination_stack_size_bytes,
        .chunks = {},
        .lanes = {},
        .scratch_requirements = {},
    };

    constexpr std::size_t max_chunk_size = 8;
    if (route->source_value_name != kInvalidValueName) {
      const auto aggregate_name = names.value_names.spelling(route->source_value_name);
      plan.chunks = collect_byval_stack_copy_store_chunks(stack_layout,
                                                          function,
                                                          addressing,
                                                          block_label,
                                                          call_plan.block_index,
                                                          call_plan.instruction_index,
                                                          aggregate_name,
                                                          abi.size_bytes);
    }
    if (plan.chunks.empty()) {
      for (std::size_t payload_offset = 0, chunk_index = 0;
           payload_offset < abi.size_bytes;
           ++chunk_index) {
        const std::size_t chunk_size =
            std::min(max_chunk_size, abi.size_bytes - payload_offset);
        plan.chunks.push_back(PreparedAggregateTransportChunk{
            .chunk_index = chunk_index,
            .kind = PreparedAggregateTransportChunkKind::RequiredPayload,
            .payload_offset_bytes = payload_offset,
            .source_offset_bytes = route->source_stack_offset_bytes + payload_offset,
            .destination_offset_bytes = payload_offset,
            .size_bytes = chunk_size,
            .align_bytes = std::min<std::size_t>(route->source_align_bytes,
                                                  chunk_size),
            .preferred_width_bytes = chunk_size,
            .fallback_width_bytes = {},
        });
        payload_offset += chunk_size;
      }
    }
    plan.scratch_requirements.push_back(
        PreparedAggregateTransportScratchRequirement{
            .kind = PreparedAggregateTransportScratchKind::GeneralPurpose,
            .width_bytes = max_chunk_size,
            .may_overlap_source = false,
            .may_overlap_destination = false,
        });
    return plan;
  }
  if (selection.kind != PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane) {
    return std::nullopt;
  }
  const bool has_register_destination =
      argument.destination_register_name.has_value() &&
      argument.destination_register_bank.has_value() &&
      !argument.destination_occupied_register_names.empty();
  const bool has_stack_destination =
      argument.destination_stack_offset_bytes.has_value() &&
      argument.destination_stack_size_bytes.has_value();
  if (abi.type != bir::TypeKind::Ptr || !abi.byval_copy ||
      !abi.passed_in_register ||
      abi.primary_class != bir::AbiValueClass::Integer ||
      abi.size_bytes == 0 ||
      !selection.byval_lane_extent_bytes.has_value() ||
      !selection.source_stack_offset_bytes.has_value() ||
      !selection.source_size_bytes.has_value() ||
      !selection.source_align_bytes.has_value() ||
      (!has_register_destination && !has_stack_destination)) {
    return std::nullopt;
  }

  const std::size_t payload_size = *selection.byval_lane_extent_bytes;
  if (payload_size == 0) {
    return std::nullopt;
  }
  const std::size_t register_lane_count =
      has_register_destination
          ? std::max<std::size_t>(
                std::size_t{1},
                std::min(argument.destination_contiguous_width,
                         argument.destination_occupied_register_names.size()))
          : std::size_t{0};
  constexpr std::size_t max_lane_size = 8;
  const std::size_t stack_lane_count =
      (payload_size + max_lane_size - 1) / max_lane_size;
  const std::size_t lane_count =
      has_register_destination ? register_lane_count : stack_lane_count;

  PreparedAggregateTransportPlan plan{
      .kind = PreparedAggregateTransportKind::ByvalRegisterLanes,
      .payload_size_bytes = payload_size,
      .payload_align_bytes = *selection.source_align_bytes,
      .copy_size_bytes = abi.size_bytes,
      .copy_align_bytes = abi.align_bytes == 0 ? *selection.source_align_bytes
                                               : abi.align_bytes,
      .source_slot_id = selection.source_slot_id,
      .source_stack_offset_bytes = selection.source_stack_offset_bytes,
      .destination_stack_offset_bytes = argument.destination_stack_offset_bytes,
      .destination_stack_size_bytes = argument.destination_stack_size_bytes,
      .chunks = {},
      .lanes = {},
      .scratch_requirements = {},
  };

  std::size_t payload_offset = 0;
  for (std::size_t lane_index = 0;
       lane_index < lane_count && payload_offset < payload_size;
       ++lane_index) {
    const std::size_t lane_size =
        std::min(max_lane_size, payload_size - payload_offset);
    plan.chunks.push_back(PreparedAggregateTransportChunk{
        .chunk_index = lane_index,
        .kind = PreparedAggregateTransportChunkKind::RequiredPayload,
        .payload_offset_bytes = payload_offset,
        .source_offset_bytes = *selection.source_stack_offset_bytes + payload_offset,
        .destination_offset_bytes = payload_offset,
        .size_bytes = lane_size,
        .align_bytes = std::min<std::size_t>(*selection.source_align_bytes, lane_size),
        .preferred_width_bytes = lane_size,
        .fallback_width_bytes = {},
    });
    plan.lanes.push_back(PreparedAggregateTransportLane{
        .lane_index = lane_index,
        .chunk_index = lane_index,
        .lane_payload_offset_bytes = payload_offset,
        .source_offset_bytes = *selection.source_stack_offset_bytes + payload_offset,
        .destination_offset_bytes = payload_offset,
        .lane_size_bytes = lane_size,
        .destination_register_name =
            has_register_destination
                ? (lane_index == 0
                       ? argument.destination_register_name
                       : std::optional<std::string>{
                             argument.destination_occupied_register_names[lane_index]})
                : std::nullopt,
        .destination_register_bank = argument.destination_register_bank,
        .destination_contiguous_width = argument.destination_contiguous_width,
        .destination_occupied_register_names =
            argument.destination_occupied_register_names,
        .destination_register_placement = argument.destination_register_placement,
        .whole_register = lane_size == max_lane_size,
    });
    payload_offset += lane_size;
  }

  if (payload_offset < payload_size) {
    return std::nullopt;
  }
  plan.scratch_requirements.push_back(
      PreparedAggregateTransportScratchRequirement{
          .kind = PreparedAggregateTransportScratchKind::GeneralPurpose,
          .width_bytes = max_lane_size,
          .may_overlap_source = false,
          .may_overlap_destination = false,
      });
  return plan;
}

}  // namespace

bool prepared_call_argument_binary_producer_opcode_is_materializable(
    bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::SRem:
      return true;
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      return false;
  }
  return false;
}

std::optional<PreparedCallArgumentBinaryProducerMaterializationFact>
find_prepared_call_argument_binary_producer_materialization_fact(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  const auto producer =
      find_prepared_same_block_scalar_producer(names,
                                              source_producers,
                                              block_label,
                                              block,
                                              value,
                                              before_instruction_index);
  if (!producer.has_value() ||
      producer->producer.kind != PreparedEdgePublicationSourceProducerKind::Binary ||
      producer->producer.binary == nullptr ||
      producer->instruction == nullptr ||
      !prepared_call_argument_binary_producer_opcode_is_materializable(
          producer->producer.binary->opcode)) {
    return std::nullopt;
  }

  return PreparedCallArgumentBinaryProducerMaterializationFact{
      .destination_value_name = producer->value_name,
      .destination_value_type = value.type,
      .producer_block_label = producer->producer.block_label,
      .producer_instruction_index = producer->instruction_index,
      .producer_kind = producer->producer.kind,
      .producer_instruction = producer->instruction,
      .binary = producer->producer.binary,
      .binary_opcode = producer->producer.binary->opcode,
      .lhs = producer->producer.binary->lhs,
      .rhs = producer->producer.binary->rhs,
      .same_block_before_call = producer->producer.block_label == block_label &&
                                producer->instruction_index <
                                    before_instruction_index,
      .materializable = true,
  };
}

std::optional<PreparedCallArgumentSourceProducerMaterialization>
find_prepared_call_argument_source_producer_materialization(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  const auto producer =
      find_prepared_same_block_scalar_producer(names,
                                              source_producers,
                                              block_label,
                                              block,
                                              value,
                                              before_instruction_index);
  if (!producer.has_value()) {
    return std::nullopt;
  }

  bool materializable = false;
  if (producer->producer.kind == PreparedEdgePublicationSourceProducerKind::LoadLocal) {
    materializable = true;
  } else if (
      find_prepared_call_argument_binary_producer_materialization_fact(
          names,
          source_producers,
          block_label,
          block,
          value,
          before_instruction_index)
          .has_value()) {
    materializable = true;
  }
  if (!materializable) {
    return std::nullopt;
  }
  return PreparedCallArgumentSourceProducerMaterialization{
      .producer = *producer,
      .materializable = true,
  };
}

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

[[nodiscard]] std::optional<bir::CallArgumentSourceEncodingKind>
route6_named_scalar_i32_source_encoding(PreparedStorageEncodingKind encoding) {
  switch (encoding) {
    case PreparedStorageEncodingKind::Register:
      return bir::CallArgumentSourceEncodingKind::Register;
    case PreparedStorageEncodingKind::FrameSlot:
      return bir::CallArgumentSourceEncodingKind::FrameSlot;
    case PreparedStorageEncodingKind::None:
    case PreparedStorageEncodingKind::Immediate:
    case PreparedStorageEncodingKind::ComputedAddress:
    case PreparedStorageEncodingKind::SymbolAddress:
      return std::nullopt;
  }
  return std::nullopt;
}

void publish_route6_named_scalar_i32_call_argument_source(
    const PreparedNameTables& names,
    bir::CallInst& call,
    const PreparedCallArgumentPlan& argument,
    std::optional<ValueNameId> source_value_name) {
  if (argument.arg_index >= call.args.size()) {
    return;
  }
  const auto& value = call.args[argument.arg_index];
  if (value.kind != bir::Value::Kind::Named ||
      value.type != bir::TypeKind::I32 ||
      value.name.empty() ||
      !argument.source_value_id.has_value() ||
      !source_value_name.has_value() ||
      *source_value_name == kInvalidValueName ||
      argument.source_selection.has_value() ||
      argument.direct_global_select_chain_dependency.available) {
    return;
  }

  const auto source_encoding =
      route6_named_scalar_i32_source_encoding(argument.source_encoding);
  if (!source_encoding.has_value()) {
    return;
  }

  const std::string_view prepared_source_name =
      names.value_names.spelling(*source_value_name);
  if (prepared_source_name.empty() || prepared_source_name != value.name) {
    return;
  }

  bir::CallArgumentSourceRelationship* existing = nullptr;
  for (auto& relationship : call.arg_sources) {
    if (relationship.arg_index != argument.arg_index) {
      continue;
    }
    if (existing != nullptr) {
      return;
    }
    existing = &relationship;
  }

  const std::size_t source_value_id =
      static_cast<std::size_t>(*argument.source_value_id);
  if (existing == nullptr) {
    call.arg_sources.push_back(bir::CallArgumentSourceRelationship{
        .arg_index = argument.arg_index,
        .source_encoding = *source_encoding,
        .source_value_id = source_value_id,
        .source_value_name = std::string{prepared_source_name},
    });
    return;
  }

  if (existing->source_selection.has_value() ||
      existing->direct_global_select_chain_dependency.has_value()) {
    return;
  }
  if (existing->source_encoding != bir::CallArgumentSourceEncodingKind::None &&
      existing->source_encoding != *source_encoding) {
    return;
  }
  if (existing->source_value_id.has_value() &&
      *existing->source_value_id != source_value_id) {
    return;
  }
  if (existing->source_value_name.has_value() &&
      *existing->source_value_name != prepared_source_name) {
    return;
  }

  existing->source_encoding = *source_encoding;
  existing->source_value_id = source_value_id;
  existing->source_value_name = std::string{prepared_source_name};
}

void seed_supported_prior_call_preservations_from_current_call(
    PreparedCallPlansFunction& function_plan,
    const PreparedCallPlan& current_call_plan) {
  for (const auto& preserved : current_call_plan.preserved_values) {
    seed_prior_call_preservation_from_later_dependency(function_plan, preserved);
  }
}

void populate_call_plans(PreparedBirModule& prepared) {
  prepared.call_plans.functions.clear();

  for (auto& function : prepared.module.functions) {
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
    function_plan.calls.reserve(count_call_instructions(function));
    const auto* frame_plan = find_prepared_frame_plan(prepared, function_name_id);
    const auto* control_flow_function =
        find_prepared_control_flow_function(prepared.control_flow, function_name_id);
    const auto* addressing_function = find_prepared_addressing(prepared, function_name_id);
    const auto* regalloc_function = find_regalloc_function(prepared.regalloc, function_name_id);
    const auto* liveness_function = find_liveness_function(prepared.liveness, function_name_id);
    const auto* value_locations = find_prepared_value_location_function(prepared, function_name_id);
    const PreparedValueHomeLookup value_home_lookup =
        make_prepared_value_home_lookup(value_locations);
    const CallPreservationCandidates call_preservation_candidates =
        make_call_preservation_candidates(regalloc_function);
    const std::vector<PreparedClobberedRegister> call_clobbers =
        build_call_clobber_set(prepared.target_profile, regalloc_function);
    std::optional<PreparedEdgePublicationSourceProducerLookups> source_producers;
    if (control_flow_function != nullptr) {
      source_producers =
          make_prepared_edge_publication_source_producer_lookups(
              prepared, *control_flow_function);
    }
    PreparedCallPlanLookups prior_preserved_lookups;

    for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
      auto& block = function.blocks[block_index];
      const BlockLabelId block_label =
          prepared_block_label_for_index(control_flow_function, function, block_index);
      for (std::size_t instruction_index = 0; instruction_index < block.insts.size(); ++instruction_index) {
        auto* call = std::get_if<bir::CallInst>(&block.insts[instruction_index]);
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
            .outgoing_stack_argument_area = std::nullopt,
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
              .allows_local_aggregate_address_publication =
                  call_argument_allows_local_aggregate_address_publication(*call, arg_index),
              .value_bank = call_argument_value_bank(*call, arg_index),
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
              .destination_stack_size_bytes = std::nullopt,
              .source_register_placement = std::nullopt,
              .destination_register_placement = std::nullopt,
              .source_selection = std::nullopt,
              .aggregate_transport = std::nullopt,
              .direct_global_select_chain_dependency = {},
          };
          const CallArgumentDestinationPlan destination =
              plan_call_argument_destination(prepared.target_profile,
                                             *call,
                                             call_plan.wrapper_kind,
                                             before_call_bundle,
                                             arg_index,
                                             arg_plan.value_bank);
          arg_plan.destination_register_name = destination.register_name;
          arg_plan.destination_contiguous_width = destination.contiguous_width;
          arg_plan.destination_occupied_register_names =
              destination.occupied_register_names;
          arg_plan.destination_register_bank = destination.register_bank;
          arg_plan.destination_stack_offset_bytes = destination.stack_offset_bytes;
          arg_plan.destination_stack_size_bytes = destination.stack_size_bytes;
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
          arg_plan.direct_global_select_chain_dependency =
              plan_call_argument_direct_global_select_chain_dependency(
                  prepared.names,
                  source_producers.has_value() ? &*source_producers : nullptr,
                  block_label,
                  block,
                  call->args[arg_index],
                  instruction_index,
                  source.value_name);
          const auto* source_home =
              source.value_id.has_value()
                  ? find_prepared_value_home(value_home_lookup, *source.value_id)
                  : (source.value_name.has_value() && value_locations != nullptr
                         ? find_prepared_value_home(*value_locations, *source.value_name)
                         : nullptr);
          arg_plan.source_selection =
              select_prepared_call_argument_source(prepared,
                                                   prepared.names,
                                                   call_plan,
                                                   prior_preserved_lookups,
                                                   function,
                                                   *call,
                                                   control_flow_function,
                                                   addressing_function,
                                                   before_call_bundle,
                                                   arg_plan,
                                                   source_home);
          arg_plan.aggregate_transport =
              plan_prepared_aggregate_transport(prepared.names,
                                                prepared.stack_layout,
                                                function,
                                                call_plan,
                                                addressing_function,
                                                block_label,
                                                *call,
                                                arg_plan);
          arg_plan.allows_local_aggregate_address_publication =
              arg_plan.source_selection.has_value() &&
              arg_plan.source_selection->kind ==
                  PreparedCallArgumentSourceSelectionKind::
                      LocalFrameAddressMaterialization;
          publish_route6_named_scalar_i32_call_argument_source(
              prepared.names, *call, arg_plan, source.value_name);

          call_plan.arguments.push_back(std::move(arg_plan));
        }

        call_plan.outgoing_stack_argument_area =
            compute_outgoing_stack_argument_area(call_plan.arguments);
        call_plan.result = build_call_result_plan(prepared.names,
                                                  prepared.target_profile,
                                                  regalloc_function,
                                                  value_locations,
                                                  after_call_bundle,
                                                  instruction_index,
                                                  *call);

        function_plan.calls.push_back(std::move(call_plan));
        seed_supported_prior_call_preservations_from_current_call(
            function_plan, function_plan.calls.back());
        append_incremental_prior_preserved_values(prior_preserved_lookups,
                                                  function_plan.calls.back());
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
  if (argument.source_selection.has_value() &&
      (argument.source_selection->kind ==
           PreparedCallArgumentSourceSelectionKind::FrameSlotValue ||
       argument.source_selection->kind ==
           PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane)) {
    const auto& selection = *argument.source_selection;
    const auto* transport = argument.aggregate_transport.has_value()
                                ? &*argument.aggregate_transport
                                : nullptr;
    const auto source_slot_id =
        selection.kind == PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane &&
                transport != nullptr && transport->source_slot_id.has_value()
            ? transport->source_slot_id
            : selection.source_slot_id;
    const auto source_stack_offset =
        selection.kind == PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane &&
                transport != nullptr && transport->source_stack_offset_bytes.has_value()
            ? transport->source_stack_offset_bytes
            : selection.source_stack_offset_bytes;
    const auto source_size =
        selection.kind == PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane &&
                transport != nullptr && transport->payload_size_bytes != 0
            ? std::optional<std::size_t>{transport->payload_size_bytes}
            : selection.source_size_bytes;
    const auto source_align =
        selection.kind == PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane &&
                transport != nullptr && transport->payload_align_bytes != 0
            ? std::optional<std::size_t>{transport->payload_align_bytes}
            : selection.source_align_bytes;
    // Outgoing stack arguments are lowered from selected prepared frame bytes,
    // not necessarily the regalloc spill slot originally assigned to the value.
    // Keep call-boundary lifetime sources aligned with lowering so the outgoing
    // stack store preserves the actual prepared source slot it reads.
    return PreparedCallBoundaryEffectEndpoint{
        .encoding = PreparedStorageEncodingKind::FrameSlot,
        .storage_kind = PreparedMoveStorageKind::StackSlot,
        .value_id = selection.source_value_id.has_value()
                        ? selection.source_value_id
                        : argument.source_value_id,
        .value_name = selection.source_value_name.value_or(kInvalidValueName),
        .register_bank = argument.source_register_bank,
        .contiguous_width = 1,
        .slot_id = source_slot_id,
        .stack_offset_bytes = source_stack_offset,
        .stack_size_bytes = source_size,
        .stack_align_bytes = source_align,
    };
  }
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
      .stack_size_bytes = argument.destination_stack_size_bytes,
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
  if (preserved.preservation_destination.storage_kind != PreparedMoveStorageKind::None) {
    return preserved.preservation_destination;
  }
  return make_preservation_destination_endpoint(preserved);
}

[[nodiscard]] PreparedCallBoundaryEffectEndpoint make_preserved_republication_endpoint(
    const PreparedCallPreservedValue& preserved) {
  if (preserved.preservation_source.storage_kind != PreparedMoveStorageKind::None) {
    return preserved.preservation_source;
  }
  return make_value_effect_endpoint(preserved.value_id, preserved.value_name);
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
    const auto preservation_source =
        preserved.preservation_source.storage_kind == PreparedMoveStorageKind::None
            ? make_value_effect_endpoint(preserved.value_id, preserved.value_name)
            : preserved.preservation_source;
    const auto storage_endpoint = make_preserved_storage_endpoint(preserved);
    const auto republication_endpoint =
        make_preserved_republication_endpoint(preserved);
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
                      ? preservation_source
                      : storage_endpoint,
        .destination = effect_kind ==
                           PreparedCallBoundaryEffectKind::PreservationHomePopulation
                           ? storage_endpoint
                           : republication_endpoint,
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
