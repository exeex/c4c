#include "liveness.hpp"
#include "../codegen/lir/call_args_ops.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace c4c::backend {

namespace {

using c4c::backend::BackendCfgBlock;
using c4c::backend::BackendCfgFunction;
using c4c::backend::BackendCfgPhiIncomingUse;
using c4c::backend::BackendCfgPoint;
using c4c::codegen::lir::LirBlock;
using c4c::codegen::lir::LirFunction;
using c4c::codegen::lir::LirInst;
using c4c::codegen::lir::LirTerminator;

std::optional<std::string> named_bir_value(const c4c::backend::bir::Value& value) {
  if (value.kind != c4c::backend::bir::Value::Kind::Named || value.name.empty()) {
    return std::nullopt;
  }
  return value.name;
}

void append_named_bir_value(std::vector<std::string>& values,
                            const c4c::backend::bir::Value& value) {
  if (const auto name = named_bir_value(value); name.has_value()) {
    values.push_back(*name);
  }
}

std::string result_name_for_inst(const c4c::codegen::lir::LirInst& inst) {
  return std::visit(
      [](const auto& op) -> std::string {
        using T = std::decay_t<decltype(op)>;
        if constexpr (std::is_same_v<T, c4c::codegen::lir::LirLoadOp> ||
                      std::is_same_v<T, c4c::codegen::lir::LirCastOp> ||
                      std::is_same_v<T, c4c::codegen::lir::LirGepOp> ||
                      std::is_same_v<T, c4c::codegen::lir::LirCallOp> ||
                      std::is_same_v<T, c4c::codegen::lir::LirBinOp> ||
                      std::is_same_v<T, c4c::codegen::lir::LirCmpOp> ||
                      std::is_same_v<T, c4c::codegen::lir::LirPhiOp> ||
                      std::is_same_v<T, c4c::codegen::lir::LirSelectOp> ||
                      std::is_same_v<T, c4c::codegen::lir::LirInsertElementOp> ||
                      std::is_same_v<T, c4c::codegen::lir::LirExtractElementOp> ||
                      std::is_same_v<T, c4c::codegen::lir::LirShuffleVectorOp> ||
                      std::is_same_v<T, c4c::codegen::lir::LirVaArgOp> ||
                      std::is_same_v<T, c4c::codegen::lir::LirInlineAsmOp> ||
                      std::is_same_v<T, c4c::codegen::lir::LirExtractValueOp> ||
                      std::is_same_v<T, c4c::codegen::lir::LirInsertValueOp> ||
                      std::is_same_v<T, c4c::codegen::lir::LirStackSaveOp> ||
                      std::is_same_v<T, c4c::codegen::lir::LirAbsOp>) {
          return op.result;
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirConstInt> ||
                             std::is_same_v<T, c4c::codegen::lir::LirConstFloat> ||
                             std::is_same_v<T, c4c::codegen::lir::LirLoad> ||
                             std::is_same_v<T, c4c::codegen::lir::LirBinary> ||
                             std::is_same_v<T, c4c::codegen::lir::LirCast> ||
                             std::is_same_v<T, c4c::codegen::lir::LirCmp> ||
                             std::is_same_v<T, c4c::codegen::lir::LirCall> ||
                             std::is_same_v<T, c4c::codegen::lir::LirGep> ||
                             std::is_same_v<T, c4c::codegen::lir::LirSelect> ||
                             std::is_same_v<T, c4c::codegen::lir::LirIntrinsic> ||
                             std::is_same_v<T, c4c::codegen::lir::LirInlineAsm>) {
          if (op.result.valid()) {
            return "%" + std::to_string(op.result.value);
          }
          return std::string{};
        } else {
          return std::string{};
        }
      },
      inst);
}

bool is_call_inst(const c4c::codegen::lir::LirInst& inst) {
  return std::holds_alternative<c4c::codegen::lir::LirCall>(inst) ||
         std::holds_alternative<c4c::codegen::lir::LirCallOp>(inst);
}

using BackendCfgPointerAccess = BackendCfgPoint::PointerAccess;

bool is_value_name(std::string_view token) {
  return c4c::codegen::lir::is_lir_value_name(token);
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

std::vector<BackendCfgPointerAccess> pointer_accesses_for_inst(const LirInst& inst) {
  return std::visit(
      [](const auto& op) -> std::vector<BackendCfgPointerAccess> {
        using T = std::decay_t<decltype(op)>;

        if constexpr (std::is_same_v<T, c4c::codegen::lir::LirLoadOp>) {
          return {{op.ptr, BackendCfgPointerAccess::Kind::Read}};
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirStoreOp>) {
          return {{op.ptr, BackendCfgPointerAccess::Kind::Store}};
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirMemcpyOp>) {
          return {
              {op.dst, BackendCfgPointerAccess::Kind::Store},
              {op.src, BackendCfgPointerAccess::Kind::Read},
          };
        } else {
          return {};
        }
      },
      inst);
}

std::vector<std::string> escaped_names_for_inst(const LirInst& inst) {
  return std::visit(
      [](const auto& op) -> std::vector<std::string> {
        using T = std::decay_t<decltype(op)>;

        if constexpr (std::is_same_v<T, c4c::codegen::lir::LirCallOp>) {
          std::vector<std::string> values;
          c4c::codegen::lir::collect_lir_value_names_from_call(op, values);
          return values;
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirInlineAsmOp>) {
          std::vector<std::string> values;
          if (is_value_name(op.args_str)) {
            append_unique(values, op.args_str);
          } else {
            c4c::codegen::lir::collect_lir_value_names_from_text(op.args_str, values);
          }
          return values;
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirPhiOp>) {
          std::vector<std::string> values;
          values.reserve(op.incoming.size());
          for (const auto& [value_name, _] : op.incoming) {
            if (is_value_name(value_name)) {
              append_unique(values, value_name);
            }
          }
          return values;
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirVaStartOp> ||
                             std::is_same_v<T, c4c::codegen::lir::LirVaEndOp> ||
                             std::is_same_v<T, c4c::codegen::lir::LirVaArgOp>) {
          return {op.ap_ptr};
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirVaCopyOp>) {
          return {op.dst_ptr, op.src_ptr};
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirCastOp>) {
          return {op.operand};
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirBinOp>) {
          return {op.lhs, op.rhs};
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirCmpOp>) {
          return {op.lhs, op.rhs};
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirSelectOp>) {
          return {op.cond, op.true_val, op.false_val};
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirExtractValueOp>) {
          return {op.agg};
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirInsertValueOp>) {
          return {op.agg, op.elem};
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirInsertElementOp>) {
          return {op.vec, op.elem, op.index};
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirExtractElementOp>) {
          return {op.vec, op.index};
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirShuffleVectorOp>) {
          return {op.vec1, op.vec2, op.mask};
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirIndirectBrOp>) {
          return {op.addr};
        } else {
          return {};
        }
      },
      inst);
}

std::optional<std::pair<std::string, std::string>> derived_pointer_root_for_inst(
    const LirInst& inst) {
  if (const auto* gep = std::get_if<c4c::codegen::lir::LirGepOp>(&inst); gep != nullptr) {
    return std::make_pair(gep->result, gep->ptr);
  }
  return std::nullopt;
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

std::vector<std::string> successor_labels(const LirTerminator& terminator) {
  return std::visit(
      [](const auto& term) -> std::vector<std::string> {
        using T = std::decay_t<decltype(term)>;
        if constexpr (std::is_same_v<T, c4c::codegen::lir::LirBr>) {
          return {term.target_label};
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirCondBr>) {
          return {term.true_label, term.false_label};
        } else if constexpr (std::is_same_v<T, c4c::codegen::lir::LirSwitch>) {
          std::vector<std::string> labels;
          labels.push_back(term.default_label);
          for (const auto& [_, label] : term.cases) {
            append_unique(labels, label);
          }
          return labels;
        } else {
          return {};
        }
      },
      terminator);
}

std::optional<std::string> result_name_for_bir_inst(const c4c::backend::bir::Inst& inst) {
  return std::visit(
      [](const auto& op) -> std::optional<std::string> {
        using T = std::decay_t<decltype(op)>;
        if constexpr (std::is_same_v<T, c4c::backend::bir::BinaryInst> ||
                      std::is_same_v<T, c4c::backend::bir::SelectInst> ||
                      std::is_same_v<T, c4c::backend::bir::CastInst> ||
                      std::is_same_v<T, c4c::backend::bir::LoadLocalInst> ||
                      std::is_same_v<T, c4c::backend::bir::LoadGlobalInst>) {
          return named_bir_value(op.result);
        } else if constexpr (std::is_same_v<T, c4c::backend::bir::CallInst>) {
          if (!op.result.has_value()) {
            return std::nullopt;
          }
          return named_bir_value(*op.result);
        } else {
          return std::nullopt;
        }
      },
      inst);
}

bool is_call_inst(const c4c::backend::bir::Inst& inst) {
  return std::holds_alternative<c4c::backend::bir::CallInst>(inst);
}

std::vector<std::string> used_names_for_bir_inst(const c4c::backend::bir::Inst& inst) {
  return std::visit(
      [](const auto& op) -> std::vector<std::string> {
        using T = std::decay_t<decltype(op)>;
        std::vector<std::string> values;

        if constexpr (std::is_same_v<T, c4c::backend::bir::BinaryInst>) {
          append_named_bir_value(values, op.lhs);
          append_named_bir_value(values, op.rhs);
        } else if constexpr (std::is_same_v<T, c4c::backend::bir::SelectInst>) {
          append_named_bir_value(values, op.lhs);
          append_named_bir_value(values, op.rhs);
          append_named_bir_value(values, op.true_value);
          append_named_bir_value(values, op.false_value);
        } else if constexpr (std::is_same_v<T, c4c::backend::bir::CastInst>) {
          append_named_bir_value(values, op.operand);
        } else if constexpr (std::is_same_v<T, c4c::backend::bir::CallInst>) {
          for (const auto& arg : op.args) {
            append_named_bir_value(values, arg);
          }
        } else if constexpr (std::is_same_v<T, c4c::backend::bir::StoreGlobalInst>) {
          append_named_bir_value(values, op.value);
        } else if constexpr (std::is_same_v<T, c4c::backend::bir::StoreLocalInst>) {
          append_named_bir_value(values, op.value);
        }

        return values;
      },
      inst);
}

std::vector<std::string> used_names_for_bir_terminator(
    const c4c::backend::bir::Terminator& terminator) {
  std::vector<std::string> values;
  if (terminator.kind == c4c::backend::bir::TerminatorKind::Return) {
    if (terminator.value.has_value()) {
      append_named_bir_value(values, *terminator.value);
    }
  } else if (terminator.kind == c4c::backend::bir::TerminatorKind::CondBranch) {
    append_named_bir_value(values, terminator.condition);
  }
  return values;
}

std::vector<std::string> successor_labels(const c4c::backend::bir::Terminator& terminator) {
  if (terminator.kind == c4c::backend::bir::TerminatorKind::Branch) {
    return {terminator.target_label};
  }
  if (terminator.kind == c4c::backend::bir::TerminatorKind::CondBranch) {
    return {terminator.true_label, terminator.false_label};
  }
  return {};
}

struct BlockState {
  std::uint32_t start = 0;
  std::uint32_t end = 0;
  std::set<std::string> gen;
  std::set<std::string> kill;
  std::set<std::string> live_in;
  std::set<std::string> live_out;
};

void ensure_interval(std::unordered_map<std::string, LiveInterval>& intervals,
                     const std::string& value_name,
                     std::uint32_t start_hint) {
  const auto [it, inserted] =
      intervals.emplace(value_name, LiveInterval{start_hint, start_hint, value_name});
  if (!inserted) {
    it->second.start = std::min(it->second.start, start_hint);
  }
}

void record_use(std::unordered_map<std::string, LiveInterval>& intervals,
                BlockState& block,
                const std::string& value_name,
                std::uint32_t point) {
  if (!is_value_name(value_name)) {
    return;
  }
  auto [it, inserted] = intervals.emplace(
      value_name, LiveInterval{0, point, value_name});
  if (!inserted) {
    it->second.end = std::max(it->second.end, point);
  }
  if (block.kill.find(value_name) == block.kill.end()) {
    block.gen.insert(value_name);
  }
}

void record_def(std::unordered_map<std::string, LiveInterval>& intervals,
                BlockState& block,
                const std::string& value_name,
                std::uint32_t point) {
  if (!is_value_name(value_name)) {
    return;
  }
  auto [it, inserted] =
      intervals.emplace(value_name, LiveInterval{point, point, value_name});
  if (!inserted) {
    it->second.start = std::min(it->second.start, point);
  }
  block.kill.insert(value_name);
}

template <typename Set>
bool assign_union_minus(std::set<std::string>& dst,
                        const Set& gen,
                        const std::set<std::string>& live_out,
                        const std::set<std::string>& kill) {
  std::set<std::string> next(gen.begin(), gen.end());
  for (const auto& value : live_out) {
    if (kill.find(value) == kill.end()) {
      next.insert(value);
    }
  }
  if (next == dst) {
    return false;
  }
  dst = std::move(next);
  return true;
}

}  // namespace

const LiveInterval* LivenessResult::find_interval(std::string_view value_name) const {
  for (const auto& interval : intervals) {
    if (interval.value_name == value_name) {
      return &interval;
    }
  }
  return nullptr;
}

BackendCfgFunction lower_bir_to_backend_cfg(const bir::Function& function) {
  BackendCfgFunction cfg;
  cfg.blocks.reserve(function.blocks.size());
  for (const auto& block : function.blocks) {
    BackendCfgBlock lowered_block;
    lowered_block.label = block.label;
    lowered_block.terminator_used_names = used_names_for_bir_terminator(block.terminator);
    lowered_block.successor_labels = successor_labels(block.terminator);
    lowered_block.insts.reserve(block.insts.size());

    for (const auto& inst : block.insts) {
      lowered_block.insts.push_back(BackendCfgPoint{
          used_names_for_bir_inst(inst),
          result_name_for_bir_inst(inst),
          is_call_inst(inst),
          {},
          {},
          std::nullopt,
      });
    }

    cfg.blocks.push_back(std::move(lowered_block));
  }
  return cfg;
}

BackendCfgFunction lower_lir_to_backend_cfg(const c4c::codegen::lir::LirFunction& function) {
  BackendCfgFunction cfg;
  cfg.entry_insts.reserve(function.alloca_insts.size());
  for (const auto& inst : function.alloca_insts) {
    std::string result_name = result_name_for_inst(inst);
    cfg.entry_insts.push_back(BackendCfgPoint{
        used_names_for_inst(inst),
        result_name.empty() ? std::nullopt : std::optional<std::string>(std::move(result_name)),
        is_call_inst(inst),
        pointer_accesses_for_inst(inst),
        escaped_names_for_inst(inst),
        derived_pointer_root_for_inst(inst),
    });
  }

  cfg.blocks.reserve(function.blocks.size());
  for (const auto& block : function.blocks) {
    BackendCfgBlock lowered_block;
    lowered_block.label = block.label;
    lowered_block.terminator_used_names = used_names_for_terminator(block.terminator);
    lowered_block.successor_labels = successor_labels(block.terminator);
    lowered_block.insts.reserve(block.insts.size());

    for (const auto& inst : block.insts) {
      std::string result_name = result_name_for_inst(inst);
      lowered_block.insts.push_back(BackendCfgPoint{
          used_names_for_inst(inst),
          result_name.empty() ? std::nullopt : std::optional<std::string>(std::move(result_name)),
          is_call_inst(inst),
          pointer_accesses_for_inst(inst),
          escaped_names_for_inst(inst),
          derived_pointer_root_for_inst(inst),
      });

      if (!std::holds_alternative<c4c::codegen::lir::LirPhiOp>(inst)) {
        continue;
      }
      const auto& phi = std::get<c4c::codegen::lir::LirPhiOp>(inst);
      for (const auto& [value_name, label] : phi.incoming) {
        cfg.phi_incoming_uses.push_back(BackendCfgPhiIncomingUse{label, value_name});
      }
    }

    cfg.blocks.push_back(std::move(lowered_block));
  }

  return cfg;
}

LivenessInput lower_backend_cfg_to_liveness_input(const BackendCfgFunction& function) {
  LivenessInput input;
  input.entry_insts.reserve(function.entry_insts.size());
  for (const auto& inst : function.entry_insts) {
    input.entry_insts.push_back(LivenessPoint{
        inst.used_names,
        inst.result_name,
        inst.is_call,
    });
  }

  input.blocks.reserve(function.blocks.size());
  for (const auto& block : function.blocks) {
    LivenessBlockInput lowered_block;
    lowered_block.label = block.label;
    lowered_block.terminator_used_names = block.terminator_used_names;
    lowered_block.successor_labels = block.successor_labels;
    lowered_block.insts.reserve(block.insts.size());
    for (const auto& inst : block.insts) {
      lowered_block.insts.push_back(LivenessPoint{
          inst.used_names,
          inst.result_name,
          inst.is_call,
      });
    }
    input.blocks.push_back(std::move(lowered_block));
  }

  input.phi_incoming_uses.reserve(function.phi_incoming_uses.size());
  for (const auto& phi_use : function.phi_incoming_uses) {
    input.phi_incoming_uses.push_back(LivenessPhiIncomingUse{
        phi_use.predecessor_label,
        phi_use.value_name,
    });
  }

  return input;
}

LivenessInput lower_lir_to_liveness_input(const c4c::codegen::lir::LirFunction& function) {
  const auto cfg = lower_lir_to_backend_cfg(function);
  auto input = lower_backend_cfg_to_liveness_input(cfg);
  return input;
}

LivenessResult compute_live_intervals(const LivenessInput& input) {
  if (input.blocks.empty() && input.entry_insts.empty()) {
    return {};
  }

  std::unordered_map<std::string, LiveInterval> interval_map;
  std::vector<std::uint32_t> call_points;
  std::vector<BlockState> blocks(input.blocks.size());
  std::unordered_map<std::string, std::size_t> label_to_index;
  label_to_index.reserve(input.blocks.size());
  for (std::size_t i = 0; i < input.blocks.size(); ++i) {
    label_to_index.emplace(input.blocks[i].label, i);
  }

  std::uint32_t point = 0;
  for (const auto& inst : input.entry_insts) {
    for (const auto& used_name : inst.used_names) {
      auto [it, inserted] =
          interval_map.emplace(used_name, LiveInterval{0, point, used_name});
      if (!inserted) {
        it->second.end = std::max(it->second.end, point);
      }
    }
    if (inst.result_name.has_value()) {
      ensure_interval(interval_map, *inst.result_name, point);
    }
    if (inst.is_call) {
      call_points.push_back(point);
    }
    ++point;
  }

  for (std::size_t block_index = 0; block_index < input.blocks.size(); ++block_index) {
    const auto& block = input.blocks[block_index];
    auto& block_state = blocks[block_index];
    block_state.start = point;

    for (const auto& inst : block.insts) {
      for (const auto& used_name : inst.used_names) {
        record_use(interval_map, block_state, used_name, point);
      }
      if (inst.is_call) {
        call_points.push_back(point);
      }
      if (inst.result_name.has_value()) {
        record_def(interval_map, block_state, *inst.result_name, point);
      }
      ++point;
    }

    for (const auto& used_name : block.terminator_used_names) {
      record_use(interval_map, block_state, used_name, point);
    }
    block_state.end = point;
    ++point;
  }

  for (const auto& phi_use : input.phi_incoming_uses) {
    const auto pred_it = label_to_index.find(phi_use.predecessor_label);
    if (pred_it == label_to_index.end()) {
      continue;
    }
    record_use(interval_map, blocks[pred_it->second], phi_use.value_name,
               blocks[pred_it->second].end);
  }

  bool changed = true;
  while (changed) {
    changed = false;
    for (std::size_t idx = input.blocks.size(); idx-- > 0;) {
      std::set<std::string> next_live_out;
      for (const auto& label : input.blocks[idx].successor_labels) {
        const auto succ_it = label_to_index.find(label);
        if (succ_it == label_to_index.end()) {
          continue;
        }
        next_live_out.insert(blocks[succ_it->second].live_in.begin(),
                             blocks[succ_it->second].live_in.end());
      }
      if (next_live_out != blocks[idx].live_out) {
        blocks[idx].live_out = std::move(next_live_out);
        changed = true;
      }
      changed |= assign_union_minus(blocks[idx].live_in, blocks[idx].gen,
                                    blocks[idx].live_out, blocks[idx].kill);
    }
  }

  for (std::size_t block_index = 0; block_index < input.blocks.size(); ++block_index) {
    const auto& block_state = blocks[block_index];
    for (const auto& value_name : block_state.live_in) {
      ensure_interval(interval_map, value_name, block_state.start);
      auto& interval = interval_map.at(value_name);
      interval.start = std::min(interval.start, block_state.start);
      interval.end = std::max(interval.end, block_state.start);
    }
    for (const auto& value_name : block_state.live_out) {
      ensure_interval(interval_map, value_name, block_state.start);
      interval_map.at(value_name).end =
          std::max(interval_map.at(value_name).end, block_state.end);
    }
  }

  std::vector<LiveInterval> intervals;
  intervals.reserve(interval_map.size());
  for (const auto& [_, interval] : interval_map) {
    intervals.push_back(interval);
  }
  std::sort(intervals.begin(), intervals.end(),
            [](const LiveInterval& lhs, const LiveInterval& rhs) {
              if (lhs.start != rhs.start) {
                return lhs.start < rhs.start;
              }
              return lhs.value_name < rhs.value_name;
            });

  return LivenessResult{std::move(intervals), std::move(call_points),
                        std::vector<std::uint32_t>(input.blocks.size(), 0)};
}

}  // namespace c4c::backend
