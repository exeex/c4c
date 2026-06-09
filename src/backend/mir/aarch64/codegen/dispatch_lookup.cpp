#include "dispatch_lookup.hpp"

#include "../abi/abi.hpp"
#include "../../../prealloc/prepared_lookups.hpp"

namespace c4c::backend::aarch64::codegen {
namespace abi = c4c::backend::aarch64::abi;
namespace prepare = c4c::backend::prepare;

namespace {

[[nodiscard]] std::optional<c4c::ValueNameId> prepared_named_value_id(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.function.prepared == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  return prepare::resolve_prepared_value_name_id(context.function.prepared->names,
                                                 value.name);
}

}  // namespace

std::optional<RegisterOperand> make_named_prepared_result_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.function.value_locations == nullptr) {
    return std::nullopt;
  }
  const auto* home = context.function.prepared == nullptr
                         ? nullptr
                         : prepare::find_prepared_value_home_for_bir_value(
                               context.function.prepared->names,
                               context.function.value_home_lookups,
                               context.function.regalloc,
                               context.function.value_locations,
                               value);
  if (home == nullptr ||
      home->kind != prepare::PreparedValueHomeKind::Register ||
      !home->register_name.has_value()) {
    return std::nullopt;
  }
  const auto expected_view = scalar_register_view(value.type);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }
  const auto parsed = abi::parse_aarch64_register_name(*home->register_name);
  if (!parsed.has_value() ||
      parsed->bank != abi::RegisterBank::GeneralPurpose) {
    return std::nullopt;
  }
  const auto viewed = abi::gp_register(parsed->index, *expected_view);
  if (!viewed.has_value()) {
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *viewed,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = home->value_id,
      .value_name = home->value_name,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = expected_view,
  };
}

bool emitted_scalar_value_available(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const BlockScalarLoweringState& scalar_state) {
  const auto value_name = prepared_named_value_id(context, value);
  return value_name.has_value() &&
         find_emitted_scalar_register(scalar_state, *value_name).has_value();
}

}  // namespace c4c::backend::aarch64::codegen
