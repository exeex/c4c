#include "src/backend/mir/aarch64/codegen/instruction.hpp"
#include "src/backend/mir/aarch64/codegen/returns.hpp"
#include "src/backend/mir/aarch64/codegen/codegen.hpp"
#include "src/target_profile.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <optional>
#include <utility>
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

prepare::PreparedStoragePlanValue fpr_storage(prepare::PreparedValueId value_id,
                                              c4c::ValueNameId value_name,
                                              const char* register_name) {
  return prepare::PreparedStoragePlanValue{
      .value_id = value_id,
      .value_name = value_name,
      .encoding = prepare::PreparedStorageEncodingKind::Register,
      .bank = prepare::PreparedRegisterBank::Fpr,
      .contiguous_width = 1,
      .register_name = register_name,
      .occupied_register_names = {register_name},
  };
}

prepare::PreparedRegisterPlacement caller_saved_gpr(std::size_t slot_index) {
  return prepare::PreparedRegisterPlacement{
      .bank = prepare::PreparedRegisterBank::Gpr,
      .pool = prepare::PreparedRegisterSlotPool::CallerSaved,
      .slot_index = slot_index,
      .contiguous_width = 1,
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

PreparedScalarFixture make_f64_fixture() {
  PreparedScalarFixture fixture;
  fixture.function_name = fixture.names.function_names.intern("f.fp");
  fixture.lhs_name = fixture.names.value_names.intern("%lhs");
  fixture.rhs_name = fixture.names.value_names.intern("%rhs");
  fixture.result_name = fixture.names.value_names.intern("%sum");
  fixture.locations = prepare::PreparedValueLocationFunction{
      .function_name = fixture.function_name,
      .value_homes =
          {
              register_home(prepare::PreparedValueId{20}, fixture.function_name, fixture.lhs_name, "d1"),
              register_home(prepare::PreparedValueId{21}, fixture.function_name, fixture.rhs_name, "d2"),
              register_home(prepare::PreparedValueId{22}, fixture.function_name, fixture.result_name, "d0"),
          },
  };
  fixture.storage = prepare::PreparedStoragePlanFunction{
      .function_name = fixture.function_name,
      .values =
          {
              fpr_storage(prepare::PreparedValueId{20}, fixture.lhs_name, "d1"),
              fpr_storage(prepare::PreparedValueId{21}, fixture.rhs_name, "d2"),
              fpr_storage(prepare::PreparedValueId{22}, fixture.result_name, "d0"),
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

bir::BinaryInst binary_with_rhs(bir::BinaryOpcode opcode,
                                bir::TypeKind type,
                                bir::Value rhs) {
  auto inst = binary(opcode, type);
  inst.rhs = std::move(rhs);
  return inst;
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
        alu.result_register->occupied_register_references.size() != 1 ||
        alu.result_register->occupied_register_references.front() !=
            aarch64_abi::x_register(0)) {
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

int shift_immediate_prepares_result_home_and_shift_operand() {
  auto fixture = make_i64_fixture();
  fixture.locations.value_homes[0].register_name = "w0";
  fixture.locations.value_homes[2].register_name = "w13";
  fixture.storage.values[0] =
      register_storage(prepare::PreparedValueId{10}, fixture.lhs_name, "w0");
  fixture.storage.values[2] =
      register_storage(prepare::PreparedValueId{12}, fixture.result_name, "w13");

  for (const auto opcode :
       {bir::BinaryOpcode::Shl, bir::BinaryOpcode::LShr, bir::BinaryOpcode::AShr}) {
    const auto result = aarch64_codegen::make_prepared_scalar_alu_instruction_record(
        fixture.names,
        fixture.locations,
        fixture.storage,
        binary_with_rhs(opcode, bir::TypeKind::I32, bir::Value::immediate_i32(1)));
    if (!result.record.has_value() ||
        result.error != aarch64_codegen::PreparedScalarAluRecordError::None ||
        !result.record->scalar_alu.has_value()) {
      return fail("expected i32 shift immediate to prepare");
    }
    const auto& alu = *result.record->scalar_alu;
    const auto* lhs = std::get_if<aarch64_codegen::RegisterOperand>(&alu.lhs.payload);
    const auto* rhs = std::get_if<aarch64_codegen::ImmediateOperand>(&alu.rhs.payload);
    if (alu.source_binary_opcode != opcode ||
        alu.operation != aarch64_codegen::ScalarAluOperationKind::LogicalShiftRight ||
        !alu.supported_integer_operation || alu.operand_type != bir::TypeKind::I32 ||
        alu.result_type != bir::TypeKind::I32 || lhs == nullptr || rhs == nullptr ||
        lhs->reg != aarch64_abi::w_register(0) ||
        lhs->expected_view != aarch64_abi::RegisterView::W ||
        rhs->unsigned_value != 1 || !alu.result_register.has_value() ||
        alu.result_register->reg != aarch64_abi::w_register(13) ||
        alu.result_register->expected_view != aarch64_abi::RegisterView::W) {
      return fail("expected shift immediate record to preserve source, amount, and result home");
    }
  }

  auto register_shift = fixture;
  register_shift.locations.value_homes[1].register_name = "w1";
  register_shift.storage.values[1] =
      register_storage(prepare::PreparedValueId{11}, register_shift.rhs_name, "w1");
  const auto unsupported_register_shift =
      aarch64_codegen::make_prepared_scalar_alu_record(
          register_shift.names,
          register_shift.locations,
          register_shift.storage,
          binary(bir::BinaryOpcode::AShr, bir::TypeKind::I32));
  if (unsupported_register_shift.record.has_value() ||
      unsupported_register_shift.error !=
          aarch64_codegen::PreparedScalarAluRecordError::UnsupportedOpcode) {
    return fail("expected shift register amount to fail closed until variable shifts are modeled");
  }

  const auto out_of_range = aarch64_codegen::make_prepared_scalar_alu_record(
      fixture.names,
      fixture.locations,
      fixture.storage,
      binary_with_rhs(bir::BinaryOpcode::AShr,
                      bir::TypeKind::I32,
                      bir::Value::immediate_i32(32)));
  if (out_of_range.record.has_value() ||
      out_of_range.error != aarch64_codegen::PreparedScalarAluRecordError::UnsupportedOpcode) {
    return fail("expected shift amount at operand width to fail closed");
  }

  return 0;
}

int supported_floating_scalar_alu_records_preserve_fpr_facts() {
  for (const auto opcode :
       {bir::BinaryOpcode::Add, bir::BinaryOpcode::Sub, bir::BinaryOpcode::Mul,
        bir::BinaryOpcode::SDiv}) {
    auto fixture = make_f64_fixture();
    const auto result = aarch64_codegen::make_prepared_scalar_alu_instruction_record(
        fixture.names, fixture.locations, fixture.storage, binary(opcode, bir::TypeKind::F64));
    if (!result.record.has_value() ||
        result.error != aarch64_codegen::PreparedScalarAluRecordError::None) {
      return fail("expected supported F64 scalar ALU prepared conversion to succeed");
    }

    const auto& instruction = *result.record;
    const auto& alu = *instruction.scalar_alu;
    const auto* lhs = std::get_if<aarch64_codegen::RegisterOperand>(&alu.lhs.payload);
    const auto* rhs = std::get_if<aarch64_codegen::RegisterOperand>(&alu.rhs.payload);
    if (!alu.supported_floating_operation || alu.supported_integer_operation ||
        alu.operand_type != bir::TypeKind::F64 || alu.result_type != bir::TypeKind::F64 ||
        !alu.result_register.has_value() || lhs == nullptr || rhs == nullptr ||
        alu.result_register->reg != aarch64_abi::d_register(0) ||
        lhs->reg != aarch64_abi::d_register(1) ||
        rhs->reg != aarch64_abi::d_register(2) ||
        alu.result_register->expected_view != aarch64_abi::RegisterView::D ||
        lhs->expected_view != aarch64_abi::RegisterView::D ||
        rhs->prepared_bank != prepare::PreparedRegisterBank::Fpr ||
        rhs->prepared_class != prepare::PreparedRegisterClass::Float) {
      return fail("expected F64 ALU record to preserve typed FPR/SIMD register facts");
    }
    const auto machine = aarch64_codegen::make_scalar_instruction(instruction);
    if (machine.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
        machine.defs.size() != 1 || machine.uses.size() != 2 ||
        machine.defs.front().reg != aarch64_abi::d_register(0)) {
      return fail("expected F64 ALU machine node to select with FPR def/use facts");
    }
  }

  auto f32 = make_f64_fixture();
  f32.locations.value_homes[0].register_name = "s1";
  f32.locations.value_homes[1].register_name = "s2";
  f32.locations.value_homes[2].register_name = "s0";
  f32.storage.values[0] = fpr_storage(prepare::PreparedValueId{20}, f32.lhs_name, "s1");
  f32.storage.values[1] = fpr_storage(prepare::PreparedValueId{21}, f32.rhs_name, "s2");
  f32.storage.values[2] = fpr_storage(prepare::PreparedValueId{22}, f32.result_name, "s0");
  const auto f32_result = aarch64_codegen::make_prepared_scalar_alu_record(
      f32.names, f32.locations, f32.storage, binary(bir::BinaryOpcode::Mul, bir::TypeKind::F32));
  if (!f32_result.record.has_value() || !f32_result.record->result_register.has_value() ||
      f32_result.record->result_register->reg != aarch64_abi::s_register(0) ||
      f32_result.record->result_register->expected_view != aarch64_abi::RegisterView::S) {
    return fail("expected F32 ALU record to preserve S-register destination facts");
  }

  return 0;
}

int prepared_scalar_alu_registers_prefer_storage_register_placement() {
  auto fixture = make_i64_fixture();
  fixture.storage.values[0].register_name = std::string{"mismatched-lhs-spelling"};
  fixture.storage.values[0].occupied_register_names = {"mismatched-lhs-spelling"};
  fixture.storage.values[0].register_placement = caller_saved_gpr(0);
  fixture.storage.values[1].register_name = std::string{"x2"};
  fixture.storage.values[1].occupied_register_names = {"x2"};
  fixture.storage.values[1].register_placement = std::nullopt;
  fixture.storage.values[2].register_name = std::string{"x0"};
  fixture.storage.values[2].occupied_register_names = {"x0"};
  fixture.storage.values[2].register_placement = std::nullopt;

  const auto result = aarch64_codegen::make_prepared_scalar_alu_record(
      fixture.names,
      fixture.locations,
      fixture.storage,
      binary(bir::BinaryOpcode::Add, bir::TypeKind::I64));
  if (!result.record.has_value() ||
      result.error != aarch64_codegen::PreparedScalarAluRecordError::None) {
    return fail("expected ALU registers to resolve from storage placement before legacy names");
  }

  const auto& alu = *result.record;
  const auto* lhs = std::get_if<aarch64_codegen::RegisterOperand>(&alu.lhs.payload);
  const auto* rhs = std::get_if<aarch64_codegen::RegisterOperand>(&alu.rhs.payload);
  if (!alu.result_register.has_value() || lhs == nullptr || rhs == nullptr ||
      alu.result_register->reg != aarch64_abi::x_register(0) ||
      lhs->reg != aarch64_abi::x_register(13) ||
      rhs->reg != aarch64_abi::x_register(2) ||
      alu.result_register->role != aarch64_codegen::RegisterOperandRole::StoragePlan ||
      lhs->role != aarch64_codegen::RegisterOperandRole::StoragePlan ||
      rhs->role != aarch64_codegen::RegisterOperandRole::StoragePlan) {
    return fail("expected ALU result and operands to use storage placement registers");
  }
  if (lhs->occupied_register_references.size() != 1 ||
      lhs->occupied_register_references.front() != aarch64_abi::x_register(13) ||
      lhs->occupied_registers.size() != 1 || lhs->occupied_registers.front() != "x13") {
    return fail("expected occupied register display to follow typed placement conversion");
  }
  return 0;
}

int prepared_i32_scalar_alu_accepts_abi_x_register_parameter_spellings() {
  auto fixture = make_i64_fixture();
  fixture.locations.value_homes[0].register_name = "x0";
  fixture.locations.value_homes[1].register_name = "x1";
  fixture.locations.value_homes[2].register_name = "x13";
  fixture.storage.values[0] =
      register_storage(prepare::PreparedValueId{10}, fixture.lhs_name, "x0");
  fixture.storage.values[1] =
      register_storage(prepare::PreparedValueId{11}, fixture.rhs_name, "x1");
  fixture.storage.values[2] =
      register_storage(prepare::PreparedValueId{12}, fixture.result_name, "x13");

  const auto result = aarch64_codegen::make_prepared_scalar_alu_instruction_record(
      fixture.names,
      fixture.locations,
      fixture.storage,
      binary(bir::BinaryOpcode::Sub, bir::TypeKind::I32));
  if (!result.record.has_value() ||
      result.error != aarch64_codegen::PreparedScalarAluRecordError::None ||
      !result.record->scalar_alu.has_value() ||
      !result.record->result_register.has_value()) {
    return fail("expected I32 ALU parameters with ABI X spellings to prepare");
  }

  const auto& alu = *result.record->scalar_alu;
  const auto* lhs = std::get_if<aarch64_codegen::RegisterOperand>(&alu.lhs.payload);
  const auto* rhs = std::get_if<aarch64_codegen::RegisterOperand>(&alu.rhs.payload);
  if (lhs == nullptr || rhs == nullptr ||
      result.record->result_register->reg != aarch64_abi::w_register(13) ||
      lhs->reg != aarch64_abi::w_register(0) ||
      rhs->reg != aarch64_abi::w_register(1) ||
      result.record->result_register->expected_view != aarch64_abi::RegisterView::W ||
      lhs->expected_view != aarch64_abi::RegisterView::W ||
      rhs->expected_view != aarch64_abi::RegisterView::W ||
      lhs->occupied_registers != std::vector<std::string_view>{"w0"} ||
      rhs->occupied_registers != std::vector<std::string_view>{"w1"}) {
    return fail("expected I32 ALU to select W views from ABI X parameter spellings");
  }

  const auto machine = aarch64_codegen::make_scalar_instruction(*result.record);
  if (machine.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      machine.uses.size() != 2 || machine.defs.size() != 1 ||
      machine.uses[0].reg != aarch64_abi::w_register(0) ||
      machine.uses[1].reg != aarch64_abi::w_register(1) ||
      machine.defs.front().reg != aarch64_abi::w_register(13)) {
    return fail("expected selected I32 ALU node to carry W-register def/use facts");
  }
  return 0;
}

int prepared_scalar_unary_records_preserve_i32_i64_register_facts() {
  for (const auto type : {bir::TypeKind::I32, bir::TypeKind::I64}) {
    auto fixture = make_i64_fixture();
    const char* lhs_register = type == bir::TypeKind::I32 ? "w1" : "x1";
    const char* result_register = type == bir::TypeKind::I32 ? "w0" : "x0";
    fixture.locations.value_homes[0].register_name = lhs_register;
    fixture.locations.value_homes[2].register_name = result_register;
    fixture.storage.values[0] = register_storage(
        prepare::PreparedValueId{10}, fixture.lhs_name, lhs_register);
    fixture.storage.values[2] = register_storage(
        prepare::PreparedValueId{12}, fixture.result_name, result_register);

    for (const auto operation : {aarch64_codegen::ScalarUnaryOperationKind::Neg,
                                 aarch64_codegen::ScalarUnaryOperationKind::BitNot,
                                 aarch64_codegen::ScalarUnaryOperationKind::CountLeadingZeros,
                                 aarch64_codegen::ScalarUnaryOperationKind::CountTrailingZeros,
                                 aarch64_codegen::ScalarUnaryOperationKind::ByteSwap}) {
      const auto result = aarch64_codegen::make_prepared_scalar_unary_instruction_record(
          fixture.names,
          fixture.locations,
          fixture.storage,
          operation,
          named_value(type, "%sum"),
          named_value(type, "%lhs"));
      if (!result.record.has_value() ||
          result.error != aarch64_codegen::PreparedScalarAluRecordError::None) {
        return fail("expected prepared scalar unary conversion to succeed");
      }
      const auto& instruction = *result.record;
      if (!instruction.scalar_unary.has_value() || instruction.scalar_alu.has_value() ||
          instruction.inputs.size() != 1 || instruction.result_type != type ||
          instruction.result_value_id != prepare::PreparedValueId{12} ||
          instruction.result_value_name != fixture.result_name ||
          !instruction.result_register.has_value()) {
        return fail("expected prepared scalar unary instruction wrapper to preserve result facts");
      }
      const auto& unary = *instruction.scalar_unary;
      const auto* operand =
          std::get_if<aarch64_codegen::RegisterOperand>(&unary.operand.payload);
      const auto expected_view = type == bir::TypeKind::I32
                                     ? aarch64_abi::RegisterView::W
                                     : aarch64_abi::RegisterView::X;
      if (unary.operation != operation || unary.operand_type != type ||
          unary.result_type != type || !unary.supported_integer_operation ||
          !unary.result_register.has_value() ||
          unary.result_register->role != aarch64_codegen::RegisterOperandRole::StoragePlan ||
          unary.result_register->expected_view != expected_view ||
          unary.result_register->value_id != prepare::PreparedValueId{12} ||
          unary.operand.kind != aarch64_codegen::OperandKind::Register ||
          operand == nullptr || operand->value_id != prepare::PreparedValueId{10} ||
          operand->value_name != fixture.lhs_name ||
          operand->expected_view != expected_view ||
          operand->role != aarch64_codegen::RegisterOperandRole::StoragePlan) {
        return fail("expected prepared scalar unary record to preserve typed register operands");
      }
      const auto machine = aarch64_codegen::make_scalar_instruction(instruction);
      if (machine.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
          machine.operands.size() != 1 || machine.uses.size() != 1 ||
          machine.defs.size() != 1 ||
          machine.defs.front().value_id != prepare::PreparedValueId{12} ||
          machine.uses.front().value_id != prepare::PreparedValueId{10}) {
        return fail("expected prepared scalar unary machine node to carry one def/use");
      }
    }
  }
  return 0;
}

int prepared_scalar_unary_records_preserve_i16_byte_swap_register_facts() {
  auto fixture = make_i64_fixture();
  fixture.locations.value_homes[0].register_name = "w1";
  fixture.locations.value_homes[2].register_name = "w0";
  fixture.storage.values[0] =
      register_storage(prepare::PreparedValueId{10}, fixture.lhs_name, "w1");
  fixture.storage.values[2] =
      register_storage(prepare::PreparedValueId{12}, fixture.result_name, "w0");

  const auto result = aarch64_codegen::make_prepared_scalar_unary_instruction_record(
      fixture.names,
      fixture.locations,
      fixture.storage,
      aarch64_codegen::ScalarUnaryOperationKind::ByteSwap,
      named_value(bir::TypeKind::I16, "%sum"),
      named_value(bir::TypeKind::I16, "%lhs"));
  if (!result.record.has_value() ||
      result.error != aarch64_codegen::PreparedScalarAluRecordError::None) {
    return fail("expected prepared I16 byte-swap unary conversion to succeed");
  }
  const auto& unary = *result.record->scalar_unary;
  const auto* operand = std::get_if<aarch64_codegen::RegisterOperand>(&unary.operand.payload);
  if (unary.operation != aarch64_codegen::ScalarUnaryOperationKind::ByteSwap ||
      unary.operand_type != bir::TypeKind::I16 || unary.result_type != bir::TypeKind::I16 ||
      !unary.result_register.has_value() ||
      unary.result_register->expected_view != aarch64_abi::RegisterView::W ||
      operand == nullptr || operand->expected_view != aarch64_abi::RegisterView::W) {
    return fail("expected prepared I16 byte-swap to use W-register views");
  }
  return 0;
}

prepare::PreparedBirModule prepared_return_scalar_module_with_placement_only_operands() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("placement.scalar");
  const auto entry_label = prepared.names.block_labels.intern("entry");
  const auto lhs_name = prepared.names.value_names.intern("%lhs");
  const auto rhs_name = prepared.names.value_names.intern("%rhs");
  const auto sum_name = prepared.names.value_names.intern("%sum");

  const auto function_link_name = prepared.module.names.link_names.intern("placement.scalar");
  const auto entry_bir_label = prepared.module.names.block_labels.intern("entry");

  bir::Block entry;
  entry.label = "entry";
  entry.label_id = entry_bir_label;
  entry.insts.push_back(binary(bir::BinaryOpcode::Add, bir::TypeKind::I64));
  entry.terminator = bir::ReturnTerminator{.value = named_value(bir::TypeKind::I64, "%sum")};

  bir::Function function;
  function.name = "placement.scalar";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::I64;
  function.blocks.push_back(std::move(entry));
  prepared.module.functions.push_back(std::move(function));

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });

  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              register_home(prepare::PreparedValueId{10}, function_name, lhs_name, "x1"),
              register_home(prepare::PreparedValueId{11}, function_name, rhs_name, "x2"),
              register_home(prepare::PreparedValueId{12}, function_name, sum_name, "x0"),
          },
      .move_bundles =
          {
              prepare::PreparedMoveBundle{
                  .function_name = function_name,
                  .phase = prepare::PreparedMovePhase::BeforeReturn,
                  .block_index = 0,
                  .instruction_index = 1,
                  .moves =
                      {
                          prepare::PreparedMoveResolution{
                              .from_value_id = prepare::PreparedValueId{12},
                              .to_value_id = prepare::PreparedValueId{12},
                              .destination_kind =
                                  prepare::PreparedMoveDestinationKind::FunctionReturnAbi,
                              .destination_storage_kind =
                                  prepare::PreparedMoveStorageKind::Register,
                              .destination_contiguous_width = 1,
                              .destination_occupied_register_names = {"x0"},
                              .block_index = 0,
                              .instruction_index = 1,
                              .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                              .destination_register_placement =
                                  prepare::PreparedRegisterPlacement{
                                      .bank = prepare::PreparedRegisterBank::Gpr,
                                      .pool = prepare::PreparedRegisterSlotPool::CallResult,
                                      .slot_index = 0,
                                      .contiguous_width = 1,
                                  },
                          },
                      },
              },
          },
  });

  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values =
          {
              prepare::PreparedStoragePlanValue{
                  .value_id = prepare::PreparedValueId{10},
                  .value_name = lhs_name,
                  .encoding = prepare::PreparedStorageEncodingKind::Register,
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .contiguous_width = 1,
                  .register_name = std::nullopt,
                  .register_placement = caller_saved_gpr(0),
              },
              prepare::PreparedStoragePlanValue{
                  .value_id = prepare::PreparedValueId{11},
                  .value_name = rhs_name,
                  .encoding = prepare::PreparedStorageEncodingKind::Register,
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .contiguous_width = 1,
                  .register_name = std::string{"x2"},
                  .occupied_register_names = {"x2"},
              },
              prepare::PreparedStoragePlanValue{
                  .value_id = prepare::PreparedValueId{12},
                  .value_name = sum_name,
                  .encoding = prepare::PreparedStorageEncodingKind::Register,
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .contiguous_width = 1,
                  .register_name = std::string{"x0"},
                  .occupied_register_names = {"x0"},
              },
          },
  });

  return prepared;
}

int return_selected_scalar_operands_prefer_storage_register_placement() {
  auto prepared = prepared_return_scalar_module_with_placement_only_operands();
  const auto result = aarch64_codegen::compile_prepared_module(prepared);
  if (result.error.has_value() || !result.module.has_value() ||
      result.module->functions.empty()) {
    return fail("expected placement-only scalar operand module to build");
  }

  const auto& machine_nodes = result.module->functions.front().machine_nodes;
  const auto scalar_it = std::find_if(
      machine_nodes.begin(), machine_nodes.end(), [](const auto& node) {
        return node.family == aarch64_codegen::InstructionFamily::Scalar &&
               node.selection.status ==
                   aarch64_codegen::MachineNodeSelectionStatus::Selected &&
               std::holds_alternative<aarch64_codegen::ScalarInstructionRecord>(node.payload);
      });
  if (scalar_it == machine_nodes.end()) {
    return fail("expected return-selected scalar ALU machine node to be built");
  }

  const auto& scalar =
      std::get<aarch64_codegen::ScalarInstructionRecord>(scalar_it->payload);
  if (!scalar.scalar_alu.has_value() || scalar.inputs.size() != 2) {
    return fail("expected selected scalar ALU node to keep two structured inputs");
  }
  const auto* lhs = std::get_if<aarch64_codegen::RegisterOperand>(
      &scalar.scalar_alu->lhs.payload);
  const auto* rhs = std::get_if<aarch64_codegen::RegisterOperand>(
      &scalar.scalar_alu->rhs.payload);
  if (lhs == nullptr || rhs == nullptr ||
      lhs->reg != aarch64_abi::x_register(13) ||
      rhs->reg != aarch64_abi::x_register(2) ||
      lhs->role != aarch64_codegen::RegisterOperandRole::StoragePlan ||
      rhs->role != aarch64_codegen::RegisterOperandRole::StoragePlan) {
    return fail("expected selected operands to convert from storage placement before legacy names");
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

int unsigned_power_of_two_reductions_prepare_as_shift_and_mask() {
  {
    auto fixture = make_i64_fixture();
    const auto result = aarch64_codegen::make_prepared_scalar_alu_instruction_record(
        fixture.names,
        fixture.locations,
        fixture.storage,
        binary_with_rhs(bir::BinaryOpcode::UDiv,
                        bir::TypeKind::I64,
                        bir::Value::immediate_i64(8)));
    if (!result.record.has_value() ||
        result.error != aarch64_codegen::PreparedScalarAluRecordError::None ||
        !result.record->scalar_alu.has_value()) {
      return fail("expected unsigned division by immediate power-of-two to prepare");
    }
    const auto& alu = *result.record->scalar_alu;
    const auto* rhs = std::get_if<aarch64_codegen::ImmediateOperand>(&alu.rhs.payload);
    if (alu.operation != aarch64_codegen::ScalarAluOperationKind::LogicalShiftRight ||
        alu.source_binary_opcode != bir::BinaryOpcode::UDiv ||
        !alu.supported_integer_operation ||
        result.record->result_register->reg != aarch64_abi::x_register(0) ||
        rhs == nullptr || rhs->unsigned_value != 3 || rhs->type != bir::TypeKind::I64) {
      return fail("expected UDiv power-of-two to rewrite rhs as structured shift count");
    }
  }

  {
    auto fixture = make_i64_fixture();
    fixture.locations.value_homes[1] =
        immediate_home(prepare::PreparedValueId{11}, fixture.function_name, fixture.rhs_name, 16);
    fixture.storage.values[1] =
        immediate_storage(prepare::PreparedValueId{11}, fixture.rhs_name, 16);
    const auto result = aarch64_codegen::make_prepared_scalar_alu_instruction_record(
        fixture.names,
        fixture.locations,
        fixture.storage,
        binary(bir::BinaryOpcode::URem, bir::TypeKind::I64));
    if (!result.record.has_value() ||
        result.error != aarch64_codegen::PreparedScalarAluRecordError::None ||
        !result.record->scalar_alu.has_value()) {
      return fail("expected unsigned remainder by rematerialized power-of-two to prepare");
    }
    const auto& alu = *result.record->scalar_alu;
    const auto* rhs = std::get_if<aarch64_codegen::ImmediateOperand>(&alu.rhs.payload);
    if (alu.operation != aarch64_codegen::ScalarAluOperationKind::And ||
        alu.source_binary_opcode != bir::BinaryOpcode::URem ||
        !alu.supported_integer_operation || rhs == nullptr || rhs->unsigned_value != 15 ||
        rhs->source_value_id != prepare::PreparedValueId{11} ||
        rhs->source_value_name != fixture.rhs_name) {
      return fail("expected URem power-of-two to rewrite rhs as structured mask");
    }
  }

  for (const auto opcode : {bir::BinaryOpcode::SDiv,
                            bir::BinaryOpcode::SRem,
                            bir::BinaryOpcode::UDiv,
                            bir::BinaryOpcode::URem}) {
    auto fixture = make_i64_fixture();
    const auto rhs = opcode == bir::BinaryOpcode::UDiv ||
                             opcode == bir::BinaryOpcode::URem
                         ? bir::Value::immediate_i64(12)
                         : bir::Value::immediate_i64(8);
    const auto result = aarch64_codegen::make_prepared_scalar_alu_record(
        fixture.names,
        fixture.locations,
        fixture.storage,
        binary_with_rhs(opcode, bir::TypeKind::I64, rhs));
    if (result.record.has_value() ||
        result.error != aarch64_codegen::PreparedScalarAluRecordError::UnsupportedOpcode) {
      return fail("expected signed reductions and non-power unsigned reductions to fail closed");
    }
  }

  return 0;
}

int narrow_unsigned_reductions_prepare_explicit_zero_extension() {
  {
    auto fixture = make_i64_fixture();
    fixture.locations.value_homes[0].register_name = "w1";
    fixture.locations.value_homes[2].register_name = "w0";
    fixture.storage.values[0] =
        register_storage(prepare::PreparedValueId{10}, fixture.lhs_name, "w1");
    fixture.storage.values[2] =
        register_storage(prepare::PreparedValueId{12}, fixture.result_name, "w0");
    const auto result = aarch64_codegen::make_prepared_scalar_alu_instruction_record(
        fixture.names,
        fixture.locations,
        fixture.storage,
        binary_with_rhs(bir::BinaryOpcode::UDiv,
                        bir::TypeKind::I8,
                        bir::Value::immediate_i32(4)));
    if (!result.record.has_value() ||
        result.error != aarch64_codegen::PreparedScalarAluRecordError::None ||
        !result.record->scalar_alu.has_value()) {
      return fail("expected I8 unsigned division by power-of-two to prepare");
    }
    const auto& alu = *result.record->scalar_alu;
    const auto* rhs = std::get_if<aarch64_codegen::ImmediateOperand>(&alu.rhs.payload);
    if (alu.operation != aarch64_codegen::ScalarAluOperationKind::LogicalShiftRight ||
        alu.source_binary_opcode != bir::BinaryOpcode::UDiv ||
        alu.result_type != bir::TypeKind::I8 ||
        !alu.post_zero_extend_result_bits.has_value() ||
        *alu.post_zero_extend_result_bits != 8U ||
        rhs == nullptr ||
        rhs->unsigned_value != 2 ||
        !result.record->result_register.has_value() ||
        result.record->result_register->expected_view != aarch64_abi::RegisterView::W) {
      return fail("expected I8 UDiv reduction to require explicit post zero-extension");
    }
  }

  {
    auto fixture = make_i64_fixture();
    fixture.locations.value_homes[0].register_name = "w1";
    fixture.locations.value_homes[2].register_name = "w0";
    fixture.storage.values[0] =
        register_storage(prepare::PreparedValueId{10}, fixture.lhs_name, "w1");
    fixture.storage.values[2] =
        register_storage(prepare::PreparedValueId{12}, fixture.result_name, "w0");
    fixture.locations.value_homes[1] =
        immediate_home(prepare::PreparedValueId{11}, fixture.function_name, fixture.rhs_name, 16);
    fixture.storage.values[1] =
        immediate_storage(prepare::PreparedValueId{11}, fixture.rhs_name, 16);
    const auto result = aarch64_codegen::make_prepared_scalar_alu_instruction_record(
        fixture.names,
        fixture.locations,
        fixture.storage,
        binary(bir::BinaryOpcode::URem, bir::TypeKind::I16));
    if (!result.record.has_value() ||
        result.error != aarch64_codegen::PreparedScalarAluRecordError::None ||
        !result.record->scalar_alu.has_value()) {
      return fail("expected I16 unsigned remainder by power-of-two to prepare");
    }
    const auto& alu = *result.record->scalar_alu;
    const auto* rhs = std::get_if<aarch64_codegen::ImmediateOperand>(&alu.rhs.payload);
    if (alu.operation != aarch64_codegen::ScalarAluOperationKind::And ||
        alu.source_binary_opcode != bir::BinaryOpcode::URem ||
        alu.result_type != bir::TypeKind::I16 ||
        !alu.post_zero_extend_result_bits.has_value() ||
        *alu.post_zero_extend_result_bits != 16U ||
        rhs == nullptr ||
        rhs->unsigned_value != 15 ||
        rhs->source_value_id != prepare::PreparedValueId{11}) {
      return fail("expected I16 URem reduction to carry rematerialized mask and zero-extension");
    }
  }

  {
    auto fixture = make_i64_fixture();
    fixture.locations.value_homes[0].register_name = "w1";
    fixture.locations.value_homes[2].register_name = "w0";
    fixture.storage.values[0] =
        register_storage(prepare::PreparedValueId{10}, fixture.lhs_name, "w1");
    fixture.storage.values[2] =
        register_storage(prepare::PreparedValueId{12}, fixture.result_name, "w0");
    const auto result = aarch64_codegen::make_prepared_scalar_alu_record(
        fixture.names,
        fixture.locations,
        fixture.storage,
        binary_with_rhs(bir::BinaryOpcode::UDiv,
                        bir::TypeKind::I8,
                        bir::Value::immediate_i32(256)));
    if (result.record.has_value() ||
        result.error != aarch64_codegen::PreparedScalarAluRecordError::UnsupportedOpcode) {
      return fail("expected I8 unsigned reduction whose shift exceeds type width to fail closed");
    }
  }

  {
    auto fixture = make_i64_fixture();
    const auto result = aarch64_codegen::make_prepared_scalar_alu_record(
        fixture.names,
        fixture.locations,
        fixture.storage,
        binary_with_rhs(bir::BinaryOpcode::UDiv,
                        bir::TypeKind::I64,
                        bir::Value::immediate_i64(8)));
    if (!result.record.has_value() ||
        result.record->post_zero_extend_result_bits.has_value()) {
      return fail("expected I64 unsigned reductions to stay plain without post extension");
    }
  }

  return 0;
}

int signed_i32_alu_results_prepare_explicit_sign_extension() {
  auto fixture = make_i64_fixture();
  fixture.locations.value_homes[0].register_name = "w1";
  fixture.locations.value_homes[1].register_name = "w2";
  fixture.storage.values[0] =
      register_storage(prepare::PreparedValueId{10}, fixture.lhs_name, "w1");
  fixture.storage.values[1] =
      register_storage(prepare::PreparedValueId{11}, fixture.rhs_name, "w2");

  for (const auto opcode : {bir::BinaryOpcode::Add, bir::BinaryOpcode::Sub}) {
    bir::BinaryInst inst{
        .opcode = opcode,
        .result = named_value(bir::TypeKind::I64, "%sum"),
        .operand_type = bir::TypeKind::I32,
        .lhs = named_value(bir::TypeKind::I32, "%lhs"),
        .rhs = named_value(bir::TypeKind::I32, "%rhs"),
    };
    const auto result = aarch64_codegen::make_prepared_scalar_alu_instruction_record(
        fixture.names, fixture.locations, fixture.storage, inst);
    if (!result.record.has_value() ||
        result.error != aarch64_codegen::PreparedScalarAluRecordError::None ||
        !result.record->scalar_alu.has_value()) {
      return fail("expected signed I32 add/sub with I64 result to prepare");
    }
    const auto& alu = *result.record->scalar_alu;
    const auto* lhs = std::get_if<aarch64_codegen::RegisterOperand>(&alu.lhs.payload);
    const auto* rhs = std::get_if<aarch64_codegen::RegisterOperand>(&alu.rhs.payload);
    if (alu.operation != aarch64_codegen::scalar_alu_operation_from_binary_opcode(opcode) ||
        alu.operand_type != bir::TypeKind::I32 ||
        alu.result_type != bir::TypeKind::I64 ||
        !alu.post_sign_extend_result_bits.has_value() ||
        *alu.post_sign_extend_result_bits != 32U ||
        alu.post_zero_extend_result_bits.has_value() ||
        lhs == nullptr ||
        rhs == nullptr ||
        lhs->expected_view != aarch64_abi::RegisterView::W ||
        rhs->expected_view != aarch64_abi::RegisterView::W ||
        !result.record->result_register.has_value() ||
        result.record->result_register->expected_view != aarch64_abi::RegisterView::X) {
      return fail("expected signed I32-to-I64 ALU route to carry post sign-extension facts");
    }
  }

  bir::BinaryInst bitwise{
      .opcode = bir::BinaryOpcode::And,
      .result = named_value(bir::TypeKind::I64, "%sum"),
      .operand_type = bir::TypeKind::I32,
      .lhs = named_value(bir::TypeKind::I32, "%lhs"),
      .rhs = named_value(bir::TypeKind::I32, "%rhs"),
  };
  const auto bitwise_result = aarch64_codegen::make_prepared_scalar_alu_record(
      fixture.names, fixture.locations, fixture.storage, bitwise);
  if (!bitwise_result.record.has_value() ||
      bitwise_result.record->post_sign_extend_result_bits.has_value()) {
    return fail("expected bitwise I32-to-I64 ALU route not to claim signed extension");
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

  auto fp = make_f64_fixture();
  const auto f128 = aarch64_codegen::make_prepared_scalar_alu_record(
      fp.names, fp.locations, fp.storage, binary(bir::BinaryOpcode::Mul, bir::TypeKind::F128));
  if (f128.record.has_value() ||
      f128.error != aarch64_codegen::PreparedScalarAluRecordError::UnsupportedOpcode) {
    return fail("expected F128 scalar ALU to fail closed");
  }

  const auto f128_sign_bit_like = aarch64_codegen::make_prepared_scalar_alu_record(
      fp.names, fp.locations, fp.storage, binary(bir::BinaryOpcode::Xor, bir::TypeKind::F128));
  if (f128_sign_bit_like.record.has_value() ||
      f128_sign_bit_like.error !=
          aarch64_codegen::PreparedScalarAluRecordError::UnsupportedOpcode) {
    return fail("expected F128 sign-bit-like xor to stay out of scalar ALU records");
  }

  fp.storage.values[0] = register_storage(prepare::PreparedValueId{20}, fp.lhs_name, "x1");
  fp.locations.value_homes[0].register_name = "x1";
  const auto wrong_bank = aarch64_codegen::make_prepared_scalar_alu_record(
      fp.names, fp.locations, fp.storage, binary(bir::BinaryOpcode::Add, bir::TypeKind::F64));
  if (wrong_bank.record.has_value() ||
      wrong_bank.error != aarch64_codegen::PreparedScalarAluRecordError::RegisterConversionFailed) {
    return fail("expected F64 scalar ALU with GPR storage to fail closed");
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

  return 0;
}

}  // namespace

int main() {
  if (const int status = supported_scalar_alu_records_preserve_prepared_and_bir_facts();
      status != 0) {
    return status;
  }
  if (const int status = shift_immediate_prepares_result_home_and_shift_operand();
      status != 0) {
    return status;
  }
  if (const int status = supported_floating_scalar_alu_records_preserve_fpr_facts();
      status != 0) {
    return status;
  }
  if (const int status = prepared_scalar_alu_registers_prefer_storage_register_placement();
      status != 0) {
    return status;
  }
  if (const int status = prepared_i32_scalar_alu_accepts_abi_x_register_parameter_spellings();
      status != 0) {
    return status;
  }
  if (const int status = prepared_scalar_unary_records_preserve_i32_i64_register_facts();
      status != 0) {
    return status;
  }
  if (const int status = prepared_scalar_unary_records_preserve_i16_byte_swap_register_facts();
      status != 0) {
    return status;
  }
  if (const int status = return_selected_scalar_operands_prefer_storage_register_placement();
      status != 0) {
    return status;
  }
  if (const int status = named_rematerialized_immediate_operands_preserve_source_identity();
      status != 0) {
    return status;
  }
  if (const int status = unsigned_power_of_two_reductions_prepare_as_shift_and_mask();
      status != 0) {
    return status;
  }
  if (const int status = narrow_unsigned_reductions_prepare_explicit_zero_extension();
      status != 0) {
    return status;
  }
  if (const int status = signed_i32_alu_results_prepare_explicit_sign_extension();
      status != 0) {
    return status;
  }
  if (const int status = unsupported_and_incomplete_facts_fail_closed(); status != 0) {
    return status;
  }
  return 0;
}
