#pragma once

#include "calls.hpp"
#include "frame.hpp"
#include "names.hpp"
#include "regalloc.hpp"
#include "special_carriers.hpp"
#include "value_locations.hpp"

#include "../bir/bir.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

enum class PreparedF128RuntimeHelperFamily {
  Arithmetic,
  Comparison,
  Cast,
};

[[nodiscard]] constexpr std::string_view prepared_f128_runtime_helper_family_name(
    PreparedF128RuntimeHelperFamily family) {
  switch (family) {
    case PreparedF128RuntimeHelperFamily::Arithmetic:
      return "arithmetic";
    case PreparedF128RuntimeHelperFamily::Comparison:
      return "comparison";
    case PreparedF128RuntimeHelperFamily::Cast:
      return "cast";
  }
  return "unknown";
}

enum class PreparedF128RuntimeHelperKind {
  Add,
  Sub,
  Mul,
  Div,
  Eq,
  Ne,
  Lt,
  Le,
  Gt,
  Ge,
  F32ToF128,
  F64ToF128,
  F128ToF32,
  F128ToF64,
};

[[nodiscard]] constexpr std::string_view prepared_f128_runtime_helper_kind_name(
    PreparedF128RuntimeHelperKind kind) {
  switch (kind) {
    case PreparedF128RuntimeHelperKind::Add:
      return "add";
    case PreparedF128RuntimeHelperKind::Sub:
      return "sub";
    case PreparedF128RuntimeHelperKind::Mul:
      return "mul";
    case PreparedF128RuntimeHelperKind::Div:
      return "div";
    case PreparedF128RuntimeHelperKind::Eq:
      return "eq";
    case PreparedF128RuntimeHelperKind::Ne:
      return "ne";
    case PreparedF128RuntimeHelperKind::Lt:
      return "lt";
    case PreparedF128RuntimeHelperKind::Le:
      return "le";
    case PreparedF128RuntimeHelperKind::Gt:
      return "gt";
    case PreparedF128RuntimeHelperKind::Ge:
      return "ge";
    case PreparedF128RuntimeHelperKind::F32ToF128:
      return "f32_to_f128";
    case PreparedF128RuntimeHelperKind::F64ToF128:
      return "f64_to_f128";
    case PreparedF128RuntimeHelperKind::F128ToF32:
      return "f128_to_f32";
    case PreparedF128RuntimeHelperKind::F128ToF64:
      return "f128_to_f64";
  }
  return "unknown";
}

enum class PreparedF128RuntimeHelperResultOwnership {
  Missing,
  FullWidthCarrier,
  ScalarCmpResult,
  ScalarValue,
};

[[nodiscard]] constexpr std::string_view prepared_f128_runtime_helper_result_ownership_name(
    PreparedF128RuntimeHelperResultOwnership ownership) {
  switch (ownership) {
    case PreparedF128RuntimeHelperResultOwnership::Missing:
      return "missing";
    case PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier:
      return "full_width_carrier";
    case PreparedF128RuntimeHelperResultOwnership::ScalarCmpResult:
      return "scalar_cmp_result";
    case PreparedF128RuntimeHelperResultOwnership::ScalarValue:
      return "scalar_value";
  }
  return "unknown";
}

enum class PreparedF128RuntimeHelperAbiTransition {
  Missing,
  DirectF128ArgumentsAndResult,
  DirectF128ArgumentsAndCmpResult,
  DirectScalarArgumentAndF128Result,
  DirectF128ArgumentAndScalarResult,
};

[[nodiscard]] constexpr std::string_view prepared_f128_runtime_helper_abi_transition_name(
    PreparedF128RuntimeHelperAbiTransition transition) {
  switch (transition) {
    case PreparedF128RuntimeHelperAbiTransition::Missing:
      return "missing";
    case PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentsAndResult:
      return "direct_f128_arguments_and_result";
    case PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentsAndCmpResult:
      return "direct_f128_arguments_and_cmp_result";
    case PreparedF128RuntimeHelperAbiTransition::DirectScalarArgumentAndF128Result:
      return "direct_scalar_argument_and_f128_result";
    case PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentAndScalarResult:
      return "direct_f128_argument_and_scalar_result";
  }
  return "unknown";
}

enum class PreparedF128RuntimeHelperMarshalDirection {
  CarrierToAbiArgument,
  AbiResultToCarrier,
  AbiCmpResultToScalar,
  ScalarToAbiArgument,
  AbiResultToScalar,
};

[[nodiscard]] constexpr std::string_view prepared_f128_runtime_helper_marshal_direction_name(
    PreparedF128RuntimeHelperMarshalDirection direction) {
  switch (direction) {
    case PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument:
      return "carrier_to_abi_argument";
    case PreparedF128RuntimeHelperMarshalDirection::AbiResultToCarrier:
      return "abi_result_to_carrier";
    case PreparedF128RuntimeHelperMarshalDirection::AbiCmpResultToScalar:
      return "abi_cmp_result_to_scalar";
    case PreparedF128RuntimeHelperMarshalDirection::ScalarToAbiArgument:
      return "scalar_to_abi_argument";
    case PreparedF128RuntimeHelperMarshalDirection::AbiResultToScalar:
      return "abi_result_to_scalar";
  }
  return "unknown";
}

enum class PreparedF128CmpResultZeroTest {
  Missing,
  EqualZero,
  NotEqualZero,
  LessThanZero,
  LessOrEqualZero,
  GreaterThanZero,
  GreaterOrEqualZero,
};

[[nodiscard]] constexpr std::string_view prepared_f128_cmp_result_zero_test_name(
    PreparedF128CmpResultZeroTest test) {
  switch (test) {
    case PreparedF128CmpResultZeroTest::Missing:
      return "missing";
    case PreparedF128CmpResultZeroTest::EqualZero:
      return "eq_zero";
    case PreparedF128CmpResultZeroTest::NotEqualZero:
      return "ne_zero";
    case PreparedF128CmpResultZeroTest::LessThanZero:
      return "lt_zero";
    case PreparedF128CmpResultZeroTest::LessOrEqualZero:
      return "le_zero";
    case PreparedF128CmpResultZeroTest::GreaterThanZero:
      return "gt_zero";
    case PreparedF128CmpResultZeroTest::GreaterOrEqualZero:
      return "ge_zero";
  }
  return "unknown";
}

struct PreparedF128RuntimeHelper {
  // Source/result carrier facts as bound for this helper-call marshal plan.
  struct CarrierBinding {
    PreparedValueId value_id = 0;
    ValueNameId value_name = kInvalidValueName;
    PreparedF128CarrierKind carrier_kind = PreparedF128CarrierKind::Missing;
    std::size_t width_bytes = 16;
    std::size_t align_bytes = 16;
    PreparedRegisterBank register_bank = PreparedRegisterBank::None;
    PreparedRegisterClass register_class = PreparedRegisterClass::None;
    std::optional<std::string> register_name;
    std::optional<PreparedFrameSlotId> slot_id;
    std::optional<std::size_t> stack_offset_bytes;
  };

  struct AbiRegisterBinding {
    PreparedValueId value_id = 0;
    ValueNameId value_name = kInvalidValueName;
    std::optional<std::size_t> helper_argument_index;
    std::size_t abi_register_index = 0;
    std::size_t width_bytes = 16;
    PreparedRegisterBank register_bank = PreparedRegisterBank::None;
    PreparedRegisterClass register_class = PreparedRegisterClass::None;
    std::string register_name;
    std::size_t contiguous_width = 1;
    std::vector<std::string> occupied_register_names;
    std::optional<PreparedRegisterPlacement> register_placement;
  };

  struct MarshalingMove {
    PreparedF128RuntimeHelperMarshalDirection direction =
        PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument;
    CarrierBinding carrier;
    AbiRegisterBinding abi_register;
  };

  struct ScalarResultOwnership {
    PreparedValueId value_id = 0;
    ValueNameId value_name = kInvalidValueName;
    bir::TypeKind type = bir::TypeKind::I32;
    std::size_t width_bytes = 4;
    PreparedRegisterBank register_bank = PreparedRegisterBank::None;
    PreparedValueHomeKind home_kind = PreparedValueHomeKind::None;
    std::optional<std::string> register_name;
    std::optional<PreparedFrameSlotId> slot_id;
    std::optional<std::size_t> stack_offset_bytes;
  };

  struct ScalarMarshalingMove {
    PreparedF128RuntimeHelperMarshalDirection direction =
        PreparedF128RuntimeHelperMarshalDirection::AbiCmpResultToScalar;
    ScalarResultOwnership scalar_result;
    AbiRegisterBinding abi_register;
  };

  struct ScalarCmpResultConsumption {
    bir::TypeKind cmp_type = bir::TypeKind::I32;
    bir::TypeKind bir_result_type = bir::TypeKind::I1;
    PreparedF128CmpResultZeroTest zero_test = PreparedF128CmpResultZeroTest::Missing;
    bool consumes_helper_cmp_result = false;
    bool owns_bir_i1_result = false;
  };

  // Helper-call resource obligations, not top-level special-carrier facts.
  struct ResourcePolicy {
    bool call_boundary = false;
    bool runtime_helper_callee = false;
    bool caller_saved_clobbers = false;
    bool preserves_source_operation_identity = false;
  };

  struct AbiPolicy {
    PreparedF128RuntimeHelperAbiTransition transition =
        PreparedF128RuntimeHelperAbiTransition::Missing;
    PreparedRegisterBank argument_bank = PreparedRegisterBank::None;
    PreparedRegisterBank result_bank = PreparedRegisterBank::None;
    std::size_t argument_count = 0;
    std::size_t result_count = 0;
    std::size_t width_bytes = 16;
  };

  struct LivePreservationPolicy {
    bool evaluated = false;
    bool caller_saved_clobbers_modeled = false;
    bool no_additional_live_preservation_required = false;
    std::vector<PreparedCallPreservedValue> preserved_values;
  };

  // Completeness summary for the selected helper call and its ABI marshaling.
  struct SelectedCallOwnershipPolicy {
    bool owns_terminal_call = false;
    bool has_callee_identity = false;
    bool has_resource_policy = false;
    bool has_clobber_policy = false;
    bool has_abi_bindings = false;
    bool has_marshaling = false;
    bool has_live_preservation = false;
  };

  FunctionNameId function_name = kInvalidFunctionName;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  bir::BinaryOpcode source_binary_opcode = bir::BinaryOpcode::Add;
  std::optional<bir::CastOpcode> source_cast_opcode;
  bir::TypeKind source_type = bir::TypeKind::F128;
  bir::TypeKind result_type = bir::TypeKind::F128;
  PreparedValueId result_value_id = 0;
  ValueNameId result_value_name = kInvalidValueName;
  PreparedValueId operand_value_id = 0;
  ValueNameId operand_value_name = kInvalidValueName;
  PreparedValueId lhs_value_id = 0;
  ValueNameId lhs_value_name = kInvalidValueName;
  PreparedValueId rhs_value_id = 0;
  ValueNameId rhs_value_name = kInvalidValueName;
  PreparedF128RuntimeHelperFamily helper_family = PreparedF128RuntimeHelperFamily::Arithmetic;
  PreparedF128RuntimeHelperKind helper_kind = PreparedF128RuntimeHelperKind::Add;
  std::string callee_name;
  PreparedF128RuntimeHelperResultOwnership result_ownership =
      PreparedF128RuntimeHelperResultOwnership::Missing;
  std::optional<CarrierBinding> lhs_carrier;
  std::optional<CarrierBinding> rhs_carrier;
  std::optional<CarrierBinding> result_carrier;
  std::optional<ScalarResultOwnership> scalar_operand;
  std::optional<ScalarResultOwnership> scalar_result;
  std::optional<AbiRegisterBinding> scalar_operand_abi_argument;
  std::optional<AbiRegisterBinding> lhs_abi_argument;
  std::optional<AbiRegisterBinding> rhs_abi_argument;
  std::optional<AbiRegisterBinding> result_abi_result;
  std::optional<ScalarMarshalingMove> scalar_operand_argument_move;
  std::optional<MarshalingMove> lhs_argument_move;
  std::optional<MarshalingMove> rhs_argument_move;
  std::optional<MarshalingMove> result_unmarshal_move;
  std::optional<ScalarMarshalingMove> scalar_result_unmarshal_move;
  std::optional<ScalarCmpResultConsumption> scalar_cmp_result_consumption;
  ResourcePolicy resource_policy;
  AbiPolicy abi_policy;
  LivePreservationPolicy live_preservation_policy;
  SelectedCallOwnershipPolicy selected_call_ownership;
  std::vector<PreparedClobberedRegister> clobbered_registers;
  std::vector<std::string> missing_required_facts;
};

struct PreparedF128RuntimeHelperFunction {
  FunctionNameId function_name = kInvalidFunctionName;
  std::vector<PreparedF128RuntimeHelper> helpers;
  std::vector<std::string> missing_required_facts;
};

struct PreparedF128RuntimeHelpers {
  std::vector<PreparedF128RuntimeHelperFunction> functions;
};

enum class PreparedI128RuntimeHelperFamily {
  DivRem,
  FloatIntegerConversion,
};

[[nodiscard]] constexpr std::string_view prepared_i128_runtime_helper_family_name(
    PreparedI128RuntimeHelperFamily family) {
  switch (family) {
    case PreparedI128RuntimeHelperFamily::DivRem:
      return "div_rem";
    case PreparedI128RuntimeHelperFamily::FloatIntegerConversion:
      return "float_integer_conversion";
  }
  return "unknown";
}

enum class PreparedI128RuntimeHelperKind {
  SignedDiv,
  UnsignedDiv,
  SignedRem,
  UnsignedRem,
  FloatToSignedInt,
  FloatToUnsignedInt,
  SignedIntToFloat,
  UnsignedIntToFloat,
};

[[nodiscard]] constexpr std::string_view prepared_i128_runtime_helper_kind_name(
    PreparedI128RuntimeHelperKind kind) {
  switch (kind) {
    case PreparedI128RuntimeHelperKind::SignedDiv:
      return "signed_div";
    case PreparedI128RuntimeHelperKind::UnsignedDiv:
      return "unsigned_div";
    case PreparedI128RuntimeHelperKind::SignedRem:
      return "signed_rem";
    case PreparedI128RuntimeHelperKind::UnsignedRem:
      return "unsigned_rem";
    case PreparedI128RuntimeHelperKind::FloatToSignedInt:
      return "float_to_signed_int";
    case PreparedI128RuntimeHelperKind::FloatToUnsignedInt:
      return "float_to_unsigned_int";
    case PreparedI128RuntimeHelperKind::SignedIntToFloat:
      return "signed_int_to_float";
    case PreparedI128RuntimeHelperKind::UnsignedIntToFloat:
      return "unsigned_int_to_float";
  }
  return "unknown";
}

enum class PreparedI128RuntimeHelperResultOwnership {
  Missing,
  DirectLowHighLanes,
  ScalarValue,
  MemoryReturn,
};

[[nodiscard]] constexpr std::string_view prepared_i128_runtime_helper_result_ownership_name(
    PreparedI128RuntimeHelperResultOwnership ownership) {
  switch (ownership) {
    case PreparedI128RuntimeHelperResultOwnership::Missing:
      return "missing";
    case PreparedI128RuntimeHelperResultOwnership::DirectLowHighLanes:
      return "direct_low_high_lanes";
    case PreparedI128RuntimeHelperResultOwnership::ScalarValue:
      return "scalar_value";
    case PreparedI128RuntimeHelperResultOwnership::MemoryReturn:
      return "memory_return";
  }
  return "unknown";
}

enum class PreparedI128RuntimeHelperAbiTransition {
  Missing,
  DirectRegisterPairArgumentsAndResult,
  ScalarArgumentAndRegisterPairResult,
  RegisterPairArgumentAndScalarResult,
  MemoryReturn,
};

[[nodiscard]] constexpr std::string_view prepared_i128_runtime_helper_abi_transition_name(
    PreparedI128RuntimeHelperAbiTransition transition) {
  switch (transition) {
    case PreparedI128RuntimeHelperAbiTransition::Missing:
      return "missing";
    case PreparedI128RuntimeHelperAbiTransition::DirectRegisterPairArgumentsAndResult:
      return "direct_register_pair_arguments_and_result";
    case PreparedI128RuntimeHelperAbiTransition::ScalarArgumentAndRegisterPairResult:
      return "scalar_argument_and_register_pair_result";
    case PreparedI128RuntimeHelperAbiTransition::RegisterPairArgumentAndScalarResult:
      return "register_pair_argument_and_scalar_result";
    case PreparedI128RuntimeHelperAbiTransition::MemoryReturn:
      return "memory_return";
  }
  return "unknown";
}

enum class PreparedI128RuntimeHelperMarshalDirection {
  CarrierLaneToAbiArgument,
  AbiResultToCarrierLane,
  ScalarValueToAbiArgument,
  AbiResultToScalarValue,
};

[[nodiscard]] constexpr std::string_view prepared_i128_runtime_helper_marshal_direction_name(
    PreparedI128RuntimeHelperMarshalDirection direction) {
  switch (direction) {
    case PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument:
      return "carrier_lane_to_abi_argument";
    case PreparedI128RuntimeHelperMarshalDirection::AbiResultToCarrierLane:
      return "abi_result_to_carrier_lane";
    case PreparedI128RuntimeHelperMarshalDirection::ScalarValueToAbiArgument:
      return "scalar_value_to_abi_argument";
    case PreparedI128RuntimeHelperMarshalDirection::AbiResultToScalarValue:
      return "abi_result_to_scalar_value";
  }
  return "unknown";
}

struct PreparedI128RuntimeHelper {
  // Source/result carrier lanes as bound for this helper-call marshal plan.
  struct LaneBinding {
    PreparedValueId value_id = 0;
    ValueNameId value_name = kInvalidValueName;
    PreparedI128CarrierKind carrier_kind = PreparedI128CarrierKind::Missing;
    PreparedI128LaneRole role = PreparedI128LaneRole::Low;
    std::size_t lane_index = 0;
    std::size_t width_bytes = 8;
    std::optional<std::string> register_name;
    std::optional<PreparedFrameSlotId> slot_id;
    std::optional<std::size_t> stack_offset_bytes;
  };

  struct AbiRegisterBinding {
    PreparedValueId value_id = 0;
    ValueNameId value_name = kInvalidValueName;
    PreparedI128LaneRole role = PreparedI128LaneRole::Low;
    std::size_t lane_index = 0;
    std::size_t width_bytes = 8;
    std::optional<std::size_t> helper_argument_index;
    std::size_t abi_register_index = 0;
    PreparedRegisterBank register_bank = PreparedRegisterBank::None;
    PreparedRegisterClass register_class = PreparedRegisterClass::None;
    std::string register_name;
    std::size_t contiguous_width = 1;
    std::vector<std::string> occupied_register_names;
    std::optional<PreparedRegisterPlacement> register_placement;
  };

  struct ScalarValueOwnership {
    PreparedValueId value_id = 0;
    ValueNameId value_name = kInvalidValueName;
    bir::TypeKind type = bir::TypeKind::Void;
    std::size_t width_bytes = 0;
    PreparedRegisterBank register_bank = PreparedRegisterBank::None;
    PreparedValueHomeKind home_kind = PreparedValueHomeKind::None;
    std::optional<std::string> register_name;
    std::optional<PreparedFrameSlotId> slot_id;
    std::optional<std::size_t> stack_offset_bytes;
  };

  struct MarshalingMove {
    PreparedI128RuntimeHelperMarshalDirection direction =
        PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument;
    PreparedMovePhase phase = PreparedMovePhase::BeforeCall;
    PreparedMoveResolutionOpKind op_kind = PreparedMoveResolutionOpKind::Move;
    LaneBinding carrier_lane;
    AbiRegisterBinding abi_register;
  };

  struct ScalarMarshalingMove {
    PreparedI128RuntimeHelperMarshalDirection direction =
        PreparedI128RuntimeHelperMarshalDirection::ScalarValueToAbiArgument;
    PreparedMovePhase phase = PreparedMovePhase::BeforeCall;
    PreparedMoveResolutionOpKind op_kind = PreparedMoveResolutionOpKind::Move;
    ScalarValueOwnership scalar_value;
    AbiRegisterBinding abi_register;
  };

  struct LivePreservationPolicy {
    bool evaluated = false;
    bool caller_saved_clobbers_modeled = false;
    bool no_additional_live_preservation_required = false;
    std::vector<PreparedCallPreservedValue> preserved_values;
  };

  // Completeness summary for the selected helper call and its ABI marshaling.
  struct SelectedCallOwnershipPolicy {
    bool owns_terminal_call = false;
    bool has_callee_identity = false;
    bool has_resource_policy = false;
    bool has_clobber_policy = false;
    bool has_abi_bindings = false;
    bool has_marshaling = false;
    bool has_live_preservation = false;
  };

  // Helper-call resource obligations, not top-level special-carrier facts.
  struct ResourcePolicy {
    bool call_boundary = false;
    bool runtime_helper_callee = false;
    bool caller_saved_clobbers = false;
    bool preserves_source_operation_identity = false;
  };

  struct AbiPolicy {
    PreparedI128RuntimeHelperAbiTransition transition =
        PreparedI128RuntimeHelperAbiTransition::Missing;
    PreparedRegisterBank argument_bank = PreparedRegisterBank::None;
    PreparedRegisterBank result_bank = PreparedRegisterBank::None;
    std::size_t argument_count = 0;
    std::size_t lanes_per_argument = 0;
    std::size_t result_lane_count = 0;
    std::size_t lane_width_bytes = 0;
  };

  struct MemoryReturnOwnership {
    PreparedValueId destination_value_id = 0;
    ValueNameId destination_value_name = kInvalidValueName;
    std::size_t size_bytes = 16;
    std::size_t align_bytes = 16;
    std::optional<PreparedFrameSlotId> slot_id;
    std::optional<std::size_t> stack_offset_bytes;
  };

  FunctionNameId function_name = kInvalidFunctionName;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  bir::BinaryOpcode source_binary_opcode = bir::BinaryOpcode::Add;
  std::optional<bir::CastOpcode> source_cast_opcode;
  bir::TypeKind source_type = bir::TypeKind::I128;
  bir::TypeKind result_type = bir::TypeKind::I128;
  std::size_t source_width_bytes = 16;
  std::size_t result_width_bytes = 16;
  bool source_signed = false;
  bool result_signed = false;
  PreparedValueId result_value_id = 0;
  ValueNameId result_value_name = kInvalidValueName;
  PreparedValueId operand_value_id = 0;
  ValueNameId operand_value_name = kInvalidValueName;
  PreparedValueId lhs_value_id = 0;
  ValueNameId lhs_value_name = kInvalidValueName;
  PreparedValueId rhs_value_id = 0;
  ValueNameId rhs_value_name = kInvalidValueName;
  PreparedI128RuntimeHelperFamily helper_family = PreparedI128RuntimeHelperFamily::DivRem;
  PreparedI128RuntimeHelperKind helper_kind = PreparedI128RuntimeHelperKind::SignedDiv;
  std::string callee_name;
  PreparedI128RuntimeHelperResultOwnership result_ownership =
      PreparedI128RuntimeHelperResultOwnership::Missing;
  std::optional<LaneBinding> lhs_low_lane;
  std::optional<LaneBinding> lhs_high_lane;
  std::optional<LaneBinding> rhs_low_lane;
  std::optional<LaneBinding> rhs_high_lane;
  std::optional<LaneBinding> result_low_lane;
  std::optional<LaneBinding> result_high_lane;
  std::optional<AbiRegisterBinding> lhs_low_abi_argument;
  std::optional<AbiRegisterBinding> lhs_high_abi_argument;
  std::optional<AbiRegisterBinding> rhs_low_abi_argument;
  std::optional<AbiRegisterBinding> rhs_high_abi_argument;
  std::optional<AbiRegisterBinding> result_low_abi_result;
  std::optional<AbiRegisterBinding> result_high_abi_result;
  std::optional<AbiRegisterBinding> scalar_operand_abi_argument;
  std::optional<AbiRegisterBinding> scalar_result_abi_result;
  std::optional<MarshalingMove> lhs_low_argument_move;
  std::optional<MarshalingMove> lhs_high_argument_move;
  std::optional<MarshalingMove> rhs_low_argument_move;
  std::optional<MarshalingMove> rhs_high_argument_move;
  std::optional<MarshalingMove> result_low_unmarshal_move;
  std::optional<MarshalingMove> result_high_unmarshal_move;
  std::optional<ScalarMarshalingMove> scalar_operand_argument_move;
  std::optional<ScalarMarshalingMove> scalar_result_unmarshal_move;
  LivePreservationPolicy live_preservation_policy;
  SelectedCallOwnershipPolicy selected_call_ownership;
  std::optional<MemoryReturnOwnership> memory_return;
  std::optional<ScalarValueOwnership> scalar_operand;
  std::optional<ScalarValueOwnership> scalar_result;
  ResourcePolicy resource_policy;
  AbiPolicy abi_policy;
  std::vector<PreparedClobberedRegister> clobbered_registers;
  std::vector<std::string> missing_required_facts;
};

struct PreparedI128RuntimeHelperFunction {
  FunctionNameId function_name = kInvalidFunctionName;
  std::vector<PreparedI128RuntimeHelper> helpers;
  std::vector<std::string> missing_required_facts;
};

struct PreparedI128RuntimeHelpers {
  std::vector<PreparedI128RuntimeHelperFunction> functions;
};

}  // namespace c4c::backend::prepare
