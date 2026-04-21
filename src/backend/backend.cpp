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

const c4c::backend::bir::Function* find_function(const c4c::backend::bir::Module& module,
                                                 std::string_view function_name) {
  for (const auto& function : module.functions) {
    if (function.name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

c4c::backend::bir::Function filter_bir_function_to_block(
    const c4c::backend::bir::Function& function,
    std::string_view block_label) {
  c4c::backend::bir::Function filtered = function;
  filtered.blocks.erase(
      std::remove_if(filtered.blocks.begin(),
                     filtered.blocks.end(),
                     [&](const c4c::backend::bir::Block& block) {
                       return block.label != block_label;
                     }),
      filtered.blocks.end());
  return filtered;
}

std::size_t count_matching_blocks(const c4c::backend::bir::Function& function,
                                  std::string_view block_label) {
  return static_cast<std::size_t>(std::count_if(function.blocks.begin(),
                                                function.blocks.end(),
                                                [&](const c4c::backend::bir::Block& block) {
                                                  return block.label == block_label;
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
                              std::size_t matched_count,
                              std::optional<std::string_view> block_label = std::nullopt,
                              std::optional<std::size_t> matched_bir_blocks = std::nullopt,
                              std::optional<std::size_t> matched_prepared_blocks = std::nullopt) {
  std::ostringstream out;
  out << "focus function: " << function_name << "\n";
  if (block_label.has_value()) {
    out << "focus block: " << *block_label << "\n";
  }
  out << "focused functions matched: " << matched_count << "\n";
  if (matched_bir_blocks.has_value()) {
    out << "focused bir blocks matched: " << *matched_bir_blocks << "\n";
  }
  if (matched_prepared_blocks.has_value()) {
    out << "focused prepared blocks matched: " << *matched_prepared_blocks << "\n";
  }
  return out.str();
}

std::optional<c4c::FunctionNameId> find_prepared_function_name_id(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view function_name) {
  return c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function_name);
}

std::size_t count_matching_prepared_blocks(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view function_name,
    std::string_view block_label) {
  const auto function_name_id = find_prepared_function_name_id(module, function_name);
  if (!function_name_id.has_value()) {
    return 0;
  }
  const auto* function_cf = c4c::backend::prepare::find_prepared_control_flow_function(
      module.control_flow, *function_name_id);
  if (function_cf == nullptr) {
    return 0;
  }
  return static_cast<std::size_t>(std::count_if(
      function_cf->blocks.begin(),
      function_cf->blocks.end(),
      [&](const c4c::backend::prepare::PreparedControlFlowBlock& block) {
        return c4c::backend::prepare::prepared_block_label(module.names, block.block_label) ==
               block_label;
      }));
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

std::optional<std::size_t> find_block_index(const c4c::backend::bir::Function& function,
                                            std::string_view block_label) {
  for (std::size_t index = 0; index < function.blocks.size(); ++index) {
    if (function.blocks[index].label == block_label) {
      return index;
    }
  }
  return std::nullopt;
}

c4c::backend::prepare::PreparedBirModule filter_prepared_module_to_block(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view function_name,
    std::string_view block_label) {
  auto filtered = filter_prepared_module_to_function(module, function_name);
  if (filtered.module.functions.empty()) {
    return filtered;
  }

  auto& filtered_function = filtered.module.functions.front();
  const auto block_index = find_block_index(filtered_function, block_label);
  filtered_function = filter_bir_function_to_block(filtered_function, block_label);

  const auto function_name_id = find_prepared_function_name_id(filtered, function_name);
  const auto block_label_id =
      c4c::backend::prepare::resolve_prepared_block_label_id(filtered.names, block_label);
  if (!function_name_id.has_value() || !block_label_id.has_value()) {
    for (auto& function_cf : filtered.control_flow.functions) {
      function_cf.blocks.clear();
      function_cf.branch_conditions.clear();
      function_cf.join_transfers.clear();
      function_cf.parallel_copy_bundles.clear();
    }
    for (auto& function_locations : filtered.value_locations.functions) {
      function_locations.move_bundles.clear();
    }
    for (auto& function_addressing : filtered.addressing.functions) {
      function_addressing.accesses.clear();
    }
    return filtered;
  }

  const auto keep_block_id = *block_label_id;
  for (auto& function_cf : filtered.control_flow.functions) {
    if (function_cf.function_name != *function_name_id) {
      continue;
    }
    function_cf.blocks.erase(
        std::remove_if(function_cf.blocks.begin(),
                       function_cf.blocks.end(),
                       [&](const auto& block) { return block.block_label != keep_block_id; }),
        function_cf.blocks.end());
    function_cf.branch_conditions.erase(
        std::remove_if(function_cf.branch_conditions.begin(),
                       function_cf.branch_conditions.end(),
                       [&](const auto& condition) {
                         return condition.block_label != keep_block_id;
                       }),
        function_cf.branch_conditions.end());
    function_cf.join_transfers.erase(
        std::remove_if(function_cf.join_transfers.begin(),
                       function_cf.join_transfers.end(),
                       [&](const auto& transfer) {
                         if (transfer.join_block_label == keep_block_id) {
                           return false;
                         }
                         if (transfer.source_branch_block_label.has_value() &&
                             *transfer.source_branch_block_label == keep_block_id) {
                           return false;
                         }
                         return std::none_of(
                             transfer.edge_transfers.begin(),
                             transfer.edge_transfers.end(),
                             [&](const auto& edge) {
                               return edge.predecessor_label == keep_block_id ||
                                      edge.successor_label == keep_block_id;
                             });
                       }),
        function_cf.join_transfers.end());
    function_cf.parallel_copy_bundles.erase(
        std::remove_if(function_cf.parallel_copy_bundles.begin(),
                       function_cf.parallel_copy_bundles.end(),
                       [&](const auto& bundle) {
                         return bundle.predecessor_label != keep_block_id &&
                                bundle.successor_label != keep_block_id;
                       }),
        function_cf.parallel_copy_bundles.end());
  }

  for (auto& function_locations : filtered.value_locations.functions) {
    if (function_locations.function_name != *function_name_id) {
      continue;
    }
    if (!block_index.has_value()) {
      function_locations.move_bundles.clear();
      continue;
    }
    function_locations.move_bundles.erase(
        std::remove_if(function_locations.move_bundles.begin(),
                       function_locations.move_bundles.end(),
                       [&](const auto& bundle) { return bundle.block_index != *block_index; }),
        function_locations.move_bundles.end());
  }

  for (auto& function_addressing : filtered.addressing.functions) {
    if (function_addressing.function_name != *function_name_id) {
      continue;
    }
    function_addressing.accesses.erase(
        std::remove_if(function_addressing.accesses.begin(),
                       function_addressing.accesses.end(),
                       [&](const auto& access) { return access.block_label != keep_block_id; }),
        function_addressing.accesses.end());
  }

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
    const auto focused_function =
        options.route_debug_focus_function.has_value()
            ? find_function(input.bir_module(), *options.route_debug_focus_function)
            : nullptr;
    const auto matched_bir_blocks =
        options.route_debug_focus_block.has_value() && focused_function != nullptr
            ? count_matching_blocks(*focused_function, *options.route_debug_focus_block)
            : 0;
    if (stage == BackendDumpStage::SemanticBir) {
      if (!options.route_debug_focus_function.has_value()) {
        return c4c::backend::bir::print(input.bir_module());
      }
      auto filtered =
          filter_bir_module_to_function(input.bir_module(), *options.route_debug_focus_function);
      if (options.route_debug_focus_block.has_value() && !filtered.functions.empty()) {
        filtered.functions.front() = filter_bir_function_to_block(filtered.functions.front(),
                                                                 *options.route_debug_focus_block);
      }
      return make_focus_header(*options.route_debug_focus_function,
                               matched_functions,
                               options.route_debug_focus_block,
                               options.route_debug_focus_block.has_value()
                                   ? std::optional<std::size_t>(matched_bir_blocks)
                                   : std::nullopt) +
             c4c::backend::bir::print(filtered);
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
    std::size_t matched_prepared_blocks = 0;
    if (options.route_debug_focus_block.has_value()) {
      matched_prepared_blocks = count_matching_prepared_blocks(prepared,
                                                               *options.route_debug_focus_function,
                                                               *options.route_debug_focus_block);
    }
    if (options.route_debug_focus_block.has_value() && focused_function != nullptr) {
      prepared = filter_prepared_module_to_block(prepared,
                                                 *options.route_debug_focus_function,
                                                 *options.route_debug_focus_block);
    } else {
      prepared = filter_prepared_module_to_function(prepared, *options.route_debug_focus_function);
    }
    return make_focus_header(
               *options.route_debug_focus_function,
               matched_functions,
               options.route_debug_focus_block,
               options.route_debug_focus_block.has_value()
                   ? std::optional<std::size_t>(matched_bir_blocks)
                   : std::nullopt,
               options.route_debug_focus_block.has_value()
                   ? std::optional<std::size_t>(matched_prepared_blocks)
                   : std::nullopt) +
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
  const auto focused_function =
      options.route_debug_focus_function.has_value()
          ? find_function(*lowering.module, *options.route_debug_focus_function)
          : nullptr;
  const auto matched_bir_blocks =
      options.route_debug_focus_block.has_value() && focused_function != nullptr
          ? count_matching_blocks(*focused_function, *options.route_debug_focus_block)
          : 0;

  if (stage == BackendDumpStage::SemanticBir) {
    if (!options.route_debug_focus_function.has_value()) {
      return c4c::backend::bir::print(*lowering.module);
    }
    auto filtered =
        filter_bir_module_to_function(*lowering.module, *options.route_debug_focus_function);
    if (options.route_debug_focus_block.has_value() && !filtered.functions.empty()) {
      filtered.functions.front() = filter_bir_function_to_block(filtered.functions.front(),
                                                               *options.route_debug_focus_block);
    }
    return make_focus_header(*options.route_debug_focus_function,
                             matched_functions,
                             options.route_debug_focus_block,
                             options.route_debug_focus_block.has_value()
                                 ? std::optional<std::size_t>(matched_bir_blocks)
                                 : std::nullopt) +
           c4c::backend::bir::print(filtered);
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
  std::size_t matched_prepared_blocks = 0;
  if (options.route_debug_focus_block.has_value()) {
    matched_prepared_blocks = count_matching_prepared_blocks(prepared,
                                                             *options.route_debug_focus_function,
                                                             *options.route_debug_focus_block);
  }
  if (options.route_debug_focus_block.has_value() && focused_function != nullptr) {
    prepared = filter_prepared_module_to_block(prepared,
                                               *options.route_debug_focus_function,
                                               *options.route_debug_focus_block);
  } else {
    prepared = filter_prepared_module_to_function(prepared, *options.route_debug_focus_function);
  }
  return make_focus_header(*options.route_debug_focus_function,
                           matched_functions,
                           options.route_debug_focus_block,
                           options.route_debug_focus_block.has_value()
                               ? std::optional<std::size_t>(matched_bir_blocks)
                               : std::nullopt,
                           options.route_debug_focus_block.has_value()
                               ? std::optional<std::size_t>(matched_prepared_blocks)
                               : std::nullopt) +
         c4c::backend::prepare::print(prepared);
}

}  // namespace c4c::backend
