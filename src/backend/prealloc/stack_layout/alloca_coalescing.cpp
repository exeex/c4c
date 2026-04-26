#include "stack_layout.hpp"

#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace c4c::backend::prepare::stack_layout {

namespace {

using SlotBlockSet = std::unordered_map<std::string_view, std::unordered_set<std::size_t>>;
using SlotNameSet = std::unordered_set<std::string_view>;
using RootNameSet = std::unordered_set<std::string_view>;
using PointerAliasMap = std::unordered_map<std::string_view, RootNameSet>;

struct SlotUseSummary {
  SlotBlockSet use_blocks;
  std::unordered_set<std::string_view> addressed_slots;
  std::unordered_set<std::string_view> direct_access_slots;
  std::unordered_set<std::string_view> sret_storage_slots;
};

struct BlockIndexLookup {
  std::unordered_map<BlockLabelId, std::size_t> by_id;
  std::unordered_map<std::string_view, std::size_t> by_raw_label;
};

[[nodiscard]] BlockIndexLookup build_block_index_lookup(const bir::Function& function) {
  BlockIndexLookup lookup;
  lookup.by_id.reserve(function.blocks.size());
  lookup.by_raw_label.reserve(function.blocks.size());

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    if (block.label_id != kInvalidBlockLabel) {
      lookup.by_id.emplace(block.label_id, block_index);
    }
    lookup.by_raw_label.emplace(block.label, block_index);
  }

  return lookup;
}

[[nodiscard]] std::optional<std::size_t> find_block_index(
    const BlockIndexLookup& lookup,
    BlockLabelId block_label_id,
    std::string_view raw_label) {
  if (block_label_id != kInvalidBlockLabel) {
    const auto by_id_it = lookup.by_id.find(block_label_id);
    if (by_id_it != lookup.by_id.end()) {
      return by_id_it->second;
    }
    return std::nullopt;
  }

  const auto by_raw_it = lookup.by_raw_label.find(raw_label);
  if (by_raw_it == lookup.by_raw_label.end()) {
    return std::nullopt;
  }
  return by_raw_it->second;
}

void record_root_pointer_use(const RootNameSet& roots,
                             std::size_t block_index,
                             SlotUseSummary& summary) {
  for (const std::string_view root_name : roots) {
    summary.use_blocks[root_name].insert(block_index);
  }
}

void record_root_pointer_escape(const RootNameSet& roots,
                                std::size_t block_index,
                                SlotUseSummary& summary) {
  record_root_pointer_use(roots, block_index, summary);
  for (const std::string_view root_name : roots) {
    summary.addressed_slots.insert(root_name);
  }
}

void merge_pointer_roots(const bir::Value& value,
                         const SlotNameSet& local_slot_names,
                         const PointerAliasMap& pointer_aliases,
                         RootNameSet& roots) {
  if (value.kind != bir::Value::Kind::Named || value.type != bir::TypeKind::Ptr ||
      value.name.empty()) {
    return;
  }

  if (const auto slot_it = local_slot_names.find(value.name); slot_it != local_slot_names.end()) {
    roots.insert(*slot_it);
  }

  if (const auto alias_it = pointer_aliases.find(value.name); alias_it != pointer_aliases.end()) {
    roots.insert(alias_it->second.begin(), alias_it->second.end());
  }
}

void record_local_slot_pointer_use(const bir::Value& value,
                                   std::size_t block_index,
                                   const SlotNameSet& local_slot_names,
                                   const PointerAliasMap& pointer_aliases,
                                   SlotUseSummary& summary) {
  RootNameSet roots;
  merge_pointer_roots(value, local_slot_names, pointer_aliases, roots);
  if (!roots.empty()) {
    record_root_pointer_use(roots, block_index, summary);
  }
}

void record_local_slot_pointer_escape(const bir::Value& value,
                                      std::size_t block_index,
                                      const SlotNameSet& local_slot_names,
                                      const PointerAliasMap& pointer_aliases,
                                      SlotUseSummary& summary) {
  RootNameSet roots;
  merge_pointer_roots(value, local_slot_names, pointer_aliases, roots);
  if (!roots.empty()) {
    record_root_pointer_escape(roots, block_index, summary);
    return;
  }

  record_local_slot_pointer_use(value, block_index, local_slot_names, pointer_aliases, summary);
}

void record_memory_address_use(const std::optional<bir::MemoryAddress>& address,
                               std::size_t block_index,
                               const SlotNameSet& local_slot_names,
                               const PointerAliasMap& pointer_aliases,
                               SlotUseSummary& summary) {
  if (!address.has_value()) {
    return;
  }

  if (address->base_kind == bir::MemoryAddress::BaseKind::LocalSlot && !address->base_name.empty()) {
    summary.use_blocks[address->base_name].insert(block_index);
    summary.addressed_slots.insert(address->base_name);
    return;
  }

  if (address->base_kind == bir::MemoryAddress::BaseKind::PointerValue) {
    record_local_slot_pointer_escape(
        address->base_value, block_index, local_slot_names, pointer_aliases, summary);
  }
}

[[nodiscard]] bool is_unrooted_pointer_value(const bir::Value& value,
                                             const SlotNameSet& local_slot_names,
                                             const PointerAliasMap& pointer_aliases) {
  if (value.kind != bir::Value::Kind::Named || value.type != bir::TypeKind::Ptr ||
      value.name.empty()) {
    return false;
  }

  if (local_slot_names.find(value.name) != local_slot_names.end()) {
    return false;
  }

  return pointer_aliases.find(value.name) == pointer_aliases.end();
}

void update_pointer_alias(const bir::Value& result,
                          const RootNameSet& roots,
                          PointerAliasMap& pointer_aliases) {
  if (result.kind != bir::Value::Kind::Named || result.type != bir::TypeKind::Ptr ||
      result.name.empty()) {
    return;
  }

  if (roots.empty()) {
    pointer_aliases.erase(result.name);
    return;
  }

  pointer_aliases[result.name] = roots;
}

void handle_single_input_pointer_transform(const bir::Value& operand,
                                           const bir::Value& result,
                                           std::size_t block_index,
                                           const SlotNameSet& local_slot_names,
                                           PointerAliasMap& pointer_aliases,
                                           SlotUseSummary& summary) {
  RootNameSet roots;
  merge_pointer_roots(operand, local_slot_names, pointer_aliases, roots);
  record_local_slot_pointer_use(
      operand, block_index, local_slot_names, pointer_aliases, summary);
  update_pointer_alias(result, roots, pointer_aliases);
}

void record_call_pointer_uses(const bir::CallInst& call,
                              std::size_t block_index,
                              const SlotNameSet& local_slot_names,
                              const PointerAliasMap& pointer_aliases,
                              SlotUseSummary& summary) {
  if (call.sret_storage_name.has_value()) {
    summary.use_blocks[*call.sret_storage_name].insert(block_index);
    summary.direct_access_slots.insert(*call.sret_storage_name);
    summary.sret_storage_slots.insert(*call.sret_storage_name);
  }
  if (call.callee_value.has_value()) {
    record_local_slot_pointer_escape(
        *call.callee_value, block_index, local_slot_names, pointer_aliases, summary);
  }
  for (const auto& arg : call.args) {
    record_local_slot_pointer_escape(arg, block_index, local_slot_names, pointer_aliases, summary);
  }
}

[[nodiscard]] SlotUseSummary collect_slot_use_summary(const bir::Function& function,
                                                      const SlotNameSet& local_slot_names) {
  SlotUseSummary summary;
  PointerAliasMap pointer_aliases;
  const BlockIndexLookup block_indices = build_block_index_lookup(function);

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (const auto& inst : block.insts) {
      if (const auto* load = std::get_if<bir::LoadLocalInst>(&inst); load != nullptr) {
        summary.use_blocks[load->slot_name].insert(block_index);
        summary.direct_access_slots.insert(load->slot_name);
        record_memory_address_use(
            load->address, block_index, local_slot_names, pointer_aliases, summary);
        continue;
      }
      if (const auto* store = std::get_if<bir::StoreLocalInst>(&inst); store != nullptr) {
        summary.use_blocks[store->slot_name].insert(block_index);
        summary.direct_access_slots.insert(store->slot_name);
        record_memory_address_use(
            store->address, block_index, local_slot_names, pointer_aliases, summary);
        record_local_slot_pointer_escape(
            store->value, block_index, local_slot_names, pointer_aliases, summary);
        continue;
      }
      if (const auto* load = std::get_if<bir::LoadGlobalInst>(&inst); load != nullptr) {
        record_memory_address_use(
            load->address, block_index, local_slot_names, pointer_aliases, summary);
        continue;
      }
      if (const auto* store = std::get_if<bir::StoreGlobalInst>(&inst); store != nullptr) {
        record_memory_address_use(
            store->address, block_index, local_slot_names, pointer_aliases, summary);
        record_local_slot_pointer_escape(
            store->value, block_index, local_slot_names, pointer_aliases, summary);
        continue;
      }
      if (const auto* call = std::get_if<bir::CallInst>(&inst); call != nullptr) {
        record_call_pointer_uses(
            *call, block_index, local_slot_names, pointer_aliases, summary);
        continue;
      }
      if (const auto* cast = std::get_if<bir::CastInst>(&inst); cast != nullptr) {
        handle_single_input_pointer_transform(cast->operand,
                                              cast->result,
                                              block_index,
                                              local_slot_names,
                                              pointer_aliases,
                                              summary);
        continue;
      }
      if (const auto* phi = std::get_if<bir::PhiInst>(&inst); phi != nullptr) {
        RootNameSet roots;
        bool saw_unrooted_pointer = false;
        for (const auto& incoming : phi->incomings) {
          const std::size_t incoming_block_index =
              find_block_index(block_indices, incoming.label_id, incoming.label)
                  .value_or(block_index);
          merge_pointer_roots(incoming.value, local_slot_names, pointer_aliases, roots);
          saw_unrooted_pointer |=
              is_unrooted_pointer_value(incoming.value, local_slot_names, pointer_aliases);
          record_local_slot_pointer_escape(
              incoming.value,
              incoming_block_index,
              local_slot_names,
              pointer_aliases,
              summary);
        }
        if (!roots.empty() && saw_unrooted_pointer) {
          record_root_pointer_escape(roots, block_index, summary);
        }
        update_pointer_alias(phi->result, roots, pointer_aliases);
        continue;
      }
      if (const auto* select = std::get_if<bir::SelectInst>(&inst); select != nullptr) {
        RootNameSet roots;
        bool saw_unrooted_pointer = false;
        merge_pointer_roots(select->lhs, local_slot_names, pointer_aliases, roots);
        merge_pointer_roots(select->rhs, local_slot_names, pointer_aliases, roots);
        merge_pointer_roots(select->true_value, local_slot_names, pointer_aliases, roots);
        merge_pointer_roots(select->false_value, local_slot_names, pointer_aliases, roots);
        saw_unrooted_pointer |=
            is_unrooted_pointer_value(select->lhs, local_slot_names, pointer_aliases);
        saw_unrooted_pointer |=
            is_unrooted_pointer_value(select->rhs, local_slot_names, pointer_aliases);
        saw_unrooted_pointer |=
            is_unrooted_pointer_value(select->true_value, local_slot_names, pointer_aliases);
        saw_unrooted_pointer |=
            is_unrooted_pointer_value(select->false_value, local_slot_names, pointer_aliases);
        if (select->compare_type == bir::TypeKind::Ptr) {
          record_local_slot_pointer_escape(
              select->lhs, block_index, local_slot_names, pointer_aliases, summary);
          record_local_slot_pointer_escape(
              select->rhs, block_index, local_slot_names, pointer_aliases, summary);
        } else {
          record_local_slot_pointer_use(
              select->lhs, block_index, local_slot_names, pointer_aliases, summary);
          record_local_slot_pointer_use(
              select->rhs, block_index, local_slot_names, pointer_aliases, summary);
        }
        record_local_slot_pointer_escape(
            select->true_value, block_index, local_slot_names, pointer_aliases, summary);
        record_local_slot_pointer_escape(
            select->false_value, block_index, local_slot_names, pointer_aliases, summary);
        if (!roots.empty() && saw_unrooted_pointer) {
          record_root_pointer_escape(roots, block_index, summary);
        }
        update_pointer_alias(select->result, roots, pointer_aliases);
        continue;
      }
      if (const auto* binary = std::get_if<bir::BinaryInst>(&inst); binary != nullptr) {
        RootNameSet roots;
        bool saw_unrooted_pointer = false;
        merge_pointer_roots(binary->lhs, local_slot_names, pointer_aliases, roots);
        merge_pointer_roots(binary->rhs, local_slot_names, pointer_aliases, roots);
        saw_unrooted_pointer |=
            is_unrooted_pointer_value(binary->lhs, local_slot_names, pointer_aliases);
        saw_unrooted_pointer |=
            is_unrooted_pointer_value(binary->rhs, local_slot_names, pointer_aliases);
        if (binary->operand_type == bir::TypeKind::Ptr) {
          record_local_slot_pointer_escape(
              binary->lhs, block_index, local_slot_names, pointer_aliases, summary);
          record_local_slot_pointer_escape(
              binary->rhs, block_index, local_slot_names, pointer_aliases, summary);
        } else {
          record_local_slot_pointer_use(
              binary->lhs, block_index, local_slot_names, pointer_aliases, summary);
          record_local_slot_pointer_use(
              binary->rhs, block_index, local_slot_names, pointer_aliases, summary);
        }
        if (!roots.empty() && saw_unrooted_pointer) {
          record_root_pointer_escape(roots, block_index, summary);
        }
        update_pointer_alias(binary->result, roots, pointer_aliases);
      }
    }

    switch (block.terminator.kind) {
      case bir::TerminatorKind::Return:
        if (block.terminator.value.has_value()) {
          record_local_slot_pointer_escape(*block.terminator.value,
                                           block_index,
                                           local_slot_names,
                                           pointer_aliases,
                                           summary);
        }
        break;
      case bir::TerminatorKind::CondBranch:
        record_local_slot_pointer_escape(block.terminator.condition,
                                         block_index,
                                         local_slot_names,
                                         pointer_aliases,
                                         summary);
        break;
      case bir::TerminatorKind::Branch:
        break;
    }
  }

  return summary;
}

}  // namespace

void apply_alloca_coalescing_hints(const PreparedNameTables& names,
                                   const bir::Function& function,
                                   std::vector<PreparedStackObject>& objects) {
  std::unordered_map<std::string_view, const bir::LocalSlot*> slots_by_name;
  slots_by_name.reserve(function.local_slots.size());
  for (const auto& slot : function.local_slots) {
    slots_by_name.emplace(slot.name, &slot);
  }

  SlotNameSet local_slot_names;
  local_slot_names.reserve(function.local_slots.size());
  for (const auto& slot : function.local_slots) {
    local_slot_names.insert(slot.name);
  }

  const SlotUseSummary use_summary = collect_slot_use_summary(function, local_slot_names);

  for (auto& object : objects) {
    if (!object.slot_name.has_value()) {
      continue;
    }
    const auto slot_it = slots_by_name.find(prepared_slot_name(names, *object.slot_name));
    if (slot_it == slots_by_name.end()) {
      continue;
    }

    const bir::LocalSlot& slot = *slot_it->second;
    const auto use_it = use_summary.use_blocks.find(slot.name);
    const bool has_explicit_uses = use_it != use_summary.use_blocks.end();
    const bool used_in_single_block = has_explicit_uses && use_it->second.size() <= 1;
    const bool has_addressed_local_uses =
        use_summary.addressed_slots.find(slot.name) != use_summary.addressed_slots.end();
    const bool has_direct_slot_accesses =
        use_summary.direct_access_slots.find(slot.name) != use_summary.direct_access_slots.end();
    const bool is_sret_storage_slot =
        use_summary.sret_storage_slots.find(slot.name) != use_summary.sret_storage_slots.end();
    const bool is_dead_local_slot =
        !has_explicit_uses && !has_addressed_local_uses && !slot.is_address_taken &&
        !slot.is_byval_copy && !slot.phi_observation.has_value();

    object.address_exposed =
        object.address_exposed || slot.is_address_taken || has_addressed_local_uses;
    const bool rooted_pointer_bookkeeping_only =
        has_explicit_uses && !has_direct_slot_accesses && !slot.is_address_taken &&
        !has_addressed_local_uses;

    object.requires_home_slot =
        !is_dead_local_slot &&
        (has_direct_slot_accesses || slot.is_address_taken || has_addressed_local_uses ||
         slot.is_byval_copy || (!used_in_single_block && !rooted_pointer_bookkeeping_only));
    if (is_sret_storage_slot) {
      object.source_kind = "call_result_sret";
      object.permanent_home_slot = true;
    }
  }
}

}  // namespace c4c::backend::prepare::stack_layout
