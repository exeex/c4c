#include "debug.hpp"

#include "../x86.hpp"
#include "../abi/abi.hpp"

#include "../../../prealloc/prealloc.hpp"

#include <algorithm>
#include <sstream>

namespace c4c::backend::x86::debug {

namespace {

bool is_grouped_span(std::size_t contiguous_width,
                     const std::vector<std::string>& occupied_register_names) {
  return contiguous_width > 1 || occupied_register_names.size() > 1;
}

std::string render_grouped_span(c4c::backend::prepare::PreparedRegisterBank bank,
                                std::optional<std::string_view> register_name,
                                std::size_t contiguous_width,
                                const std::vector<std::string>& occupied_register_names) {
  std::ostringstream out;
  out << c4c::backend::prepare::prepared_register_bank_name(bank) << ":";
  if (register_name.has_value()) {
    out << *register_name;
  } else {
    out << "<none>";
  }
  out << "/w" << contiguous_width;
  if (!occupied_register_names.empty()) {
    out << "[";
    for (std::size_t index = 0; index < occupied_register_names.size(); ++index) {
      if (index != 0) {
        out << ",";
      }
      out << occupied_register_names[index];
    }
    out << "]";
  }
  return out.str();
}

std::string render_optional_grouped_span(
    std::optional<c4c::backend::prepare::PreparedRegisterBank> bank,
    std::optional<std::string_view> register_name,
    std::size_t contiguous_width,
    const std::vector<std::string>& occupied_register_names) {
  return render_grouped_span(bank.value_or(c4c::backend::prepare::PreparedRegisterBank::None),
                             register_name,
                             contiguous_width,
                             occupied_register_names);
}

void append_grouped_authority(std::ostringstream& out,
                              const c4c::backend::prepare::PreparedBirModule& module,
                              c4c::FunctionNameId function_name,
                              bool trace) {
  const auto consumed = c4c::backend::x86::consume_plans(module, function_name);
  const auto* frame_plan = consumed.frame;
  const auto* call_plans = consumed.calls;
  const auto* regalloc = consumed.regalloc;
  const auto* storage_plan = consumed.storage;

  std::size_t grouped_saved = 0;
  std::size_t grouped_preserved = 0;
  std::size_t grouped_clobbered = 0;
  std::size_t grouped_call_argument_spans = 0;
  std::size_t grouped_call_result_spans = 0;
  std::size_t grouped_spills = 0;
  std::size_t grouped_reloads = 0;
  std::size_t grouped_storage = 0;

  if (frame_plan != nullptr) {
    for (const auto& saved : frame_plan->saved_callee_registers) {
      if (!is_grouped_span(saved.contiguous_width, saved.occupied_register_names)) {
        continue;
      }
      ++grouped_saved;
    }
  }

  if (call_plans != nullptr) {
    for (const auto& call : call_plans->calls) {
      for (const auto& argument : call.arguments) {
        if (is_grouped_span(argument.destination_contiguous_width,
                            argument.destination_occupied_register_names)) {
          ++grouped_call_argument_spans;
        }
      }
      if (call.result.has_value() &&
          is_grouped_span(call.result->source_contiguous_width,
                          call.result->source_occupied_register_names)) {
        ++grouped_call_result_spans;
      }
      for (const auto& preserved : call.preserved_values) {
        if (!is_grouped_span(preserved.contiguous_width, preserved.occupied_register_names)) {
          continue;
        }
        ++grouped_preserved;
      }
      for (const auto& clobbered : call.clobbered_registers) {
        if (!is_grouped_span(clobbered.contiguous_width, clobbered.occupied_register_names)) {
          continue;
        }
        ++grouped_clobbered;
      }
    }
  }

  if (storage_plan != nullptr) {
    for (const auto& value : storage_plan->values) {
      if (!is_grouped_span(value.contiguous_width, value.occupied_register_names)) {
        continue;
      }
      ++grouped_storage;
    }
  }

  if (regalloc != nullptr) {
    for (const auto& op : regalloc->spill_reload_ops) {
      if (!is_grouped_span(op.contiguous_width, op.occupied_register_names)) {
        continue;
      }
      switch (op.op_kind) {
        case c4c::backend::prepare::PreparedSpillReloadOpKind::Spill:
          ++grouped_spills;
          break;
        case c4c::backend::prepare::PreparedSpillReloadOpKind::Reload:
          ++grouped_reloads;
          break;
        case c4c::backend::prepare::PreparedSpillReloadOpKind::Rematerialize:
          break;
      }
    }
  }

  if (grouped_saved == 0 && grouped_preserved == 0 && grouped_clobbered == 0 &&
      grouped_call_argument_spans == 0 && grouped_call_result_spans == 0 &&
      grouped_spills == 0 && grouped_reloads == 0 && grouped_storage == 0) {
    return;
  }

  out << "  grouped authority: saved=" << grouped_saved
      << " preserved=" << grouped_preserved
      << " clobbered=" << grouped_clobbered
      << " call_args=" << grouped_call_argument_spans
      << " call_results=" << grouped_call_result_spans
      << " spills=" << grouped_spills
      << " reloads=" << grouped_reloads
      << " storage=" << grouped_storage << "\n";

  if (!trace) {
    return;
  }

  if (frame_plan != nullptr) {
    for (const auto& saved : frame_plan->saved_callee_registers) {
      if (!is_grouped_span(saved.contiguous_width, saved.occupied_register_names)) {
        continue;
      }
      out << "    grouped saved save_index=" << saved.save_index
          << " span="
          << render_grouped_span(saved.bank,
                                 std::string_view(saved.register_name),
                                 saved.contiguous_width,
                                 saved.occupied_register_names)
          << "\n";
    }
  }

  if (call_plans != nullptr) {
    for (std::size_t call_index = 0; call_index < call_plans->calls.size(); ++call_index) {
      const auto& call = call_plans->calls[call_index];
      for (const auto& argument : call.arguments) {
        if (!is_grouped_span(argument.destination_contiguous_width,
                             argument.destination_occupied_register_names)) {
          continue;
        }
        out << "    grouped arg call#" << call_index
            << " arg#" << argument.arg_index;
        if (argument.source_value_id.has_value()) {
          out << " source_value_id=" << *argument.source_value_id;
        }
        out << " dest_span="
            << render_optional_grouped_span(argument.destination_register_bank,
                                            argument.destination_register_name.has_value()
                                                ? std::optional<std::string_view>{
                                                      *argument.destination_register_name}
                                                : std::nullopt,
                                            argument.destination_contiguous_width,
                                            argument.destination_occupied_register_names)
            << "\n";
      }
      if (call.result.has_value() &&
          is_grouped_span(call.result->source_contiguous_width,
                          call.result->source_occupied_register_names)) {
        const auto& result = *call.result;
        out << "    grouped result call#" << call_index;
        if (result.destination_value_id.has_value()) {
          out << " destination_value_id=" << *result.destination_value_id;
        }
        out << " source_span="
            << render_optional_grouped_span(result.source_register_bank,
                                            result.source_register_name.has_value()
                                                ? std::optional<std::string_view>{
                                                      *result.source_register_name}
                                                : std::nullopt,
                                            result.source_contiguous_width,
                                            result.source_occupied_register_names)
            << "\n";
      }
      for (const auto& preserved : call.preserved_values) {
        if (!is_grouped_span(preserved.contiguous_width, preserved.occupied_register_names)) {
          continue;
        }
        out << "    grouped preserve call#" << call_index
            << " value="
            << c4c::backend::prepare::prepared_value_name(module.names, preserved.value_name)
            << " route="
            << c4c::backend::prepare::prepared_call_preservation_route_name(preserved.route)
            << " span="
            << render_grouped_span(
                   preserved.register_bank.value_or(
                       c4c::backend::prepare::PreparedRegisterBank::None),
                   preserved.register_name.has_value()
                       ? std::optional<std::string_view>{*preserved.register_name}
                       : std::nullopt,
                   preserved.contiguous_width,
                   preserved.occupied_register_names);
        if (preserved.callee_saved_save_index.has_value()) {
          out << " save_index=" << *preserved.callee_saved_save_index;
        }
        out << "\n";
      }
      for (const auto& clobbered : call.clobbered_registers) {
        if (!is_grouped_span(clobbered.contiguous_width, clobbered.occupied_register_names)) {
          continue;
        }
        out << "    grouped clobber call#" << call_index
            << " span="
            << render_grouped_span(clobbered.bank,
                                   std::string_view(clobbered.register_name),
                                   clobbered.contiguous_width,
                                   clobbered.occupied_register_names)
            << "\n";
      }
    }
  }

  if (storage_plan != nullptr) {
    for (const auto& value : storage_plan->values) {
      if (!is_grouped_span(value.contiguous_width, value.occupied_register_names)) {
        continue;
      }
      out << "    grouped storage value="
          << c4c::backend::prepare::prepared_value_name(module.names, value.value_name)
          << " span="
          << render_grouped_span(value.bank,
                                 value.register_name.has_value()
                                     ? std::optional<std::string_view>{*value.register_name}
                                     : std::nullopt,
                                 value.contiguous_width,
                                 value.occupied_register_names)
          << "\n";
    }
  }

  if (regalloc != nullptr) {
    for (const auto& op : regalloc->spill_reload_ops) {
      if (!is_grouped_span(op.contiguous_width, op.occupied_register_names)) {
        continue;
      }
      if (op.op_kind != c4c::backend::prepare::PreparedSpillReloadOpKind::Spill &&
          op.op_kind != c4c::backend::prepare::PreparedSpillReloadOpKind::Reload) {
        continue;
      }
      out << "    grouped "
          << c4c::backend::prepare::prepared_spill_reload_op_kind_name(op.op_kind)
          << " value_id=" << op.value_id;
      if (const auto value_it = std::find_if(
              regalloc->values.begin(),
              regalloc->values.end(),
              [&op](const c4c::backend::prepare::PreparedRegallocValue& value) {
                return value.value_id == op.value_id;
              });
          value_it != regalloc->values.end()) {
        out << " value="
            << c4c::backend::prepare::prepared_value_name(module.names, value_it->value_name);
      }
      out << " span="
          << render_grouped_span(op.register_bank,
                                 op.register_name.has_value()
                                     ? std::optional<std::string_view>{*op.register_name}
                                     : std::nullopt,
                                 op.contiguous_width,
                                 op.occupied_register_names);
      if (op.slot_id.has_value()) {
        out << " slot_id=#" << *op.slot_id;
      }
      if (op.stack_offset_bytes.has_value()) {
        out << " stack_offset=" << *op.stack_offset_bytes;
      }
      out << "\n";
    }
  }
}

std::size_t count_defined_functions(const c4c::backend::prepare::PreparedBirModule& module) {
  std::size_t count = 0;
  for (const auto& function : module.module.functions) {
    if (!function.is_declaration) {
      ++count;
    }
  }
  return count;
}

bool has_indirect_variadic_call(const c4c::backend::prepare::PreparedBirModule& module) {
  for (const auto& function : module.module.functions) {
    if (function.is_declaration) {
      continue;
    }
    for (const auto& block : function.blocks) {
      for (const auto& inst : block.insts) {
        const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
        if (call != nullptr && call->is_indirect && call->is_variadic) {
          return true;
        }
      }
    }
  }
  return false;
}

std::string render_report(const c4c::backend::prepare::PreparedBirModule& module,
                          bool trace,
                          std::optional<std::string_view> focus_function,
                          std::optional<std::string_view> focus_block,
                          std::optional<std::string_view> focus_value) {
  std::ostringstream out;
  const auto target_profile = c4c::backend::x86::abi::resolve_target_profile(module);
  const auto target_triple = c4c::backend::x86::abi::resolve_target_triple(module);
  const auto defined_functions = count_defined_functions(module);
  out << "x86 route " << (trace ? "trace" : "summary") << "\n";
  out << "target triple: " << target_triple << "\n";
  out << "target arch: " << static_cast<int>(target_profile.arch) << "\n";
  out << "defined functions: " << defined_functions << "\n";
  if (focus_function.has_value()) {
    out << "focus function: " << *focus_function << "\n";
  }
  if (focus_block.has_value()) {
    out << "focus block: " << *focus_block << "\n";
  }
  if (focus_value.has_value()) {
    out << "focus value: " << *focus_value << "\n";
  }
  out << "route owner: x86/debug\n";
  out << "module emitter: x86/module\n";
  if (defined_functions > 1 && !module.module.globals.empty() &&
      has_indirect_variadic_call(module)) {
    if (trace) {
      out << "final: rejected: bounded multi-function handoff recognized the module, "
             "but the prepared shape is outside the current x86 support\n";
      out << "facts: one bounded multi-defined-function main-entry lane with same-module "
             "symbol calls and direct variadic runtime calls\n";
      out << "next inspect: inspect the current x86 bounded multi-function shape support "
             "in src/backend/mir/x86/codegen/module/module_emit.cpp\n";
    } else {
      out << "module-level bounded multi-function lane: rejected\n";
      out << "- module-level final rejection: bounded multi-function handoff recognized "
             "the module, but the prepared shape is outside the current x86 support\n";
      out << "- module-level facts: one bounded multi-defined-function main-entry lane "
             "with same-module symbol calls and direct variadic runtime calls\n";
      out << "- module-level next inspect: inspect the current x86 bounded multi-function "
             "shape support in src/backend/mir/x86/codegen/module/module_emit.cpp\n";
    }
  }
  if (trace) {
    for (const auto& function : module.module.functions) {
      if (function.is_declaration) {
        continue;
      }
      if (focus_function.has_value() && function.name != *focus_function) {
        continue;
      }
      out << "- function " << function.name << ": "
          << function.blocks.size() << " blocks, return type "
          << c4c::backend::bir::render_type(function.return_type) << "\n";
      const auto function_name =
          c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function.name);
      if (function_name.has_value()) {
        append_grouped_authority(out, module, *function_name, trace);
      }
    }
  }
  if (!trace) {
    for (const auto& function : module.module.functions) {
      if (function.is_declaration) {
        continue;
      }
      if (focus_function.has_value() && function.name != *focus_function) {
        continue;
      }
      const auto function_name =
          c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function.name);
      if (!function_name.has_value()) {
        continue;
      }
      append_grouped_authority(out, module, *function_name, trace);
    }
  }
  out << "status: contract-first debug surface; structured lane diagnostics still deferred\n";
  return out.str();
}

}  // namespace

std::string summarize_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function,
    std::optional<std::string_view> focus_block,
    std::optional<std::string_view> focus_value) {
  return render_report(module, false, focus_function, focus_block, focus_value);
}

std::string trace_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function,
    std::optional<std::string_view> focus_block,
    std::optional<std::string_view> focus_value) {
  return render_report(module, true, focus_function, focus_block, focus_value);
}

}  // namespace c4c::backend::x86::debug
