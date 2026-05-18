#include "src/backend/mir/aarch64/codegen/cast_ops.hpp"
#include "src/backend/mir/aarch64/codegen/instruction.hpp"

#include <iostream>
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

const char* register_name_for_type(bir::TypeKind type, unsigned index) {
  if (type == bir::TypeKind::F64) {
    return index == 0 ? "d0" : "d1";
  }
  if (type == bir::TypeKind::F32) {
    return index == 0 ? "s0" : "s1";
  }
  if (type == bir::TypeKind::I64 || type == bir::TypeKind::Ptr) {
    return index == 0 ? "x0" : "x1";
  }
  return index == 0 ? "w0" : "w1";
}

struct PreparedCastFixture {
  prepare::PreparedNameTables names;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::ValueNameId source_name = c4c::kInvalidValueName;
  c4c::ValueNameId result_name = c4c::kInvalidValueName;
  prepare::PreparedValueLocationFunction locations;
  prepare::PreparedStoragePlanFunction storage;
};

PreparedCastFixture make_fixture(bir::TypeKind source_type, bir::TypeKind result_type) {
  PreparedCastFixture fixture;
  fixture.function_name = fixture.names.function_names.intern("f");
  fixture.source_name = fixture.names.value_names.intern("%src");
  fixture.result_name = fixture.names.value_names.intern("%cast");
  const char* source_register = register_name_for_type(source_type, 1);
  const char* result_register = register_name_for_type(result_type, 0);
  fixture.locations = prepare::PreparedValueLocationFunction{
      .function_name = fixture.function_name,
      .value_homes =
          {
              register_home(prepare::PreparedValueId{20},
                            fixture.function_name,
                            fixture.source_name,
                            source_register),
              register_home(prepare::PreparedValueId{21},
                            fixture.function_name,
                            fixture.result_name,
                            result_register),
          },
  };
  fixture.storage = prepare::PreparedStoragePlanFunction{
      .function_name = fixture.function_name,
      .values =
          {
              source_type == bir::TypeKind::F32 || source_type == bir::TypeKind::F64
                  ? fpr_storage(prepare::PreparedValueId{20}, fixture.source_name, source_register)
                  : register_storage(prepare::PreparedValueId{20}, fixture.source_name, source_register),
              result_type == bir::TypeKind::F32 || result_type == bir::TypeKind::F64
                  ? fpr_storage(prepare::PreparedValueId{21}, fixture.result_name, result_register)
                  : register_storage(prepare::PreparedValueId{21}, fixture.result_name, result_register),
          },
  };
  return fixture;
}

bir::CastInst cast_inst(bir::CastOpcode opcode,
                        bir::TypeKind source_type,
                        bir::TypeKind result_type) {
  return bir::CastInst{
      .opcode = opcode,
      .result = named_value(result_type, "%cast"),
      .operand = named_value(source_type, "%src"),
  };
}

int supported_scalar_cast_records_preserve_prepared_and_bir_facts() {
  struct Case {
    bir::CastOpcode opcode;
    bir::TypeKind source_type;
    bir::TypeKind result_type;
    aarch64_codegen::ScalarCastOperationKind operation;
    aarch64_abi::RegisterReference source_register;
    aarch64_abi::RegisterView source_view;
    aarch64_abi::RegisterReference result_register;
    aarch64_abi::RegisterView result_view;
  };
  const Case cases[] = {
      {bir::CastOpcode::SExt,
       bir::TypeKind::I32,
       bir::TypeKind::I64,
       aarch64_codegen::ScalarCastOperationKind::SignExtend,
       aarch64_abi::w_register(1),
       aarch64_abi::RegisterView::W,
       aarch64_abi::x_register(0),
       aarch64_abi::RegisterView::X},
      {bir::CastOpcode::ZExt,
       bir::TypeKind::I1,
       bir::TypeKind::I32,
       aarch64_codegen::ScalarCastOperationKind::ZeroExtend,
       aarch64_abi::w_register(1),
       aarch64_abi::RegisterView::W,
       aarch64_abi::w_register(0),
       aarch64_abi::RegisterView::W},
      {bir::CastOpcode::Trunc,
       bir::TypeKind::I64,
       bir::TypeKind::I32,
       aarch64_codegen::ScalarCastOperationKind::Truncate,
       aarch64_abi::x_register(1),
       aarch64_abi::RegisterView::X,
       aarch64_abi::w_register(0),
       aarch64_abi::RegisterView::W},
  };

  for (const auto& test_case : cases) {
    auto fixture = make_fixture(test_case.source_type, test_case.result_type);
    const auto result = aarch64_codegen::make_prepared_scalar_cast_instruction_record(
        fixture.names,
        fixture.locations,
        fixture.storage,
        cast_inst(test_case.opcode, test_case.source_type, test_case.result_type));
    if (!result.record.has_value() ||
        result.error != aarch64_codegen::PreparedScalarCastRecordError::None) {
      return fail("expected supported prepared scalar cast conversion to succeed");
    }

    const auto& instruction = *result.record;
    if (instruction.result_value_id != prepare::PreparedValueId{21} ||
        instruction.result_value_name != fixture.result_name ||
        instruction.result_type != test_case.result_type ||
        instruction.source_cast_opcode != test_case.opcode ||
        instruction.source_binary_opcode.has_value() || !instruction.scalar_cast.has_value() ||
        instruction.scalar_alu.has_value() || instruction.inputs.size() != 1 ||
        !instruction.result_register.has_value() ||
        instruction.result_register->reg != test_case.result_register ||
        instruction.result_register->expected_view != test_case.result_view ||
        instruction.result_register->role != aarch64_codegen::RegisterOperandRole::StoragePlan) {
      return fail("expected scalar instruction wrapper to preserve cast result and source opcode");
    }

    const auto& cast = *instruction.scalar_cast;
    if (cast.surface != aarch64_codegen::RecordSurfaceKind::RecordOnly ||
        cast.operation != test_case.operation || cast.source_cast_opcode != test_case.opcode ||
        cast.source_type != test_case.source_type || cast.result_type != test_case.result_type ||
        cast.result_value_id != prepare::PreparedValueId{21} ||
        cast.result_value_name != fixture.result_name || !cast.result_register.has_value() ||
        cast.result_register->reg != test_case.result_register ||
        cast.result_register->expected_view != test_case.result_view ||
        !cast.supported_simple_integer_cast) {
      return fail("expected cast record to preserve structured BIR and prepared ids");
    }

    const auto* source = std::get_if<aarch64_codegen::RegisterOperand>(&cast.source.payload);
    if (cast.source.kind != aarch64_codegen::OperandKind::Register || source == nullptr ||
        source->role != aarch64_codegen::RegisterOperandRole::StoragePlan ||
        source->value_id != prepare::PreparedValueId{20} ||
        source->value_name != fixture.source_name || source->reg != test_case.source_register ||
        source->expected_view != test_case.source_view ||
        source->prepared_bank != prepare::PreparedRegisterBank::Gpr ||
        source->prepared_class != prepare::PreparedRegisterClass::General) {
      return fail("expected cast source to become a typed prepared register operand");
    }
  }

  return 0;
}

int prepared_scalar_cast_registers_prefer_storage_register_placement() {
  auto fixture = make_fixture(bir::TypeKind::I32, bir::TypeKind::I64);
  fixture.storage.values[0].register_name = std::string{"mismatched-source-spelling"};
  fixture.storage.values[0].occupied_register_names = {"mismatched-source-spelling"};
  fixture.storage.values[0].register_placement = caller_saved_gpr(0);
  fixture.storage.values[1].register_name = std::string{"x0"};
  fixture.storage.values[1].occupied_register_names = {"x0"};
  fixture.storage.values[1].register_placement = std::nullopt;

  const auto result = aarch64_codegen::make_prepared_scalar_cast_record(
      fixture.names,
      fixture.locations,
      fixture.storage,
      cast_inst(bir::CastOpcode::SExt, bir::TypeKind::I32, bir::TypeKind::I64));
  if (!result.record.has_value() ||
      result.error != aarch64_codegen::PreparedScalarCastRecordError::None) {
    return fail("expected cast registers to resolve from storage placement before legacy names");
  }

  const auto* source =
      std::get_if<aarch64_codegen::RegisterOperand>(&result.record->source.payload);
  if (source == nullptr || source->reg != aarch64_abi::w_register(13) ||
      source->role != aarch64_codegen::RegisterOperandRole::StoragePlan ||
      source->expected_view != aarch64_abi::RegisterView::W) {
    return fail("expected cast source to use storage placement register");
  }
  if (!result.record->result_register.has_value() ||
      result.record->result_register->reg != aarch64_abi::x_register(0) ||
      result.record->result_register->role !=
          aarch64_codegen::RegisterOperandRole::StoragePlan ||
      result.record->result_register->expected_view != aarch64_abi::RegisterView::X) {
    return fail("expected cast result to use storage placement register");
  }
  return 0;
}

int supported_float_integer_and_width_conversions_preserve_bank_transition_facts() {
  struct Case {
    bir::CastOpcode opcode;
    bir::TypeKind source_type;
    bir::TypeKind result_type;
    aarch64_codegen::ScalarCastOperationKind operation;
    aarch64_abi::RegisterReference source_register;
    aarch64_abi::RegisterReference result_register;
    bool crosses_bank;
    bool float_integer;
    bool float_width;
  };
  const Case cases[] = {
      {bir::CastOpcode::SIToFP,
       bir::TypeKind::I32,
       bir::TypeKind::F64,
       aarch64_codegen::ScalarCastOperationKind::SignedIntToFloat,
       aarch64_abi::w_register(1),
       aarch64_abi::d_register(0),
       true,
       true,
       false},
      {bir::CastOpcode::UIToFP,
       bir::TypeKind::I64,
       bir::TypeKind::F32,
       aarch64_codegen::ScalarCastOperationKind::UnsignedIntToFloat,
       aarch64_abi::x_register(1),
       aarch64_abi::s_register(0),
       true,
       true,
       false},
      {bir::CastOpcode::FPToSI,
       bir::TypeKind::F32,
       bir::TypeKind::I64,
       aarch64_codegen::ScalarCastOperationKind::FloatToSignedInt,
       aarch64_abi::s_register(1),
       aarch64_abi::x_register(0),
       true,
       true,
       false},
      {bir::CastOpcode::FPToUI,
       bir::TypeKind::F64,
       bir::TypeKind::I32,
       aarch64_codegen::ScalarCastOperationKind::FloatToUnsignedInt,
       aarch64_abi::d_register(1),
       aarch64_abi::w_register(0),
       true,
       true,
       false},
      {bir::CastOpcode::FPExt,
       bir::TypeKind::F32,
       bir::TypeKind::F64,
       aarch64_codegen::ScalarCastOperationKind::FloatExtend,
       aarch64_abi::s_register(1),
       aarch64_abi::d_register(0),
       false,
       false,
       true},
      {bir::CastOpcode::FPTrunc,
       bir::TypeKind::F64,
       bir::TypeKind::F32,
       aarch64_codegen::ScalarCastOperationKind::FloatTruncate,
       aarch64_abi::d_register(1),
       aarch64_abi::s_register(0),
       false,
       false,
       true},
  };

  for (const auto& test_case : cases) {
    auto fixture = make_fixture(test_case.source_type, test_case.result_type);
    const auto result = aarch64_codegen::make_prepared_scalar_cast_instruction_record(
        fixture.names,
        fixture.locations,
        fixture.storage,
        cast_inst(test_case.opcode, test_case.source_type, test_case.result_type));
    if (!result.record.has_value() ||
        result.error != aarch64_codegen::PreparedScalarCastRecordError::None) {
      return fail("expected supported prepared conversion cast to succeed");
    }
    const auto& cast = *result.record->scalar_cast;
    const auto* source = std::get_if<aarch64_codegen::RegisterOperand>(&cast.source.payload);
    if (cast.operation != test_case.operation ||
        cast.supported_float_integer_conversion != test_case.float_integer ||
        cast.supported_float_width_conversion != test_case.float_width ||
        cast.crosses_register_bank != test_case.crosses_bank ||
        !cast.result_register.has_value() ||
        cast.result_register->reg != test_case.result_register ||
        source == nullptr || source->reg != test_case.source_register ||
        result.record->result_register->reg != test_case.result_register) {
      return fail("expected conversion cast to preserve typed bank transition facts");
    }
  }
  return 0;
}

int unsupported_and_incomplete_cast_facts_fail_closed() {
  auto fixture = make_fixture(bir::TypeKind::I32, bir::TypeKind::I64);
  const auto unsupported = aarch64_codegen::make_prepared_scalar_cast_record(
      fixture.names,
      fixture.locations,
      fixture.storage,
      cast_inst(bir::CastOpcode::Bitcast, bir::TypeKind::F32, bir::TypeKind::F64));
  if (unsupported.record.has_value() ||
      unsupported.error != aarch64_codegen::PreparedScalarCastRecordError::UnsupportedOpcode ||
      aarch64_codegen::prepared_scalar_cast_record_error_name(unsupported.error) !=
          "unsupported_opcode") {
    return fail("expected unsupported cast opcode to fail closed");
  }

  struct F128CastCase {
    bir::CastOpcode opcode;
    bir::TypeKind source_type;
    bir::TypeKind result_type;
  };
  const F128CastCase f128_casts[] = {
      {bir::CastOpcode::FPExt, bir::TypeKind::F32, bir::TypeKind::F128},
      {bir::CastOpcode::FPExt, bir::TypeKind::F64, bir::TypeKind::F128},
      {bir::CastOpcode::FPTrunc, bir::TypeKind::F128, bir::TypeKind::F32},
      {bir::CastOpcode::FPTrunc, bir::TypeKind::F128, bir::TypeKind::F64},
  };
  for (const auto& f128_cast : f128_casts) {
    auto f128_fixture = make_fixture(bir::TypeKind::F64, bir::TypeKind::F32);
    const auto f128 = aarch64_codegen::make_prepared_scalar_cast_record(
        f128_fixture.names,
        f128_fixture.locations,
        f128_fixture.storage,
        cast_inst(f128_cast.opcode, f128_cast.source_type, f128_cast.result_type));
    if (f128.record.has_value() ||
        f128.error !=
            aarch64_codegen::PreparedScalarCastRecordError::UnsupportedOperandType) {
      return fail("expected all F128 helper casts to stay out of scalar cast records");
    }
  }
  auto f128_fixture = make_fixture(bir::TypeKind::F64, bir::TypeKind::F32);
  const auto f128_bitcast = aarch64_codegen::make_prepared_scalar_cast_record(
      f128_fixture.names,
      f128_fixture.locations,
      f128_fixture.storage,
      cast_inst(bir::CastOpcode::Bitcast, bir::TypeKind::F128, bir::TypeKind::I128));
  if (f128_bitcast.record.has_value() ||
      f128_bitcast.error !=
          aarch64_codegen::PreparedScalarCastRecordError::UnsupportedOpcode) {
    return fail("expected F128 bitcast to stay fail-closed");
  }

  const auto unsupported_type = aarch64_codegen::make_prepared_scalar_cast_record(
      fixture.names,
      fixture.locations,
      fixture.storage,
      cast_inst(bir::CastOpcode::SExt, bir::TypeKind::I64, bir::TypeKind::I128));
  if (unsupported_type.record.has_value() ||
      unsupported_type.error !=
          aarch64_codegen::PreparedScalarCastRecordError::UnsupportedOperandType) {
    return fail("expected i128 simple cast form to fail closed");
  }

  fixture = make_fixture(bir::TypeKind::I32, bir::TypeKind::I64);
  fixture.locations.value_homes.pop_back();
  const auto missing_result = aarch64_codegen::make_prepared_scalar_cast_record(
      fixture.names,
      fixture.locations,
      fixture.storage,
      cast_inst(bir::CastOpcode::SExt, bir::TypeKind::I32, bir::TypeKind::I64));
  if (missing_result.record.has_value() ||
      missing_result.error !=
          aarch64_codegen::PreparedScalarCastRecordError::MissingResultValueHome) {
    return fail("expected missing result home to fail closed");
  }

  fixture = make_fixture(bir::TypeKind::I32, bir::TypeKind::I64);
  fixture.storage.values.erase(fixture.storage.values.begin());
  const auto missing_operand_storage = aarch64_codegen::make_prepared_scalar_cast_record(
      fixture.names,
      fixture.locations,
      fixture.storage,
      cast_inst(bir::CastOpcode::SExt, bir::TypeKind::I32, bir::TypeKind::I64));
  if (missing_operand_storage.record.has_value() ||
      missing_operand_storage.error !=
          aarch64_codegen::PreparedScalarCastRecordError::MissingOperandStorage) {
    return fail("expected missing operand storage to fail closed");
  }

  fixture = make_fixture(bir::TypeKind::I32, bir::TypeKind::I64);
  fixture.storage.values[0].encoding = prepare::PreparedStorageEncodingKind::FrameSlot;
  fixture.storage.values[0].register_name = std::nullopt;
  fixture.storage.values[0].slot_id = prepare::PreparedFrameSlotId{4};
  const auto unsupported_storage = aarch64_codegen::make_prepared_scalar_cast_record(
      fixture.names,
      fixture.locations,
      fixture.storage,
      cast_inst(bir::CastOpcode::SExt, bir::TypeKind::I32, bir::TypeKind::I64));
  if (unsupported_storage.record.has_value() ||
      unsupported_storage.error !=
          aarch64_codegen::PreparedScalarCastRecordError::UnsupportedOperandStorage) {
    return fail("expected unsupported operand storage to fail closed");
  }

  fixture = make_fixture(bir::TypeKind::I32, bir::TypeKind::I64);
  fixture.locations.value_homes[0].register_name = "x1";
  fixture.storage.values[0].register_name = "x1";
  const auto view_mismatch = aarch64_codegen::make_prepared_scalar_cast_record(
      fixture.names,
      fixture.locations,
      fixture.storage,
      cast_inst(bir::CastOpcode::SExt, bir::TypeKind::I32, bir::TypeKind::I64));
  if (view_mismatch.record.has_value() ||
      view_mismatch.error !=
          aarch64_codegen::PreparedScalarCastRecordError::RegisterConversionFailed) {
    return fail("expected prepared register view mismatch to fail closed");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int status = supported_scalar_cast_records_preserve_prepared_and_bir_facts();
      status != 0) {
    return status;
  }
  if (const int status = prepared_scalar_cast_registers_prefer_storage_register_placement();
      status != 0) {
    return status;
  }
  if (const int status =
          supported_float_integer_and_width_conversions_preserve_bank_transition_facts();
      status != 0) {
    return status;
  }
  if (const int status = unsupported_and_incomplete_cast_facts_fail_closed(); status != 0) {
    return status;
  }
  return 0;
}
