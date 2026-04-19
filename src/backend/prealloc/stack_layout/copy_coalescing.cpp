#include "support.hpp"

#include <string_view>
#include <unordered_map>
#include <variant>

namespace c4c::backend::prepare::stack_layout {

namespace {

struct SlotAccessSummary {
  std::size_t store_count = 0;
  std::size_t load_count = 0;
  std::size_t first_store_block = 0;
  std::size_t first_load_block = 0;
  bool saw_store = false;
  bool saw_load = false;
};

[[nodiscard]] std::unordered_map<std::string_view, SlotAccessSummary> summarize_slot_accesses(
    const bir::Function& function) {
  std::unordered_map<std::string_view, SlotAccessSummary> summaries;

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (const auto& inst : block.insts) {
      if (const auto* store = std::get_if<bir::StoreLocalInst>(&inst); store != nullptr) {
        auto& summary = summaries[store->slot_name];
        ++summary.store_count;
        if (!summary.saw_store) {
          summary.first_store_block = block_index;
          summary.saw_store = true;
        }
        continue;
      }

      if (const auto* load = std::get_if<bir::LoadLocalInst>(&inst); load != nullptr) {
        auto& summary = summaries[load->slot_name];
        ++summary.load_count;
        if (!summary.saw_load) {
          summary.first_load_block = block_index;
          summary.saw_load = true;
        }
      }
    }
  }

  return summaries;
}

[[nodiscard]] bool is_copy_coalescing_candidate(
    const bir::LocalSlot& slot,
    const std::unordered_map<std::string_view, SlotAccessSummary>& access_summaries) {
  if (slot.storage_kind != bir::LocalSlotStorageKind::LoweringScratch || slot.is_address_taken ||
      slot.is_byval_copy) {
    return false;
  }

  const auto it = access_summaries.find(slot.name);
  if (it == access_summaries.end()) {
    return false;
  }

  const SlotAccessSummary& summary = it->second;
  if (summary.store_count != 1 || summary.load_count != 1) {
    return false;
  }

  // Keep the first-pass hint local to one block; cross-block aliasing needs real liveness data.
  return summary.first_store_block == summary.first_load_block;
}

}  // namespace

void apply_copy_coalescing_hints(const PreparedNameTables& names,
                                 const bir::Function& function,
                                 std::vector<PreparedStackObject>& objects) {
  if (objects.empty() || function.local_slots.empty()) {
    return;
  }

  const auto access_summaries = summarize_slot_accesses(function);
  std::unordered_map<std::string_view, bool> candidate_slots;
  candidate_slots.reserve(function.local_slots.size());

  for (const auto& slot : function.local_slots) {
    candidate_slots.emplace(slot.name, is_copy_coalescing_candidate(slot, access_summaries));
  }

  for (auto& object : objects) {
    if (!object.slot_name.has_value()) {
      continue;
    }
    const auto candidate_it = candidate_slots.find(prepared_slot_name(names, *object.slot_name));
    if (candidate_it == candidate_slots.end() || !candidate_it->second) {
      continue;
    }

    // This is only a metadata hint for the provisional C++ pipeline.
    object.source_kind = "copy_coalescing_candidate";
    object.requires_home_slot = false;
    object.address_exposed = false;
  }
}

}  // namespace c4c::backend::prepare::stack_layout
