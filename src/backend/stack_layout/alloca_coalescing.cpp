#include "alloca_coalescing.hpp"

#include <algorithm>
#include <string_view>

namespace c4c::backend::stack_layout {

namespace {

bool is_param_alloca_name(std::string_view value_name) {
  return value_name.rfind("%lv.param.", 0) == 0;
}

struct AllocaEscapeAnalysis {
  std::unordered_set<std::string> alloca_set;
  std::unordered_map<std::string, std::string> pointer_roots;
  std::unordered_set<std::string> escaped;
  std::unordered_map<std::string, std::vector<std::size_t>> use_blocks;

  explicit AllocaEscapeAnalysis(const StackLayoutInput& input) {
    for (const auto& alloca : input.entry_allocas) {
      if (is_param_alloca_name(alloca.alloca_name)) {
        continue;
      }
      alloca_set.insert(alloca.alloca_name);
      pointer_roots.emplace(alloca.alloca_name, alloca.alloca_name);
    }
  }

  [[nodiscard]] std::optional<std::string> resolve_root(
      std::string_view value_name) const {
    const auto it = pointer_roots.find(std::string(value_name));
    if (it == pointer_roots.end()) {
      return std::nullopt;
    }
    return it->second;
  }

  void record_use(std::string_view value_name, std::size_t block_index) {
    const auto root = resolve_root(value_name);
    if (!root.has_value()) {
      return;
    }
    auto& blocks = use_blocks[*root];
    if (blocks.empty() || blocks.back() != block_index) {
      blocks.push_back(block_index);
    }
  }

  void mark_escaped(std::string_view value_name, std::size_t block_index) {
    const auto root = resolve_root(value_name);
    if (!root.has_value()) {
      return;
    }
    escaped.insert(*root);
    auto& blocks = use_blocks[*root];
    if (blocks.empty() || blocks.back() != block_index) {
      blocks.push_back(block_index);
    }
  }
};

}  // namespace

std::optional<std::size_t> CoalescableAllocas::find_single_block(
    std::string_view alloca_name) const {
  const auto it = single_block_allocas.find(std::string(alloca_name));
  if (it == single_block_allocas.end()) {
    return std::nullopt;
  }
  return it->second;
}

bool CoalescableAllocas::is_dead_alloca(std::string_view alloca_name) const {
  return dead_allocas.find(std::string(alloca_name)) != dead_allocas.end();
}

CoalescableAllocas compute_coalescable_allocas(
    const StackLayoutInput& input,
    const StackLayoutAnalysis&) {
  AllocaEscapeAnalysis escape_analysis(input);
  if (escape_analysis.alloca_set.empty()) {
    return {};
  }

  for (std::size_t block_index = 0; block_index < input.blocks.size(); ++block_index) {
    const auto& block = input.blocks[block_index];
    for (const auto& point : block.insts) {
      if (point.derived_pointer_root.has_value()) {
        const auto root = escape_analysis.resolve_root(point.derived_pointer_root->second);
        if (root.has_value()) {
          escape_analysis.pointer_roots.emplace(point.derived_pointer_root->first, *root);
          escape_analysis.record_use(point.derived_pointer_root->first, block_index);
        }
      }

      for (const auto& access : point.pointer_accesses) {
        escape_analysis.record_use(access.value_name, block_index);
      }
      for (const auto& escaped_name : point.escaped_names) {
        escape_analysis.mark_escaped(escaped_name, block_index);
      }
    }

    for (const auto& value_name : block.terminator_used_names) {
      escape_analysis.mark_escaped(value_name, block_index);
    }
  }

  CoalescableAllocas result;
  for (const auto& alloca_name : escape_analysis.alloca_set) {
    if (escape_analysis.escaped.find(alloca_name) != escape_analysis.escaped.end()) {
      continue;
    }

    const auto use_it = escape_analysis.use_blocks.find(alloca_name);
    if (use_it == escape_analysis.use_blocks.end()) {
      result.dead_allocas.insert(alloca_name);
      continue;
    }

    auto unique_blocks = use_it->second;
    std::sort(unique_blocks.begin(), unique_blocks.end());
    unique_blocks.erase(std::unique(unique_blocks.begin(), unique_blocks.end()),
                        unique_blocks.end());
    if (unique_blocks.size() == 1) {
      result.single_block_allocas.emplace(alloca_name, unique_blocks.front());
    }
  }

  return result;
}

CoalescableAllocas compute_coalescable_allocas(
    const c4c::codegen::lir::LirFunction& function,
    const StackLayoutAnalysis& analysis) {
  return compute_coalescable_allocas(lower_lir_to_stack_layout_input(function), analysis);
}

}  // namespace c4c::backend::stack_layout
