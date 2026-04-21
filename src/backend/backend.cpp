#include "backend.hpp"

#include "bir/bir_printer.hpp"
#include "mir/x86/codegen/x86_codegen.hpp"
#include "prealloc/prepared_printer.hpp"

#include "../codegen/lir/lir_printer.hpp"

#include <algorithm>
#include <optional>
#include <sstream>

namespace c4c::backend {

namespace {

const c4c::TargetProfile& profile_or_default(const c4c::TargetProfile& target_profile,
                                             c4c::TargetProfile& storage,
                                             std::string_view module_triple) {
  if (target_profile.arch != c4c::TargetArch::Unknown) {
    return target_profile;
  }
  storage = c4c::target_profile_from_triple(
      module_triple.empty() ? c4c::default_host_target_triple() : module_triple);
  return storage;
}

c4c::TargetProfile resolve_public_lir_target_profile(const c4c::codegen::lir::LirModule& module,
                                                     const c4c::TargetProfile& public_target) {
  c4c::TargetProfile module_profile_storage;
  const auto& requested = public_target.arch != c4c::TargetArch::Unknown
                              ? public_target
                              : (module.target_profile.arch != c4c::TargetArch::Unknown
                                     ? module.target_profile
                                     : profile_or_default(public_target,
                                                          module_profile_storage,
                                                          std::string_view{}));
  if (requested.arch != c4c::TargetArch::X86_64) {
    return requested;
  }

  if (module.target_profile.arch == c4c::TargetArch::Unknown) {
    return requested;
  }

  const auto module_profile = module.target_profile;
  if (module_profile.arch == c4c::TargetArch::I686) {
    return module_profile;
  }
  return requested;
}

std::string emit_bootstrap_lir_module(const c4c::codegen::lir::LirModule& module,
                                      const c4c::TargetProfile& target_profile) {
  (void)target_profile;
  return c4c::codegen::lir::print_llvm(module);
}

bool is_x86_target(const c4c::TargetProfile& target_profile) {
  return target_profile.arch == c4c::TargetArch::X86_64 ||
         target_profile.arch == c4c::TargetArch::I686;
}

std::string make_x86_lir_handoff_failure_message(
    const c4c::backend::BirLoweringResult& lowering) {
  std::string message =
      "x86 backend emit path requires semantic lir_to_bir lowering before the canonical prepared-module handoff";
  for (auto it = lowering.notes.rbegin(); it != lowering.notes.rend(); ++it) {
    if (it->phase == "module" || it->phase == "function") {
      message += ": ";
      message += it->message;
      break;
    }
  }
  return message;
}

std::string make_backend_dump_failure_message(
    const c4c::backend::BirLoweringResult& lowering) {
  std::string message =
      "backend BIR dump requires semantic lir_to_bir lowering before the prepared handoff";
  for (auto it = lowering.notes.rbegin(); it != lowering.notes.rend(); ++it) {
    if (it->phase == "module" || it->phase == "function") {
      message += ": ";
      message += it->message;
      break;
    }
  }
  return message;
}

// The active public backend entry still stops at prepared semantic BIR text.
// Keep this helper name honest until x86 is wired to a real backend-side handoff.
std::string render_prepared_bir_text(const c4c::backend::bir::Module& module) {
  return c4c::backend::bir::print(module);
}

c4c::backend::prepare::PreparedBirModule prepare_semantic_bir_pipeline(
    const c4c::backend::bir::Module& module,
    const c4c::TargetProfile& target_profile) {
  return c4c::backend::prepare::prepare_semantic_bir_module_with_options(module, target_profile);
}

std::size_t count_matching_functions(const c4c::backend::bir::Module& module,
                                     std::string_view function_name) {
  return static_cast<std::size_t>(std::count_if(module.functions.begin(),
                                                module.functions.end(),
                                                [&](const c4c::backend::bir::Function& function) {
                                                  return function.name == function_name;
                                                }));
}

c4c::backend::bir::Module filter_bir_module_to_function(
    const c4c::backend::bir::Module& module,
    std::string_view function_name) {
  c4c::backend::bir::Module filtered = module;
  filtered.functions.clear();
  for (const auto& function : module.functions) {
    if (function.name == function_name) {
      filtered.functions.push_back(function);
    }
  }
  return filtered;
}

std::string make_focus_header(std::string_view function_name,
                              std::size_t matched_count) {
  std::ostringstream out;
  out << "focus function: " << function_name << "\n";
  out << "focused functions matched: " << matched_count << "\n";
  return out.str();
}

std::optional<c4c::FunctionNameId> find_prepared_function_name_id(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view function_name) {
  return c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function_name);
}

c4c::backend::prepare::PreparedBirModule filter_prepared_module_to_function(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view function_name) {
  c4c::backend::prepare::PreparedBirModule filtered = module;
  filtered.module = filter_bir_module_to_function(module.module, function_name);

  const auto function_name_id = find_prepared_function_name_id(module, function_name);
  if (!function_name_id.has_value()) {
    filtered.control_flow.functions.clear();
    filtered.value_locations.functions.clear();
    filtered.addressing.functions.clear();
    filtered.liveness.functions.clear();
    filtered.regalloc.functions.clear();
    filtered.stack_layout.objects.clear();
    filtered.stack_layout.frame_slots.clear();
    filtered.stack_layout.frame_size_bytes = 0;
    filtered.stack_layout.frame_alignment_bytes = 0;
    filtered.notes.erase(std::remove_if(filtered.notes.begin(),
                                        filtered.notes.end(),
                                        [](const c4c::backend::prepare::PrepareNote& note) {
                                          return note.phase == "stack_layout" ||
                                                 note.phase == "regalloc" ||
                                                 note.phase == "liveness";
                                        }),
                         filtered.notes.end());
    return filtered;
  }

  const auto keep_function_id = *function_name_id;
  auto keep_function = [&](const auto& function_record) {
    return function_record.function_name == keep_function_id;
  };
  filtered.control_flow.functions.erase(
      std::remove_if(filtered.control_flow.functions.begin(),
                     filtered.control_flow.functions.end(),
                     [&](const auto& function_record) { return !keep_function(function_record); }),
      filtered.control_flow.functions.end());
  filtered.value_locations.functions.erase(
      std::remove_if(filtered.value_locations.functions.begin(),
                     filtered.value_locations.functions.end(),
                     [&](const auto& function_record) { return !keep_function(function_record); }),
      filtered.value_locations.functions.end());
  filtered.addressing.functions.erase(
      std::remove_if(filtered.addressing.functions.begin(),
                     filtered.addressing.functions.end(),
                     [&](const auto& function_record) { return !keep_function(function_record); }),
      filtered.addressing.functions.end());
  filtered.liveness.functions.erase(
      std::remove_if(filtered.liveness.functions.begin(),
                     filtered.liveness.functions.end(),
                     [&](const auto& function_record) { return !keep_function(function_record); }),
      filtered.liveness.functions.end());
  filtered.regalloc.functions.erase(
      std::remove_if(filtered.regalloc.functions.begin(),
                     filtered.regalloc.functions.end(),
                     [&](const auto& function_record) { return !keep_function(function_record); }),
      filtered.regalloc.functions.end());
  filtered.stack_layout.objects.erase(
      std::remove_if(filtered.stack_layout.objects.begin(),
                     filtered.stack_layout.objects.end(),
                     [&](const c4c::backend::prepare::PreparedStackObject& object) {
                       return object.function_name != keep_function_id;
                     }),
      filtered.stack_layout.objects.end());
  filtered.stack_layout.frame_slots.erase(
      std::remove_if(filtered.stack_layout.frame_slots.begin(),
                     filtered.stack_layout.frame_slots.end(),
                     [&](const c4c::backend::prepare::PreparedFrameSlot& slot) {
                       return slot.function_name != keep_function_id;
                     }),
      filtered.stack_layout.frame_slots.end());

  if (const auto* focused_addressing =
          c4c::backend::prepare::find_prepared_addressing(module, keep_function_id);
      focused_addressing != nullptr) {
    filtered.stack_layout.frame_size_bytes = focused_addressing->frame_size_bytes;
    filtered.stack_layout.frame_alignment_bytes = focused_addressing->frame_alignment_bytes;
  } else {
    filtered.stack_layout.frame_size_bytes = 0;
    filtered.stack_layout.frame_alignment_bytes = 0;
  }

  const std::string function_note_marker = std::string("function '") + std::string(function_name) + "'";
  filtered.notes.erase(std::remove_if(filtered.notes.begin(),
                                      filtered.notes.end(),
                                      [&](const c4c::backend::prepare::PrepareNote& note) {
                                        if (note.phase != "stack_layout" &&
                                            note.phase != "regalloc" &&
                                            note.phase != "liveness") {
                                          return false;
                                        }
                                        return note.message.find(function_note_marker) ==
                                               std::string::npos;
                                      }),
                       filtered.notes.end());
  return filtered;
}

}  // namespace

BackendModuleInput::BackendModuleInput(const bir::Module& bir_module)
    : module_(bir_module) {}

BackendModuleInput::BackendModuleInput(const c4c::codegen::lir::LirModule& lir_module)
    : module_(std::cref(lir_module)) {}

c4c::backend::bir::Module prepare_bir_module_for_target(
    const c4c::backend::bir::Module& module,
    const c4c::TargetProfile& target_profile) {
  return prepare_semantic_bir_pipeline(module, target_profile).module;
}

std::string emit_target_bir_module(const bir::Module& module,
                                   const c4c::TargetProfile& target_profile) {
  const auto prepared = prepare_semantic_bir_pipeline(module, target_profile);
  if (is_x86_target(target_profile)) {
    return c4c::backend::x86::emit_prepared_module(prepared);
  }
  return render_prepared_bir_text(prepared.module);
}

std::string emit_target_lir_module(const c4c::codegen::lir::LirModule& module,
                                   const c4c::TargetProfile& target_profile) {
  c4c::backend::BirLoweringOptions lowering_options{};
  auto lowering = c4c::backend::try_lower_to_bir_with_options(module, lowering_options);
  if (lowering.module.has_value()) {
    const auto prepared_bir = prepare_semantic_bir_pipeline(*lowering.module, target_profile);
    if (is_x86_target(target_profile)) {
      return c4c::backend::x86::emit_prepared_module(prepared_bir);
    }
    return render_prepared_bir_text(prepared_bir.module);
  }
  if (is_x86_target(target_profile)) {
    throw std::invalid_argument(make_x86_lir_handoff_failure_message(lowering));
  }
  return emit_bootstrap_lir_module(module, target_profile);
}

BackendAssembleResult assemble_target_lir_module(
    const c4c::codegen::lir::LirModule& module,
    const c4c::TargetProfile& target_profile,
    const std::string& output_path) {
  const auto emitted = emit_target_lir_module(module, target_profile);
  return BackendAssembleResult{
      .staged_text = emitted,
      .output_path = output_path,
      .object_emitted = false,
      .error = "backend bootstrap mode does not assemble objects yet",
  };
}

std::string emit_module(const BackendModuleInput& input,
                        const BackendOptions& options) {
  if (input.holds_bir_module()) {
    if (options.emit_semantic_bir) {
      return c4c::backend::bir::print(input.bir_module());
    }
    c4c::TargetProfile target_profile_storage;
    return emit_target_bir_module(
        input.bir_module(),
        profile_or_default(options.target_profile,
                           target_profile_storage,
                           input.bir_module().target_triple));
  }

  const auto& lir_module = input.lir_module();
  const auto target_profile = resolve_public_lir_target_profile(lir_module, options.target_profile);

  c4c::backend::BirLoweringOptions lowering_options{};
  lowering_options.preserve_dynamic_alloca = true;
  auto lowering = c4c::backend::try_lower_to_bir_with_options(lir_module, lowering_options);

  if (lowering.module.has_value()) {
    if (options.emit_semantic_bir) {
      return c4c::backend::bir::print(*lowering.module);
    }
    const auto prepared_bir = prepare_semantic_bir_pipeline(*lowering.module, target_profile);
    if (is_x86_target(target_profile)) {
      return c4c::backend::x86::emit_prepared_module(prepared_bir);
    }
    return render_prepared_bir_text(prepared_bir.module);
  }
  if (is_x86_target(target_profile)) {
    throw std::invalid_argument(make_x86_lir_handoff_failure_message(lowering));
  }
  return emit_bootstrap_lir_module(lir_module, target_profile);
}

std::string dump_module(const BackendModuleInput& input,
                        const BackendOptions& options,
                        BackendDumpStage stage) {
  if (input.holds_bir_module()) {
    const auto matched_functions =
        options.route_debug_focus_function.has_value()
            ? count_matching_functions(input.bir_module(), *options.route_debug_focus_function)
            : 0;
    if (stage == BackendDumpStage::SemanticBir) {
      if (!options.route_debug_focus_function.has_value()) {
        return c4c::backend::bir::print(input.bir_module());
      }
      return make_focus_header(*options.route_debug_focus_function, matched_functions) +
             c4c::backend::bir::print(
                 filter_bir_module_to_function(input.bir_module(),
                                               *options.route_debug_focus_function));
    }
    c4c::TargetProfile target_profile_storage;
    auto prepared = prepare_semantic_bir_pipeline(
        input.bir_module(),
        profile_or_default(options.target_profile,
                           target_profile_storage,
                           input.bir_module().target_triple));
    if (stage == BackendDumpStage::MirSummary) {
      return c4c::backend::x86::summarize_prepared_module_routes(
          prepared,
          options.route_debug_focus_function,
          options.route_debug_focus_block);
    }
    if (stage == BackendDumpStage::MirTrace) {
      return c4c::backend::x86::trace_prepared_module_routes(
          prepared,
          options.route_debug_focus_function,
          options.route_debug_focus_block);
    }
    if (!options.route_debug_focus_function.has_value()) {
      return c4c::backend::prepare::print(prepared);
    }
    prepared = filter_prepared_module_to_function(prepared, *options.route_debug_focus_function);
    return make_focus_header(*options.route_debug_focus_function, matched_functions) +
           c4c::backend::prepare::print(prepared);
  }

  const auto& lir_module = input.lir_module();
  const auto target_profile = resolve_public_lir_target_profile(lir_module, options.target_profile);

  c4c::backend::BirLoweringOptions lowering_options{};
  lowering_options.preserve_dynamic_alloca = true;
  auto lowering = c4c::backend::try_lower_to_bir_with_options(lir_module, lowering_options);
  if (!lowering.module.has_value()) {
    throw std::invalid_argument(make_backend_dump_failure_message(lowering));
  }

  const auto matched_functions =
      options.route_debug_focus_function.has_value()
          ? count_matching_functions(*lowering.module, *options.route_debug_focus_function)
          : 0;

  if (stage == BackendDumpStage::SemanticBir) {
    if (!options.route_debug_focus_function.has_value()) {
      return c4c::backend::bir::print(*lowering.module);
    }
    return make_focus_header(*options.route_debug_focus_function, matched_functions) +
           c4c::backend::bir::print(
               filter_bir_module_to_function(*lowering.module,
                                             *options.route_debug_focus_function));
  }
  auto prepared = prepare_semantic_bir_pipeline(*lowering.module, target_profile);
  if (stage == BackendDumpStage::MirSummary) {
    return c4c::backend::x86::summarize_prepared_module_routes(
        prepared,
        options.route_debug_focus_function,
        options.route_debug_focus_block);
  }
  if (stage == BackendDumpStage::MirTrace) {
    return c4c::backend::x86::trace_prepared_module_routes(
        prepared,
        options.route_debug_focus_function,
        options.route_debug_focus_block);
  }
  if (!options.route_debug_focus_function.has_value()) {
    return c4c::backend::prepare::print(prepared);
  }
  prepared = filter_prepared_module_to_function(prepared, *options.route_debug_focus_function);
  return make_focus_header(*options.route_debug_focus_function, matched_functions) +
         c4c::backend::prepare::print(prepared);
}

}  // namespace c4c::backend
