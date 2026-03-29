#include "alloca_coalescing.hpp"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <utility>

namespace c4c::backend::stack_layout {

namespace {

using c4c::codegen::lir::LirAllocaOp;
using c4c::codegen::lir::LirCallOp;
using c4c::codegen::lir::LirCondBr;
using c4c::codegen::lir::LirFunction;
using c4c::codegen::lir::LirGepOp;
using c4c::codegen::lir::LirIndirectBrOp;
using c4c::codegen::lir::LirInst;
using c4c::codegen::lir::LirMemcpyOp;
using c4c::codegen::lir::LirRet;
using c4c::codegen::lir::LirStoreOp;
using c4c::codegen::lir::LirSwitch;
using c4c::codegen::lir::LirTerminator;

bool is_value_name_char(char ch) {
  return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '.' ||
         ch == '-';
}

bool is_value_name(std::string_view token) {
  return !token.empty() && token.front() == '%';
}

bool is_param_alloca_name(std::string_view value_name) {
  return value_name.rfind("%lv.param.", 0) == 0;
}

void append_unique(std::vector<std::string>& values, std::string value) {
  if (value.empty()) {
    return;
  }
  if (std::find(values.begin(), values.end(), value) == values.end()) {
    values.push_back(std::move(value));
  }
}

void collect_value_names_from_text(std::string_view text,
                                   std::vector<std::string>& values) {
  std::size_t pos = 0;
  while (pos < text.size()) {
    pos = text.find('%', pos);
    if (pos == std::string_view::npos) {
      return;
    }
    std::size_t end = pos + 1;
    while (end < text.size() && is_value_name_char(text[end])) {
      ++end;
    }
    if (end > pos + 1) {
      append_unique(values, std::string(text.substr(pos, end - pos)));
    }
    pos = end;
  }
}

struct AllocaEscapeAnalysis {
  std::unordered_set<std::string> alloca_set;
  std::unordered_map<std::string, std::string> gep_to_alloca;
  std::unordered_set<std::string> escaped;
  std::unordered_map<std::string, std::vector<std::size_t>> use_blocks;

  explicit AllocaEscapeAnalysis(const LirFunction& function) {
    for (const auto& inst : function.alloca_insts) {
      const auto* alloca = std::get_if<LirAllocaOp>(&inst);
      if (alloca == nullptr || is_param_alloca_name(alloca->result)) {
        continue;
      }
      alloca_set.insert(alloca->result);
    }
  }

  [[nodiscard]] std::optional<std::string> resolve_root(
      std::string_view value_name) const {
    const std::string key(value_name);
    if (alloca_set.find(key) != alloca_set.end()) {
      return key;
    }
    const auto it = gep_to_alloca.find(key);
    if (it == gep_to_alloca.end()) {
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

  void mark_escaped_values_in_text(std::string_view text, std::size_t block_index) {
    std::vector<std::string> values;
    collect_value_names_from_text(text, values);
    for (const auto& value_name : values) {
      mark_escaped(value_name, block_index);
    }
  }

  void scan_gep(const LirInst& inst, std::size_t block_index) {
    const auto* gep = std::get_if<LirGepOp>(&inst);
    if (gep == nullptr) {
      return;
    }
    const auto root = resolve_root(gep->ptr);
    if (!root.has_value()) {
      return;
    }
    gep_to_alloca.emplace(gep->result, *root);
    record_use(gep->result, block_index);
  }

  void scan_instruction(const LirInst& inst, std::size_t block_index) {
    std::visit(
        [&](const auto& op) {
          using T = std::decay_t<decltype(op)>;

          if constexpr (std::is_same_v<T, LirCallOp>) {
            if (is_value_name(op.callee)) {
              mark_escaped(op.callee, block_index);
            }
            mark_escaped_values_in_text(op.args_str, block_index);
          } else if constexpr (std::is_same_v<T, LirStoreOp>) {
            mark_escaped(op.val, block_index);
            record_use(op.ptr, block_index);
          } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirLoadOp>) {
            record_use(op.ptr, block_index);
          } else if constexpr (std::is_same_v<T, LirMemcpyOp>) {
            record_use(op.dst, block_index);
            record_use(op.src, block_index);
          } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirInlineAsmOp>) {
            mark_escaped_values_in_text(op.args_str, block_index);
          } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirVaStartOp> ||
                               std::is_same_v<T, c4c::codegen::lir::LirVaEndOp> ||
                               std::is_same_v<T, c4c::codegen::lir::LirVaArgOp>) {
            mark_escaped(op.ap_ptr, block_index);
          } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirVaCopyOp>) {
            mark_escaped(op.dst_ptr, block_index);
            mark_escaped(op.src_ptr, block_index);
          } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirCastOp>) {
            mark_escaped(op.operand, block_index);
          } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirBinOp>) {
            mark_escaped(op.lhs, block_index);
            mark_escaped(op.rhs, block_index);
          } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirCmpOp>) {
            mark_escaped(op.lhs, block_index);
            mark_escaped(op.rhs, block_index);
          } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirSelectOp>) {
            mark_escaped(op.cond, block_index);
            mark_escaped(op.true_val, block_index);
            mark_escaped(op.false_val, block_index);
          } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirPhiOp>) {
            for (const auto& [value_name, _] : op.incoming) {
              mark_escaped(value_name, block_index);
            }
          } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirExtractValueOp>) {
            mark_escaped(op.agg, block_index);
          } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirInsertValueOp>) {
            mark_escaped(op.agg, block_index);
            mark_escaped(op.elem, block_index);
          } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirInsertElementOp>) {
            mark_escaped(op.vec, block_index);
            mark_escaped(op.elem, block_index);
            mark_escaped(op.index, block_index);
          } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirExtractElementOp>) {
            mark_escaped(op.vec, block_index);
            mark_escaped(op.index, block_index);
          } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirShuffleVectorOp>) {
            mark_escaped(op.vec1, block_index);
            mark_escaped(op.vec2, block_index);
            mark_escaped(op.mask, block_index);
          } else if constexpr (std::is_same_v<T, LirIndirectBrOp>) {
            mark_escaped(op.addr, block_index);
          }
        },
        inst);
  }

  void scan_terminator(const LirTerminator& terminator, std::size_t block_index) {
    std::visit(
        [&](const auto& term) {
          using T = std::decay_t<decltype(term)>;

          if constexpr (std::is_same_v<T, LirCondBr>) {
            mark_escaped(term.cond_name, block_index);
          } else if constexpr (std::is_same_v<T, LirRet>) {
            if (term.value_str.has_value()) {
              mark_escaped(*term.value_str, block_index);
            }
          } else if constexpr (std::is_same_v<T, LirSwitch>) {
            mark_escaped(term.selector_name, block_index);
          }
        },
        terminator);
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
    const LirFunction& function,
    const StackLayoutAnalysis&) {
  AllocaEscapeAnalysis escape_analysis(function);
  if (escape_analysis.alloca_set.empty()) {
    return {};
  }

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (const auto& inst : block.insts) {
      escape_analysis.scan_gep(inst, block_index);
      escape_analysis.scan_instruction(inst, block_index);
    }
    escape_analysis.scan_terminator(block.terminator, block_index);
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

}  // namespace c4c::backend::stack_layout
