#include "prepared_value_home_materialization.hpp"

#include "../abi/abi.hpp"
#include "dispatch_publication_common.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace abi = c4c::backend::aarch64::abi;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

namespace {

[[nodiscard]] bool emit_prepared_scalar_publication_plan_to_register(
    const prepare::PreparedStackLayout* stack_layout,
    const prepare::PreparedScalarPublicationPlan& plan,
    std::uint8_t target_index,
    std::vector<std::string>& lines,
    bool use_frame_pointer_base);

}  // namespace

[[nodiscard]] bool emit_prepared_value_home_to_register(
    const prepare::PreparedStackLayout* stack_layout,
    const prepare::PreparedValueHome& home,
    bir::TypeKind type,
    std::uint8_t target_index,
    std::vector<std::string>& lines,
    bool use_frame_pointer_base) {
  const auto target_view = scalar_view_for_type(type);
  if (!target_view.has_value()) {
    return false;
  }
  const auto target = gp_register_name(target_index, *target_view);
  if (!target.has_value()) {
    return false;
  }
  if (home.kind == prepare::PreparedValueHomeKind::StackSlot &&
      home.offset_bytes.has_value()) {
    const auto type_width = dispatch_publication_scalar_type_size_bytes(type);
    if (!type_width.has_value()) {
      return false;
    }
    std::optional<std::size_t> home_width = home.size_bytes;
    if (stack_layout != nullptr && home.slot_id.has_value()) {
      if (const auto* slot = find_frame_slot(*stack_layout, *home.slot_id); slot != nullptr) {
        home_width = home_width.has_value()
                         ? std::min(*home_width, slot->size_bytes)
                         : std::optional<std::size_t>{slot->size_bytes};
      }
    }
    const auto load_width =
        home_width.has_value()
            ? std::min(*home_width, *type_width)
            : *type_width;
    const auto mnemonic = scalar_load_mnemonic_for_width(load_width);
    const auto load_view = load_width == 8 ? abi::RegisterView::X
                                           : abi::RegisterView::W;
    const auto load_target = gp_register_name(target_index, load_view);
    if (!mnemonic.has_value() || !load_target.has_value()) {
      return false;
    }
    lines.push_back(std::string{*mnemonic} + " " + *load_target + ", " +
                    frame_slot_address(*home.offset_bytes,
                                       use_frame_pointer_base ? "x29" : "sp"));
    return true;
  }
  if (home.kind == prepare::PreparedValueHomeKind::Register &&
      home.register_name.has_value()) {
    const auto parsed = abi::parse_aarch64_register_name(*home.register_name);
    if (!parsed.has_value() || parsed->bank != abi::RegisterBank::GeneralPurpose) {
      return false;
    }
    const auto source = abi::gp_register(parsed->index, *target_view);
    if (!source.has_value()) {
      return false;
    }
    const auto source_name = std::string{abi::register_name(*source)};
    if (source_name != *target) {
      lines.push_back("mov " + *target + ", " + source_name);
    }
    return true;
  }
  return false;
}

namespace {

[[nodiscard]] bool emit_prepared_scalar_publication_plan_to_register(
    const prepare::PreparedStackLayout* stack_layout,
    const prepare::PreparedScalarPublicationPlan& plan,
    std::uint8_t target_index,
    std::vector<std::string>& lines,
    bool use_frame_pointer_base) {
  if (!prepare::prepared_scalar_publication_available(plan) ||
      plan.destination_home == nullptr) {
    return false;
  }

  switch (plan.hook_kind) {
    case prepare::PreparedScalarPublicationHookKind::RegisterHome:
      if (plan.storage_encoding != prepare::PreparedStorageEncodingKind::Register) {
        return false;
      }
      return emit_prepared_value_home_to_register(stack_layout,
                                                  *plan.destination_home,
                                                  plan.source_value.type,
                                                  target_index,
                                                  lines,
                                                  use_frame_pointer_base);
    case prepare::PreparedScalarPublicationHookKind::StackSlotHome:
      if (plan.storage_encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
          !plan.stack_offset_bytes.has_value()) {
        return false;
      }
      return emit_prepared_value_home_to_register(stack_layout,
                                                  *plan.destination_home,
                                                  plan.source_value.type,
                                                  target_index,
                                                  lines,
                                                  use_frame_pointer_base);
    case prepare::PreparedScalarPublicationHookKind::RematerializableImmediate:
    case prepare::PreparedScalarPublicationHookKind::PointerBasePlusOffset:
    case prepare::PreparedScalarPublicationHookKind::None:
      return false;
  }
  return false;
}

[[nodiscard]] bool emit_prepared_scalar_publication_plan_to_register(
    const module::BlockLoweringContext& context,
    const prepare::PreparedScalarPublicationPlan& plan,
    std::uint8_t target_index,
    std::vector<std::string>& lines) {
  return emit_prepared_scalar_publication_plan_to_register(
      context.function.prepared != nullptr ? &context.function.prepared->stack_layout
                                           : nullptr,
      plan,
      target_index,
      lines,
      fixed_slots_use_frame_pointer(context.function));
}

[[nodiscard]] bool emit_prepared_home_publication_plan_to_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const prepare::PreparedValueHome& home,
    std::uint8_t target_index,
    std::vector<std::string>& lines) {
  const auto plan = prepare::plan_prepared_scalar_publication(
      prepare::PreparedScalarPublicationInputs{
          .source_value = &value,
          .destination_home = &home,
      });
  return emit_prepared_scalar_publication_plan_to_register(context,
                                                          plan,
                                                          target_index,
                                                          lines);
}

}  // namespace

[[nodiscard]] bool emit_prepared_value_home_publication_to_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const prepare::PreparedValueHome& home,
    std::uint8_t target_index,
    std::vector<std::string>& lines) {
  return emit_prepared_home_publication_plan_to_register(context,
                                                        value,
                                                        home,
                                                        target_index,
                                                        lines);
}

}  // namespace c4c::backend::aarch64::codegen
