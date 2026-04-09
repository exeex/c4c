#include "passes.hpp"

#include <algorithm>
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

using c4c::codegen::lir::LirBlock;
using c4c::codegen::lir::LirBlockId;
using c4c::codegen::lir::LirCondBr;
using c4c::codegen::lir::LirFunction;
using c4c::codegen::lir::LirIndirectBr;
using c4c::codegen::lir::LirInst;
using c4c::codegen::lir::LirModule;
using c4c::codegen::lir::LirOperand;
using c4c::codegen::lir::LirPhiOp;
using c4c::codegen::lir::LirRet;
using c4c::codegen::lir::LirSwitch;
using c4c::codegen::lir::LirTerminator;

struct PhiIncoming {
  LirOperand value;
  std::string pred_label;
};

struct PhiInfo {
  LirOperand dest;
  std::string type_text;
  std::optional<bir::TypeKind> type_kind;
  std::vector<PhiIncoming> incoming;
};

struct LoweredPhiCopy {
  bir::Value dest;
  bir::Value src;
};

struct TrampolineBlock {
  std::string label;
  std::vector<LoweredPhiCopy> copies;
  std::string branch_target;
  std::size_t pred_index = 0;
  std::string old_target;
};

struct PhiLoweringCtx {
  std::unordered_map<std::string, std::size_t> label_to_idx;
  std::vector<bool> multi_succ;
  std::vector<bool> is_indirect_branch;
  std::unordered_map<std::size_t, std::vector<LoweredPhiCopy>> pred_copies;
  std::vector<std::vector<LoweredPhiCopy>> target_copies;
  std::vector<TrampolineBlock> trampolines;
  std::unordered_map<std::string, std::size_t> trampoline_map;
  std::uint32_t next_block_id = 0;
  std::uint32_t next_value = 0;
};

std::optional<bir::TypeKind> lower_phi_scalar_type(std::string_view type_text) {
  if (type_text == "i8") {
    return bir::TypeKind::I8;
  }
  if (type_text == "i32") {
    return bir::TypeKind::I32;
  }
  if (type_text == "i64" || type_text == "ptr") {
    return bir::TypeKind::I64;
  }
  return std::nullopt;
}

std::optional<bir::Value> lower_phi_operand_to_bir_value(const LirOperand& operand,
                                                         bir::TypeKind type) {
  const std::string_view text = operand.str();
  if (text.empty()) {
    return std::nullopt;
  }
  if (text.front() == '%') {
    return bir::Value::named(type, std::string(text));
  }

  std::int64_t value = 0;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto parsed = std::from_chars(begin, end, value);
  if (parsed.ec != std::errc() || parsed.ptr != end) {
    return std::nullopt;
  }

  switch (type) {
    case bir::TypeKind::I8:
      return bir::Value::immediate_i8(static_cast<std::int8_t>(value));
    case bir::TypeKind::I32:
      return bir::Value::immediate_i32(static_cast<std::int32_t>(value));
    case bir::TypeKind::I64:
      return bir::Value::immediate_i64(value);
    case bir::TypeKind::Void:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      break;
  }
  return std::nullopt;
}

std::size_t successor_count(const LirBlock& block) {
  std::unordered_set<std::string> seen;
  const auto record_label = [&seen](std::string_view label) {
    if (!label.empty()) {
      seen.emplace(label);
    }
  };

  if (std::holds_alternative<c4c::codegen::lir::LirBr>(block.terminator)) {
    record_label(std::get<c4c::codegen::lir::LirBr>(block.terminator).target_label);
  } else if (const auto* branch = std::get_if<LirCondBr>(&block.terminator)) {
    record_label(branch->true_label);
    record_label(branch->false_label);
  } else if (const auto* sw = std::get_if<LirSwitch>(&block.terminator)) {
    record_label(sw->default_label);
    for (const auto& [_, label] : sw->cases) {
      record_label(label);
    }
  } else if (const auto* ind = std::get_if<LirIndirectBr>(&block.terminator)) {
    for (const auto& target : ind->targets) {
      seen.emplace("#" + std::to_string(target.value));
    }
  } else {
    // Return / unreachable do not add any successors.
  }

  return seen.size();
}

void retarget_block_edge_once(LirBlock& block,
                             const std::string& old_target,
                             const std::string& new_target) {
  if (auto* br = std::get_if<c4c::codegen::lir::LirBr>(&block.terminator)) {
    if (br->target_label == old_target) {
      br->target_label = new_target;
      return;
    }
  } else if (auto* branch = std::get_if<LirCondBr>(&block.terminator)) {
    if (branch->true_label == old_target) {
      branch->true_label = new_target;
      return;
    }
    if (branch->false_label == old_target) {
      branch->false_label = new_target;
      return;
    }
  } else if (auto* sw = std::get_if<LirSwitch>(&block.terminator)) {
    if (sw->default_label == old_target) {
      sw->default_label = new_target;
      return;
    }
    for (auto& [_, label] : sw->cases) {
      if (label == old_target) {
        label = new_target;
        return;
      }
    }
  }
}

std::size_t get_or_create_trampoline(
    std::unordered_map<std::string, std::size_t>& trampoline_map,
    std::vector<TrampolineBlock>& trampolines,
    std::size_t pred_idx,
    const std::string& target_block_id,
    std::uint32_t* next_block_id) {
  const std::string key = std::to_string(pred_idx) + ":" + target_block_id;
  const auto [it, inserted] = trampoline_map.emplace(key, trampolines.size());
  if (inserted) {
    TrampolineBlock trampoline;
    trampoline.label = ".LBB" + std::to_string(*next_block_id);
    ++(*next_block_id);
    trampoline.branch_target = target_block_id;
    trampoline.pred_index = pred_idx;
    trampoline.old_target = target_block_id;
    trampolines.push_back(std::move(trampoline));
  }
  return it->second;
}

std::unordered_set<std::size_t> find_conflicting_phis(
    const std::vector<std::pair<std::string, std::optional<std::string>>>& copies) {
  std::unordered_set<std::string> dest_names;
  for (const auto& [dest, _] : copies) {
    if (!dest.empty()) {
      dest_names.insert(dest);
    }
  }

  std::unordered_set<std::size_t> needs_temp;
  for (std::size_t index = 0; index < copies.size(); ++index) {
    const auto& [dest, src] = copies[index];
    if (!src.has_value()) {
      continue;
    }
    if (*src != dest && dest_names.find(*src) != dest_names.end()) {
      needs_temp.insert(index);
    }
  }

  if (!needs_temp.empty()) {
    std::unordered_set<std::string> conflicting_sources;
    for (const auto index : needs_temp) {
      if (copies[index].second.has_value()) {
        conflicting_sources.insert(*copies[index].second);
      }
    }
    for (std::size_t index = 0; index < copies.size(); ++index) {
      if (conflicting_sources.find(copies[index].first) != conflicting_sources.end()) {
        needs_temp.insert(index);
      }
    }
  }

  return needs_temp;
}

std::vector<std::vector<PhiInfo>> collect_block_phis(const LirFunction& function) {
  std::vector<std::vector<PhiInfo>> block_phis;
  block_phis.reserve(function.blocks.size());
  for (const auto& block : function.blocks) {
    std::vector<PhiInfo> phis;
    for (const auto& inst : block.insts) {
      if (const auto* phi = std::get_if<LirPhiOp>(&inst)) {
        auto lowered_type = lower_phi_scalar_type(phi->type_str.str());
        PhiInfo info;
        info.dest = phi->result;
        info.type_text = phi->type_str.str();
        info.type_kind = lowered_type;
        for (const auto& [value, label] : phi->incoming) {
          info.incoming.push_back(PhiIncoming{LirOperand(value), label});
        }
        phis.push_back(std::move(info));
      }
    }
    block_phis.push_back(std::move(phis));
  }
  return block_phis;
}

void place_copy(PhiLoweringCtx& ctx,
                std::size_t pred_idx,
                const std::string& target_block_id,
                LoweredPhiCopy copy) {
  if (ctx.multi_succ[pred_idx] && !ctx.is_indirect_branch[pred_idx]) {
    const auto tramp_idx = get_or_create_trampoline(
        ctx.trampoline_map, ctx.trampolines, pred_idx, target_block_id, &ctx.next_block_id);
    ctx.trampolines[tramp_idx].copies.push_back(std::move(copy));
    return;
  }
  ctx.pred_copies[pred_idx].push_back(std::move(copy));
}

void place_copies(PhiLoweringCtx& ctx,
                  std::size_t pred_idx,
                  const std::string& target_block_id,
                  std::vector<LoweredPhiCopy> copies) {
  if (copies.empty()) {
    return;
  }
  if (ctx.multi_succ[pred_idx] && !ctx.is_indirect_branch[pred_idx]) {
    const auto tramp_idx = get_or_create_trampoline(
        ctx.trampoline_map, ctx.trampolines, pred_idx, target_block_id, &ctx.next_block_id);
    auto& staged = ctx.trampolines[tramp_idx].copies;
    std::move(copies.begin(), copies.end(), std::back_inserter(staged));
    return;
  }
  auto& staged = ctx.pred_copies[pred_idx];
  std::move(copies.begin(), copies.end(), std::back_inserter(staged));
}

std::vector<LoweredPhiCopy> build_edge_copies(
    const std::vector<PhiInfo>& phis,
    const std::vector<std::optional<bir::Value>>& phi_temps,
    const std::vector<std::unordered_map<std::string, std::string>>& phi_src_maps,
    const std::string& pred_label) {
  std::vector<LoweredPhiCopy> copies;

  // Pass 1: save temporary sources for phis that participate in a cycle.
  for (std::size_t index = 0; index < phis.size(); ++index) {
    if (!phi_temps[index].has_value()) {
      continue;
    }
    if (!phis[index].type_kind.has_value()) {
      continue;
    }
    const auto source_it = phi_src_maps[index].find(pred_label);
    if (source_it == phi_src_maps[index].end()) {
      continue;
    }
    const auto src_value =
        lower_phi_operand_to_bir_value(LirOperand(source_it->second), *phis[index].type_kind);
    if (!src_value.has_value()) {
      continue;
    }
    copies.push_back(LoweredPhiCopy{
        .dest = *phi_temps[index],
        .src = *src_value,
    });
  }

  // Pass 2: direct copies for the non-conflicting phis.
  for (std::size_t index = 0; index < phis.size(); ++index) {
    if (phi_temps[index].has_value()) {
      continue;
    }
    if (!phis[index].type_kind.has_value()) {
      continue;
    }
    const auto source_it = phi_src_maps[index].find(pred_label);
    if (source_it == phi_src_maps[index].end()) {
      continue;
    }
    if (source_it->second == phis[index].dest.str()) {
      continue;
    }
    const auto src_value =
        lower_phi_operand_to_bir_value(LirOperand(source_it->second), *phis[index].type_kind);
    if (!src_value.has_value()) {
      continue;
    }
    copies.push_back(LoweredPhiCopy{
        .dest = bir::Value::named(*phis[index].type_kind, phis[index].dest.str()),
        .src = *src_value,
    });
  }

  return copies;
}

void emit_single_phi_copies(const PhiInfo& phi,
                            const std::string& target_block_id,
                            PhiLoweringCtx& ctx) {
  if (!phi.type_kind.has_value()) {
    return;
  }
  for (const auto& incoming : phi.incoming) {
    const auto pred_it = ctx.label_to_idx.find(incoming.pred_label);
    if (pred_it == ctx.label_to_idx.end()) {
      continue;
    }
    if (incoming.value.str() == phi.dest.str()) {
      continue;
    }
    const auto src_value = lower_phi_operand_to_bir_value(incoming.value, *phi.type_kind);
    if (!src_value.has_value()) {
      continue;
    }
    place_copy(ctx,
               pred_it->second,
               target_block_id,
               LoweredPhiCopy{
                   .dest = bir::Value::named(*phi.type_kind, phi.dest.str()),
                   .src = *src_value,
               });
  }
}

void emit_multi_phi_copies(const std::vector<PhiInfo>& phis,
                           std::size_t block_idx,
                           const std::string& target_block_id,
                           PhiLoweringCtx& ctx) {
  std::unordered_set<std::string> pred_label_set;
  std::vector<std::string> pred_labels;
  for (const auto& phi : phis) {
    for (const auto& incoming : phi.incoming) {
      if (pred_label_set.insert(incoming.pred_label).second) {
        pred_labels.push_back(incoming.pred_label);
      }
    }
  }

  std::vector<std::unordered_map<std::string, std::string>> phi_src_maps;
  phi_src_maps.reserve(phis.size());
  for (const auto& phi : phis) {
    std::unordered_map<std::string, std::string> src_map;
    for (const auto& incoming : phi.incoming) {
      src_map.emplace(incoming.pred_label, incoming.value.str());
    }
    phi_src_maps.push_back(std::move(src_map));
  }

  std::vector<std::optional<bir::Value>> phi_temps(phis.size());
  std::vector<std::pair<std::string, std::optional<std::string>>> copies_info;
  copies_info.reserve(phis.size());

  for (const auto& pred_label : pred_labels) {
    copies_info.clear();
    for (std::size_t index = 0; index < phis.size(); ++index) {
      const auto src_it = phi_src_maps[index].find(pred_label);
      const std::optional<std::string> src =
          src_it == phi_src_maps[index].end() ? std::nullopt
                                                           : std::optional<std::string>(src_it->second);
      copies_info.emplace_back(phis[index].dest.str(), src);
    }

    const auto conflicting = find_conflicting_phis(copies_info);
    for (const auto index : conflicting) {
      if (!phi_temps[index].has_value() && phis[index].type_kind.has_value()) {
        phi_temps[index] = bir::Value::named(*phis[index].type_kind,
                                             "%phi_tmp" + std::to_string(ctx.next_value++));
      }
    }
  }

  for (std::size_t index = 0; index < phis.size(); ++index) {
    if (phi_temps[index].has_value()) {
      ctx.target_copies[block_idx].push_back(
          LoweredPhiCopy{
              .dest = bir::Value::named(*phis[index].type_kind, phis[index].dest.str()),
              .src = *phi_temps[index],
          });
    }
  }

  for (const auto& pred_label : pred_labels) {
    const auto pred_it = ctx.label_to_idx.find(pred_label);
    if (pred_it == ctx.label_to_idx.end()) {
      continue;
    }
    const auto copies = build_edge_copies(phis, phi_temps, phi_src_maps, pred_label);
    place_copies(ctx, pred_it->second, target_block_id, std::move(copies));
  }
}

void apply_phi_transformations(LirFunction& function, PhiLoweringCtx& ctx) {
  for (std::size_t block_idx = 0; block_idx < function.blocks.size(); ++block_idx) {
    auto& block = function.blocks[block_idx];

    // Remove the original phi instructions. The actual BIR materialization
    // will be wired in at the lowering bridge once this scaffold is connected.
    block.insts.erase(
        std::remove_if(block.insts.begin(), block.insts.end(), [](const LirInst& inst) {
          return std::holds_alternative<LirPhiOp>(inst);
        }),
        block.insts.end());

    (void)ctx;
  }

  for (const auto& trampoline : ctx.trampolines) {
    if (trampoline.pred_index < function.blocks.size()) {
      retarget_block_edge_once(function.blocks[trampoline.pred_index],
                               trampoline.old_target,
                               trampoline.label);
    }
  }
}

void eliminate_phis_in_function(LirFunction& function, std::uint32_t* next_block_id) {
  PhiLoweringCtx ctx;
  for (const auto& block : function.blocks) {
    ctx.label_to_idx.emplace(block.label, ctx.label_to_idx.size());
  }
  ctx.multi_succ.reserve(function.blocks.size());
  ctx.is_indirect_branch.reserve(function.blocks.size());
  for (const auto& block : function.blocks) {
    ctx.multi_succ.push_back(successor_count(block) > 1);
    ctx.is_indirect_branch.push_back(std::holds_alternative<LirIndirectBr>(block.terminator));
  }
  ctx.target_copies.resize(function.blocks.size());
  ctx.next_block_id = *next_block_id;
  ctx.next_value = function.next_value_id;

  const auto block_phis = collect_block_phis(function);
  for (std::size_t block_idx = 0; block_idx < block_phis.size(); ++block_idx) {
    const auto& phis = block_phis[block_idx];
    if (phis.empty()) {
      continue;
    }
    const std::string target_block_id = function.blocks[block_idx].label;
    if (phis.size() == 1) {
      emit_single_phi_copies(phis.front(), target_block_id, ctx);
    } else {
      emit_multi_phi_copies(phis, block_idx, target_block_id, ctx);
    }
  }

  apply_phi_transformations(function, ctx);
  function.next_value_id = ctx.next_value;
  *next_block_id = ctx.next_block_id;
}

}  // namespace

// Translation scaffold entry point.
//
// The main lowering pipeline can call this once the LIR module has already been
// normalized into backend-owned block/value form. The scaffold intentionally
// stops at the phi-elimination shape from the Rust reference:
//   - collect phi nodes per block
//   - detect copy-cycle interference per predecessor edge
//   - split critical edges with trampolines
//   - remove the original phi instructions
//
// The concrete materialization into BIR instructions is intentionally left to
// the owning lowering bridge so this file can stay focused on the phi seam.
void lower_phi_nodes_scaffold(LirModule& module) {
  std::uint32_t next_block_id = 0;
  for (const auto& function : module.functions) {
    for (const auto& block : function.blocks) {
      next_block_id = std::max(next_block_id, block.id.value + 1);
    }
  }

  for (auto& function : module.functions) {
    if (function.is_declaration || function.blocks.empty()) {
      continue;
    }
    eliminate_phis_in_function(function, &next_block_id);
  }
}

void lower_phi_nodes_in_place(LirModule& module, std::vector<BirLoweringNote>* notes) {
  lower_phi_nodes_scaffold(module);
  if (notes != nullptr) {
    notes->push_back(BirLoweringNote{
        .phase = "phi-lowering",
        .message = "phi lowering scaffold moved into lir_to_bir/phi.cpp",
    });
  }
}

}  // namespace c4c::backend
