#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/aarch64/api/api.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <iostream>
#include <string_view>
#include <utility>
#include <variant>

namespace {

namespace aarch64_api = c4c::backend::aarch64::api;
namespace aarch64_abi = c4c::backend::aarch64::abi;
namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedValueHome register_home(prepare::PreparedValueId value_id,
                                         c4c::FunctionNameId function_name,
                                         c4c::ValueNameId value_name,
                                         const char* register_name) {
  return prepare::PreparedValueHome{
      .value_id = value_id,
      .function_name = function_name,
      .value_name = value_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = register_name,
  };
}

prepare::PreparedStoragePlanValue register_storage(prepare::PreparedValueId value_id,
                                                   c4c::ValueNameId value_name,
                                                   const char* register_name) {
  return prepare::PreparedStoragePlanValue{
      .value_id = value_id,
      .value_name = value_name,
      .encoding = prepare::PreparedStorageEncodingKind::Register,
      .bank = prepare::PreparedRegisterBank::Gpr,
      .contiguous_width = 1,
      .register_name = register_name,
      .occupied_register_names = {register_name},
  };
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

prepare::PreparedBirModule prepared_return_scalar_alu_module(bir::BinaryOpcode opcode,
                                                             bir::TypeKind type) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("return.scalar.fn");
  const auto block_label = prepared.names.block_labels.intern("entry.return.scalar");
  const auto lhs_name = prepared.names.value_names.intern("%lhs");
  const auto rhs_name = prepared.names.value_names.intern("%rhs");
  const auto result_name = prepared.names.value_names.intern("%result");

  const auto function_link_name = prepared.module.names.link_names.intern("return.scalar.fn");
  const auto block_bir_label = prepared.module.names.block_labels.intern("entry.return.scalar");

  bir::Block block;
  block.label = "raw.entry.return.scalar";
  block.label_id = block_bir_label;
  block.insts.push_back(bir::BinaryInst{
      .opcode = opcode,
      .result = bir::Value::named(type, "%result"),
      .operand_type = type,
      .lhs = bir::Value::named(type, "%lhs"),
      .rhs = bir::Value::named(type, "%rhs"),
  });
  block.terminator = bir::ReturnTerminator{.value = bir::Value::named(type, "%result")};

  bir::Function function;
  function.name = "raw.return.scalar.display.drift";
  function.link_name_id = function_link_name;
  function.return_type = type;
  function.blocks.push_back(std::move(block));
  prepared.module.functions.push_back(std::move(function));

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks =
          {
              prepare::PreparedControlFlowBlock{
                  .block_label = block_label,
                  .terminator_kind = bir::TerminatorKind::Return,
              },
          },
  });
  const char* lhs_register = type == bir::TypeKind::I32 ? "w1" : "x1";
  const char* rhs_register = type == bir::TypeKind::I32 ? "w2" : "x2";
  const char* result_register = type == bir::TypeKind::I32 ? "w0" : "x0";
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              register_home(prepare::PreparedValueId{10}, function_name, lhs_name, lhs_register),
              register_home(prepare::PreparedValueId{11}, function_name, rhs_name, rhs_register),
              register_home(prepare::PreparedValueId{12},
                            function_name,
                            result_name,
                            result_register),
          },
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values =
          {
              register_storage(prepare::PreparedValueId{10}, lhs_name, lhs_register),
              register_storage(prepare::PreparedValueId{11}, rhs_name, rhs_register),
              register_storage(prepare::PreparedValueId{12}, result_name, result_register),
          },
  });

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

int returned_scalar_add_sub_build_selected_machine_nodes() {
  struct Case {
    bir::BinaryOpcode opcode;
    bir::TypeKind type;
    aarch64_codegen::MachineOpcode machine_opcode;
    aarch64_codegen::ScalarAluOperationKind operation;
    aarch64_abi::RegisterReference lhs_register;
    aarch64_abi::RegisterReference rhs_register;
    aarch64_abi::RegisterReference result_register;
    aarch64_abi::RegisterView view;
  };

  for (const auto& test_case :
       {Case{.opcode = bir::BinaryOpcode::Add,
             .type = bir::TypeKind::I64,
             .machine_opcode = aarch64_codegen::MachineOpcode::Add,
             .operation = aarch64_codegen::ScalarAluOperationKind::Add,
             .lhs_register = aarch64_abi::x_register(1),
             .rhs_register = aarch64_abi::x_register(2),
             .result_register = aarch64_abi::x_register(0),
             .view = aarch64_abi::RegisterView::X},
        Case{.opcode = bir::BinaryOpcode::Sub,
             .type = bir::TypeKind::I32,
             .machine_opcode = aarch64_codegen::MachineOpcode::Sub,
             .operation = aarch64_codegen::ScalarAluOperationKind::Sub,
             .lhs_register = aarch64_abi::w_register(1),
             .rhs_register = aarch64_abi::w_register(2),
             .result_register = aarch64_abi::w_register(0),
             .view = aarch64_abi::RegisterView::W}}) {
    auto prepared = prepared_return_scalar_alu_module(test_case.opcode, test_case.type);

    const auto result = aarch64_api::build_prepared_module(prepared);
    if (result.error.has_value() || !result.module.has_value()) {
      return fail("expected prepared scalar return module to build");
    }
    const auto& function = result.module->functions.front();
    const auto function_name =
        prepare::resolve_prepared_function_name_id(prepared.names, "return.scalar.fn");
    const auto block_label =
        prepare::resolve_prepared_block_label_id(prepared.names, "entry.return.scalar");
    if (!function_name.has_value() || !block_label.has_value()) {
      return fail("test setup failed to publish scalar return identities");
    }
    if (function.machine_nodes.size() != 2) {
      return fail("expected scalar ALU node followed by return node");
    }

    const auto& scalar_node = function.machine_nodes[0];
    const auto* scalar =
        std::get_if<aarch64_codegen::ScalarInstructionRecord>(&scalar_node.payload);
    if (scalar == nullptr || scalar_node.family != aarch64_codegen::InstructionFamily::Scalar ||
        scalar_node.opcode != test_case.machine_opcode ||
        scalar_node.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
        scalar_node.function_name != *function_name || scalar_node.block_label != *block_label ||
        scalar_node.instruction_index != 0 || scalar_node.operands.size() != 2 ||
        scalar_node.uses.size() != 2 || scalar_node.defs.size() != 1 ||
        scalar_node.defs.front().kind !=
            aarch64_codegen::MachineEffectResourceKind::Register ||
        scalar_node.defs.front().value_id != prepare::PreparedValueId{12} ||
        scalar_node.defs.front().reg != test_case.result_register) {
      return fail("expected selected scalar add/sub node to carry order, labels, uses, and def");
    }
    if (!scalar->scalar_alu.has_value() || !scalar->result_register.has_value() ||
        scalar->scalar_alu->operation != test_case.operation ||
        scalar->result_register->role != aarch64_codegen::RegisterOperandRole::StoragePlan ||
        scalar->result_register->value_id != prepare::PreparedValueId{12} ||
        scalar->result_register->reg != test_case.result_register ||
        scalar->result_register->expected_view != test_case.view ||
        scalar->result_register->prepared_bank != prepare::PreparedRegisterBank::Gpr ||
        scalar->result_register->prepared_class != prepare::PreparedRegisterClass::General) {
      return fail("expected scalar add/sub result to use structured prepared destination register");
    }
    if (scalar_node.uses[0].kind != aarch64_codegen::MachineEffectResourceKind::Register ||
        scalar_node.uses[0].value_id != prepare::PreparedValueId{10} ||
        scalar_node.uses[0].reg != test_case.lhs_register ||
        scalar_node.uses[1].kind != aarch64_codegen::MachineEffectResourceKind::Register ||
        scalar_node.uses[1].value_id != prepare::PreparedValueId{11} ||
        scalar_node.uses[1].reg != test_case.rhs_register) {
      return fail("expected scalar add/sub lhs and rhs to remain prepared register uses");
    }

    const auto& return_node = function.machine_nodes[1];
    const auto* ret = std::get_if<aarch64_codegen::ReturnInstructionRecord>(&return_node.payload);
    if (ret == nullptr || return_node.family != aarch64_codegen::InstructionFamily::Return ||
        return_node.function_name != *function_name || return_node.block_label != *block_label ||
        return_node.uses.size() != 1 ||
        return_node.uses.front().kind != aarch64_codegen::MachineEffectResourceKind::Register ||
        return_node.uses.front().value_id != prepare::PreparedValueId{12} ||
        return_node.uses.front().reg != test_case.result_register || !ret->value.has_value() ||
        ret->value->kind != aarch64_codegen::OperandKind::Register) {
      return fail("expected return node to consume the structured scalar result register");
    }
    if (std::holds_alternative<aarch64_codegen::PreparedValueOperand>(ret->value->payload)) {
      return fail("expected scalar result return to avoid generic prepared-value placeholder");
    }
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = records_preserve_prepared_function_and_block_identities();
      status != 0) {
    return status;
  }
  if (const int status = returned_scalar_add_sub_build_selected_machine_nodes(); status != 0) {
    return status;
  }
  return 0;
}
