#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/aarch64/api/api.hpp"
#include "src/backend/mir/aarch64/codegen/dispatch.hpp"
#include "src/backend/mir/aarch64/module/module.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <iostream>
#include <string_view>
#include <variant>

namespace {

namespace aarch64_api = c4c::backend::aarch64::api;
namespace aarch64_codegen = c4c::backend::aarch64::codegen;
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

prepare::PreparedBirModule prepared_with_reordered_retained_bir() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto first_function = prepared.names.function_names.intern("dispatch.first");
  const auto second_function = prepared.names.function_names.intern("dispatch.second");
  const auto first_entry = prepared.names.block_labels.intern("first.entry");
  const auto second_entry = prepared.names.block_labels.intern("second.entry");
  const auto second_late = prepared.names.block_labels.intern("second.late");

  const auto bir_first_entry = prepared.module.names.block_labels.intern("first.entry");
  const auto bir_second_entry = prepared.module.names.block_labels.intern("second.entry");
  const auto bir_second_late = prepared.module.names.block_labels.intern("second.late");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.second",
      .return_type = bir::TypeKind::I32,
      .blocks = {bir::Block{
                     .label = "second.late",
                     .insts = {bir::CallInst{
                         .result = bir::Value::named(bir::TypeKind::I32, "late_call"),
                         .callee = "external",
                         .return_type = bir::TypeKind::I32,
                     }},
                     .terminator = bir::Terminator{bir::ReturnTerminator{}},
                     .label_id = bir_second_late,
                 },
                 bir::Block{
                     .label = "second.entry",
                     .insts = {bir::BinaryInst{
                         .opcode = bir::BinaryOpcode::Mul,
                         .result = bir::Value::named(bir::TypeKind::I32, "entry_product"),
                         .operand_type = bir::TypeKind::I32,
                         .lhs = bir::Value::immediate_i32(3),
                         .rhs = bir::Value::immediate_i32(4),
                     }},
                     .terminator = bir::Terminator{bir::ReturnTerminator{}},
                     .label_id = bir_second_entry,
                 }},
  });
  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.first",
      .return_type = bir::TypeKind::I32,
      .blocks = {bir::Block{
          .label = "first.entry",
          .insts = {bir::CallInst{
              .result = bir::Value::named(bir::TypeKind::I32, "first_call"),
              .callee = "external",
              .return_type = bir::TypeKind::I32,
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
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = second_function,
      .blocks = {prepare::PreparedControlFlowBlock{
                     .block_label = second_entry,
                     .terminator_kind = bir::TerminatorKind::Return,
                 },
                 prepare::PreparedControlFlowBlock{
                     .block_label = second_late,
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
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

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
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block{
      .block_label = c4c::kInvalidBlockLabel,
      .index = 99,
      .instructions = {},
  };
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 2 || !result.visited_terminator ||
      result.emitted_instructions != 1) {
    return fail("expected dispatch to visit prepared instructions and emit return");
  }
  if (block.block_label != block_cf.block_label || block.index != 0 ||
      block.instructions.size() != 1) {
    return fail("expected dispatch to preserve identity and emit one return instruction");
  }
  if (!std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          block.instructions.front().target.payload)) {
    return fail("expected dispatch to emit canonical return instruction target");
  }
  if (diagnostics.entries.size() != 2) {
    return fail("expected unsupported instruction diagnostics only");
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
  return 0;
}

int block_dispatch_maps_retained_bir_by_prepared_identity_not_index() {
  auto prepared = prepared_with_reordered_retained_bir();
  const auto& function_cf = prepared.control_flow.functions[1];
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_module::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);
  if (block_context.bir_block == nullptr ||
      block_context.bir_block->label != "second.entry") {
    return fail("expected block context to map retained BIR by prepared identity");
  }

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 1) {
    return fail("expected retained BIR dispatch to visit the identity-mapped block instruction");
  }
  if (!result.visited_terminator) {
    return fail("expected retained BIR dispatch to visit the prepared terminator");
  }
  if (result.emitted_instructions != 1 || block.instructions.size() != 1) {
    return fail("expected retained BIR dispatch to emit one return instruction");
  }
  if (diagnostics.entries.size() != 1) {
    return fail("expected identity-mapped retained BIR dispatch to record operation diagnostic only");
  }
  if (diagnostics.entries[0].kind !=
      aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily) {
    return fail("expected identity-mapped retained BIR dispatch instruction diagnostic");
  }
  if (diagnostics.entries[0].instruction_family !=
      aarch64_module::InstructionLoweringFamily::Scalar) {
    return fail("expected identity-mapped retained BIR dispatch to classify the instruction");
  }
  if (diagnostics.entries[0].block_label != block_cf.block_label) {
    return fail("expected identity-mapped retained BIR dispatch diagnostic block identity");
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
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 0 || !result.visited_terminator ||
      result.emitted_instructions != 1 || block.instructions.size() != 1) {
    return fail("expected missing mapping to skip operations but still lower return terminator");
  }
  if (diagnostics.entries.size() != 1 ||
      diagnostics.entries[0].kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::MissingInstructionBlockMapping ||
      diagnostics.entries[0].block_label != block_cf.block_label) {
    return fail("expected typed missing block mapping diagnostic");
  }
  return 0;
}

int module_build_dispatch_scaffold_lowers_return_and_keeps_machine_nodes_empty() {
  auto prepared = prepared_with_unsupported_instructions();
  const auto result = aarch64_api::build_prepared_module(prepared);
  if (result.error.has_value() || !result.module.has_value()) {
    return fail("expected prepared module with dispatch scaffold to build");
  }

  const auto& function = result.module->mir.functions.front();
  if (function.blocks.size() != 1 || function.blocks.front().instructions.size() != 1) {
    return fail("expected dispatch scaffold to emit one return instruction");
  }
  if (!std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          function.blocks.front().instructions.front().target.payload)) {
    return fail("expected module build to preserve canonical return target");
  }
  for (const auto& record : result.module->functions) {
    if (!record.machine_nodes.empty()) {
      return fail("expected dispatch scaffold not to fake flat compatibility machine nodes");
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
          block_dispatch_maps_retained_bir_by_prepared_identity_not_index();
      status != 0) {
    return status;
  }
  if (const int status = missing_bir_block_mapping_is_diagnostic_only();
      status != 0) {
    return status;
  }
  if (const int status = module_build_dispatch_scaffold_lowers_return_and_keeps_machine_nodes_empty();
      status != 0) {
    return status;
  }
  return 0;
}
