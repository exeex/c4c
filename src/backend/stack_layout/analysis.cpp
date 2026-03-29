#include "analysis.hpp"

#include "regalloc_helpers.hpp"

#include <algorithm>
#include <cctype>

namespace c4c::backend::stack_layout {

namespace {

using c4c::codegen::lir::LirFunction;
using c4c::codegen::lir::LirInst;
using c4c::codegen::lir::LirTerminator;

bool is_value_name_char(char ch) {
  return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '.' ||
         ch == '-';
}

bool is_value_name(std::string_view token) {
  return !token.empty() && token.front() == '%';
}

bool starts_with(std::string_view text, std::string_view prefix) {
  return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

bool is_entry_alloca_name(std::string_view value_name) {
  return starts_with(value_name, "%lv.");
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

std::vector<std::string> used_names_for_inst(const LirInst& inst) {
  return std::visit(
      [](const auto& op) -> std::vector<std::string> {
        using T = std::decay_t<decltype(op)>;
        std::vector<std::string> values;

        auto add_text = [&values](std::string_view text) {
          if (is_value_name(text)) {
            append_unique(values, std::string(text));
            return;
          }
          collect_value_names_from_text(text, values);
        };
        auto add_id = [&values](c4c::codegen::lir::LirValueId id) {
          if (id.valid()) {
            append_unique(values, "%" + std::to_string(id.value));
          }
        };

        if constexpr (std::is_same_v<T, c4c::codegen::lir::LirLoad>) {
          add_id(op.ptr);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirStore>) {
          add_id(op.ptr);
          add_id(op.val);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirBinary>) {
          add_id(op.lhs);
          add_id(op.rhs);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirCast>) {
          add_id(op.operand);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirCmp>) {
          add_id(op.lhs);
          add_id(op.rhs);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirCall>) {
          add_id(op.callee_ptr);
          for (const auto arg : op.args) {
            add_id(arg);
          }
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirGep>) {
          add_id(op.base_ptr);
          for (const auto index : op.indices) {
            add_id(index);
          }
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirSelect>) {
          add_id(op.cond);
          add_id(op.true_val);
          add_id(op.false_val);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirIntrinsic>) {
          for (const auto arg : op.args) {
            add_id(arg);
          }
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirInlineAsm>) {
          for (const auto operand : op.operands) {
            add_id(operand);
          }
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirMemcpyOp>) {
          add_text(op.dst);
          add_text(op.src);
          add_text(op.size);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirVaStartOp> ||
                             std::is_same_v<T, c4c::codegen::lir::LirVaEndOp>) {
          add_text(op.ap_ptr);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirVaCopyOp>) {
          add_text(op.dst_ptr);
          add_text(op.src_ptr);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirStackRestoreOp>) {
          add_text(op.saved_ptr);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirAbsOp>) {
          add_text(op.arg);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirIndirectBrOp>) {
          add_text(op.addr);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirExtractValueOp>) {
          add_text(op.agg);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirInsertValueOp>) {
          add_text(op.agg);
          add_text(op.elem);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirLoadOp>) {
          add_text(op.ptr);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirStoreOp>) {
          add_text(op.val);
          add_text(op.ptr);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirCastOp>) {
          add_text(op.operand);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirGepOp>) {
          add_text(op.ptr);
          for (const auto& index : op.indices) {
            add_text(index);
          }
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirCallOp>) {
          add_text(op.callee);
          add_text(op.args_str);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirBinOp>) {
          add_text(op.lhs);
          add_text(op.rhs);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirCmpOp>) {
          add_text(op.lhs);
          add_text(op.rhs);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirPhiOp>) {
          for (const auto& [value_name, _] : op.incoming) {
            add_text(value_name);
          }
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirSelectOp>) {
          add_text(op.cond);
          add_text(op.true_val);
          add_text(op.false_val);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirInsertElementOp>) {
          add_text(op.vec);
          add_text(op.elem);
          add_text(op.index);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirExtractElementOp>) {
          add_text(op.vec);
          add_text(op.index);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirShuffleVectorOp>) {
          add_text(op.vec1);
          add_text(op.vec2);
          add_text(op.mask);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirVaArgOp>) {
          add_text(op.ap_ptr);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirAllocaOp>) {
          add_text(op.count);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirInlineAsmOp>) {
          add_text(op.args_str);
        }

        return values;
      },
      inst);
}

std::vector<std::string> used_names_for_terminator(const LirTerminator& terminator) {
  return std::visit(
      [](const auto& term) -> std::vector<std::string> {
        using T = std::decay_t<decltype(term)>;
        std::vector<std::string> values;
        auto add_text = [&values](std::string_view text) {
          if (is_value_name(text)) {
            append_unique(values, std::string(text));
            return;
          }
          collect_value_names_from_text(text, values);
        };
        auto add_id = [&values](c4c::codegen::lir::LirValueId id) {
          if (id.valid()) {
            append_unique(values, "%" + std::to_string(id.value));
          }
        };

        if constexpr (std::is_same_v<T, c4c::codegen::lir::LirCondBr>) {
          add_text(term.cond_name);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirRet>) {
          if (term.value_str.has_value()) {
            add_text(*term.value_str);
          }
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirSwitch>) {
          add_text(term.selector_name);
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirIndirectBr>) {
          add_id(term.addr);
        }

        return values;
      },
      terminator);
}

void record_use_block(std::unordered_map<std::string, std::vector<std::size_t>>& use_blocks,
                      const std::string& value_name,
                      std::size_t block_index) {
  if (!is_value_name(value_name)) {
    return;
  }
  auto& blocks = use_blocks[value_name];
  if (blocks.empty() || blocks.back() != block_index) {
    blocks.push_back(block_index);
  }
}

bool is_callee_saved_reg(PhysReg reg, const std::vector<PhysReg>& callee_saved_regs) {
  return std::any_of(callee_saved_regs.begin(), callee_saved_regs.end(),
                     [&](const PhysReg& candidate) { return candidate.index == reg.index; });
}

std::unordered_map<std::string, std::string> collect_param_alloca_inputs(
    const LirFunction& function) {
  std::unordered_map<std::string, std::string> param_alloca_inputs;
  for (std::size_t index = 0; index + 1 < function.alloca_insts.size(); ++index) {
    const auto* alloca = std::get_if<c4c::codegen::lir::LirAllocaOp>(&function.alloca_insts[index]);
    const auto* store =
        std::get_if<c4c::codegen::lir::LirStoreOp>(&function.alloca_insts[index + 1]);
    if (alloca == nullptr || store == nullptr) {
      continue;
    }
    if (!starts_with(alloca->result, "%lv.param.")) {
      continue;
    }
    if (store->ptr != alloca->result || !starts_with(store->val, "%p.")) {
      continue;
    }
    param_alloca_inputs.emplace(alloca->result, store->val);
  }
  return param_alloca_inputs;
}

enum class AllocaAccessKind {
  Store,
  Read,
};

void record_first_access(
    std::unordered_map<std::string, AllocaAccessKind>& first_access_kind,
    const std::unordered_map<std::string, std::string>& pointer_roots,
    std::string_view value_name,
    AllocaAccessKind access_kind) {
  const auto root_it = pointer_roots.find(std::string(value_name));
  if (root_it == pointer_roots.end()) {
    return;
  }
  first_access_kind.try_emplace(root_it->second, access_kind);
}

void collect_first_entry_alloca_accesses(
    const LirFunction& function,
    std::unordered_set<std::string>& entry_allocas_overwritten_before_read) {
  std::unordered_map<std::string, std::string> pointer_roots;
  for (const auto& inst : function.alloca_insts) {
    if (const auto* alloca = std::get_if<c4c::codegen::lir::LirAllocaOp>(&inst)) {
      if (is_entry_alloca_name(alloca->result)) {
        pointer_roots.emplace(alloca->result, alloca->result);
      }
    }
  }

  std::unordered_map<std::string, AllocaAccessKind> first_access_kind;

  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      std::visit(
          [&](const auto& op) {
            using T = std::decay_t<decltype(op)>;

            if constexpr (std::is_same_v<T, c4c::codegen::lir::LirGepOp>) {
              const auto base_it = pointer_roots.find(op.ptr);
              if (base_it != pointer_roots.end()) {
                pointer_roots.emplace(op.result, base_it->second);
              }
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirLoadOp>) {
              record_first_access(first_access_kind, pointer_roots, op.ptr,
                                  AllocaAccessKind::Read);
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirStoreOp>) {
              record_first_access(first_access_kind, pointer_roots, op.ptr,
                                  AllocaAccessKind::Store);
              record_first_access(first_access_kind, pointer_roots, op.val,
                                  AllocaAccessKind::Read);
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirMemcpyOp>) {
              record_first_access(first_access_kind, pointer_roots, op.dst,
                                  AllocaAccessKind::Store);
              record_first_access(first_access_kind, pointer_roots, op.src,
                                  AllocaAccessKind::Read);
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirCallOp>) {
              std::vector<std::string> values;
              collect_value_names_from_text(op.args_str, values);
              for (const auto& value_name : values) {
                record_first_access(first_access_kind, pointer_roots, value_name,
                                    AllocaAccessKind::Read);
              }
            } else {
              for (const auto& value_name : used_names_for_inst(inst)) {
                record_first_access(first_access_kind, pointer_roots, value_name,
                                    AllocaAccessKind::Read);
              }
            }
          },
          inst);
    }

    for (const auto& value_name : used_names_for_terminator(block.terminator)) {
      record_first_access(first_access_kind, pointer_roots, value_name,
                          AllocaAccessKind::Read);
    }
  }

  for (const auto& [alloca_name, access_kind] : first_access_kind) {
    if (access_kind == AllocaAccessKind::Store) {
      entry_allocas_overwritten_before_read.insert(alloca_name);
    }
  }
}

}  // namespace

const std::vector<std::size_t>* StackLayoutAnalysis::find_use_blocks(
    std::string_view value_name) const {
  const auto it = value_use_blocks.find(std::string(value_name));
  if (it == value_use_blocks.end()) {
    return nullptr;
  }
  return &it->second;
}

bool StackLayoutAnalysis::uses_value(std::string_view value_name) const {
  return used_values.find(std::string(value_name)) != used_values.end();
}

bool StackLayoutAnalysis::is_dead_param_alloca(std::string_view value_name) const {
  return dead_param_allocas.find(std::string(value_name)) != dead_param_allocas.end();
}

bool StackLayoutAnalysis::is_entry_alloca_overwritten_before_read(
    std::string_view value_name) const {
  return entry_allocas_overwritten_before_read.find(std::string(value_name)) !=
         entry_allocas_overwritten_before_read.end();
}

StackLayoutAnalysis analyze_stack_layout(
    const c4c::codegen::lir::LirFunction& function,
    const RegAllocIntegrationResult& regalloc,
    const std::vector<PhysReg>& callee_saved_regs) {
  StackLayoutAnalysis analysis;

  std::unordered_map<std::string, std::size_t> label_to_index;
  label_to_index.reserve(function.blocks.size());
  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    label_to_index.emplace(function.blocks[block_index].label, block_index);
  }

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (const auto& inst : block.insts) {
      if (const auto* phi = std::get_if<c4c::codegen::lir::LirPhiOp>(&inst)) {
        for (const auto& [value_name, label] : phi->incoming) {
          const auto pred_it = label_to_index.find(label);
          const std::size_t use_block =
              pred_it == label_to_index.end() ? block_index : pred_it->second;
          record_use_block(analysis.value_use_blocks, value_name, use_block);
          if (is_value_name(value_name)) {
            analysis.used_values.insert(value_name);
          }
        }
        continue;
      }

      for (const auto& value_name : used_names_for_inst(inst)) {
        record_use_block(analysis.value_use_blocks, value_name, block_index);
        analysis.used_values.insert(value_name);
      }
    }

    for (const auto& value_name : used_names_for_terminator(block.terminator)) {
      record_use_block(analysis.value_use_blocks, value_name, block_index);
      analysis.used_values.insert(value_name);
    }
  }

  for (const auto& [alloca_name, param_name] : collect_param_alloca_inputs(function)) {
    if (analysis.used_values.find(alloca_name) != analysis.used_values.end()) {
      continue;
    }
    const auto* assigned_reg = find_assigned_reg(regalloc, param_name);
    if (assigned_reg != nullptr && is_callee_saved_reg(*assigned_reg, callee_saved_regs)) {
      analysis.dead_param_allocas.insert(alloca_name);
    }
  }

  collect_first_entry_alloca_accesses(function,
                                      analysis.entry_allocas_overwritten_before_read);

  return analysis;
}

}  // namespace c4c::backend::stack_layout
