#include "frame_plan.hpp"

#include "dynamic_stack.hpp"
#include "target_register_profile.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
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

[[nodiscard]] std::optional<PreparedRegisterPlacement> find_register_pool_placement(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class,
    std::size_t contiguous_width,
    const std::vector<std::string>& occupied_register_names,
    bool caller_pool) {
  if (occupied_register_names.empty()) {
    return std::nullopt;
  }
  const auto spans = caller_pool
                         ? caller_saved_register_spans(target_profile, reg_class, contiguous_width)
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
  if (const auto callee_saved = find_register_pool_placement(target_profile,
                                                             reg_class,
                                                             contiguous_width,
                                                             occupied_register_names,
                                                             false);
      callee_saved.has_value()) {
    return callee_saved;
  }
  return find_register_pool_placement(target_profile,
                                      reg_class,
                                      contiguous_width,
                                      occupied_register_names,
                                      true);
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

}  // namespace

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
        plan.frame_alignment_bytes = std::max(plan.frame_alignment_bytes, slot.align_bytes);
      }
    }

    std::vector<PreparedSavedRegister> saved_registers;
    if (const auto* regalloc_function = find_regalloc_function(prepared.regalloc, function_name_id);
        regalloc_function != nullptr) {
      for (const auto& value : regalloc_function->values) {
        if (value.assigned_stack_slot.has_value()) {
          plan.frame_size_bytes =
              std::max(plan.frame_size_bytes,
                       value.assigned_stack_slot->offset_bytes +
                           value.assigned_stack_slot->size_bytes.value_or(0));
          plan.frame_alignment_bytes =
              std::max(plan.frame_alignment_bytes,
                       value.assigned_stack_slot->align_bytes.value_or(1));
          plan.frame_slot_order.push_back(value.assigned_stack_slot->slot_id);
        }
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
            .placement =
                assignment_register_placement(prepared.target_profile, *value.assigned_register),
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
    std::unordered_map<PreparedFrameSlotId, std::size_t> frame_slot_offsets;
    frame_slot_offsets.reserve(prepared.stack_layout.frame_slots.size());
    for (const auto& slot : prepared.stack_layout.frame_slots) {
      if (slot.function_name == function_name_id) {
        frame_slot_offsets.emplace(slot.slot_id, slot.offset_bytes);
      }
    }
    std::sort(plan.frame_slot_order.begin(),
              plan.frame_slot_order.end(),
              [&frame_slot_offsets](auto lhs, auto rhs) {
                const auto lhs_it = frame_slot_offsets.find(lhs);
                const auto rhs_it = frame_slot_offsets.find(rhs);
                if (lhs_it == frame_slot_offsets.end() ||
                    rhs_it == frame_slot_offsets.end()) {
                  return lhs < rhs;
                }
                if (lhs_it->second != rhs_it->second) {
                  return lhs_it->second < rhs_it->second;
                }
                return lhs < rhs;
              });
    plan.frame_slot_order.erase(std::unique(plan.frame_slot_order.begin(),
                                            plan.frame_slot_order.end()),
                                plan.frame_slot_order.end());
    plan.saved_callee_registers = std::move(saved_registers);

    plan.uses_frame_pointer_for_fixed_slots =
        plan.has_dynamic_stack && !plan.frame_slot_order.empty();

    prepared.frame_plan.functions.push_back(std::move(plan));
  }
}

}  // namespace c4c::backend::prepare
