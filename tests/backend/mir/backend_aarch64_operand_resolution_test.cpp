#include "src/backend/mir/aarch64/codegen/operands.hpp"
#include "src/backend/mir/aarch64/codegen/traversal.hpp"
#include "src/backend/mir/aarch64/module/module.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <variant>

namespace {

namespace aarch64_module = c4c::backend::aarch64::module;
namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace mir = c4c::backend::mir;
namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedRegisterPlacement caller_saved_gpr(std::size_t slot_index) {
  return prepare::PreparedRegisterPlacement{
      .bank = prepare::PreparedRegisterBank::Gpr,
      .pool = prepare::PreparedRegisterSlotPool::CallerSaved,
      .slot_index = slot_index,
      .contiguous_width = 1,
  };
}

aarch64_module::FunctionLoweringContext make_context(prepare::PreparedBirModule& prepared,
                                                     c4c::FunctionNameId function_name) {
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = prepared.names.block_labels.intern("operand.entry"),
      }},
  });
  return aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, prepared.control_flow.functions.front());
}

int storage_plan_register_precedes_value_home() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("operand.fn");
  const auto value_name = prepared.names.value_names.intern("v");
  const auto value_id = prepare::PreparedValueId{10};

  auto context = make_context(prepared, function_name);
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = value_id,
          .function_name = function_name,
          .value_name = value_name,
          .kind = prepare::PreparedValueHomeKind::StackSlot,
          .slot_id = prepare::PreparedFrameSlotId{44},
          .offset_bytes = std::size_t{64},
      }},
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values = {prepare::PreparedStoragePlanValue{
          .value_id = value_id,
          .value_name = value_name,
          .encoding = prepare::PreparedStorageEncodingKind::Register,
          .register_placement = caller_saved_gpr(0),
      }},
  });
  context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, prepared.control_flow.functions.front());

  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto resolved = aarch64_codegen::resolve_value_operand(value_id, context, diagnostics);
  if (!resolved.has_value()) {
    return fail("expected storage-plan register operand to resolve");
  }
  if (resolved->authority != aarch64_codegen::OperandAuthority::StoragePlan) {
    return fail("expected storage-plan authority to precede value-home authority");
  }
  const auto* reg = std::get_if<mir::PhysicalRegister>(&resolved->operand.payload);
  if (reg == nullptr || !resolved->register_reference.has_value()) {
    return fail("expected resolved storage-plan operand to be a typed register");
  }
  if (!diagnostics.empty()) {
    return fail("expected storage-plan register resolution to be diagnostic-free");
  }
  return 0;
}

int regalloc_assignment_precedes_storage_plan() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("operand.regalloc");
  const auto value_name = prepared.names.value_names.intern("rv");
  const auto value_id = prepare::PreparedValueId{20};

  auto context = make_context(prepared, function_name);
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values = {prepare::PreparedStoragePlanValue{
          .value_id = value_id,
          .value_name = value_name,
          .encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
          .slot_id = prepare::PreparedFrameSlotId{21},
          .stack_offset_bytes = std::size_t{32},
      }},
  });
  prepared.regalloc.functions.push_back(prepare::PreparedRegallocFunction{
      .function_name = function_name,
      .values = {prepare::PreparedRegallocValue{
          .value_id = value_id,
          .function_name = function_name,
          .value_name = value_name,
          .register_class = prepare::PreparedRegisterClass::General,
          .assigned_register = prepare::PreparedPhysicalRegisterAssignment{
              .reg_class = prepare::PreparedRegisterClass::General,
              .placement = caller_saved_gpr(1),
          },
      }},
  });
  context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, prepared.control_flow.functions.front());

  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto resolved = aarch64_codegen::resolve_value_operand(value_id, context, diagnostics);
  if (!resolved.has_value()) {
    return fail("expected regalloc register operand to resolve");
  }
  if (resolved->authority != aarch64_codegen::OperandAuthority::RegallocAssignment) {
    return fail("expected regalloc assignment to precede storage-plan authority");
  }
  if (std::get_if<mir::PhysicalRegister>(&resolved->operand.payload) == nullptr) {
    return fail("expected regalloc assignment to produce a typed register operand");
  }
  return diagnostics.empty() ? 0 : fail("expected regalloc resolution to be diagnostic-free");
}

int literals_labels_symbols_and_register_spellings_are_narrow() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("operand.failclosed");
  const auto value_name = prepared.names.value_names.intern("legacy_register_home");
  const auto value_id = prepare::PreparedValueId{30};
  const auto label = prepared.names.block_labels.intern("operand.target");
  const auto symbol = prepared.names.link_names.intern("operand.global");

  auto context = make_context(prepared, function_name);
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = value_id,
          .function_name = function_name,
          .value_name = value_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"x0"},
      }},
  });
  context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, prepared.control_flow.functions.front());

  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto missing = aarch64_codegen::resolve_value_operand(value_id, context, diagnostics);
  if (missing.has_value()) {
    return fail("expected register spelling-only value home to fail closed");
  }
  if (diagnostics.entries.empty() ||
      diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority) {
    return fail("expected missing typed register authority diagnostic");
  }

  const auto immediate = aarch64_codegen::resolve_immediate_operand(mir::Immediate{
      .kind = mir::ImmediateKind::Signed,
      .signed_value = -7,
      .unsigned_value = 0,
  });
  const auto target = aarch64_codegen::resolve_label_operand(label);
  const auto global = aarch64_codegen::resolve_symbol_operand(symbol, 16);
  if (immediate.authority != aarch64_codegen::OperandAuthority::Immediate ||
      target.authority != aarch64_codegen::OperandAuthority::Label ||
      global.authority != aarch64_codegen::OperandAuthority::Symbol) {
    return fail("expected literal, label, and symbol helpers to carry narrow authority");
  }
  if (std::get_if<mir::Immediate>(&immediate.operand.payload) == nullptr ||
      std::get_if<mir::Label>(&target.operand.payload) == nullptr ||
      std::get_if<mir::Symbol>(&global.operand.payload) == nullptr) {
    return fail("expected literal, label, and symbol helpers to produce typed MIR operands");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = storage_plan_register_precedes_value_home(); status != 0) {
    return status;
  }
  if (const int status = regalloc_assignment_precedes_storage_plan(); status != 0) {
    return status;
  }
  if (const int status = literals_labels_symbols_and_register_spellings_are_narrow();
      status != 0) {
    return status;
  }
  return 0;
}
