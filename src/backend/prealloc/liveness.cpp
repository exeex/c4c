#include "prealloc.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <queue>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace c4c::backend::prepare {

namespace {

constexpr std::size_t kNoPoint = std::numeric_limits<std::size_t>::max();

class BitSet {
 public:
  BitSet() = default;

  explicit BitSet(std::size_t bit_count)
      : words_((bit_count + 63U) / 64U, 0) {}

  [[nodiscard]] bool contains(std::size_t bit) const {
    const std::size_t word_index = bit / 64U;
    const std::size_t word_bit = bit % 64U;
    return word_index < words_.size() && ((words_[word_index] >> word_bit) & 1U) != 0;
  }

  void insert(std::size_t bit) {
    const std::size_t word_index = bit / 64U;
    const std::size_t word_bit = bit % 64U;
    words_[word_index] |= (std::uint64_t{1} << word_bit);
  }

  [[nodiscard]] bool union_with(const BitSet& other) {
    bool changed = false;
    for (std::size_t index = 0; index < words_.size(); ++index) {
      const std::uint64_t old_word = words_[index];
      words_[index] |= other.words_[index];
      changed = changed || words_[index] != old_word;
    }
    return changed;
  }

  [[nodiscard]] bool assign(const BitSet& other) {
    if (words_ == other.words_) {
      return false;
    }
    words_ = other.words_;
    return true;
  }

  [[nodiscard]] bool assign_gen_union_out_minus_kill(const BitSet& gen,
                                                     const BitSet& out,
                                                     const BitSet& kill) {
    bool changed = false;
    for (std::size_t index = 0; index < words_.size(); ++index) {
      const std::uint64_t next_word = gen.words_[index] | (out.words_[index] & ~kill.words_[index]);
      if (next_word != words_[index]) {
        words_[index] = next_word;
        changed = true;
      }
    }
    return changed;
  }

  template <typename Fn>
  void for_each_set_bit(Fn&& fn) const {
    for (std::size_t word_index = 0; word_index < words_.size(); ++word_index) {
      std::uint64_t word = words_[word_index];
      while (word != 0) {
        const std::size_t bit = static_cast<std::size_t>(__builtin_ctzll(word));
        fn((word_index * 64U) + bit);
        word &= (word - 1U);
      }
    }
  }

 private:
  std::vector<std::uint64_t> words_;
};

struct DenseValueInfo {
  ValueNameId value_name = kInvalidValueName;
  bir::TypeKind type = bir::TypeKind::Void;
  PreparedValueKind value_kind = PreparedValueKind::Temporary;
  std::optional<PreparedObjectId> stack_object_id;
  bool address_taken = false;
  bool requires_home_slot = false;
};

struct DenseValueSet {
  std::vector<DenseValueInfo> values;
  std::unordered_map<ValueNameId, std::size_t> name_to_index;
};

struct BlockProgramPoints {
  std::size_t start_point = 0;
  std::size_t end_point = 0;
  BitSet gen;
  BitSet kill;
};

struct ProgramPointState {
  std::vector<BlockProgramPoints> blocks;
  std::vector<std::optional<std::size_t>> def_points;
  std::vector<std::vector<std::size_t>> use_points;
  std::vector<std::size_t> first_points;
  std::vector<std::size_t> last_points;
  std::vector<bool> seen_activity;
  std::vector<std::size_t> call_points;
  std::size_t total_points = 0;
};

void absorb_value_metadata(DenseValueInfo& info,
                           bir::TypeKind type,
                           PreparedValueKind value_kind,
                           std::optional<PreparedObjectId> stack_object_id,
                           bool address_taken,
                           bool requires_home_slot) {
  if (info.type == bir::TypeKind::Void && type != bir::TypeKind::Void) {
    info.type = type;
  }
  if (value_kind != PreparedValueKind::Temporary) {
    info.value_kind = value_kind;
  }
  if (!info.stack_object_id.has_value() && stack_object_id.has_value()) {
    info.stack_object_id = stack_object_id;
  }
  info.address_taken = info.address_taken || address_taken;
  info.requires_home_slot = info.requires_home_slot || requires_home_slot;
}

[[nodiscard]] std::unordered_map<ValueNameId, std::vector<const PreparedStackObject*>>
build_stack_object_lookup(FunctionNameId function_name,
                          const PreparedStackLayout& stack_layout) {
  std::unordered_map<ValueNameId, std::vector<const PreparedStackObject*>> lookup;
  for (const auto& object : stack_layout.objects) {
    if (object.function_name != function_name) {
      continue;
    }
    if (!object.value_name.has_value()) {
      continue;
    }
    lookup[*object.value_name].push_back(&object);
  }
  return lookup;
}

[[nodiscard]] std::optional<ValueNameId> resolve_named_value_id(const PreparedNameTables& names,
                                                                const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return std::nullopt;
  }
  return resolve_prepared_value_name_id(names, value.name);
}

[[nodiscard]] std::optional<const PreparedStackObject*> find_unique_stack_object(
    ValueNameId value_name,
    const std::unordered_map<ValueNameId, std::vector<const PreparedStackObject*>>& lookup) {
  const auto found = lookup.find(value_name);
  if (found == lookup.end() || found->second.size() != 1U) {
    return std::nullopt;
  }
  return found->second.front();
}

std::size_t ensure_dense_value(DenseValueSet& dense_values,
                               ValueNameId value_name,
                               bir::TypeKind type,
                               PreparedValueKind value_kind,
                               const std::unordered_map<ValueNameId, std::vector<const PreparedStackObject*>>&
                                   stack_object_lookup,
                               bool address_taken = false,
                               bool requires_home_slot = false) {
  const auto [it, inserted] = dense_values.name_to_index.emplace(value_name, dense_values.values.size());
  if (inserted) {
    DenseValueInfo info{
        .value_name = value_name,
        .type = type,
        .value_kind = value_kind,
        .stack_object_id = std::nullopt,
        .address_taken = address_taken,
        .requires_home_slot = requires_home_slot,
    };
    if (const auto linked_object = find_unique_stack_object(value_name, stack_object_lookup);
        linked_object.has_value()) {
      absorb_value_metadata(info,
                            type,
                            value_kind,
                            (*linked_object)->object_id,
                            (*linked_object)->address_exposed || address_taken,
                            (*linked_object)->requires_home_slot || requires_home_slot);
    }
    dense_values.values.push_back(std::move(info));
    return dense_values.values.size() - 1U;
  }

  DenseValueInfo& info = dense_values.values[it->second];
  absorb_value_metadata(info, type, value_kind, std::nullopt, address_taken, requires_home_slot);
  return it->second;
}

void maybe_add_named_value(DenseValueSet& dense_values,
                           PreparedNameTables& names,
                           const bir::Value& value,
                           PreparedValueKind value_kind,
                           const std::unordered_map<ValueNameId, std::vector<const PreparedStackObject*>>&
                               stack_object_lookup,
                           bool address_taken = false,
                           bool requires_home_slot = false) {
  const auto value_name_id = prepared_named_value_id(names, value);
  if (!value_name_id.has_value()) {
    return;
  }
  (void)ensure_dense_value(
      dense_values,
      *value_name_id,
      value.type,
      value_kind,
      stack_object_lookup,
      address_taken,
      requires_home_slot);
}

void maybe_add_pointer_base_value(
    PreparedNameTables& names,
    DenseValueSet& dense_values,
    const std::optional<bir::MemoryAddress>& address,
    const std::unordered_map<ValueNameId, std::vector<const PreparedStackObject*>>& stack_object_lookup) {
  if (!address.has_value() || address->base_kind != bir::MemoryAddress::BaseKind::PointerValue) {
    return;
  }
  maybe_add_named_value(
      dense_values, names, address->base_value, PreparedValueKind::Temporary, stack_object_lookup, true);
}

void collect_dense_values_from_instruction(
    PreparedNameTables& names,
    DenseValueSet& dense_values,
    const bir::Inst& inst,
    const std::unordered_map<ValueNameId, std::vector<const PreparedStackObject*>>& stack_object_lookup) {
  std::visit(
      [&](const auto& typed_inst) {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::BinaryInst>) {
          maybe_add_named_value(
              dense_values, names, typed_inst.result, PreparedValueKind::Temporary, stack_object_lookup);
          maybe_add_named_value(
              dense_values, names, typed_inst.lhs, PreparedValueKind::Temporary, stack_object_lookup);
          maybe_add_named_value(
              dense_values, names, typed_inst.rhs, PreparedValueKind::Temporary, stack_object_lookup);
        } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
          maybe_add_named_value(
              dense_values, names, typed_inst.result, PreparedValueKind::Temporary, stack_object_lookup);
          maybe_add_named_value(
              dense_values, names, typed_inst.lhs, PreparedValueKind::Temporary, stack_object_lookup);
          maybe_add_named_value(
              dense_values, names, typed_inst.rhs, PreparedValueKind::Temporary, stack_object_lookup);
          maybe_add_named_value(
              dense_values, names, typed_inst.true_value, PreparedValueKind::Temporary, stack_object_lookup);
          maybe_add_named_value(
              dense_values, names, typed_inst.false_value, PreparedValueKind::Temporary, stack_object_lookup);
        } else if constexpr (std::is_same_v<T, bir::CastInst>) {
          maybe_add_named_value(
              dense_values, names, typed_inst.result, PreparedValueKind::Temporary, stack_object_lookup);
          maybe_add_named_value(
              dense_values, names, typed_inst.operand, PreparedValueKind::Temporary, stack_object_lookup);
        } else if constexpr (std::is_same_v<T, bir::PhiInst>) {
          maybe_add_named_value(
              dense_values, names, typed_inst.result, PreparedValueKind::Phi, stack_object_lookup);
          for (const auto& incoming : typed_inst.incomings) {
            maybe_add_named_value(
                dense_values, names, incoming.value, PreparedValueKind::Temporary, stack_object_lookup);
          }
        } else if constexpr (std::is_same_v<T, bir::CallInst>) {
          if (typed_inst.result.has_value()) {
            maybe_add_named_value(
                dense_values,
                names,
                *typed_inst.result,
                PreparedValueKind::CallResult,
                stack_object_lookup);
          }
          if (typed_inst.callee_value.has_value()) {
            maybe_add_named_value(
                dense_values,
                names,
                *typed_inst.callee_value,
                PreparedValueKind::Temporary,
                stack_object_lookup);
          }
          for (const auto& arg : typed_inst.args) {
            maybe_add_named_value(
                dense_values, names, arg, PreparedValueKind::Temporary, stack_object_lookup);
          }
        } else if constexpr (std::is_same_v<T, bir::LoadLocalInst>) {
          maybe_add_named_value(
              dense_values, names, typed_inst.result, PreparedValueKind::Temporary, stack_object_lookup);
          maybe_add_pointer_base_value(names, dense_values, typed_inst.address, stack_object_lookup);
        } else if constexpr (std::is_same_v<T, bir::LoadGlobalInst>) {
          maybe_add_named_value(
              dense_values, names, typed_inst.result, PreparedValueKind::Temporary, stack_object_lookup);
          maybe_add_pointer_base_value(names, dense_values, typed_inst.address, stack_object_lookup);
        } else if constexpr (std::is_same_v<T, bir::StoreGlobalInst>) {
          maybe_add_named_value(
              dense_values, names, typed_inst.value, PreparedValueKind::Temporary, stack_object_lookup);
          maybe_add_pointer_base_value(names, dense_values, typed_inst.address, stack_object_lookup);
        } else if constexpr (std::is_same_v<T, bir::StoreLocalInst>) {
          maybe_add_named_value(
              dense_values, names, typed_inst.value, PreparedValueKind::Temporary, stack_object_lookup);
          maybe_add_pointer_base_value(names, dense_values, typed_inst.address, stack_object_lookup);
        }
      },
      inst);
}

void collect_dense_values_from_terminator(
    PreparedNameTables& names,
    DenseValueSet& dense_values,
    const bir::Terminator& terminator,
    const std::unordered_map<ValueNameId, std::vector<const PreparedStackObject*>>& stack_object_lookup) {
  if (terminator.kind == bir::TerminatorKind::Return && terminator.value.has_value()) {
    maybe_add_named_value(
        dense_values, names, *terminator.value, PreparedValueKind::Temporary, stack_object_lookup);
  } else if (terminator.kind == bir::TerminatorKind::CondBranch) {
    maybe_add_named_value(
        dense_values, names, terminator.condition, PreparedValueKind::Temporary, stack_object_lookup);
  }
}

[[nodiscard]] DenseValueSet build_dense_value_set(PreparedNameTables& names,
                                                  const bir::Function& function,
                                                  const PreparedStackLayout& stack_layout) {
  const FunctionNameId function_name_id = names.function_names.intern(function.name);
  const auto stack_object_lookup = build_stack_object_lookup(function_name_id, stack_layout);
  DenseValueSet dense_values;

  for (const auto& param : function.params) {
    const bool requires_home_slot = param.is_byval || param.is_sret;
    const bool address_taken = param.is_byval || param.is_sret;
    const ValueNameId param_name_id = names.value_names.intern(param.name);
    if (param_name_id == kInvalidValueName) {
      continue;
    }
    (void)ensure_dense_value(dense_values,
                             param_name_id,
                             param.type,
                             PreparedValueKind::Parameter,
                             stack_object_lookup,
                             address_taken,
                             requires_home_slot);
  }

  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      collect_dense_values_from_instruction(names, dense_values, inst, stack_object_lookup);
    }
    collect_dense_values_from_terminator(names, dense_values, block.terminator, stack_object_lookup);
  }

  return dense_values;
}

void append_named_value_match(const PreparedNameTables& names,
                              const bir::Value& value,
                              const DenseValueSet& dense_values,
                              std::vector<std::size_t>& matches) {
  const auto value_name_id = resolve_named_value_id(names, value);
  if (!value_name_id.has_value()) {
    return;
  }
  const auto found = dense_values.name_to_index.find(*value_name_id);
  if (found != dense_values.name_to_index.end()) {
    matches.push_back(found->second);
  }
}

void append_pointer_base_match(const PreparedNameTables& names,
                               const std::optional<bir::MemoryAddress>& address,
                               const DenseValueSet& dense_values,
                               std::vector<std::size_t>& matches) {
  if (!address.has_value() || address->base_kind != bir::MemoryAddress::BaseKind::PointerValue) {
    return;
  }
  append_named_value_match(names, address->base_value, dense_values, matches);
}

void sort_and_unique(std::vector<std::size_t>& items) {
  std::sort(items.begin(), items.end());
  items.erase(std::unique(items.begin(), items.end()), items.end());
}

void collect_instruction_uses_and_defs(const PreparedNameTables& names,
                                       const bir::Inst& inst,
                                       const DenseValueSet& dense_values,
                                       std::vector<std::size_t>& uses,
                                       std::vector<std::size_t>& defs) {
  std::visit(
      [&](const auto& typed_inst) {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::BinaryInst>) {
          append_named_value_match(names, typed_inst.lhs, dense_values, uses);
          append_named_value_match(names, typed_inst.rhs, dense_values, uses);
          append_named_value_match(names, typed_inst.result, dense_values, defs);
        } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
          append_named_value_match(names, typed_inst.lhs, dense_values, uses);
          append_named_value_match(names, typed_inst.rhs, dense_values, uses);
          append_named_value_match(names, typed_inst.true_value, dense_values, uses);
          append_named_value_match(names, typed_inst.false_value, dense_values, uses);
          append_named_value_match(names, typed_inst.result, dense_values, defs);
        } else if constexpr (std::is_same_v<T, bir::CastInst>) {
          append_named_value_match(names, typed_inst.operand, dense_values, uses);
          append_named_value_match(names, typed_inst.result, dense_values, defs);
        } else if constexpr (std::is_same_v<T, bir::PhiInst>) {
          append_named_value_match(names, typed_inst.result, dense_values, defs);
        } else if constexpr (std::is_same_v<T, bir::CallInst>) {
          if (typed_inst.result.has_value()) {
            append_named_value_match(names, *typed_inst.result, dense_values, defs);
          }
          if (typed_inst.callee_value.has_value()) {
            append_named_value_match(names, *typed_inst.callee_value, dense_values, uses);
          }
          for (const auto& arg : typed_inst.args) {
            append_named_value_match(names, arg, dense_values, uses);
          }
        } else if constexpr (std::is_same_v<T, bir::LoadLocalInst>) {
          append_pointer_base_match(names, typed_inst.address, dense_values, uses);
          append_named_value_match(names, typed_inst.result, dense_values, defs);
        } else if constexpr (std::is_same_v<T, bir::LoadGlobalInst>) {
          append_pointer_base_match(names, typed_inst.address, dense_values, uses);
          append_named_value_match(names, typed_inst.result, dense_values, defs);
        } else if constexpr (std::is_same_v<T, bir::StoreGlobalInst>) {
          append_named_value_match(names, typed_inst.value, dense_values, uses);
          append_pointer_base_match(names, typed_inst.address, dense_values, uses);
        } else if constexpr (std::is_same_v<T, bir::StoreLocalInst>) {
          append_named_value_match(names, typed_inst.value, dense_values, uses);
          append_pointer_base_match(names, typed_inst.address, dense_values, uses);
        }
      },
      inst);
  sort_and_unique(uses);
  sort_and_unique(defs);
}

void collect_terminator_uses(const PreparedNameTables& names,
                             const bir::Terminator& terminator,
                             const DenseValueSet& dense_values,
                             std::vector<std::size_t>& uses) {
  if (terminator.kind == bir::TerminatorKind::Return && terminator.value.has_value()) {
    append_named_value_match(names, *terminator.value, dense_values, uses);
  } else if (terminator.kind == bir::TerminatorKind::CondBranch) {
    append_named_value_match(names, terminator.condition, dense_values, uses);
  }
  sort_and_unique(uses);
}

void record_point_activity(const std::vector<std::size_t>& uses,
                           const std::vector<std::size_t>& defs,
                           std::size_t point,
                           std::vector<std::optional<std::size_t>>& def_points,
                           std::vector<std::vector<std::size_t>>& use_points,
                           std::vector<std::size_t>& first_points,
                           std::vector<std::size_t>& last_points,
                           std::vector<bool>& seen_activity,
                           BitSet& gen,
                           BitSet& kill) {
  for (const std::size_t dense_index : uses) {
    seen_activity[dense_index] = true;
    use_points[dense_index].push_back(point);
    if (first_points[dense_index] == kNoPoint) {
      first_points[dense_index] = point;
    }
    last_points[dense_index] = std::max(last_points[dense_index], point);
    if (!kill.contains(dense_index)) {
      gen.insert(dense_index);
    }
  }

  for (const std::size_t dense_index : defs) {
    seen_activity[dense_index] = true;
    if (!def_points[dense_index].has_value()) {
      def_points[dense_index] = point;
    }
    if (first_points[dense_index] == kNoPoint) {
      first_points[dense_index] = point;
    }
    last_points[dense_index] = std::max(last_points[dense_index], point);
    kill.insert(dense_index);
  }
}

[[nodiscard]] BlockLabelId intern_preferred_block_label(PreparedNameTables& names,
                                                        const bir::NameTables& bir_names,
                                                        BlockLabelId block_label_id,
                                                        std::string_view raw_label) {
  if (block_label_id != kInvalidBlockLabel) {
    const std::string_view structured_label = bir_names.block_labels.spelling(block_label_id);
    if (!structured_label.empty()) {
      return names.block_labels.intern(structured_label);
    }
  }
  return names.block_labels.intern(raw_label);
}

[[nodiscard]] std::optional<BlockLabelId> resolve_preferred_existing_block_label(
    const PreparedNameTables& names,
    const bir::NameTables& bir_names,
    BlockLabelId block_label_id,
    std::string_view raw_label) {
  if (block_label_id != kInvalidBlockLabel) {
    const std::string_view structured_label = bir_names.block_labels.spelling(block_label_id);
    if (!structured_label.empty()) {
      const BlockLabelId prepared_label_id = names.block_labels.find(structured_label);
      if (prepared_label_id == kInvalidBlockLabel) {
        return std::nullopt;
      }
      return prepared_label_id;
    }
  }
  return resolve_prepared_block_label_id(names, raw_label);
}

[[nodiscard]] std::vector<BlockLabelId> build_function_block_label_ids(
    PreparedNameTables& names,
    const bir::NameTables& bir_names,
    const bir::Function& function) {
  std::vector<BlockLabelId> block_label_ids;
  block_label_ids.reserve(function.blocks.size());
  for (const auto& block : function.blocks) {
    block_label_ids.push_back(
        intern_preferred_block_label(names, bir_names, block.label_id, block.label));
  }
  return block_label_ids;
}

[[nodiscard]] std::unordered_map<BlockLabelId, std::size_t> build_block_index_map(
    const std::vector<BlockLabelId>& block_label_ids) {
  std::unordered_map<BlockLabelId, std::size_t> indices;
  indices.reserve(block_label_ids.size());
  for (std::size_t block_index = 0; block_index < block_label_ids.size(); ++block_index) {
    const BlockLabelId block_label_id = block_label_ids[block_index];
    if (block_label_id != kInvalidBlockLabel) {
      indices.emplace(block_label_id, block_index);
    }
  }
  return indices;
}

[[nodiscard]] std::vector<std::vector<std::size_t>> build_successors(
    const PreparedNameTables& names,
    const bir::NameTables& bir_names,
    const bir::Function& function,
    const std::unordered_map<BlockLabelId, std::size_t>& block_indices) {
  std::vector<std::vector<std::size_t>> successors(function.blocks.size());
  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& terminator = function.blocks[block_index].terminator;
    auto append_successor = [&](BlockLabelId label_id, std::string_view label) {
      const auto block_label_id =
          resolve_preferred_existing_block_label(names, bir_names, label_id, label);
      if (!block_label_id.has_value()) {
        return;
      }
      const auto found = block_indices.find(*block_label_id);
      if (found != block_indices.end()) {
        successors[block_index].push_back(found->second);
      }
    };

    if (terminator.kind == bir::TerminatorKind::Branch) {
      append_successor(terminator.target_label_id, terminator.target_label);
    } else if (terminator.kind == bir::TerminatorKind::CondBranch) {
      append_successor(terminator.true_label_id, terminator.true_label);
      append_successor(terminator.false_label_id, terminator.false_label);
    }
    sort_and_unique(successors[block_index]);
  }
  return successors;
}

[[nodiscard]] std::vector<std::vector<std::size_t>> build_predecessors(
    const std::vector<std::vector<std::size_t>>& successors) {
  std::vector<std::vector<std::size_t>> predecessors(successors.size());
  for (std::size_t block_index = 0; block_index < successors.size(); ++block_index) {
    for (const std::size_t successor : successors[block_index]) {
      predecessors[successor].push_back(block_index);
    }
  }
  return predecessors;
}

[[nodiscard]] std::vector<std::vector<std::size_t>> collect_phi_predecessor_uses(
    const PreparedNameTables& names,
    const bir::NameTables& bir_names,
    const bir::Function& function,
    const DenseValueSet& dense_values,
    const std::unordered_map<BlockLabelId, std::size_t>& block_indices) {
  std::vector<std::vector<std::size_t>> phi_uses(function.blocks.size());
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      const auto* phi = std::get_if<bir::PhiInst>(&inst);
      if (phi == nullptr) {
        continue;
      }
      for (const auto& incoming : phi->incomings) {
        const auto predecessor_label_id = resolve_preferred_existing_block_label(
            names, bir_names, incoming.label_id, incoming.label);
        const auto value_name_id = resolve_named_value_id(names, incoming.value);
        if (!predecessor_label_id.has_value() || !value_name_id.has_value()) {
          continue;
        }
        const auto pred_it = block_indices.find(*predecessor_label_id);
        const auto value_it = dense_values.name_to_index.find(*value_name_id);
        if (pred_it == block_indices.end() || value_it == dense_values.name_to_index.end()) {
          continue;
        }
        phi_uses[pred_it->second].push_back(value_it->second);
      }
    }
  }
  for (auto& uses : phi_uses) {
    sort_and_unique(uses);
  }
  return phi_uses;
}

[[nodiscard]] std::vector<std::size_t> compute_loop_depth(
    const std::vector<std::vector<std::size_t>>& successors) {
  const std::size_t block_count = successors.size();
  std::vector<std::size_t> loop_depth(block_count, 0);
  if (block_count == 0) {
    return loop_depth;
  }

  const auto predecessors = build_predecessors(successors);
  std::vector<bool> visited(block_count, false);
  std::vector<bool> on_stack(block_count, false);
  std::vector<std::pair<std::size_t, std::size_t>> back_edges;

  std::function<void(std::size_t)> dfs = [&](std::size_t block_index) {
    visited[block_index] = true;
    on_stack[block_index] = true;
    for (const std::size_t successor : successors[block_index]) {
      if (!visited[successor]) {
        dfs(successor);
      } else if (on_stack[successor]) {
        back_edges.emplace_back(block_index, successor);
      }
    }
    on_stack[block_index] = false;
  };
  dfs(0);

  for (const auto& [latch, header] : back_edges) {
    std::vector<bool> in_loop(block_count, false);
    std::queue<std::size_t> worklist;
    in_loop[header] = true;
    in_loop[latch] = true;
    worklist.push(latch);

    while (!worklist.empty()) {
      const std::size_t block_index = worklist.front();
      worklist.pop();
      for (const std::size_t predecessor : predecessors[block_index]) {
        if (!in_loop[predecessor]) {
          in_loop[predecessor] = true;
          worklist.push(predecessor);
        }
      }
    }

    for (std::size_t block_index = 0; block_index < block_count; ++block_index) {
      if (in_loop[block_index]) {
        ++loop_depth[block_index];
      }
    }
  }

  return loop_depth;
}

[[nodiscard]] ProgramPointState assign_program_points(
    const bir::Function& function,
    const PreparedNameTables& names,
    const DenseValueSet& dense_values,
    const std::vector<std::vector<std::size_t>>& phi_predecessor_uses) {
  const std::size_t value_count = dense_values.values.size();
  ProgramPointState state{
      .blocks = {},
      .def_points = std::vector<std::optional<std::size_t>>(value_count),
      .use_points = std::vector<std::vector<std::size_t>>(value_count),
      .first_points = std::vector<std::size_t>(value_count, kNoPoint),
      .last_points = std::vector<std::size_t>(value_count, 0),
      .seen_activity = std::vector<bool>(value_count, false),
      .call_points = {},
      .total_points = 0,
  };
  state.blocks.reserve(function.blocks.size());

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    state.blocks.push_back(BlockProgramPoints{
        .start_point = state.total_points,
        .end_point = state.total_points,
        .gen = BitSet(value_count),
        .kill = BitSet(value_count),
    });
    auto& block_state = state.blocks.back();

    if (block_index == 0) {
      for (std::size_t dense_index = 0; dense_index < dense_values.values.size(); ++dense_index) {
        if (dense_values.values[dense_index].value_kind != PreparedValueKind::Parameter) {
          continue;
        }
        state.seen_activity[dense_index] = true;
        state.def_points[dense_index] = block_state.start_point;
        state.first_points[dense_index] = block_state.start_point;
        state.last_points[dense_index] = block_state.start_point;
        block_state.kill.insert(dense_index);
      }
    }

    for (const auto& inst : function.blocks[block_index].insts) {
      std::vector<std::size_t> uses;
      std::vector<std::size_t> defs;
      collect_instruction_uses_and_defs(names, inst, dense_values, uses, defs);
      record_point_activity(uses,
                            defs,
                            state.total_points,
                            state.def_points,
                            state.use_points,
                            state.first_points,
                            state.last_points,
                            state.seen_activity,
                            block_state.gen,
                            block_state.kill);
      if (std::holds_alternative<bir::CallInst>(inst)) {
        state.call_points.push_back(state.total_points);
      }
      ++state.total_points;
    }

    std::vector<std::size_t> terminator_uses;
    collect_terminator_uses(names, function.blocks[block_index].terminator, dense_values, terminator_uses);
    terminator_uses.insert(terminator_uses.end(),
                           phi_predecessor_uses[block_index].begin(),
                           phi_predecessor_uses[block_index].end());
    sort_and_unique(terminator_uses);
    record_point_activity(terminator_uses,
                          {},
                          state.total_points,
                          state.def_points,
                          state.use_points,
                          state.first_points,
                          state.last_points,
                          state.seen_activity,
                          block_state.gen,
                          block_state.kill);
    block_state.end_point = state.total_points;
    ++state.total_points;
  }

  return state;
}

[[nodiscard]] std::pair<std::vector<BitSet>, std::vector<BitSet>> run_backward_dataflow(
    const std::vector<std::vector<std::size_t>>& successors,
    const std::vector<BlockProgramPoints>& block_points,
    std::size_t value_count) {
  std::vector<BitSet> live_in;
  std::vector<BitSet> live_out;
  live_in.reserve(block_points.size());
  live_out.reserve(block_points.size());
  for (std::size_t block_index = 0; block_index < block_points.size(); ++block_index) {
    live_in.emplace_back(value_count);
    live_out.emplace_back(value_count);
  }

  bool changed = true;
  while (changed) {
    changed = false;
    for (std::size_t reverse_index = block_points.size(); reverse_index > 0; --reverse_index) {
      const std::size_t block_index = reverse_index - 1U;
      BitSet next_live_out(value_count);
      for (const std::size_t successor : successors[block_index]) {
        (void)next_live_out.union_with(live_in[successor]);
      }
      changed = live_out[block_index].assign(next_live_out) || changed;
      changed = live_in[block_index].assign_gen_union_out_minus_kill(block_points[block_index].gen,
                                                                     live_out[block_index],
                                                                     block_points[block_index].kill) ||
                changed;
    }
  }

  return {std::move(live_in), std::move(live_out)};
}

[[nodiscard]] std::vector<PreparedValueId> decode_live_set(const BitSet& set,
                                                           const std::vector<PreparedValueId>& dense_value_ids) {
  std::vector<PreparedValueId> decoded;
  set.for_each_set_bit([&](std::size_t dense_index) {
    decoded.push_back(dense_value_ids[dense_index]);
  });
  return decoded;
}

[[nodiscard]] std::vector<PreparedLivenessBlock> build_prepared_blocks(
    const std::vector<BlockLabelId>& block_label_ids,
    const std::vector<BlockProgramPoints>& block_points,
    const std::vector<std::vector<std::size_t>>& successors,
    const std::vector<std::vector<std::size_t>>& predecessors,
    const std::vector<BitSet>& live_in,
    const std::vector<BitSet>& live_out,
    const std::vector<PreparedValueId>& dense_value_ids) {
  std::vector<PreparedLivenessBlock> blocks;
  blocks.reserve(block_label_ids.size());
  for (std::size_t block_index = 0; block_index < block_label_ids.size(); ++block_index) {
    blocks.push_back(PreparedLivenessBlock{
        .block_name = block_label_ids[block_index],
        .block_index = block_index,
        .start_point = block_points[block_index].start_point,
        .end_point = block_points[block_index].end_point,
        .predecessor_block_indices = predecessors[block_index],
        .successor_block_indices = successors[block_index],
        .live_in = decode_live_set(live_in[block_index], dense_value_ids),
        .live_out = decode_live_set(live_out[block_index], dense_value_ids),
    });
  }
  return blocks;
}

void extend_intervals_from_liveness(const std::vector<BlockProgramPoints>& block_points,
                                    const std::vector<BitSet>& live_in,
                                    const std::vector<BitSet>& live_out,
                                    std::vector<std::size_t>& first_points,
                                    std::vector<std::size_t>& last_points,
                                    std::vector<bool>& seen_activity) {
  for (std::size_t block_index = 0; block_index < block_points.size(); ++block_index) {
    const auto& block = block_points[block_index];
    auto extend_value = [&](std::size_t dense_index) {
      seen_activity[dense_index] = true;
      first_points[dense_index] = std::min(first_points[dense_index], block.start_point);
      last_points[dense_index] = std::max(last_points[dense_index], block.end_point);
    };
    live_in[block_index].for_each_set_bit(extend_value);
    live_out[block_index].for_each_set_bit(extend_value);
  }
}

[[nodiscard]] PreparedLivenessValue build_liveness_value(const DenseValueInfo& info,
                                                         FunctionNameId function_name,
                                                         PreparedValueId value_id,
                                                         std::optional<std::size_t> definition_point,
                                                         std::vector<std::size_t> use_points,
                                                         std::optional<PreparedLiveInterval> interval,
                                                         const std::vector<std::size_t>& call_points) {
  PreparedLivenessValue value{
      .value_id = value_id,
      .stack_object_id = info.stack_object_id,
      .function_name = function_name,
      .value_name = info.value_name,
      .type = info.type,
      .value_kind = info.value_kind,
      .address_taken = info.address_taken,
      .requires_home_slot = info.requires_home_slot,
      .crosses_call = false,
      .definition_point = definition_point,
      .use_points = std::move(use_points),
      .live_interval = std::move(interval),
  };

  if (value.live_interval.has_value()) {
    value.crosses_call =
        std::any_of(call_points.begin(),
                    call_points.end(),
                    [&](std::size_t call_point) {
                      return call_point > value.live_interval->start_point &&
                             call_point < value.live_interval->end_point;
                    });
  }

  return value;
}

}  // namespace

void BirPreAlloc::run_liveness() {
  prepared_.completed_phases.push_back("liveness");
  prepared_.liveness.functions.clear();
  prepared_.liveness.functions.reserve(prepared_.module.functions.size());

  PreparedValueId next_value_id = 0;
  for (const auto& function : prepared_.module.functions) {
    if (function.is_declaration) {
      continue;
    }

    const DenseValueSet dense_values =
        build_dense_value_set(prepared_.names, function, prepared_.stack_layout);
    const auto block_label_ids =
        build_function_block_label_ids(prepared_.names, prepared_.module.names, function);
    const auto block_indices = build_block_index_map(block_label_ids);
    const auto successors =
        build_successors(prepared_.names, prepared_.module.names, function, block_indices);
    const auto predecessors = build_predecessors(successors);
    const auto phi_predecessor_uses = collect_phi_predecessor_uses(
        prepared_.names, prepared_.module.names, function, dense_values, block_indices);
    ProgramPointState points =
        assign_program_points(function, prepared_.names, dense_values, phi_predecessor_uses);
    const auto [live_in, live_out] =
        run_backward_dataflow(successors, points.blocks, dense_values.values.size());
    extend_intervals_from_liveness(points.blocks,
                                   live_in,
                                   live_out,
                                   points.first_points,
                                   points.last_points,
                                   points.seen_activity);

    std::vector<PreparedValueId> dense_value_ids;
    dense_value_ids.reserve(dense_values.values.size());
    for (std::size_t dense_index = 0; dense_index < dense_values.values.size(); ++dense_index) {
      dense_value_ids.push_back(next_value_id + dense_index);
    }

    const FunctionNameId function_name_id =
        prepared_.names.function_names.intern(function.name);
    PreparedLivenessFunction prepared_function{
        .function_name = function_name_id,
        .instruction_count = points.total_points,
        .intervals = {},
        .call_points = std::move(points.call_points),
        .block_loop_depth = compute_loop_depth(successors),
        .blocks = build_prepared_blocks(
            block_label_ids,
            points.blocks,
            successors,
            predecessors,
            live_in,
            live_out,
            dense_value_ids),
        .values = {},
    };
    prepared_function.values.reserve(dense_values.values.size());
    prepared_function.intervals.reserve(dense_values.values.size());

    for (std::size_t dense_index = 0; dense_index < dense_values.values.size(); ++dense_index) {
      std::optional<PreparedLiveInterval> interval;
      if (points.seen_activity[dense_index] && points.first_points[dense_index] != kNoPoint) {
        interval = PreparedLiveInterval{
            .value_id = dense_value_ids[dense_index],
            .start_point = points.first_points[dense_index],
            .end_point = std::max(points.first_points[dense_index], points.last_points[dense_index]),
        };
      }
      auto value =
          build_liveness_value(dense_values.values[dense_index],
                               function_name_id,
                               dense_value_ids[dense_index],
                               points.def_points[dense_index],
                               points.use_points[dense_index],
                               interval,
                               prepared_function.call_points);
      if (value.live_interval.has_value()) {
        prepared_function.intervals.push_back(*value.live_interval);
      }
      prepared_function.values.push_back(std::move(value));
    }
    next_value_id += dense_values.values.size();

    std::sort(prepared_function.intervals.begin(),
              prepared_function.intervals.end(),
              [](const PreparedLiveInterval& lhs, const PreparedLiveInterval& rhs) {
                if (lhs.start_point != rhs.start_point) {
                  return lhs.start_point < rhs.start_point;
                }
                if (lhs.end_point != rhs.end_point) {
                  return lhs.end_point < rhs.end_point;
                }
                return lhs.value_id < rhs.value_id;
              });

    prepared_.liveness.functions.push_back(std::move(prepared_function));
  }

  prepared_.notes.push_back(PrepareNote{
      .phase = "liveness",
      .message =
          "liveness now computes BIR named-value intervals, exact def/use program points, "
          "predecessor-edge phi uses, call points, CFG-aware live-through extension, and loop depth "
          "independently of stack-layout object identity",
  });
}

}  // namespace c4c::backend::prepare
