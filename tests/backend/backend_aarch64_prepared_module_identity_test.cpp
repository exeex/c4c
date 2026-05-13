#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/aarch64/api/api.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <iostream>
#include <string_view>
#include <utility>

namespace {

namespace aarch64_api = c4c::backend::aarch64::api;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedBirModule prepared_identity_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("identity.fn");
  const auto entry_label = prepared.names.block_labels.intern("entry.authoritative");
  const auto exit_label = prepared.names.block_labels.intern("exit.authoritative");

  const auto function_link_name = prepared.module.names.link_names.intern("identity.fn");
  const auto entry_bir_label = prepared.module.names.block_labels.intern("entry.authoritative");
  const auto exit_bir_label = prepared.module.names.block_labels.intern("exit.authoritative");

  bir::Block entry;
  entry.label = "raw.entry.drift";
  entry.label_id = entry_bir_label;
  entry.terminator = bir::BranchTerminator{
      .target_label = "raw.exit.drift",
      .target_label_id = exit_bir_label,
  };

  bir::Block exit;
  exit.label = "raw.exit.drift";
  exit.label_id = exit_bir_label;
  exit.terminator = bir::ReturnTerminator{};

  bir::Function function;
  function.name = "raw.identity.display.drift";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::Void;
  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(exit));
  prepared.module.functions.push_back(std::move(function));

  prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_name,
      .blocks = {
          prepare::PreparedControlFlowBlock{
              .block_label = entry_label,
              .terminator_kind = bir::TerminatorKind::Branch,
              .branch_target_label = exit_label,
          },
          prepare::PreparedControlFlowBlock{
              .block_label = exit_label,
              .terminator_kind = bir::TerminatorKind::Return,
          },
      },
  };
  prepared.control_flow.functions.push_back(std::move(control_flow));

  return prepared;
}

int records_preserve_prepared_function_and_block_identities() {
  auto prepared = prepared_identity_module();

  const auto result = aarch64_api::build_prepared_module(prepared);
  if (result.error.has_value()) {
    return fail("expected AArch64 identity module to pass handoff gate");
  }
  if (!result.module.has_value()) {
    return fail("expected AArch64 identity module to build");
  }

  const auto& module = *result.module;
  if (module.functions.size() != 1) {
    return fail("expected one AArch64 function record");
  }

  const auto function_name = prepare::resolve_prepared_function_name_id(prepared.names,
                                                                       "identity.fn");
  if (!function_name.has_value()) {
    return fail("test setup failed to publish prepared function id");
  }

  const auto& function = module.functions.front();
  if (function.function_name != *function_name || function.label != "identity.fn") {
    return fail("expected function record to retain prepared function id and label");
  }
  if (function.source_function != &prepared.module.functions.front()) {
    return fail("expected function source lookup to use structured link identity");
  }
  if (function.source_function->name != "raw.identity.display.drift") {
    return fail("expected raw source function name drift to remain only source debug text");
  }
  if (function.control_flow != &prepared.control_flow.functions.front()) {
    return fail("expected function record to reference prepared control-flow facts");
  }
  if (function.blocks.size() != 2) {
    return fail("expected two AArch64 block records");
  }

  const auto entry_label = prepare::resolve_prepared_block_label_id(prepared.names,
                                                                   "entry.authoritative");
  const auto exit_label = prepare::resolve_prepared_block_label_id(prepared.names,
                                                                  "exit.authoritative");
  if (!entry_label.has_value() || !exit_label.has_value()) {
    return fail("test setup failed to publish prepared block ids");
  }

  const auto& entry = function.blocks[0];
  if (entry.block_label != *entry_label || entry.label != "entry.authoritative") {
    return fail("expected entry block record to retain prepared block id and label");
  }
  if (entry.branch_target_label != *exit_label ||
      entry.terminator_kind != bir::TerminatorKind::Branch) {
    return fail("expected entry block record to retain prepared branch target id");
  }
  if (entry.source_block != &prepared.module.functions.front().blocks.front()) {
    return fail("expected entry block source lookup to use structured label identity");
  }
  if (entry.source_block->label != "raw.entry.drift") {
    return fail("expected raw source label drift to remain only source debug text");
  }

  const auto& exit = function.blocks[1];
  if (exit.block_label != *exit_label || exit.label != "exit.authoritative") {
    return fail("expected exit block record to retain prepared block id and label");
  }
  if (exit.source_block != &prepared.module.functions.front().blocks.back()) {
    return fail("expected exit block source lookup to use structured label identity");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int status = records_preserve_prepared_function_and_block_identities();
      status != 0) {
    return status;
  }
  return 0;
}
