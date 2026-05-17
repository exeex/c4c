#include "f128.hpp"

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

}  // namespace

PreparedF128TransportRecordResult f128_transport_record_error(
    PreparedF128TransportRecordError error) {
  return PreparedF128TransportRecordResult{
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

}  // namespace c4c::backend::aarch64::codegen
