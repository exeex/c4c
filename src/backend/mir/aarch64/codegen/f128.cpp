#include "f128.hpp"
#include "calls.hpp"

#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace {

std::string_view f128_register_display_name(abi::RegisterReference reg) {
  static constexpr std::string_view x_names[] = {
      "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",
      "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
      "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
      "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31"};
  static constexpr std::string_view w_names[] = {
      "w0",  "w1",  "w2",  "w3",  "w4",  "w5",  "w6",  "w7",
      "w8",  "w9",  "w10", "w11", "w12", "w13", "w14", "w15",
      "w16", "w17", "w18", "w19", "w20", "w21", "w22", "w23",
      "w24", "w25", "w26", "w27", "w28", "w29", "w30", "w31"};
  static constexpr std::string_view v_names[] = {
      "v0",  "v1",  "v2",  "v3",  "v4",  "v5",  "v6",  "v7",
      "v8",  "v9",  "v10", "v11", "v12", "v13", "v14", "v15",
      "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",
      "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31"};
  static constexpr std::string_view s_names[] = {
      "s0",  "s1",  "s2",  "s3",  "s4",  "s5",  "s6",  "s7",
      "s8",  "s9",  "s10", "s11", "s12", "s13", "s14", "s15",
      "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23",
      "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31"};
  static constexpr std::string_view d_names[] = {
      "d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
      "d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15",
      "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
      "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31"};
  static constexpr std::string_view q_names[] = {
      "q0",  "q1",  "q2",  "q3",  "q4",  "q5",  "q6",  "q7",
      "q8",  "q9",  "q10", "q11", "q12", "q13", "q14", "q15",
      "q16", "q17", "q18", "q19", "q20", "q21", "q22", "q23",
      "q24", "q25", "q26", "q27", "q28", "q29", "q30", "q31"};
  if (reg.index >= 32U) {
    return {};
  }
  switch (reg.view) {
    case abi::RegisterView::X:
      return x_names[reg.index];
    case abi::RegisterView::W:
      return w_names[reg.index];
    case abi::RegisterView::V:
      return v_names[reg.index];
    case abi::RegisterView::S:
      return s_names[reg.index];
    case abi::RegisterView::D:
      return d_names[reg.index];
    case abi::RegisterView::Q:
      return q_names[reg.index];
    case abi::RegisterView::Sp:
      return "sp";
  }
  return {};
}

std::vector<std::string_view> f128_occupied_register_views(
    abi::RegisterReference reg) {
  const auto display_name = f128_register_display_name(reg);
  if (display_name.empty()) {
    return {};
  }
  return {display_name};
}

std::vector<abi::RegisterReference> f128_occupied_register_references(
    abi::RegisterReference reg) {
  return {reg};
}

MachineEffectResource f128_register_effect(const RegisterOperand& reg) {
  MachineEffectResource resource;
  resource.operand = make_register_operand(reg);
  resource.kind = MachineEffectResourceKind::Register;
  resource.value_id = reg.value_id;
  resource.value_name = reg.value_name;
  resource.reg = reg.reg;
  return resource;
}

MachineEffectResource f128_memory_effect(const MemoryOperand& memory) {
  MachineEffectResource resource;
  resource.operand = make_memory_operand(memory);
  resource.kind = MachineEffectResourceKind::Memory;
  resource.value_id =
      memory.result_value_id.has_value() ? memory.result_value_id : memory.stored_value_id;
  if (memory.result_value_name.has_value()) {
    resource.value_name = *memory.result_value_name;
  } else if (memory.stored_value_name.has_value()) {
    resource.value_name = *memory.stored_value_name;
  }
  resource.frame_slot_id = memory.frame_slot_id;
  resource.symbol_name =
      memory.symbol_name.has_value() ? memory.symbol_name : memory.string_symbol_name;
  return resource;
}

MachineEffectResource f128_prepared_value_def(
    std::optional<prepare::PreparedValueId> value_id,
    c4c::ValueNameId value_name) {
  return MachineEffectResource{
      .kind = MachineEffectResourceKind::PreparedValue,
      .value_id = value_id,
      .value_name = value_name,
  };
}

MachineNodeStatusRecord validate_f128_transport_instruction(
    const F128TransportRecord& instruction) {
  if (instruction.source_carrier == nullptr) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 transport is missing prepared f128 carrier provenance"};
  }
  if (instruction.carrier_kind == prepare::PreparedF128CarrierKind::Missing) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 transport carrier is missing complete full-width authority"};
  }
  if (instruction.total_size_bytes != 16 || instruction.total_align_bytes != 16) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 transport carrier requires complete 16-byte size and alignment"};
  }
  if (instruction.carrier_kind == prepare::PreparedF128CarrierKind::FullWidthRegister &&
      !instruction.reg.has_value()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 full-width register transport is missing q-register authority"};
  }
  if (instruction.carrier_kind == prepare::PreparedF128CarrierKind::MemoryBacked &&
      (!instruction.slot_id.has_value() || !instruction.stack_offset_bytes.has_value())) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 memory-backed transport is missing frame-slot authority"};
  }
  if ((instruction.transport_kind == F128TransportKind::LoadFromMemory ||
       instruction.transport_kind == F128TransportKind::StoreToMemory) &&
      !instruction.memory.has_value()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 memory transport is missing structured memory operand"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

MachineNodeStatusRecord validate_f128_runtime_helper_boundary_instruction(
    const F128RuntimeHelperBoundaryRecord& instruction) {
  const auto has_full_width_register = [](const F128RuntimeHelperOperandRecord& operand) {
    return operand.source_carrier != nullptr &&
           operand.carrier_kind == prepare::PreparedF128CarrierKind::FullWidthRegister &&
           operand.carrier_register.has_value() &&
           operand.abi_register.has_value() &&
           operand.width_bytes == 16 &&
           operand.align_bytes == 16;
  };
  if (instruction.source_helper == nullptr) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 helper boundary is missing prepared helper provenance"};
  }
  const bool supported_helper =
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::Add &&
       instruction.source_binary_opcode == bir::BinaryOpcode::Add &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::Add) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::Sub &&
       instruction.source_binary_opcode == bir::BinaryOpcode::Sub &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::Sub) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::Mul &&
       instruction.source_binary_opcode == bir::BinaryOpcode::Mul &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::Mul) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::Div &&
       instruction.source_binary_opcode == bir::BinaryOpcode::SDiv &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::Div) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::Eq &&
       instruction.source_binary_opcode == bir::BinaryOpcode::Eq &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::Eq) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::Ne &&
       instruction.source_binary_opcode == bir::BinaryOpcode::Ne &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::Ne) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::Lt &&
       instruction.source_binary_opcode == bir::BinaryOpcode::Slt &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::Lt) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::Le &&
       instruction.source_binary_opcode == bir::BinaryOpcode::Sle &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::Le) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::Gt &&
       instruction.source_binary_opcode == bir::BinaryOpcode::Sgt &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::Gt) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::Ge &&
       instruction.source_binary_opcode == bir::BinaryOpcode::Sge &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::Ge) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::F32ToF128 &&
       instruction.source_cast_opcode == bir::CastOpcode::FPExt &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::F32ToF128) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::F64ToF128 &&
       instruction.source_cast_opcode == bir::CastOpcode::FPExt &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::F64ToF128) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::F128ToF32 &&
       instruction.source_cast_opcode == bir::CastOpcode::FPTrunc &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::F128ToF32) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::F128ToF64 &&
       instruction.source_cast_opcode == bir::CastOpcode::FPTrunc &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::F128ToF64);
  const bool comparison_helper =
      instruction.helper_family == prepare::PreparedF128RuntimeHelperFamily::Comparison;
  const bool cast_helper =
      instruction.helper_family == prepare::PreparedF128RuntimeHelperFamily::Cast;
  if ((instruction.helper_family != prepare::PreparedF128RuntimeHelperFamily::Arithmetic &&
       !comparison_helper && !cast_helper) ||
      !supported_helper ||
      instruction.callee_name.empty()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 helper boundary is missing helper family or callee identity"};
  }
  if ((!comparison_helper && !cast_helper &&
       instruction.result_ownership !=
           prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier) ||
      (comparison_helper &&
       instruction.result_ownership !=
           prepare::PreparedF128RuntimeHelperResultOwnership::ScalarCmpResult) ||
      (cast_helper && instruction.result_type == bir::TypeKind::F128 &&
       instruction.result_ownership !=
           prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier) ||
      (cast_helper && instruction.source_type == bir::TypeKind::F128 &&
       instruction.result_ownership !=
           prepare::PreparedF128RuntimeHelperResultOwnership::ScalarValue)) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic = "f128 helper boundary requires modeled result ownership"};
  }
  if (instruction.width_bytes != 16 ||
      instruction.align_bytes != 16 ||
      (!cast_helper &&
       (instruction.source_type != bir::TypeKind::F128 ||
        (!comparison_helper ? instruction.result_type != bir::TypeKind::F128
                            : instruction.result_type != bir::TypeKind::I32))) ||
      (cast_helper && !((instruction.source_type == bir::TypeKind::F32 &&
                         instruction.result_type == bir::TypeKind::F128) ||
                        (instruction.source_type == bir::TypeKind::F64 &&
                         instruction.result_type == bir::TypeKind::F128) ||
                        (instruction.source_type == bir::TypeKind::F128 &&
                         instruction.result_type == bir::TypeKind::F32) ||
                        (instruction.source_type == bir::TypeKind::F128 &&
                         instruction.result_type == bir::TypeKind::F64)))) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 helper boundary has invalid operation, type, size, or alignment facts"};
  }
  const auto has_scalar_fp = [](const F128RuntimeHelperScalarResultRecord& scalar) {
    return (scalar.type == bir::TypeKind::F32 || scalar.type == bir::TypeKind::F64) &&
           (scalar.width_bytes == 4 || scalar.width_bytes == 8) &&
           scalar.register_bank == prepare::PreparedRegisterBank::Fpr &&
           scalar.materialized_i1_register.has_value() &&
           scalar.abi_register.has_value();
  };
  if ((!comparison_helper && !cast_helper && !has_full_width_register(instruction.result)) ||
      (cast_helper && instruction.result_type == bir::TypeKind::F128 &&
       (!has_scalar_fp(instruction.scalar_operand) ||
        !has_full_width_register(instruction.result))) ||
      (cast_helper && instruction.source_type == bir::TypeKind::F128 &&
       (!has_full_width_register(instruction.lhs) || !has_scalar_fp(instruction.scalar_result))) ||
      (comparison_helper &&
       (instruction.scalar_result.type != bir::TypeKind::I32 ||
        instruction.scalar_result.width_bytes != 4 ||
        instruction.scalar_result.register_bank != prepare::PreparedRegisterBank::Gpr ||
        !instruction.scalar_result.abi_register.has_value() ||
        !instruction.scalar_result.materialized_i1_register.has_value() ||
        !instruction.scalar_result.cmp_result_consumption.has_value() ||
        instruction.scalar_result.cmp_result_consumption->cmp_type != bir::TypeKind::I32 ||
        instruction.scalar_result.cmp_result_consumption->bir_result_type !=
            bir::TypeKind::I1 ||
        instruction.scalar_result.cmp_result_consumption->zero_test ==
            prepare::PreparedF128CmpResultZeroTest::Missing ||
        !instruction.scalar_result.cmp_result_consumption->consumes_helper_cmp_result ||
        !instruction.scalar_result.cmp_result_consumption->owns_bir_i1_result)) ||
      (!cast_helper && !has_full_width_register(instruction.lhs)) ||
      (!cast_helper && !has_full_width_register(instruction.rhs))) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 helper boundary is missing full-width q-register carrier or ABI facts"};
  }
  if (!instruction.resource_policy.call_boundary ||
      !instruction.resource_policy.runtime_helper_callee ||
      !instruction.resource_policy.caller_saved_clobbers ||
      !instruction.resource_policy.preserves_source_operation_identity) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 helper boundary is missing resource policy facts"};
  }
  if ((!comparison_helper && !cast_helper &&
       instruction.abi_policy.transition !=
           prepare::PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentsAndResult) ||
      (comparison_helper &&
       instruction.abi_policy.transition !=
           prepare::PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentsAndCmpResult) ||
      (cast_helper && instruction.result_type == bir::TypeKind::F128 &&
       instruction.abi_policy.transition !=
           prepare::PreparedF128RuntimeHelperAbiTransition::DirectScalarArgumentAndF128Result) ||
      (cast_helper && instruction.source_type == bir::TypeKind::F128 &&
       instruction.abi_policy.transition !=
           prepare::PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentAndScalarResult) ||
      (!cast_helper && instruction.abi_policy.argument_bank != prepare::PreparedRegisterBank::Vreg) ||
      (cast_helper && instruction.result_type == bir::TypeKind::F128 &&
       instruction.abi_policy.argument_bank != prepare::PreparedRegisterBank::Fpr) ||
      (cast_helper && instruction.source_type == bir::TypeKind::F128 &&
       instruction.abi_policy.argument_bank != prepare::PreparedRegisterBank::Vreg) ||
      (!comparison_helper && instruction.result_type == bir::TypeKind::F128
           ? instruction.abi_policy.result_bank != prepare::PreparedRegisterBank::Vreg
           : (comparison_helper
                  ? instruction.abi_policy.result_bank != prepare::PreparedRegisterBank::Gpr
                  : instruction.abi_policy.result_bank != prepare::PreparedRegisterBank::Fpr)) ||
      (!cast_helper ? instruction.abi_policy.argument_count != 2
                    : instruction.abi_policy.argument_count != 1) ||
      instruction.abi_policy.result_count != 1 ||
      (!comparison_helper && instruction.result_type == bir::TypeKind::F128
           ? instruction.abi_policy.width_bytes != 16
           : (comparison_helper ? instruction.abi_policy.width_bytes != 4
                                : (instruction.abi_policy.width_bytes != 4 &&
                                   instruction.abi_policy.width_bytes != 8)))) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 helper boundary is missing ABI/register-bank policy facts"};
  }
  if (instruction.clobbered_registers.empty()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 helper boundary is missing caller-saved clobber policy"};
  }
  if (!instruction.live_preservation_policy.evaluated ||
      !instruction.live_preservation_policy.caller_saved_clobbers_modeled ||
      !instruction.live_preservation_policy.no_additional_live_preservation_required) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 helper boundary is missing live-preservation policy"};
  }
  if (!instruction.selected_call_ownership.owns_terminal_call ||
      !instruction.selected_call_ownership.has_callee_identity ||
      !instruction.selected_call_ownership.has_resource_policy ||
      !instruction.selected_call_ownership.has_clobber_policy ||
      !instruction.selected_call_ownership.has_abi_bindings ||
      !instruction.selected_call_ownership.has_marshaling ||
      !instruction.selected_call_ownership.has_live_preservation) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 helper boundary is missing selected-call ownership policy"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

}  // namespace

PreparedF128TransportRecordResult f128_transport_record_error(
    PreparedF128TransportRecordError error) {
  return PreparedF128TransportRecordResult{
      .record = std::nullopt,
      .error = error,
  };
}

PreparedF128RuntimeHelperRecordResult f128_runtime_helper_record_error(
    PreparedF128RuntimeHelperRecordError error) {
  return PreparedF128RuntimeHelperRecordResult{
      .record = std::nullopt,
      .error = error,
  };
}

std::optional<RegisterOperand> make_f128_register_operand(
    const prepare::PreparedF128Carrier& carrier) {
  if (!carrier.register_name.has_value()) {
    return std::nullopt;
  }
  const auto converted = abi::convert_prepared_register(
      *carrier.register_name, carrier.register_bank, carrier.register_class, abi::RegisterView::Q);
  if (!converted.has_value()) {
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = carrier.value_id,
      .value_name = carrier.value_name,
      .prepared_class = carrier.register_class,
      .prepared_bank = carrier.register_bank,
      .expected_view = abi::RegisterView::Q,
      .contiguous_width = carrier.contiguous_width,
      .occupied_register_references = f128_occupied_register_references(*converted.reg),
      .occupied_registers = f128_occupied_register_views(*converted.reg),
  };
}

std::optional<RegisterOperand> make_f128_abi_register_operand(
    const prepare::PreparedF128RuntimeHelper::AbiRegisterBinding& binding) {
  abi::RegisterView view = abi::RegisterView::Q;
  if (binding.register_bank == prepare::PreparedRegisterBank::Gpr) {
    view = abi::RegisterView::W;
  } else if (binding.register_bank == prepare::PreparedRegisterBank::Fpr) {
    view = binding.width_bytes == 4 ? abi::RegisterView::S : abi::RegisterView::D;
  }
  const auto converted = abi::convert_prepared_register(
      binding.register_name, binding.register_bank, binding.register_class, view);
  if (!converted.has_value()) {
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::CallAbi,
      .value_id = binding.value_id,
      .value_name = binding.value_name,
      .prepared_class = binding.register_class,
      .prepared_bank = binding.register_bank,
      .expected_view = view,
      .contiguous_width = binding.contiguous_width,
      .occupied_register_references = f128_occupied_register_references(*converted.reg),
      .occupied_registers = f128_occupied_register_views(*converted.reg),
  };
}

std::optional<RegisterOperand> make_f128_scalar_register_operand(
    const prepare::PreparedF128RuntimeHelper::ScalarResultOwnership& scalar) {
  if (!scalar.register_name.has_value()) {
    return std::nullopt;
  }
  abi::RegisterView view = abi::RegisterView::W;
  prepare::PreparedRegisterClass prepared_class = prepare::PreparedRegisterClass::General;
  switch (scalar.register_bank) {
    case prepare::PreparedRegisterBank::Gpr:
      view = abi::RegisterView::W;
      prepared_class = prepare::PreparedRegisterClass::General;
      break;
    case prepare::PreparedRegisterBank::Fpr:
      view = scalar.width_bytes == 4 ? abi::RegisterView::S : abi::RegisterView::D;
      prepared_class = prepare::PreparedRegisterClass::Float;
      break;
    case prepare::PreparedRegisterBank::Vreg:
    case prepare::PreparedRegisterBank::AggregateAddress:
    case prepare::PreparedRegisterBank::None:
      return std::nullopt;
  }
  const auto converted = abi::convert_prepared_register(
      *scalar.register_name, scalar.register_bank, prepared_class, view);
  if (!converted.has_value()) {
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = scalar.value_id,
      .value_name = scalar.value_name,
      .prepared_class = prepared_class,
      .prepared_bank = scalar.register_bank,
      .expected_view = view,
      .contiguous_width = 1,
      .occupied_register_references = f128_occupied_register_references(*converted.reg),
      .occupied_registers = f128_occupied_register_views(*converted.reg),
  };
}

std::optional<RegisterOperand> make_f128_cmp_materialized_i1_register_operand(
    const prepare::PreparedF128RuntimeHelper::ScalarResultOwnership& scalar) {
  if (!scalar.register_name.has_value() ||
      scalar.register_bank != prepare::PreparedRegisterBank::Gpr) {
    return std::nullopt;
  }
  const auto converted = abi::convert_prepared_register(
      *scalar.register_name,
      scalar.register_bank,
      prepare::PreparedRegisterClass::General,
      abi::RegisterView::W);
  if (!converted.has_value()) {
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = scalar.value_id,
      .value_name = scalar.value_name,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = scalar.register_bank,
      .expected_view = abi::RegisterView::W,
      .contiguous_width = 1,
      .occupied_register_references = f128_occupied_register_references(*converted.reg),
      .occupied_registers = f128_occupied_register_views(*converted.reg),
  };
}

PreparedF128TransportRecordResult make_prepared_f128_carrier_transport_record(
    const prepare::PreparedF128CarrierFunction& f128_carriers,
    c4c::ValueNameId value_name,
    F128TransportKind transport_kind,
    std::optional<MemoryOperand> memory) {
  if (f128_carriers.function_name == c4c::kInvalidFunctionName) {
    return f128_transport_record_error(PreparedF128TransportRecordError::InvalidFunction);
  }
  const auto* carrier = prepare::find_prepared_f128_carrier(f128_carriers, value_name);
  if (carrier == nullptr) {
    return f128_transport_record_error(
        PreparedF128TransportRecordError::MissingPreparedF128Carrier);
  }
  if (!carrier->missing_required_facts.empty() ||
      carrier->kind == prepare::PreparedF128CarrierKind::Missing) {
    return f128_transport_record_error(
        PreparedF128TransportRecordError::IncompletePreparedF128Carrier);
  }
  if (carrier->total_size_bytes != 16 || carrier->total_align_bytes != 16) {
    return f128_transport_record_error(
        PreparedF128TransportRecordError::IncompletePreparedF128Carrier);
  }
  if (transport_kind != F128TransportKind::CarrierSnapshot) {
    if (!memory.has_value()) {
      return f128_transport_record_error(PreparedF128TransportRecordError::MissingMemoryOperand);
    }
    if (memory->size_bytes != carrier->total_size_bytes) {
      return f128_transport_record_error(
          PreparedF128TransportRecordError::MemoryAccessSizeMismatch);
    }
  }

  F128TransportRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .transport_kind = transport_kind,
      .function_name = f128_carriers.function_name,
      .block_label = memory.has_value() ? memory->block_label : c4c::kInvalidBlockLabel,
      .instruction_index = memory.has_value() ? memory->instruction_index : 0,
      .value_id = carrier->value_id,
      .value_name = carrier->value_name,
      .value_type = bir::TypeKind::F128,
      .carrier_kind = carrier->kind,
      .total_size_bytes = carrier->total_size_bytes,
      .total_align_bytes = carrier->total_align_bytes,
      .register_bank = carrier->register_bank,
      .register_class = carrier->register_class,
      .contiguous_width = carrier->contiguous_width,
      .occupied_register_names = carrier->occupied_register_names,
      .register_placement = carrier->register_placement,
      .slot_id = carrier->slot_id,
      .stack_offset_bytes = carrier->stack_offset_bytes,
      .memory = std::move(memory),
      .source_carrier = carrier,
  };

  switch (carrier->kind) {
    case prepare::PreparedF128CarrierKind::FullWidthRegister:
      record.reg = make_f128_register_operand(*carrier);
      if (!record.reg.has_value()) {
        return f128_transport_record_error(
            PreparedF128TransportRecordError::RegisterConversionFailed);
      }
      break;
    case prepare::PreparedF128CarrierKind::MemoryBacked:
      if (!record.slot_id.has_value() || !record.stack_offset_bytes.has_value()) {
        return f128_transport_record_error(
            PreparedF128TransportRecordError::IncompletePreparedF128Carrier);
      }
      break;
    case prepare::PreparedF128CarrierKind::Missing:
      return f128_transport_record_error(
          PreparedF128TransportRecordError::IncompletePreparedF128Carrier);
  }

  return PreparedF128TransportRecordResult{
      .record = std::move(record),
      .error = PreparedF128TransportRecordError::None,
  };
}

PreparedF128RuntimeHelperRecordError make_f128_helper_operand_record(
    const prepare::PreparedF128CarrierFunction& f128_carriers,
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    const std::optional<prepare::PreparedF128RuntimeHelper::CarrierBinding>& carrier_binding,
    const std::optional<prepare::PreparedF128RuntimeHelper::AbiRegisterBinding>& abi_binding,
    const std::optional<prepare::PreparedF128RuntimeHelper::MarshalingMove>& marshaling_move,
    prepare::PreparedF128RuntimeHelperMarshalDirection direction,
    F128RuntimeHelperOperandRecord& operand) {
  if (!carrier_binding.has_value() ||
      !abi_binding.has_value() ||
      !marshaling_move.has_value()) {
    return PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper;
  }
  if (carrier_binding->value_id != value_id ||
      carrier_binding->value_name != value_name ||
      abi_binding->value_id != value_id ||
      abi_binding->value_name != value_name ||
      marshaling_move->carrier.value_id != value_id ||
      marshaling_move->carrier.value_name != value_name ||
      marshaling_move->abi_register.value_id != value_id ||
      marshaling_move->abi_register.value_name != value_name ||
      marshaling_move->direction != direction) {
    return PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper;
  }
  if (carrier_binding->carrier_kind != prepare::PreparedF128CarrierKind::FullWidthRegister ||
      carrier_binding->width_bytes != 16 ||
      carrier_binding->align_bytes != 16 ||
      carrier_binding->register_bank != prepare::PreparedRegisterBank::Vreg ||
      carrier_binding->register_class != prepare::PreparedRegisterClass::Vector ||
      !carrier_binding->register_name.has_value()) {
    return PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper;
  }
  if (abi_binding->width_bytes != 16 ||
      abi_binding->register_bank != prepare::PreparedRegisterBank::Vreg ||
      abi_binding->register_class != prepare::PreparedRegisterClass::Vector ||
      abi_binding->register_name.empty() ||
      abi_binding->contiguous_width != 1 ||
      abi_binding->occupied_register_names.empty() ||
      !abi_binding->register_placement.has_value()) {
    return PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper;
  }

  const auto* carrier = prepare::find_prepared_f128_carrier(f128_carriers, value_id);
  if (carrier == nullptr || carrier->value_name != value_name) {
    return PreparedF128RuntimeHelperRecordError::MissingPreparedF128Carrier;
  }
  if (!carrier->missing_required_facts.empty() ||
      carrier->kind == prepare::PreparedF128CarrierKind::Missing ||
      carrier->total_size_bytes != 16 ||
      carrier->total_align_bytes != 16) {
    return PreparedF128RuntimeHelperRecordError::IncompletePreparedF128Carrier;
  }
  if (carrier->kind != prepare::PreparedF128CarrierKind::FullWidthRegister) {
    return PreparedF128RuntimeHelperRecordError::UnsupportedCarrierKind;
  }
  if (!carrier->register_name.has_value() ||
      carrier->register_name != carrier_binding->register_name ||
      carrier->register_bank != carrier_binding->register_bank ||
      carrier->register_class != carrier_binding->register_class) {
    return PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper;
  }

  operand = F128RuntimeHelperOperandRecord{
      .value_id = value_id,
      .value_name = value_name,
      .carrier_kind = carrier->kind,
      .width_bytes = carrier->total_size_bytes,
      .align_bytes = carrier->total_align_bytes,
      .register_bank = carrier->register_bank,
      .register_class = carrier->register_class,
      .carrier_binding = *carrier_binding,
      .abi_binding = *abi_binding,
      .marshaling_move = *marshaling_move,
      .source_carrier = carrier,
  };
  operand.carrier_register = make_f128_register_operand(*carrier);
  operand.abi_register = make_f128_abi_register_operand(*abi_binding);
  if (!operand.carrier_register.has_value() || !operand.abi_register.has_value()) {
    return PreparedF128RuntimeHelperRecordError::RegisterConversionFailed;
  }
  return PreparedF128RuntimeHelperRecordError::None;
}

PreparedF128RuntimeHelperRecordError make_f128_helper_scalar_record(
    const std::optional<prepare::PreparedF128RuntimeHelper::ScalarResultOwnership>& scalar,
    const std::optional<prepare::PreparedF128RuntimeHelper::AbiRegisterBinding>& abi_binding,
    const std::optional<prepare::PreparedF128RuntimeHelper::ScalarMarshalingMove>& marshaling_move,
    prepare::PreparedF128RuntimeHelperMarshalDirection direction,
    F128RuntimeHelperScalarResultRecord& record) {
  if (!scalar.has_value() || !abi_binding.has_value() || !marshaling_move.has_value()) {
    return PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper;
  }
  if (marshaling_move->direction != direction ||
      marshaling_move->scalar_result.value_id != scalar->value_id ||
      marshaling_move->scalar_result.value_name != scalar->value_name ||
      marshaling_move->abi_register.value_id != abi_binding->value_id ||
      marshaling_move->abi_register.value_name != abi_binding->value_name) {
    return PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper;
  }
  if (scalar->register_bank != prepare::PreparedRegisterBank::Fpr ||
      (scalar->width_bytes != 4 && scalar->width_bytes != 8) ||
      abi_binding->register_bank != prepare::PreparedRegisterBank::Fpr ||
      abi_binding->width_bytes != scalar->width_bytes) {
    return PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper;
  }
  record = F128RuntimeHelperScalarResultRecord{
      .value_id = scalar->value_id,
      .value_name = scalar->value_name,
      .type = scalar->type,
      .width_bytes = scalar->width_bytes,
      .register_bank = scalar->register_bank,
      .home_kind = scalar->home_kind,
      .materialized_i1_register = make_f128_scalar_register_operand(*scalar),
      .abi_register = make_f128_abi_register_operand(*abi_binding),
      .scalar_ownership = *scalar,
      .marshaling_move = marshaling_move,
  };
  if (!record.materialized_i1_register.has_value() || !record.abi_register.has_value()) {
    return PreparedF128RuntimeHelperRecordError::RegisterConversionFailed;
  }
  return PreparedF128RuntimeHelperRecordError::None;
}

PreparedF128RuntimeHelperRecordResult make_prepared_f128_runtime_helper_boundary_record(
    const prepare::PreparedF128CarrierFunction& f128_carriers,
    const prepare::PreparedF128RuntimeHelper& helper) {
  if (f128_carriers.function_name == c4c::kInvalidFunctionName ||
      helper.function_name != f128_carriers.function_name) {
    return f128_runtime_helper_record_error(
        PreparedF128RuntimeHelperRecordError::InvalidFunction);
  }
  if (helper.helper_family != prepare::PreparedF128RuntimeHelperFamily::Arithmetic) {
    if (helper.helper_family != prepare::PreparedF128RuntimeHelperFamily::Comparison &&
        helper.helper_family != prepare::PreparedF128RuntimeHelperFamily::Cast) {
      return f128_runtime_helper_record_error(
          PreparedF128RuntimeHelperRecordError::UnsupportedHelperFamily);
    }
  }
  auto boundary_kind = F128RuntimeHelperBoundaryKind::Add;
  if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::Add &&
      helper.source_binary_opcode == bir::BinaryOpcode::Add) {
    boundary_kind = F128RuntimeHelperBoundaryKind::Add;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::Sub &&
             helper.source_binary_opcode == bir::BinaryOpcode::Sub) {
    boundary_kind = F128RuntimeHelperBoundaryKind::Sub;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::Mul &&
             helper.source_binary_opcode == bir::BinaryOpcode::Mul) {
    boundary_kind = F128RuntimeHelperBoundaryKind::Mul;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::Div &&
             helper.source_binary_opcode == bir::BinaryOpcode::SDiv) {
    boundary_kind = F128RuntimeHelperBoundaryKind::Div;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::Eq &&
             helper.source_binary_opcode == bir::BinaryOpcode::Eq) {
    boundary_kind = F128RuntimeHelperBoundaryKind::Eq;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::Ne &&
             helper.source_binary_opcode == bir::BinaryOpcode::Ne) {
    boundary_kind = F128RuntimeHelperBoundaryKind::Ne;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::Lt &&
             helper.source_binary_opcode == bir::BinaryOpcode::Slt) {
    boundary_kind = F128RuntimeHelperBoundaryKind::Lt;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::Le &&
             helper.source_binary_opcode == bir::BinaryOpcode::Sle) {
    boundary_kind = F128RuntimeHelperBoundaryKind::Le;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::Gt &&
             helper.source_binary_opcode == bir::BinaryOpcode::Sgt) {
    boundary_kind = F128RuntimeHelperBoundaryKind::Gt;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::Ge &&
             helper.source_binary_opcode == bir::BinaryOpcode::Sge) {
    boundary_kind = F128RuntimeHelperBoundaryKind::Ge;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::F32ToF128 &&
             helper.source_cast_opcode == bir::CastOpcode::FPExt &&
             helper.source_type == bir::TypeKind::F32 &&
             helper.result_type == bir::TypeKind::F128) {
    boundary_kind = F128RuntimeHelperBoundaryKind::F32ToF128;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::F64ToF128 &&
             helper.source_cast_opcode == bir::CastOpcode::FPExt &&
             helper.source_type == bir::TypeKind::F64 &&
             helper.result_type == bir::TypeKind::F128) {
    boundary_kind = F128RuntimeHelperBoundaryKind::F64ToF128;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::F128ToF32 &&
             helper.source_cast_opcode == bir::CastOpcode::FPTrunc &&
             helper.source_type == bir::TypeKind::F128 &&
             helper.result_type == bir::TypeKind::F32) {
    boundary_kind = F128RuntimeHelperBoundaryKind::F128ToF32;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::F128ToF64 &&
             helper.source_cast_opcode == bir::CastOpcode::FPTrunc &&
             helper.source_type == bir::TypeKind::F128 &&
             helper.result_type == bir::TypeKind::F64) {
    boundary_kind = F128RuntimeHelperBoundaryKind::F128ToF64;
  } else {
    return f128_runtime_helper_record_error(
        PreparedF128RuntimeHelperRecordError::UnsupportedSourceOperation);
  }
  const bool cast_helper =
      helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Cast;
  if ((!cast_helper &&
       (helper.source_type != bir::TypeKind::F128 ||
        (helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Comparison
             ? helper.result_type != bir::TypeKind::I32
             : helper.result_type != bir::TypeKind::F128))) ||
      (cast_helper && !((helper.source_type == bir::TypeKind::F32 &&
                         helper.result_type == bir::TypeKind::F128) ||
                        (helper.source_type == bir::TypeKind::F64 &&
                         helper.result_type == bir::TypeKind::F128) ||
                        (helper.source_type == bir::TypeKind::F128 &&
                         helper.result_type == bir::TypeKind::F32) ||
                        (helper.source_type == bir::TypeKind::F128 &&
                         helper.result_type == bir::TypeKind::F64))) ||
      helper.callee_name.empty()) {
    return f128_runtime_helper_record_error(
        PreparedF128RuntimeHelperRecordError::UnsupportedSourceOperation);
  }
  if (!helper.missing_required_facts.empty()) {
    return f128_runtime_helper_record_error(
        PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper);
  }
  if ((helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Arithmetic &&
       helper.result_ownership !=
           prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier) ||
      (helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Comparison &&
       helper.result_ownership !=
           prepare::PreparedF128RuntimeHelperResultOwnership::ScalarCmpResult) ||
      (cast_helper && helper.result_type == bir::TypeKind::F128 &&
       helper.result_ownership !=
           prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier) ||
      (cast_helper && helper.source_type == bir::TypeKind::F128 &&
       helper.result_ownership !=
           prepare::PreparedF128RuntimeHelperResultOwnership::ScalarValue)) {
    return f128_runtime_helper_record_error(
        PreparedF128RuntimeHelperRecordError::UnsupportedResultOwnership);
  }
  if (!helper.resource_policy.call_boundary ||
      !helper.resource_policy.runtime_helper_callee ||
      !helper.resource_policy.caller_saved_clobbers ||
      !helper.resource_policy.preserves_source_operation_identity) {
    return f128_runtime_helper_record_error(
        PreparedF128RuntimeHelperRecordError::MissingBoundaryResourcePolicy);
  }
  if ((helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Arithmetic &&
       helper.abi_policy.transition !=
           prepare::PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentsAndResult) ||
      (helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Comparison &&
       helper.abi_policy.transition !=
           prepare::PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentsAndCmpResult) ||
      (cast_helper && helper.result_type == bir::TypeKind::F128 &&
       helper.abi_policy.transition !=
           prepare::PreparedF128RuntimeHelperAbiTransition::DirectScalarArgumentAndF128Result) ||
      (cast_helper && helper.source_type == bir::TypeKind::F128 &&
       helper.abi_policy.transition !=
           prepare::PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentAndScalarResult) ||
      (!cast_helper && helper.abi_policy.argument_bank != prepare::PreparedRegisterBank::Vreg) ||
      (cast_helper && helper.result_type == bir::TypeKind::F128 &&
       helper.abi_policy.argument_bank != prepare::PreparedRegisterBank::Fpr) ||
      (cast_helper && helper.source_type == bir::TypeKind::F128 &&
       helper.abi_policy.argument_bank != prepare::PreparedRegisterBank::Vreg) ||
      (helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Arithmetic
           ? helper.abi_policy.result_bank != prepare::PreparedRegisterBank::Vreg
           : (helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Comparison
                  ? helper.abi_policy.result_bank != prepare::PreparedRegisterBank::Gpr
                  : (helper.result_type == bir::TypeKind::F128
                         ? helper.abi_policy.result_bank != prepare::PreparedRegisterBank::Vreg
                         : helper.abi_policy.result_bank != prepare::PreparedRegisterBank::Fpr))) ||
      (!cast_helper ? helper.abi_policy.argument_count != 2
                    : helper.abi_policy.argument_count != 1) ||
      helper.abi_policy.result_count != 1 ||
      (helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Arithmetic
           ? helper.abi_policy.width_bytes != 16
           : (helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Comparison
                  ? helper.abi_policy.width_bytes != 4
                  : (helper.result_type == bir::TypeKind::F128
                         ? helper.abi_policy.width_bytes != 16
                         : (helper.abi_policy.width_bytes != 4 &&
                            helper.abi_policy.width_bytes != 8))))) {
    return f128_runtime_helper_record_error(
        PreparedF128RuntimeHelperRecordError::MissingBoundaryAbiPolicy);
  }
  if (helper.clobbered_registers.empty()) {
    return f128_runtime_helper_record_error(
        PreparedF128RuntimeHelperRecordError::MissingClobberPolicy);
  }
  if (!helper.live_preservation_policy.evaluated ||
      !helper.live_preservation_policy.caller_saved_clobbers_modeled ||
      !helper.live_preservation_policy.no_additional_live_preservation_required ||
      !helper.selected_call_ownership.owns_terminal_call ||
      !helper.selected_call_ownership.has_callee_identity ||
      !helper.selected_call_ownership.has_resource_policy ||
      !helper.selected_call_ownership.has_clobber_policy ||
      !helper.selected_call_ownership.has_abi_bindings ||
      !helper.selected_call_ownership.has_marshaling ||
      !helper.selected_call_ownership.has_live_preservation) {
    return f128_runtime_helper_record_error(
        PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper);
  }

  F128RuntimeHelperBoundaryRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .boundary_kind = boundary_kind,
      .helper_family = helper.helper_family,
      .helper_kind = helper.helper_kind,
      .callee_name = helper.callee_name,
      .source_binary_opcode = helper.source_binary_opcode,
      .source_cast_opcode = helper.source_cast_opcode,
      .function_name = helper.function_name,
      .block_index = helper.block_index,
      .instruction_index = helper.instruction_index,
      .source_type = helper.source_type,
      .result_type = helper.result_type,
      .result_value_id = helper.result_value_id,
      .result_value_name = helper.result_value_name,
      .operand_value_id = helper.operand_value_id,
      .operand_value_name = helper.operand_value_name,
      .lhs_value_id = helper.lhs_value_id,
      .lhs_value_name = helper.lhs_value_name,
      .rhs_value_id = helper.rhs_value_id,
      .rhs_value_name = helper.rhs_value_name,
      .result_ownership = helper.result_ownership,
      .width_bytes = 16,
      .align_bytes = 16,
      .resource_policy = helper.resource_policy,
      .abi_policy = helper.abi_policy,
      .live_preservation_policy = helper.live_preservation_policy,
      .selected_call_ownership = helper.selected_call_ownership,
      .clobbered_registers = helper.clobbered_registers,
      .source_helper = &helper,
  };
  if (helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Comparison) {
    if (!helper.scalar_result.has_value() || !helper.result_abi_result.has_value() ||
        !helper.scalar_result_unmarshal_move.has_value() ||
        !helper.scalar_cmp_result_consumption.has_value() ||
        helper.scalar_result->type != bir::TypeKind::I32 ||
        helper.scalar_result->width_bytes != 4 ||
        helper.result_abi_result->register_bank != prepare::PreparedRegisterBank::Gpr ||
        helper.scalar_cmp_result_consumption->cmp_type != bir::TypeKind::I32 ||
        helper.scalar_cmp_result_consumption->bir_result_type != bir::TypeKind::I1 ||
        helper.scalar_cmp_result_consumption->zero_test ==
            prepare::PreparedF128CmpResultZeroTest::Missing ||
        !helper.scalar_cmp_result_consumption->consumes_helper_cmp_result ||
        !helper.scalar_cmp_result_consumption->owns_bir_i1_result) {
      return f128_runtime_helper_record_error(
          PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper);
    }
    record.scalar_result = F128RuntimeHelperScalarResultRecord{
        .value_id = helper.scalar_result->value_id,
        .value_name = helper.scalar_result->value_name,
        .type = helper.scalar_result->type,
        .width_bytes = helper.scalar_result->width_bytes,
        .register_bank = helper.scalar_result->register_bank,
        .home_kind = helper.scalar_result->home_kind,
        .materialized_i1_register =
            make_f128_cmp_materialized_i1_register_operand(*helper.scalar_result),
        .abi_register = make_f128_abi_register_operand(*helper.result_abi_result),
        .scalar_ownership = *helper.scalar_result,
        .marshaling_move = helper.scalar_result_unmarshal_move,
        .cmp_result_consumption = helper.scalar_cmp_result_consumption,
    };
    if (!record.scalar_result.abi_register.has_value() ||
        !record.scalar_result.materialized_i1_register.has_value()) {
      return f128_runtime_helper_record_error(
          PreparedF128RuntimeHelperRecordError::RegisterConversionFailed);
    }
  } else if (cast_helper && helper.result_type == bir::TypeKind::F128) {
    if (const auto error =
            make_f128_helper_scalar_record(
                helper.scalar_operand,
                helper.scalar_operand_abi_argument,
                helper.scalar_operand_argument_move,
                prepare::PreparedF128RuntimeHelperMarshalDirection::ScalarToAbiArgument,
                record.scalar_operand);
        error != PreparedF128RuntimeHelperRecordError::None) {
      return f128_runtime_helper_record_error(error);
    }
    if (const auto error =
            make_f128_helper_operand_record(
                f128_carriers,
                helper.result_value_id,
                helper.result_value_name,
                helper.result_carrier,
                helper.result_abi_result,
                helper.result_unmarshal_move,
                prepare::PreparedF128RuntimeHelperMarshalDirection::AbiResultToCarrier,
                record.result);
        error != PreparedF128RuntimeHelperRecordError::None) {
      return f128_runtime_helper_record_error(error);
    }
  } else if (cast_helper && helper.source_type == bir::TypeKind::F128) {
    if (const auto error =
            make_f128_helper_operand_record(
                f128_carriers,
                helper.operand_value_id,
                helper.operand_value_name,
                helper.lhs_carrier,
                helper.lhs_abi_argument,
                helper.lhs_argument_move,
                prepare::PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument,
                record.lhs);
        error != PreparedF128RuntimeHelperRecordError::None) {
      return f128_runtime_helper_record_error(error);
    }
    if (const auto error =
            make_f128_helper_scalar_record(
                helper.scalar_result,
                helper.result_abi_result,
                helper.scalar_result_unmarshal_move,
                prepare::PreparedF128RuntimeHelperMarshalDirection::AbiResultToScalar,
                record.scalar_result);
        error != PreparedF128RuntimeHelperRecordError::None) {
      return f128_runtime_helper_record_error(error);
    }
  } else if (const auto error =
          make_f128_helper_operand_record(
              f128_carriers,
              helper.result_value_id,
              helper.result_value_name,
              helper.result_carrier,
              helper.result_abi_result,
              helper.result_unmarshal_move,
              prepare::PreparedF128RuntimeHelperMarshalDirection::AbiResultToCarrier,
              record.result);
      error != PreparedF128RuntimeHelperRecordError::None) {
    return f128_runtime_helper_record_error(error);
  }
  if (!cast_helper) {
    if (const auto error =
            make_f128_helper_operand_record(
                f128_carriers,
                helper.lhs_value_id,
                helper.lhs_value_name,
                helper.lhs_carrier,
                helper.lhs_abi_argument,
                helper.lhs_argument_move,
                prepare::PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument,
                record.lhs);
        error != PreparedF128RuntimeHelperRecordError::None) {
      return f128_runtime_helper_record_error(error);
    }
    if (const auto error =
            make_f128_helper_operand_record(
                f128_carriers,
                helper.rhs_value_id,
                helper.rhs_value_name,
                helper.rhs_carrier,
                helper.rhs_abi_argument,
                helper.rhs_argument_move,
                prepare::PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument,
                record.rhs);
        error != PreparedF128RuntimeHelperRecordError::None) {
      return f128_runtime_helper_record_error(error);
    }
  }
  return PreparedF128RuntimeHelperRecordResult{
      .record = std::move(record),
      .error = PreparedF128RuntimeHelperRecordError::None,
  };
}

InstructionRecord make_f128_transport_instruction(F128TransportRecord instruction) {
  std::vector<OperandRecord> operands;
  if (instruction.memory.has_value()) {
    operands.push_back(make_memory_operand(*instruction.memory));
  }
  if (instruction.reg.has_value()) {
    operands.push_back(make_register_operand(*instruction.reg));
  }

  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  if (instruction.transport_kind == F128TransportKind::LoadFromMemory ||
      instruction.transport_kind == F128TransportKind::CarrierSnapshot) {
    if (instruction.reg.has_value()) {
      defs.push_back(f128_register_effect(*instruction.reg));
    } else if (instruction.carrier_kind == prepare::PreparedF128CarrierKind::MemoryBacked) {
      defs.push_back(f128_prepared_value_def(instruction.value_id, instruction.value_name));
    }
  }
  if ((instruction.transport_kind == F128TransportKind::StoreToMemory ||
       instruction.transport_kind == F128TransportKind::CarrierSnapshot) &&
      instruction.reg.has_value()) {
    uses.push_back(f128_register_effect(*instruction.reg));
  }
  if (instruction.memory.has_value()) {
    auto memory_effect = f128_memory_effect(*instruction.memory);
    if (instruction.transport_kind == F128TransportKind::LoadFromMemory) {
      uses.push_back(std::move(memory_effect));
    } else if (instruction.transport_kind == F128TransportKind::StoreToMemory) {
      defs.push_back(std::move(memory_effect));
    }
  }

  std::vector<MachineSideEffectKind> side_effects;
  if (instruction.transport_kind == F128TransportKind::LoadFromMemory) {
    side_effects.push_back(MachineSideEffectKind::MemoryRead);
  } else if (instruction.transport_kind == F128TransportKind::StoreToMemory) {
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }

  return InstructionRecord{
      .family = InstructionFamily::F128Transport,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::F128Transport,
      .selection = validate_f128_transport_instruction(instruction),
      .function_name = instruction.function_name,
      .block_label = instruction.block_label,
      .instruction_index = instruction.instruction_index,
      .operands = std::move(operands),
      .defs = std::move(defs),
      .uses = std::move(uses),
      .side_effects = std::move(side_effects),
      .payload = instruction,
  };
}

InstructionRecord make_f128_runtime_helper_boundary_instruction(
    F128RuntimeHelperBoundaryRecord instruction) {
  std::vector<OperandRecord> operands;
  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  const auto add_def = [&](const std::optional<RegisterOperand>& reg) {
    if (reg.has_value()) {
      operands.push_back(make_register_operand(*reg));
      defs.push_back(f128_register_effect(*reg));
    }
  };
  const auto add_use = [&](const std::optional<RegisterOperand>& reg) {
    if (reg.has_value()) {
      operands.push_back(make_register_operand(*reg));
      uses.push_back(f128_register_effect(*reg));
    }
  };
  add_def(instruction.result.carrier_register);
  add_def(instruction.result.abi_register);
  add_def(instruction.scalar_result.abi_register);
  if (instruction.helper_family == prepare::PreparedF128RuntimeHelperFamily::Cast) {
    add_def(instruction.scalar_result.materialized_i1_register);
    if (instruction.scalar_operand.materialized_i1_register.has_value()) {
      add_use(instruction.scalar_operand.materialized_i1_register);
    }
    add_use(instruction.scalar_operand.abi_register);
  }
  if (instruction.helper_family == prepare::PreparedF128RuntimeHelperFamily::Comparison) {
    add_def(instruction.scalar_result.materialized_i1_register);
    if (instruction.scalar_result.abi_register.has_value()) {
      uses.push_back(f128_register_effect(*instruction.scalar_result.abi_register));
    }
    defs.push_back(f128_prepared_value_def(instruction.result_value_id,
                                           instruction.result_value_name));
  }
  add_use(instruction.lhs.carrier_register);
  add_use(instruction.lhs.abi_register);
  add_use(instruction.rhs.carrier_register);
  add_use(instruction.rhs.abi_register);

  return InstructionRecord{
      .family = InstructionFamily::CallBoundary,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::F128RuntimeHelper,
      .selection = validate_f128_runtime_helper_boundary_instruction(instruction),
      .function_name = instruction.function_name,
      .block_label = instruction.block_label,
      .block_index = instruction.block_index,
      .instruction_index = instruction.instruction_index,
      .operands = std::move(operands),
      .defs = std::move(defs),
      .uses = std::move(uses),
      .clobbers = effects_from_prepared_call_clobbers(instruction.clobbered_registers),
      .side_effects = {MachineSideEffectKind::Call},
      .payload = instruction,
  };
}

}  // namespace c4c::backend::aarch64::codegen
