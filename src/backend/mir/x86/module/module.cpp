#include "module.hpp"

#include "../x86.hpp"
#include "../abi/abi.hpp"
#include "../core/core.hpp"

#include "../../../prealloc/prepared_printer.hpp"

#include <algorithm>
#include <optional>

namespace c4c::backend::x86::module {

namespace {

bool is_grouped_span(std::size_t contiguous_width,
                     const std::vector<std::string>& occupied_register_names) {
  return contiguous_width > 1 || occupied_register_names.size() > 1;
}

std::string render_grouped_span(c4c::backend::prepare::PreparedRegisterBank bank,
                                std::optional<std::string_view> register_name,
                                std::size_t contiguous_width,
                                const std::vector<std::string>& occupied_register_names) {
  std::string rendered =
      std::string(c4c::backend::prepare::prepared_register_bank_name(bank)) + ":";
  rendered += register_name.has_value() ? std::string(*register_name) : std::string("<none>");
  rendered += "/w" + std::to_string(contiguous_width);
  if (!occupied_register_names.empty()) {
    rendered += "[";
    for (std::size_t index = 0; index < occupied_register_names.size(); ++index) {
      if (index != 0) {
        rendered += ",";
      }
      rendered += occupied_register_names[index];
    }
    rendered += "]";
  }
  return rendered;
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

void append_grouped_authority_comments(c4c::backend::x86::core::Text& out,
                                       const c4c::backend::prepare::PreparedBirModule& module,
                                       c4c::FunctionNameId function_name) {
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
    grouped_saved = static_cast<std::size_t>(std::count_if(
        frame_plan->saved_callee_registers.begin(),
        frame_plan->saved_callee_registers.end(),
        [](const c4c::backend::prepare::PreparedSavedRegister& saved) {
          return is_grouped_span(saved.contiguous_width, saved.occupied_register_names);
        }));
  }

  if (call_plans != nullptr) {
    for (const auto& call : call_plans->calls) {
      grouped_call_argument_spans += static_cast<std::size_t>(std::count_if(
          call.arguments.begin(),
          call.arguments.end(),
          [](const c4c::backend::prepare::PreparedCallArgumentPlan& argument) {
            return is_grouped_span(argument.destination_contiguous_width,
                                   argument.destination_occupied_register_names);
          }));
      if (call.result.has_value() &&
          is_grouped_span(call.result->source_contiguous_width,
                          call.result->source_occupied_register_names)) {
        ++grouped_call_result_spans;
      }
      grouped_preserved += static_cast<std::size_t>(std::count_if(
          call.preserved_values.begin(),
          call.preserved_values.end(),
          [](const c4c::backend::prepare::PreparedCallPreservedValue& preserved) {
            return is_grouped_span(preserved.contiguous_width, preserved.occupied_register_names);
          }));
      grouped_clobbered += static_cast<std::size_t>(std::count_if(
          call.clobbered_registers.begin(),
          call.clobbered_registers.end(),
          [](const c4c::backend::prepare::PreparedClobberedRegister& clobber) {
            return is_grouped_span(clobber.contiguous_width, clobber.occupied_register_names);
          }));
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

  if (storage_plan != nullptr) {
    grouped_storage = static_cast<std::size_t>(std::count_if(
        storage_plan->values.begin(),
        storage_plan->values.end(),
        [](const c4c::backend::prepare::PreparedStoragePlanValue& value) {
          return is_grouped_span(value.contiguous_width, value.occupied_register_names);
        }));
  }

  if (grouped_saved == 0 && grouped_preserved == 0 && grouped_clobbered == 0 &&
      grouped_call_argument_spans == 0 && grouped_call_result_spans == 0 &&
      grouped_spills == 0 && grouped_reloads == 0 && grouped_storage == 0) {
    return;
  }

  out.append_line("    # grouped authority: saved=" + std::to_string(grouped_saved) +
                  " preserved=" + std::to_string(grouped_preserved) +
                  " clobbered=" + std::to_string(grouped_clobbered) +
                  " call_args=" + std::to_string(grouped_call_argument_spans) +
                  " call_results=" + std::to_string(grouped_call_result_spans) +
                  " spills=" + std::to_string(grouped_spills) +
                  " reloads=" + std::to_string(grouped_reloads) +
                  " storage=" + std::to_string(grouped_storage));

  if (frame_plan != nullptr) {
    for (const auto& saved : frame_plan->saved_callee_registers) {
      if (!is_grouped_span(saved.contiguous_width, saved.occupied_register_names)) {
        continue;
      }
      out.append_line("    # grouped saved save_index=" + std::to_string(saved.save_index) +
                      " span=" +
                      render_grouped_span(saved.bank,
                                          std::string_view(saved.register_name),
                                          saved.contiguous_width,
                                          saved.occupied_register_names));
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
        std::string line = "    # grouped arg call#" + std::to_string(call_index) +
                           " arg#" + std::to_string(argument.arg_index);
        if (argument.source_value_id.has_value()) {
          line += " source_value_id=" + std::to_string(*argument.source_value_id);
        }
        line += " dest_span=" +
                render_optional_grouped_span(argument.destination_register_bank,
                                             argument.destination_register_name.has_value()
                                                 ? std::optional<std::string_view>{
                                                       *argument.destination_register_name}
                                                 : std::nullopt,
                                             argument.destination_contiguous_width,
                                             argument.destination_occupied_register_names);
        out.append_line(line);
      }
      if (call.result.has_value() &&
          is_grouped_span(call.result->source_contiguous_width,
                          call.result->source_occupied_register_names)) {
        const auto& result = *call.result;
        std::string line = "    # grouped result call#" + std::to_string(call_index);
        if (result.destination_value_id.has_value()) {
          line += " destination_value_id=" + std::to_string(*result.destination_value_id);
        }
        line += " source_span=" +
                render_optional_grouped_span(result.source_register_bank,
                                             result.source_register_name.has_value()
                                                 ? std::optional<std::string_view>{
                                                       *result.source_register_name}
                                                 : std::nullopt,
                                             result.source_contiguous_width,
                                             result.source_occupied_register_names);
        out.append_line(line);
      }
      for (const auto& preserved : call.preserved_values) {
        if (!is_grouped_span(preserved.contiguous_width, preserved.occupied_register_names)) {
          continue;
        }
        std::string line =
            "    # grouped preserve call#" + std::to_string(call_index) + " value=" +
            std::string(
                c4c::backend::prepare::prepared_value_name(module.names, preserved.value_name)) +
            " route=" +
            std::string(
                c4c::backend::prepare::prepared_call_preservation_route_name(preserved.route)) +
            " span=" +
            render_grouped_span(
                preserved.register_bank.value_or(
                    c4c::backend::prepare::PreparedRegisterBank::None),
                preserved.register_name.has_value()
                    ? std::optional<std::string_view>{*preserved.register_name}
                    : std::nullopt,
                preserved.contiguous_width,
                preserved.occupied_register_names);
        if (preserved.callee_saved_save_index.has_value()) {
          line += " save_index=" + std::to_string(*preserved.callee_saved_save_index);
        }
        out.append_line(line);
      }
      for (const auto& clobbered : call.clobbered_registers) {
        if (!is_grouped_span(clobbered.contiguous_width, clobbered.occupied_register_names)) {
          continue;
        }
        out.append_line("    # grouped clobber call#" + std::to_string(call_index) +
                        " span=" +
                        render_grouped_span(clobbered.bank,
                                            std::string_view(clobbered.register_name),
                                            clobbered.contiguous_width,
                                            clobbered.occupied_register_names));
      }
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
      std::string line =
          "    # grouped " +
          std::string(c4c::backend::prepare::prepared_spill_reload_op_kind_name(op.op_kind)) +
          " value_id=" + std::to_string(op.value_id);
      if (const auto value_it = std::find_if(
              regalloc->values.begin(),
              regalloc->values.end(),
              [&op](const c4c::backend::prepare::PreparedRegallocValue& value) {
                return value.value_id == op.value_id;
              });
          value_it != regalloc->values.end()) {
        line += " value=" +
                std::string(
                    c4c::backend::prepare::prepared_value_name(module.names, value_it->value_name));
      }
      line += " span=" +
              render_grouped_span(op.register_bank,
                                  op.register_name.has_value()
                                      ? std::optional<std::string_view>{*op.register_name}
                                      : std::nullopt,
                                  op.contiguous_width,
                                  op.occupied_register_names);
      if (op.slot_id.has_value()) {
        line += " slot_id=#" + std::to_string(*op.slot_id);
      }
      if (op.stack_offset_bytes.has_value()) {
        line += " stack_offset=" + std::to_string(*op.stack_offset_bytes);
      }
      out.append_line(line);
    }
  }

  if (storage_plan != nullptr) {
    for (const auto& value : storage_plan->values) {
      if (!is_grouped_span(value.contiguous_width, value.occupied_register_names)) {
        continue;
      }
      out.append_line(
          "    # grouped storage value=" +
          std::string(c4c::backend::prepare::prepared_value_name(module.names, value.value_name)) +
          " span=" +
          render_grouped_span(value.bank,
                              value.register_name.has_value()
                                  ? std::optional<std::string_view>{*value.register_name}
                                  : std::nullopt,
                              value.contiguous_width,
                              value.occupied_register_names));
    }
  }
}

void append_function_stub(c4c::backend::x86::core::Text& out,
                          const c4c::backend::prepare::PreparedBirModule& module,
                          const c4c::backend::bir::Function& function,
                          const Data& data) {
  const auto symbol_name = data.render_asm_symbol_name(function.name);
  out.append_line(".globl " + symbol_name);
  out.append_line(".type " + symbol_name + ", @function");
  out.append_line(symbol_name + ":");
  if (const auto function_name =
          c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function.name);
      function_name.has_value()) {
    append_grouped_authority_comments(out, module, *function_name);
  }
  out.append_line("    # x86 backend contract-first stub");
  out.append_line("    # behavior recovery will replace this body");
  if (function.return_type != c4c::backend::bir::TypeKind::Void) {
    out.append_line("    xor eax, eax");
  }
  out.append_line("    ret");
}

}  // namespace

std::string emit(const c4c::backend::prepare::PreparedBirModule& module) {
  const auto target_profile = c4c::backend::x86::abi::resolve_target_profile(module);
  if (!c4c::backend::x86::abi::is_x86_target(target_profile)) {
    throw std::invalid_argument("x86::module::emit requires an x86 target profile");
  }

  const auto target_triple = c4c::backend::x86::abi::resolve_target_triple(module);
  const auto data = make_data(module, target_triple);

  c4c::backend::x86::core::Text out;
  out.append_line(".intel_syntax noprefix");
  out.append_line(".text");
  out.append_line("# x86 backend contract-first module emitter");

  bool emitted_any_function = false;
  for (const auto& function : module.module.functions) {
    if (function.is_declaration) {
      continue;
    }
    emitted_any_function = true;
    append_function_stub(out, module, function, data);
  }

  if (!emitted_any_function) {
    out.append_line("# no defined functions");
  }

  out.append_raw(data.emit_data());
  return out.take_text();
}

}  // namespace c4c::backend::x86::module
