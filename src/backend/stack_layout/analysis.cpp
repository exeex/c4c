#include "analysis.hpp"

#include "regalloc_helpers.hpp"
#include "../lowering/call_decode.hpp"
#include "../../codegen/lir/call_args_ops.hpp"

#include <algorithm>

namespace c4c::backend::stack_layout {

namespace {

using c4c::codegen::lir::LirFunction;
using c4c::codegen::lir::LirInst;
using c4c::codegen::lir::LirTerminator;

bool is_value_name(std::string_view token) {
  return c4c::codegen::lir::is_lir_value_name(token);
}

bool starts_with(std::string_view text, std::string_view prefix) {
  return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

bool is_entry_alloca_name(std::string_view value_name) {
  return starts_with(value_name, "%lv.");
}

void append_unique(std::vector<std::string>& values, std::string value) {
  c4c::codegen::lir::append_unique_lir_value_name(values, std::move(value));
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
          c4c::codegen::lir::collect_lir_value_names_from_text(text, values);
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
          c4c::codegen::lir::collect_lir_value_names_from_call(op, values);
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
          c4c::codegen::lir::collect_lir_value_names_from_text(text, values);
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
    const StackLayoutInput& input) {
  std::unordered_map<std::string, std::string> param_alloca_inputs;
  for (const auto& alloca : input.entry_allocas) {
    if (!starts_with(alloca.alloca_name, "%lv.param.") || !alloca.paired_store_value.has_value() ||
        !starts_with(*alloca.paired_store_value, "%p.")) {
      continue;
    }
    param_alloca_inputs.emplace(alloca.alloca_name, *alloca.paired_store_value);
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
    const StackLayoutInput& input,
    std::unordered_set<std::string>& entry_allocas_overwritten_before_read) {
  std::unordered_map<std::string, std::string> pointer_roots;
  for (const auto& alloca : input.entry_allocas) {
    if (is_entry_alloca_name(alloca.alloca_name)) {
      pointer_roots.emplace(alloca.alloca_name, alloca.alloca_name);
    }
  }

  std::unordered_map<std::string, AllocaAccessKind> first_access_kind;

  for (const auto& block : input.blocks) {
    for (const auto& point : block.insts) {
      if (point.derived_pointer_root.has_value()) {
        const auto base_it = pointer_roots.find(point.derived_pointer_root->second);
        if (base_it != pointer_roots.end()) {
          pointer_roots.emplace(point.derived_pointer_root->first, base_it->second);
        }
      }

      for (const auto& access : point.pointer_accesses) {
        record_first_access(first_access_kind, pointer_roots, access.value_name,
                            access.kind == PointerAccessKind::Store ? AllocaAccessKind::Store
                                                                    : AllocaAccessKind::Read);
      }
      for (const auto& value_name : point.used_names) {
        record_first_access(first_access_kind, pointer_roots, value_name,
                            AllocaAccessKind::Read);
      }
    }

    for (const auto& value_name : block.terminator_used_names) {
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

void lower_function_entry_allocas(const c4c::codegen::lir::LirFunction& function,
                                  StackLayoutInput& input) {
  input.entry_allocas.reserve(function.alloca_insts.size());
  for (std::size_t index = 0; index < function.alloca_insts.size(); ++index) {
    const auto* alloca = std::get_if<c4c::codegen::lir::LirAllocaOp>(&function.alloca_insts[index]);
    if (alloca == nullptr) {
      continue;
    }

    EntryAllocaInput entry_alloca{
        alloca->result,
        alloca->type_str,
        alloca->align,
        std::nullopt,
    };
    if (index + 1 < function.alloca_insts.size()) {
      if (const auto* store =
              std::get_if<c4c::codegen::lir::LirStoreOp>(&function.alloca_insts[index + 1]);
          store != nullptr && store->ptr == alloca->result) {
        entry_alloca.paired_store_value = store->val;
      }
    }
    input.entry_allocas.push_back(std::move(entry_alloca));
  }
}

void lower_backend_cfg_signature_and_call_results(
    const c4c::backend::BackendCfgFunction& backend_cfg,
    StackLayoutInput& input) {
  input.signature_params.reserve(backend_cfg.signature_params.size());
  for (const auto& param : backend_cfg.signature_params) {
    input.signature_params.push_back(
        StackLayoutSignatureParam{param.type, param.operand, param.is_varargs});
    input.is_variadic = input.is_variadic || param.is_varargs;
  }
  input.return_type = backend_cfg.return_type;

  input.call_results.reserve(backend_cfg.call_results.size());
  for (const auto& call_result : backend_cfg.call_results) {
    input.call_results.push_back(
        StackLayoutCallResultInput{call_result.value_name, call_result.type_str});
  }
}

void lower_lir_blocks_to_stack_layout_input(
    const c4c::codegen::lir::LirFunction& function,
    StackLayoutInput& input) {
  input.blocks.reserve(function.blocks.size());
  for (const auto& block : function.blocks) {
    StackLayoutBlockInput lowered_block;
    lowered_block.label = block.label;
    lowered_block.terminator_used_names = used_names_for_terminator(block.terminator);
    lowered_block.insts.reserve(block.insts.size());

    for (const auto& inst : block.insts) {
      StackLayoutPoint point;
      point.used_names = used_names_for_inst(inst);

      std::visit(
          [&](const auto& op) {
            using T = std::decay_t<decltype(op)>;

            if constexpr (std::is_same_v<T, c4c::codegen::lir::LirGepOp>) {
              point.derived_pointer_root = std::make_pair(op.result, op.ptr);
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirLoadOp>) {
              point.pointer_accesses.push_back(PointerAccess{op.ptr, PointerAccessKind::Read});
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirStoreOp>) {
              point.pointer_accesses.push_back(PointerAccess{op.ptr, PointerAccessKind::Store});
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirMemcpyOp>) {
              point.pointer_accesses.push_back(PointerAccess{op.dst, PointerAccessKind::Store});
              point.pointer_accesses.push_back(PointerAccess{op.src, PointerAccessKind::Read});
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirCallOp> ||
                                 std::is_same_v<T, c4c::codegen::lir::LirInlineAsmOp>) {
              point.escaped_names = point.used_names;
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirPhiOp>) {
              point.escaped_names = point.used_names;
              point.used_names.clear();
              for (const auto& [value_name, label] : op.incoming) {
                input.phi_incoming_uses.push_back(PhiIncomingUse{label, value_name});
              }
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirVaStartOp> ||
                                 std::is_same_v<T, c4c::codegen::lir::LirVaEndOp> ||
                                 std::is_same_v<T, c4c::codegen::lir::LirVaArgOp>) {
              point.escaped_names.push_back(op.ap_ptr);
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirVaCopyOp>) {
              point.escaped_names.push_back(op.dst_ptr);
              point.escaped_names.push_back(op.src_ptr);
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirCastOp>) {
              point.escaped_names.push_back(op.operand);
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirBinOp>) {
              point.escaped_names.push_back(op.lhs);
              point.escaped_names.push_back(op.rhs);
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirCmpOp>) {
              point.escaped_names.push_back(op.lhs);
              point.escaped_names.push_back(op.rhs);
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirSelectOp>) {
              point.escaped_names.push_back(op.cond);
              point.escaped_names.push_back(op.true_val);
              point.escaped_names.push_back(op.false_val);
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirExtractValueOp>) {
              point.escaped_names.push_back(op.agg);
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirInsertValueOp>) {
              point.escaped_names.push_back(op.agg);
              point.escaped_names.push_back(op.elem);
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirInsertElementOp>) {
              point.escaped_names.push_back(op.vec);
              point.escaped_names.push_back(op.elem);
              point.escaped_names.push_back(op.index);
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirExtractElementOp>) {
              point.escaped_names.push_back(op.vec);
              point.escaped_names.push_back(op.index);
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirShuffleVectorOp>) {
              point.escaped_names.push_back(op.vec1);
              point.escaped_names.push_back(op.vec2);
              point.escaped_names.push_back(op.mask);
            } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirIndirectBrOp>) {
              point.escaped_names.push_back(op.addr);
            }
          },
          inst);

      lowered_block.insts.push_back(std::move(point));
    }

    input.blocks.push_back(std::move(lowered_block));
  }
}

void lower_backend_cfg_to_stack_layout_input(
    const c4c::backend::BackendCfgFunction& backend_cfg,
    StackLayoutInput& input) {
  input.blocks.reserve(backend_cfg.blocks.size());
  for (const auto& block : backend_cfg.blocks) {
    StackLayoutBlockInput lowered_block;
    lowered_block.label = block.label;
    lowered_block.terminator_used_names = block.terminator_used_names;
    lowered_block.insts.reserve(block.insts.size());

    for (const auto& inst : block.insts) {
      StackLayoutPoint point;
      point.used_names = inst.used_names;
      point.escaped_names = inst.escaped_names;
      point.derived_pointer_root = inst.derived_pointer_root;
      point.pointer_accesses.reserve(inst.pointer_accesses.size());
      for (const auto& access : inst.pointer_accesses) {
        point.pointer_accesses.push_back(PointerAccess{
            access.value_name,
            access.kind == c4c::backend::BackendCfgPoint::PointerAccess::Kind::Store
                ? PointerAccessKind::Store
                : PointerAccessKind::Read,
        });
      }
      lowered_block.insts.push_back(std::move(point));
    }

    input.blocks.push_back(std::move(lowered_block));
  }

  input.phi_incoming_uses.reserve(backend_cfg.phi_incoming_uses.size());
  for (const auto& phi_use : backend_cfg.phi_incoming_uses) {
    input.phi_incoming_uses.push_back(
        PhiIncomingUse{phi_use.predecessor_label, phi_use.value_name});
  }
}

}  // namespace

StackLayoutInput lower_lir_to_stack_layout_input(const c4c::codegen::lir::LirFunction& function) {
  StackLayoutInput input;
  const auto backend_cfg = c4c::backend::lower_lir_to_backend_cfg(function);
  lower_function_entry_allocas(function, input);
  lower_backend_cfg_signature_and_call_results(backend_cfg, input);
  lower_lir_blocks_to_stack_layout_input(function, input);
  return input;
}

StackLayoutInput lower_function_entry_alloca_stack_layout_input(
    const c4c::codegen::lir::LirFunction& function,
    const c4c::backend::BackendCfgFunction& backend_cfg) {
  StackLayoutInput input;
  lower_function_entry_allocas(function, input);
  lower_backend_cfg_signature_and_call_results(backend_cfg, input);
  lower_backend_cfg_to_stack_layout_input(backend_cfg, input);
  return input;
}

std::vector<std::string> collect_stack_layout_value_names(const StackLayoutInput& input) {
  std::vector<std::string> values;

  auto add_name = [&](std::string_view value_name) {
    if (!is_value_name(value_name)) {
      return;
    }
    append_unique(values, std::string(value_name));
  };

  for (const auto& alloca : input.entry_allocas) {
    add_name(alloca.alloca_name);
    if (alloca.paired_store_value.has_value()) {
      add_name(*alloca.paired_store_value);
    }
  }

  for (const auto& block : input.blocks) {
    for (const auto& point : block.insts) {
      for (const auto& value_name : point.used_names) {
        add_name(value_name);
      }
      for (const auto& access : point.pointer_accesses) {
        add_name(access.value_name);
      }
      for (const auto& value_name : point.escaped_names) {
        add_name(value_name);
      }
      if (point.derived_pointer_root.has_value()) {
        add_name(point.derived_pointer_root->first);
        add_name(point.derived_pointer_root->second);
      }
    }
    for (const auto& value_name : block.terminator_used_names) {
      add_name(value_name);
    }
  }

  for (const auto& incoming : input.phi_incoming_uses) {
    add_name(incoming.value_name);
  }

  return values;
}

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
    const StackLayoutInput& input,
    const RegAllocIntegrationResult& regalloc,
    const std::vector<PhysReg>& callee_saved_regs) {
  StackLayoutAnalysis analysis;

  std::unordered_map<std::string, std::size_t> label_to_index;
  label_to_index.reserve(input.blocks.size());
  for (std::size_t block_index = 0; block_index < input.blocks.size(); ++block_index) {
    label_to_index.emplace(input.blocks[block_index].label, block_index);
  }

  for (const auto& phi_use : input.phi_incoming_uses) {
    const auto pred_it = label_to_index.find(phi_use.predecessor_label);
    const std::size_t use_block =
        pred_it == label_to_index.end() ? 0 : pred_it->second;
    record_use_block(analysis.value_use_blocks, phi_use.value_name, use_block);
    if (is_value_name(phi_use.value_name)) {
      analysis.used_values.insert(phi_use.value_name);
    }
  }

  for (std::size_t block_index = 0; block_index < input.blocks.size(); ++block_index) {
    const auto& block = input.blocks[block_index];
    for (const auto& point : block.insts) {
      for (const auto& value_name : point.used_names) {
        record_use_block(analysis.value_use_blocks, value_name, block_index);
        if (is_value_name(value_name)) {
          analysis.used_values.insert(value_name);
        }
      }
    }

    for (const auto& value_name : block.terminator_used_names) {
      record_use_block(analysis.value_use_blocks, value_name, block_index);
      if (is_value_name(value_name)) {
        analysis.used_values.insert(value_name);
      }
    }
  }

  for (const auto& [alloca_name, param_name] : collect_param_alloca_inputs(input)) {
    if (analysis.used_values.find(alloca_name) != analysis.used_values.end()) {
      continue;
    }
    const auto* assigned_reg = find_assigned_reg(regalloc, param_name);
    if (assigned_reg != nullptr && is_callee_saved_reg(*assigned_reg, callee_saved_regs)) {
      analysis.dead_param_allocas.insert(alloca_name);
    }
  }

  collect_first_entry_alloca_accesses(input,
                                      analysis.entry_allocas_overwritten_before_read);

  return analysis;
}

}  // namespace c4c::backend::stack_layout
