#include "prealloc.hpp"
#include "stack_layout/support.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::prepare {

namespace {

struct ActiveRegisterAssignment {
  std::size_t value_index = 0;
  std::size_t end_point = 0;
  std::string register_name;
};

struct ProgramPointLocation {
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
};

constexpr std::array<const char*, 6> kX86GeneralAbiRegisters = {
    "rdi", "rsi", "rdx", "rcx", "r8", "r9",
};
constexpr std::array<const char*, 8> kX86FloatAbiRegisters = {
    "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
};
constexpr std::array<const char*, 8> kAarch64GeneralAbiRegisters = {
    "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7",
};
constexpr std::array<const char*, 8> kRiscvGeneralAbiRegisters = {
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
};
constexpr std::array<const char*, 8> kRiscvFloatAbiRegisters = {
    "fa0", "fa1", "fa2", "fa3", "fa4", "fa5", "fa6", "fa7",
};

template <std::size_t N>
[[nodiscard]] std::optional<std::string> indexed_register_name(
    const std::array<const char*, N>& register_names,
    std::size_t index) {
  if (index >= register_names.size() || register_names[index] == nullptr ||
      register_names[index][0] == '\0') {
    return std::nullopt;
  }
  return std::string(register_names[index]);
}

[[nodiscard]] bool is_float_type(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return true;
    default:
      return false;
  }
}

[[nodiscard]] bool is_float_abi_class(bir::AbiValueClass abi_class) {
  switch (abi_class) {
    case bir::AbiValueClass::Sse:
    case bir::AbiValueClass::X87:
      return true;
    default:
      return false;
  }
}

[[nodiscard]] std::optional<std::string> aarch64_float_register_name(bir::TypeKind type,
                                                                     std::size_t index) {
  if (index >= 8) {
    return std::nullopt;
  }
  if (type == bir::TypeKind::F32) {
    return "s" + std::to_string(index);
  }
  if (type == bir::TypeKind::F64) {
    return "d" + std::to_string(index);
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string> call_arg_destination_register_name(
    Target target,
    const bir::CallArgAbiInfo& abi,
    std::size_t arg_index) {
  if (!abi.passed_in_register) {
    return std::nullopt;
  }

  switch (target) {
    case Target::X86_64:
      if (is_float_abi_class(abi.primary_class)) {
        return indexed_register_name(kX86FloatAbiRegisters, arg_index);
      }
      if (abi.primary_class == bir::AbiValueClass::Integer) {
        return indexed_register_name(kX86GeneralAbiRegisters, arg_index);
      }
      return std::nullopt;
    case Target::I686:
      return std::nullopt;
    case Target::Aarch64:
      if (is_float_abi_class(abi.primary_class)) {
        return aarch64_float_register_name(abi.type, arg_index);
      }
      if (abi.primary_class == bir::AbiValueClass::Integer) {
        return indexed_register_name(kAarch64GeneralAbiRegisters, arg_index);
      }
      return std::nullopt;
    case Target::Riscv64:
      if (is_float_abi_class(abi.primary_class) || is_float_type(abi.type)) {
        return indexed_register_name(kRiscvFloatAbiRegisters, arg_index);
      }
      if (abi.primary_class == bir::AbiValueClass::Integer) {
        return indexed_register_name(kRiscvGeneralAbiRegisters, arg_index);
      }
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string> call_result_destination_register_name(
    Target target,
    const bir::CallResultAbiInfo& abi) {
  if (abi.returned_in_memory || abi.primary_class == bir::AbiValueClass::Memory ||
      abi.type == bir::TypeKind::Void) {
    return std::nullopt;
  }

  switch (target) {
    case Target::X86_64:
      if (is_float_abi_class(abi.primary_class) || is_float_type(abi.type)) {
        return std::string("xmm0");
      }
      return std::string("rax");
    case Target::I686:
      return is_float_type(abi.type) ? std::nullopt : std::optional<std::string>{"eax"};
    case Target::Aarch64:
      if (is_float_abi_class(abi.primary_class) || is_float_type(abi.type)) {
        return aarch64_float_register_name(abi.type, 0);
      }
      return std::string("x0");
    case Target::Riscv64:
      if (is_float_abi_class(abi.primary_class) || is_float_type(abi.type)) {
        return std::string("fa0");
      }
      return std::string("a0");
  }
  return std::nullopt;
}

[[nodiscard]] PreparedRegisterClass classify_register_class(const PreparedLivenessValue& value) {
  switch (value.type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return PreparedRegisterClass::General;
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return PreparedRegisterClass::Float;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
      return PreparedRegisterClass::None;
  }
  return PreparedRegisterClass::None;
}

[[nodiscard]] bool intervals_overlap(const PreparedLiveInterval& lhs,
                                     const PreparedLiveInterval& rhs) {
  return std::max(lhs.start_point, rhs.start_point) <= std::min(lhs.end_point, rhs.end_point);
}

[[nodiscard]] std::size_t value_priority(const PreparedLivenessValue& value) {
  std::size_t priority = value.use_points.size();
  if (value.live_interval.has_value() && value.live_interval->end_point >= value.live_interval->start_point) {
    priority += (value.live_interval->end_point - value.live_interval->start_point) + 1U;
  }
  if (value.crosses_call) {
    priority += 2U;
  }
  if (value.requires_home_slot) {
    priority += 1U;
  }
  return priority;
}

[[nodiscard]] std::size_t loop_depth_weight(std::size_t loop_depth) {
  switch (loop_depth) {
    case 0:
      return 1U;
    case 1:
      return 10U;
    case 2:
      return 100U;
    case 3:
      return 1000U;
    default:
      return 10000U;
  }
}

[[nodiscard]] std::optional<ProgramPointLocation> locate_program_point(
    const PreparedLivenessFunction& function,
    std::size_t point) {
  for (const auto& block : function.blocks) {
    if (point < block.start_point || point > block.end_point) {
      continue;
    }
    return ProgramPointLocation{
        .block_index = block.block_index,
        .instruction_index = point - block.start_point,
    };
  }
  return std::nullopt;
}

[[nodiscard]] std::size_t weighted_use_score(const PreparedLivenessFunction& function,
                                             const PreparedLivenessValue& value) {
  std::size_t weighted_uses = 0;
  for (const std::size_t use_point : value.use_points) {
    std::size_t weight = 1U;
    if (const auto location = locate_program_point(function, use_point); location.has_value() &&
        location->block_index < function.block_loop_depth.size()) {
      weight = loop_depth_weight(function.block_loop_depth[location->block_index]);
    }
    weighted_uses += weight;
  }
  return weighted_uses;
}

[[nodiscard]] std::vector<std::string_view> caller_saved_registers(Target target,
                                                                   PreparedRegisterClass reg_class) {
  if (reg_class != PreparedRegisterClass::General) {
    return {};
  }
  switch (target) {
    case Target::X86_64:
      return {"r11"};
    case Target::I686:
      return {"ecx"};
    case Target::Aarch64:
      return {"x13"};
    case Target::Riscv64:
      return {"t0"};
  }
  return {};
}

[[nodiscard]] std::vector<std::string_view> callee_saved_registers(Target target,
                                                                   PreparedRegisterClass reg_class) {
  if (reg_class != PreparedRegisterClass::General) {
    return {};
  }
  switch (target) {
    case Target::X86_64:
      return {"rbx", "r12"};
    case Target::I686:
      return {"ebx", "esi"};
    case Target::Aarch64:
      return {"x20", "x21"};
    case Target::Riscv64:
      return {"s1", "s2"};
  }
  return {};
}

[[nodiscard]] std::vector<std::string> materialize_register_names(
    const std::vector<std::string_view>& register_names) {
  std::vector<std::string> materialized;
  materialized.reserve(register_names.size());
  for (const std::string_view register_name : register_names) {
    materialized.emplace_back(register_name);
  }
  return materialized;
}

[[nodiscard]] std::size_t normalized_value_size(const PreparedRegallocValue& value) {
  return stack_layout::normalize_size(value.type, 0);
}

[[nodiscard]] std::size_t normalized_value_alignment(const PreparedRegallocValue& value) {
  return stack_layout::normalize_alignment(value.type, 0, normalized_value_size(value));
}

[[nodiscard]] PreparedMoveStorageKind assigned_storage_kind(const PreparedRegallocValue& value) {
  if (value.assigned_register.has_value()) {
    return PreparedMoveStorageKind::Register;
  }
  if (value.assigned_stack_slot.has_value()) {
    return PreparedMoveStorageKind::StackSlot;
  }
  return PreparedMoveStorageKind::None;
}

[[nodiscard]] bool assigned_storage_matches(const PreparedRegallocValue& lhs,
                                            const PreparedRegallocValue& rhs) {
  const PreparedMoveStorageKind lhs_kind = assigned_storage_kind(lhs);
  const PreparedMoveStorageKind rhs_kind = assigned_storage_kind(rhs);
  if (lhs_kind != rhs_kind) {
    return false;
  }
  switch (lhs_kind) {
    case PreparedMoveStorageKind::None:
      return true;
    case PreparedMoveStorageKind::Register:
      return lhs.assigned_register->register_name == rhs.assigned_register->register_name;
    case PreparedMoveStorageKind::StackSlot:
      return lhs.assigned_stack_slot->slot_id == rhs.assigned_stack_slot->slot_id;
  }
  return false;
}

[[nodiscard]] std::string storage_transfer_reason(std::string_view prefix,
                                                  PreparedMoveStorageKind from_kind,
                                                  PreparedMoveStorageKind to_kind) {
  if (from_kind == PreparedMoveStorageKind::StackSlot &&
      to_kind == PreparedMoveStorageKind::Register) {
    return std::string(prefix) + "_stack_to_register";
  }
  if (from_kind == PreparedMoveStorageKind::Register &&
      to_kind == PreparedMoveStorageKind::StackSlot) {
    return std::string(prefix) + "_register_to_stack";
  }
  if (from_kind == PreparedMoveStorageKind::Register &&
      to_kind == PreparedMoveStorageKind::Register) {
    return std::string(prefix) + "_register_to_register";
  }
  if (from_kind == PreparedMoveStorageKind::StackSlot &&
      to_kind == PreparedMoveStorageKind::StackSlot) {
    return std::string(prefix) + "_stack_to_stack";
  }
  return std::string(prefix) + "_storage_resolution";
}

[[nodiscard]] std::string storage_transfer_reason(std::string_view prefix,
                                                  const PreparedRegallocValue& from,
                                                  const PreparedRegallocValue& to) {
  return storage_transfer_reason(prefix, assigned_storage_kind(from), assigned_storage_kind(to));
}

void append_move_resolution_record(PreparedRegallocFunction& regalloc_function,
                                   const PreparedRegallocValue& source,
                                   const PreparedRegallocValue& destination,
                                   std::size_t block_index,
                                   std::size_t instruction_index,
                                   std::string reason) {
  if (assigned_storage_kind(source) == PreparedMoveStorageKind::None ||
      assigned_storage_kind(destination) == PreparedMoveStorageKind::None ||
      assigned_storage_matches(source, destination)) {
    return;
  }

  const auto duplicate = std::find_if(
      regalloc_function.move_resolution.begin(),
      regalloc_function.move_resolution.end(),
      [&](const PreparedMoveResolution& move) {
        return move.from_value_id == source.value_id && move.to_value_id == destination.value_id &&
               move.destination_kind == PreparedMoveDestinationKind::Value &&
               move.destination_storage_kind == assigned_storage_kind(destination) &&
               !move.destination_abi_index.has_value() &&
               !move.destination_register_name.has_value() && move.block_index == block_index &&
               move.instruction_index == instruction_index;
      });
  if (duplicate != regalloc_function.move_resolution.end()) {
    return;
  }

  regalloc_function.move_resolution.push_back(PreparedMoveResolution{
      .from_value_id = source.value_id,
      .to_value_id = destination.value_id,
      .destination_kind = PreparedMoveDestinationKind::Value,
      .destination_storage_kind = assigned_storage_kind(destination),
      .destination_abi_index = std::nullopt,
      .destination_register_name = std::nullopt,
      .block_index = block_index,
      .instruction_index = instruction_index,
      .reason = std::move(reason),
  });
}

void append_move_resolution_record(PreparedRegallocFunction& regalloc_function,
                                   PreparedValueId from_value_id,
                                   PreparedValueId to_value_id,
                                   PreparedMoveDestinationKind destination_kind,
                                   PreparedMoveStorageKind from_kind,
                                   PreparedMoveStorageKind to_kind,
                                   std::optional<std::size_t> destination_abi_index,
                                   std::optional<std::string> destination_register_name,
                                   std::size_t block_index,
                                   std::size_t instruction_index,
                                   std::string reason) {
  if (from_kind == PreparedMoveStorageKind::None || to_kind == PreparedMoveStorageKind::None) {
    return;
  }

  const auto duplicate = std::find_if(
      regalloc_function.move_resolution.begin(),
      regalloc_function.move_resolution.end(),
      [&](const PreparedMoveResolution& move) {
        return move.from_value_id == from_value_id && move.to_value_id == to_value_id &&
               move.destination_kind == destination_kind &&
               move.destination_storage_kind == to_kind &&
               move.destination_abi_index == destination_abi_index &&
               move.destination_register_name == destination_register_name &&
               move.block_index == block_index && move.instruction_index == instruction_index;
      });
  if (duplicate != regalloc_function.move_resolution.end()) {
    return;
  }

  regalloc_function.move_resolution.push_back(PreparedMoveResolution{
      .from_value_id = from_value_id,
      .to_value_id = to_value_id,
      .destination_kind = destination_kind,
      .destination_storage_kind = to_kind,
      .destination_abi_index = destination_abi_index,
      .destination_register_name = std::move(destination_register_name),
      .block_index = block_index,
      .instruction_index = instruction_index,
      .reason = std::move(reason),
  });
}

void expire_completed_assignments(std::vector<ActiveRegisterAssignment>& active,
                                  std::size_t start_point) {
  active.erase(std::remove_if(active.begin(),
                              active.end(),
                              [start_point](const ActiveRegisterAssignment& assignment) {
                                return assignment.end_point < start_point;
                              }),
               active.end());
}

[[nodiscard]] bool register_is_active(const std::vector<ActiveRegisterAssignment>& active,
                                      std::string_view register_name) {
  return std::any_of(active.begin(),
                     active.end(),
                     [register_name](const ActiveRegisterAssignment& assignment) {
                       return assignment.register_name == register_name;
                     });
}

[[nodiscard]] std::optional<std::string> choose_register(
    const std::vector<ActiveRegisterAssignment>& active,
    const std::vector<std::string_view>& register_names) {
  for (const std::string_view register_name : register_names) {
    if (!register_is_active(active, register_name)) {
      return std::string(register_name);
    }
  }
  return std::nullopt;
}

[[nodiscard]] bool has_lower_allocation_rank(const PreparedRegallocValue& lhs,
                                             const PreparedRegallocValue& rhs) {
  if (lhs.spill_weight != rhs.spill_weight) {
    return lhs.spill_weight < rhs.spill_weight;
  }
  if (lhs.priority != rhs.priority) {
    return lhs.priority < rhs.priority;
  }
  if (lhs.live_interval.has_value() != rhs.live_interval.has_value()) {
    return !lhs.live_interval.has_value();
  }
  if (lhs.live_interval.has_value() && rhs.live_interval.has_value() &&
      lhs.live_interval->start_point != rhs.live_interval->start_point) {
    return lhs.live_interval->start_point > rhs.live_interval->start_point;
  }
  return lhs.value_id > rhs.value_id;
}

template <typename CanEvict>
[[nodiscard]] std::optional<std::size_t> choose_eviction_candidate(
    const PreparedRegallocFunction& function,
    const std::vector<ActiveRegisterAssignment>& active,
    const std::vector<std::string_view>& register_names,
    const PreparedRegallocValue& value,
    CanEvict can_evict) {
  std::optional<std::size_t> weakest_active_index;
  for (std::size_t active_index = 0; active_index < active.size(); ++active_index) {
    const auto& assignment = active[active_index];
    if (std::find(register_names.begin(), register_names.end(), assignment.register_name) ==
            register_names.end() ||
        !can_evict(assignment)) {
      continue;
    }

    const auto& active_value = function.values[assignment.value_index];
    if (!has_lower_allocation_rank(active_value, value)) {
      continue;
    }
    if (!weakest_active_index.has_value() ||
        has_lower_allocation_rank(active_value,
                                  function.values[active[*weakest_active_index].value_index])) {
      weakest_active_index = active_index;
    }
  }
  return weakest_active_index;
}

[[nodiscard]] std::optional<PreparedStackSlotAssignment> existing_stack_slot_assignment(
    const PreparedStackLayout& stack_layout,
    std::string_view function_name,
    const PreparedRegallocValue& value) {
  if (!value.stack_object_id.has_value()) {
    return std::nullopt;
  }
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.function_name != function_name || slot.object_id != *value.stack_object_id) {
      continue;
    }
    return PreparedStackSlotAssignment{
        .slot_id = slot.slot_id,
        .offset_bytes = slot.offset_bytes,
    };
  }
  return std::nullopt;
}

[[nodiscard]] PreparedStackSlotAssignment allocate_stack_slot(std::string_view function_name,
                                                              const PreparedRegallocValue& value,
                                                              const PreparedStackLayout& stack_layout,
                                                              PreparedFrameSlotId& next_slot_id,
                                                              std::size_t& next_offset_bytes,
                                                              std::size_t& frame_alignment_bytes) {
  if (const auto existing = existing_stack_slot_assignment(stack_layout, function_name, value);
      existing.has_value()) {
    return *existing;
  }

  const std::size_t size_bytes = normalized_value_size(value);
  const std::size_t align_bytes = normalized_value_alignment(value);
  next_offset_bytes = stack_layout::align_to(next_offset_bytes, align_bytes);
  PreparedStackSlotAssignment slot{
      .slot_id = next_slot_id++,
      .offset_bytes = next_offset_bytes,
  };
  next_offset_bytes += size_bytes;
  frame_alignment_bytes = std::max(frame_alignment_bytes, align_bytes);
  return slot;
}

[[nodiscard]] std::size_t interval_start_sort_key(const PreparedRegallocValue& value) {
  if (!value.live_interval.has_value()) {
    return std::numeric_limits<std::size_t>::max();
  }
  return value.live_interval->start_point;
}

[[nodiscard]] const bir::Function* find_bir_function(const bir::Module& module,
                                                     std::string_view function_name) {
  const auto it = std::find_if(module.functions.begin(),
                               module.functions.end(),
                               [function_name](const bir::Function& function) {
                                 return function.name == function_name;
                               });
  if (it == module.functions.end()) {
    return nullptr;
  }
  return &*it;
}

[[nodiscard]] const PreparedRegallocValue* find_regalloc_value(
    const PreparedRegallocFunction& function,
    std::string_view value_name) {
  const auto it = std::find_if(function.values.begin(),
                               function.values.end(),
                               [value_name](const PreparedRegallocValue& value) {
                                 return value.value_name == value_name;
                               });
  if (it == function.values.end()) {
    return nullptr;
  }
  return &*it;
}

void append_phi_move_resolution(const bir::Function& function,
                                PreparedRegallocFunction& regalloc_function) {
  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (std::size_t instruction_index = 0; instruction_index < block.insts.size(); ++instruction_index) {
      const auto* phi = std::get_if<bir::PhiInst>(&block.insts[instruction_index]);
      if (phi == nullptr || phi->result.kind != bir::Value::Kind::Named) {
        continue;
      }

      const auto* destination = find_regalloc_value(regalloc_function, phi->result.name);
      if (destination == nullptr || assigned_storage_kind(*destination) == PreparedMoveStorageKind::None) {
        continue;
      }

      for (const auto& incoming : phi->incomings) {
        if (incoming.value.kind != bir::Value::Kind::Named) {
          continue;
        }

        const auto* source = find_regalloc_value(regalloc_function, incoming.value.name);
        if (source == nullptr || assigned_storage_kind(*source) == PreparedMoveStorageKind::None ||
            assigned_storage_matches(*source, *destination)) {
          continue;
        }

        append_move_resolution_record(regalloc_function,
                                      *source,
                                      *destination,
                                      block_index,
                                      instruction_index,
                                      storage_transfer_reason("phi_join", *source, *destination));
      }
    }
  }
}

void append_consumer_move_resolution(const bir::Function& function,
                                     PreparedRegallocFunction& regalloc_function) {
  auto append_for_instruction = [&](const bir::Value& result,
                                    std::initializer_list<const bir::Value*> operands,
                                    std::size_t block_index,
                                    std::size_t instruction_index) {
    if (result.kind != bir::Value::Kind::Named) {
      return;
    }

    const auto* destination = find_regalloc_value(regalloc_function, result.name);
    if (destination == nullptr) {
      return;
    }

    for (const bir::Value* operand : operands) {
      if (operand == nullptr || operand->kind != bir::Value::Kind::Named) {
        continue;
      }
      const auto* source = find_regalloc_value(regalloc_function, operand->name);
      if (source == nullptr) {
        continue;
      }

      append_move_resolution_record(regalloc_function,
                                    *source,
                                    *destination,
                                    block_index,
                                    instruction_index,
                                    storage_transfer_reason("consumer", *source, *destination));
    }
  };

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (std::size_t instruction_index = 0; instruction_index < block.insts.size(); ++instruction_index) {
      std::visit(
          [&](const auto& inst) {
            using Inst = std::decay_t<decltype(inst)>;
            if constexpr (std::is_same_v<Inst, bir::BinaryInst>) {
              append_for_instruction(inst.result,
                                     {&inst.lhs, &inst.rhs},
                                     block_index,
                                     instruction_index);
            } else if constexpr (std::is_same_v<Inst, bir::SelectInst>) {
              append_for_instruction(inst.result,
                                     {&inst.lhs, &inst.rhs, &inst.true_value, &inst.false_value},
                                     block_index,
                                     instruction_index);
            } else if constexpr (std::is_same_v<Inst, bir::CastInst>) {
              append_for_instruction(inst.result, {&inst.operand}, block_index, instruction_index);
            }
          },
          block.insts[instruction_index]);
    }
  }
}

[[nodiscard]] PreparedMoveStorageKind call_arg_storage_kind(const bir::CallInst& call,
                                                            std::size_t arg_index) {
  if (arg_index >= call.arg_abi.size()) {
    return PreparedMoveStorageKind::None;
  }

  const auto& abi = call.arg_abi[arg_index];
  if (abi.passed_in_register) {
    return PreparedMoveStorageKind::Register;
  }
  if (abi.passed_on_stack || abi.byval_copy || abi.sret_pointer ||
      abi.primary_class == bir::AbiValueClass::Memory) {
    return PreparedMoveStorageKind::StackSlot;
  }
  return PreparedMoveStorageKind::None;
}

[[nodiscard]] PreparedMoveStorageKind call_result_storage_kind(const bir::CallInst& call) {
  if (!call.result.has_value() || !call.result_abi.has_value() || call.result->kind != bir::Value::Kind::Named) {
    return PreparedMoveStorageKind::None;
  }

  const auto& abi = *call.result_abi;
  if (abi.returned_in_memory || abi.primary_class == bir::AbiValueClass::Memory) {
    return PreparedMoveStorageKind::StackSlot;
  }
  if (abi.type != bir::TypeKind::Void) {
    return PreparedMoveStorageKind::Register;
  }
  return PreparedMoveStorageKind::None;
}

[[nodiscard]] PreparedMoveStorageKind function_return_storage_kind(const bir::Function& function) {
  if (!function.return_abi.has_value()) {
    return PreparedMoveStorageKind::None;
  }
  const auto& abi = *function.return_abi;
  if (abi.returned_in_memory || abi.primary_class == bir::AbiValueClass::Memory) {
    return PreparedMoveStorageKind::StackSlot;
  }
  if (abi.type != bir::TypeKind::Void) {
    return PreparedMoveStorageKind::Register;
  }
  return PreparedMoveStorageKind::None;
}

void append_call_arg_move_resolution(Target target,
                                     const bir::Function& function,
                                     PreparedRegallocFunction& regalloc_function) {
  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (std::size_t instruction_index = 0; instruction_index < block.insts.size(); ++instruction_index) {
      const auto* call = std::get_if<bir::CallInst>(&block.insts[instruction_index]);
      if (call == nullptr) {
        continue;
      }

      for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
        const auto& arg = call->args[arg_index];
        if (arg.kind != bir::Value::Kind::Named) {
          continue;
        }

        const auto* source = find_regalloc_value(regalloc_function, arg.name);
        if (source == nullptr) {
          continue;
        }

        const PreparedMoveStorageKind consumed_kind = call_arg_storage_kind(*call, arg_index);
        const PreparedMoveStorageKind source_kind = assigned_storage_kind(*source);
        const auto destination_register_name = arg_index < call->arg_abi.size()
                                                   ? call_arg_destination_register_name(target,
                                                                                       call->arg_abi[arg_index],
                                                                                       arg_index)
                                                   : std::nullopt;
        if (source_kind == PreparedMoveStorageKind::Register &&
            consumed_kind == PreparedMoveStorageKind::Register && source->assigned_register.has_value() &&
            destination_register_name == std::optional<std::string>{source->assigned_register->register_name}) {
          continue;
        }
        append_move_resolution_record(regalloc_function,
                                      source->value_id,
                                      source->value_id,
                                      PreparedMoveDestinationKind::CallArgumentAbi,
                                      source_kind,
                                      consumed_kind,
                                      arg_index,
                                      destination_register_name,
                                      block_index,
                                      instruction_index,
                                      storage_transfer_reason("call_arg",
                                                              source_kind,
                                                              consumed_kind));
      }
    }
  }
}

void append_call_result_move_resolution(Target target,
                                        const bir::Function& function,
                                        PreparedRegallocFunction& regalloc_function) {
  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (std::size_t instruction_index = 0; instruction_index < block.insts.size(); ++instruction_index) {
      const auto* call = std::get_if<bir::CallInst>(&block.insts[instruction_index]);
      if (call == nullptr || !call->result.has_value() || call->result->kind != bir::Value::Kind::Named) {
        continue;
      }

      const auto* result = find_regalloc_value(regalloc_function, call->result->name);
      if (result == nullptr) {
        continue;
      }

      const PreparedMoveStorageKind consumed_kind = call_result_storage_kind(*call);
      const PreparedMoveStorageKind source_kind = assigned_storage_kind(*result);
      const auto destination_register_name = call->result_abi.has_value()
                                                 ? call_result_destination_register_name(
                                                       target, *call->result_abi)
                                                 : std::nullopt;
      if (source_kind == PreparedMoveStorageKind::Register &&
          consumed_kind == PreparedMoveStorageKind::Register && result->assigned_register.has_value() &&
          destination_register_name == std::optional<std::string>{result->assigned_register->register_name}) {
        continue;
      }
      append_move_resolution_record(regalloc_function,
                                    result->value_id,
                                    result->value_id,
                                    PreparedMoveDestinationKind::CallResultAbi,
                                    source_kind,
                                    consumed_kind,
                                    std::nullopt,
                                    destination_register_name,
                                    block_index,
                                    instruction_index,
                                    storage_transfer_reason("call_result",
                                                            source_kind,
                                                            consumed_kind));
    }
  }
}

void append_return_move_resolution(Target target,
                                   const bir::Function& function,
                                   PreparedRegallocFunction& regalloc_function) {
  const PreparedMoveStorageKind consumed_kind = function_return_storage_kind(function);
  if (consumed_kind == PreparedMoveStorageKind::None) {
    return;
  }

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    if (block.terminator.kind != bir::TerminatorKind::Return || !block.terminator.value.has_value() ||
        block.terminator.value->kind != bir::Value::Kind::Named) {
      continue;
    }

    const auto* source = find_regalloc_value(regalloc_function, block.terminator.value->name);
    if (source == nullptr) {
      continue;
    }

    const PreparedMoveStorageKind source_kind = assigned_storage_kind(*source);
    const auto destination_register_name = function.return_abi.has_value()
                                               ? call_result_destination_register_name(
                                                     target, *function.return_abi)
                                               : std::nullopt;
    if (source_kind == PreparedMoveStorageKind::Register &&
        consumed_kind == PreparedMoveStorageKind::Register && source->assigned_register.has_value() &&
        destination_register_name == std::optional<std::string>{source->assigned_register->register_name}) {
      continue;
    }
    append_move_resolution_record(regalloc_function,
                                  source->value_id,
                                  source->value_id,
                                  PreparedMoveDestinationKind::FunctionReturnAbi,
                                  source_kind,
                                  consumed_kind,
                                  std::nullopt,
                                  destination_register_name,
                                  block_index,
                                  block.insts.size(),
                                  storage_transfer_reason("return",
                                                          source_kind,
                                                          consumed_kind));
  }
}

void append_spill_reload_ops(const PreparedLivenessFunction& liveness_function,
                             const std::vector<std::optional<std::size_t>>& spill_points,
                             PreparedRegallocFunction& regalloc_function) {
  for (std::size_t value_index = 0; value_index < regalloc_function.values.size(); ++value_index) {
    const auto spill_point = spill_points[value_index];
    const auto& value = regalloc_function.values[value_index];
    if (!spill_point.has_value() || !value.assigned_stack_slot.has_value()) {
      continue;
    }

    if (const auto spill_location = locate_program_point(liveness_function, *spill_point);
        spill_location.has_value()) {
      regalloc_function.spill_reload_ops.push_back(PreparedSpillReloadOp{
          .value_id = value.value_id,
          .op_kind = PreparedSpillReloadOpKind::Spill,
          .block_index = spill_location->block_index,
          .instruction_index = spill_location->instruction_index,
      });
    }

    std::optional<std::size_t> last_reload_point;
    for (const std::size_t use_point : liveness_function.values[value_index].use_points) {
      if (use_point <= *spill_point || last_reload_point == use_point) {
        continue;
      }
      if (const auto reload_location = locate_program_point(liveness_function, use_point);
          reload_location.has_value()) {
        regalloc_function.spill_reload_ops.push_back(PreparedSpillReloadOp{
            .value_id = value.value_id,
            .op_kind = PreparedSpillReloadOpKind::Reload,
            .block_index = reload_location->block_index,
            .instruction_index = reload_location->instruction_index,
        });
        last_reload_point = use_point;
      }
    }
  }
}

}  // namespace

void BirPreAlloc::run_regalloc() {
  prepared_.completed_phases.push_back("regalloc");
  prepared_.regalloc.functions.clear();
  prepared_.regalloc.functions.reserve(prepared_.liveness.functions.size());

  for (const auto& liveness_function : prepared_.liveness.functions) {
    PreparedRegallocFunction regalloc_function{
        .function_name = liveness_function.function_name,
        .values = {},
        .constraints = {},
        .interference = {},
        .move_resolution = {},
        .spill_reload_ops = {},
    };
    regalloc_function.values.reserve(liveness_function.values.size());
    regalloc_function.constraints.reserve(liveness_function.values.size());

    for (const auto& liveness_value : liveness_function.values) {
      const PreparedRegisterClass register_class = classify_register_class(liveness_value);
      const bool eligible_for_register_seed =
          register_class != PreparedRegisterClass::None &&
          liveness_value.value_kind != PreparedValueKind::StackObject;
      const std::size_t base_priority = value_priority(liveness_value);
      const std::size_t weighted_use_priority = weighted_use_score(liveness_function, liveness_value);
      const std::size_t priority = weighted_use_priority +
                                   (base_priority >= liveness_value.use_points.size()
                                        ? base_priority - liveness_value.use_points.size()
                                        : 0U);
      const auto caller_saved_names = caller_saved_registers(prepared_.target, register_class);
      const auto callee_saved_names = callee_saved_registers(prepared_.target, register_class);

      regalloc_function.values.push_back(PreparedRegallocValue{
          .value_id = liveness_value.value_id,
          .stack_object_id = liveness_value.stack_object_id,
          .function_name = liveness_value.function_name,
          .value_name = liveness_value.value_name,
          .type = liveness_value.type,
          .value_kind = liveness_value.value_kind,
          .register_class = register_class,
          .allocation_status = PreparedAllocationStatus::Unallocated,
          .spillable = eligible_for_register_seed && !liveness_value.requires_home_slot,
          .requires_home_slot = liveness_value.requires_home_slot,
          .crosses_call = liveness_value.crosses_call,
          .priority = priority,
          .spill_weight = static_cast<double>(priority),
          .live_interval = liveness_value.live_interval,
          .assigned_register = std::nullopt,
          .assigned_stack_slot = std::nullopt,
      });

      if (!eligible_for_register_seed) {
        continue;
      }

      regalloc_function.constraints.push_back(PreparedAllocationConstraint{
          .value_id = liveness_value.value_id,
          .register_class = register_class,
          .requires_register = !liveness_value.requires_home_slot,
          .requires_home_slot = liveness_value.requires_home_slot,
          .cannot_cross_call = liveness_value.crosses_call &&
                               register_class != PreparedRegisterClass::Float,
          .fixed_register_name = std::nullopt,
          .preferred_register_names = materialize_register_names(liveness_value.crosses_call
                                                                    ? callee_saved_names
                                                                    : caller_saved_names),
          .forbidden_register_names = materialize_register_names(liveness_value.crosses_call
                                                                     ? caller_saved_names
                                                                     : std::vector<std::string_view>{}),
      });
    }

    for (std::size_t lhs_index = 0; lhs_index < regalloc_function.values.size(); ++lhs_index) {
      const auto& lhs = regalloc_function.values[lhs_index];
      if (!lhs.live_interval.has_value() || lhs.register_class == PreparedRegisterClass::None) {
        continue;
      }
      for (std::size_t rhs_index = lhs_index + 1U; rhs_index < regalloc_function.values.size(); ++rhs_index) {
        const auto& rhs = regalloc_function.values[rhs_index];
        if (!rhs.live_interval.has_value() || rhs.register_class == PreparedRegisterClass::None) {
          continue;
        }
        if (!intervals_overlap(*lhs.live_interval, *rhs.live_interval)) {
          continue;
        }
        regalloc_function.interference.push_back(PreparedInterferenceEdge{
            .lhs_value_id = lhs.value_id,
            .rhs_value_id = rhs.value_id,
            .reason = "overlapping_live_intervals",
        });
      }
    }

    PreparedFrameSlotId next_slot_id = 0;
    std::size_t next_offset_bytes = prepared_.stack_layout.frame_size_bytes;
    std::size_t frame_alignment_bytes = prepared_.stack_layout.frame_alignment_bytes;
    for (const auto& slot : prepared_.stack_layout.frame_slots) {
      next_slot_id = std::max(next_slot_id, slot.slot_id + 1U);
      next_offset_bytes = std::max(next_offset_bytes, slot.offset_bytes + slot.size_bytes);
      frame_alignment_bytes = std::max(frame_alignment_bytes, slot.align_bytes);
    }

    std::vector<std::size_t> allocation_order(regalloc_function.values.size());
    for (std::size_t index = 0; index < allocation_order.size(); ++index) {
      allocation_order[index] = index;
    }
    std::sort(allocation_order.begin(),
              allocation_order.end(),
              [&regalloc_function](std::size_t lhs_index, std::size_t rhs_index) {
                const auto& lhs = regalloc_function.values[lhs_index];
                const auto& rhs = regalloc_function.values[rhs_index];
                const std::size_t lhs_start = interval_start_sort_key(lhs);
                const std::size_t rhs_start = interval_start_sort_key(rhs);
                if (lhs_start != rhs_start) {
                  return lhs_start < rhs_start;
                }
                if (lhs.priority != rhs.priority) {
                  return lhs.priority > rhs.priority;
                }
                return lhs.value_id < rhs.value_id;
              });

    std::vector<ActiveRegisterAssignment> active_caller_saved_assignments;
    active_caller_saved_assignments.reserve(regalloc_function.values.size());
    std::vector<ActiveRegisterAssignment> active_callee_saved_assignments;
    active_callee_saved_assignments.reserve(regalloc_function.values.size());
    std::vector<std::optional<std::size_t>> spill_points(regalloc_function.values.size(), std::nullopt);
    std::size_t assigned_register_count = 0;
    std::size_t assigned_stack_count = 0;

    auto assign_from_pool = [&](const auto& should_consider,
                                const auto& register_pool_for_value,
                                std::vector<ActiveRegisterAssignment>& active_assignments,
                                const auto& can_evict_assignment) {
      for (const std::size_t value_index : allocation_order) {
        auto& value = regalloc_function.values[value_index];
        if (value.assigned_register.has_value() || value.assigned_stack_slot.has_value() ||
            value.requires_home_slot || !value.live_interval.has_value() ||
            value.register_class == PreparedRegisterClass::None || !should_consider(value)) {
          continue;
        }

        expire_completed_assignments(active_assignments, value.live_interval->start_point);
        const auto register_names = register_pool_for_value(value);
        if (const auto chosen_register = choose_register(active_assignments, register_names);
            chosen_register.has_value()) {
          value.assigned_register = PreparedPhysicalRegisterAssignment{
              .reg_class = value.register_class,
              .register_name = std::move(*chosen_register),
          };
          value.allocation_status = PreparedAllocationStatus::AssignedRegister;
          active_assignments.push_back(ActiveRegisterAssignment{
              .value_index = value_index,
              .end_point = value.live_interval->end_point,
              .register_name = value.assigned_register->register_name,
          });
          ++assigned_register_count;
          continue;
        }

        const auto eviction_index = choose_eviction_candidate(regalloc_function,
                                                              active_assignments,
                                                              register_names,
                                                              value,
                                                              can_evict_assignment);
        if (!eviction_index.has_value()) {
          continue;
        }

        auto& evicted_value = regalloc_function.values[active_assignments[*eviction_index].value_index];
        spill_points[active_assignments[*eviction_index].value_index] = value.live_interval->start_point;
        value.assigned_register = PreparedPhysicalRegisterAssignment{
            .reg_class = value.register_class,
            .register_name = active_assignments[*eviction_index].register_name,
        };
        value.allocation_status = PreparedAllocationStatus::AssignedRegister;

        evicted_value.assigned_register.reset();
        evicted_value.allocation_status = PreparedAllocationStatus::Unallocated;

        active_assignments[*eviction_index] = ActiveRegisterAssignment{
            .value_index = value_index,
            .end_point = value.live_interval->end_point,
            .register_name = value.assigned_register->register_name,
        };
      }
    };

    assign_from_pool([](const PreparedRegallocValue& value) { return value.crosses_call; },
                     [this](const PreparedRegallocValue& value) {
                       return callee_saved_registers(prepared_.target, value.register_class);
                     },
                     active_callee_saved_assignments,
                     [](const ActiveRegisterAssignment&) { return true; });
    assign_from_pool([](const PreparedRegallocValue& value) { return !value.crosses_call; },
                     [this](const PreparedRegallocValue& value) {
                       return caller_saved_registers(prepared_.target, value.register_class);
                     },
                     active_caller_saved_assignments,
                     [](const ActiveRegisterAssignment&) { return true; });
    assign_from_pool([](const PreparedRegallocValue& value) { return !value.crosses_call; },
                     [this](const PreparedRegallocValue& value) {
                       return callee_saved_registers(prepared_.target, value.register_class);
                     },
                     active_callee_saved_assignments,
                     [&regalloc_function](const ActiveRegisterAssignment& assignment) {
                       return !regalloc_function.values[assignment.value_index].crosses_call;
                     });

    for (const std::size_t value_index : allocation_order) {
      auto& value = regalloc_function.values[value_index];

      if (value.assigned_register.has_value() || value.assigned_stack_slot.has_value()) {
        continue;
      }

      if (value.requires_home_slot) {
        value.assigned_stack_slot = allocate_stack_slot(regalloc_function.function_name,
                                                        value,
                                                        prepared_.stack_layout,
                                                        next_slot_id,
                                                        next_offset_bytes,
                                                        frame_alignment_bytes);
        value.allocation_status = PreparedAllocationStatus::AssignedStackSlot;
        ++assigned_stack_count;
        continue;
      }

      if (!value.live_interval.has_value() || value.register_class == PreparedRegisterClass::None) {
        if (normalized_value_size(value) != 0U) {
          value.assigned_stack_slot = allocate_stack_slot(regalloc_function.function_name,
                                                          value,
                                                          prepared_.stack_layout,
                                                          next_slot_id,
                                                          next_offset_bytes,
                                                          frame_alignment_bytes);
          value.allocation_status = PreparedAllocationStatus::AssignedStackSlot;
          ++assigned_stack_count;
        }
        continue;
      }

      value.assigned_stack_slot = allocate_stack_slot(regalloc_function.function_name,
                                                      value,
                                                      prepared_.stack_layout,
                                                      next_slot_id,
                                                      next_offset_bytes,
                                                      frame_alignment_bytes);
      value.allocation_status = PreparedAllocationStatus::AssignedStackSlot;
      ++assigned_stack_count;
    }

    append_spill_reload_ops(liveness_function, spill_points, regalloc_function);
    if (const auto* function = find_bir_function(prepared_.module, regalloc_function.function_name);
        function != nullptr) {
      append_phi_move_resolution(*function, regalloc_function);
      append_consumer_move_resolution(*function, regalloc_function);
      append_call_arg_move_resolution(prepared_.target, *function, regalloc_function);
      append_call_result_move_resolution(prepared_.target, *function, regalloc_function);
      append_return_move_resolution(prepared_.target, *function, regalloc_function);
    }

    prepared_.stack_layout.frame_size_bytes =
        std::max(prepared_.stack_layout.frame_size_bytes,
                 stack_layout::align_to(next_offset_bytes, std::max<std::size_t>(frame_alignment_bytes, 1U)));
    prepared_.stack_layout.frame_alignment_bytes =
        std::max(prepared_.stack_layout.frame_alignment_bytes, frame_alignment_bytes);

    prepared_.notes.push_back(PrepareNote{
        .phase = "regalloc",
        .message = "regalloc seeded function '" + regalloc_function.function_name + "' with " +
                   std::to_string(regalloc_function.constraints.size()) +
                   " allocation constraint(s) and " +
                   std::to_string(regalloc_function.interference.size()) +
                   " interference edge(s) from active liveness; assigned " +
                   std::to_string(assigned_register_count) + " register(s) and " +
                   std::to_string(assigned_stack_count) + " stack slot(s)",
    });
    prepared_.regalloc.functions.push_back(std::move(regalloc_function));
  }
}

}  // namespace c4c::backend::prepare
