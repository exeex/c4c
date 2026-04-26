#include "prealloc.hpp"

#include <algorithm>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace c4c::backend::prepare {

namespace {

bool should_promote_i1(const c4c::TargetProfile& target_profile) {
  switch (target_profile.arch) {
    case c4c::TargetArch::X86_64:
    case c4c::TargetArch::I686:
    case c4c::TargetArch::Aarch64:
    case c4c::TargetArch::Riscv64:
      return true;
    case c4c::TargetArch::Unknown:
      return false;
  }
  return false;
}

bir::TypeKind legalize_type(const c4c::TargetProfile& target_profile, bir::TypeKind type) {
  if (should_promote_i1(target_profile) && type == bir::TypeKind::I1) {
    return bir::TypeKind::I32;
  }
  return type;
}

void legalize_value(const c4c::TargetProfile& target_profile, bir::Value& value) {
  const auto original_type = value.type;
  value.type = legalize_type(target_profile, value.type);
  if (original_type == bir::TypeKind::I1 && value.type == bir::TypeKind::I32 &&
      value.kind == bir::Value::Kind::Immediate) {
    value.immediate = value.immediate != 0 ? 1 : 0;
    value.immediate_bits = value.immediate != 0 ? 1u : 0u;
  }
}

std::size_t type_size_bytes(bir::TypeKind type) {
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
    default:
      return 0;
  }
}

void legalize_sized_type(const c4c::TargetProfile& target_profile,
                         bir::TypeKind& type,
                         std::size_t& size_bytes,
                         std::size_t& align_bytes) {
  const auto original_type = type;
  type = legalize_type(target_profile, type);
  if (original_type != type) {
    const auto legalized_size = type_size_bytes(type);
    size_bytes = legalized_size;
    align_bytes = legalized_size;
  }
}

void legalize_call_arg_abi(const c4c::TargetProfile& target_profile, bir::CallArgAbiInfo& abi) {
  legalize_sized_type(target_profile, abi.type, abi.size_bytes, abi.align_bytes);
}

void legalize_call_result_abi(const c4c::TargetProfile& target_profile,
                              bir::CallResultAbiInfo& abi) {
  abi.type = legalize_type(target_profile, abi.type);
}

std::optional<bir::CallArgAbiInfo> infer_call_arg_abi_impl(
    const c4c::TargetProfile& target_profile,
    bir::TypeKind type) {
  if (type == bir::TypeKind::Void) {
    return std::nullopt;
  }

  bir::CallArgAbiInfo abi{
      .type = type,
      .size_bytes = type_size_bytes(type),
      .align_bytes = type_size_bytes(type),
      .passed_in_register = true,
  };
  switch (type) {
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
      abi.primary_class = target_profile.has_float_arg_registers
                              ? bir::AbiValueClass::Sse
                              : bir::AbiValueClass::Integer;
      return abi;
    case bir::TypeKind::F128:
      if (target_profile.arch == c4c::TargetArch::X86_64) {
        abi.primary_class = bir::AbiValueClass::Memory;
        abi.passed_in_register = false;
        abi.passed_on_stack = true;
        return abi;
      }
      abi.primary_class = target_profile.has_float_arg_registers
                              ? bir::AbiValueClass::Sse
                              : bir::AbiValueClass::Integer;
      return abi;
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      abi.primary_class = bir::AbiValueClass::Integer;
      return abi;
    case bir::TypeKind::I128:
      abi.primary_class = bir::AbiValueClass::Memory;
      abi.passed_in_register = false;
      abi.passed_on_stack = true;
      return abi;
    case bir::TypeKind::Void:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<bir::CallResultAbiInfo> infer_call_result_abi(
    const c4c::TargetProfile& target_profile,
    bir::TypeKind return_type);

std::optional<bir::CallResultAbiInfo> infer_function_return_abi(
    const c4c::TargetProfile& target_profile,
    const bir::Function& function) {
  if (function.return_type == bir::TypeKind::Void) {
    return std::nullopt;
  }

  return infer_call_result_abi(target_profile, function.return_type);
}

std::optional<bir::CallResultAbiInfo> infer_call_result_abi(
    const c4c::TargetProfile& target_profile,
    bir::TypeKind return_type) {
  if (return_type == bir::TypeKind::Void) {
    return std::nullopt;
  }

  bir::CallResultAbiInfo abi;
  abi.type = return_type;
  switch (return_type) {
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      abi.primary_class = target_profile.has_float_return_registers
                              ? bir::AbiValueClass::Sse
                              : bir::AbiValueClass::Integer;
      break;
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      abi.primary_class = bir::AbiValueClass::Integer;
      break;
    case bir::TypeKind::I128:
      abi.primary_class = bir::AbiValueClass::Memory;
      abi.returned_in_memory = true;
      break;
    case bir::TypeKind::Void:
      return std::nullopt;
  }
  return abi;
}

void legalize_memory_access_metadata(const c4c::TargetProfile& target_profile,
                                     bir::TypeKind original_type,
                                     std::size_t& align_bytes,
                                     std::optional<bir::MemoryAddress>& address) {
  const auto legalized_type = legalize_type(target_profile, original_type);
  if (legalized_type == original_type) {
    return;
  }
  const auto original_size = type_size_bytes(original_type);
  const auto legalized_size = type_size_bytes(legalized_type);
  if (original_size == 0 || legalized_size == 0) {
    return;
  }
  if (align_bytes == original_size) {
    align_bytes = legalized_size;
  }
  if (!address.has_value()) {
    return;
  }
  if (address->size_bytes == original_size) {
    address->size_bytes = legalized_size;
  }
  if (address->align_bytes == original_size) {
    address->align_bytes = legalized_size;
  }
}

const bir::BinaryInst* find_compare_for_condition(const bir::Block& block,
                                                  const bir::Value& condition) {
  if (condition.kind != bir::Value::Kind::Named) {
    return nullptr;
  }
  for (const auto& inst : block.insts) {
    const auto* binary = std::get_if<bir::BinaryInst>(&inst);
    if (binary == nullptr || binary->result.name != condition.name ||
        !bir::is_compare_opcode(binary->opcode)) {
      continue;
    }
    return binary;
  }
  return nullptr;
}

BlockLabelId intern_prepared_block_label(PreparedNameTables& names,
                                         const bir::NameTables& bir_names,
                                         BlockLabelId label_id,
                                         std::string_view raw_label) {
  if (label_id != kInvalidBlockLabel) {
    const std::string_view structured_label = bir_names.block_labels.spelling(label_id);
    if (!structured_label.empty()) {
      return names.block_labels.intern(structured_label);
    }
  }
  return names.block_labels.intern(raw_label);
}

PreparedBranchCondition make_branch_condition(PreparedNameTables& names,
                                              const bir::NameTables& bir_names,
                                              FunctionNameId function_name,
                                              const bir::Block& block) {
  PreparedBranchCondition condition{
      .function_name = function_name,
      .block_label =
          intern_prepared_block_label(names, bir_names, block.label_id, block.label),
      .condition_value = block.terminator.condition,
      .true_label = intern_prepared_block_label(
          names, bir_names, block.terminator.true_label_id, block.terminator.true_label),
      .false_label = intern_prepared_block_label(
          names, bir_names, block.terminator.false_label_id, block.terminator.false_label),
  };
  if (const auto* compare = find_compare_for_condition(block, block.terminator.condition);
      compare != nullptr) {
    condition.kind = PreparedBranchConditionKind::FusedCompare;
    condition.predicate = compare->opcode;
    condition.compare_type = compare->operand_type;
    condition.lhs = compare->lhs;
    condition.rhs = compare->rhs;
    condition.can_fuse_with_branch = true;
  }
  return condition;
}

void legalize_module(const c4c::TargetProfile& target_profile,
                     bir::Module& module,
                     PreparedNameTables& names,
                     PreparedControlFlow* control_flow) {
  for (auto& global : module.globals) {
    legalize_sized_type(target_profile, global.type, global.size_bytes, global.align_bytes);
    if (global.initializer.has_value()) {
      legalize_value(target_profile, *global.initializer);
    }
    for (auto& element : global.initializer_elements) {
      legalize_value(target_profile, element);
    }
  }

  for (auto& function : module.functions) {
    const FunctionNameId function_name_id = names.function_names.intern(function.name);
    PreparedControlFlowFunction function_control_flow{
        .function_name = function_name_id,
    };
    legalize_sized_type(
        target_profile, function.return_type, function.return_size_bytes, function.return_align_bytes);
    if (!function.return_abi.has_value()) {
      function.return_abi = infer_function_return_abi(target_profile, function);
    }
    if (function.return_abi.has_value()) {
      legalize_call_result_abi(target_profile, *function.return_abi);
      if (function.return_abi->type == bir::TypeKind::Void &&
          !function.return_abi->returned_in_memory &&
          function.return_abi->primary_class != bir::AbiValueClass::Memory) {
        function.return_abi.reset();
      }
    }
    for (auto& param : function.params) {
      legalize_sized_type(target_profile, param.type, param.size_bytes, param.align_bytes);
      if (!param.abi.has_value()) {
        param.abi = infer_call_arg_abi_impl(target_profile, param.type);
        if (param.abi.has_value()) {
          if (param.is_sret) {
            param.abi->sret_pointer = true;
          }
          if (param.is_byval) {
            param.abi->byval_copy = true;
            param.abi->size_bytes = param.size_bytes;
            param.abi->align_bytes = param.align_bytes;
          }
        }
      }
      if (param.abi.has_value()) {
        legalize_call_arg_abi(target_profile, *param.abi);
      }
    }
    for (auto& slot : function.local_slots) {
      legalize_sized_type(target_profile, slot.type, slot.size_bytes, slot.align_bytes);
    }
    for (auto& block : function.blocks) {
      for (auto& inst : block.insts) {
        std::visit(
            [&](auto& lowered) {
              using T = std::decay_t<decltype(lowered)>;
              if constexpr (std::is_same_v<T, bir::BinaryInst>) {
                lowered.result.type = legalize_type(target_profile, lowered.result.type);
                lowered.operand_type = legalize_type(target_profile, lowered.operand_type);
                legalize_value(target_profile, lowered.lhs);
                legalize_value(target_profile, lowered.rhs);
              } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
                lowered.result.type = legalize_type(target_profile, lowered.result.type);
                lowered.compare_type = legalize_type(target_profile, lowered.compare_type);
                legalize_value(target_profile, lowered.lhs);
                legalize_value(target_profile, lowered.rhs);
                legalize_value(target_profile, lowered.true_value);
                legalize_value(target_profile, lowered.false_value);
              } else if constexpr (std::is_same_v<T, bir::CastInst>) {
                lowered.result.type = legalize_type(target_profile, lowered.result.type);
                legalize_value(target_profile, lowered.operand);
              } else if constexpr (std::is_same_v<T, bir::PhiInst>) {
                lowered.result.type = legalize_type(target_profile, lowered.result.type);
                for (auto& incoming : lowered.incomings) {
                  legalize_value(target_profile, incoming.value);
                }
              } else if constexpr (std::is_same_v<T, bir::CallInst>) {
                lowered.return_type = legalize_type(target_profile, lowered.return_type);
                if (!lowered.return_type_name.empty()) {
                  lowered.return_type_name = bir::render_type(lowered.return_type);
                }
                if (lowered.result.has_value()) {
                  legalize_value(target_profile, *lowered.result);
                }
                if (lowered.callee_value.has_value()) {
                  legalize_value(target_profile, *lowered.callee_value);
                }
                for (auto& arg : lowered.args) {
                  legalize_value(target_profile, arg);
                }
                for (auto& arg_type : lowered.arg_types) {
                  arg_type = legalize_type(target_profile, arg_type);
                }
                while (lowered.arg_abi.size() < lowered.arg_types.size()) {
                  const auto inferred_arg_abi =
                      infer_call_arg_abi_impl(target_profile, lowered.arg_types[lowered.arg_abi.size()]);
                  if (!inferred_arg_abi.has_value()) {
                    break;
                  }
                  lowered.arg_abi.push_back(*inferred_arg_abi);
                }
                for (auto& arg_abi : lowered.arg_abi) {
                  legalize_call_arg_abi(target_profile, arg_abi);
                }
                if (!lowered.result_abi.has_value() && lowered.result.has_value() &&
                    lowered.result->kind == bir::Value::Kind::Named) {
                  lowered.result_abi = infer_call_result_abi(target_profile, lowered.return_type);
                }
                if (lowered.result_abi.has_value()) {
                  legalize_call_result_abi(target_profile, *lowered.result_abi);
                }
              } else if constexpr (std::is_same_v<T, bir::LoadLocalInst> ||
                                   std::is_same_v<T, bir::LoadGlobalInst>) {
                const auto original_type = lowered.result.type;
                lowered.result.type = legalize_type(target_profile, lowered.result.type);
                legalize_memory_access_metadata(
                    target_profile, original_type, lowered.align_bytes, lowered.address);
                if (lowered.address.has_value()) {
                  legalize_value(target_profile, lowered.address->base_value);
                }
              } else if constexpr (std::is_same_v<T, bir::StoreLocalInst> ||
                                   std::is_same_v<T, bir::StoreGlobalInst>) {
                const auto original_type = lowered.value.type;
                legalize_value(target_profile, lowered.value);
                legalize_memory_access_metadata(
                    target_profile, original_type, lowered.align_bytes, lowered.address);
                if (lowered.address.has_value()) {
                  legalize_value(target_profile, lowered.address->base_value);
                }
              }
            },
            inst);
      }

      if (block.terminator.value.has_value()) {
        legalize_value(target_profile, *block.terminator.value);
      }
      legalize_value(target_profile, block.terminator.condition);
      PreparedControlFlowBlock prepared_block{
          .block_label =
              intern_prepared_block_label(names, module.names, block.label_id, block.label),
          .terminator_kind = block.terminator.kind,
      };
      if (block.terminator.kind == bir::TerminatorKind::Branch) {
        prepared_block.branch_target_label = intern_prepared_block_label(
            names, module.names, block.terminator.target_label_id, block.terminator.target_label);
      } else if (block.terminator.kind == bir::TerminatorKind::CondBranch) {
        prepared_block.true_label = intern_prepared_block_label(
            names, module.names, block.terminator.true_label_id, block.terminator.true_label);
        prepared_block.false_label = intern_prepared_block_label(
            names, module.names, block.terminator.false_label_id, block.terminator.false_label);
      }
      function_control_flow.blocks.push_back(std::move(prepared_block));
      if (block.terminator.kind == bir::TerminatorKind::CondBranch) {
        function_control_flow.branch_conditions.push_back(
            make_branch_condition(names, module.names, function_name_id, block));
      }
    }

    if (control_flow != nullptr &&
        (!function_control_flow.blocks.empty() ||
         !function_control_flow.branch_conditions.empty() ||
         !function_control_flow.join_transfers.empty() ||
         !function_control_flow.parallel_copy_bundles.empty())) {
      control_flow->functions.push_back(std::move(function_control_flow));
    }
  }
}

}  // namespace

std::optional<bir::CallArgAbiInfo> infer_call_arg_abi(
    const c4c::TargetProfile& target_profile,
    bir::TypeKind type) {
  return infer_call_arg_abi_impl(target_profile, type);
}

void BirPreAlloc::run_legalize() {
  prepared_.completed_phases.push_back("legalize");
  prepared_.control_flow.functions.clear();
  legalize_module(
      prepared_.target_profile, prepared_.module, prepared_.names, &prepared_.control_flow);
  if (should_promote_i1(prepared_.target_profile)) {
    prepared_.invariants.push_back(PreparedBirInvariant::NoTargetFacingI1);
  }

  prepared_.notes.push_back(PrepareNote{
      .phase = "legalize",
      .message =
          "bootstrap BIR legalize kept semantic CFG and phi form intact while promoting i1 "
          "values plus function signature/storage bookkeeping, memory-address/load-store "
          "bookkeeping, and call ABI metadata",
  });
}

}  // namespace c4c::backend::prepare
