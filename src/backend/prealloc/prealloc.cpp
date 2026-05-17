#include "prealloc.hpp"
#include "atomics.hpp"
#include "call_plans.hpp"
#include "dynamic_stack.hpp"
#include "f128_runtime_helpers.hpp"
#include "i128_runtime_helpers.hpp"
#include "inline_asm.hpp"
#include "intrinsics.hpp"
#include "label_identity.hpp"
#include "regalloc_placement_identity.hpp"
#include "special_carriers.hpp"
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
