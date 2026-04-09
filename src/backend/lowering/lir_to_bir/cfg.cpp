#include "passes.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace c4c::backend {
namespace {

namespace lir = c4c::codegen::lir;

using LabelToIndex = std::unordered_map<std::string, std::size_t>;
using PredCountMap = std::unordered_map<std::string, std::size_t>;
using SinglePredMap = std::unordered_map<std::string, std::string>;

constexpr std::size_t kMaxChainDepth = 32;
constexpr std::size_t kMaxPredChainDepth = 8;
constexpr std::size_t kMaxGlobalResolveDepth = 16;

struct PredInfo {
  PredCountMap pred_count;
  SinglePredMap single_pred;
};

struct CfgNormalizationContext {
  lir::LirFunction* function = nullptr;
  const BirLoweringOptions* options = nullptr;
  LabelToIndex label_to_idx;
  PredInfo pred_info;
};

struct JumpThread {
  std::size_t block_index = 0;
  std::string old_target;
  std::string new_target;
  std::string phi_lookup_label;
};

LabelToIndex build_label_to_idx(const lir::LirFunction& function) {
  LabelToIndex result;
  for (std::size_t i = 0; i < function.blocks.size(); ++i) {
    result.emplace(function.blocks[i].label, i);
  }
  return result;
}

template <typename Fn>
void for_each_terminator_target(const lir::LirTerminator& terminator, Fn&& fn) {
  if (const auto* br = std::get_if<lir::LirBr>(&terminator)) {
    fn(br->target_label);
    return;
  }
  if (const auto* cond = std::get_if<lir::LirCondBr>(&terminator)) {
    fn(cond->true_label);
    fn(cond->false_label);
    return;
  }
  if (const auto* sw = std::get_if<lir::LirSwitch>(&terminator)) {
    fn(sw->default_label);
    for (const auto& [value, label] : sw->cases) {
      (void)value;
      fn(label);
    }
    return;
  }

  // LIR indirect branches are preserved as a follow-up lowering concern.
  // The Rust reference handles computed-goto style edges; here we keep the
  // scaffold focused on the label-based CFG normalization path.
}

PredInfo build_pred_info(const lir::LirFunction& function) {
  PredInfo info;

  for (const auto& block : function.blocks) {
    const auto src_label = block.label;
    auto add_edge = [&](std::string_view target) {
      auto& count = info.pred_count[std::string(target)];
      ++count;
      if (count == 1) {
        info.single_pred[std::string(target)] = src_label;
      } else {
        info.single_pred.erase(std::string(target));
      }
    };

    for_each_terminator_target(block.terminator, add_edge);
  }

  return info;
}

void remove_phi_entries_from(lir::LirBlock& block, std::string_view source_label) {
  for (auto& inst : block.insts) {
    if (auto* phi = std::get_if<lir::LirPhiOp>(&inst)) {
      phi->incoming.erase(
          std::remove_if(
              phi->incoming.begin(),
              phi->incoming.end(),
              [&](const auto& incoming) { return incoming.second == source_label; }),
          phi->incoming.end());
    }
  }
}

void remove_phi_entries_from_set(lir::LirBlock& block,
                                 const std::unordered_set<std::string>& dead_labels) {
  for (auto& inst : block.insts) {
    if (auto* phi = std::get_if<lir::LirPhiOp>(&inst)) {
      phi->incoming.erase(
          std::remove_if(
              phi->incoming.begin(),
              phi->incoming.end(),
              [&](const auto& incoming) {
                return dead_labels.find(incoming.second) != dead_labels.end();
              }),
          phi->incoming.end());
    }
  }
}

void remap_terminator_targets(lir::LirTerminator& terminator,
                              const std::unordered_map<std::string, std::string>& remap) {
  if (auto* br = std::get_if<lir::LirBr>(&terminator)) {
    if (const auto it = remap.find(br->target_label); it != remap.end()) {
      br->target_label = it->second;
    }
    return;
  }

  if (auto* cond = std::get_if<lir::LirCondBr>(&terminator)) {
    if (const auto it = remap.find(cond->true_label); it != remap.end()) {
      cond->true_label = it->second;
    }
    if (const auto it = remap.find(cond->false_label); it != remap.end()) {
      cond->false_label = it->second;
    }
    return;
  }

  if (auto* sw = std::get_if<lir::LirSwitch>(&terminator)) {
    if (const auto it = remap.find(sw->default_label); it != remap.end()) {
      sw->default_label = it->second;
    }
    for (auto& [value, label] : sw->cases) {
      (void)value;
      if (const auto it = remap.find(label); it != remap.end()) {
        label = it->second;
      }
    }
  }
}

bool is_forwarding_block(const lir::LirBlock& block) {
  return block.insts.empty() && std::holds_alternative<lir::LirBr>(block.terminator);
}

// The remaining helper names intentionally mirror the reference pass shape so
// the eventual implementation can stay phase-aligned with cfg_simplify.rs.
std::size_t fold_constant_cond_branches(CfgNormalizationContext& context) {
  (void)context;
  // TODO: Resolve branch conditions through the local LIR value table, then
  // rewrite to unconditional branches and clean up dead incoming phi edges.
  return 0;
}

std::size_t fold_constant_switches(CfgNormalizationContext& context) {
  (void)context;
  // TODO: Lower constant switches into direct branches once selector values are
  // resolved. The reference pass folds matching and default cases separately.
  return 0;
}

std::size_t thread_jump_chains(CfgNormalizationContext& context) {
  (void)context;
  // TODO: Thread empty forwarding blocks out of the CFG, with phi-aware guards
  // that avoid collapsing distinct incoming values into the same edge.
  return 0;
}

std::size_t remove_dead_blocks(CfgNormalizationContext& context) {
  (void)context;
  // TODO: Compute reachability from the entry block, keep address-taken / asm
  // target blocks alive, and drop the rest.
  return 0;
}

std::size_t simplify_trivial_phis(CfgNormalizationContext& context) {
  (void)context;
  // TODO: Replace single-incoming and all-same incoming phi nodes with their
  // incoming value before BIR lowering consumes the simplified graph.
  return 0;
}

std::size_t merge_single_pred_blocks(CfgNormalizationContext& context) {
  (void)context;
  // TODO: Fuse linear chains of blocks once they are proven safe to absorb,
  // preserving block identity only where labels remain externally observable.
  return 0;
}

}  // namespace

// -----------------------------------------------------------------------------
// CFG normalization entry point
// -----------------------------------------------------------------------------
//
// This file is intentionally backend-owned and phase-structured. The goal is to
// keep the lowering pipeline's CFG normalization logic in one place, with the
// same conceptual stages as the reference pass:
//   1. fold constant conditional branches
//   2. fold constant switches
//   3. thread jump chains
//   4. remove dead blocks
//   5. simplify trivial phis
//   6. merge single-predecessor blocks
//
// The current file is a scaffold, not a testcase matcher. It is set up so the
// eventual implementation can fill in the phase bodies without changing the
// surrounding lowering structure.
std::size_t normalize_cfg(lir::LirFunction& function, const BirLoweringOptions& options) {
  if (!options.normalize_cfg || function.blocks.size() <= 1) {
    return 0;
  }

  CfgNormalizationContext context;
  context.function = &function;
  context.options = &options;

  std::size_t total_changes = 0;
  for (;;) {
    context.label_to_idx = build_label_to_idx(function);
    context.pred_info = build_pred_info(function);

    std::size_t changed = 0;
    changed += fold_constant_cond_branches(context);
    changed += fold_constant_switches(context);
    changed += thread_jump_chains(context);
    changed += remove_dead_blocks(context);
    changed += simplify_trivial_phis(context);
    changed += merge_single_pred_blocks(context);

    if (changed == 0) {
      break;
    }
    total_changes += changed;
  }

  return total_changes;
}

std::size_t normalize_cfg_in_place(lir::LirModule& module,
                                   const BirLoweringOptions& options,
                                   std::vector<BirLoweringNote>* notes) {
  if (!options.normalize_cfg) {
    return 0;
  }

  std::size_t total_changes = 0;
  for (auto& function : module.functions) {
    if (function.is_declaration || function.blocks.empty()) {
      continue;
    }
    total_changes += normalize_cfg(function, options);
  }

  if (notes != nullptr) {
    notes->push_back(BirLoweringNote{
        .phase = "cfg-normalization",
        .message = "ran split CFG normalization scaffold from lir_to_bir/cfg.cpp",
    });
  }

  return total_changes;
}

}  // namespace c4c::backend
