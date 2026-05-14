#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/aarch64/api/api.hpp"
#include "src/backend/mir/aarch64/module/module.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <iostream>
#include <string_view>
#include <utility>
#include <variant>

namespace {

namespace aarch64_api = c4c::backend::aarch64::api;
namespace aarch64_module = c4c::backend::aarch64::module;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedBirModule prepared_module_with_function_and_block_identity() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("skeleton.fn");
  const auto entry_label = prepared.names.block_labels.intern("entry.semantic");
  const auto function_link_name = prepared.module.names.link_names.intern("skeleton.fn");
  const auto entry_bir_label = prepared.module.names.block_labels.intern("entry.semantic");

  bir::Block entry;
  entry.label = "entry.raw.display";
  entry.label_id = entry_bir_label;
  entry.terminator = bir::ReturnTerminator{};

  bir::Function function;
  function.name = "raw.skeleton.display";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::Void;
  function.blocks.push_back(std::move(entry));
  prepared.module.functions.push_back(std::move(function));

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });

  return prepared;
}

int skeleton_validates_handoff_and_returns_empty_canonical_product() {
  auto prepared = prepared_module_with_function_and_block_identity();

  const auto result = aarch64_api::build_prepared_module(prepared);
  if (result.error.has_value()) {
    return fail("expected AArch64 skeleton contract module to pass handoff validation");
  }
  if (!result.module.has_value()) {
    return fail("expected AArch64 skeleton contract module to build");
  }

  const auto& module = *result.module;
  if (module.prepared != &prepared) {
    return fail("expected skeleton product to retain prepared module handoff identity");
  }
  if (module.target_profile.arch != c4c::TargetArch::Aarch64 ||
      module.target_profile.backend_abi != c4c::BackendAbiKind::Aapcs64) {
    return fail("expected skeleton product to publish resolved AArch64/AAPCS64 target profile");
  }
  if (module.mir.functions.size() != 1) {
    return fail("expected skeleton to publish one canonical traversed function");
  }
  const auto& function = module.mir.functions.front();
  if (function.function_name != prepared.control_flow.functions.front().function_name ||
      function.blocks.size() != 1) {
    return fail("expected skeleton to preserve prepared function and block traversal");
  }
  const auto& block = function.blocks.front();
  if (block.block_label != prepared.control_flow.functions.front().blocks.front().block_label ||
      block.index != 0 || block.instructions.size() != 1) {
    return fail("expected skeleton traversal to lower one prepared return instruction");
  }
  const auto& instruction = block.instructions.front();
  if (!instruction.origin.has_value() ||
      instruction.origin->reason !=
          c4c::backend::mir::MachineOriginReason::BirTerminator ||
      instruction.origin->function_name != function.function_name ||
      instruction.origin->block_label != block.block_label ||
      instruction.target.family != aarch64_module::codegen::InstructionFamily::Return ||
      instruction.target.surface !=
          aarch64_module::codegen::RecordSurfaceKind::MachineInstructionNode ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          instruction.target.payload)) {
    return fail("expected skeleton traversal to publish a canonical return instruction");
  }
  if (module.functions.size() != 1 || module.compatibility.functions.size() != 1 ||
      !module.functions.front().machine_nodes.empty() ||
      !module.compatibility.functions.front().machine_nodes.empty()) {
    return fail("expected compatibility projections to remain minimal derived mirrors");
  }
  return 0;
}

}  // namespace

int main() {
  return skeleton_validates_handoff_and_returns_empty_canonical_product();
}
