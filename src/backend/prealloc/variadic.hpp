#pragma once

#include "names.hpp"
#include "value_locations.hpp"

#include "../bir/bir.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

struct PreparedVariadicEntryNamedRegisterCounts {
  std::optional<std::size_t> gp;
  std::optional<std::size_t> fp;
};

struct PreparedVariadicEntryRegisterSaveArea {
  bool required = false;
  std::optional<std::size_t> size_bytes;
  std::optional<std::size_t> align_bytes;
  std::optional<PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  std::optional<std::size_t> gp_offset_bytes;
  std::optional<std::size_t> fp_offset_bytes;
  std::optional<std::size_t> gp_slot_size_bytes;
  std::optional<std::size_t> fp_slot_size_bytes;
  std::optional<std::size_t> saved_gp_register_count;
  std::optional<std::size_t> saved_fp_register_count;
  std::optional<std::ptrdiff_t> initial_gp_offset_bytes;
  std::optional<std::ptrdiff_t> initial_fp_offset_bytes;
};

struct PreparedVariadicEntryOverflowArea {
  bool required = false;
  std::optional<PreparedFrameSlotId> base_slot_id;
  std::optional<std::size_t> base_stack_offset_bytes;
  std::optional<std::size_t> align_bytes;
};

struct PreparedRv64IncomingVariadicGprPublication {
  std::size_t abi_gpr_index = 0;
  std::size_t variadic_argument_index = 0;
  std::string source_register_name;
  PreparedFrameSlotId destination_slot_id = 0;
  std::size_t destination_stack_offset_bytes = 0;
  std::size_t destination_offset_bytes = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
};

enum class PreparedVariadicVaListFieldKind {
  GpOffset,
  FpOffset,
  OverflowArgArea,
  RegisterSaveArea,
  GpRegisterSaveArea,
  FpRegisterSaveArea,
};

[[nodiscard]] constexpr std::string_view prepared_variadic_va_list_field_kind_name(
    PreparedVariadicVaListFieldKind kind) {
  switch (kind) {
    case PreparedVariadicVaListFieldKind::GpOffset:
      return "gp_offset";
    case PreparedVariadicVaListFieldKind::FpOffset:
      return "fp_offset";
    case PreparedVariadicVaListFieldKind::OverflowArgArea:
      return "overflow_arg_area";
    case PreparedVariadicVaListFieldKind::RegisterSaveArea:
      return "register_save_area";
    case PreparedVariadicVaListFieldKind::GpRegisterSaveArea:
      return "gp_register_save_area";
    case PreparedVariadicVaListFieldKind::FpRegisterSaveArea:
      return "fp_register_save_area";
  }
  return "unknown";
}

struct PreparedVariadicVaListField {
  PreparedVariadicVaListFieldKind kind = PreparedVariadicVaListFieldKind::GpOffset;
  std::size_t offset_bytes = 0;
  std::size_t size_bytes = 0;
};

struct PreparedVariadicVaListLayout {
  bool required = false;
  std::optional<std::size_t> size_bytes;
  std::optional<std::size_t> align_bytes;
  std::vector<PreparedVariadicVaListField> fields;
};

enum class PreparedVariadicEntryHelperKind {
  VaStart,
  VaArg,
  VaArgAggregate,
  VaCopy,
};

[[nodiscard]] constexpr std::string_view prepared_variadic_entry_helper_kind_name(
    PreparedVariadicEntryHelperKind kind) {
  switch (kind) {
    case PreparedVariadicEntryHelperKind::VaStart:
      return "va_start";
    case PreparedVariadicEntryHelperKind::VaArg:
      return "va_arg";
    case PreparedVariadicEntryHelperKind::VaArgAggregate:
      return "va_arg_aggregate";
    case PreparedVariadicEntryHelperKind::VaCopy:
      return "va_copy";
  }
  return "unknown";
}

[[nodiscard]] inline std::optional<PreparedVariadicEntryHelperKind>
prepared_variadic_entry_helper_kind_for_callee(std::string_view callee) {
  if (callee == "llvm.va_start.p0") {
    return PreparedVariadicEntryHelperKind::VaStart;
  }
  if (callee == "llvm.va_copy.p0.p0") {
    return PreparedVariadicEntryHelperKind::VaCopy;
  }
  constexpr std::string_view va_arg_prefix = "llvm.va_arg.";
  if (callee.substr(0, va_arg_prefix.size()) == va_arg_prefix) {
    if (callee == "llvm.va_arg.aggregate") {
      return PreparedVariadicEntryHelperKind::VaArgAggregate;
    }
    return PreparedVariadicEntryHelperKind::VaArg;
  }
  return std::nullopt;
}

[[nodiscard]] inline bool call_has_runtime_placeholder_shape(const bir::CallInst& call) {
  return !call.is_indirect &&
         call.callee_link_name_id == kInvalidLinkName &&
         !call.callee_value.has_value();
}

[[nodiscard]] inline bool call_is_runtime_intrinsic_placeholder(
    const bir::CallInst& call) {
  if (!call_has_runtime_placeholder_shape(call)) {
    return false;
  }
  if (call.intrinsic.has_value() || call.inline_asm.has_value()) {
    return true;
  }
  constexpr std::string_view llvm_prefix = "llvm.";
  return call.callee.substr(0, llvm_prefix.size()) == llvm_prefix;
}

[[nodiscard]] inline std::optional<PreparedVariadicEntryHelperKind>
prepared_variadic_entry_helper_kind_for_call(const bir::CallInst& call) {
  if (!call_is_runtime_intrinsic_placeholder(call)) {
    return std::nullopt;
  }
  return prepared_variadic_entry_helper_kind_for_callee(call.callee);
}

struct PreparedVariadicEntryHelperResources {
  std::vector<PreparedVariadicEntryHelperKind> required_helpers;
  // Maximum simultaneous reserved MIR scratch resources required by any
  // recognized helper in this entry plan.
  std::optional<std::size_t> scratch_register_count;
  std::optional<std::size_t> scratch_stack_bytes;
};

enum class PreparedVariadicScalarVaArgSourceClass {
  Unknown,
  GpRegisterSaveArea,
  FpRegisterSaveArea,
  OverflowArgArea,
};

[[nodiscard]] constexpr std::string_view
prepared_variadic_scalar_va_arg_source_class_name(
    PreparedVariadicScalarVaArgSourceClass source_class) {
  switch (source_class) {
    case PreparedVariadicScalarVaArgSourceClass::Unknown:
      return "unknown";
    case PreparedVariadicScalarVaArgSourceClass::GpRegisterSaveArea:
      return "gp_register_save_area";
    case PreparedVariadicScalarVaArgSourceClass::FpRegisterSaveArea:
      return "fp_register_save_area";
    case PreparedVariadicScalarVaArgSourceClass::OverflowArgArea:
      return "overflow_arg_area";
  }
  return "unknown";
}

struct PreparedVariadicScalarVaArgAccessPlan {
  PreparedVariadicScalarVaArgSourceClass source_class =
      PreparedVariadicScalarVaArgSourceClass::Unknown;
  bir::TypeKind value_type = bir::TypeKind::Void;
  std::size_t value_size_bytes = 0;
  std::size_t value_align_bytes = 0;
  std::optional<PreparedValueHome> result_home;
  std::optional<PreparedVariadicVaListFieldKind> source_field;
  std::optional<std::size_t> source_field_offset_bytes;
  std::optional<std::size_t> source_slot_size_bytes;
  std::optional<PreparedVariadicVaListFieldKind> progression_field;
  std::optional<std::size_t> progression_field_offset_bytes;
  std::optional<std::size_t> progression_stride_bytes;
  std::optional<PreparedVariadicVaListFieldKind> overflow_source_field;
  std::optional<std::size_t> overflow_source_field_offset_bytes;
  std::optional<std::size_t> overflow_stride_bytes;
};

[[nodiscard]] inline bool is_complete_prepared_variadic_scalar_va_arg_access_plan(
    const PreparedVariadicScalarVaArgAccessPlan& plan) {
  return plan.source_class != PreparedVariadicScalarVaArgSourceClass::Unknown &&
         plan.value_type != bir::TypeKind::Void &&
         plan.value_size_bytes > 0 &&
         plan.value_align_bytes > 0 &&
         plan.result_home.has_value() &&
         plan.source_field.has_value() &&
         plan.source_field_offset_bytes.has_value() &&
         plan.source_slot_size_bytes.has_value() &&
         plan.progression_field.has_value() &&
         plan.progression_field_offset_bytes.has_value() &&
         plan.progression_stride_bytes.has_value() &&
         plan.overflow_source_field.has_value() &&
         plan.overflow_source_field_offset_bytes.has_value() &&
         plan.overflow_stride_bytes.has_value();
}

enum class PreparedVariadicAggregateVaArgSourceClass {
  Unknown,
  RegisterSaveArea,
  OverflowArgArea,
  ExplicitPayload,
};

[[nodiscard]] constexpr std::string_view
prepared_variadic_aggregate_va_arg_source_class_name(
    PreparedVariadicAggregateVaArgSourceClass source_class) {
  switch (source_class) {
    case PreparedVariadicAggregateVaArgSourceClass::Unknown:
      return "unknown";
    case PreparedVariadicAggregateVaArgSourceClass::RegisterSaveArea:
      return "register_save_area";
    case PreparedVariadicAggregateVaArgSourceClass::OverflowArgArea:
      return "overflow_arg_area";
    case PreparedVariadicAggregateVaArgSourceClass::ExplicitPayload:
      return "explicit_payload";
  }
  return "unknown";
}

struct PreparedVariadicAggregatePayloadWriteAddress {
  ValueNameId result_value_name = kInvalidValueName;
  BlockLabelId materialization_block_label = kInvalidBlockLabel;
  std::size_t materialization_instruction_index = 0;
  PreparedFrameSlotId frame_slot_id = 0;
  std::size_t stack_offset_bytes = 0;
};

struct PreparedVariadicAggregateVaArgAccessPlan {
  PreparedVariadicAggregateVaArgSourceClass source_class =
      PreparedVariadicAggregateVaArgSourceClass::Unknown;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  std::size_t payload_size_bytes = 0;
  std::size_t payload_align_bytes = 0;
  std::optional<PreparedValueHome> destination_payload_home;
  std::optional<PreparedVariadicAggregatePayloadWriteAddress> payload_write_address;
  std::optional<PreparedVariadicVaListFieldKind> source_field;
  std::optional<std::size_t> source_field_offset_bytes;
  std::optional<std::size_t> source_payload_offset_bytes;
  std::optional<std::size_t> source_slot_size_bytes;
  std::optional<std::size_t> copy_size_bytes;
  std::optional<std::size_t> copy_align_bytes;
  std::optional<PreparedVariadicVaListFieldKind> progression_field;
  std::optional<std::size_t> progression_field_offset_bytes;
  std::optional<std::size_t> progression_stride_bytes;
  std::optional<std::size_t> overflow_source_field_offset_bytes;
  std::optional<std::size_t> overflow_stride_bytes;
  std::optional<std::size_t> register_save_lane_count;
  std::optional<std::size_t> register_save_lane_size_bytes;
  std::vector<PreparedValueHome> register_save_lane_destination_homes;
};

[[nodiscard]] inline bool
is_complete_prepared_variadic_aggregate_va_arg_access_plan(
    const PreparedVariadicAggregateVaArgAccessPlan& plan) {
  return plan.source_class != PreparedVariadicAggregateVaArgSourceClass::Unknown &&
         plan.payload_size_bytes > 0 &&
         plan.payload_align_bytes > 0 &&
         plan.destination_payload_home.has_value() &&
         plan.source_field.has_value() &&
         plan.source_field_offset_bytes.has_value() &&
         plan.source_payload_offset_bytes.has_value() &&
         plan.source_slot_size_bytes.has_value() &&
         plan.copy_size_bytes.has_value() &&
         *plan.copy_size_bytes > 0 &&
         plan.copy_align_bytes.has_value() &&
         *plan.copy_align_bytes > 0 &&
         plan.progression_field.has_value() &&
         plan.progression_field_offset_bytes.has_value() &&
         plan.progression_stride_bytes.has_value() &&
         *plan.progression_stride_bytes > 0;
}

struct PreparedVariadicVaStartOperandHomes {
  PreparedValueHome destination_va_list;
  PreparedValueHome destination_va_list_address;
};

struct PreparedVariadicScalarVaArgOperandHomes {
  PreparedValueHome source_va_list;
  PreparedValueHome scalar_result;
  PreparedVariadicScalarVaArgAccessPlan scalar_access_plan;
};

struct PreparedVariadicAggregateVaArgOperandHomes {
  PreparedValueHome source_va_list;
  PreparedValueHome aggregate_destination_payload;
  PreparedVariadicAggregateVaArgAccessPlan aggregate_access_plan;
};

struct PreparedVariadicEntryHelperOperandHomes {
  PreparedVariadicEntryHelperKind helper = PreparedVariadicEntryHelperKind::VaStart;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  mutable std::optional<PreparedVariadicVaStartOperandHomes> va_start_operand_homes;
  mutable std::optional<PreparedVariadicScalarVaArgOperandHomes>
      scalar_va_arg_operand_homes;
  mutable std::optional<PreparedVariadicAggregateVaArgOperandHomes>
      aggregate_va_arg_operand_homes;
  std::optional<PreparedValueHome> destination_va_list;
  std::optional<PreparedValueHome> destination_va_list_address;
  std::optional<PreparedValueHome> source_va_list;
  std::optional<PreparedValueHome> scalar_result;
  std::optional<PreparedValueHome> aggregate_destination_payload;
  std::optional<PreparedVariadicScalarVaArgAccessPlan> scalar_access_plan;
  std::optional<PreparedVariadicAggregateVaArgAccessPlan> aggregate_access_plan;
};

[[nodiscard]] inline const PreparedVariadicVaStartOperandHomes*
find_prepared_variadic_va_start_operand_homes(
    const PreparedVariadicEntryHelperOperandHomes& homes) {
  if (homes.helper != PreparedVariadicEntryHelperKind::VaStart) {
    return nullptr;
  }
  if (!homes.va_start_operand_homes.has_value() &&
      homes.destination_va_list.has_value() &&
      homes.destination_va_list_address.has_value()) {
    homes.va_start_operand_homes = PreparedVariadicVaStartOperandHomes{
        .destination_va_list = *homes.destination_va_list,
        .destination_va_list_address = *homes.destination_va_list_address,
    };
  }
  if (!homes.va_start_operand_homes.has_value()) {
    return nullptr;
  }
  return &*homes.va_start_operand_homes;
}

inline void publish_prepared_variadic_va_start_operand_homes(
    PreparedVariadicEntryHelperOperandHomes& homes) {
  if (homes.helper != PreparedVariadicEntryHelperKind::VaStart ||
      !homes.destination_va_list.has_value() ||
      !homes.destination_va_list_address.has_value()) {
    homes.va_start_operand_homes = std::nullopt;
    return;
  }
  homes.va_start_operand_homes = PreparedVariadicVaStartOperandHomes{
      .destination_va_list = *homes.destination_va_list,
      .destination_va_list_address = *homes.destination_va_list_address,
  };
}

[[nodiscard]] inline const PreparedVariadicScalarVaArgOperandHomes*
find_prepared_variadic_scalar_va_arg_operand_homes(
    const PreparedVariadicEntryHelperOperandHomes& homes) {
  if (homes.helper != PreparedVariadicEntryHelperKind::VaArg) {
    return nullptr;
  }
  if (!homes.scalar_va_arg_operand_homes.has_value() &&
      homes.source_va_list.has_value() &&
      homes.scalar_result.has_value() &&
      homes.scalar_access_plan.has_value() &&
      is_complete_prepared_variadic_scalar_va_arg_access_plan(
          *homes.scalar_access_plan)) {
    homes.scalar_va_arg_operand_homes = PreparedVariadicScalarVaArgOperandHomes{
        .source_va_list = *homes.source_va_list,
        .scalar_result = *homes.scalar_result,
        .scalar_access_plan = *homes.scalar_access_plan,
    };
  }
  if (!homes.scalar_va_arg_operand_homes.has_value()) {
    return nullptr;
  }
  return &*homes.scalar_va_arg_operand_homes;
}

inline void publish_prepared_variadic_scalar_va_arg_operand_homes(
    PreparedVariadicEntryHelperOperandHomes& homes) {
  if (homes.helper != PreparedVariadicEntryHelperKind::VaArg ||
      !homes.source_va_list.has_value() ||
      !homes.scalar_result.has_value() ||
      !homes.scalar_access_plan.has_value() ||
      !is_complete_prepared_variadic_scalar_va_arg_access_plan(
          *homes.scalar_access_plan)) {
    homes.scalar_va_arg_operand_homes = std::nullopt;
    return;
  }
  homes.scalar_va_arg_operand_homes = PreparedVariadicScalarVaArgOperandHomes{
      .source_va_list = *homes.source_va_list,
      .scalar_result = *homes.scalar_result,
      .scalar_access_plan = *homes.scalar_access_plan,
  };
}

[[nodiscard]] inline const PreparedVariadicAggregateVaArgOperandHomes*
find_prepared_variadic_aggregate_va_arg_operand_homes(
    const PreparedVariadicEntryHelperOperandHomes& homes) {
  if (homes.helper != PreparedVariadicEntryHelperKind::VaArgAggregate) {
    return nullptr;
  }
  if (!homes.aggregate_va_arg_operand_homes.has_value() &&
      homes.source_va_list.has_value() &&
      homes.aggregate_destination_payload.has_value() &&
      homes.aggregate_access_plan.has_value() &&
      is_complete_prepared_variadic_aggregate_va_arg_access_plan(
          *homes.aggregate_access_plan)) {
    homes.aggregate_va_arg_operand_homes =
        PreparedVariadicAggregateVaArgOperandHomes{
            .source_va_list = *homes.source_va_list,
            .aggregate_destination_payload =
                *homes.aggregate_destination_payload,
            .aggregate_access_plan = *homes.aggregate_access_plan,
        };
  }
  if (!homes.aggregate_va_arg_operand_homes.has_value()) {
    return nullptr;
  }
  return &*homes.aggregate_va_arg_operand_homes;
}

inline void publish_prepared_variadic_aggregate_va_arg_operand_homes(
    PreparedVariadicEntryHelperOperandHomes& homes) {
  if (homes.helper != PreparedVariadicEntryHelperKind::VaArgAggregate ||
      !homes.source_va_list.has_value() ||
      !homes.aggregate_destination_payload.has_value() ||
      !homes.aggregate_access_plan.has_value() ||
      !is_complete_prepared_variadic_aggregate_va_arg_access_plan(
          *homes.aggregate_access_plan)) {
    homes.aggregate_va_arg_operand_homes = std::nullopt;
    return;
  }
  homes.aggregate_va_arg_operand_homes =
      PreparedVariadicAggregateVaArgOperandHomes{
          .source_va_list = *homes.source_va_list,
          .aggregate_destination_payload =
              *homes.aggregate_destination_payload,
          .aggregate_access_plan = *homes.aggregate_access_plan,
      };
}

[[nodiscard]] inline bool has_complete_prepared_variadic_va_start_operand_homes(
    const PreparedVariadicEntryHelperOperandHomes& homes) {
  return find_prepared_variadic_va_start_operand_homes(homes) != nullptr;
}

[[nodiscard]] inline bool has_complete_prepared_variadic_scalar_va_arg_access_plan(
    const PreparedVariadicEntryHelperOperandHomes& homes) {
  return find_prepared_variadic_scalar_va_arg_operand_homes(homes) != nullptr;
}

[[nodiscard]] inline bool has_complete_prepared_variadic_aggregate_va_arg_access_plan(
    const PreparedVariadicEntryHelperOperandHomes& homes) {
  return find_prepared_variadic_aggregate_va_arg_operand_homes(homes) !=
         nullptr;
}

[[nodiscard]] inline bool has_complete_prepared_variadic_va_copy_operand_homes(
    const PreparedVariadicEntryHelperOperandHomes& homes) {
  return homes.helper == PreparedVariadicEntryHelperKind::VaCopy &&
         homes.destination_va_list.has_value() &&
         homes.source_va_list.has_value();
}

[[nodiscard]] inline bool has_complete_prepared_variadic_entry_helper_operand_homes(
    const PreparedVariadicEntryHelperOperandHomes& homes) {
  switch (homes.helper) {
    case PreparedVariadicEntryHelperKind::VaStart:
      return has_complete_prepared_variadic_va_start_operand_homes(homes);
    case PreparedVariadicEntryHelperKind::VaArg:
      return has_complete_prepared_variadic_scalar_va_arg_access_plan(homes);
    case PreparedVariadicEntryHelperKind::VaArgAggregate:
      return has_complete_prepared_variadic_aggregate_va_arg_access_plan(homes);
    case PreparedVariadicEntryHelperKind::VaCopy:
      return has_complete_prepared_variadic_va_copy_operand_homes(homes);
  }
  return false;
}

struct PreparedVariadicEntryPlanFunction {
  FunctionNameId function_name = kInvalidFunctionName;
  std::size_t named_parameter_count = 0;
  PreparedVariadicEntryNamedRegisterCounts named_register_counts;
  PreparedVariadicEntryRegisterSaveArea register_save_area;
  PreparedVariadicEntryOverflowArea overflow_area;
  PreparedVariadicVaListLayout va_list_layout;
  PreparedVariadicEntryHelperResources helper_resources;
  std::vector<PreparedVariadicEntryHelperOperandHomes> helper_operand_homes;
  std::vector<PreparedRv64IncomingVariadicGprPublication>
      rv64_incoming_variadic_gpr_publications;
  std::vector<std::string> missing_required_facts;
};

struct PreparedVariadicEntryPlans {
  std::vector<PreparedVariadicEntryPlanFunction> functions;
};

}  // namespace c4c::backend::prepare
