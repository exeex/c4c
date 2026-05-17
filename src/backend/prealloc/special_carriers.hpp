#pragma once

#include "frame.hpp"
#include "names.hpp"
#include "value_locations.hpp"

#include "../bir/bir.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

struct PreparedBirModule;

void populate_i128_carriers(PreparedBirModule& prepared);
void populate_f128_carriers(PreparedBirModule& prepared);

enum class PreparedI128CarrierKind {
  Missing,
  RegisterPair,
  MemoryBacked,
};

[[nodiscard]] constexpr std::string_view prepared_i128_carrier_kind_name(
    PreparedI128CarrierKind kind) {
  switch (kind) {
    case PreparedI128CarrierKind::Missing:
      return "missing";
    case PreparedI128CarrierKind::RegisterPair:
      return "register_pair";
    case PreparedI128CarrierKind::MemoryBacked:
      return "memory_backed";
  }
  return "unknown";
}

enum class PreparedI128LaneRole {
  Low,
  High,
};

[[nodiscard]] constexpr std::string_view prepared_i128_lane_role_name(
    PreparedI128LaneRole role) {
  switch (role) {
    case PreparedI128LaneRole::Low:
      return "low";
    case PreparedI128LaneRole::High:
      return "high";
  }
  return "unknown";
}

struct PreparedI128LaneCarrier {
  PreparedI128LaneRole role = PreparedI128LaneRole::Low;
  std::size_t lane_index = 0;
  std::size_t width_bytes = 8;
  std::optional<std::string> register_name;
  std::optional<PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
};

struct PreparedI128Carrier {
  FunctionNameId function_name = kInvalidFunctionName;
  PreparedValueId value_id = 0;
  ValueNameId value_name = kInvalidValueName;
  c4c::backend::bir::TypeKind source_type = c4c::backend::bir::TypeKind::I128;
  PreparedI128CarrierKind kind = PreparedI128CarrierKind::Missing;
  std::size_t lane_width_bytes = 8;
  std::size_t total_size_bytes = 16;
  std::size_t total_align_bytes = 16;
  PreparedRegisterBank register_bank = PreparedRegisterBank::None;
  PreparedRegisterClass register_class = PreparedRegisterClass::None;
  std::size_t contiguous_width = 1;
  std::vector<std::string> occupied_register_names;
  std::optional<PreparedRegisterPlacement> register_placement;
  std::optional<PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  PreparedI128LaneCarrier low_lane{.role = PreparedI128LaneRole::Low, .lane_index = 0};
  PreparedI128LaneCarrier high_lane{.role = PreparedI128LaneRole::High, .lane_index = 1};
  std::vector<std::string> missing_required_facts;
};

struct PreparedI128CarrierFunction {
  FunctionNameId function_name = kInvalidFunctionName;
  std::vector<PreparedI128Carrier> carriers;
  std::vector<std::string> missing_required_facts;
};

struct PreparedI128Carriers {
  std::vector<PreparedI128CarrierFunction> functions;
};

enum class PreparedF128CarrierKind {
  Missing,
  FullWidthRegister,
  MemoryBacked,
};

[[nodiscard]] constexpr std::string_view prepared_f128_carrier_kind_name(
    PreparedF128CarrierKind kind) {
  switch (kind) {
    case PreparedF128CarrierKind::Missing:
      return "missing";
    case PreparedF128CarrierKind::FullWidthRegister:
      return "full_width_register";
    case PreparedF128CarrierKind::MemoryBacked:
      return "memory_backed";
  }
  return "unknown";
}

struct PreparedF128Carrier {
  FunctionNameId function_name = kInvalidFunctionName;
  PreparedValueId value_id = 0;
  ValueNameId value_name = kInvalidValueName;
  c4c::backend::bir::TypeKind source_type = c4c::backend::bir::TypeKind::F128;
  PreparedF128CarrierKind kind = PreparedF128CarrierKind::Missing;
  std::size_t total_size_bytes = 16;
  std::size_t total_align_bytes = 16;
  PreparedRegisterBank register_bank = PreparedRegisterBank::None;
  PreparedRegisterClass register_class = PreparedRegisterClass::None;
  std::size_t contiguous_width = 1;
  std::optional<std::string> register_name;
  std::vector<std::string> occupied_register_names;
  std::optional<PreparedRegisterPlacement> register_placement;
  std::optional<PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  std::optional<c4c::backend::bir::Value::F128Payload> constant_payload;
  std::vector<std::string> missing_required_facts;
};

struct PreparedF128CarrierFunction {
  FunctionNameId function_name = kInvalidFunctionName;
  std::vector<PreparedF128Carrier> carriers;
  std::vector<std::string> missing_required_facts;
};

struct PreparedF128Carriers {
  std::vector<PreparedF128CarrierFunction> functions;
};

enum class PreparedAtomicOperationCarrierKind {
  Missing,
  Complete,
};

[[nodiscard]] constexpr std::string_view prepared_atomic_operation_carrier_kind_name(
    PreparedAtomicOperationCarrierKind kind) {
  switch (kind) {
    case PreparedAtomicOperationCarrierKind::Missing:
      return "missing";
    case PreparedAtomicOperationCarrierKind::Complete:
      return "complete";
  }
  return "unknown";
}

struct PreparedAtomicOperationCarrier {
  FunctionNameId function_name = kInvalidFunctionName;
  PreparedAtomicOperationCarrierKind carrier_kind =
      PreparedAtomicOperationCarrierKind::Missing;
  c4c::backend::bir::AtomicOperationKind operation_kind =
      c4c::backend::bir::AtomicOperationKind::None;
  BlockLabelId block_label = kInvalidBlockLabel;
  std::size_t inst_index = 0;
  c4c::backend::bir::TypeKind value_type = c4c::backend::bir::TypeKind::Void;
  std::size_t width_bytes = 0;
  std::optional<c4c::backend::bir::Value> result;
  std::optional<c4c::backend::bir::Value> pointer;
  std::optional<c4c::backend::bir::Value> value;
  std::optional<c4c::backend::bir::Value> expected;
  std::optional<c4c::backend::bir::Value> desired;
  std::optional<ValueNameId> result_value_name;
  std::optional<ValueNameId> pointer_value_name;
  std::optional<ValueNameId> value_name;
  std::optional<ValueNameId> expected_value_name;
  std::optional<ValueNameId> desired_value_name;
  c4c::backend::bir::AtomicOrdering ordering =
      c4c::backend::bir::AtomicOrdering::None;
  c4c::backend::bir::AtomicOrdering failure_ordering =
      c4c::backend::bir::AtomicOrdering::None;
  c4c::backend::bir::AtomicRmwOpcode rmw_opcode =
      c4c::backend::bir::AtomicRmwOpcode::None;
  c4c::backend::bir::AtomicResultMode result_mode =
      c4c::backend::bir::AtomicResultMode::None;
  c4c::backend::bir::AddressSpace address_space =
      c4c::backend::bir::AddressSpace::Default;
  std::vector<std::string> missing_required_facts;
};

struct PreparedAtomicOperationFunction {
  FunctionNameId function_name = kInvalidFunctionName;
  std::vector<PreparedAtomicOperationCarrier> operations;
  std::vector<std::string> missing_required_facts;
};

struct PreparedAtomicOperations {
  std::vector<PreparedAtomicOperationFunction> functions;
};

enum class PreparedIntrinsicCarrierKind {
  Missing,
  Complete,
};

[[nodiscard]] constexpr std::string_view prepared_intrinsic_carrier_kind_name(
    PreparedIntrinsicCarrierKind kind) {
  switch (kind) {
    case PreparedIntrinsicCarrierKind::Missing:
      return "missing";
    case PreparedIntrinsicCarrierKind::Complete:
      return "complete";
  }
  return "unknown";
}

struct PreparedIntrinsicCarrier {
  FunctionNameId function_name = kInvalidFunctionName;
  PreparedIntrinsicCarrierKind carrier_kind = PreparedIntrinsicCarrierKind::Missing;
  c4c::backend::bir::IntrinsicFamilyKind family =
      c4c::backend::bir::IntrinsicFamilyKind::None;
  c4c::backend::bir::IntrinsicOperationKind operation =
      c4c::backend::bir::IntrinsicOperationKind::None;
  c4c::backend::bir::IntrinsicFeatureKind required_feature =
      c4c::backend::bir::IntrinsicFeatureKind::None;
  std::size_t block_index = 0;
  std::size_t inst_index = 0;
  c4c::backend::bir::TypeKind operand_type = c4c::backend::bir::TypeKind::Void;
  c4c::backend::bir::TypeKind result_type = c4c::backend::bir::TypeKind::Void;
  std::vector<c4c::backend::bir::IntrinsicOperandRole> operand_roles;
  c4c::backend::bir::TypeKind vector_element_type =
      c4c::backend::bir::TypeKind::Void;
  std::size_t vector_element_width_bytes = 0;
  std::size_t vector_lane_count = 0;
  std::size_t vector_total_width_bytes = 0;
  c4c::backend::bir::IntrinsicSignedness signedness =
      c4c::backend::bir::IntrinsicSignedness::None;
  std::optional<c4c::backend::bir::MemoryAddress> memory_operand;
  c4c::backend::bir::IntrinsicMemoryAccessKind memory_access =
      c4c::backend::bir::IntrinsicMemoryAccessKind::None;
  c4c::backend::bir::IntrinsicBarrierDomainKind barrier_domain =
      c4c::backend::bir::IntrinsicBarrierDomainKind::None;
  bool has_immediate_operand = false;
  bool requires_immediate_operand = false;
  std::optional<std::int64_t> immediate_value;
  std::optional<c4c::backend::bir::Value> operand;
  std::vector<c4c::backend::bir::Value> operands;
  std::optional<c4c::backend::bir::Value> result;
  std::optional<ValueNameId> operand_value_name;
  std::vector<ValueNameId> operand_value_names;
  std::optional<ValueNameId> result_value_name;
  std::vector<std::optional<PreparedValueHome>> operand_homes;
  std::optional<PreparedValueHome> result_home;
  bool has_side_effects = false;
  bool requires_feature = false;
  std::optional<std::string> source_callee_name;
  bool has_prepared_call_plan = false;
  std::vector<std::string> missing_required_facts;
};

struct PreparedIntrinsicCarrierFunction {
  FunctionNameId function_name = kInvalidFunctionName;
  std::vector<PreparedIntrinsicCarrier> carriers;
  std::vector<std::string> missing_required_facts;
};

struct PreparedIntrinsicCarriers {
  std::vector<PreparedIntrinsicCarrierFunction> functions;
};

enum class PreparedInlineAsmCarrierKind {
  Missing,
  Complete,
};

[[nodiscard]] constexpr std::string_view prepared_inline_asm_carrier_kind_name(
    PreparedInlineAsmCarrierKind kind) {
  switch (kind) {
    case PreparedInlineAsmCarrierKind::Missing:
      return "missing";
    case PreparedInlineAsmCarrierKind::Complete:
      return "complete";
  }
  return "unknown";
}

struct PreparedInlineAsmTiedHomeAuthority {
  std::size_t tied_output_index = 0;
  PreparedTargetRegisterIdentity shared_register;
};

struct PreparedInlineAsmOperand {
  c4c::backend::bir::InlineAsmOperandKind kind =
      c4c::backend::bir::InlineAsmOperandKind::Unsupported;
  std::size_t constraint_index = 0;
  std::string constraint;
  std::optional<std::size_t> arg_index;
  std::optional<std::size_t> output_index;
  std::optional<std::size_t> tied_output_index;
  std::optional<c4c::backend::bir::Value> value;
  std::optional<ValueNameId> value_name;
  std::optional<PreparedValueHome> home;
  std::optional<PreparedInlineAsmTiedHomeAuthority> tied_home_authority;
  std::optional<std::int64_t> immediate_value;
  std::optional<std::string> name;
  std::optional<c4c::backend::bir::MemoryAddress> memory_address;
  std::optional<c4c::backend::bir::MemoryAddress> address;
};

struct PreparedInlineAsmCarrier {
  FunctionNameId function_name = kInvalidFunctionName;
  PreparedInlineAsmCarrierKind carrier_kind = PreparedInlineAsmCarrierKind::Missing;
  std::size_t block_index = 0;
  std::size_t inst_index = 0;
  std::string asm_text;
  std::string constraints;
  bool side_effects = false;
  bool has_named_operand_references = false;
  bool has_template_modifiers = false;
  std::vector<PreparedInlineAsmOperand> operands;
  std::vector<std::string> clobbers;
  std::optional<c4c::backend::bir::Value> result;
  std::optional<ValueNameId> result_value_name;
  std::optional<PreparedValueHome> result_home;
  std::vector<std::string> missing_required_facts;
};

struct PreparedInlineAsmCarrierFunction {
  FunctionNameId function_name = kInvalidFunctionName;
  std::vector<PreparedInlineAsmCarrier> carriers;
  std::vector<std::string> missing_required_facts;
};

struct PreparedInlineAsmCarriers {
  std::vector<PreparedInlineAsmCarrierFunction> functions;
};

}  // namespace c4c::backend::prepare
