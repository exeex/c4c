#include "prealloc.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <queue>
#include <type_traits>
#include <unordered_map>
#include <variant>

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

  void clear() {
    std::fill(words_.begin(), words_.end(), 0);
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

struct DenseObjectSet {
  std::vector<const PreparedStackObject*> objects;
  std::unordered_map<std::string_view, std::vector<std::size_t>> name_to_indices;
};

struct BlockProgramPoints {
  std::size_t start_point = 0;
  std::size_t end_point = 0;
  BitSet gen;
  BitSet kill;
};

struct ProgramPointState {
  std::vector<BlockProgramPoints> blocks;
  std::vector<std::size_t> first_points;
  std::vector<std::size_t> last_points;
  std::vector<bool> seen_activity;
  std::vector<std::size_t> call_points;
  std::size_t total_points = 0;
};

[[nodiscard]] PreparedValueKind prepared_value_kind_from_source(std::string_view source_kind) {
  if (source_kind == "call_result_sret") {
    return PreparedValueKind::CallResult;
  }
  if (source_kind == "byval_param" || source_kind == "sret_param") {
    return PreparedValueKind::Parameter;
  }
  return PreparedValueKind::StackObject;
}

[[nodiscard]] bool is_entry_defined_object(const PreparedStackObject& object) {
  return object.source_kind == "byval_param" || object.source_kind == "sret_param";
}

[[nodiscard]] DenseObjectSet build_dense_object_set(const bir::Function& function,
                                                    const PreparedStackLayout& stack_layout) {
  DenseObjectSet dense;
  for (const auto& object : stack_layout.objects) {
    if (object.function_name != function.name) {
      continue;
    }
    dense.objects.push_back(&object);
    dense.name_to_indices[object.source_name].push_back(dense.objects.size() - 1U);
  }
  return dense;
}

void append_named_value_matches(const bir::Value& value,
                                const DenseObjectSet& dense_objects,
                                std::vector<std::size_t>& matches) {
  if (value.kind != bir::Value::Kind::Named) {
    return;
  }
  const auto found = dense_objects.name_to_indices.find(value.name);
  if (found == dense_objects.name_to_indices.end()) {
    return;
  }
  matches.insert(matches.end(), found->second.begin(), found->second.end());
}

void append_memory_address_matches(const std::optional<bir::MemoryAddress>& address,
                                   const DenseObjectSet& dense_objects,
                                   std::vector<std::size_t>& matches) {
  if (!address.has_value() || address->base_kind != bir::MemoryAddress::BaseKind::LocalSlot) {
    return;
  }
  const auto found = dense_objects.name_to_indices.find(address->base_name);
  if (found == dense_objects.name_to_indices.end()) {
    return;
  }
  matches.insert(matches.end(), found->second.begin(), found->second.end());
}

void append_named_definition_matches(const bir::Value& value,
                                     const DenseObjectSet& dense_objects,
                                     std::vector<std::size_t>& matches) {
  append_named_value_matches(value, dense_objects, matches);
}

void append_named_definition_matches(const std::optional<bir::Value>& value,
                                     const DenseObjectSet& dense_objects,
                                     std::vector<std::size_t>& matches) {
  if (!value.has_value()) {
    return;
  }
  append_named_value_matches(*value, dense_objects, matches);
}

void append_slot_definition_matches(std::string_view slot_name,
                                    const DenseObjectSet& dense_objects,
                                    std::vector<std::size_t>& matches) {
  const auto found = dense_objects.name_to_indices.find(slot_name);
  if (found == dense_objects.name_to_indices.end()) {
    return;
  }
  matches.insert(matches.end(), found->second.begin(), found->second.end());
}

void sort_and_unique(std::vector<std::size_t>& items) {
  std::sort(items.begin(), items.end());
  items.erase(std::unique(items.begin(), items.end()), items.end());
}

void collect_instruction_uses_and_defs(const bir::Inst& inst,
                                       const DenseObjectSet& dense_objects,
                                       std::vector<std::size_t>& uses,
                                       std::vector<std::size_t>& defs) {
  std::visit(
      [&](const auto& typed_inst) {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::BinaryInst>) {
          append_named_value_matches(typed_inst.lhs, dense_objects, uses);
          append_named_value_matches(typed_inst.rhs, dense_objects, uses);
          append_named_definition_matches(typed_inst.result, dense_objects, defs);
        } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
          append_named_value_matches(typed_inst.lhs, dense_objects, uses);
          append_named_value_matches(typed_inst.rhs, dense_objects, uses);
          append_named_value_matches(typed_inst.true_value, dense_objects, uses);
          append_named_value_matches(typed_inst.false_value, dense_objects, uses);
          append_named_definition_matches(typed_inst.result, dense_objects, defs);
        } else if constexpr (std::is_same_v<T, bir::CastInst>) {
          append_named_value_matches(typed_inst.operand, dense_objects, uses);
          append_named_definition_matches(typed_inst.result, dense_objects, defs);
        } else if constexpr (std::is_same_v<T, bir::PhiInst>) {
          for (const auto& incoming : typed_inst.incomings) {
            append_named_value_matches(incoming.value, dense_objects, uses);
          }
          append_named_definition_matches(typed_inst.result, dense_objects, defs);
        } else if constexpr (std::is_same_v<T, bir::CallInst>) {
          append_named_definition_matches(typed_inst.result, dense_objects, defs);
          append_named_value_matches(typed_inst.callee_value.value_or(bir::Value{}), dense_objects, uses);
          for (const auto& arg : typed_inst.args) {
            append_named_value_matches(arg, dense_objects, uses);
          }
          if (typed_inst.sret_storage_name.has_value()) {
            append_slot_definition_matches(*typed_inst.sret_storage_name, dense_objects, defs);
          }
        } else if constexpr (std::is_same_v<T, bir::LoadLocalInst>) {
          append_slot_definition_matches(typed_inst.slot_name, dense_objects, uses);
          append_memory_address_matches(typed_inst.address, dense_objects, uses);
          append_named_definition_matches(typed_inst.result, dense_objects, defs);
        } else if constexpr (std::is_same_v<T, bir::LoadGlobalInst>) {
          append_memory_address_matches(typed_inst.address, dense_objects, uses);
          append_named_definition_matches(typed_inst.result, dense_objects, defs);
        } else if constexpr (std::is_same_v<T, bir::StoreGlobalInst>) {
          append_named_value_matches(typed_inst.value, dense_objects, uses);
          append_memory_address_matches(typed_inst.address, dense_objects, uses);
        } else if constexpr (std::is_same_v<T, bir::StoreLocalInst>) {
          append_named_value_matches(typed_inst.value, dense_objects, uses);
          append_memory_address_matches(typed_inst.address, dense_objects, uses);
          append_slot_definition_matches(typed_inst.slot_name, dense_objects, defs);
        }
      },
      inst);
  sort_and_unique(uses);
  sort_and_unique(defs);
}

void collect_terminator_uses_and_defs(const bir::Terminator& terminator,
                                      const DenseObjectSet& dense_objects,
                                      std::vector<std::size_t>& uses,
                                      std::vector<std::size_t>& defs) {
  if (terminator.kind == bir::TerminatorKind::Return && terminator.value.has_value()) {
    append_named_value_matches(*terminator.value, dense_objects, uses);
  } else if (terminator.kind == bir::TerminatorKind::CondBranch) {
    append_named_value_matches(terminator.condition, dense_objects, uses);
  }
  sort_and_unique(uses);
  sort_and_unique(defs);
}

void record_point_activity(const std::vector<std::size_t>& uses,
                           const std::vector<std::size_t>& defs,
                           std::size_t point,
                           std::vector<std::size_t>& first_points,
                           std::vector<std::size_t>& last_points,
                           std::vector<bool>& seen_activity,
                           BitSet& gen,
                           BitSet& kill) {
  for (const std::size_t dense_index : uses) {
    seen_activity[dense_index] = true;
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
    if (first_points[dense_index] == kNoPoint) {
      first_points[dense_index] = point;
    }
    last_points[dense_index] = std::max(last_points[dense_index], point);
    kill.insert(dense_index);
  }
}

[[nodiscard]] std::unordered_map<std::string, std::size_t> build_block_index_map(
    const bir::Function& function) {
  std::unordered_map<std::string, std::size_t> indices;
  indices.reserve(function.blocks.size());
  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    indices.emplace(function.blocks[block_index].label, block_index);
  }
  return indices;
}

[[nodiscard]] std::vector<std::vector<std::size_t>> build_successors(
    const bir::Function& function,
    const std::unordered_map<std::string, std::size_t>& block_indices) {
  std::vector<std::vector<std::size_t>> successors(function.blocks.size());
  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& terminator = function.blocks[block_index].terminator;
    auto append_successor = [&](std::string_view label) {
      const auto found = block_indices.find(std::string(label));
      if (found != block_indices.end()) {
        successors[block_index].push_back(found->second);
      }
    };

    if (terminator.kind == bir::TerminatorKind::Branch) {
      append_successor(terminator.target_label);
    } else if (terminator.kind == bir::TerminatorKind::CondBranch) {
      append_successor(terminator.true_label);
      append_successor(terminator.false_label);
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

[[nodiscard]] ProgramPointState assign_program_points(const bir::Function& function,
                                                      const DenseObjectSet& dense_objects) {
  const std::size_t value_count = dense_objects.objects.size();
  ProgramPointState state{
      .blocks = {},
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
      for (std::size_t dense_index = 0; dense_index < dense_objects.objects.size(); ++dense_index) {
        if (!is_entry_defined_object(*dense_objects.objects[dense_index])) {
          continue;
        }
        state.first_points[dense_index] = block_state.start_point;
        state.last_points[dense_index] = block_state.start_point;
        state.seen_activity[dense_index] = true;
        block_state.kill.insert(dense_index);
      }
    }

    for (const auto& inst : function.blocks[block_index].insts) {
      std::vector<std::size_t> uses;
      std::vector<std::size_t> defs;
      collect_instruction_uses_and_defs(inst, dense_objects, uses, defs);
      record_point_activity(uses,
                            defs,
                            state.total_points,
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

    std::vector<std::size_t> term_uses;
    std::vector<std::size_t> term_defs;
    collect_terminator_uses_and_defs(function.blocks[block_index].terminator,
                                     dense_objects,
                                     term_uses,
                                     term_defs);
    record_point_activity(term_uses,
                          term_defs,
                          state.total_points,
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

[[nodiscard]] PreparedLivenessValue build_liveness_value(const PreparedStackObject& object,
                                                         PreparedValueId value_id,
                                                         std::optional<PreparedLiveInterval> interval,
                                                         const std::vector<std::size_t>& call_points) {
  PreparedLivenessValue value{
      .value_id = value_id,
      .stack_object_id = object.object_id,
      .function_name = object.function_name,
      .source_name = object.source_name,
      .source_kind = object.source_kind,
      .type = object.type,
      .value_kind = prepared_value_kind_from_source(object.source_kind),
      .address_taken = object.address_exposed,
      .requires_home_slot = object.requires_home_slot,
      .crosses_call = false,
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

    const DenseObjectSet dense_objects = build_dense_object_set(function, prepared_.stack_layout);
    const auto block_indices = build_block_index_map(function);
    const auto successors = build_successors(function, block_indices);
    ProgramPointState points = assign_program_points(function, dense_objects);
    const auto [live_in, live_out] =
        run_backward_dataflow(successors, points.blocks, dense_objects.objects.size());
    extend_intervals_from_liveness(points.blocks,
                                   live_in,
                                   live_out,
                                   points.first_points,
                                   points.last_points,
                                   points.seen_activity);

    PreparedLivenessFunction prepared_function{
        .function_name = function.name,
        .instruction_count = points.total_points,
        .intervals = {},
        .call_points = std::move(points.call_points),
        .block_loop_depth = compute_loop_depth(successors),
        .values = {},
    };
    prepared_function.values.reserve(dense_objects.objects.size());
    prepared_function.intervals.reserve(dense_objects.objects.size());

    for (std::size_t dense_index = 0; dense_index < dense_objects.objects.size(); ++dense_index) {
      std::optional<PreparedLiveInterval> interval;
      if (points.seen_activity[dense_index] && points.first_points[dense_index] != kNoPoint) {
        interval = PreparedLiveInterval{
            .value_id = next_value_id,
            .start_point = points.first_points[dense_index],
            .end_point = std::max(points.first_points[dense_index], points.last_points[dense_index]),
        };
      }
      auto value = build_liveness_value(*dense_objects.objects[dense_index],
                                        next_value_id++,
                                        interval,
                                        prepared_function.call_points);
      if (value.live_interval.has_value()) {
        prepared_function.intervals.push_back(*value.live_interval);
      }
      prepared_function.values.push_back(std::move(value));
    }

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
          "liveness now computes program-point intervals, call points, CFG-aware live-through "
          "extension, and approximate loop depth for prepared values",
  });
}

}  // namespace c4c::backend::prepare
