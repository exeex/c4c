#pragma once

#include "../../../bir/bir.hpp"
#include "../../../prealloc/prealloc.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::x86::prepared {

struct Query {
  const c4c::backend::prepare::PreparedBirModule* module = nullptr;
  std::optional<c4c::FunctionNameId> function_name;

  [[nodiscard]] const c4c::backend::prepare::PreparedAddressingFunction* addressing() const {
    if (module == nullptr || !function_name.has_value()) {
      return nullptr;
    }
    return c4c::backend::prepare::find_prepared_addressing(*module, *function_name);
  }

  [[nodiscard]] const c4c::backend::prepare::PreparedValueLocationFunction* locations() const {
    if (module == nullptr || !function_name.has_value()) {
      return nullptr;
    }
    return c4c::backend::prepare::find_prepared_value_location_function(*module, *function_name);
  }

  [[nodiscard]] const c4c::backend::prepare::PreparedCallPlansFunction* call_plans() const {
    if (module == nullptr || !function_name.has_value()) {
      return nullptr;
    }
    return c4c::backend::prepare::find_prepared_call_plans(*module, *function_name);
  }

  [[nodiscard]] const c4c::backend::prepare::PreparedRegallocFunction* regalloc() const {
    if (module == nullptr || !function_name.has_value()) {
      return nullptr;
    }
    for (const auto& function_regalloc : module->regalloc.functions) {
      if (function_regalloc.function_name == *function_name) {
        return &function_regalloc;
      }
    }
    return nullptr;
  }

  [[nodiscard]] const c4c::backend::prepare::PreparedStoragePlanFunction* storage_plan() const {
    if (module == nullptr || !function_name.has_value()) {
      return nullptr;
    }
    return c4c::backend::prepare::find_prepared_storage_plan(*module, *function_name);
  }

  [[nodiscard]] const c4c::backend::bir::Function* bir_function() const {
    if (module == nullptr || !function_name.has_value()) {
      return nullptr;
    }
    for (const auto& function : module->module.functions) {
      if (module->names.function_names.find(function.name) == *function_name) {
        return &function;
      }
    }
    return nullptr;
  }

  [[nodiscard]] c4c::backend::prepare::PreparedDecodedHomeStorage decode_home_storage(
      c4c::backend::prepare::PreparedValueId value_id) const {
    return c4c::backend::prepare::decode_prepared_home_storage(
        c4c::backend::prepare::PreparedHomeStorageDecodeInputs{
            .regalloc = regalloc(),
            .storage_plan = storage_plan(),
            .value_locations = locations(),
            .value_home_lookups = nullptr,
        },
        value_id);
  }

  [[nodiscard]] c4c::backend::prepare::PreparedDecodedHomeStorageDiagnostic
  diagnose_home_storage(c4c::backend::prepare::PreparedValueId value_id) const {
    return c4c::backend::prepare::build_prepared_decoded_home_storage_diagnostic(
        decode_home_storage(value_id));
  }

  [[nodiscard]] c4c::backend::prepare::PreparedCallBoundaryMoveClassification
  classify_call_boundary_move(
      const c4c::backend::prepare::PreparedCallPlan& call_plan,
      const c4c::backend::prepare::PreparedMoveBundle& bundle,
      const c4c::backend::prepare::PreparedMoveResolution& move) const {
    return c4c::backend::prepare::classify_prepared_call_boundary_move(
        call_plan, bundle, move);
  }

  [[nodiscard]] c4c::backend::prepare::PreparedFormalPublicationPlan
  plan_formal_publication(std::size_t formal_index) const {
    return c4c::backend::prepare::plan_prepared_formal_publication(
        c4c::backend::prepare::PreparedFormalPublicationInputs{
            .names = module == nullptr ? nullptr : &module->names,
            .function = bir_function(),
            .value_locations = locations(),
            .value_home_lookups = nullptr,
        },
        formal_index);
  }

  [[nodiscard]] std::vector<c4c::backend::prepare::PreparedFormalPublicationPlan>
  plan_formal_publications() const {
    return c4c::backend::prepare::plan_prepared_formal_publications(
        c4c::backend::prepare::PreparedFormalPublicationInputs{
            .names = module == nullptr ? nullptr : &module->names,
            .function = bir_function(),
            .value_locations = locations(),
            .value_home_lookups = nullptr,
        });
  }

  [[nodiscard]] std::vector<c4c::backend::prepare::PreparedBlockEntryPublication>
  collect_block_entry_publications(c4c::BlockLabelId successor_label) const {
    return c4c::backend::prepare::collect_prepared_block_entry_publications(
        locations(), successor_label);
  }
};

[[nodiscard]] inline Query make_query(const c4c::backend::prepare::PreparedBirModule& module,
                                      std::string_view function_name) {
  // Public x86 entry points still accept rendered focus/entry names. Convert
  // them to the prepared name table before touching prepared route state.
  return Query{
      .module = &module,
      .function_name =
          c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function_name),
  };
}

struct FastPath {
  bool accepted = false;
  std::string lane;
  std::string reason;
};

struct Operand {
  // Final x86 operand spelling only; this text is not a BIR value identity.
  std::string text;
  bool materialize = false;
};

[[nodiscard]] FastPath classify_module_fast_path(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function = std::nullopt);
[[nodiscard]] std::optional<Operand> render_immediate_operand(
    const c4c::backend::bir::Value& value);

}  // namespace c4c::backend::x86::prepared
