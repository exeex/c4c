#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/aarch64/api/api.hpp"
#include "src/backend/mir/aarch64/module/module.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <iostream>
#include <string_view>

namespace {

namespace aarch64_api = c4c::backend::aarch64::api;
namespace aarch64_module = c4c::backend::aarch64::module;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedBirModule prepared_with_unsupported_instructions() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.fn");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.entry");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("dispatch.entry");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.fn",
      .return_type = bir::TypeKind::I32,
      .blocks = {bir::Block{
          .label = "dispatch.entry",
          .insts = {bir::BinaryInst{
                        .opcode = bir::BinaryOpcode::Add,
                        .result = bir::Value::named(bir::TypeKind::I32, "sum"),
                        .operand_type = bir::TypeKind::I32,
                        .lhs = bir::Value::immediate_i32(1),
                        .rhs = bir::Value::immediate_i32(2),
                    },
                    bir::CallInst{
                        .result = bir::Value::named(bir::TypeKind::I32, "call_result"),
                        .callee = "external",
                        .return_type = bir::TypeKind::I32,
                    }},
          .terminator = bir::Terminator{bir::ReturnTerminator{}},
          .label_id = bir_entry_label,
      }},
  });

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_mismatched_retained_bir_labels() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto first_function = prepared.names.function_names.intern("dispatch.first");
  const auto first_entry = prepared.names.block_labels.intern("first.entry");

  const auto bir_first_entry =
      prepared.module.names.block_labels.intern("raw.display.only.entry");

  prepared.module.functions.push_back(bir::Function{
      .name = "raw.display.only.function",
      .return_type = bir::TypeKind::I32,
      .blocks = {bir::Block{
          .label = "raw.display.only.label",
          .insts = {bir::BinaryInst{
              .opcode = bir::BinaryOpcode::Add,
              .result = bir::Value::named(bir::TypeKind::I32, "first_sum"),
              .operand_type = bir::TypeKind::I32,
              .lhs = bir::Value::immediate_i32(1),
              .rhs = bir::Value::immediate_i32(2),
          }},
          .terminator = bir::Terminator{bir::ReturnTerminator{}},
          .label_id = bir_first_entry,
      }},
  });

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = first_function,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = first_entry,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });

  return prepared;
}

prepare::PreparedBirModule prepared_with_control_flow_only() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.cf_only");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.cf_entry");
  const auto exit_label = prepared.names.block_labels.intern("dispatch.cf_exit");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Branch,
          .branch_target_label = exit_label,
      }},
  });
  return prepared;
}

int block_dispatch_visits_prepared_terminator_without_bir_block_mapping() {
  auto prepared = prepared_with_control_flow_only();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_module::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_module::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_module::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 0 || !result.visited_terminator ||
      result.emitted_instructions != 0 || !block.instructions.empty()) {
    return fail("expected control-flow-only dispatch to visit terminator only");
  }
  if (block.block_label != block_cf.block_label || block.index != 0) {
    return fail("expected control-flow-only dispatch to preserve prepared block identity");
  }
  if (diagnostics.entries.size() != 1 ||
      diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily ||
      diagnostics.entries.front().block_label != block_cf.block_label) {
    return fail("expected unsupported terminator diagnostic without BIR mapping");
  }
  return 0;
}

int block_dispatch_visits_prepared_instructions_in_order_and_fails_closed() {
  auto prepared = prepared_with_unsupported_instructions();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_module::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_module::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block{
      .block_label = c4c::kInvalidBlockLabel,
      .index = 99,
      .instructions = {},
  };
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_module::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 2 || !result.visited_terminator ||
      result.emitted_instructions != 0) {
    return fail("expected dispatch to visit prepared instructions and emit nothing");
  }
  if (block.block_label != block_cf.block_label || block.index != 0 ||
      !block.instructions.empty()) {
    return fail("expected dispatch to preserve identity and emit no fake instructions");
  }
  if (diagnostics.entries.size() != 3) {
    return fail("expected unsupported instruction diagnostics plus terminator diagnostic");
  }
  if (diagnostics.entries[0].kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily ||
      diagnostics.entries[0].instruction_index != 0 ||
      diagnostics.entries[0].instruction_family !=
          aarch64_module::InstructionLoweringFamily::Scalar) {
    return fail("expected first diagnostic to describe scalar instruction zero");
  }
  if (diagnostics.entries[1].kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily ||
      diagnostics.entries[1].instruction_index != 1 ||
      diagnostics.entries[1].instruction_family !=
          aarch64_module::InstructionLoweringFamily::Call) {
    return fail("expected second diagnostic to describe call instruction one");
  }
  if (diagnostics.entries[2].kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily ||
      diagnostics.entries[2].function_name != function_cf.function_name ||
      diagnostics.entries[2].block_label != block_cf.block_label) {
    return fail("expected typed unsupported-terminator diagnostic with prepared identity");
  }
  return 0;
}

int block_dispatch_maps_retained_bir_by_index_not_label_spelling() {
  auto prepared = prepared_with_mismatched_retained_bir_labels();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks[0];
  const auto function_context = aarch64_module::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_module::make_block_lowering_context(function_context, block_cf, 0);
  if (block_context.bir_block == nullptr) {
    return fail("expected block context to map retained BIR by prepared index");
  }

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_module::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 1) {
    return fail("expected retained BIR dispatch to visit the indexed block instruction");
  }
  if (!result.visited_terminator) {
    return fail("expected retained BIR dispatch to visit the prepared terminator");
  }
  if (result.emitted_instructions != 0 || !block.instructions.empty()) {
    return fail("expected retained BIR dispatch not to emit fake instructions");
  }
  if (diagnostics.entries.size() != 2) {
    return fail("expected index-mapped retained BIR dispatch to record two diagnostics");
  }
  if (diagnostics.entries[0].kind !=
      aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily) {
    return fail("expected index-mapped retained BIR dispatch instruction diagnostic");
  }
  if (diagnostics.entries[0].instruction_family !=
      aarch64_module::InstructionLoweringFamily::Scalar) {
    return fail("expected index-mapped retained BIR dispatch to classify the instruction");
  }
  if (diagnostics.entries[0].block_label != block_cf.block_label) {
    return fail("expected index-mapped retained BIR dispatch diagnostic block identity");
  }
  if (diagnostics.entries[1].kind !=
      aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily) {
    return fail("expected index-mapped retained BIR dispatch diagnostics");
  }
  return 0;
}

int missing_bir_block_mapping_is_diagnostic_only() {
  auto prepared = prepared_with_unsupported_instructions();
  prepared.module.functions.front().blocks.front().label_id = c4c::kInvalidBlockLabel;
  prepared.module.functions.front().blocks.front().label = "dispatch.unmatched";

  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_module::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_module::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_module::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 0 || !result.visited_terminator ||
      result.emitted_instructions != 0 || !block.instructions.empty()) {
    return fail("expected missing mapping to skip operations but still visit terminator");
  }
  if (diagnostics.entries.size() != 2 ||
      diagnostics.entries[0].kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::MissingInstructionBlockMapping ||
      diagnostics.entries[0].block_label != block_cf.block_label ||
      diagnostics.entries[1].kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily ||
      diagnostics.entries[1].block_label != block_cf.block_label) {
    return fail("expected typed missing block mapping diagnostic");
  }
  return 0;
}

int module_build_dispatch_scaffold_keeps_machine_nodes_empty() {
  auto prepared = prepared_with_unsupported_instructions();
  const auto result = aarch64_api::build_prepared_module(prepared);
  if (result.error.has_value() || !result.module.has_value()) {
    return fail("expected prepared module with dispatch scaffold to build");
  }

  const auto& function = result.module->mir.functions.front();
  if (function.blocks.size() != 1 || !function.blocks.front().instructions.empty()) {
    return fail("expected unsupported dispatch scaffold not to emit instructions");
  }
  for (const auto& record : result.module->functions) {
    if (!record.machine_nodes.empty()) {
      return fail("expected unsupported dispatch scaffold not to fake flat machine nodes");
    }
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status =
          block_dispatch_visits_prepared_terminator_without_bir_block_mapping();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_visits_prepared_instructions_in_order_and_fails_closed();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_maps_retained_bir_by_index_not_label_spelling();
      status != 0) {
    return status;
  }
  if (const int status = missing_bir_block_mapping_is_diagnostic_only();
      status != 0) {
    return status;
  }
  if (const int status = module_build_dispatch_scaffold_keeps_machine_nodes_empty();
      status != 0) {
    return status;
  }
  return 0;
}
