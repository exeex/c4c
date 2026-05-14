#include "src/backend/mir/aarch64/codegen/records.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
#include <variant>

namespace {

namespace aarch64_abi = c4c::backend::aarch64::abi;
namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

bir::Value named_value(bir::TypeKind type, const char* name) {
  return bir::Value::named(type, name);
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

prepare::PreparedValueHome immediate_home(prepare::PreparedValueId value_id,
                                          c4c::FunctionNameId function_name,
                                          c4c::ValueNameId value_name,
                                          std::int64_t immediate) {
  return prepare::PreparedValueHome{
      .value_id = value_id,
      .function_name = function_name,
      .value_name = value_name,
      .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
      .immediate_i32 = immediate,
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

prepare::PreparedStoragePlanValue immediate_storage(prepare::PreparedValueId value_id,
                                                    c4c::ValueNameId value_name,
                                                    std::int64_t immediate) {
  return prepare::PreparedStoragePlanValue{
      .value_id = value_id,
      .value_name = value_name,
      .encoding = prepare::PreparedStorageEncodingKind::Immediate,
      .bank = prepare::PreparedRegisterBank::Gpr,
      .immediate_i32 = immediate,
  };
}

struct PreparedScalarFixture {
  prepare::PreparedNameTables names;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::ValueNameId lhs_name = c4c::kInvalidValueName;
  c4c::ValueNameId rhs_name = c4c::kInvalidValueName;
  c4c::ValueNameId result_name = c4c::kInvalidValueName;
  prepare::PreparedValueLocationFunction locations;
  prepare::PreparedStoragePlanFunction storage;
};

PreparedScalarFixture make_i64_fixture() {
  PreparedScalarFixture fixture;
  fixture.function_name = fixture.names.function_names.intern("f");
  fixture.lhs_name = fixture.names.value_names.intern("%lhs");
  fixture.rhs_name = fixture.names.value_names.intern("%rhs");
  fixture.result_name = fixture.names.value_names.intern("%sum");
  fixture.locations = prepare::PreparedValueLocationFunction{
      .function_name = fixture.function_name,
      .value_homes =
          {
              register_home(prepare::PreparedValueId{10}, fixture.function_name, fixture.lhs_name, "x1"),
              register_home(prepare::PreparedValueId{11}, fixture.function_name, fixture.rhs_name, "x2"),
              register_home(prepare::PreparedValueId{12},
                            fixture.function_name,
                            fixture.result_name,
                            "x0"),
          },
  };
  fixture.storage = prepare::PreparedStoragePlanFunction{
      .function_name = fixture.function_name,
      .values =
          {
              register_storage(prepare::PreparedValueId{10}, fixture.lhs_name, "x1"),
              register_storage(prepare::PreparedValueId{11}, fixture.rhs_name, "x2"),
              register_storage(prepare::PreparedValueId{12}, fixture.result_name, "x0"),
          },
  };
  return fixture;
}

bir::BinaryInst binary(bir::BinaryOpcode opcode, bir::TypeKind type) {
  return bir::BinaryInst{
      .opcode = opcode,
      .result = named_value(type, "%sum"),
      .operand_type = type,
      .lhs = named_value(type, "%lhs"),
      .rhs = named_value(type, "%rhs"),
  };
}

int supported_scalar_alu_records_preserve_prepared_and_bir_facts() {
  for (const auto opcode : {bir::BinaryOpcode::Add,
                            bir::BinaryOpcode::Sub,
                            bir::BinaryOpcode::And,
                            bir::BinaryOpcode::Or,
                            bir::BinaryOpcode::Xor}) {
    auto fixture = make_i64_fixture();
    const auto result = aarch64_codegen::make_prepared_scalar_alu_instruction_record(
        fixture.names, fixture.locations, fixture.storage, binary(opcode, bir::TypeKind::I64));
    if (!result.record.has_value() ||
        result.error != aarch64_codegen::PreparedScalarAluRecordError::None) {
      return fail("expected supported scalar ALU prepared conversion to succeed");
    }

    const auto& instruction = *result.record;
    if (instruction.result_value_id != prepare::PreparedValueId{12} ||
        instruction.result_value_name != fixture.result_name ||
        instruction.result_type != bir::TypeKind::I64 ||
        instruction.source_binary_opcode != opcode || !instruction.scalar_alu.has_value() ||
        instruction.inputs.size() != 2 || !instruction.result_register.has_value()) {
      return fail("expected scalar instruction wrapper to preserve result and source opcode");
    }

    const auto& alu = *instruction.scalar_alu;
    if (alu.surface != aarch64_codegen::RecordSurfaceKind::RecordOnly ||
        alu.operation != aarch64_codegen::scalar_alu_operation_from_binary_opcode(opcode) ||
        alu.source_binary_opcode != opcode || alu.operand_type != bir::TypeKind::I64 ||
        alu.result_type != bir::TypeKind::I64 ||
        alu.result_value_id != prepare::PreparedValueId{12} ||
        alu.result_value_name != fixture.result_name || !alu.supported_integer_operation ||
        !alu.result_register.has_value()) {
      return fail("expected ALU record to preserve structured BIR and prepared ids");
    }
    if (alu.result_register->role != aarch64_codegen::RegisterOperandRole::StoragePlan ||
        alu.result_register->value_id != prepare::PreparedValueId{12} ||
        alu.result_register->value_name != fixture.result_name ||
        alu.result_register->reg != aarch64_abi::x_register(0) ||
        alu.result_register->expected_view != aarch64_abi::RegisterView::X ||
        alu.result_register->prepared_bank != prepare::PreparedRegisterBank::Gpr ||
        alu.result_register->prepared_class != prepare::PreparedRegisterClass::General ||
        alu.result_register->occupied_registers.size() != 1 ||
        alu.result_register->occupied_registers.front() != "x0") {
      return fail("expected ALU result to become a typed prepared destination register");
    }

    const auto* lhs = std::get_if<aarch64_codegen::RegisterOperand>(&alu.lhs.payload);
    const auto* rhs = std::get_if<aarch64_codegen::RegisterOperand>(&alu.rhs.payload);
    if (alu.lhs.kind != aarch64_codegen::OperandKind::Register ||
        alu.rhs.kind != aarch64_codegen::OperandKind::Register || lhs == nullptr ||
        rhs == nullptr || lhs->role != aarch64_codegen::RegisterOperandRole::StoragePlan ||
        rhs->role != aarch64_codegen::RegisterOperandRole::StoragePlan ||
        lhs->value_id != prepare::PreparedValueId{10} ||
        rhs->value_id != prepare::PreparedValueId{11} ||
        lhs->value_name != fixture.lhs_name || rhs->value_name != fixture.rhs_name ||
        lhs->reg != aarch64_abi::x_register(1) || rhs->reg != aarch64_abi::x_register(2) ||
        lhs->expected_view != aarch64_abi::RegisterView::X ||
        rhs->expected_view != aarch64_abi::RegisterView::X ||
        lhs->prepared_bank != prepare::PreparedRegisterBank::Gpr ||
        rhs->prepared_class != prepare::PreparedRegisterClass::General) {
      return fail("expected named operands to become typed prepared register operands");
    }
    if (opcode == bir::BinaryOpcode::Add || opcode == bir::BinaryOpcode::Sub) {
      const auto machine = aarch64_codegen::make_scalar_instruction(instruction);
      if (machine.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
          machine.operands.size() != 2 || machine.uses.size() != 2 ||
          machine.defs.size() != 1 ||
          machine.defs.front().kind != aarch64_codegen::MachineEffectResourceKind::Register ||
          machine.defs.front().value_id != prepare::PreparedValueId{12} ||
          machine.defs.front().reg != aarch64_abi::x_register(0) ||
          machine.opcode != (opcode == bir::BinaryOpcode::Add
                                 ? aarch64_codegen::MachineOpcode::Add
                                 : aarch64_codegen::MachineOpcode::Sub)) {
        return fail("expected add/sub selected machine nodes to carry lhs/rhs and destination");
      }

      const auto ret = aarch64_codegen::make_return_instruction(
          aarch64_codegen::ReturnInstructionRecord{
              .value = aarch64_codegen::make_register_operand(*instruction.result_register),
              .value_type = instruction.result_type,
          });
      if (ret.operands.size() != 1 || ret.uses.size() != 1 ||
          ret.uses.front().kind != aarch64_codegen::MachineEffectResourceKind::Register ||
          ret.uses.front().value_id != prepare::PreparedValueId{12} ||
          ret.uses.front().reg != aarch64_abi::x_register(0)) {
        return fail("expected return record to reference the prepared scalar result");
      }
    }
  }
  return 0;
}

int named_rematerialized_immediate_operands_preserve_source_identity() {
  auto fixture = make_i64_fixture();
  fixture.locations.value_homes[1] =
      immediate_home(prepare::PreparedValueId{11}, fixture.function_name, fixture.rhs_name, 7);
  fixture.storage.values[1] = immediate_storage(prepare::PreparedValueId{11}, fixture.rhs_name, 7);
  const auto result = aarch64_codegen::make_prepared_scalar_alu_record(
      fixture.names, fixture.locations, fixture.storage, binary(bir::BinaryOpcode::Add, bir::TypeKind::I64));
  if (!result.record.has_value()) {
    return fail("expected rematerialized immediate source conversion to succeed");
  }

  const auto& rhs_record = result.record->rhs;
  const auto* rhs = std::get_if<aarch64_codegen::ImmediateOperand>(&rhs_record.payload);
  if (rhs_record.kind != aarch64_codegen::OperandKind::Immediate || rhs == nullptr ||
      rhs->type != bir::TypeKind::I32 || rhs->signed_value != 7 || rhs->unsigned_value != 7 ||
      rhs->source_value_id != prepare::PreparedValueId{11} ||
      rhs->source_value_name != fixture.rhs_name) {
    return fail("expected rematerialized immediate operand to preserve prepared source identity");
  }
  return 0;
}

int unsupported_and_incomplete_facts_fail_closed() {
  auto fixture = make_i64_fixture();
  const auto unsupported = aarch64_codegen::make_prepared_scalar_alu_record(
      fixture.names, fixture.locations, fixture.storage, binary(bir::BinaryOpcode::Mul, bir::TypeKind::I64));
  if (unsupported.record.has_value() ||
      unsupported.error != aarch64_codegen::PreparedScalarAluRecordError::UnsupportedOpcode ||
      aarch64_codegen::prepared_scalar_alu_record_error_name(unsupported.error) !=
          "unsupported_opcode") {
    return fail("expected unsupported scalar opcode to fail closed");
  }

  const auto compare = aarch64_codegen::make_prepared_scalar_alu_record(
      fixture.names, fixture.locations, fixture.storage, binary(bir::BinaryOpcode::Eq, bir::TypeKind::I64));
  if (compare.record.has_value() ||
      compare.error != aarch64_codegen::PreparedScalarAluRecordError::UnsupportedOpcode) {
    return fail("expected compare predicate to stay outside scalar ALU conversion");
  }

  fixture.locations.value_homes.pop_back();
  const auto missing_result = aarch64_codegen::make_prepared_scalar_alu_record(
      fixture.names, fixture.locations, fixture.storage, binary(bir::BinaryOpcode::Add, bir::TypeKind::I64));
  if (missing_result.record.has_value() ||
      missing_result.error !=
          aarch64_codegen::PreparedScalarAluRecordError::MissingResultValueHome) {
    return fail("expected missing result home to fail closed");
  }

  fixture = make_i64_fixture();
  fixture.storage.values.erase(fixture.storage.values.begin() + 1);
  const auto missing_operand_storage = aarch64_codegen::make_prepared_scalar_alu_record(
      fixture.names, fixture.locations, fixture.storage, binary(bir::BinaryOpcode::Add, bir::TypeKind::I64));
  if (missing_operand_storage.record.has_value() ||
      missing_operand_storage.error !=
          aarch64_codegen::PreparedScalarAluRecordError::MissingOperandStorage) {
    return fail("expected missing operand storage to fail closed");
  }

  fixture = make_i64_fixture();
  fixture.storage.values[1].encoding = prepare::PreparedStorageEncodingKind::FrameSlot;
  fixture.storage.values[1].register_name = std::nullopt;
  fixture.storage.values[1].slot_id = prepare::PreparedFrameSlotId{4};
  const auto unsupported_storage = aarch64_codegen::make_prepared_scalar_alu_record(
      fixture.names, fixture.locations, fixture.storage, binary(bir::BinaryOpcode::Add, bir::TypeKind::I64));
  if (unsupported_storage.record.has_value() ||
      unsupported_storage.error !=
          aarch64_codegen::PreparedScalarAluRecordError::UnsupportedOperandStorage) {
    return fail("expected unsupported operand storage to fail closed");
  }

  fixture = make_i64_fixture();
  fixture.locations.value_homes[0].register_name = "x1";
  fixture.storage.values[0].register_name = "x1";
  const auto view_mismatch = aarch64_codegen::make_prepared_scalar_alu_record(
      fixture.names, fixture.locations, fixture.storage, binary(bir::BinaryOpcode::Add, bir::TypeKind::I32));
  if (view_mismatch.record.has_value() ||
      view_mismatch.error !=
          aarch64_codegen::PreparedScalarAluRecordError::RegisterConversionFailed) {
    return fail("expected prepared register view mismatch to fail closed");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int status = supported_scalar_alu_records_preserve_prepared_and_bir_facts();
      status != 0) {
    return status;
  }
  if (const int status = named_rematerialized_immediate_operands_preserve_source_identity();
      status != 0) {
    return status;
  }
  if (const int status = unsupported_and_incomplete_facts_fail_closed(); status != 0) {
    return status;
  }
  return 0;
}
