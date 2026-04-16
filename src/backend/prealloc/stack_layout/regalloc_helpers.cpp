#include "support.hpp"

#include <string_view>

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

bool source_kind_requires_home_slot(std::string_view source_kind) {
  return source_kind == "byval_param" || source_kind == "sret_param" ||
         source_kind == "call_result_sret" || source_kind == "byval_copy" ||
         source_kind == "phi";
}

bool source_kind_requires_permanent_home_slot(std::string_view source_kind) {
  return source_kind == "byval_param" || source_kind == "sret_param" ||
         source_kind == "call_result_sret" || source_kind == "byval_copy" ||
         source_kind == "phi";
}

bool source_kind_elides_dedicated_home_slot(std::string_view source_kind) {
  return source_kind == "copy_coalescing_candidate";
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
    bool requires_home_slot =
        object.requires_home_slot || source_kind_requires_home_slot(object.source_kind);
    bool permanent_home_slot =
        object.permanent_home_slot || source_kind_requires_permanent_home_slot(object.source_kind);
    const bool elides_dedicated_home_slot = source_kind_elides_dedicated_home_slot(object.source_kind);

    if (const auto* slot = find_local_slot(function, object.source_name); slot != nullptr) {
      address_exposed = address_exposed || slot->is_address_taken;
      requires_home_slot = requires_home_slot || slot->is_address_taken ||
                           (slot->storage_kind == bir::LocalSlotStorageKind::LoweringScratch &&
                            !elides_dedicated_home_slot);
      permanent_home_slot =
          permanent_home_slot || slot->is_address_taken ||
          (slot->storage_kind == bir::LocalSlotStorageKind::LoweringScratch &&
           !elides_dedicated_home_slot);
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
