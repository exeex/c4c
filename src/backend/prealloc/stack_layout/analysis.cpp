#include "../module.hpp"
#include "../variadic.hpp"
#include "stack_layout.hpp"

#include <string_view>
#include <unordered_set>
#include <vector>

namespace c4c::backend::prepare::stack_layout {

namespace {

[[nodiscard]] std::string_view local_slot_source_kind(const bir::LocalSlot& slot) {
  if (slot.phi_observation.has_value()) {
    return "phi";
  }
  if (slot.is_byval_copy) {
    return "byval_copy";
  }
  if (slot.storage_kind == bir::LocalSlotStorageKind::LoweringScratch) {
    return "lowering_scratch";
  }
  return "local_slot";
}

[[nodiscard]] bool source_kind_requires_permanent_home_slot(std::string_view source_kind) {
  return source_kind == "byval_copy" || source_kind == "phi";
}

[[nodiscard]] bool local_slot_has_explicit_home_slot_contract(const bir::LocalSlot& slot,
                                                              std::string_view source_kind) {
  return slot.is_address_taken || source_kind_requires_permanent_home_slot(source_kind);
}

[[nodiscard]] PreparedStackObject make_local_slot_object(const bir::Function& function,
                                                         PreparedNameTables& names,
                                                         const bir::NameTables& bir_names,
                                                         const bir::LocalSlot& slot,
                                                         PreparedObjectId object_id) {
  const std::string_view source_kind = local_slot_source_kind(slot);
  const bool has_explicit_home_slot_contract =
      local_slot_has_explicit_home_slot_contract(slot, source_kind);
  return PreparedStackObject{
      .object_id = object_id,
      .function_name = names.function_names.intern(function.name),
      .slot_name = intern_prepared_slot_name(names, bir_names, slot.slot_id, slot.name),
      .source_kind = std::string(source_kind),
      .type = slot.type,
      .size_bytes = slot.size_bytes,
      .align_bytes = slot.align_bytes,
      .address_exposed = slot.is_address_taken,
      .requires_home_slot = has_explicit_home_slot_contract,
      .permanent_home_slot = has_explicit_home_slot_contract,
  };
}

[[nodiscard]] std::optional<PreparedStackObject> make_param_object(const bir::Function& function,
                                                                   PreparedNameTables& names,
                                                                   const bir::Param& param,
                                                                   PreparedObjectId object_id) {
  // The active C++ BIR models aggregate params directly on `bir::Param`.
  // Unlike the retained Rust route, this layer does not receive
  // `param_alloca_values`, `ParamRef` destinations, or callee-saved register
  // assignment data, so Rust `dead_param_alloca` elision is not representable
  // here yet. Keep aggregate params materialized until the public contract
  // grows those inputs explicitly.
  std::string_view source_kind;
  bool address_exposed = false;
  bool requires_home_slot = false;
  bool permanent_home_slot = false;

  if (param.is_byval) {
    source_kind = "byval_param";
    address_exposed = true;
    requires_home_slot = true;
    permanent_home_slot = true;
  } else if (param.is_sret) {
    source_kind = "sret_param";
    address_exposed = true;
    requires_home_slot = true;
    permanent_home_slot = true;
  } else {
    return std::nullopt;
  }

  return PreparedStackObject{
      .object_id = object_id,
      .function_name = names.function_names.intern(function.name),
      .value_name = names.value_names.intern(param.name),
      .source_kind = std::string(source_kind),
      .type = param.type,
      .size_bytes = param.size_bytes,
      .align_bytes = param.align_bytes,
      .address_exposed = address_exposed,
      .requires_home_slot = requires_home_slot,
      .permanent_home_slot = permanent_home_slot,
  };
}

[[nodiscard]] std::optional<std::string_view> slot_slice_family(std::string_view slot_name) {
  const auto dot = slot_name.rfind('.');
  if (dot == std::string_view::npos || dot + 1 >= slot_name.size()) {
    return std::nullopt;
  }
  for (std::size_t index = dot + 1; index < slot_name.size(); ++index) {
    const char ch = slot_name[index];
    if (ch < '0' || ch > '9') {
      return std::nullopt;
    }
  }
  return slot_name.substr(0, dot);
}

void collect_published_pointer_value(std::unordered_set<std::string_view>& families,
                                     const bir::Value& value) {
  if (value.type == bir::TypeKind::Ptr &&
      value.kind == bir::Value::Kind::Named &&
      !value.name.empty()) {
    families.insert(value.name);
  }
}

}  // namespace

std::vector<PreparedStackObject> collect_function_stack_objects(PreparedNameTables& names,
                                                                const bir::NameTables& bir_names,
                                                                const bir::Function& function,
                                                                PreparedObjectId& next_object_id) {
  std::vector<PreparedStackObject> objects;
  objects.reserve(function.local_slots.size() + function.params.size());

  for (const auto& slot : function.local_slots) {
    objects.push_back(make_local_slot_object(function, names, bir_names, slot, next_object_id++));
  }

  for (const auto& param : function.params) {
    if (auto object = make_param_object(function, names, param, next_object_id); object.has_value()) {
      ++next_object_id;
      objects.push_back(*object);
    }
  }

  return objects;
}

void apply_aggregate_address_publication_hints(const PreparedNameTables& names,
                                               const bir::Function& function,
                                               std::vector<PreparedStackObject>& objects) {
  std::unordered_set<std::string_view> published_pointer_values;
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (const auto* call = std::get_if<bir::CallInst>(&inst); call != nullptr) {
        if (call_is_runtime_intrinsic_placeholder(*call)) {
          continue;
        }
        for (std::size_t index = 0; index < call->args.size(); ++index) {
          if (index < call->arg_abi.size() && call->arg_abi[index].byval_copy) {
            continue;
          }
          collect_published_pointer_value(published_pointer_values, call->args[index]);
        }
      } else if (const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
                 store != nullptr) {
        collect_published_pointer_value(published_pointer_values, store->value);
      }
    }
  }
  if (published_pointer_values.empty()) {
    return;
  }

  for (auto& object : objects) {
    if (!object.slot_name.has_value()) {
      continue;
    }
    const auto family = slot_slice_family(prepared_slot_name(names, *object.slot_name));
    if (!family.has_value() ||
        published_pointer_values.find(*family) == published_pointer_values.end()) {
      continue;
    }
    object.requires_home_slot = true;
    object.permanent_home_slot = true;
  }
}

}  // namespace c4c::backend::prepare::stack_layout
