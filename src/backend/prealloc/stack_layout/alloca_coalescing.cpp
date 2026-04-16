#include "support.hpp"

#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace c4c::backend::prepare::stack_layout {

namespace {

using SlotBlockSet = std::unordered_map<std::string_view, std::unordered_set<std::size_t>>;

struct SlotUseSummary {
  SlotBlockSet use_blocks;
  std::unordered_set<std::string_view> addressed_slots;
};

void record_addressed_local_slot_use(const std::optional<bir::MemoryAddress>& address,
                                     std::size_t block_index,
                                     SlotUseSummary& summary) {
  if (!address.has_value() ||
      address->base_kind != bir::MemoryAddress::BaseKind::LocalSlot ||
      address->base_name.empty()) {
    return;
  }

  summary.use_blocks[address->base_name].insert(block_index);
  summary.addressed_slots.insert(address->base_name);
}

[[nodiscard]] SlotUseSummary collect_slot_use_summary(const bir::Function& function) {
  SlotUseSummary summary;

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (const auto& inst : block.insts) {
      if (const auto* load = std::get_if<bir::LoadLocalInst>(&inst); load != nullptr) {
        summary.use_blocks[load->slot_name].insert(block_index);
        record_addressed_local_slot_use(load->address, block_index, summary);
        continue;
      }
      if (const auto* store = std::get_if<bir::StoreLocalInst>(&inst); store != nullptr) {
        summary.use_blocks[store->slot_name].insert(block_index);
        record_addressed_local_slot_use(store->address, block_index, summary);
        continue;
      }
      if (const auto* load = std::get_if<bir::LoadGlobalInst>(&inst); load != nullptr) {
        record_addressed_local_slot_use(load->address, block_index, summary);
        continue;
      }
      if (const auto* store = std::get_if<bir::StoreGlobalInst>(&inst); store != nullptr) {
        record_addressed_local_slot_use(store->address, block_index, summary);
        continue;
      }
      if (const auto* call = std::get_if<bir::CallInst>(&inst);
          call != nullptr && call->sret_storage_name.has_value()) {
        summary.use_blocks[*call->sret_storage_name].insert(block_index);
      }
    }
  }

  return summary;
}

}  // namespace

void apply_alloca_coalescing_hints(const bir::Function& function,
                                   std::vector<PreparedStackObject>& objects) {
  std::unordered_map<std::string_view, const bir::LocalSlot*> slots_by_name;
  slots_by_name.reserve(function.local_slots.size());
  for (const auto& slot : function.local_slots) {
    slots_by_name.emplace(slot.name, &slot);
  }

  const SlotUseSummary use_summary = collect_slot_use_summary(function);

  for (auto& object : objects) {
    const auto slot_it = slots_by_name.find(object.source_name);
    if (slot_it == slots_by_name.end()) {
      continue;
    }

    const bir::LocalSlot& slot = *slot_it->second;
    const auto use_it = use_summary.use_blocks.find(slot.name);
    const bool has_explicit_uses = use_it != use_summary.use_blocks.end();
    const bool used_in_single_block = has_explicit_uses && use_it->second.size() <= 1;
    const bool has_addressed_local_uses =
        use_summary.addressed_slots.find(slot.name) != use_summary.addressed_slots.end();
    const bool is_dead_local_slot =
        !has_explicit_uses && !has_addressed_local_uses && !slot.is_address_taken &&
        !slot.is_byval_copy &&
        slot.storage_kind == bir::LocalSlotStorageKind::None &&
        !slot.phi_observation.has_value();

    object.address_exposed =
        object.address_exposed || slot.is_address_taken || has_addressed_local_uses;
    object.requires_home_slot =
        !is_dead_local_slot &&
        (object.requires_home_slot || slot.is_address_taken || has_addressed_local_uses ||
         slot.is_byval_copy || slot.storage_kind == bir::LocalSlotStorageKind::LoweringScratch ||
         !used_in_single_block);
  }
}

}  // namespace c4c::backend::prepare::stack_layout
