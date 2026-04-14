#include "legalize.hpp"

#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace c4c::backend::prepare {

namespace {

bool should_promote_i1(Target target) {
  switch (target) {
    case Target::X86_64:
    case Target::I686:
    case Target::Aarch64:
    case Target::Riscv64:
      return true;
  }
  return false;
}

bir::TypeKind legalize_type(Target target, bir::TypeKind type) {
  if (should_promote_i1(target) && type == bir::TypeKind::I1) {
    return bir::TypeKind::I32;
  }
  return type;
}

void legalize_value(Target target, bir::Value& value) {
  const auto original_type = value.type;
  value.type = legalize_type(target, value.type);
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
      return 4;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return 8;
    default:
      return 0;
  }
}

std::string make_phi_slot_name(std::string_view result_name) {
  return std::string(result_name) + ".phi";
}

void lower_phi_nodes(bir::Function* function) {
  struct PhiLoweringPlan {
    std::string slot_name;
    std::vector<bir::PhiIncoming> incomings;
  };

  std::unordered_map<std::string, std::vector<PhiLoweringPlan>> phis_by_block;
  std::unordered_set<std::string> used_slot_names;
  for (const auto& slot : function->local_slots) {
    used_slot_names.insert(slot.name);
  }

  for (auto& block : function->blocks) {
    std::vector<bir::Inst> rewritten;
    rewritten.reserve(block.insts.size());

    bool saw_non_phi = false;
    for (auto& inst : block.insts) {
      if (const auto* phi = std::get_if<bir::PhiInst>(&inst)) {
        if (saw_non_phi) {
          rewritten.push_back(inst);
          continue;
        }

        auto slot_name = make_phi_slot_name(phi->result.name);
        if (used_slot_names.emplace(slot_name).second) {
          const auto slot_size = type_size_bytes(phi->result.type);
          function->local_slots.push_back(bir::LocalSlot{
              .name = slot_name,
              .type = phi->result.type,
              .size_bytes = slot_size,
              .align_bytes = slot_size,
          });
        }

        phis_by_block[block.label].push_back(PhiLoweringPlan{
            .slot_name = slot_name,
            .incomings = phi->incomings,
        });
        rewritten.push_back(bir::LoadLocalInst{
            .result = phi->result,
            .slot_name = std::move(slot_name),
        });
        continue;
      }

      saw_non_phi = true;
      rewritten.push_back(std::move(inst));
    }

    block.insts = std::move(rewritten);
  }

  if (phis_by_block.empty()) {
    return;
  }

  std::unordered_map<std::string, bir::Block*> blocks_by_label;
  for (auto& block : function->blocks) {
    blocks_by_label.emplace(block.label, &block);
  }

  for (const auto& [block_label, plans] : phis_by_block) {
    (void)block_label;
    for (const auto& plan : plans) {
      for (const auto& incoming : plan.incomings) {
        const auto pred_it = blocks_by_label.find(incoming.label);
        if (pred_it == blocks_by_label.end()) {
          continue;
        }
        pred_it->second->insts.push_back(bir::StoreLocalInst{
            .slot_name = plan.slot_name,
            .value = incoming.value,
        });
      }
    }
  }
}

void legalize_module(Target target, bir::Module& module) {
  for (auto& global : module.globals) {
    global.type = legalize_type(target, global.type);
    if (global.initializer.has_value()) {
      legalize_value(target, *global.initializer);
    }
    for (auto& element : global.initializer_elements) {
      legalize_value(target, element);
    }
  }

  for (auto& function : module.functions) {
    lower_phi_nodes(&function);
    function.return_type = legalize_type(target, function.return_type);
    for (auto& param : function.params) {
      param.type = legalize_type(target, param.type);
    }
    for (auto& slot : function.local_slots) {
      slot.type = legalize_type(target, slot.type);
    }
    for (auto& block : function.blocks) {
      for (auto& inst : block.insts) {
        std::visit(
            [&](auto& lowered) {
              using T = std::decay_t<decltype(lowered)>;
              if constexpr (std::is_same_v<T, bir::BinaryInst>) {
                lowered.result.type = legalize_type(target, lowered.result.type);
                lowered.operand_type = legalize_type(target, lowered.operand_type);
                legalize_value(target, lowered.lhs);
                legalize_value(target, lowered.rhs);
              } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
                lowered.result.type = legalize_type(target, lowered.result.type);
                lowered.compare_type = legalize_type(target, lowered.compare_type);
                legalize_value(target, lowered.lhs);
                legalize_value(target, lowered.rhs);
                legalize_value(target, lowered.true_value);
                legalize_value(target, lowered.false_value);
              } else if constexpr (std::is_same_v<T, bir::CastInst>) {
                lowered.result.type = legalize_type(target, lowered.result.type);
                legalize_value(target, lowered.operand);
              } else if constexpr (std::is_same_v<T, bir::PhiInst>) {
                lowered.result.type = legalize_type(target, lowered.result.type);
                for (auto& incoming : lowered.incomings) {
                  legalize_value(target, incoming.value);
                }
              } else if constexpr (std::is_same_v<T, bir::CallInst>) {
                lowered.return_type = legalize_type(target, lowered.return_type);
                if (lowered.result.has_value()) {
                  legalize_value(target, *lowered.result);
                }
                if (lowered.callee_value.has_value()) {
                  legalize_value(target, *lowered.callee_value);
                }
                for (auto& arg : lowered.args) {
                  legalize_value(target, arg);
                }
                for (auto& arg_type : lowered.arg_types) {
                  arg_type = legalize_type(target, arg_type);
                }
                if (lowered.result_abi.has_value()) {
                  lowered.result_abi->type = legalize_type(target, lowered.result_abi->type);
                }
              } else if constexpr (std::is_same_v<T, bir::LoadLocalInst> ||
                                   std::is_same_v<T, bir::LoadGlobalInst>) {
                lowered.result.type = legalize_type(target, lowered.result.type);
                if (lowered.address.has_value()) {
                  legalize_value(target, lowered.address->base_value);
                }
              } else if constexpr (std::is_same_v<T, bir::StoreLocalInst> ||
                                   std::is_same_v<T, bir::StoreGlobalInst>) {
                legalize_value(target, lowered.value);
                if (lowered.address.has_value()) {
                  legalize_value(target, lowered.address->base_value);
                }
              }
            },
            inst);
      }

      if (block.terminator.value.has_value()) {
        legalize_value(target, *block.terminator.value);
      }
      legalize_value(target, block.terminator.condition);
    }
  }
}

}  // namespace

void run_legalize(PreparedLirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("legalize");
  module.notes.push_back(PrepareNote{
      .phase = "legalize",
      .message =
          "target-dependent legalization skeleton is wired; bool promotion and ABI/type legalization still need implementation",
  });
}

void run_legalize(PreparedBirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("legalize");
  legalize_module(module.target, module.module);

  module.notes.push_back(PrepareNote{
      .phase = "legalize",
      .message =
          "bootstrap BIR legalize promoted i1 values to i32 for x86/i686/aarch64/riscv64",
  });
}

}  // namespace c4c::backend::prepare
