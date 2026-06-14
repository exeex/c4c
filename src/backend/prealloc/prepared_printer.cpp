#include "prepared_printer.hpp"

#include "prepared_printer/private.hpp"

#include "../bir/bir.hpp"

#include <sstream>

namespace c4c::backend::prepare {

namespace {

struct PreparedBirBodyTextAgreement {
  std::string text;
  bool complete_module_text = false;
  bool requires_post_body_blank_line = false;
};

[[nodiscard]] bool complete_module_text_agrees_with_bir_printer(
    const bir::Module& module,
    const std::string& text) {
  return text == c4c::backend::bir::print(module);
}

[[nodiscard]] PreparedBirBodyTextAgreement prepared_bir_body_text_agreement(
    const bir::Module& module) {
  PreparedBirBodyTextAgreement agreement;
  agreement.text = c4c::backend::bir::print(module);
  agreement.complete_module_text =
      complete_module_text_agrees_with_bir_printer(module, agreement.text);
  agreement.requires_post_body_blank_line =
      !module.functions.empty() || !module.globals.empty() ||
      !module.string_constants.empty();
  return agreement;
}

}  // namespace

std::string print(const PreparedBirModule& module) {
  std::ostringstream out;
  out << "prepared.module target=" << module.target_profile.triple
      << " route=" << prepare_route_name(prepared_route(module)) << "\n";

  if (!module.completed_phases.empty()) {
    out << "completed_phases:";
    for (const auto& phase : module.completed_phases) {
      out << " " << phase;
    }
    out << "\n";
  }

  if (!module.invariants.empty()) {
    out << "invariants:\n";
    for (const auto& invariant : module.invariants) {
      out << "  - " << prepared_bir_invariant_name(invariant) << "\n";
    }
  }

  if (!module.notes.empty()) {
    out << "notes:\n";
    for (const auto& note : module.notes) {
      out << "  - [" << note.phase << "] " << note.message << "\n";
    }
  }

  out << "--- prepared-bir ---\n";
  const auto body_agreement = prepared_bir_body_text_agreement(module.module);
  if (body_agreement.complete_module_text) {
    out << body_agreement.text;
  } else {
    out << c4c::backend::bir::print(module.module);
  }
  if (body_agreement.requires_post_body_blank_line) {
    out << "\n";
  }

  append_function_summaries(out, module);
  append_prepared_control_flow(out, module);
  append_value_locations(out, module);
  append_stack_layout(out, module);
  append_frame_plan(out, module);
  append_dynamic_stack_plan(out, module);
  append_call_plans(out, module);
  append_store_source_publications(out, module);
  append_select_chain_materializations(out, module);
  append_variadic_entry_plans(out, module);
  append_regalloc(out, module);
  append_storage_plans(out, module);
  append_i128_carriers(out, module);
  append_f128_carriers(out, module);
  append_atomic_operations(out, module);
  append_intrinsic_carriers(out, module);
  append_inline_asm_carriers(out, module);
  append_f128_runtime_helpers(out, module);
  append_i128_runtime_helpers(out, module);
  append_addressing(out, module);
  return out.str();
}

}  // namespace c4c::backend::prepare
