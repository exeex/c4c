#include "prealloc.hpp"
#include "target_register_profile.hpp"

#include <algorithm>
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
    case bir::TypeKind::F128:
      return PreparedRegisterBank::Fpr;
    case bir::TypeKind::I128:
      return PreparedRegisterBank::Vreg;
    case bir::TypeKind::Void:
      return PreparedRegisterBank::None;
  }
  return PreparedRegisterBank::None;
}

[[nodiscard]] PreparedRegisterBank register_bank_from_arg_abi(const bir::CallArgAbiInfo& abi) {
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

[[nodiscard]] bool is_dynamic_alloca_call(std::string_view callee) {
  return callee.substr(0, std::string_view("llvm.dynamic_alloca.").size()) ==
         "llvm.dynamic_alloca.";
}

[[nodiscard]] std::string dynamic_alloca_type_text(std::string_view callee) {
  constexpr std::string_view kPrefix = "llvm.dynamic_alloca.";
  if (!is_dynamic_alloca_call(callee)) {
    return {};
  }
  return std::string(callee.substr(kPrefix.size()));
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

[[nodiscard]] PreparedCallWrapperKind classify_call_wrapper_kind(const bir::Module& module,
                                                                 const bir::CallInst& call) {
  if (call.is_indirect) {
    return PreparedCallWrapperKind::Indirect;
  }

  if (const auto* callee = find_module_function(module, call.callee); callee != nullptr) {
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

[[nodiscard]] const PreparedRegallocValue* find_regalloc_value_by_id(
    const PreparedRegallocFunction& function,
    PreparedValueId value_id) {
  for (const auto& value : function.values) {
    if (value.value_id == value_id) {
      return &value;
    }
  }
  return nullptr;
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

[[nodiscard]] PreparedStoragePlanValue build_storage_plan_value(
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueHome& home,
    bir::TypeKind type) {
  return PreparedStoragePlanValue{
      .value_id = home.value_id,
      .value_name = home.value_name,
      .encoding = storage_encoding_from_home(home),
      .bank = published_bank_for_value(regalloc_function, home.value_name, type),
      .register_name = home.register_name,
      .slot_id = home.slot_id,
      .stack_offset_bytes = home.offset_bytes,
      .immediate_i32 = home.immediate_i32,
      .symbol_name = std::nullopt,
  };
}

[[nodiscard]] std::optional<PreparedIndirectCalleePlan> build_indirect_callee_plan(
    const PreparedNameTables& names,
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
      .encoding = PreparedStorageEncodingKind::None,
      .bank = published_bank_for_value(regalloc_function, *value_name_id, call.callee_value->type),
      .register_name = std::nullopt,
      .slot_id = std::nullopt,
      .stack_offset_bytes = std::nullopt,
      .immediate_i32 = std::nullopt,
      .pointer_base_value_name = std::nullopt,
      .pointer_byte_delta = std::nullopt,
  };

  if (value_locations == nullptr) {
    return plan;
  }

  if (const auto* home = find_prepared_value_home(*value_locations, *value_name_id); home != nullptr) {
    plan.encoding = storage_encoding_from_home(*home);
    plan.register_name = home->register_name;
    plan.slot_id = home->slot_id;
    plan.stack_offset_bytes = home->offset_bytes;
    plan.immediate_i32 = home->immediate_i32;
    plan.pointer_base_value_name = home->pointer_base_value_name;
    plan.pointer_byte_delta = home->pointer_byte_delta;
  }

  return plan;
}

[[nodiscard]] std::optional<PreparedMemoryReturnPlan> build_memory_return_plan(
    const PreparedNameTables& names,
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

  const SlotNameId slot_name_id = names.slot_names.find(*call.sret_storage_name);
  if (slot_name_id == kInvalidSlotName) {
    return plan;
  }
  plan.storage_slot_name = slot_name_id;

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

[[nodiscard]] std::vector<PreparedClobberedRegister> build_call_clobber_set(
    const c4c::TargetProfile& target_profile) {
  std::vector<PreparedClobberedRegister> clobbers;
  for (PreparedRegisterClass reg_class :
       {PreparedRegisterClass::General, PreparedRegisterClass::Float, PreparedRegisterClass::Vector}) {
    const PreparedRegisterBank bank = register_bank_from_class(reg_class);
    for (const auto& register_span : caller_saved_register_spans(target_profile, reg_class, 1)) {
      clobbers.push_back(PreparedClobberedRegister{
          .bank = bank,
          .register_name = register_span.register_name,
          .contiguous_width = register_span.contiguous_width,
          .occupied_register_names = register_span.occupied_register_names,
      });
    }
  }
  return clobbers;
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

    std::vector<PreparedSavedRegister> saved_registers;
    if (const auto* regalloc_function = find_regalloc_function(prepared.regalloc, function_name_id);
        regalloc_function != nullptr) {
      for (PreparedRegisterClass reg_class :
           {PreparedRegisterClass::General, PreparedRegisterClass::Float, PreparedRegisterClass::Vector}) {
        const auto callee_saved = callee_saved_register_spans(prepared.target_profile, reg_class, 1);
        for (std::size_t save_index = 0; save_index < callee_saved.size(); ++save_index) {
          const auto& callee_saved_span = callee_saved[save_index];
          bool used = false;
          for (const auto& value : regalloc_function->values) {
            if (!value.assigned_register.has_value()) {
              continue;
            }
            if (value.assigned_register->register_name == callee_saved_span.register_name) {
              used = true;
              break;
            }
          }
          if (!used) {
            continue;
          }
          saved_registers.push_back(PreparedSavedRegister{
              .bank = register_bank_from_class(reg_class),
              .register_name = callee_saved_span.register_name,
              .contiguous_width = callee_saved_span.contiguous_width,
              .occupied_register_names = callee_saved_span.occupied_register_names,
              .save_index = save_index,
          });
        }
      }
    }
    plan.saved_callee_registers = std::move(saved_registers);

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
    plan.uses_frame_pointer_for_fixed_slots =
        plan.has_dynamic_stack && !plan.frame_slot_order.empty();

    prepared.frame_plan.functions.push_back(std::move(plan));
  }
}

void populate_dynamic_stack_plan(PreparedBirModule& prepared) {
  prepared.dynamic_stack_plan.functions.clear();

  for (const auto& function : prepared.module.functions) {
    if (function.is_declaration) {
      continue;
    }
    const FunctionNameId function_name_id = prepared.names.function_names.find(function.name);
    if (function_name_id == kInvalidFunctionName) {
      continue;
    }

    PreparedDynamicStackPlanFunction function_plan{
        .function_name = function_name_id,
        .requires_stack_save_restore = false,
        .uses_frame_pointer_for_fixed_slots = false,
        .operations = {},
    };

    for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
      const auto& block = function.blocks[block_index];
      const BlockLabelId block_label_id = prepared.names.block_labels.find(block.label);
      for (std::size_t inst_index = 0; inst_index < block.insts.size(); ++inst_index) {
        const auto* call = std::get_if<bir::CallInst>(&block.insts[inst_index]);
        if (call == nullptr) {
          continue;
        }

        std::optional<PreparedDynamicStackOpKind> op_kind;
        if (call->callee == "llvm.stacksave") {
          op_kind = PreparedDynamicStackOpKind::StackSave;
        } else if (is_dynamic_alloca_call(call->callee)) {
          op_kind = PreparedDynamicStackOpKind::DynamicAlloca;
        } else if (call->callee == "llvm.stackrestore") {
          op_kind = PreparedDynamicStackOpKind::StackRestore;
        }
        if (!op_kind.has_value()) {
          continue;
        }

        PreparedDynamicStackOp op{
            .function_name = function_name_id,
            .block_label = block_label_id,
            .instruction_index = inst_index,
            .kind = *op_kind,
            .result_value_name = std::nullopt,
            .operand_value_name = std::nullopt,
            .allocation_type_text = {},
            .element_size_bytes = 0,
            .element_align_bytes = 0,
        };
        if (call->result.has_value()) {
          op.result_value_name = maybe_named_value_id(prepared.names, *call->result);
        }
        if (!call->args.empty()) {
          op.operand_value_name = maybe_named_value_id(prepared.names, call->args.front());
        }
        if (*op_kind == PreparedDynamicStackOpKind::DynamicAlloca) {
          op.allocation_type_text = dynamic_alloca_type_text(call->callee);
        } else {
          function_plan.requires_stack_save_restore = true;
        }
        function_plan.operations.push_back(std::move(op));
      }
    }

    if (const auto* frame_plan = find_prepared_frame_plan(prepared, function_name_id);
        frame_plan != nullptr) {
      function_plan.uses_frame_pointer_for_fixed_slots =
          frame_plan->uses_frame_pointer_for_fixed_slots;
    }

    if (!function_plan.operations.empty()) {
      prepared.dynamic_stack_plan.functions.push_back(std::move(function_plan));
    }
  }
}

void populate_storage_plans(PreparedBirModule& prepared) {
  prepared.storage_plans.functions.clear();

  for (const auto& function_locations : prepared.value_locations.functions) {
    PreparedStoragePlanFunction function_plan{
        .function_name = function_locations.function_name,
        .values = {},
    };

    const auto* regalloc_function = find_regalloc_function(prepared.regalloc, function_locations.function_name);
    function_plan.values.reserve(function_locations.value_homes.size());
    for (const auto& home : function_locations.value_homes) {
      bir::TypeKind type = bir::TypeKind::Void;
      if (regalloc_function != nullptr) {
        if (const auto* regalloc_value = find_regalloc_value_by_id(*regalloc_function, home.value_id);
            regalloc_value != nullptr) {
          type = regalloc_value->type;
        }
      }
      function_plan.values.push_back(build_storage_plan_value(regalloc_function, home, type));
    }

    if (!function_plan.values.empty()) {
      prepared.storage_plans.functions.push_back(std::move(function_plan));
    }
  }
}

void populate_call_plans(PreparedBirModule& prepared) {
  prepared.call_plans.functions.clear();
  const std::vector<PreparedClobberedRegister> call_clobbers =
      build_call_clobber_set(prepared.target_profile);

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
    const auto* regalloc_function = find_regalloc_function(prepared.regalloc, function_name_id);
    const auto* value_locations = find_prepared_value_location_function(prepared, function_name_id);

    for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
      const auto& block = function.blocks[block_index];
      for (std::size_t instruction_index = 0; instruction_index < block.insts.size(); ++instruction_index) {
        const auto* call = std::get_if<bir::CallInst>(&block.insts[instruction_index]);
        if (call == nullptr) {
          continue;
        }

        PreparedCallPlan call_plan{
            .block_index = block_index,
            .instruction_index = instruction_index,
            .wrapper_kind = classify_call_wrapper_kind(prepared.module, *call),
            .variadic_fpr_arg_register_count = variadic_fpr_arg_register_count(*call),
            .is_indirect = call->is_indirect,
            .direct_callee_name = call->is_indirect ? std::nullopt
                                                    : std::optional<std::string>{call->callee},
            .indirect_callee =
                build_indirect_callee_plan(prepared.names, regalloc_function, value_locations, *call),
            .memory_return =
                build_memory_return_plan(prepared.names, prepared.stack_layout, function_name_id, *call),
            .arguments = {},
            .result = std::nullopt,
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
              .source_literal = std::nullopt,
              .source_symbol_name = std::nullopt,
              .source_register_name = std::nullopt,
              .source_slot_id = std::nullopt,
              .source_stack_offset_bytes = std::nullopt,
              .source_register_bank = std::nullopt,
              .source_base_value_name = std::nullopt,
              .source_pointer_byte_delta = std::nullopt,
              .destination_register_name = std::nullopt,
              .destination_register_bank = std::nullopt,
              .destination_stack_offset_bytes = std::nullopt,
          };

          if (const auto* binding = find_call_abi_binding(before_call_bundle,
                                                          PreparedMoveDestinationKind::CallArgumentAbi,
                                                          arg_index);
              binding != nullptr) {
            arg_plan.destination_register_name = binding->destination_register_name;
            arg_plan.destination_stack_offset_bytes = binding->destination_stack_offset_bytes;
            if (binding->destination_register_name.has_value()) {
              arg_plan.destination_register_bank = arg_plan.value_bank;
            }
          }

          if (arg_index < call->args.size()) {
            if (const auto value_name_id = maybe_named_value_id(prepared.names, call->args[arg_index]);
                value_name_id.has_value() && value_locations != nullptr) {
              if (const auto* home = find_prepared_value_home(*value_locations, *value_name_id);
                  home != nullptr) {
                arg_plan.source_encoding = storage_encoding_from_home(*home);
                arg_plan.source_register_name = home->register_name;
                arg_plan.source_slot_id = home->slot_id;
                arg_plan.source_stack_offset_bytes = home->offset_bytes;
                if (home->immediate_i32.has_value()) {
                  arg_plan.source_literal = bir::Value::immediate_i32(*home->immediate_i32);
                }
                arg_plan.source_base_value_name = home->pointer_base_value_name;
                arg_plan.source_pointer_byte_delta = home->pointer_byte_delta;
              }
              if (regalloc_function != nullptr) {
                if (const auto* regalloc_value =
                        find_regalloc_value_by_name(*regalloc_function, *value_name_id);
                    regalloc_value != nullptr) {
                  arg_plan.source_value_id = regalloc_value->value_id;
                  arg_plan.source_register_bank =
                      register_bank_from_class(regalloc_value->register_class);
                }
              }
              if (!call->args[arg_index].name.empty() && call->args[arg_index].name.front() == '@') {
                arg_plan.source_encoding = PreparedStorageEncodingKind::SymbolAddress;
                arg_plan.source_symbol_name = call->args[arg_index].name;
              }
            } else if (call->args[arg_index].kind == bir::Value::Kind::Named &&
                       !call->args[arg_index].name.empty() &&
                       call->args[arg_index].name.front() == '@') {
              arg_plan.source_encoding = PreparedStorageEncodingKind::SymbolAddress;
              arg_plan.source_symbol_name = call->args[arg_index].name;
            } else if (call->args[arg_index].kind != bir::Value::Kind::Named) {
              arg_plan.source_encoding = PreparedStorageEncodingKind::Immediate;
              arg_plan.source_literal = call->args[arg_index];
            }
          }

          call_plan.arguments.push_back(std::move(arg_plan));
        }

        if (call->result.has_value() && call->result->kind == bir::Value::Kind::Named &&
            value_locations != nullptr) {
          PreparedCallResultPlan result_plan{
              .instruction_index = instruction_index,
              .value_bank = call->result_abi.has_value()
                                ? register_bank_from_result_abi(*call->result_abi)
                                : register_bank_from_type(call->result->type),
              .destination_storage_kind = PreparedMoveStorageKind::None,
              .destination_value_id = std::nullopt,
              .source_register_name = std::nullopt,
              .source_register_bank = std::nullopt,
              .destination_register_name = std::nullopt,
              .destination_register_bank = std::nullopt,
              .destination_slot_id = std::nullopt,
              .destination_stack_offset_bytes = std::nullopt,
          };

          if (const auto* binding = find_call_abi_binding(after_call_bundle,
                                                          PreparedMoveDestinationKind::CallResultAbi,
                                                          std::nullopt);
              binding != nullptr) {
            result_plan.source_register_name = binding->destination_register_name;
            if (binding->destination_register_name.has_value()) {
              result_plan.source_register_bank = result_plan.value_bank;
            }
          }

          if (const auto value_name_id = maybe_named_value_id(prepared.names, *call->result);
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
            }
            if (regalloc_function != nullptr) {
              if (const auto* regalloc_value =
                      find_regalloc_value_by_name(*regalloc_function, *value_name_id);
                  regalloc_value != nullptr) {
                result_plan.destination_value_id = regalloc_value->value_id;
              }
            }
          }
          call_plan.result = std::move(result_plan);
        }

        function_plan.calls.push_back(std::move(call_plan));
      }
    }

    if (!function_plan.calls.empty()) {
      prepared.call_plans.functions.push_back(std::move(function_plan));
    }
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
  populate_frame_plan(prepared_);
  populate_dynamic_stack_plan(prepared_);
  populate_call_plans(prepared_);
  populate_storage_plans(prepared_);
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
