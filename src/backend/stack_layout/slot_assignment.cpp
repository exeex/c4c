#include "slot_assignment.hpp"

#include "alloca_coalescing.hpp"
#include "regalloc_helpers.hpp"
#include "../lowering/lir_to_bir.hpp"
#include "../../codegen/lir/call_args_ops.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace c4c::backend::stack_layout {

namespace {

using c4c::codegen::lir::LirAllocaOp;
using c4c::codegen::lir::LirFunction;
using c4c::codegen::lir::LirInst;
using c4c::codegen::lir::LirModule;
using c4c::codegen::lir::LirStoreOp;
using c4c::codegen::lir::LirTerminator;

bool is_param_alloca_name(std::string_view value_name) {
  return value_name.rfind("%lv.param.", 0) == 0;
}

bool is_param_name(std::string_view value_name) {
  return value_name.rfind("%p.", 0) == 0;
}

bool is_zero_initializer_store(const LirStoreOp* store) {
  return store != nullptr && store->val == "zeroinitializer";
}

bool is_zero_initializer_store(const EntryAllocaInput& alloca) {
  return alloca.paired_store_value.has_value() && *alloca.paired_store_value == "zeroinitializer";
}

bool is_zero_initializer_store(const EntryAllocaPlanInput& alloca) {
  return alloca.paired_store.has_store && alloca.paired_store.is_zero_initializer;
}

bool is_scalar_alloca_type(std::string_view type_str) {
  return !type_str.empty() && type_str.front() != '[' && type_str.front() != '{' &&
         type_str.front() != '%';
}

bool is_callee_saved_reg(PhysReg reg, const std::vector<PhysReg>& callee_saved_regs) {
  return std::find(callee_saved_regs.begin(), callee_saved_regs.end(), reg) !=
         callee_saved_regs.end();
}

EntryAllocaRewriteInput make_entry_alloca_rewrite_input_from_classification(
    std::vector<EntryAllocaInput> entry_allocas,
    const PreparedEntryAllocaStackLayoutClassificationInput& classification) {
  EntryAllocaRewriteInput input;
  input.entry_allocas = std::move(entry_allocas);
  input.escaped_entry_allocas = classification.escaped_entry_allocas;
  input.entry_alloca_use_blocks = classification.entry_alloca_use_blocks;
  input.entry_alloca_first_accesses = classification.entry_alloca_first_accesses;
  return input;
}

EntryAllocaPlanInput lower_entry_alloca_input_to_plan_input(const EntryAllocaInput& alloca) {
  EntryAllocaPlanInput lowered;
  lowered.alloca_name = alloca.alloca_name;
  lowered.type_str = alloca.type_str;
  lowered.align = alloca.align;
  lowered.paired_store.has_store = alloca.paired_store_value.has_value();
  lowered.paired_store.is_zero_initializer = is_zero_initializer_store(alloca);
  if (alloca.paired_store_value.has_value() && is_param_name(*alloca.paired_store_value)) {
    lowered.paired_store.param_name = *alloca.paired_store_value;
  }
  return lowered;
}

EntryAllocaPlanningInput lower_entry_alloca_rewrite_input_to_planning_input(
    const EntryAllocaRewriteInput& input) {
  EntryAllocaPlanningInput lowered;
  lowered.entry_allocas.reserve(input.entry_allocas.size());
  for (const auto& alloca : input.entry_allocas) {
    lowered.entry_allocas.push_back(lower_entry_alloca_input_to_plan_input(alloca));
  }
  lowered.escaped_entry_allocas = input.escaped_entry_allocas;
  lowered.entry_alloca_use_blocks = input.entry_alloca_use_blocks;
  return lowered;
}

std::optional<std::vector<std::string>> collect_prepared_escaped_entry_allocas(
    const StackLayoutInput& input) {
  std::unordered_map<std::string, std::string> pointer_roots;
  pointer_roots.reserve(input.entry_allocas.size());
  for (const auto& alloca : input.entry_allocas) {
    if (is_param_alloca_name(alloca.alloca_name)) {
      continue;
    }
    pointer_roots.emplace(alloca.alloca_name, alloca.alloca_name);
  }

  std::vector<std::string> escaped_entry_allocas;
  auto mark_escaped = [&](std::string_view value_name) {
    const auto it = pointer_roots.find(std::string(value_name));
    if (it == pointer_roots.end()) {
      return;
    }
    c4c::codegen::lir::append_unique_lir_value_name(escaped_entry_allocas, it->second);
  };

  for (const auto& block : input.blocks) {
    for (const auto& point : block.insts) {
      if (point.derived_pointer_root.has_value()) {
        const auto root_it = pointer_roots.find(point.derived_pointer_root->second);
        if (root_it != pointer_roots.end()) {
          pointer_roots.emplace(point.derived_pointer_root->first, root_it->second);
        }
      }

      for (const auto& escaped_name : point.escaped_names) {
        mark_escaped(escaped_name);
      }
    }

    for (const auto& value_name : block.terminator_used_names) {
      mark_escaped(value_name);
    }
  }

  return escaped_entry_allocas;
}

std::optional<std::vector<EntryAllocaUseBlocks>> collect_prepared_entry_alloca_use_blocks(
    const StackLayoutInput& input) {
  std::unordered_map<std::string, std::string> pointer_roots;
  pointer_roots.reserve(input.entry_allocas.size());
  for (const auto& alloca : input.entry_allocas) {
    if (is_param_alloca_name(alloca.alloca_name)) {
      continue;
    }
    pointer_roots.emplace(alloca.alloca_name, alloca.alloca_name);
  }

  std::unordered_map<std::string, std::vector<std::size_t>> use_blocks_by_alloca;
  use_blocks_by_alloca.reserve(pointer_roots.size());
  auto record_use = [&](std::string_view value_name, std::size_t block_index) {
    const auto root_it = pointer_roots.find(std::string(value_name));
    if (root_it == pointer_roots.end()) {
      return;
    }
    auto& blocks = use_blocks_by_alloca[root_it->second];
    if (blocks.empty() || blocks.back() != block_index) {
      blocks.push_back(block_index);
    }
  };

  for (std::size_t block_index = 0; block_index < input.blocks.size(); ++block_index) {
    const auto& block = input.blocks[block_index];
    for (const auto& point : block.insts) {
      if (point.derived_pointer_root.has_value()) {
        const auto root_it = pointer_roots.find(point.derived_pointer_root->second);
        if (root_it != pointer_roots.end()) {
          pointer_roots.emplace(point.derived_pointer_root->first, root_it->second);
          record_use(point.derived_pointer_root->first, block_index);
        }
      }

      for (const auto& access : point.pointer_accesses) {
        record_use(access.value_name, block_index);
      }
    }
  }

  std::vector<EntryAllocaUseBlocks> entry_alloca_use_blocks;
  entry_alloca_use_blocks.reserve(use_blocks_by_alloca.size());
  for (const auto& [alloca_name, block_indices] : use_blocks_by_alloca) {
    entry_alloca_use_blocks.push_back(EntryAllocaUseBlocks{alloca_name, block_indices});
  }
  std::sort(entry_alloca_use_blocks.begin(), entry_alloca_use_blocks.end(),
            [](const EntryAllocaUseBlocks& lhs, const EntryAllocaUseBlocks& rhs) {
              return lhs.alloca_name < rhs.alloca_name;
            });
  return entry_alloca_use_blocks;
}

std::optional<std::vector<EntryAllocaFirstAccess>> collect_prepared_entry_alloca_first_accesses(
    const StackLayoutInput& input) {
  std::unordered_map<std::string, std::string> pointer_roots;
  pointer_roots.reserve(input.entry_allocas.size());
  for (const auto& alloca : input.entry_allocas) {
    if (is_param_alloca_name(alloca.alloca_name)) {
      continue;
    }
    pointer_roots.emplace(alloca.alloca_name, alloca.alloca_name);
  }

  std::unordered_map<std::string, PointerAccessKind> first_access_kind;
  first_access_kind.reserve(pointer_roots.size());
  auto record_first_access = [&](std::string_view value_name, PointerAccessKind kind) {
    const auto root_it = pointer_roots.find(std::string(value_name));
    if (root_it == pointer_roots.end()) {
      return;
    }
    first_access_kind.try_emplace(root_it->second, kind);
  };

  for (const auto& block : input.blocks) {
    for (const auto& point : block.insts) {
      if (point.derived_pointer_root.has_value()) {
        const auto root_it = pointer_roots.find(point.derived_pointer_root->second);
        if (root_it != pointer_roots.end()) {
          pointer_roots.emplace(point.derived_pointer_root->first, root_it->second);
        }
      }

      for (const auto& access : point.pointer_accesses) {
        record_first_access(access.value_name, access.kind);
      }
      for (const auto& value_name : point.used_names) {
        record_first_access(value_name, PointerAccessKind::Read);
      }
    }

    for (const auto& value_name : block.terminator_used_names) {
      record_first_access(value_name, PointerAccessKind::Read);
    }
  }

  std::vector<EntryAllocaFirstAccess> entry_alloca_first_accesses;
  entry_alloca_first_accesses.reserve(first_access_kind.size());
  for (const auto& [alloca_name, kind] : first_access_kind) {
    entry_alloca_first_accesses.push_back(EntryAllocaFirstAccess{alloca_name, kind});
  }
  std::sort(entry_alloca_first_accesses.begin(), entry_alloca_first_accesses.end(),
            [](const EntryAllocaFirstAccess& lhs, const EntryAllocaFirstAccess& rhs) {
              return lhs.alloca_name < rhs.alloca_name;
            });
  return entry_alloca_first_accesses;
}

PreparedEntryAllocaStackLayoutClassificationInput lower_prepared_stack_layout_classification_input(
    StackLayoutInput input) {
  PreparedEntryAllocaStackLayoutClassificationInput classification;
  classification.escaped_entry_allocas = collect_prepared_escaped_entry_allocas(input);
  classification.entry_alloca_use_blocks = collect_prepared_entry_alloca_use_blocks(input);
  classification.entry_alloca_first_accesses =
      collect_prepared_entry_alloca_first_accesses(input);
  return classification;
}

void apply_prepared_stack_layout_metadata(
    StackLayoutInput& input,
    const PreparedEntryAllocaStackLayoutMetadata& metadata) {
  input.signature_params = metadata.signature_params;
  input.return_type = metadata.return_type;
  input.is_variadic = metadata.is_variadic;
  input.call_results = metadata.call_results;
}

StackLayoutInput lower_prepared_stack_layout_input(
    const PreparedEntryAllocaRewriteMetadata& rewrite_metadata,
    const PreparedEntryAllocaStackLayoutClassificationInput& classification,
    const PreparedEntryAllocaStackLayoutMetadata& metadata,
    const LivenessInput* liveness_input) {
  StackLayoutInput input;
  input.entry_allocas = rewrite_metadata.entry_allocas;
  input.escaped_entry_allocas = classification.escaped_entry_allocas;
  input.entry_alloca_use_blocks = classification.entry_alloca_use_blocks;
  input.entry_alloca_first_accesses = classification.entry_alloca_first_accesses;
  if (liveness_input != nullptr) {
    input.blocks.reserve(liveness_input->blocks.size());
  }
  for (std::size_t block_index = 0; liveness_input != nullptr && block_index < liveness_input->blocks.size();
       ++block_index) {
    const auto& liveness_block = liveness_input->blocks[block_index];
    StackLayoutBlockInput lowered_block;
    lowered_block.label = liveness_block.label;
    lowered_block.insts.reserve(liveness_block.insts.size());
    for (std::size_t inst_index = 0; inst_index < liveness_block.insts.size(); ++inst_index) {
      (void)inst_index;
      StackLayoutPoint lowered_point;
      lowered_block.insts.push_back(std::move(lowered_point));
    }
    input.blocks.push_back(std::move(lowered_block));
  }
  apply_prepared_stack_layout_metadata(input, metadata);

  if (liveness_input == nullptr) {
    return input;
  }

  for (std::size_t block_index = 0; block_index < input.blocks.size(); ++block_index) {
    auto& lowered_block = input.blocks[block_index];
    const auto& liveness_block = liveness_input->blocks[block_index];
    lowered_block.terminator_used_names = liveness_block.terminator_used_names;

    for (std::size_t inst_index = 0; inst_index < lowered_block.insts.size(); ++inst_index) {
      lowered_block.insts[inst_index].used_names = liveness_block.insts[inst_index].used_names;
    }
  }

  input.phi_incoming_uses.reserve(liveness_input->phi_incoming_uses.size());
  for (const auto& phi_use : liveness_input->phi_incoming_uses) {
    input.phi_incoming_uses.push_back(
        PhiIncomingUse{phi_use.predecessor_label, phi_use.value_name});
  }

  return input;
}

EntryAllocaRewriteInput lower_prepared_entry_alloca_rewrite_input(
    const PreparedEntryAllocaRewriteMetadata& rewrite_metadata,
    const PreparedEntryAllocaStackLayoutClassificationInput& classification) {
  return make_entry_alloca_rewrite_input_from_classification(
      rewrite_metadata.entry_allocas, classification);
}

EntryAllocaRewriteInput lower_stack_layout_input_to_entry_alloca_rewrite_input(
    StackLayoutInput input) {
  auto classification = lower_prepared_stack_layout_classification_input(input);
  return make_entry_alloca_rewrite_input_from_classification(
      std::move(input.entry_allocas), classification);
}

StackLayoutAnalysis analyze_entry_alloca_rewrite_input(
    const EntryAllocaRewriteInput& input,
    const RegAllocIntegrationResult& regalloc,
    const std::vector<PhysReg>& callee_saved_regs) {
  StackLayoutAnalysis analysis;

  if (input.entry_alloca_use_blocks.has_value()) {
    for (const auto& use_blocks : *input.entry_alloca_use_blocks) {
      if (use_blocks.block_indices.empty()) {
        continue;
      }
      analysis.value_use_blocks.emplace(use_blocks.alloca_name, use_blocks.block_indices);
      analysis.used_values.insert(use_blocks.alloca_name);
    }
  }

  if (input.escaped_entry_allocas.has_value()) {
    for (const auto& alloca_name : *input.escaped_entry_allocas) {
      analysis.used_values.insert(alloca_name);
    }
  }

  if (input.entry_alloca_first_accesses.has_value()) {
    for (const auto& first_access : *input.entry_alloca_first_accesses) {
      analysis.used_values.insert(first_access.alloca_name);
      if (first_access.kind == PointerAccessKind::Store) {
        analysis.entry_allocas_overwritten_before_read.insert(first_access.alloca_name);
      }
    }
  }

  for (const auto& alloca : input.entry_allocas) {
    if (!is_param_alloca_name(alloca.alloca_name) || !alloca.paired_store_value.has_value() ||
        !is_param_name(*alloca.paired_store_value)) {
      continue;
    }
    if (analysis.used_values.find(alloca.alloca_name) != analysis.used_values.end()) {
      continue;
    }
    const auto* assigned_reg = find_assigned_reg(regalloc, *alloca.paired_store_value);
    if (assigned_reg != nullptr && is_callee_saved_reg(*assigned_reg, callee_saved_regs)) {
      analysis.dead_param_allocas.insert(alloca.alloca_name);
    }
  }

  return analysis;
}

std::vector<LirInst> build_pruned_entry_alloca_insts(
    const std::vector<EntryAllocaInput>& entry_allocas,
    const std::vector<EntryAllocaSlotPlan>& plans) {
  std::unordered_map<std::string, EntryAllocaSlotPlan> plans_by_alloca;
  plans_by_alloca.reserve(plans.size());
  for (const auto& plan : plans) {
    plans_by_alloca.emplace(plan.alloca_name, plan);
  }

  std::vector<LirInst> pruned;
  pruned.reserve(entry_allocas.size() * 2);

  for (const auto& alloca : entry_allocas) {
    const auto plan_it = plans_by_alloca.find(alloca.alloca_name);
    if (plan_it == plans_by_alloca.end()) {
      pruned.push_back(LirAllocaOp{alloca.alloca_name, alloca.type_str, "", alloca.align});
      if (alloca.paired_store_value.has_value()) {
        pruned.push_back(LirStoreOp{alloca.type_str, *alloca.paired_store_value, alloca.alloca_name});
      }
      continue;
    }

    if (plan_it->second.needs_stack_slot) {
      pruned.push_back(LirAllocaOp{alloca.alloca_name, alloca.type_str, "", alloca.align});
      if (alloca.paired_store_value.has_value() && !plan_it->second.remove_following_entry_store) {
        pruned.push_back(LirStoreOp{alloca.type_str, *alloca.paired_store_value, alloca.alloca_name});
      }
      continue;
    }
  }

  return pruned;
}

struct AssignedEntrySlot {
  std::size_t slot = 0;
  std::string type_str;
  int align = 0;
  std::unordered_set<std::size_t> occupied_blocks;
};

template <typename Map>
void rewrite_value_name(std::string& value_name, const Map& canonical_names) {
  const auto it = canonical_names.find(value_name);
  if (it != canonical_names.end()) {
    value_name = it->second;
  }
}

template <typename Map>
void rewrite_inst_alloca_refs(c4c::codegen::lir::LirInst& inst, const Map& canonical_names) {
  std::visit(
      [&](auto& op) {
        using Op = std::decay_t<decltype(op)>;
        if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirMemcpyOp>) {
          rewrite_value_name(op.dst, canonical_names);
          rewrite_value_name(op.src, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirVaStartOp>) {
          rewrite_value_name(op.ap_ptr, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirVaEndOp>) {
          rewrite_value_name(op.ap_ptr, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirVaCopyOp>) {
          rewrite_value_name(op.dst_ptr, canonical_names);
          rewrite_value_name(op.src_ptr, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirStackRestoreOp>) {
          rewrite_value_name(op.saved_ptr, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirAbsOp>) {
          rewrite_value_name(op.arg, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirIndirectBrOp>) {
          rewrite_value_name(op.addr, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirExtractValueOp>) {
          rewrite_value_name(op.agg, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirInsertValueOp>) {
          rewrite_value_name(op.agg, canonical_names);
          rewrite_value_name(op.elem, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirLoadOp>) {
          rewrite_value_name(op.ptr, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirStoreOp>) {
          rewrite_value_name(op.val, canonical_names);
          rewrite_value_name(op.ptr, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirCastOp>) {
          rewrite_value_name(op.operand, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirGepOp>) {
          rewrite_value_name(op.ptr, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirBinOp>) {
          rewrite_value_name(op.lhs, canonical_names);
          rewrite_value_name(op.rhs, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirCmpOp>) {
          rewrite_value_name(op.lhs, canonical_names);
          rewrite_value_name(op.rhs, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirPhiOp>) {
          for (auto& [value, _] : op.incoming) {
            rewrite_value_name(value, canonical_names);
          }
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirSelectOp>) {
          rewrite_value_name(op.cond, canonical_names);
          rewrite_value_name(op.true_val, canonical_names);
          rewrite_value_name(op.false_val, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirInsertElementOp>) {
          rewrite_value_name(op.vec, canonical_names);
          rewrite_value_name(op.elem, canonical_names);
          rewrite_value_name(op.index, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirExtractElementOp>) {
          rewrite_value_name(op.vec, canonical_names);
          rewrite_value_name(op.index, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirShuffleVectorOp>) {
          rewrite_value_name(op.vec1, canonical_names);
          rewrite_value_name(op.vec2, canonical_names);
          rewrite_value_name(op.mask, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirVaArgOp>) {
          rewrite_value_name(op.ap_ptr, canonical_names);
        } else if constexpr (std::is_same_v<Op, c4c::codegen::lir::LirCallOp>) {
          c4c::codegen::lir::rewrite_lir_call_operands(
              op, [&](std::string_view operand) -> std::optional<std::string_view> {
                const auto it = canonical_names.find(std::string(operand));
                if (it == canonical_names.end()) {
                  return std::nullopt;
                }
                return std::string_view(it->second);
              });
        }
      },
      inst);
}

template <typename Map>
void rewrite_terminator_alloca_refs(LirTerminator& terminator, const Map& canonical_names) {
  std::visit(
      [&](auto& term) {
        using Term = std::decay_t<decltype(term)>;
        if constexpr (std::is_same_v<Term, c4c::codegen::lir::LirCondBr>) {
          rewrite_value_name(term.cond_name, canonical_names);
        } else if constexpr (std::is_same_v<Term, c4c::codegen::lir::LirRet>) {
          if (term.value_str.has_value()) {
            rewrite_value_name(*term.value_str, canonical_names);
          }
        } else if constexpr (std::is_same_v<Term, c4c::codegen::lir::LirSwitch>) {
          rewrite_value_name(term.selector_name, canonical_names);
        }
      },
      terminator);
}

}  // namespace

StackLayoutPlanBundle build_stack_layout_plan_bundle(
    const EntryAllocaPlanningInput& input,
    const StackLayoutAnalysis& analysis) {
  StackLayoutPlanBundle bundle;
  bundle.analysis = analysis;
  bundle.entry_alloca_plans = plan_entry_alloca_slots(input, bundle.analysis);
  bundle.param_alloca_plans = plan_param_alloca_slots(input, bundle.analysis);
  return bundle;
}

StackLayoutPlanBundle build_stack_layout_plan_bundle(
    const StackLayoutInput& input,
    const RegAllocIntegrationResult& regalloc,
    const std::vector<PhysReg>& callee_saved_regs) {
  const auto analysis = analyze_stack_layout(input, regalloc, callee_saved_regs);
  return build_stack_layout_plan_bundle(
      lower_entry_alloca_rewrite_input_to_planning_input(
          lower_stack_layout_input_to_entry_alloca_rewrite_input(input)),
      analysis);
}

StackLayoutPlanBundle build_stack_layout_plan_bundle(
    const LivenessInput& liveness_input,
    const StackLayoutInput& stack_layout_input,
    const RegAllocConfig& regalloc_config,
    const std::vector<PhysReg>& asm_clobbered,
    const std::vector<PhysReg>& callee_saved_regs) {
  const auto regalloc =
      run_regalloc_and_merge_clobbers(liveness_input, regalloc_config, asm_clobbered);
  return build_stack_layout_plan_bundle(stack_layout_input, regalloc, callee_saved_regs);
}

EntryAllocaRewritePatch prepare_entry_alloca_rewrite_patch(
    const LivenessInput& liveness_input,
    const StackLayoutInput& stack_layout_input,
    const RegAllocConfig& regalloc_config,
    const std::vector<PhysReg>& asm_clobbered,
    const std::vector<PhysReg>& callee_saved_regs) {
  return prepare_entry_alloca_rewrite_patch(
      liveness_input,
      lower_stack_layout_input_to_entry_alloca_rewrite_input(stack_layout_input),
      regalloc_config,
      asm_clobbered,
      callee_saved_regs);
}

EntryAllocaRewritePatch prepare_entry_alloca_rewrite_patch(
    const LivenessInput& liveness_input,
    const EntryAllocaRewriteInput& rewrite_input,
    const RegAllocConfig& regalloc_config,
    const std::vector<PhysReg>& asm_clobbered,
    const std::vector<PhysReg>& callee_saved_regs) {
  const auto regalloc =
      run_regalloc_and_merge_clobbers(liveness_input, regalloc_config, asm_clobbered);
  const auto analysis =
      analyze_entry_alloca_rewrite_input(rewrite_input, regalloc, callee_saved_regs);
  const auto entry_plans = plan_entry_alloca_slots(rewrite_input, analysis);
  return build_entry_alloca_rewrite_patch(rewrite_input, entry_plans);
}

LirModule rewrite_module_entry_allocas(
    const c4c::codegen::lir::LirModule& module,
    const RegAllocConfig& regalloc_config,
    const std::vector<PhysReg>& asm_clobbered,
    const std::vector<PhysReg>& callee_saved_regs) {
  auto rewritten = module;
  for (std::size_t function_index = 0; function_index < rewritten.functions.size(); ++function_index) {
    auto& function = rewritten.functions[function_index];
    const auto prepared_inputs =
        prepare_module_function_entry_alloca_inputs(module, function_index);
    const auto patch = prepare_entry_alloca_rewrite_patch(
        prepared_inputs.liveness_input,
        prepared_inputs.rewrite_input,
        regalloc_config,
        asm_clobbered,
        callee_saved_regs);
    apply_entry_alloca_rewrite_patch(function, patch);
  }
  return rewritten;
}

std::optional<LivenessInput> try_lower_module_function_to_bir_liveness_input(
    const c4c::codegen::lir::LirModule& module,
    std::size_t function_index) {
  if (function_index >= module.functions.size()) {
    return std::nullopt;
  }

  const auto& selected_function = module.functions[function_index];
  if (!selected_function.alloca_insts.empty()) {
    return std::nullopt;
  }

  c4c::codegen::lir::LirModule single_function_module;
  single_function_module.target_triple = module.target_triple;
  single_function_module.data_layout = module.data_layout;
  single_function_module.globals = module.globals;
  single_function_module.string_pool = module.string_pool;
  single_function_module.extern_decls = module.extern_decls;
  single_function_module.extern_decl_map = module.extern_decl_map;
  single_function_module.type_decls = module.type_decls;
  single_function_module.need_va_start = module.need_va_start;
  single_function_module.need_va_end = module.need_va_end;
  single_function_module.need_va_copy = module.need_va_copy;
  single_function_module.need_memcpy = module.need_memcpy;
  single_function_module.need_memset = module.need_memset;
  single_function_module.need_stacksave = module.need_stacksave;
  single_function_module.need_stackrestore = module.need_stackrestore;
  single_function_module.need_abs = module.need_abs;
  single_function_module.need_ptrmask = module.need_ptrmask;
  single_function_module.spec_entries = module.spec_entries;

  for (const auto& function : module.functions) {
    if (function.name == selected_function.name || function.is_declaration) {
      single_function_module.functions.push_back(function);
    }
  }

  const auto bir_module = c4c::backend::try_lower_to_bir(single_function_module);
  if (!bir_module.has_value()) {
    return std::nullopt;
  }

  for (const auto& function : bir_module->functions) {
    if (!function.is_declaration && function.name == selected_function.name) {
      return lower_backend_cfg_to_liveness_input(lower_bir_to_backend_cfg(function));
    }
  }

  return std::nullopt;
}

EntryAllocaRewriteInputs prepare_module_function_entry_alloca_inputs(
    const c4c::codegen::lir::LirModule& module,
    std::size_t function_index) {
  return lower_prepared_entry_alloca_function_inputs(
      prepare_module_function_entry_alloca_preparation(module, function_index));
}

PreparedEntryAllocaFunctionInputs prepare_module_function_entry_alloca_preparation(
    const c4c::codegen::lir::LirModule& module,
    std::size_t function_index) {
  PreparedEntryAllocaFunctionInputs inputs;
  if (function_index >= module.functions.size()) {
    return inputs;
  }

  const auto& function = module.functions[function_index];
  if (const auto bir_liveness =
          try_lower_module_function_to_bir_liveness_input(module, function_index);
      bir_liveness.has_value()) {
    auto stack_layout_input = lower_lir_to_stack_layout_input(function);
    inputs.rewrite_metadata.entry_allocas = stack_layout_input.entry_allocas;
    inputs.stack_layout_metadata.signature_params = stack_layout_input.signature_params;
    inputs.stack_layout_metadata.return_type = stack_layout_input.return_type;
    inputs.stack_layout_metadata.is_variadic = stack_layout_input.is_variadic;
    inputs.stack_layout_metadata.call_results = stack_layout_input.call_results;
    inputs.stack_layout_classification =
        lower_prepared_stack_layout_classification_input(std::move(stack_layout_input));
    inputs.liveness_input = std::move(*bir_liveness);
    inputs.liveness_source = EntryAllocaRewriteLivenessSource::PerFunctionBir;
    return inputs;
  }

  const auto backend_cfg = lower_lir_to_backend_cfg(function);
  inputs.stack_layout_metadata.signature_params.reserve(backend_cfg.signature_params.size());
  for (const auto& param : backend_cfg.signature_params) {
    inputs.stack_layout_metadata.signature_params.push_back(
        StackLayoutSignatureParam{param.type, param.operand, param.is_varargs});
    inputs.stack_layout_metadata.is_variadic =
        inputs.stack_layout_metadata.is_variadic || param.is_varargs;
  }
  inputs.stack_layout_metadata.return_type = backend_cfg.return_type;
  inputs.stack_layout_metadata.call_results.reserve(backend_cfg.call_results.size());
  for (const auto& call_result : backend_cfg.call_results) {
    inputs.stack_layout_metadata.call_results.push_back(
        StackLayoutCallResultInput{call_result.value_name, call_result.type_str});
  }
  auto stack_layout_input = lower_function_entry_alloca_stack_layout_input(function, backend_cfg);
  inputs.rewrite_metadata.entry_allocas = stack_layout_input.entry_allocas;
  inputs.stack_layout_classification =
      lower_prepared_stack_layout_classification_input(std::move(stack_layout_input));
  inputs.backend_cfg_liveness = lower_backend_cfg_to_liveness_function(backend_cfg);
  inputs.stack_layout_source =
      EntryAllocaRewriteStackLayoutSource::EntryAllocasAndBackendCfg;
  return inputs;
}

EntryAllocaRewriteInputs lower_prepared_entry_alloca_function_inputs(
    const PreparedEntryAllocaFunctionInputs& prepared_inputs) {
  EntryAllocaRewriteInputs inputs;
  inputs.liveness_source = prepared_inputs.liveness_source;
  inputs.stack_layout_source = prepared_inputs.stack_layout_source;

  if (prepared_inputs.liveness_input.has_value()) {
    inputs.liveness_input = *prepared_inputs.liveness_input;
  } else if (prepared_inputs.backend_cfg_liveness.has_value()) {
    inputs.liveness_input =
        lower_backend_cfg_to_liveness_input(*prepared_inputs.backend_cfg_liveness);
  }

  inputs.stack_layout_input = lower_prepared_stack_layout_input(
      prepared_inputs.rewrite_metadata,
      prepared_inputs.stack_layout_classification,
      prepared_inputs.stack_layout_metadata,
      &inputs.liveness_input);
  inputs.rewrite_input = lower_prepared_entry_alloca_rewrite_input(
      prepared_inputs.rewrite_metadata,
      prepared_inputs.stack_layout_classification);
  return inputs;
}

std::vector<EntryAllocaSlotPlan> plan_entry_alloca_slots(
    const StackLayoutInput& input,
    const StackLayoutAnalysis& analysis) {
  return plan_entry_alloca_slots(
      lower_stack_layout_input_to_entry_alloca_rewrite_input(input), analysis);
}

std::vector<EntryAllocaSlotPlan> plan_entry_alloca_slots(
    const EntryAllocaRewriteInput& input,
    const StackLayoutAnalysis& analysis) {
  return plan_entry_alloca_slots(
      lower_entry_alloca_rewrite_input_to_planning_input(input), analysis);
}

std::vector<EntryAllocaSlotPlan> plan_entry_alloca_slots(
    const EntryAllocaPlanningInput& input,
    const StackLayoutAnalysis& analysis) {
  std::vector<EntryAllocaSlotPlan> plans;
  StackLayoutInput coalescing_input;
  coalescing_input.entry_allocas.reserve(input.entry_allocas.size());
  for (const auto& alloca : input.entry_allocas) {
    coalescing_input.entry_allocas.push_back(
        EntryAllocaInput{alloca.alloca_name, alloca.type_str, alloca.align, std::nullopt});
  }
  coalescing_input.escaped_entry_allocas = input.escaped_entry_allocas;
  coalescing_input.entry_alloca_use_blocks = input.entry_alloca_use_blocks;
  const auto coalescable_allocas = compute_coalescable_allocas(coalescing_input, analysis);
  std::vector<AssignedEntrySlot> assigned_slots;
  std::size_t next_slot = 0;

  for (const auto& alloca : input.entry_allocas) {
    EntryAllocaSlotPlan plan;
    plan.alloca_name = alloca.alloca_name;
    plan.needs_stack_slot = true;

    const bool has_paired_store = alloca.paired_store.has_store;

    if (is_param_alloca_name(alloca.alloca_name)) {
      if (alloca.paired_store.param_name.has_value() &&
          analysis.is_dead_param_alloca(alloca.alloca_name)) {
        plan.needs_stack_slot = false;
        plan.remove_following_entry_store = true;
      }
    } else if (!analysis.uses_value(alloca.alloca_name)) {
      plan.needs_stack_slot = false;
      plan.remove_following_entry_store = has_paired_store;
    } else if (is_scalar_alloca_type(alloca.type_str) && is_zero_initializer_store(alloca) &&
               analysis.is_entry_alloca_overwritten_before_read(alloca.alloca_name)) {
      plan.remove_following_entry_store = true;
    }

    if (plan.needs_stack_slot && !is_param_alloca_name(alloca.alloca_name)) {
      plan.coalesced_block = coalescable_allocas.find_single_block(alloca.alloca_name);
    }
    if (plan.needs_stack_slot) {
      if (plan.coalesced_block.has_value()) {
        for (auto& assigned_slot : assigned_slots) {
          if (assigned_slot.occupied_blocks.empty() ||
              assigned_slot.type_str != alloca.type_str ||
              assigned_slot.align != alloca.align ||
              assigned_slot.occupied_blocks.find(*plan.coalesced_block) !=
                  assigned_slot.occupied_blocks.end()) {
            continue;
          }
          plan.assigned_slot = assigned_slot.slot;
          assigned_slot.occupied_blocks.insert(*plan.coalesced_block);
          break;
        }
      }
      if (!plan.assigned_slot.has_value()) {
        plan.assigned_slot = next_slot++;
        AssignedEntrySlot assigned_slot{*plan.assigned_slot, alloca.type_str, alloca.align, {}};
        if (plan.coalesced_block.has_value()) {
          assigned_slot.occupied_blocks.insert(*plan.coalesced_block);
        }
        assigned_slots.push_back(std::move(assigned_slot));
      }
    }

    plans.push_back(std::move(plan));
  }

  return plans;
}

EntryAllocaRewritePatch build_entry_alloca_rewrite_patch(
    const StackLayoutInput& input,
    const std::vector<EntryAllocaSlotPlan>& plans) {
  return build_entry_alloca_rewrite_patch(
      lower_stack_layout_input_to_entry_alloca_rewrite_input(input), plans);
}

EntryAllocaRewritePatch build_entry_alloca_rewrite_patch(
    const EntryAllocaRewriteInput& input,
    const std::vector<EntryAllocaSlotPlan>& plans) {
  std::unordered_map<std::size_t, std::string> canonical_by_slot;
  std::unordered_map<std::string, std::string> canonical_by_alloca;
  EntryAllocaRewritePatch patch;

  for (const auto& plan : plans) {
    if (!plan.needs_stack_slot || !plan.assigned_slot.has_value()) {
      continue;
    }
    auto [it, inserted] =
        canonical_by_slot.emplace(*plan.assigned_slot, plan.alloca_name);
    canonical_by_alloca.emplace(plan.alloca_name, it->second);
    (void)inserted;
  }

  const auto pruned_alloca_insts = build_pruned_entry_alloca_insts(input.entry_allocas, plans);
  if (canonical_by_alloca.empty()) {
    patch.alloca_insts = pruned_alloca_insts;
    return patch;
  }

  patch.alloca_insts.reserve(pruned_alloca_insts.size());
  std::unordered_set<std::string> kept_allocas;

  for (auto inst : pruned_alloca_insts) {
    rewrite_inst_alloca_refs(inst, canonical_by_alloca);

    const auto* alloca = std::get_if<LirAllocaOp>(&inst);
    if (alloca == nullptr) {
      patch.alloca_insts.push_back(std::move(inst));
      continue;
    }

    const auto canonical_it = canonical_by_alloca.find(alloca->result);
    if (canonical_it == canonical_by_alloca.end()) {
      patch.alloca_insts.push_back(std::move(inst));
      continue;
    }
    if (!kept_allocas.insert(canonical_it->second).second) {
      continue;
    }
    patch.alloca_insts.push_back(std::move(inst));
  }

  patch.canonical_allocas.reserve(canonical_by_alloca.size());
  for (const auto& [alloca_name, canonical_name] : canonical_by_alloca) {
    patch.canonical_allocas.emplace_back(alloca_name, canonical_name);
  }
  return patch;
}

void apply_entry_alloca_rewrite_patch(
    LirFunction& function,
    const EntryAllocaRewritePatch& patch) {
  function.alloca_insts = patch.alloca_insts;
  if (patch.canonical_allocas.empty()) {
    return;
  }

  std::unordered_map<std::string, std::string> canonical_by_alloca;
  canonical_by_alloca.reserve(patch.canonical_allocas.size());
  for (const auto& [alloca_name, canonical_name] : patch.canonical_allocas) {
    canonical_by_alloca.emplace(alloca_name, canonical_name);
  }

  for (auto& block : function.blocks) {
    for (auto& inst : block.insts) {
      rewrite_inst_alloca_refs(inst, canonical_by_alloca);
    }
    rewrite_terminator_alloca_refs(block.terminator, canonical_by_alloca);
  }
}

std::vector<ParamAllocaSlotPlan> plan_param_alloca_slots(
    const StackLayoutInput& input,
    const StackLayoutAnalysis& analysis) {
  return plan_param_alloca_slots(
      lower_stack_layout_input_to_entry_alloca_rewrite_input(input), analysis);
}

std::vector<ParamAllocaSlotPlan> plan_param_alloca_slots(
    const EntryAllocaRewriteInput& input,
    const StackLayoutAnalysis& analysis) {
  return plan_param_alloca_slots(
      lower_entry_alloca_rewrite_input_to_planning_input(input), analysis);
}

std::vector<ParamAllocaSlotPlan> plan_param_alloca_slots(
    const EntryAllocaPlanningInput& input,
    const StackLayoutAnalysis& analysis) {
  std::vector<ParamAllocaSlotPlan> plans;
  const auto entry_plans = plan_entry_alloca_slots(input, analysis);

  for (const auto& alloca : input.entry_allocas) {
    if (!is_param_alloca_name(alloca.alloca_name) || !alloca.paired_store.param_name.has_value()) {
      continue;
    }

    bool needs_stack_slot = true;
    for (const auto& entry_plan : entry_plans) {
      if (entry_plan.alloca_name == alloca.alloca_name) {
        needs_stack_slot = entry_plan.needs_stack_slot;
        break;
      }
    }

    plans.push_back(
        ParamAllocaSlotPlan{alloca.alloca_name, *alloca.paired_store.param_name, needs_stack_slot});
  }

  return plans;
}

}  // namespace c4c::backend::stack_layout
