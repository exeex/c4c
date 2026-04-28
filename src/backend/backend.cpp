#include "backend.hpp"

#include "bir/bir.hpp"
#include "mir/x86/api/api.hpp"
#include "mir/x86/x86.hpp"
#include "prealloc/prepared_printer.hpp"

#include "../codegen/lir/ir.hpp"

#include <algorithm>
#include <optional>
#include <sstream>
#include <type_traits>
#include <unordered_set>

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

void require_x86_module_entry_target(const c4c::TargetProfile& target_profile,
                                     std::string_view api_name) {
  if (is_x86_target(target_profile)) {
    return;
  }
  throw std::invalid_argument(std::string(api_name) +
                              " requires an x86 target profile");
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

bool dump_stage_uses_target_local_route_debug(c4c::backend::BackendDumpStage stage) {
  switch (stage) {
    case c4c::backend::BackendDumpStage::SemanticBir:
    case c4c::backend::BackendDumpStage::PreparedBir:
      return false;
    case c4c::backend::BackendDumpStage::MirSummary:
    case c4c::backend::BackendDumpStage::MirTrace:
      return true;
  }
  return false;
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
                                     std::string_view function_name);
const c4c::backend::bir::Function* find_function(const c4c::backend::bir::Module& module,
                                                 std::string_view function_name);
std::size_t count_matching_blocks(const c4c::backend::bir::Function& function,
                                  std::string_view block_label);
c4c::backend::bir::Module filter_bir_module_to_function(
    const c4c::backend::bir::Module& module,
    std::string_view function_name);
c4c::backend::bir::Function filter_bir_function_to_block(
    const c4c::backend::bir::Function& function,
    std::string_view block_label);
std::size_t count_matching_bir_value_lines(const c4c::backend::bir::Function& function,
                                           std::string_view focus_value);
c4c::backend::bir::Function filter_bir_function_to_value(
    const c4c::backend::bir::Function& function,
    std::string_view focus_value);
std::string make_focus_header(std::string_view function_name,
                              std::size_t matched_count,
                              std::optional<std::string_view> block_label,
                              std::optional<std::size_t> matched_bir_blocks,
                              std::optional<std::size_t> matched_prepared_blocks,
                              std::optional<std::string_view> value_name,
                              std::optional<std::size_t> matched_semantic_value_lines,
                              std::optional<std::size_t> matched_prepared_value_lines);
std::size_t count_matching_prepared_blocks(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view function_name,
    std::string_view block_label);
c4c::backend::prepare::PreparedBirModule filter_prepared_module_to_block(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view function_name,
    std::string_view block_label);
c4c::backend::prepare::PreparedBirModule filter_prepared_module_to_function(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view function_name);
std::size_t count_matching_prepared_value_lines(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view focus_value);
c4c::backend::prepare::PreparedBirModule filter_prepared_module_to_value(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view focus_value);

std::string dump_target_local_prepared_mir(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::BackendOptions& options,
    c4c::backend::BackendDumpStage stage) {
  switch (stage) {
    case c4c::backend::BackendDumpStage::MirSummary:
      return c4c::backend::x86::summarize_prepared_module_routes(
          prepared,
          options.route_debug_focus_function,
          options.route_debug_focus_block,
          options.route_debug_focus_value);
    case c4c::backend::BackendDumpStage::MirTrace:
      return c4c::backend::x86::trace_prepared_module_routes(
          prepared,
          options.route_debug_focus_function,
          options.route_debug_focus_block,
          options.route_debug_focus_value);
    case c4c::backend::BackendDumpStage::SemanticBir:
    case c4c::backend::BackendDumpStage::PreparedBir:
      break;
  }
  throw std::invalid_argument("target-local MIR dump requested for a generic backend dump stage");
}

std::string dump_generic_semantic_bir(
    const c4c::backend::bir::Module& module,
    const c4c::backend::BackendOptions& options) {
  const auto matched_functions =
      options.route_debug_focus_function.has_value()
          ? count_matching_functions(module, *options.route_debug_focus_function)
          : 0;
  const auto focused_function =
      options.route_debug_focus_function.has_value()
          ? find_function(module, *options.route_debug_focus_function)
          : nullptr;
  const auto matched_bir_blocks =
      options.route_debug_focus_block.has_value() && focused_function != nullptr
          ? count_matching_blocks(*focused_function, *options.route_debug_focus_block)
          : 0;
  if (!options.route_debug_focus_function.has_value()) {
    return c4c::backend::bir::print(module);
  }
  auto filtered = filter_bir_module_to_function(module, *options.route_debug_focus_function);
  if (options.route_debug_focus_block.has_value() && !filtered.functions.empty()) {
    filtered.functions.front() =
        filter_bir_function_to_block(filtered.functions.front(), *options.route_debug_focus_block);
  }
  std::optional<std::size_t> matched_semantic_value_lines;
  if (options.route_debug_focus_value.has_value() && !filtered.functions.empty()) {
    matched_semantic_value_lines =
        count_matching_bir_value_lines(filtered.functions.front(), *options.route_debug_focus_value);
    filtered.functions.front() =
        filter_bir_function_to_value(filtered.functions.front(), *options.route_debug_focus_value);
  } else if (options.route_debug_focus_value.has_value()) {
    matched_semantic_value_lines = 0;
  }
  return make_focus_header(*options.route_debug_focus_function,
                           matched_functions,
                           options.route_debug_focus_block,
                           options.route_debug_focus_block.has_value()
                               ? std::optional<std::size_t>(matched_bir_blocks)
                               : std::nullopt,
                           std::nullopt,
                           options.route_debug_focus_value,
                           matched_semantic_value_lines,
                           std::nullopt) +
         c4c::backend::bir::print(filtered);
}

std::string dump_generic_prepared_bir(
    c4c::backend::prepare::PreparedBirModule prepared,
    const c4c::backend::BackendOptions& options) {
  const auto matched_functions =
      options.route_debug_focus_function.has_value()
          ? count_matching_functions(prepared.module, *options.route_debug_focus_function)
          : 0;
  const auto focused_function =
      options.route_debug_focus_function.has_value()
          ? find_function(prepared.module, *options.route_debug_focus_function)
          : nullptr;
  const auto matched_bir_blocks =
      options.route_debug_focus_block.has_value() && focused_function != nullptr
          ? count_matching_blocks(*focused_function, *options.route_debug_focus_block)
          : 0;
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
  std::optional<std::size_t> matched_semantic_value_lines;
  std::optional<std::size_t> matched_prepared_value_lines;
  if (options.route_debug_focus_value.has_value()) {
    matched_prepared_value_lines =
        count_matching_prepared_value_lines(prepared, *options.route_debug_focus_value);
    if (!prepared.module.functions.empty()) {
      matched_semantic_value_lines = count_matching_bir_value_lines(
          prepared.module.functions.front(), *options.route_debug_focus_value);
    } else {
      matched_semantic_value_lines = 0;
    }
    prepared = filter_prepared_module_to_value(prepared, *options.route_debug_focus_value);
  }
  return make_focus_header(*options.route_debug_focus_function,
                           matched_functions,
                           options.route_debug_focus_block,
                           options.route_debug_focus_block.has_value()
                               ? std::optional<std::size_t>(matched_bir_blocks)
                               : std::nullopt,
                           options.route_debug_focus_block.has_value()
                               ? std::optional<std::size_t>(matched_prepared_blocks)
                               : std::nullopt,
                           options.route_debug_focus_value,
                           matched_semantic_value_lines,
                           matched_prepared_value_lines) +
         c4c::backend::prepare::print(prepared);
}

bool named_value_matches(const c4c::backend::bir::Value& value,
                         std::string_view focus_value) {
  return value.kind == c4c::backend::bir::Value::Kind::Named && value.name == focus_value;
}

bool memory_address_mentions_value(
    const std::optional<c4c::backend::bir::MemoryAddress>& address,
    std::string_view focus_value) {
  if (!address.has_value()) {
    return false;
  }
  return address->base_name == focus_value || named_value_matches(address->base_value, focus_value);
}

bool bir_instruction_mentions_value(const c4c::backend::bir::Inst& inst,
                                    std::string_view focus_value) {
  return std::visit(
      [&](const auto& lowered) {
        using Inst = std::decay_t<decltype(lowered)>;
        if constexpr (std::is_same_v<Inst, c4c::backend::bir::BinaryInst>) {
          return named_value_matches(lowered.result, focus_value) ||
                 named_value_matches(lowered.lhs, focus_value) ||
                 named_value_matches(lowered.rhs, focus_value);
        } else if constexpr (std::is_same_v<Inst, c4c::backend::bir::SelectInst>) {
          return named_value_matches(lowered.result, focus_value) ||
                 named_value_matches(lowered.lhs, focus_value) ||
                 named_value_matches(lowered.rhs, focus_value) ||
                 named_value_matches(lowered.true_value, focus_value) ||
                 named_value_matches(lowered.false_value, focus_value);
        } else if constexpr (std::is_same_v<Inst, c4c::backend::bir::CastInst>) {
          return named_value_matches(lowered.result, focus_value) ||
                 named_value_matches(lowered.operand, focus_value);
        } else if constexpr (std::is_same_v<Inst, c4c::backend::bir::PhiInst>) {
          if (named_value_matches(lowered.result, focus_value)) {
            return true;
          }
          return std::any_of(
              lowered.incomings.begin(),
              lowered.incomings.end(),
              [&](const c4c::backend::bir::PhiIncoming& incoming) {
                return named_value_matches(incoming.value, focus_value);
              });
        } else if constexpr (std::is_same_v<Inst, c4c::backend::bir::CallInst>) {
          if (lowered.result.has_value() && named_value_matches(*lowered.result, focus_value)) {
            return true;
          }
          if (lowered.callee_value.has_value() &&
              named_value_matches(*lowered.callee_value, focus_value)) {
            return true;
          }
          return std::any_of(lowered.args.begin(),
                             lowered.args.end(),
                             [&](const c4c::backend::bir::Value& arg) {
                               return named_value_matches(arg, focus_value);
                             });
        } else if constexpr (std::is_same_v<Inst, c4c::backend::bir::LoadLocalInst>) {
          return named_value_matches(lowered.result, focus_value) ||
                 lowered.slot_name == focus_value ||
                 memory_address_mentions_value(lowered.address, focus_value);
        } else if constexpr (std::is_same_v<Inst, c4c::backend::bir::LoadGlobalInst>) {
          return named_value_matches(lowered.result, focus_value) ||
                 lowered.global_name == focus_value ||
                 memory_address_mentions_value(lowered.address, focus_value);
        } else if constexpr (std::is_same_v<Inst, c4c::backend::bir::StoreGlobalInst>) {
          return lowered.global_name == focus_value ||
                 named_value_matches(lowered.value, focus_value) ||
                 memory_address_mentions_value(lowered.address, focus_value);
        } else if constexpr (std::is_same_v<Inst, c4c::backend::bir::StoreLocalInst>) {
          return lowered.slot_name == focus_value ||
                 named_value_matches(lowered.value, focus_value) ||
                 memory_address_mentions_value(lowered.address, focus_value);
        }
        return false;
      },
      inst);
}

bool bir_terminator_mentions_value(const c4c::backend::bir::Terminator& terminator,
                                   std::string_view focus_value) {
  return (terminator.value.has_value() && named_value_matches(*terminator.value, focus_value)) ||
         named_value_matches(terminator.condition, focus_value);
}

std::size_t count_matching_bir_value_lines(const c4c::backend::bir::Function& function,
                                           std::string_view focus_value) {
  std::size_t matched_lines = 0;
  for (const auto& block : function.blocks) {
    matched_lines += static_cast<std::size_t>(std::count_if(
        block.insts.begin(),
        block.insts.end(),
        [&](const c4c::backend::bir::Inst& inst) {
          return bir_instruction_mentions_value(inst, focus_value);
        }));
    if (bir_terminator_mentions_value(block.terminator, focus_value)) {
      ++matched_lines;
    }
  }
  return matched_lines;
}

c4c::backend::bir::Function filter_bir_function_to_value(
    const c4c::backend::bir::Function& function,
    std::string_view focus_value) {
  c4c::backend::bir::Function filtered = function;
  filtered.blocks.clear();
  for (const auto& block : function.blocks) {
    c4c::backend::bir::Block filtered_block = block;
    filtered_block.insts.erase(
        std::remove_if(filtered_block.insts.begin(),
                       filtered_block.insts.end(),
                       [&](const c4c::backend::bir::Inst& inst) {
                         return !bir_instruction_mentions_value(inst, focus_value);
                       }),
        filtered_block.insts.end());
    if (filtered_block.insts.empty() &&
        !bir_terminator_mentions_value(block.terminator, focus_value)) {
      continue;
    }
    filtered.blocks.push_back(std::move(filtered_block));
  }
  return filtered;
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
                              std::optional<std::size_t> matched_prepared_blocks = std::nullopt,
                              std::optional<std::string_view> value_name = std::nullopt,
                              std::optional<std::size_t> matched_semantic_value_lines =
                                  std::nullopt,
                              std::optional<std::size_t> matched_prepared_value_lines =
                                  std::nullopt) {
  std::ostringstream out;
  out << "focus function: " << function_name << "\n";
  if (block_label.has_value()) {
    out << "focus block: " << *block_label << "\n";
  }
  if (value_name.has_value()) {
    out << "focus value: " << *value_name << "\n";
  }
  out << "focused functions matched: " << matched_count << "\n";
  if (matched_bir_blocks.has_value()) {
    out << "focused bir blocks matched: " << *matched_bir_blocks << "\n";
  }
  if (matched_prepared_blocks.has_value()) {
    out << "focused prepared blocks matched: " << *matched_prepared_blocks << "\n";
  }
  if (matched_semantic_value_lines.has_value()) {
    out << "focused semantic value-owned lines matched: "
        << *matched_semantic_value_lines << "\n";
  }
  if (matched_prepared_value_lines.has_value()) {
    out << "focused prepared value-owned lines matched: "
        << *matched_prepared_value_lines << "\n";
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

bool prepared_branch_condition_mentions_value(
    const c4c::backend::prepare::PreparedBranchCondition& condition,
    std::string_view focus_value) {
  return named_value_matches(condition.condition_value, focus_value) ||
         (condition.lhs.has_value() && named_value_matches(*condition.lhs, focus_value)) ||
         (condition.rhs.has_value() && named_value_matches(*condition.rhs, focus_value));
}

bool prepared_join_transfer_mentions_value(
    const c4c::backend::prepare::PreparedJoinTransfer& transfer,
    std::string_view focus_value) {
  if (named_value_matches(transfer.result, focus_value)) {
    return true;
  }
  if (std::any_of(
          transfer.incomings.begin(),
          transfer.incomings.end(),
          [&](const c4c::backend::bir::PhiIncoming& incoming) {
            return named_value_matches(incoming.value, focus_value);
          })) {
    return true;
  }
  return std::any_of(
      transfer.edge_transfers.begin(),
      transfer.edge_transfers.end(),
      [&](const auto& edge) {
        return named_value_matches(edge.incoming_value, focus_value) ||
               named_value_matches(edge.destination_value, focus_value);
      });
}

bool prepared_parallel_copy_move_mentions_value(
    const c4c::backend::prepare::PreparedParallelCopyMove& move,
    std::string_view focus_value) {
  return named_value_matches(move.source_value, focus_value) ||
         named_value_matches(move.destination_value, focus_value);
}

bool prepared_access_mentions_value(
    const c4c::backend::prepare::PreparedMemoryAccess& access,
    c4c::ValueNameId focus_value_id) {
  return access.result_value_name == focus_value_id ||
         access.stored_value_name == focus_value_id ||
         access.address.pointer_value_name == focus_value_id;
}

bool prepared_call_argument_mentions_value(
    const c4c::backend::prepare::PreparedCallArgumentPlan& argument,
    c4c::ValueNameId focus_value_id,
    const std::unordered_set<c4c::backend::prepare::PreparedValueId>& focus_value_ids) {
  return (argument.source_value_id.has_value() &&
          focus_value_ids.count(*argument.source_value_id) != 0) ||
         (argument.source_base_value_id.has_value() &&
          focus_value_ids.count(*argument.source_base_value_id) != 0) ||
         argument.source_base_value_name == focus_value_id;
}

bool prepared_call_result_mentions_value(
    const c4c::backend::prepare::PreparedCallResultPlan& result,
    const std::unordered_set<c4c::backend::prepare::PreparedValueId>& focus_value_ids) {
  return result.destination_value_id.has_value() &&
         focus_value_ids.count(*result.destination_value_id) != 0;
}

bool prepared_call_preserved_value_mentions_value(
    const c4c::backend::prepare::PreparedCallPreservedValue& preserved,
    c4c::ValueNameId focus_value_id,
    const std::unordered_set<c4c::backend::prepare::PreparedValueId>& focus_value_ids) {
  return preserved.value_name == focus_value_id ||
         focus_value_ids.count(preserved.value_id) != 0;
}

bool prepared_indirect_callee_mentions_value(
    const c4c::backend::prepare::PreparedIndirectCalleePlan& callee,
    c4c::ValueNameId focus_value_id,
    const std::unordered_set<c4c::backend::prepare::PreparedValueId>& focus_value_ids) {
  return (callee.value_id.has_value() && focus_value_ids.count(*callee.value_id) != 0) ||
         callee.pointer_base_value_name == focus_value_id;
}

bool prepared_call_plan_mentions_value(
    const c4c::backend::prepare::PreparedCallPlan& call,
    c4c::ValueNameId focus_value_id,
    const std::unordered_set<c4c::backend::prepare::PreparedValueId>& focus_value_ids) {
  if (call.indirect_callee.has_value() &&
      prepared_indirect_callee_mentions_value(*call.indirect_callee, focus_value_id, focus_value_ids)) {
    return true;
  }
  if (call.result.has_value() && prepared_call_result_mentions_value(*call.result, focus_value_ids)) {
    return true;
  }
  if (std::any_of(call.arguments.begin(),
                  call.arguments.end(),
                  [&](const c4c::backend::prepare::PreparedCallArgumentPlan& argument) {
                    return prepared_call_argument_mentions_value(argument, focus_value_id, focus_value_ids);
                  })) {
    return true;
  }
  return std::any_of(call.preserved_values.begin(),
                     call.preserved_values.end(),
                     [&](const c4c::backend::prepare::PreparedCallPreservedValue& preserved) {
                       return prepared_call_preserved_value_mentions_value(
                           preserved, focus_value_id, focus_value_ids);
                     });
}

bool prepared_storage_plan_value_mentions_value(
    const c4c::backend::prepare::PreparedStoragePlanValue& value,
    c4c::ValueNameId focus_value_id,
    const std::unordered_set<c4c::backend::prepare::PreparedValueId>& focus_value_ids) {
  return value.value_name == focus_value_id || focus_value_ids.count(value.value_id) != 0;
}

std::unordered_set<c4c::backend::prepare::PreparedValueId> collect_prepared_value_ids(
    const c4c::backend::prepare::PreparedBirModule& module,
    c4c::ValueNameId focus_value_id) {
  std::unordered_set<c4c::backend::prepare::PreparedValueId> value_ids;
  for (const auto& function_locations : module.value_locations.functions) {
    for (const auto& home : function_locations.value_homes) {
      if (home.value_name == focus_value_id) {
        value_ids.insert(home.value_id);
      }
    }
  }
  return value_ids;
}

std::size_t count_matching_prepared_value_lines(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view focus_value) {
  std::size_t matched_lines = 0;
  for (const auto& function : module.module.functions) {
    matched_lines += count_matching_bir_value_lines(function, focus_value);
  }

  const auto focus_value_id =
      c4c::backend::prepare::resolve_prepared_value_name_id(module.names, focus_value);
  if (!focus_value_id.has_value()) {
    return matched_lines;
  }
  const auto focus_value_ids = collect_prepared_value_ids(module, *focus_value_id);

  for (const auto& function_cf : module.control_flow.functions) {
    matched_lines += static_cast<std::size_t>(std::count_if(
        function_cf.branch_conditions.begin(),
        function_cf.branch_conditions.end(),
        [&](const c4c::backend::prepare::PreparedBranchCondition& condition) {
          return prepared_branch_condition_mentions_value(condition, focus_value);
        }));
    matched_lines += static_cast<std::size_t>(std::count_if(
        function_cf.join_transfers.begin(),
        function_cf.join_transfers.end(),
        [&](const c4c::backend::prepare::PreparedJoinTransfer& transfer) {
          return prepared_join_transfer_mentions_value(transfer, focus_value);
        }));
    for (const auto& bundle : function_cf.parallel_copy_bundles) {
      matched_lines += static_cast<std::size_t>(std::count_if(
          bundle.moves.begin(),
          bundle.moves.end(),
          [&](const c4c::backend::prepare::PreparedParallelCopyMove& move) {
            return prepared_parallel_copy_move_mentions_value(move, focus_value);
          }));
    }
  }

  std::unordered_set<c4c::backend::prepare::PreparedObjectId> matched_object_ids;
  for (const auto& function_locations : module.value_locations.functions) {
    matched_lines += static_cast<std::size_t>(std::count_if(
        function_locations.value_homes.begin(),
        function_locations.value_homes.end(),
        [&](const c4c::backend::prepare::PreparedValueHome& home) {
          return home.value_name == *focus_value_id;
        }));
    for (const auto& bundle : function_locations.move_bundles) {
      matched_lines += static_cast<std::size_t>(std::count_if(
          bundle.moves.begin(),
          bundle.moves.end(),
          [&](const c4c::backend::prepare::PreparedMoveResolution& move) {
            return focus_value_ids.count(move.from_value_id) != 0 ||
                   focus_value_ids.count(move.to_value_id) != 0;
          }));
    }
  }

  for (const auto& object : module.stack_layout.objects) {
    const bool object_matches =
        object.value_name == *focus_value_id ||
        c4c::backend::prepare::prepared_stack_object_name(module.names, object) == focus_value;
    if (!object_matches) {
      continue;
    }
    ++matched_lines;
    matched_object_ids.insert(object.object_id);
  }
  matched_lines += static_cast<std::size_t>(std::count_if(
      module.stack_layout.frame_slots.begin(),
      module.stack_layout.frame_slots.end(),
      [&](const c4c::backend::prepare::PreparedFrameSlot& slot) {
        return matched_object_ids.count(slot.object_id) != 0;
      }));

  for (const auto& function_addressing : module.addressing.functions) {
    matched_lines += static_cast<std::size_t>(std::count_if(
        function_addressing.accesses.begin(),
        function_addressing.accesses.end(),
        [&](const c4c::backend::prepare::PreparedMemoryAccess& access) {
          return prepared_access_mentions_value(access, *focus_value_id);
        }));
  }
  for (const auto& function_plan : module.storage_plans.functions) {
    matched_lines += static_cast<std::size_t>(std::count_if(
        function_plan.values.begin(),
        function_plan.values.end(),
        [&](const c4c::backend::prepare::PreparedStoragePlanValue& value) {
          return prepared_storage_plan_value_mentions_value(value, *focus_value_id, focus_value_ids);
        }));
  }

  for (const auto& function_plan : module.call_plans.functions) {
    for (const auto& call : function_plan.calls) {
      if (!prepared_call_plan_mentions_value(call, *focus_value_id, focus_value_ids)) {
        continue;
      }
      ++matched_lines;
      matched_lines += static_cast<std::size_t>(std::count_if(
          call.arguments.begin(),
          call.arguments.end(),
          [&](const c4c::backend::prepare::PreparedCallArgumentPlan& argument) {
            return prepared_call_argument_mentions_value(argument, *focus_value_id, focus_value_ids);
          }));
      if (call.result.has_value() && prepared_call_result_mentions_value(*call.result, focus_value_ids)) {
        ++matched_lines;
      }
      matched_lines += static_cast<std::size_t>(std::count_if(
          call.preserved_values.begin(),
          call.preserved_values.end(),
          [&](const c4c::backend::prepare::PreparedCallPreservedValue& preserved) {
            return prepared_call_preserved_value_mentions_value(
                preserved, *focus_value_id, focus_value_ids);
          }));
    }
  }

  return matched_lines;
}

c4c::backend::prepare::PreparedBirModule filter_prepared_module_to_value(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view focus_value) {
  auto filtered = module;
  for (auto& function : filtered.module.functions) {
    function = filter_bir_function_to_value(function, focus_value);
  }

  const auto focus_value_id =
      c4c::backend::prepare::resolve_prepared_value_name_id(filtered.names, focus_value);
  if (!focus_value_id.has_value()) {
    for (auto& function_cf : filtered.control_flow.functions) {
      function_cf.blocks.clear();
      function_cf.branch_conditions.clear();
      function_cf.join_transfers.clear();
      function_cf.parallel_copy_bundles.clear();
    }
    for (auto& function_locations : filtered.value_locations.functions) {
      function_locations.value_homes.clear();
      function_locations.move_bundles.clear();
    }
    filtered.stack_layout.objects.clear();
    filtered.stack_layout.frame_slots.clear();
    for (auto& function_addressing : filtered.addressing.functions) {
      function_addressing.accesses.clear();
    }
    return filtered;
  }

  const auto focus_value_ids = collect_prepared_value_ids(filtered, *focus_value_id);
  std::unordered_set<c4c::backend::prepare::PreparedObjectId> matched_object_ids;

  for (auto& function_cf : filtered.control_flow.functions) {
    std::unordered_set<c4c::BlockLabelId> keep_block_ids;
    const auto function_name =
        c4c::backend::prepare::prepared_function_name(filtered.names, function_cf.function_name);
    if (const auto* filtered_function = find_function(filtered.module, function_name);
        filtered_function != nullptr) {
      for (const auto& block : filtered_function->blocks) {
        const auto block_label_id =
            c4c::backend::prepare::resolve_prepared_block_label_id(filtered.names, block.label);
        if (block_label_id.has_value()) {
          keep_block_ids.insert(*block_label_id);
        }
      }
    }

    function_cf.branch_conditions.erase(
        std::remove_if(function_cf.branch_conditions.begin(),
                       function_cf.branch_conditions.end(),
                       [&](const c4c::backend::prepare::PreparedBranchCondition& condition) {
                         if (!prepared_branch_condition_mentions_value(condition, focus_value)) {
                           return true;
                         }
                         keep_block_ids.insert(condition.block_label);
                         keep_block_ids.insert(condition.true_label);
                         keep_block_ids.insert(condition.false_label);
                         return false;
                       }),
        function_cf.branch_conditions.end());

    function_cf.join_transfers.erase(
        std::remove_if(function_cf.join_transfers.begin(),
                       function_cf.join_transfers.end(),
                       [&](const c4c::backend::prepare::PreparedJoinTransfer& transfer) {
                         if (!prepared_join_transfer_mentions_value(transfer, focus_value)) {
                           return true;
                         }
                         keep_block_ids.insert(transfer.join_block_label);
                         if (transfer.source_branch_block_label.has_value()) {
                           keep_block_ids.insert(*transfer.source_branch_block_label);
                         }
                         if (transfer.source_true_incoming_label.has_value()) {
                           keep_block_ids.insert(*transfer.source_true_incoming_label);
                         }
                         if (transfer.source_false_incoming_label.has_value()) {
                           keep_block_ids.insert(*transfer.source_false_incoming_label);
                         }
                         for (const auto& edge : transfer.edge_transfers) {
                           keep_block_ids.insert(edge.predecessor_label);
                           keep_block_ids.insert(edge.successor_label);
                         }
                         return false;
                       }),
        function_cf.join_transfers.end());

    function_cf.parallel_copy_bundles.erase(
        std::remove_if(
            function_cf.parallel_copy_bundles.begin(),
            function_cf.parallel_copy_bundles.end(),
            [&](c4c::backend::prepare::PreparedParallelCopyBundle& bundle) {
              std::vector<c4c::backend::prepare::PreparedParallelCopyMove> kept_moves;
              kept_moves.reserve(bundle.moves.size());
              std::vector<std::size_t> move_index_map(bundle.moves.size(),
                                                      static_cast<std::size_t>(-1));
              for (std::size_t move_index = 0; move_index < bundle.moves.size(); ++move_index) {
                if (!prepared_parallel_copy_move_mentions_value(bundle.moves[move_index],
                                                                focus_value)) {
                  continue;
                }
                move_index_map[move_index] = kept_moves.size();
                kept_moves.push_back(bundle.moves[move_index]);
              }
              if (kept_moves.empty()) {
                return true;
              }
              keep_block_ids.insert(bundle.predecessor_label);
              keep_block_ids.insert(bundle.successor_label);
              bundle.moves = std::move(kept_moves);
              bundle.steps.erase(
                  std::remove_if(bundle.steps.begin(),
                                 bundle.steps.end(),
                                 [&](auto& step) {
                                   if (step.move_index >= move_index_map.size() ||
                                       move_index_map[step.move_index] ==
                                           static_cast<std::size_t>(-1)) {
                                     return true;
                                   }
                                   step.move_index = move_index_map[step.move_index];
                                   return false;
                                 }),
                  bundle.steps.end());
              return false;
            }),
        function_cf.parallel_copy_bundles.end());

    function_cf.blocks.erase(
        std::remove_if(function_cf.blocks.begin(),
                       function_cf.blocks.end(),
                       [&](const c4c::backend::prepare::PreparedControlFlowBlock& block) {
                         return keep_block_ids.count(block.block_label) == 0;
                       }),
        function_cf.blocks.end());
  }

  for (auto& function_locations : filtered.value_locations.functions) {
    function_locations.value_homes.erase(
        std::remove_if(function_locations.value_homes.begin(),
                       function_locations.value_homes.end(),
                       [&](const c4c::backend::prepare::PreparedValueHome& home) {
                         return home.value_name != *focus_value_id;
                       }),
        function_locations.value_homes.end());
    function_locations.move_bundles.erase(
        std::remove_if(
            function_locations.move_bundles.begin(),
            function_locations.move_bundles.end(),
            [&](c4c::backend::prepare::PreparedMoveBundle& bundle) {
              bundle.moves.erase(
                  std::remove_if(
                      bundle.moves.begin(),
                      bundle.moves.end(),
                      [&](const c4c::backend::prepare::PreparedMoveResolution& move) {
                        return focus_value_ids.count(move.from_value_id) == 0 &&
                               focus_value_ids.count(move.to_value_id) == 0;
                      }),
                  bundle.moves.end());
              bundle.abi_bindings.clear();
              return bundle.moves.empty();
            }),
        function_locations.move_bundles.end());
  }

  filtered.stack_layout.objects.erase(
      std::remove_if(filtered.stack_layout.objects.begin(),
                     filtered.stack_layout.objects.end(),
                     [&](const c4c::backend::prepare::PreparedStackObject& object) {
                       const bool keep_object =
                           object.value_name == *focus_value_id ||
                           c4c::backend::prepare::prepared_stack_object_name(filtered.names,
                                                                            object) ==
                               focus_value;
                       if (keep_object) {
                         matched_object_ids.insert(object.object_id);
                       }
                       return !keep_object;
                     }),
      filtered.stack_layout.objects.end());
  filtered.stack_layout.frame_slots.erase(
      std::remove_if(filtered.stack_layout.frame_slots.begin(),
                     filtered.stack_layout.frame_slots.end(),
                     [&](const c4c::backend::prepare::PreparedFrameSlot& slot) {
                       return matched_object_ids.count(slot.object_id) == 0;
                     }),
      filtered.stack_layout.frame_slots.end());

  for (auto& function_addressing : filtered.addressing.functions) {
    function_addressing.accesses.erase(
        std::remove_if(function_addressing.accesses.begin(),
                       function_addressing.accesses.end(),
                       [&](const c4c::backend::prepare::PreparedMemoryAccess& access) {
                         return !prepared_access_mentions_value(access, *focus_value_id);
                       }),
        function_addressing.accesses.end());
  }

  filtered.call_plans.functions.erase(
      std::remove_if(
          filtered.call_plans.functions.begin(),
          filtered.call_plans.functions.end(),
          [&](c4c::backend::prepare::PreparedCallPlansFunction& function_plan) {
            function_plan.calls.erase(
                std::remove_if(
                    function_plan.calls.begin(),
                    function_plan.calls.end(),
                    [&](c4c::backend::prepare::PreparedCallPlan& call) {
                      if (!prepared_call_plan_mentions_value(call, *focus_value_id, focus_value_ids)) {
                        return true;
                      }
                      call.arguments.erase(
                          std::remove_if(
                              call.arguments.begin(),
                              call.arguments.end(),
                              [&](const c4c::backend::prepare::PreparedCallArgumentPlan& argument) {
                                return !prepared_call_argument_mentions_value(
                                    argument, *focus_value_id, focus_value_ids);
                              }),
                          call.arguments.end());
                      if (call.result.has_value() &&
                          !prepared_call_result_mentions_value(*call.result, focus_value_ids)) {
                        call.result.reset();
                      }
                      call.preserved_values.erase(
                          std::remove_if(
                              call.preserved_values.begin(),
                              call.preserved_values.end(),
                              [&](const c4c::backend::prepare::PreparedCallPreservedValue& preserved) {
                                return !prepared_call_preserved_value_mentions_value(
                                    preserved, *focus_value_id, focus_value_ids);
                              }),
                          call.preserved_values.end());
                      return false;
                    }),
                function_plan.calls.end());
            return function_plan.calls.empty();
          }),
      filtered.call_plans.functions.end());

  filtered.storage_plans.functions.erase(
      std::remove_if(
          filtered.storage_plans.functions.begin(),
          filtered.storage_plans.functions.end(),
          [&](c4c::backend::prepare::PreparedStoragePlanFunction& function_plan) {
            function_plan.values.erase(
                std::remove_if(
                    function_plan.values.begin(),
                    function_plan.values.end(),
                    [&](const c4c::backend::prepare::PreparedStoragePlanValue& value) {
                      return !prepared_storage_plan_value_mentions_value(
                          value, *focus_value_id, focus_value_ids);
                    }),
                function_plan.values.end());
            return function_plan.values.empty();
          }),
      filtered.storage_plans.functions.end());

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

std::string emit_x86_bir_module_entry(const bir::Module& module,
                                      const c4c::TargetProfile& target_profile) {
  require_x86_module_entry_target(target_profile, "emit_x86_bir_module_entry");
  const auto prepared = prepare_semantic_bir_pipeline(module, target_profile);
  return c4c::backend::x86::api::emit_prepared_module(prepared);
}

std::string emit_target_bir_module(const bir::Module& module,
                                   const c4c::TargetProfile& target_profile) {
  if (is_x86_target(target_profile)) {
    return emit_x86_bir_module_entry(module, target_profile);
  }

  const auto prepared = prepare_semantic_bir_pipeline(module, target_profile);
  return render_prepared_bir_text(prepared.module);
}

std::string emit_x86_lir_module_entry(const c4c::codegen::lir::LirModule& module,
                                      const c4c::TargetProfile& target_profile) {
  require_x86_module_entry_target(target_profile, "emit_x86_lir_module_entry");
  c4c::backend::BirLoweringOptions lowering_options{};
  auto lowering = c4c::backend::try_lower_to_bir_with_options(module, lowering_options);
  if (lowering.module.has_value()) {
    const auto prepared_bir = prepare_semantic_bir_pipeline(*lowering.module, target_profile);
    return c4c::backend::x86::api::emit_prepared_module(prepared_bir);
  }
  throw std::invalid_argument(make_x86_lir_handoff_failure_message(lowering));
}

std::string emit_target_lir_module(const c4c::codegen::lir::LirModule& module,
                                   const c4c::TargetProfile& target_profile) {
  if (is_x86_target(target_profile)) {
    return emit_x86_lir_module_entry(module, target_profile);
  }

  c4c::backend::BirLoweringOptions lowering_options{};
  auto lowering = c4c::backend::try_lower_to_bir_with_options(module, lowering_options);
  if (lowering.module.has_value()) {
    const auto prepared_bir = prepare_semantic_bir_pipeline(*lowering.module, target_profile);
    return render_prepared_bir_text(prepared_bir.module);
  }
  return emit_bootstrap_lir_module(module, target_profile);
}

BackendAssembleResult stage_x86_lir_module_entry(
    const c4c::codegen::lir::LirModule& module,
    const c4c::TargetProfile& target_profile,
    const std::string& output_path) {
  require_x86_module_entry_target(target_profile, "stage_x86_lir_module_entry");
  return BackendAssembleResult{
      .staged_text = c4c::backend::x86::api::emit_module(module, target_profile),
      .output_path = output_path,
      .object_emitted = false,
      .error = "backend bootstrap mode does not assemble objects yet",
  };
}

BackendAssembleResult assemble_target_lir_module(
    const c4c::codegen::lir::LirModule& module,
    const c4c::TargetProfile& target_profile,
    const std::string& output_path) {
  if (is_x86_target(target_profile)) {
    return stage_x86_lir_module_entry(module, target_profile, output_path);
  }

  // Generic backend assembly front door keeps the existing bootstrap assemble
  // contract for non-target-local object work.
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
    const auto& target_profile = profile_or_default(options.target_profile,
                                                    target_profile_storage,
                                                    input.bir_module().target_triple);
    if (is_x86_target(target_profile)) {
      return emit_x86_bir_module_entry(input.bir_module(), target_profile);
    }
    return emit_target_bir_module(input.bir_module(), target_profile);
  }

  const auto& lir_module = input.lir_module();
  const auto target_profile = resolve_public_lir_target_profile(lir_module, options.target_profile);

  if (!options.emit_semantic_bir && is_x86_target(target_profile)) {
    return emit_x86_lir_module_entry(lir_module, target_profile);
  }

  c4c::backend::BirLoweringOptions lowering_options{};
  lowering_options.preserve_dynamic_alloca = true;
  auto lowering = c4c::backend::try_lower_to_bir_with_options(lir_module, lowering_options);

  if (lowering.module.has_value()) {
    if (options.emit_semantic_bir) {
      return c4c::backend::bir::print(*lowering.module);
    }
    const auto prepared_bir = prepare_semantic_bir_pipeline(*lowering.module, target_profile);
    return render_prepared_bir_text(prepared_bir.module);
  }
  return emit_bootstrap_lir_module(lir_module, target_profile);
}

std::string dump_module(const BackendModuleInput& input,
                        const BackendOptions& options,
                        BackendDumpStage stage) {
  if (input.holds_bir_module()) {
    if (stage == BackendDumpStage::SemanticBir) {
      return dump_generic_semantic_bir(input.bir_module(), options);
    }
    c4c::TargetProfile target_profile_storage;
    auto prepared = prepare_semantic_bir_pipeline(
        input.bir_module(),
        profile_or_default(options.target_profile,
                           target_profile_storage,
                           input.bir_module().target_triple));
    if (dump_stage_uses_target_local_route_debug(stage)) {
      return dump_target_local_prepared_mir(prepared, options, stage);
    }
    return dump_generic_prepared_bir(std::move(prepared), options);
  }

  const auto& lir_module = input.lir_module();
  const auto target_profile = resolve_public_lir_target_profile(lir_module, options.target_profile);

  c4c::backend::BirLoweringOptions lowering_options{};
  lowering_options.preserve_dynamic_alloca = true;
  auto lowering = c4c::backend::try_lower_to_bir_with_options(lir_module, lowering_options);
  if (!lowering.module.has_value()) {
    throw std::invalid_argument(make_backend_dump_failure_message(lowering));
  }

  if (stage == BackendDumpStage::SemanticBir) {
    return dump_generic_semantic_bir(*lowering.module, options);
  }
  auto prepared = prepare_semantic_bir_pipeline(*lowering.module, target_profile);
  if (dump_stage_uses_target_local_route_debug(stage)) {
    return dump_target_local_prepared_mir(prepared, options, stage);
  }
  return dump_generic_prepared_bir(std::move(prepared), options);
}

}  // namespace c4c::backend
