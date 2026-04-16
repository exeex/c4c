#include "support.hpp"

namespace c4c::backend::prepare::stack_layout {

namespace {

const bir::LocalSlot* find_local_slot(const bir::Function& function, std::string_view name) {
  for (const auto& slot : function.local_slots) {
    if (slot.name == name) {
      return &slot;
    }
  }
  return nullptr;
}

}  // namespace

void apply_regalloc_hints(const bir::Function& function,
                          const FunctionInlineAsmSummary& inline_asm_summary,
                          std::vector<PreparedStackObject>& objects) {
  for (auto& object : objects) {
    if (object.function_name != function.name) {
      continue;
    }

    bool address_exposed = object.address_exposed;
    bool requires_home_slot = object.requires_home_slot;
    bool permanent_home_slot = object.permanent_home_slot;

    if (const auto* slot = find_local_slot(function, object.source_name); slot != nullptr) {
      address_exposed = address_exposed || slot->is_address_taken;
      requires_home_slot = requires_home_slot || slot->is_address_taken;
      permanent_home_slot = permanent_home_slot || slot->is_address_taken;
    }
    permanent_home_slot = permanent_home_slot || address_exposed;

    if (inline_asm_summary.has_side_effects && address_exposed) {
      requires_home_slot = true;
      permanent_home_slot = true;
    }

    object.address_exposed = address_exposed;
    object.requires_home_slot = requires_home_slot;
    object.permanent_home_slot = permanent_home_slot;
  }
}

}  // namespace c4c::backend::prepare::stack_layout
