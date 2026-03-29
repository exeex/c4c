#include "liveness.hpp"

#include <algorithm>
#include <unordered_map>

namespace c4c::backend {

namespace {

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
                      std::is_same_v<T, c4c::codegen::lir::LirAllocaOp> ||
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

void record_inst(const c4c::codegen::lir::LirInst& inst,
                 std::uint32_t point,
                 std::unordered_map<std::string, LiveInterval>& intervals,
                 std::vector<std::uint32_t>& call_points) {
  if (is_call_inst(inst)) {
    call_points.push_back(point);
  }
  const std::string result_name = result_name_for_inst(inst);
  if (result_name.empty()) {
    return;
  }
  intervals.emplace(result_name, LiveInterval{point, point, result_name});
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

LivenessResult compute_live_intervals(const c4c::codegen::lir::LirFunction& function) {
  std::unordered_map<std::string, LiveInterval> interval_map;
  std::vector<std::uint32_t> call_points;
  std::uint32_t point = 0;

  for (const auto& inst : function.alloca_insts) {
    record_inst(inst, point++, interval_map, call_points);
  }

  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      record_inst(inst, point++, interval_map, call_points);
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
                        std::vector<std::uint32_t>(function.blocks.size(), 0)};
}

}  // namespace c4c::backend
