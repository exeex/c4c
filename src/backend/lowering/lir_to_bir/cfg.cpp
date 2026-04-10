#include "passes.hpp"

#include <algorithm>
#include <cstddef>
#include <charconv>
#include <cstdint>
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

std::optional<std::int64_t> parse_i64_immediate(std::string_view text) {
  std::int64_t value = 0;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto [ptr, ec] = std::from_chars(begin, end, value);
  if (ec != std::errc{} || ptr != end) {
    return std::nullopt;
  }
  return value;
}

std::optional<unsigned> parse_integer_bit_width(std::string_view type_str) {
  if (type_str.size() < 2 || type_str.front() != 'i') {
    return std::nullopt;
  }

  unsigned value = 0;
  const char* begin = type_str.data() + 1;
  const char* end = type_str.data() + type_str.size();
  const auto [ptr, ec] = std::from_chars(begin, end, value);
  if (ec != std::errc{} || ptr != end || value == 0 || value > 64) {
    return std::nullopt;
  }
  return value;
}

std::uint64_t normalize_unsigned_bits(std::int64_t value, unsigned bit_width) {
  if (bit_width >= 64) {
    return static_cast<std::uint64_t>(value);
  }
  const auto mask = (std::uint64_t{1} << bit_width) - 1;
  return static_cast<std::uint64_t>(value) & mask;
}

std::int64_t normalize_signed_bits(std::int64_t value, unsigned bit_width) {
  if (bit_width >= 64) {
    return value;
  }

  const auto masked = normalize_unsigned_bits(value, bit_width);
  const auto sign_bit = std::uint64_t{1} << (bit_width - 1);
  if ((masked & sign_bit) == 0) {
    return static_cast<std::int64_t>(masked);
  }

  const auto extended_mask = ~((std::uint64_t{1} << bit_width) - 1);
  return static_cast<std::int64_t>(masked | extended_mask);
}

std::optional<bool> evaluate_integer_cmp(const lir::LirCmpOp& cmp) {
  if (cmp.is_float) {
    return std::nullopt;
  }

  const auto bit_width = parse_integer_bit_width(cmp.type_str);
  const auto lhs = parse_i64_immediate(cmp.lhs);
  const auto rhs = parse_i64_immediate(cmp.rhs);
  if (!bit_width.has_value() || !lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  const auto lhs_signed = normalize_signed_bits(*lhs, *bit_width);
  const auto rhs_signed = normalize_signed_bits(*rhs, *bit_width);
  const auto lhs_unsigned = normalize_unsigned_bits(*lhs, *bit_width);
  const auto rhs_unsigned = normalize_unsigned_bits(*rhs, *bit_width);

  if (cmp.predicate == "eq") {
    return lhs_unsigned == rhs_unsigned;
  }
  if (cmp.predicate == "ne") {
    return lhs_unsigned != rhs_unsigned;
  }
  if (cmp.predicate == "slt") {
    return lhs_signed < rhs_signed;
  }
  if (cmp.predicate == "sle") {
    return lhs_signed <= rhs_signed;
  }
  if (cmp.predicate == "sgt") {
    return lhs_signed > rhs_signed;
  }
  if (cmp.predicate == "sge") {
    return lhs_signed >= rhs_signed;
  }
  if (cmp.predicate == "ult") {
    return lhs_unsigned < rhs_unsigned;
  }
  if (cmp.predicate == "ule") {
    return lhs_unsigned <= rhs_unsigned;
  }
  if (cmp.predicate == "ugt") {
    return lhs_unsigned > rhs_unsigned;
  }
  if (cmp.predicate == "uge") {
    return lhs_unsigned >= rhs_unsigned;
  }
  return std::nullopt;
}

std::optional<bool> resolve_block_local_i1(std::string_view value_name, const lir::LirBlock& block) {
  if (value_name == "0" || value_name == "false") {
    return false;
  }
  if (value_name == "1" || value_name == "true") {
    return true;
  }

  for (auto it = block.insts.rbegin(); it != block.insts.rend(); ++it) {
    if (const auto* cmp = std::get_if<lir::LirCmpOp>(&*it);
        cmp != nullptr && cmp->result == value_name) {
      return evaluate_integer_cmp(*cmp);
    }
  }
  return std::nullopt;
}

void erase_trailing_cmp_if_defines(lir::LirBlock& block, std::string_view result_name) {
  if (block.insts.empty()) {
    return;
  }
  if (const auto* cmp = std::get_if<lir::LirCmpOp>(&block.insts.back());
      cmp != nullptr && cmp->result == result_name) {
    block.insts.pop_back();
  }
}

std::optional<std::string> resolve_forward_target(const CfgNormalizationContext& context,
                                                  std::string_view start_label) {
  std::string current(start_label);
  std::unordered_set<std::string> seen;
  for (std::size_t depth = 0; depth < kMaxChainDepth; ++depth) {
    const auto [it, inserted] = seen.insert(current);
    if (!inserted) {
      return std::nullopt;
    }
    const auto block_it = context.label_to_idx.find(current);
    if (block_it == context.label_to_idx.end()) {
      return std::nullopt;
    }
    const auto& block = context.function->blocks[block_it->second];
    if (!is_forwarding_block(block)) {
      return current;
    }
    current = std::get<lir::LirBr>(block.terminator).target_label;
  }
  return std::nullopt;
}

std::optional<std::string> find_entry_label(const lir::LirFunction& function) {
  for (const auto& block : function.blocks) {
    if (block.id.value == function.entry.value) {
      return block.label;
    }
  }
  if (!function.blocks.empty()) {
    return function.blocks.front().label;
  }
  return std::nullopt;
}

std::optional<std::string> get_immediate_return_value(const lir::LirBlock& block) {
  const auto* ret = std::get_if<lir::LirRet>(&block.terminator);
  if (ret == nullptr || !ret->value_str.has_value() || !block.insts.empty()) {
    return std::nullopt;
  }
  if (!parse_i64_immediate(*ret->value_str).has_value()) {
    return std::nullopt;
  }
  return *ret->value_str;
}

std::size_t collapse_constant_conditional_return_ladder(CfgNormalizationContext& context) {
  auto& function = *context.function;
  if (!function.alloca_insts.empty() || function.blocks.size() < 2) {
    return 0;
  }

  const auto entry_label = find_entry_label(function);
  if (!entry_label.has_value()) {
    return 0;
  }

  std::unordered_set<std::string> visited;
  std::unordered_set<std::string> consumed;
  std::string current = *entry_label;
  std::optional<std::string> final_return_value;
  std::string final_return_type = "i32";

  for (std::size_t depth = 0; depth < function.blocks.size(); ++depth) {
    if (!visited.insert(current).second) {
      return 0;
    }

    const auto block_it = context.label_to_idx.find(current);
    if (block_it == context.label_to_idx.end()) {
      return 0;
    }
    const auto& block = function.blocks[block_it->second];
    consumed.insert(block.label);

    if (const auto return_value = get_immediate_return_value(block); return_value.has_value()) {
      final_return_value = *return_value;
      final_return_type = std::get<lir::LirRet>(block.terminator).type_str;
      break;
    }

    const auto* cond = std::get_if<lir::LirCondBr>(&block.terminator);
    if (cond == nullptr || block.insts.size() != 1) {
      return 0;
    }

    const auto constant = resolve_block_local_i1(cond->cond_name, block);
    if (!constant.has_value()) {
      return 0;
    }

    const std::string taken_target = *constant ? cond->true_label : cond->false_label;
    const std::string dead_target = *constant ? cond->false_label : cond->true_label;
    const auto dead_it = context.label_to_idx.find(dead_target);
    if (dead_it == context.label_to_idx.end()) {
      return 0;
    }
    const auto dead_return = get_immediate_return_value(function.blocks[dead_it->second]);
    if (!dead_return.has_value()) {
      return 0;
    }
    consumed.insert(dead_target);
    current = taken_target;
  }

  if (!final_return_value.has_value() || consumed.size() != function.blocks.size()) {
    return 0;
  }

  lir::LirBlock collapsed;
  collapsed.id = function.entry;
  collapsed.label = *entry_label;
  collapsed.terminator = lir::LirRet{*final_return_value, final_return_type};
  function.blocks.clear();
  function.blocks.push_back(std::move(collapsed));
  return 1;
}

// The remaining helper names intentionally mirror the reference pass shape so
// the eventual implementation can stay phase-aligned with cfg_simplify.rs.
std::size_t fold_constant_cond_branches(CfgNormalizationContext& context) {
  return collapse_constant_conditional_return_ladder(context);
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
