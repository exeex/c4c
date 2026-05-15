#include "machine_printer.hpp"

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace {

mir::TargetInstructionPrintResult target_unsupported(std::string diagnostic) {
  return mir::target_instruction_unsupported(std::move(diagnostic));
}

mir::TargetInstructionPrintResult target_printed(std::vector<std::string> lines) {
  return mir::target_instruction_lines_printed(std::move(lines));
}

std::string block_label(c4c::FunctionNameId function_name, c4c::BlockLabelId block_label) {
  return ".LBB" + std::to_string(function_name) + "_" + std::to_string(block_label);
}

std::string register_name(const RegisterOperand& operand) {
  return abi::register_name(operand.reg);
}

std::optional<unsigned> integer_type_bit_width(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
      return 1U;
    case bir::TypeKind::I8:
      return 8U;
    case bir::TypeKind::I16:
      return 16U;
    case bir::TypeKind::I32:
      return 32U;
    case bir::TypeKind::I64:
      return 64U;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<std::string> register_name_with_view(const RegisterOperand& operand,
                                                   abi::RegisterView view) {
  if (!abi::is_gp_register(operand.reg)) {
    return std::nullopt;
  }
  const auto viewed = abi::gp_register(operand.reg.index, view);
  if (!viewed.has_value()) {
    return std::nullopt;
  }
  return abi::register_name(*viewed);
}

std::optional<std::string> fp_register_name_with_view(const RegisterOperand& operand,
                                                      abi::RegisterView view) {
  if (!abi::is_fp_simd_register(operand.reg)) {
    return std::nullopt;
  }
  const auto viewed = abi::fp_simd_register(operand.reg.index, view);
  if (!viewed.has_value()) {
    return std::nullopt;
  }
  return abi::register_name(*viewed);
}

std::optional<std::string> f128_q_register_name(const RegisterOperand& operand) {
  if (operand.expected_view != abi::RegisterView::Q ||
      operand.prepared_bank != prepare::PreparedRegisterBank::Vreg ||
      operand.prepared_class != prepare::PreparedRegisterClass::Vector ||
      operand.contiguous_width != 1) {
    return std::nullopt;
  }
  return fp_register_name_with_view(operand, abi::RegisterView::Q);
}

std::optional<std::string> f128_vector_register_name(const RegisterOperand& operand) {
  if (operand.expected_view != abi::RegisterView::Q ||
      operand.prepared_bank != prepare::PreparedRegisterBank::Vreg ||
      operand.prepared_class != prepare::PreparedRegisterClass::Vector ||
      operand.contiguous_width != 1) {
    return std::nullopt;
  }
  auto name = fp_register_name_with_view(operand, abi::RegisterView::V);
  if (!name.has_value()) {
    return std::nullopt;
  }
  *name += ".16b";
  return name;
}

bool has_complete_intrinsic_source(const prepare::PreparedIntrinsicCarrier* source) {
  return source != nullptr &&
         source->carrier_kind == prepare::PreparedIntrinsicCarrierKind::Complete;
}

bool has_exact_intrinsic_roles(const std::vector<bir::IntrinsicOperandRole>& roles,
                               std::initializer_list<bir::IntrinsicOperandRole> expected) {
  return roles.size() == expected.size() &&
         std::equal(roles.begin(), roles.end(), expected.begin(), expected.end());
}

bool has_v16i8_unsigned_shape(bir::TypeKind element_type,
                              std::size_t element_width_bytes,
                              std::size_t lane_count,
                              std::size_t total_width_bytes,
                              bir::IntrinsicSignedness signedness) {
  return element_type == bir::TypeKind::I8 && element_width_bytes == 1 &&
         lane_count == 16 && total_width_bytes == 16 &&
         signedness == bir::IntrinsicSignedness::Unsigned;
}

std::optional<std::string> f128_scalar_fp_register_name(
    const RegisterOperand& operand,
    std::size_t width_bytes) {
  if (operand.prepared_bank != prepare::PreparedRegisterBank::Fpr ||
      operand.prepared_class != prepare::PreparedRegisterClass::Float ||
      operand.contiguous_width != 1) {
    return std::nullopt;
  }
  if (width_bytes == 4) {
    if (operand.expected_view != abi::RegisterView::S) {
      return std::nullopt;
    }
    return fp_register_name_with_view(operand, abi::RegisterView::S);
  }
  if (width_bytes == 8) {
    if (operand.expected_view != abi::RegisterView::D) {
      return std::nullopt;
    }
    return fp_register_name_with_view(operand, abi::RegisterView::D);
  }
  return std::nullopt;
}

std::optional<abi::RegisterView> integer_register_view(unsigned bit_width) {
  if (bit_width <= 32U) {
    return abi::RegisterView::W;
  }
  if (bit_width == 64U) {
    return abi::RegisterView::X;
  }
  return std::nullopt;
}

std::optional<abi::RegisterView> floating_register_view(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::F32:
      return abi::RegisterView::S;
    case bir::TypeKind::F64:
      return abi::RegisterView::D;
    case bir::TypeKind::Void:
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::I128:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

std::string_view floating_alu_mnemonic(ScalarAluOperationKind operation) {
  switch (operation) {
    case ScalarAluOperationKind::Add:
      return "fadd";
    case ScalarAluOperationKind::Sub:
      return "fsub";
    case ScalarAluOperationKind::Mul:
      return "fmul";
    case ScalarAluOperationKind::Div:
      return "fdiv";
    case ScalarAluOperationKind::And:
    case ScalarAluOperationKind::Or:
    case ScalarAluOperationKind::Xor:
    case ScalarAluOperationKind::Deferred:
      return {};
  }
  return {};
}

std::string immediate_name(const ImmediateOperand& operand) {
  return "#" + std::to_string(operand.signed_value);
}

bool decimal_digits_only(std::string_view text) {
  if (text.empty()) {
    return false;
  }
  for (const char ch : text) {
    if (ch < '0' || ch > '9') {
      return false;
    }
  }
  return true;
}

std::optional<std::size_t> parse_decimal_index(std::string_view text) {
  if (!decimal_digits_only(text)) {
    return std::nullopt;
  }
  std::size_t value = 0;
  for (const char ch : text) {
    const auto digit = static_cast<std::size_t>(ch - '0');
    if (value > (static_cast<std::size_t>(-1) - digit) / 10U) {
      return std::nullopt;
    }
    value = value * 10U + digit;
  }
  return value;
}

std::string prepared_value_home_name(const prepare::PreparedValueHome& home) {
  std::ostringstream out;
  out << "value#" << home.value_id << ":"
      << prepare::prepared_value_home_kind_name(home.kind);
  if (home.register_name.has_value()) {
    out << ":" << *home.register_name;
  }
  if (home.slot_id.has_value()) {
    out << ":slot#" << *home.slot_id;
  }
  if (home.offset_bytes.has_value()) {
    out << ":offset+" << *home.offset_bytes;
  }
  return out.str();
}

bool complete_preserved_value_route(
    const prepare::PreparedCallPreservedValue& preserved) {
  switch (preserved.route) {
    case prepare::PreparedCallPreservationRoute::Unknown:
      return false;
    case prepare::PreparedCallPreservationRoute::CalleeSavedRegister:
      return preserved.register_name.has_value() &&
             preserved.register_bank.has_value() &&
             !preserved.occupied_register_names.empty() &&
             preserved.register_placement.has_value() &&
             preserved.callee_saved_save_index.has_value();
    case prepare::PreparedCallPreservationRoute::StackSlot:
      return preserved.slot_id.has_value() &&
             preserved.stack_offset_bytes.has_value() &&
             preserved.stack_size_bytes.has_value() &&
             *preserved.stack_size_bytes > 0 &&
             preserved.stack_align_bytes.has_value() &&
             *preserved.stack_align_bytes > 0 &&
             preserved.spill_slot_placement.has_value();
  }
  return false;
}

bool has_complete_live_preservation(
    const prepare::PreparedI128RuntimeHelper::LivePreservationPolicy& policy) {
  if (!policy.evaluated || !policy.caller_saved_clobbers_modeled ||
      !policy.no_additional_live_preservation_required) {
    return false;
  }
  for (const auto& preserved : policy.preserved_values) {
    if (!complete_preserved_value_route(preserved)) {
      return false;
    }
  }
  return true;
}

bool has_complete_selected_call_ownership(
    const prepare::PreparedI128RuntimeHelper::SelectedCallOwnershipPolicy& ownership) {
  return ownership.owns_terminal_call &&
         ownership.has_callee_identity &&
         ownership.has_resource_policy &&
         ownership.has_clobber_policy &&
         ownership.has_abi_bindings &&
         ownership.has_marshaling &&
         ownership.has_live_preservation;
}

bool is_decimal_digit(char ch) {
  return ch >= '0' && ch <= '9';
}

bool is_printable_x_register_name(std::string_view name) {
  if (name.size() < 2 || name.front() != 'x') {
    return false;
  }
  unsigned index = 0;
  for (std::size_t pos = 1; pos < name.size(); ++pos) {
    if (!is_decimal_digit(name[pos])) {
      return false;
    }
    index = index * 10U + static_cast<unsigned>(name[pos] - '0');
  }
  return index <= 30U;
}

std::optional<std::string> validate_i128_helper_move(
    const prepare::PreparedI128RuntimeHelper::MarshalingMove& move,
    prepare::PreparedI128RuntimeHelperMarshalDirection direction,
    prepare::PreparedMovePhase phase) {
  if (move.direction != direction || move.phase != phase ||
      move.op_kind != prepare::PreparedMoveResolutionOpKind::Move) {
    return std::string{"i128 helper boundary has unsupported marshal/unmarshal move shape"};
  }
  if (move.carrier_lane.width_bytes != 8 ||
      !move.carrier_lane.register_name.has_value() ||
      !is_printable_x_register_name(*move.carrier_lane.register_name)) {
    return std::string{"i128 helper boundary requires register-backed carrier lane moves"};
  }
  if (move.abi_register.width_bytes != 8 ||
      move.abi_register.register_bank != prepare::PreparedRegisterBank::Gpr ||
      move.abi_register.register_class != prepare::PreparedRegisterClass::General ||
      !is_printable_x_register_name(move.abi_register.register_name)) {
    return std::string{"i128 helper boundary requires GPR ABI register bindings"};
  }
  if (move.carrier_lane.value_id != move.abi_register.value_id ||
      move.carrier_lane.value_name != move.abi_register.value_name ||
      move.carrier_lane.role != move.abi_register.role ||
      move.carrier_lane.lane_index != move.abi_register.lane_index ||
      move.carrier_lane.width_bytes != move.abi_register.width_bytes) {
    return std::string{"i128 helper boundary marshal/unmarshal lane facts mismatch"};
  }
  return std::nullopt;
}

std::optional<std::string> append_i128_helper_move_line(
    std::vector<std::string>& lines,
    const std::optional<prepare::PreparedI128RuntimeHelper::MarshalingMove>& move,
    prepare::PreparedI128RuntimeHelperMarshalDirection direction,
    prepare::PreparedMovePhase phase) {
  if (!move.has_value()) {
    return std::string{"i128 helper boundary is missing structured marshal/unmarshal moves"};
  }
  if (const auto invalid = validate_i128_helper_move(*move, direction, phase);
      invalid.has_value()) {
    return invalid;
  }

  std::ostringstream line;
  if (direction ==
      prepare::PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument) {
    line << "mov " << move->abi_register.register_name << ", "
         << *move->carrier_lane.register_name;
  } else {
    line << "mov " << *move->carrier_lane.register_name << ", "
         << move->abi_register.register_name;
  }
  lines.push_back(line.str());
  return std::nullopt;
}

std::optional<std::string> append_f128_helper_move_line(
    std::vector<std::string>& lines,
    const F128RuntimeHelperOperandRecord& operand,
    prepare::PreparedF128RuntimeHelperMarshalDirection direction) {
  if (operand.marshaling_move.direction != direction ||
      operand.carrier_kind != prepare::PreparedF128CarrierKind::FullWidthRegister ||
      operand.width_bytes != 16 ||
      operand.align_bytes != 16 ||
      operand.register_bank != prepare::PreparedRegisterBank::Vreg ||
      operand.register_class != prepare::PreparedRegisterClass::Vector ||
      !operand.carrier_register.has_value() ||
      !operand.abi_register.has_value()) {
    return std::string{
        "f128 helper boundary requires structured full-width q-register moves"};
  }
  const auto carrier = f128_vector_register_name(*operand.carrier_register);
  const auto abi = f128_vector_register_name(*operand.abi_register);
  if (!carrier.has_value() || !abi.has_value()) {
    return std::string{
        "f128 helper boundary has incomplete printable q-register facts"};
  }

  std::ostringstream line;
  if (direction ==
      prepare::PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument) {
    line << "mov " << *abi << ", " << *carrier;
  } else if (direction ==
             prepare::PreparedF128RuntimeHelperMarshalDirection::AbiResultToCarrier) {
    line << "mov " << *carrier << ", " << *abi;
  } else {
    return std::string{"f128 helper boundary has unsupported full-width move direction"};
  }
  lines.push_back(line.str());
  return std::nullopt;
}

std::optional<std::string> append_f128_scalar_move_line(
    std::vector<std::string>& lines,
    const F128RuntimeHelperScalarResultRecord& scalar,
    prepare::PreparedF128RuntimeHelperMarshalDirection direction) {
  if (!scalar.marshaling_move.has_value() ||
      scalar.marshaling_move->direction != direction ||
      !scalar.materialized_i1_register.has_value() ||
      !scalar.abi_register.has_value()) {
    return std::string{
        "f128 helper boundary requires structured scalar marshal/unmarshal moves"};
  }

  const auto materialized =
      f128_scalar_fp_register_name(*scalar.materialized_i1_register, scalar.width_bytes);
  const auto abi = f128_scalar_fp_register_name(*scalar.abi_register, scalar.width_bytes);
  if (!materialized.has_value() || !abi.has_value()) {
    return std::string{
        "f128 helper boundary has incomplete printable scalar FP register facts"};
  }

  std::ostringstream line;
  if (direction == prepare::PreparedF128RuntimeHelperMarshalDirection::ScalarToAbiArgument) {
    line << "fmov " << *abi << ", " << *materialized;
  } else if (direction ==
             prepare::PreparedF128RuntimeHelperMarshalDirection::AbiResultToScalar) {
    line << "fmov " << *materialized << ", " << *abi;
  } else {
    return std::string{"f128 helper boundary has unsupported scalar FP move direction"};
  }
  lines.push_back(line.str());
  return std::nullopt;
}

std::optional<std::string> validate_f128_cmp_scalar_result(
    const F128RuntimeHelperScalarResultRecord& scalar) {
  if (!scalar.marshaling_move.has_value() ||
      scalar.marshaling_move->direction !=
          prepare::PreparedF128RuntimeHelperMarshalDirection::AbiCmpResultToScalar ||
      !scalar.abi_register.has_value() ||
      !scalar.materialized_i1_register.has_value()) {
    return std::string{
        "f128 comparison helper requires structured scalar cmp-result marshal facts"};
  }
  if (scalar.type != bir::TypeKind::I32 ||
      scalar.width_bytes != 4 ||
      scalar.register_bank != prepare::PreparedRegisterBank::Gpr ||
      scalar.scalar_ownership.type != bir::TypeKind::I32 ||
      scalar.scalar_ownership.width_bytes != 4 ||
      scalar.scalar_ownership.register_bank != prepare::PreparedRegisterBank::Gpr ||
      scalar.marshaling_move->scalar_result.type != bir::TypeKind::I32 ||
      scalar.marshaling_move->scalar_result.width_bytes != 4 ||
      scalar.marshaling_move->scalar_result.register_bank !=
          prepare::PreparedRegisterBank::Gpr ||
      scalar.marshaling_move->abi_register.register_bank !=
          prepare::PreparedRegisterBank::Gpr ||
      scalar.marshaling_move->abi_register.width_bytes != 4) {
    return std::string{
        "f128 comparison helper requires structured i32 GPR cmp-result facts"};
  }
  if (!register_name_with_view(*scalar.abi_register, abi::RegisterView::W).has_value() ||
      !register_name_with_view(*scalar.materialized_i1_register,
                               abi::RegisterView::W)
           .has_value()) {
    return std::string{
        "f128 comparison helper has incomplete printable scalar GPR register facts"};
  }
  return std::nullopt;
}

std::optional<std::string> f128_cmp_condition(
    prepare::PreparedF128CmpResultZeroTest zero_test) {
  switch (zero_test) {
    case prepare::PreparedF128CmpResultZeroTest::EqualZero:
      return std::string{"eq"};
    case prepare::PreparedF128CmpResultZeroTest::NotEqualZero:
      return std::string{"ne"};
    case prepare::PreparedF128CmpResultZeroTest::LessThanZero:
      return std::string{"lt"};
    case prepare::PreparedF128CmpResultZeroTest::LessOrEqualZero:
      return std::string{"le"};
    case prepare::PreparedF128CmpResultZeroTest::GreaterThanZero:
      return std::string{"gt"};
    case prepare::PreparedF128CmpResultZeroTest::GreaterOrEqualZero:
      return std::string{"ge"};
    case prepare::PreparedF128CmpResultZeroTest::Missing:
      return std::nullopt;
  }
  return std::nullopt;
}

bool is_plain_add_sub_immediate(const ImmediateOperand& operand) {
  return operand.kind == ImmediateKind::SignedInteger && operand.signed_value >= 0 &&
         operand.signed_value <= 4095;
}

std::string memory_address(const MemoryOperand& address) {
  std::ostringstream out;
  if (address.base_kind == MemoryBaseKind::FrameSlot) {
    out << "[sp";
  } else if (address.base_kind == MemoryBaseKind::PointerValue && address.base_register.has_value()) {
    out << "[" << register_name(*address.base_register);
  } else {
    return {};
  }
  if (address.byte_offset != 0) {
    out << ", #" << address.byte_offset;
  }
  out << "]";
  return out.str();
}

std::optional<std::string> lane_register_name(const I128LaneTransportRecord& lane) {
  if (!lane.reg.has_value()) {
    return std::nullopt;
  }
  return register_name(*lane.reg);
}

std::string atomic_loop_label(const AtomicMemoryInstructionRecord& atomic,
                              std::string_view suffix) {
  std::ostringstream out;
  out << ".Latomic_" << atomic.function_name << "_" << atomic.block_label << "_"
      << atomic.instruction_index << "_" << suffix;
  return out.str();
}

std::optional<std::string> atomic_pointer_register_name(
    const AtomicMemoryInstructionRecord& atomic) {
  if (!atomic.pointer_register.has_value()) {
    return std::nullopt;
  }
  return register_name_with_view(*atomic.pointer_register, abi::RegisterView::X);
}

std::optional<std::string> atomic_value_register_name(
    const RegisterOperand& operand,
    std::size_t width_bytes) {
  if (width_bytes == 0 || width_bytes > 8) {
    return std::nullopt;
  }
  return register_name_with_view(operand,
                                 width_bytes == 8 ? abi::RegisterView::X
                                                  : abi::RegisterView::W);
}

std::string_view atomic_plain_load_mnemonic(std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return "ldrb";
    case 2:
      return "ldrh";
    case 4:
    case 8:
      return "ldr";
  }
  return {};
}

std::string_view atomic_acquire_load_mnemonic(std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return "ldarb";
    case 2:
      return "ldarh";
    case 4:
    case 8:
      return "ldar";
  }
  return {};
}

std::string_view atomic_plain_store_mnemonic(std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return "strb";
    case 2:
      return "strh";
    case 4:
    case 8:
      return "str";
  }
  return {};
}

std::string_view atomic_release_store_mnemonic(std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return "stlrb";
    case 2:
      return "stlrh";
    case 4:
    case 8:
      return "stlr";
  }
  return {};
}

std::string_view atomic_exclusive_load_mnemonic(std::size_t width_bytes, bool acquire) {
  switch (width_bytes) {
    case 1:
      return acquire ? "ldaxrb" : "ldxrb";
    case 2:
      return acquire ? "ldaxrh" : "ldxrh";
    case 4:
    case 8:
      return acquire ? "ldaxr" : "ldxr";
  }
  return {};
}

std::string_view atomic_exclusive_store_mnemonic(std::size_t width_bytes, bool release) {
  switch (width_bytes) {
    case 1:
      return release ? "stlxrb" : "stxrb";
    case 2:
      return release ? "stlxrh" : "stxrh";
    case 4:
    case 8:
      return release ? "stlxr" : "stxr";
  }
  return {};
}

std::string_view atomic_rmw_operation_mnemonic(bir::AtomicRmwOpcode opcode) {
  switch (opcode) {
    case bir::AtomicRmwOpcode::Exchange:
      return "mov";
    case bir::AtomicRmwOpcode::Add:
      return "add";
    case bir::AtomicRmwOpcode::Sub:
      return "sub";
    case bir::AtomicRmwOpcode::And:
      return "and";
    case bir::AtomicRmwOpcode::Or:
      return "orr";
    case bir::AtomicRmwOpcode::Xor:
      return "eor";
    case bir::AtomicRmwOpcode::None:
      return {};
  }
  return {};
}

bool atomic_ordering_has_acquire(bir::AtomicOrdering ordering) {
  return ordering == bir::AtomicOrdering::Acquire ||
         ordering == bir::AtomicOrdering::AcqRel ||
         ordering == bir::AtomicOrdering::SeqCst;
}

std::optional<std::string> pair_low_register_name(const I128PairOperandRecord& operand) {
  return lane_register_name(operand.low_lane);
}

std::optional<std::string> pair_high_register_name(const I128PairOperandRecord& operand) {
  return lane_register_name(operand.high_lane);
}

std::string relocation_operand(std::string_view label, std::int64_t byte_offset) {
  std::string operand(label);
  if (byte_offset > 0) {
    operand += "+";
    operand += std::to_string(byte_offset);
  } else if (byte_offset < 0) {
    operand += std::to_string(byte_offset);
  }
  return operand;
}

std::string prefixed_relocation_operand(std::string_view prefix, std::string_view label) {
  std::string operand(prefix);
  operand += label;
  return operand;
}

std::string tls_relocation_prefix(prepare::PreparedTlsRelocationKind kind) {
  switch (kind) {
    case prepare::PreparedTlsRelocationKind::Aarch64TprelHi12:
      return ":tprel_hi12:";
    case prepare::PreparedTlsRelocationKind::Aarch64TprelLo12Nc:
      return ":tprel_lo12_nc:";
    case prepare::PreparedTlsRelocationKind::None:
      return {};
  }
  return {};
}

struct InlineAsmSubstitutionResult {
  std::optional<std::vector<std::string>> lines;
  std::string diagnostic;
};

const InlineAsmMachineOperandRecord* find_inline_asm_constraint_operand(
    const AssemblerInstructionRecord& assembler,
    std::size_t constraint_index) {
  for (const auto& operand : assembler.inline_asm_operands) {
    if (operand.constraint_index == constraint_index) {
      return &operand;
    }
  }
  return nullptr;
}

const InlineAsmMachineOperandRecord* find_inline_asm_output_operand(
    const AssemblerInstructionRecord& assembler,
    std::size_t output_index) {
  for (const auto& operand : assembler.inline_asm_operands) {
    if (operand.kind == bir::InlineAsmOperandKind::RegisterOutput &&
        operand.output_index.has_value() && *operand.output_index == output_index) {
      return &operand;
    }
  }
  return nullptr;
}

bool inline_asm_home_is_concrete_register(
    const prepare::PreparedValueHome& home) {
  return home.kind == prepare::PreparedValueHomeKind::Register &&
         home.register_name.has_value();
}

bool inline_asm_tied_homes_agree(
    const prepare::PreparedValueHome& tied_home,
    const prepare::PreparedValueHome& output_home) {
  return inline_asm_home_is_concrete_register(tied_home) &&
         inline_asm_home_is_concrete_register(output_home) &&
         *tied_home.register_name == *output_home.register_name;
}

struct InlineAsmNamedOperandLookup {
  const InlineAsmMachineOperandRecord* operand = nullptr;
  std::string diagnostic;
};

InlineAsmNamedOperandLookup find_inline_asm_named_operand(
    const AssemblerInstructionRecord& assembler,
    std::string_view name) {
  if (name.empty()) {
    return {.diagnostic = "inline-asm template has missing named operand name"};
  }

  const InlineAsmMachineOperandRecord* found = nullptr;
  std::size_t matching_names = 0;
  for (const auto& operand : assembler.inline_asm_operands) {
    if (!operand.name.has_value() || operand.name->empty()) {
      continue;
    }
    if (std::string_view{*operand.name} == name) {
      found = &operand;
      ++matching_names;
    }
  }

  if (matching_names > 1) {
    return {.diagnostic = "inline-asm template references duplicate named operand"};
  }
  if (found == nullptr) {
    return {.diagnostic = "inline-asm template references unknown named operand"};
  }
  return {.operand = found};
}

bool inline_asm_constraint_matches_kind(const InlineAsmMachineOperandRecord& operand) {
  switch (operand.kind) {
    case bir::InlineAsmOperandKind::RegisterInput:
      return operand.constraint == "r";
    case bir::InlineAsmOperandKind::RegisterOutput:
      return operand.constraint == "=r";
    case bir::InlineAsmOperandKind::TiedInput:
      return decimal_digits_only(operand.constraint);
    case bir::InlineAsmOperandKind::IntegerImmediateInput:
      return operand.constraint == "i" || operand.constraint == "I";
    case bir::InlineAsmOperandKind::MemoryInput:
      return operand.constraint == "m";
    case bir::InlineAsmOperandKind::AddressInput:
      return operand.constraint == "p";
    case bir::InlineAsmOperandKind::Clobber:
    case bir::InlineAsmOperandKind::Unsupported:
      return false;
  }
  return false;
}

std::optional<std::string> inline_asm_register_operand_text(
    const RegisterOperand& reg,
    std::optional<char> modifier) {
  if (!modifier.has_value()) {
    return register_name(reg);
  }
  switch (*modifier) {
    case 'w':
      return register_name_with_view(reg, abi::RegisterView::W);
    case 'x':
      return register_name_with_view(reg, abi::RegisterView::X);
    default:
      return std::nullopt;
  }
}

std::optional<std::string> inline_asm_operand_text(
    const AssemblerInstructionRecord& assembler,
    const InlineAsmMachineOperandRecord& operand,
    std::optional<char> modifier,
    std::string* diagnostic) {
  if (operand.kind == bir::InlineAsmOperandKind::Clobber) {
    *diagnostic = "inline-asm clobber operand requires structured clobber authority";
    return std::nullopt;
  }
  if (operand.kind == bir::InlineAsmOperandKind::Unsupported) {
    *diagnostic = "inline-asm operand kind is unsupported for selected printer";
    return std::nullopt;
  }
  if (!inline_asm_constraint_matches_kind(operand)) {
    *diagnostic = "inline-asm operand has unsupported constraint for selected printer";
    return std::nullopt;
  }

  const InlineAsmMachineOperandRecord* printable_operand = &operand;
  if (operand.kind == bir::InlineAsmOperandKind::TiedInput) {
    if (!operand.tied_output_index.has_value()) {
      *diagnostic = "inline-asm tied input is missing tied output index";
      return std::nullopt;
    }
    printable_operand = find_inline_asm_output_operand(assembler, *operand.tied_output_index);
    if (printable_operand == nullptr || !printable_operand->selected_operand.has_value()) {
      *diagnostic = "inline-asm tied input is missing selected tied output operand";
      return std::nullopt;
    }
    if (!operand.selected_operand.has_value()) {
      *diagnostic = "inline-asm tied input is missing selected operand";
      return std::nullopt;
    }
    if (!operand.home.has_value() || !printable_operand->home.has_value()) {
      *diagnostic = "inline-asm tied input is missing prepared tied home";
      return std::nullopt;
    }
    if (!inline_asm_home_is_concrete_register(*operand.home) ||
        !inline_asm_home_is_concrete_register(*printable_operand->home)) {
      *diagnostic = "inline-asm tied input requires concrete prepared register homes";
      return std::nullopt;
    }
    if (!inline_asm_tied_homes_agree(*operand.home, *printable_operand->home)) {
      *diagnostic = "inline-asm tied input prepared home disagrees with output";
      return std::nullopt;
    }
  }

  if (!printable_operand->selected_operand.has_value()) {
    *diagnostic = "inline-asm operand is missing selected operand";
    return std::nullopt;
  }
  const auto& selected = *printable_operand->selected_operand;
  if (const auto* reg = std::get_if<RegisterOperand>(&selected.payload);
      selected.kind == OperandKind::Register && reg != nullptr) {
    const auto text = inline_asm_register_operand_text(*reg, modifier);
    if (!text.has_value()) {
      *diagnostic = modifier.has_value()
                        ? "inline-asm register operand has unsupported template modifier"
                        : "inline-asm register operand is not printable";
      return std::nullopt;
    }
    return text;
  }
  if (const auto* immediate = std::get_if<ImmediateOperand>(&selected.payload);
      selected.kind == OperandKind::Immediate && immediate != nullptr) {
    if (modifier.has_value()) {
      *diagnostic = "inline-asm immediate operand has unsupported template modifier";
      return std::nullopt;
    }
    if (immediate->kind != ImmediateKind::SignedInteger) {
      *diagnostic = "inline-asm immediate operand is not a signed integer";
      return std::nullopt;
    }
    return std::to_string(immediate->signed_value);
  }
  if (const auto* memory = std::get_if<MemoryOperand>(&selected.payload);
      selected.kind == OperandKind::Memory && memory != nullptr) {
    if (modifier.has_value()) {
      *diagnostic = "inline-asm memory operand has unsupported template modifier";
      return std::nullopt;
    }
    if (memory->support != MemoryOperandSupportKind::Prepared ||
        !memory->can_use_base_plus_offset) {
      *diagnostic = "inline-asm memory operand is not a prepared base+offset address";
      return std::nullopt;
    }
    const auto text = memory_address(*memory);
    if (text.empty()) {
      *diagnostic = "inline-asm memory operand is not printable";
      return std::nullopt;
    }
    return text;
  }

  *diagnostic = "inline-asm operand kind is outside the selected printer subset";
  return std::nullopt;
}

void append_inline_asm_output_character(std::vector<std::string>* lines,
                                        char ch) {
  if (ch == '\n') {
    lines->emplace_back();
    return;
  }
  if (ch != '\r') {
    lines->back().push_back(ch);
  }
}

void append_inline_asm_output_text(std::vector<std::string>* lines,
                                   std::string_view text) {
  for (const char ch : text) {
    append_inline_asm_output_character(lines, ch);
  }
}

InlineAsmSubstitutionResult substitute_inline_asm_template(
    const AssemblerInstructionRecord& assembler) {
  if (!assembler.has_inline_asm_payload) {
    return {.diagnostic = "assembler node is missing inline-asm payload"};
  }
  if (assembler.inline_asm_template.empty()) {
    return {.diagnostic = "inline-asm template is empty"};
  }
  for (const auto& operand : assembler.inline_asm_operands) {
    if (operand.kind == bir::InlineAsmOperandKind::Clobber) {
      return {.diagnostic = "inline-asm clobber operand requires structured clobber authority"};
    }
    if (operand.kind == bir::InlineAsmOperandKind::Unsupported) {
      return {.diagnostic = "inline-asm operand kind is unsupported for selected printer"};
    }
    if (operand.kind == bir::InlineAsmOperandKind::MemoryInput &&
        !operand.selected_operand.has_value()) {
      return {
          .diagnostic =
              "inline-asm memory operand requires selected structured memory address authority"};
    }
    if (operand.kind == bir::InlineAsmOperandKind::AddressInput &&
        !operand.selected_operand.has_value()) {
      return {
          .diagnostic =
              "inline-asm address operand requires selected structured address authority"};
    }
    if (!inline_asm_constraint_matches_kind(operand)) {
      return {.diagnostic = "inline-asm operand has unsupported constraint for selected printer"};
    }
  }

  std::vector<std::string> lines;
  lines.emplace_back();
  const auto& templ = assembler.inline_asm_template;
  for (std::size_t index = 0; index < templ.size(); ++index) {
    const char ch = templ[index];
    if (ch != '%') {
      append_inline_asm_output_character(&lines, ch);
      continue;
    }
    if (index + 1 >= templ.size()) {
      return {.diagnostic = "inline-asm template has unterminated placeholder"};
    }
    if (templ[index + 1] == '%') {
      append_inline_asm_output_character(&lines, '%');
      ++index;
      continue;
    }
    std::optional<char> modifier;
    std::size_t operand_start = index + 1;
    if ((templ[operand_start] >= 'A' && templ[operand_start] <= 'Z') ||
        (templ[operand_start] >= 'a' && templ[operand_start] <= 'z')) {
      modifier = templ[operand_start];
      ++operand_start;
    }
    if (operand_start < templ.size() && templ[operand_start] == '[') {
      const std::size_t name_start = operand_start + 1;
      std::size_t name_end = name_start;
      while (name_end < templ.size() && templ[name_end] != ']') {
        ++name_end;
      }
      if (name_end >= templ.size()) {
        return {.diagnostic = "inline-asm template has malformed named operand reference"};
      }

      const std::string_view name{templ.data() + name_start, name_end - name_start};
      const auto lookup = find_inline_asm_named_operand(assembler, name);
      if (!lookup.diagnostic.empty()) {
        return {.diagnostic = lookup.diagnostic};
      }

      std::string diagnostic;
      const auto replacement =
          inline_asm_operand_text(assembler, *lookup.operand, modifier, &diagnostic);
      if (!replacement.has_value()) {
        return {.diagnostic = std::move(diagnostic)};
      }
      append_inline_asm_output_text(&lines, *replacement);
      index = name_end;
      continue;
    }
    if (operand_start >= templ.size() || templ[operand_start] < '0' ||
        templ[operand_start] > '9') {
      return {.diagnostic = "inline-asm template has malformed placeholder"};
    }
    std::size_t operand_end = operand_start;
    while (operand_end < templ.size() && templ[operand_end] >= '0' &&
           templ[operand_end] <= '9') {
      ++operand_end;
    }
    const auto constraint_index =
        parse_decimal_index(std::string_view{templ}.substr(operand_start,
                                                           operand_end - operand_start));
    if (!constraint_index.has_value()) {
      return {.diagnostic = "inline-asm template operand index is not printable"};
    }
    const auto* operand =
        find_inline_asm_constraint_operand(assembler, *constraint_index);
    if (operand == nullptr) {
      return {.diagnostic = "inline-asm template references unknown operand"};
    }

    std::string diagnostic;
    const auto replacement = inline_asm_operand_text(assembler, *operand, modifier, &diagnostic);
    if (!replacement.has_value()) {
      return {.diagnostic = std::move(diagnostic)};
    }
    append_inline_asm_output_text(&lines, *replacement);
    index = operand_end - 1;
  }

  if (!lines.empty() && lines.back().empty() && templ.back() == '\n') {
    lines.pop_back();
  }
  if (lines.empty()) {
    return {.diagnostic = "inline-asm template produced no printable lines"};
  }
  return {.lines = std::move(lines)};
}

std::string bad_header(const InstructionRecord& instruction) {
  return std::string("cannot print AArch64 machine node family=") +
         std::string(instruction_family_name(instruction.family)) + " opcode=" +
         std::string(machine_opcode_name(instruction.opcode)) + ": ";
}

std::string_view required_primary_mnemonic(const InstructionRecord& instruction) {
  return machine_instruction_primary_printer_mnemonic(instruction);
}

std::string_view required_auxiliary_mnemonic(const InstructionRecord& instruction) {
  return machine_instruction_auxiliary_printer_mnemonic(instruction);
}

std::string_view required_branch_mnemonic() {
  return machine_printer_mnemonic_kind_name(MachinePrinterMnemonicKind::Branch);
}

std::optional<std::string> validate_selected_machine_node(const InstructionRecord& instruction) {
  if (instruction.surface != RecordSurfaceKind::MachineInstructionNode) {
    return std::string("printer requires surface machine_instruction_node, got ") +
           std::string(record_surface_kind_name(instruction.surface));
  }
  if (instruction.selection.status != MachineNodeSelectionStatus::Selected) {
    std::string diagnostic = "printer requires selected machine node, got ";
    diagnostic += machine_node_selection_status_name(instruction.selection.status);
    if (!instruction.selection.diagnostic.empty()) {
      diagnostic += ": ";
      diagnostic += instruction.selection.diagnostic;
    }
    return diagnostic;
  }
  return std::nullopt;
}

mir::TargetInstructionPrintResult print_assembler(
    const InstructionRecord& instruction,
    const AssemblerInstructionRecord& assembler) {
  if (instruction.family != InstructionFamily::Assembler) {
    return target_unsupported(bad_header(instruction) +
                              "inline-asm printer requires an assembler machine node");
  }
  const auto substituted = substitute_inline_asm_template(assembler);
  if (!substituted.lines.has_value()) {
    return target_unsupported(bad_header(instruction) + substituted.diagnostic);
  }
  return target_printed(*substituted.lines);
}

mir::TargetInstructionPrintResult print_address_materialization(
    const InstructionRecord& instruction,
    const AddressMaterializationRecord& address) {
  if (!address.result_register.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "address materialization node is missing result register");
  }

  std::string_view label;
  switch (address.kind) {
    case AddressMaterializationKind::DirectPageLow12:
      label = address.symbol_label;
      if (label.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "direct address materialization is missing symbol label");
      }
      break;
    case AddressMaterializationKind::GotPageLow12:
      label = address.symbol_label;
      if (label.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "GOT address materialization is missing symbol label");
      }
      if (address.address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::GotRequired) {
        return target_unsupported(bad_header(instruction) +
                                  "GOT address materialization is missing GOT-required policy");
      }
      break;
    case AddressMaterializationKind::StringConstant:
      label = address.text_label;
      if (label.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "string address materialization is missing text label");
      }
      break;
    case AddressMaterializationKind::LabelPageLow12:
      label = address.target_label_name;
      if (label.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "label address materialization is missing target label text");
      }
      break;
    case AddressMaterializationKind::TlsRelative:
      label = address.symbol_label;
      if (label.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "TLS address materialization is missing symbol label");
      }
      if (address.tls_model !=
              prepare::PreparedTlsMaterializationModel::LocalExecThreadPointerRelative ||
          address.tls_thread_pointer_register !=
              prepare::PreparedTlsThreadPointerRegister::Aarch64TpidrEl0) {
        return target_unsupported(bad_header(instruction) +
                                  "TLS address materialization is missing local-exec TLS facts");
      }
      if (address.tls_high_relocation != prepare::PreparedTlsRelocationKind::Aarch64TprelHi12 ||
          address.tls_low_relocation != prepare::PreparedTlsRelocationKind::Aarch64TprelLo12Nc) {
        return target_unsupported(
            bad_header(instruction) +
            "TLS address materialization is missing thread-pointer-relative relocations");
      }
      break;
    case AddressMaterializationKind::DeferredUnsupported:
      return target_unsupported(bad_header(instruction) +
                                "address materialization printer path is deferred for " +
                                std::string(prepare::prepared_address_materialization_kind_name(
                                    address.prepared_kind)));
  }

  const std::string result = register_name(*address.result_register);
  if (address.kind == AddressMaterializationKind::GotPageLow12) {
    std::vector<std::string> lines{
        "adrp " + result + ", " + prefixed_relocation_operand(":got:", label),
        "ldr " + result + ", [" + result + ", " +
            prefixed_relocation_operand(":got_lo12:", label) + "]",
    };
    if (address.byte_offset != 0) {
      lines.push_back("add " + result + ", " + result + ", #" +
                      std::to_string(address.byte_offset));
    }
    return target_printed(std::move(lines));
  }
  if (address.kind == AddressMaterializationKind::TlsRelative) {
    std::vector<std::string> lines{
        "mrs " + result + ", tpidr_el0",
        "add " + result + ", " + result + ", " +
            prefixed_relocation_operand(tls_relocation_prefix(address.tls_high_relocation), label),
        "add " + result + ", " + result + ", " +
            prefixed_relocation_operand(tls_relocation_prefix(address.tls_low_relocation), label),
    };
    if (address.byte_offset != 0) {
      lines.push_back("add " + result + ", " + result + ", #" +
                      std::to_string(address.byte_offset));
    }
    return target_printed(std::move(lines));
  }

  const std::string reloc = relocation_operand(label, address.byte_offset);
  return target_printed({
      "adrp " + result + ", " + reloc,
      "add " + result + ", " + result + ", :lo12:" + reloc,
  });
}

mir::TargetInstructionPrintResult print_spill_reload(
    const InstructionRecord& instruction,
    const SpillReloadInstructionRecord& spill_reload) {
  if (!spill_reload.scratch.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "spill/reload node is missing scratch register");
  }
  if (!spill_reload.stack_offset_bytes.has_value() || !spill_reload.stack_offset_is_prepared_snapshot ||
      !spill_reload.slot_id.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "spill/reload node is missing prepared stack-slot offset");
  }
  if (spill_reload.slot.support != MemoryOperandSupportKind::Prepared ||
      spill_reload.slot.base_kind != MemoryBaseKind::FrameSlot ||
      !spill_reload.slot.can_use_base_plus_offset) {
    return target_unsupported(bad_header(instruction) +
                              "spill/reload node is not a prepared frame-slot address");
  }

  const auto address = memory_address(spill_reload.slot);
  if (address.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "spill/reload address is not printable");
  }

  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "spill/reload pseudo kind is unsupported");
  }

  std::ostringstream out;
  out << mnemonic << " " << register_name(*spill_reload.scratch) << ", " << address;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_branch(const InstructionRecord& instruction,
                                               const BranchInstructionRecord& branch) {
  if (branch.target.function_name == c4c::kInvalidFunctionName ||
      branch.target.block_label == c4c::kInvalidBlockLabel) {
    return target_unsupported(bad_header(instruction) + "branch target identity is missing");
  }
  if (!branch.conditional) {
    const auto mnemonic = required_primary_mnemonic(instruction);
    if (mnemonic.empty()) {
      return target_unsupported(bad_header(instruction) + "branch mnemonic is not printable");
    }
    std::ostringstream out;
    out << mnemonic << " " << block_label(branch.target.function_name, branch.target.block_label);
    return target_printed({out.str()});
  }
  if (!branch.target_pair.has_value() || !branch.condition.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "conditional branch is missing target pair or condition operand");
  }
  const auto* condition = std::get_if<RegisterOperand>(&branch.condition->payload);
  if (branch.condition->kind != OperandKind::Register || condition == nullptr) {
    return target_unsupported(bad_header(instruction) +
                              "materialized-bool branch condition is not a register operand");
  }
  if (branch.condition_record.has_value() &&
      branch.condition_record->form != BranchConditionForm::MaterializedBool) {
    return target_unsupported(bad_header(instruction) +
                              "only materialized-bool conditional branches are printable");
  }

  const auto& targets = *branch.target_pair;
  if (targets.true_target.function_name == c4c::kInvalidFunctionName ||
      targets.true_target.block_label == c4c::kInvalidBlockLabel ||
      targets.false_target.function_name == c4c::kInvalidFunctionName ||
      targets.false_target.block_label == c4c::kInvalidBlockLabel) {
    return target_unsupported(bad_header(instruction) +
                              "conditional branch target identity is missing");
  }

  const auto condition_mnemonic = required_primary_mnemonic(instruction);
  const auto branch_mnemonic = required_branch_mnemonic();
  if (condition_mnemonic.empty() || branch_mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) + "branch mnemonic is not printable");
  }

  std::ostringstream condition_line;
  condition_line << condition_mnemonic << " " << register_name(*condition) << ", "
                 << block_label(targets.true_target.function_name,
                                targets.true_target.block_label);
  std::ostringstream branch_line;
  branch_line << branch_mnemonic << " "
              << block_label(targets.false_target.function_name,
                             targets.false_target.block_label);
  return target_printed({condition_line.str(), branch_line.str()});
}

mir::TargetInstructionPrintResult print_memory(const InstructionRecord& instruction,
                                               const MemoryInstructionRecord& memory) {
  if (memory.address.support != MemoryOperandSupportKind::Prepared ||
      !memory.address.can_use_base_plus_offset) {
    return target_unsupported(bad_header(instruction) +
                              "memory address is not a prepared base+offset");
  }
  const auto address = memory_address(memory.address);
  if (address.empty()) {
    return target_unsupported(bad_header(instruction) + "memory address is not printable");
  }
  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) + "memory mnemonic is not printable");
  }

  if (memory.memory_kind == MemoryInstructionKind::Load) {
    if (!memory.result_register.has_value()) {
      return target_unsupported(bad_header(instruction) +
                                "load node is missing a structured destination register operand");
    }
    std::ostringstream out;
    out << mnemonic << " " << register_name(*memory.result_register) << ", " << address;
    return target_printed({out.str()});
  }
  if (!memory.value.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "store node is missing stored value operand");
  }
  const auto* value = std::get_if<RegisterOperand>(&memory.value->payload);
  if (memory.value->kind != OperandKind::Register || value == nullptr) {
    return target_unsupported(bad_header(instruction) + "store value is not a register operand");
  }
  std::ostringstream out;
  out << mnemonic << " " << register_name(*value) << ", " << address;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_atomic_memory(
    const InstructionRecord& instruction,
    const AtomicMemoryInstructionRecord& atomic) {
  const auto pointer = atomic_pointer_register_name(atomic);
  if (atomic.atomic_kind != AtomicMemoryInstructionKind::Fence && !pointer.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "atomic memory node is missing printable pointer register");
  }
  const auto address = pointer.has_value() ? "[" + *pointer + "]" : std::string{};

  switch (atomic.atomic_kind) {
    case AtomicMemoryInstructionKind::Load: {
      if (!atomic.result_register.has_value()) {
        return target_unsupported(bad_header(instruction) +
                                  "atomic load is missing result register");
      }
      const auto result = atomic_value_register_name(*atomic.result_register, atomic.width_bytes);
      const auto mnemonic = atomic.acquire_semantics
                                ? atomic_acquire_load_mnemonic(atomic.width_bytes)
                                : atomic_plain_load_mnemonic(atomic.width_bytes);
      if (!result.has_value() || mnemonic.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "atomic load has unsupported width or register view");
      }
      std::ostringstream out;
      out << mnemonic << " " << *result << ", " << address;
      return target_printed({out.str()});
    }
    case AtomicMemoryInstructionKind::Store: {
      if (!atomic.stored_register.has_value()) {
        return target_unsupported(bad_header(instruction) +
                                  "atomic store is missing stored register");
      }
      const auto stored = atomic_value_register_name(*atomic.stored_register, atomic.width_bytes);
      const auto mnemonic = atomic.release_semantics
                                ? atomic_release_store_mnemonic(atomic.width_bytes)
                                : atomic_plain_store_mnemonic(atomic.width_bytes);
      if (!stored.has_value() || mnemonic.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "atomic store has unsupported width or register view");
      }
      std::ostringstream out;
      out << mnemonic << " " << *stored << ", " << address;
      return target_printed({out.str()});
    }
    case AtomicMemoryInstructionKind::Fence:
      if (!atomic.memory_barrier_required) {
        return target_unsupported(bad_header(instruction) +
                                  "relaxed atomic fence is outside the printable subset");
      }
      return target_printed({"dmb ish"});
    case AtomicMemoryInstructionKind::RmwLoop: {
      if (!atomic.result_register.has_value() || !atomic.stored_register.has_value() ||
          !atomic.rmw_new_value_register.has_value() ||
          !atomic.exclusive_status_register.has_value() || !atomic.exclusive_retry_loop) {
        return target_unsupported(
            bad_header(instruction) +
            "atomic rmw loop is missing structured result, value, scratch, or status registers");
      }
      const auto old_value = atomic_value_register_name(*atomic.result_register,
                                                        atomic.width_bytes);
      const auto operand = atomic_value_register_name(*atomic.stored_register,
                                                      atomic.width_bytes);
      const auto new_value = atomic_value_register_name(*atomic.rmw_new_value_register,
                                                        atomic.width_bytes);
      const auto status = atomic_value_register_name(*atomic.exclusive_status_register, 4);
      const auto load = atomic_exclusive_load_mnemonic(atomic.width_bytes,
                                                       atomic.acquire_semantics);
      const auto store = atomic_exclusive_store_mnemonic(atomic.width_bytes,
                                                         atomic.release_semantics);
      const auto op = atomic_rmw_operation_mnemonic(atomic.rmw_opcode);
      if (!old_value.has_value() || !operand.has_value() || !new_value.has_value() ||
          !status.has_value() || load.empty() || store.empty() || op.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "atomic rmw loop has unsupported width, opcode, or register view");
      }
      const auto retry = atomic_loop_label(atomic, "retry");
      std::vector<std::string> lines;
      lines.push_back(retry + ":");
      lines.push_back(std::string(load) + " " + *old_value + ", " + address);
      if (atomic.rmw_opcode == bir::AtomicRmwOpcode::Exchange) {
        lines.push_back(std::string(op) + " " + *new_value + ", " + *operand);
      } else {
        lines.push_back(std::string(op) + " " + *new_value + ", " + *old_value + ", " +
                        *operand);
      }
      lines.push_back(std::string(store) + " " + *status + ", " + *new_value + ", " +
                      address);
      lines.push_back("cbnz " + *status + ", " + retry);
      return target_printed(std::move(lines));
    }
    case AtomicMemoryInstructionKind::CompareExchangeLoop: {
      if (!atomic.expected_register.has_value() || !atomic.desired_register.has_value() ||
          !atomic.result_register.has_value() || !atomic.exclusive_status_register.has_value() ||
          !atomic.exclusive_retry_loop || !atomic.compare_exchange_failure_clears_monitor) {
        return target_unsupported(
            bad_header(instruction) +
            "atomic compare-exchange loop is missing structured operands, status, or monitor-clear facts");
      }
      const RegisterOperand* loaded_operand = nullptr;
      if (atomic.compare_exchange_result_is_old_value) {
        loaded_operand = &*atomic.result_register;
      } else if (atomic.compare_loaded_register.has_value()) {
        loaded_operand = &*atomic.compare_loaded_register;
      }
      if (loaded_operand == nullptr) {
        return target_unsupported(bad_header(instruction) +
                                  "atomic compare-exchange loop is missing loaded-value register");
      }
      const bool failure_acquire = atomic_ordering_has_acquire(atomic.failure_ordering);
      const bool success_needs_post_acquire =
          atomic.acquire_semantics && !failure_acquire;
      const auto loaded = atomic_value_register_name(*loaded_operand, atomic.width_bytes);
      const auto expected = atomic_value_register_name(*atomic.expected_register,
                                                       atomic.width_bytes);
      const auto desired = atomic_value_register_name(*atomic.desired_register,
                                                      atomic.width_bytes);
      const auto result = atomic_value_register_name(*atomic.result_register,
                                                     atomic.compare_exchange_result_is_boolean
                                                         ? 4
                                                         : atomic.width_bytes);
      const auto status = atomic_value_register_name(*atomic.exclusive_status_register, 4);
      const auto load = atomic_exclusive_load_mnemonic(atomic.width_bytes, failure_acquire);
      const auto store = atomic_exclusive_store_mnemonic(atomic.width_bytes,
                                                         atomic.release_semantics);
      if (!loaded.has_value() || !expected.has_value() || !desired.has_value() ||
          !result.has_value() || !status.has_value() || load.empty() || store.empty()) {
        return target_unsupported(
            bad_header(instruction) +
            "atomic compare-exchange loop has unsupported width or register view");
      }
      const auto retry = atomic_loop_label(atomic, "retry");
      const auto failure = atomic_loop_label(atomic, "failure");
      const auto done = atomic_loop_label(atomic, "done");
      std::vector<std::string> lines;
      lines.push_back(retry + ":");
      lines.push_back(std::string(load) + " " + *loaded + ", " + address);
      lines.push_back("cmp " + *loaded + ", " + *expected);
      lines.push_back("b.ne " + failure);
      lines.push_back(std::string(store) + " " + *status + ", " + *desired + ", " +
                      address);
      lines.push_back("cbnz " + *status + ", " + retry);
      if (success_needs_post_acquire) {
        lines.push_back("dmb ishld");
      }
      if (atomic.compare_exchange_result_is_boolean) {
        lines.push_back("mov " + *result + ", #1");
        lines.push_back("b " + done);
      }
      lines.push_back(failure + ":");
      lines.push_back("clrex");
      if (atomic.compare_exchange_result_is_boolean) {
        lines.push_back("mov " + *result + ", #0");
        lines.push_back(done + ":");
      }
      return target_printed(std::move(lines));
    }
  }
  return target_unsupported(bad_header(instruction) +
                            "atomic memory kind is outside the printable subset");
}

mir::TargetInstructionPrintResult print_i128_transport(
    const InstructionRecord& instruction,
    const I128TransportRecord& transport) {
  const auto low = lane_register_name(transport.low_lane);
  const auto high = lane_register_name(transport.high_lane);
  if (!low.has_value() || !high.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "i128 transport node is missing structured low/high registers");
  }
  if (transport.transport_kind == I128TransportKind::CarrierSnapshot) {
    std::ostringstream out;
    out << "// i128 carrier " << *low << ", " << *high;
    return target_printed({out.str()});
  }
  if (!transport.memory.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "i128 memory transport is missing structured memory operand");
  }
  const auto address = memory_address(*transport.memory);
  if (address.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "i128 memory transport address is not printable");
  }
  std::ostringstream out;
  if (transport.transport_kind == I128TransportKind::LoadFromMemory) {
    out << "ldp " << *low << ", " << *high << ", " << address;
    return target_printed({out.str()});
  }
  if (transport.transport_kind == I128TransportKind::StoreToMemory) {
    out << "stp " << *low << ", " << *high << ", " << address;
    return target_printed({out.str()});
  }
  return target_unsupported(bad_header(instruction) +
                            "i128 transport kind is outside the printable subset");
}

mir::TargetInstructionPrintResult print_f128_transport(
    const InstructionRecord& instruction,
    const F128TransportRecord& transport) {
  if (transport.carrier_kind != prepare::PreparedF128CarrierKind::FullWidthRegister ||
      !transport.reg.has_value()) {
    return target_unsupported(
        bad_header(instruction) +
        "f128 transport printer requires structured full-width q-register authority");
  }
  const auto reg = f128_q_register_name(*transport.reg);
  if (!reg.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "f128 transport q-register is not printable");
  }
  if (transport.transport_kind == F128TransportKind::CarrierSnapshot) {
    std::ostringstream out;
    out << "// f128 carrier " << *reg;
    return target_printed({out.str()});
  }
  if (!transport.memory.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "f128 memory transport is missing structured memory operand");
  }
  if (transport.memory->size_bytes != 16 || transport.memory->align_bytes != 16) {
    return target_unsupported(bad_header(instruction) +
                              "f128 memory transport requires 16-byte memory facts");
  }
  const auto address = memory_address(*transport.memory);
  if (address.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "f128 memory transport address is not printable");
  }
  std::ostringstream out;
  if (transport.transport_kind == F128TransportKind::LoadFromMemory) {
    out << "ldr " << *reg << ", " << address;
    return target_printed({out.str()});
  }
  if (transport.transport_kind == F128TransportKind::StoreToMemory) {
    out << "str " << *reg << ", " << address;
    return target_printed({out.str()});
  }
  return target_unsupported(bad_header(instruction) +
                            "f128 transport kind is outside the printable subset");
}

mir::TargetInstructionPrintResult print_i128_pair_operation(
    const InstructionRecord& instruction,
    const I128PairOperationRecord& pair) {
  const auto result_low = pair_low_register_name(pair.result);
  const auto result_high = pair_high_register_name(pair.result);
  const auto lhs_low = pair_low_register_name(pair.lhs);
  const auto lhs_high = pair_high_register_name(pair.lhs);
  const auto rhs_low = pair_low_register_name(pair.rhs);
  const auto rhs_high = pair_high_register_name(pair.rhs);
  if (!result_low.has_value() || !result_high.has_value() ||
      !lhs_low.has_value() || !lhs_high.has_value() ||
      !rhs_low.has_value() || !rhs_high.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "i128 pair node is missing structured low/high registers");
  }

  std::vector<std::string> lines;
  auto emit_independent = [&](std::string_view mnemonic) {
    std::ostringstream low;
    low << mnemonic << " " << *result_low << ", " << *lhs_low << ", " << *rhs_low;
    lines.push_back(low.str());
    std::ostringstream high;
    high << mnemonic << " " << *result_high << ", " << *lhs_high << ", " << *rhs_high;
    lines.push_back(high.str());
  };

  switch (pair.operation) {
    case I128PairOperationKind::Add: {
      std::ostringstream low;
      low << "adds " << *result_low << ", " << *lhs_low << ", " << *rhs_low;
      lines.push_back(low.str());
      std::ostringstream high;
      high << "adc " << *result_high << ", " << *lhs_high << ", " << *rhs_high;
      lines.push_back(high.str());
      return target_printed(std::move(lines));
    }
    case I128PairOperationKind::Sub: {
      std::ostringstream low;
      low << "subs " << *result_low << ", " << *lhs_low << ", " << *rhs_low;
      lines.push_back(low.str());
      std::ostringstream high;
      high << "sbc " << *result_high << ", " << *lhs_high << ", " << *rhs_high;
      lines.push_back(high.str());
      return target_printed(std::move(lines));
    }
    case I128PairOperationKind::And:
      emit_independent("and");
      return target_printed(std::move(lines));
    case I128PairOperationKind::Or:
      emit_independent("orr");
      return target_printed(std::move(lines));
    case I128PairOperationKind::Xor:
      emit_independent("eor");
      return target_printed(std::move(lines));
  }
  return target_unsupported(bad_header(instruction) +
                            "i128 pair operation is outside the printable subset");
}

mir::TargetInstructionPrintResult print_i128_shift(const InstructionRecord& instruction,
                                                   const I128ShiftRecord& shift) {
  if (shift.count_kind != I128ShiftCountKind::Immediate) {
    return target_unsupported(bad_header(instruction) +
                              "i128 shift printer currently requires an immediate shift count");
  }
  const auto* immediate = std::get_if<ImmediateOperand>(&shift.shift_count.payload);
  if (shift.shift_count.kind != OperandKind::Immediate || immediate == nullptr ||
      immediate->signed_value < 0 || immediate->signed_value >= 64) {
    return target_unsupported(bad_header(instruction) +
                              "i128 shift immediate is outside the printable 0..63 subset");
  }
  const auto amount = immediate->signed_value;
  const auto result_low = pair_low_register_name(shift.result);
  const auto result_high = pair_high_register_name(shift.result);
  const auto source_low = pair_low_register_name(shift.source);
  const auto source_high = pair_high_register_name(shift.source);
  if (!result_low.has_value() || !result_high.has_value() ||
      !source_low.has_value() || !source_high.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "i128 shift node is missing structured low/high registers");
  }

  std::vector<std::string> lines;
  auto emit_mov_pair = [&]() {
    std::ostringstream low;
    low << "mov " << *result_low << ", " << *source_low;
    lines.push_back(low.str());
    std::ostringstream high;
    high << "mov " << *result_high << ", " << *source_high;
    lines.push_back(high.str());
  };
  if (amount == 0) {
    emit_mov_pair();
    return target_printed(std::move(lines));
  }

  switch (shift.shift_kind) {
    case I128ShiftKind::Left: {
      std::ostringstream low;
      low << "lsl " << *result_low << ", " << *source_low << ", #" << amount;
      lines.push_back(low.str());
      std::ostringstream high;
      high << "extr " << *result_high << ", " << *source_high << ", " << *source_low
           << ", #" << (64 - amount);
      lines.push_back(high.str());
      return target_printed(std::move(lines));
    }
    case I128ShiftKind::LogicalRight: {
      std::ostringstream low;
      low << "extr " << *result_low << ", " << *source_low << ", " << *source_high
          << ", #" << amount;
      lines.push_back(low.str());
      std::ostringstream high;
      high << "lsr " << *result_high << ", " << *source_high << ", #" << amount;
      lines.push_back(high.str());
      return target_printed(std::move(lines));
    }
    case I128ShiftKind::ArithmeticRight: {
      std::ostringstream low;
      low << "extr " << *result_low << ", " << *source_low << ", " << *source_high
          << ", #" << amount;
      lines.push_back(low.str());
      std::ostringstream high;
      high << "asr " << *result_high << ", " << *source_high << ", #" << amount;
      lines.push_back(high.str());
      return target_printed(std::move(lines));
    }
  }
  return target_unsupported(bad_header(instruction) +
                            "i128 shift kind is outside the printable subset");
}

mir::TargetInstructionPrintResult print_i128_compare(const InstructionRecord& instruction,
                                                     const I128CompareRecord& compare) {
  if (!compare.result_register.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "i128 compare node is missing structured result register");
  }
  const auto result = register_name(*compare.result_register);
  const auto lhs_low = pair_low_register_name(compare.lhs);
  const auto lhs_high = pair_high_register_name(compare.lhs);
  const auto rhs_low = pair_low_register_name(compare.rhs);
  const auto rhs_high = pair_high_register_name(compare.rhs);
  if (!lhs_low.has_value() || !lhs_high.has_value() ||
      !rhs_low.has_value() || !rhs_high.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "i128 compare node is missing structured low/high registers");
  }

  std::vector<std::string> lines;
  switch (compare.predicate) {
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne: {
      const auto condition = compare.predicate == bir::BinaryOpcode::Eq ? "eq" : "ne";
      {
        std::ostringstream high;
        high << "cmp " << *lhs_high << ", " << *rhs_high;
        lines.push_back(high.str());
      }
      {
        std::ostringstream low;
        low << "ccmp " << *lhs_low << ", " << *rhs_low << ", #0, eq";
        lines.push_back(low.str());
      }
      {
        std::ostringstream cset;
        cset << "cset " << result << ", " << condition;
        lines.push_back(cset.str());
      }
      return target_printed(std::move(lines));
    }
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      break;
    default:
      return target_unsupported(
          bad_header(instruction) +
          "i128 compare printer supports equality and relational predicates only");
  }

  const auto signed_high =
      compare.predicate == bir::BinaryOpcode::Slt ||
      compare.predicate == bir::BinaryOpcode::Sle ||
      compare.predicate == bir::BinaryOpcode::Sgt ||
      compare.predicate == bir::BinaryOpcode::Sge;
  const auto greater_predicate =
      compare.predicate == bir::BinaryOpcode::Sgt ||
      compare.predicate == bir::BinaryOpcode::Sge ||
      compare.predicate == bir::BinaryOpcode::Ugt ||
      compare.predicate == bir::BinaryOpcode::Uge;
  const auto inclusive_predicate =
      compare.predicate == bir::BinaryOpcode::Sle ||
      compare.predicate == bir::BinaryOpcode::Sge ||
      compare.predicate == bir::BinaryOpcode::Ule ||
      compare.predicate == bir::BinaryOpcode::Uge;
  const auto high_true = greater_predicate ? "gt" : "lt";
  const auto high_false = greater_predicate ? "lt" : "gt";
  const auto unsigned_high_true = greater_predicate ? "hi" : "lo";
  const auto unsigned_high_false = greater_predicate ? "lo" : "hi";
  const auto low_true =
      greater_predicate ? (inclusive_predicate ? "hs" : "hi")
                        : (inclusive_predicate ? "ls" : "lo");
  const auto true_label = ".L_i128cmp_" + std::to_string(compare.function_name) + "_" +
                          std::to_string(compare.block_label) + "_" +
                          std::to_string(compare.instruction_index) + "_true";
  const auto false_label = ".L_i128cmp_" + std::to_string(compare.function_name) + "_" +
                           std::to_string(compare.block_label) + "_" +
                           std::to_string(compare.instruction_index) + "_false";
  const auto done_label = ".L_i128cmp_" + std::to_string(compare.function_name) + "_" +
                          std::to_string(compare.block_label) + "_" +
                          std::to_string(compare.instruction_index) + "_done";
  {
    std::ostringstream high;
    high << "cmp " << *lhs_high << ", " << *rhs_high;
    lines.push_back(high.str());
  }
  {
    std::ostringstream high_true_branch;
    high_true_branch << "b." << (signed_high ? high_true : unsigned_high_true) << " "
                     << true_label;
    lines.push_back(high_true_branch.str());
  }
  {
    std::ostringstream high_false_branch;
    high_false_branch << "b." << (signed_high ? high_false : unsigned_high_false) << " "
                      << false_label;
    lines.push_back(high_false_branch.str());
  }
  {
    std::ostringstream low;
    low << "cmp " << *lhs_low << ", " << *rhs_low;
    lines.push_back(low.str());
  }
  {
    std::ostringstream low_true_branch;
    low_true_branch << "b." << low_true << " " << true_label;
    lines.push_back(low_true_branch.str());
  }
  {
    std::ostringstream fallthrough;
    fallthrough << "b " << false_label;
    lines.push_back(fallthrough.str());
  }
  lines.push_back(true_label + ":");
  {
    std::ostringstream set_true;
    set_true << "mov " << result << ", #1";
    lines.push_back(set_true.str());
  }
  {
    std::ostringstream done;
    done << "b " << done_label;
    lines.push_back(done.str());
  }
  lines.push_back(false_label + ":");
  {
    std::ostringstream set_false;
    set_false << "mov " << result << ", #0";
    lines.push_back(set_false.str());
  }
  lines.push_back(done_label + ":");
  return target_printed(std::move(lines));
}

mir::TargetInstructionPrintResult print_i128_runtime_helper(
    const InstructionRecord& instruction,
    const I128RuntimeHelperBoundaryRecord& helper) {
  if (helper.source_helper == nullptr) {
    return target_unsupported(bad_header(instruction) +
                              "i128 helper boundary is missing prepared helper provenance");
  }
  if (helper.helper_family != prepare::PreparedI128RuntimeHelperFamily::DivRem ||
      helper.callee_name.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "i128 helper boundary is missing div/rem callee facts");
  }
  if (helper.result_ownership !=
      prepare::PreparedI128RuntimeHelperResultOwnership::DirectLowHighLanes) {
    return target_unsupported(bad_header(instruction) +
                              "i128 helper boundary requires direct low/high result ownership");
  }
  if (!helper.resource_policy.call_boundary ||
      !helper.resource_policy.runtime_helper_callee ||
      !helper.resource_policy.caller_saved_clobbers ||
      !helper.resource_policy.preserves_source_operation_identity ||
      helper.clobbered_registers.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "i128 helper boundary is missing resource or clobber policy");
  }
  if (helper.abi_policy.transition !=
          prepare::PreparedI128RuntimeHelperAbiTransition::
              DirectRegisterPairArgumentsAndResult ||
      helper.abi_policy.argument_bank != prepare::PreparedRegisterBank::Gpr ||
      helper.abi_policy.result_bank != prepare::PreparedRegisterBank::Gpr ||
      helper.abi_policy.argument_count != 2 ||
      helper.abi_policy.lanes_per_argument != 2 ||
      helper.abi_policy.result_lane_count != 2 ||
      helper.abi_policy.lane_width_bytes != 8) {
    return target_unsupported(bad_header(instruction) +
                              "i128 helper boundary is missing ABI/register-bank policy");
  }
  if (!has_complete_live_preservation(helper.live_preservation_policy) ||
      !has_complete_live_preservation(helper.source_helper->live_preservation_policy)) {
    return target_unsupported(
        bad_header(instruction) +
        "i128 helper boundary printing requires complete live-preservation facts");
  }
  if (!has_complete_selected_call_ownership(helper.selected_call_ownership) ||
      !has_complete_selected_call_ownership(helper.source_helper->selected_call_ownership)) {
    return target_unsupported(
        bad_header(instruction) +
        "i128 helper boundary printing requires selected-call ownership facts");
  }
  if (helper.source_helper->callee_name != helper.callee_name ||
      helper.source_helper->helper_family != helper.helper_family ||
      helper.source_helper->helper_kind != helper.helper_kind ||
      helper.source_helper->source_binary_opcode != helper.source_binary_opcode ||
      helper.source_helper->result_ownership != helper.result_ownership) {
    return target_unsupported(
        bad_header(instruction) +
        "i128 helper boundary source helper facts do not match selected record");
  }

  std::vector<std::string> lines;
  const auto append_move =
      [&](const std::optional<prepare::PreparedI128RuntimeHelper::MarshalingMove>& move,
          prepare::PreparedI128RuntimeHelperMarshalDirection direction,
          prepare::PreparedMovePhase phase) -> std::optional<std::string> {
    return append_i128_helper_move_line(lines, move, direction, phase);
  };

  const auto before_call =
      prepare::PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument;
  const auto after_call =
      prepare::PreparedI128RuntimeHelperMarshalDirection::AbiResultToCarrierLane;
  if (const auto error = append_move(helper.source_helper->lhs_low_argument_move,
                                     before_call,
                                     prepare::PreparedMovePhase::BeforeCall);
      error.has_value()) {
    return target_unsupported(bad_header(instruction) + *error);
  }
  if (const auto error = append_move(helper.source_helper->lhs_high_argument_move,
                                     before_call,
                                     prepare::PreparedMovePhase::BeforeCall);
      error.has_value()) {
    return target_unsupported(bad_header(instruction) + *error);
  }
  if (const auto error = append_move(helper.source_helper->rhs_low_argument_move,
                                     before_call,
                                     prepare::PreparedMovePhase::BeforeCall);
      error.has_value()) {
    return target_unsupported(bad_header(instruction) + *error);
  }
  if (const auto error = append_move(helper.source_helper->rhs_high_argument_move,
                                     before_call,
                                     prepare::PreparedMovePhase::BeforeCall);
      error.has_value()) {
    return target_unsupported(bad_header(instruction) + *error);
  }
  {
    std::ostringstream call;
    call << "bl " << helper.callee_name;
    lines.push_back(call.str());
  }
  if (const auto error = append_move(helper.source_helper->result_low_unmarshal_move,
                                     after_call,
                                     prepare::PreparedMovePhase::AfterCall);
      error.has_value()) {
    return target_unsupported(bad_header(instruction) + *error);
  }
  if (const auto error = append_move(helper.source_helper->result_high_unmarshal_move,
                                     after_call,
                                     prepare::PreparedMovePhase::AfterCall);
      error.has_value()) {
    return target_unsupported(bad_header(instruction) + *error);
  }

  return target_printed(std::move(lines));
}

mir::TargetInstructionPrintResult print_f128_runtime_helper(
    const InstructionRecord& instruction,
    const F128RuntimeHelperBoundaryRecord& helper) {
  if (helper.source_helper == nullptr) {
    return target_unsupported(bad_header(instruction) +
                              "f128 helper boundary is missing prepared helper provenance");
  }
  if (helper.callee_name.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "f128 helper boundary is missing callee identity");
  }
  if (!helper.resource_policy.call_boundary ||
      !helper.resource_policy.runtime_helper_callee ||
      !helper.resource_policy.caller_saved_clobbers ||
      !helper.resource_policy.preserves_source_operation_identity ||
      helper.clobbered_registers.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "f128 helper boundary is missing resource or clobber policy");
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
    return target_unsupported(
        bad_header(instruction) +
        "f128 helper boundary printing requires complete selected-call ownership facts");
  }

  std::vector<std::string> lines;
  auto append_full_width =
      [&](const F128RuntimeHelperOperandRecord& operand,
          prepare::PreparedF128RuntimeHelperMarshalDirection direction)
      -> std::optional<std::string> {
    return append_f128_helper_move_line(lines, operand, direction);
  };
  auto append_scalar =
      [&](const F128RuntimeHelperScalarResultRecord& scalar,
          prepare::PreparedF128RuntimeHelperMarshalDirection direction)
      -> std::optional<std::string> {
    return append_f128_scalar_move_line(lines, scalar, direction);
  };
  const auto emit_call = [&]() {
    std::ostringstream call;
    call << "bl " << helper.callee_name;
    lines.push_back(call.str());
  };

  const auto carrier_to_abi =
      prepare::PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument;
  const auto abi_to_carrier =
      prepare::PreparedF128RuntimeHelperMarshalDirection::AbiResultToCarrier;
  const auto scalar_to_abi =
      prepare::PreparedF128RuntimeHelperMarshalDirection::ScalarToAbiArgument;
  const auto abi_to_scalar =
      prepare::PreparedF128RuntimeHelperMarshalDirection::AbiResultToScalar;

  if (helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Arithmetic) {
    if (helper.result_ownership !=
            prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier ||
        helper.source_type != bir::TypeKind::F128 ||
        helper.result_type != bir::TypeKind::F128 ||
        helper.abi_policy.transition !=
            prepare::PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentsAndResult) {
      return target_unsupported(bad_header(instruction) +
                                "f128 arithmetic helper has unsupported ownership or ABI facts");
    }
    if (const auto error = append_full_width(helper.lhs, carrier_to_abi);
        error.has_value()) {
      return target_unsupported(bad_header(instruction) + *error);
    }
    if (const auto error = append_full_width(helper.rhs, carrier_to_abi);
        error.has_value()) {
      return target_unsupported(bad_header(instruction) + *error);
    }
    emit_call();
    if (const auto error = append_full_width(helper.result, abi_to_carrier);
        error.has_value()) {
      return target_unsupported(bad_header(instruction) + *error);
    }
    return target_printed(std::move(lines));
  }

  if (helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Comparison) {
    if (helper.result_ownership !=
            prepare::PreparedF128RuntimeHelperResultOwnership::ScalarCmpResult ||
        helper.source_type != bir::TypeKind::F128 ||
        helper.result_type != bir::TypeKind::I32 ||
        helper.abi_policy.transition !=
            prepare::PreparedF128RuntimeHelperAbiTransition::
                DirectF128ArgumentsAndCmpResult ||
        !helper.scalar_result.cmp_result_consumption.has_value()) {
      return target_unsupported(bad_header(instruction) +
                                "f128 comparison helper has unsupported ownership or ABI facts");
    }
    if (const auto error = append_full_width(helper.lhs, carrier_to_abi);
        error.has_value()) {
      return target_unsupported(bad_header(instruction) + *error);
    }
    if (const auto error = append_full_width(helper.rhs, carrier_to_abi);
        error.has_value()) {
      return target_unsupported(bad_header(instruction) + *error);
    }
    emit_call();
    if (const auto error = validate_f128_cmp_scalar_result(helper.scalar_result);
        error.has_value()) {
      return target_unsupported(bad_header(instruction) + *error);
    }
    const auto condition = f128_cmp_condition(
        helper.scalar_result.cmp_result_consumption->zero_test);
    if (!condition.has_value()) {
      return target_unsupported(bad_header(instruction) +
                                "f128 comparison helper has unsupported zero-test facts");
    }
    std::ostringstream cmp;
    cmp << "cmp " << register_name(*helper.scalar_result.abi_register) << ", #0";
    lines.push_back(cmp.str());
    std::ostringstream cset;
    cset << "cset " << register_name(*helper.scalar_result.materialized_i1_register)
         << ", " << *condition;
    lines.push_back(cset.str());
    return target_printed(std::move(lines));
  }

  if (helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Cast) {
    if (helper.result_type == bir::TypeKind::F128) {
      if (helper.result_ownership !=
              prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier ||
          helper.abi_policy.transition !=
              prepare::PreparedF128RuntimeHelperAbiTransition::
                  DirectScalarArgumentAndF128Result) {
        return target_unsupported(bad_header(instruction) +
                                  "f128 cast helper has unsupported scalar-to-f128 ABI facts");
      }
      if (const auto error = append_scalar(helper.scalar_operand, scalar_to_abi);
          error.has_value()) {
        return target_unsupported(bad_header(instruction) + *error);
      }
      emit_call();
      if (const auto error = append_full_width(helper.result, abi_to_carrier);
          error.has_value()) {
        return target_unsupported(bad_header(instruction) + *error);
      }
      return target_printed(std::move(lines));
    }
    if (helper.source_type == bir::TypeKind::F128) {
      if (helper.result_ownership !=
              prepare::PreparedF128RuntimeHelperResultOwnership::ScalarValue ||
          helper.abi_policy.transition !=
              prepare::PreparedF128RuntimeHelperAbiTransition::
                  DirectF128ArgumentAndScalarResult) {
        return target_unsupported(bad_header(instruction) +
                                  "f128 cast helper has unsupported f128-to-scalar ABI facts");
      }
      if (const auto error = append_full_width(helper.lhs, carrier_to_abi);
          error.has_value()) {
        return target_unsupported(bad_header(instruction) + *error);
      }
      emit_call();
      if (const auto error = append_scalar(helper.scalar_result, abi_to_scalar);
          error.has_value()) {
        return target_unsupported(bad_header(instruction) + *error);
      }
      return target_printed(std::move(lines));
    }
  }

  return target_unsupported(bad_header(instruction) +
                            "f128 helper family is outside the printable subset");
}

mir::TargetInstructionPrintResult print_call(const InstructionRecord& instruction,
                                             const CallInstructionRecord& call) {
  if (call.variadic_entry_helper ==
      std::optional<prepare::PreparedVariadicEntryHelperKind>{
          prepare::PreparedVariadicEntryHelperKind::VaStart}) {
    if (!call.variadic_va_start.has_value() || call.source_variadic_entry == nullptr ||
        call.source_variadic_helper_operand_homes == nullptr) {
      return target_unsupported(
          bad_header(instruction) +
          "va_start node is missing structured prepared va_start provenance");
    }
    const auto mnemonic = required_primary_mnemonic(instruction);
    if (mnemonic.empty()) {
      return target_unsupported(bad_header(instruction) +
                                "va_start mnemonic is not printable");
    }

    const auto& va_start = *call.variadic_va_start;
    std::vector<std::string> lines;
    {
      std::ostringstream out;
      out << mnemonic << " dest="
          << prepared_value_home_name(va_start.destination_va_list)
          << " named_gp=" << va_start.named_gp_register_count
          << " named_fp=" << va_start.named_fp_register_count
          << " va_list_size=" << va_start.va_list_size_bytes
          << " va_list_align=" << va_start.va_list_align_bytes
          << " scratch_registers=" << va_start.scratch_register_count
          << " scratch_stack=" << va_start.scratch_stack_bytes;
      lines.push_back(out.str());
    }
    {
      std::ostringstream out;
      out << "va.start.rsa slot#" << va_start.register_save_area_slot_id
          << " stack+" << va_start.register_save_area_stack_offset_bytes
          << " size=" << va_start.register_save_area_size_bytes
          << " align=" << va_start.register_save_area_align_bytes
          << " gp_offset=" << va_start.register_save_area_gp_offset_bytes
          << " fp_offset=" << va_start.register_save_area_fp_offset_bytes
          << " gp_slot=" << va_start.register_save_area_gp_slot_size_bytes
          << " fp_slot=" << va_start.register_save_area_fp_slot_size_bytes
          << " saved_gp=" << va_start.saved_gp_register_count
          << " saved_fp=" << va_start.saved_fp_register_count;
      lines.push_back(out.str());
    }
    {
      std::ostringstream out;
      out << "va.start.initial_offsets gp=" << va_start.initial_gp_offset_bytes
          << " fp=" << va_start.initial_fp_offset_bytes
          << " overflow_slot#" << va_start.overflow_area_base_slot_id
          << " overflow_stack+" << va_start.overflow_area_base_stack_offset_bytes
          << " overflow_align=" << va_start.overflow_area_align_bytes;
      lines.push_back(out.str());
    }
    for (const auto& field : va_start.va_list_fields) {
      std::ostringstream out;
      out << "va.start.field kind="
          << prepare::prepared_variadic_va_list_field_kind_name(field.kind)
          << " offset=" << field.offset_bytes
          << " size=" << field.size_bytes;
      lines.push_back(out.str());
    }
    return target_printed(std::move(lines));
  }

  if (call.variadic_entry_helper ==
      std::optional<prepare::PreparedVariadicEntryHelperKind>{
          prepare::PreparedVariadicEntryHelperKind::VaArg}) {
    if (!call.variadic_scalar_va_arg.has_value() ||
        call.source_variadic_entry == nullptr ||
        call.source_variadic_helper_operand_homes == nullptr) {
      return target_unsupported(
          bad_header(instruction) +
          "scalar va_arg node is missing structured prepared access-plan provenance");
    }
    const auto mnemonic = required_primary_mnemonic(instruction);
    if (mnemonic.empty()) {
      return target_unsupported(bad_header(instruction) +
                                "scalar va_arg mnemonic is not printable");
    }

    const auto& va_arg = *call.variadic_scalar_va_arg;
    std::vector<std::string> lines;
    {
      std::ostringstream out;
      out << mnemonic << " source="
          << prepare::prepared_variadic_scalar_va_arg_source_class_name(
                 va_arg.source_class)
          << " va_list=" << prepared_value_home_name(va_arg.source_va_list)
          << " result=" << prepared_value_home_name(va_arg.result_home)
          << " value_size=" << va_arg.value_size_bytes
          << " value_align=" << va_arg.value_align_bytes
          << " scratch_registers=" << va_arg.scratch_register_count
          << " scratch_stack=" << va_arg.scratch_stack_bytes;
      lines.push_back(out.str());
    }
    {
      std::ostringstream out;
      out << "va.arg.scalar.source field="
          << prepare::prepared_variadic_va_list_field_kind_name(va_arg.source_field)
          << " field_offset=" << va_arg.source_field_offset_bytes
          << " slot_size=" << va_arg.source_slot_size_bytes
          << " rsa_slot#" << va_arg.register_save_area_slot_id
          << " rsa_stack+" << va_arg.register_save_area_stack_offset_bytes
          << " rsa_size=" << va_arg.register_save_area_size_bytes
          << " rsa_align=" << va_arg.register_save_area_align_bytes
          << " gp_base=" << va_arg.register_save_area_gp_offset_bytes
          << " fp_base=" << va_arg.register_save_area_fp_offset_bytes
          << " gp_slot=" << va_arg.register_save_area_gp_slot_size_bytes
          << " fp_slot=" << va_arg.register_save_area_fp_slot_size_bytes;
      lines.push_back(out.str());
    }
    {
      std::ostringstream out;
      out << "va.arg.scalar.progress field="
          << prepare::prepared_variadic_va_list_field_kind_name(
                 va_arg.progression_field)
          << " field_offset=" << va_arg.progression_field_offset_bytes
          << " stride=" << va_arg.progression_stride_bytes
          << " overflow_field="
          << prepare::prepared_variadic_va_list_field_kind_name(
                 va_arg.overflow_source_field)
          << " overflow_field_offset=" << va_arg.overflow_source_field_offset_bytes
          << " overflow_stride=" << va_arg.overflow_stride_bytes
          << " overflow_slot#" << va_arg.overflow_area_base_slot_id
          << " overflow_stack+" << va_arg.overflow_area_base_stack_offset_bytes
          << " overflow_align=" << va_arg.overflow_area_align_bytes;
      lines.push_back(out.str());
    }
    return target_printed(std::move(lines));
  }

  if (call.variadic_entry_helper ==
      std::optional<prepare::PreparedVariadicEntryHelperKind>{
          prepare::PreparedVariadicEntryHelperKind::VaArgAggregate}) {
    if (!call.variadic_aggregate_va_arg.has_value() ||
        call.source_variadic_entry == nullptr ||
        call.source_variadic_helper_operand_homes == nullptr) {
      return target_unsupported(
          bad_header(instruction) +
          "aggregate va_arg node is missing structured prepared access-plan provenance");
    }
    const auto mnemonic = required_primary_mnemonic(instruction);
    if (mnemonic.empty()) {
      return target_unsupported(bad_header(instruction) +
                                "aggregate va_arg mnemonic is not printable");
    }

    const auto& va_arg = *call.variadic_aggregate_va_arg;
    std::vector<std::string> lines;
    {
      std::ostringstream out;
      out << mnemonic << " source="
          << prepare::prepared_variadic_aggregate_va_arg_source_class_name(
                 va_arg.source_class)
          << " va_list=" << prepared_value_home_name(va_arg.source_va_list)
          << " destination_payload="
          << prepared_value_home_name(va_arg.destination_payload_home)
          << " payload_size=" << va_arg.payload_size_bytes
          << " payload_align=" << va_arg.payload_align_bytes
          << " copy_size=" << va_arg.copy_size_bytes
          << " copy_align=" << va_arg.copy_align_bytes
          << " scratch_registers=" << va_arg.scratch_register_count
          << " scratch_stack=" << va_arg.scratch_stack_bytes;
      lines.push_back(out.str());
    }
    {
      std::ostringstream out;
      out << "va.arg.aggregate.source field="
          << prepare::prepared_variadic_va_list_field_kind_name(va_arg.source_field)
          << " field_offset=" << va_arg.source_field_offset_bytes
          << " payload_offset=" << va_arg.source_payload_offset_bytes
          << " slot_size=" << va_arg.source_slot_size_bytes
          << " rsa_slot#" << va_arg.register_save_area_slot_id
          << " rsa_stack+" << va_arg.register_save_area_stack_offset_bytes
          << " rsa_size=" << va_arg.register_save_area_size_bytes
          << " rsa_align=" << va_arg.register_save_area_align_bytes
          << " gp_base=" << va_arg.register_save_area_gp_offset_bytes
          << " fp_base=" << va_arg.register_save_area_fp_offset_bytes
          << " gp_slot=" << va_arg.register_save_area_gp_slot_size_bytes
          << " fp_slot=" << va_arg.register_save_area_fp_slot_size_bytes;
      lines.push_back(out.str());
    }
    {
      std::ostringstream out;
      out << "va.arg.aggregate.progress field="
          << prepare::prepared_variadic_va_list_field_kind_name(
                 va_arg.progression_field)
          << " field_offset=" << va_arg.progression_field_offset_bytes
          << " stride=" << va_arg.progression_stride_bytes
          << " overflow_slot#" << va_arg.overflow_area_base_slot_id
          << " overflow_stack+" << va_arg.overflow_area_base_stack_offset_bytes
          << " overflow_align=" << va_arg.overflow_area_align_bytes;
      lines.push_back(out.str());
    }
    return target_printed(std::move(lines));
  }

  if (call.variadic_entry_helper ==
      std::optional<prepare::PreparedVariadicEntryHelperKind>{
          prepare::PreparedVariadicEntryHelperKind::VaCopy}) {
    if (!call.variadic_va_copy.has_value() ||
        call.source_variadic_entry == nullptr ||
        call.source_variadic_helper_operand_homes == nullptr) {
      return target_unsupported(
          bad_header(instruction) +
          "va_copy node is missing structured prepared va_copy provenance");
    }
    const auto mnemonic = required_primary_mnemonic(instruction);
    if (mnemonic.empty()) {
      return target_unsupported(bad_header(instruction) +
                                "va_copy mnemonic is not printable");
    }

    const auto& va_copy = *call.variadic_va_copy;
    std::vector<std::string> lines;
    {
      std::ostringstream out;
      out << mnemonic << " dest="
          << prepared_value_home_name(va_copy.destination_va_list)
          << " source=" << prepared_value_home_name(va_copy.source_va_list)
          << " va_list_size=" << va_copy.va_list_size_bytes
          << " va_list_align=" << va_copy.va_list_align_bytes
          << " scratch_registers=" << va_copy.scratch_register_count
          << " scratch_stack=" << va_copy.scratch_stack_bytes;
      lines.push_back(out.str());
    }
    for (const auto& field : va_copy.field_copies) {
      std::ostringstream out;
      out << "va.copy.field kind="
          << prepare::prepared_variadic_va_list_field_kind_name(field.kind)
          << " source_offset=" << field.source_offset_bytes
          << " destination_offset=" << field.destination_offset_bytes
          << " size=" << field.size_bytes;
      lines.push_back(out.str());
    }
    return target_printed(std::move(lines));
  }

  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) + "call mnemonic is not printable");
  }
  if (call.is_indirect) {
    if (!call.indirect_callee.has_value() ||
        !call.prepared_indirect_callee.has_value() || call.source_call == nullptr) {
      return target_unsupported(bad_header(instruction) +
                                "indirect call node is missing prepared callee provenance");
    }
    const auto* callee = std::get_if<RegisterOperand>(&call.indirect_callee->payload);
    if (call.indirect_callee->kind != OperandKind::Register || callee == nullptr) {
      return target_unsupported(bad_header(instruction) +
                                "indirect call callee is not a register operand");
    }
    std::ostringstream out;
    out << mnemonic << " " << register_name(*callee);
    return target_printed({out.str()});
  }
  if (!call.direct_callee.has_value() || call.direct_callee_label.empty() ||
      call.source_call == nullptr || !call.wrapper_kind.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "direct call node is missing prepared callee provenance");
  }

  std::ostringstream out;
  out << mnemonic << " " << call.direct_callee_label;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_frame(const InstructionRecord& instruction,
                                              const FrameInstructionRecord& frame) {
  if (frame.source_frame == nullptr) {
    return target_unsupported(bad_header(instruction) +
                              "frame node is missing prepared frame provenance");
  }
  if (frame.function_name == c4c::kInvalidFunctionName) {
    return target_unsupported(bad_header(instruction) +
                              "frame node is missing function identity");
  }
  if (frame.frame_alignment_bytes == 0) {
    return target_unsupported(bad_header(instruction) +
                              "frame node is missing prepared frame alignment");
  }
  if (frame.has_dynamic_stack) {
    return target_unsupported(bad_header(instruction) +
                              "dynamic-stack frame node is outside the printable subset");
  }
  if (!frame.saved_callee_registers.empty() || frame.callee_save.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "callee-save frame node is outside the printable subset");
  }

  if (frame.frame_kind != FrameInstructionKind::PrologueSetup &&
      frame.frame_kind != FrameInstructionKind::EpilogueTeardown) {
    return target_unsupported(bad_header(instruction) +
                              "frame node kind is outside the printable subset");
  }
  if (frame.frame_size_bytes == 0) {
    return target_printed({});
  }
  if (frame.frame_size_bytes > 4095) {
    return target_unsupported(bad_header(instruction) +
                              "frame adjustment immediate is outside the plain #imm "
                              "encoding range 0..4095");
  }

  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) + "frame mnemonic is not printable");
  }

  std::ostringstream out;
  out << mnemonic << " sp, sp, #" << frame.frame_size_bytes;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_call_boundary_move(
    const InstructionRecord& instruction,
    const CallBoundaryMoveInstructionRecord& move) {
  if (move.source_bundle == nullptr || move.source_move == nullptr) {
    return target_unsupported(bad_header(instruction) +
                              "call-boundary move node is missing prepared move provenance");
  }
  if (!move.source_register.has_value() || !move.destination_register.has_value()) {
    return target_unsupported(
        bad_header(instruction) +
        "call-boundary move node requires prepared register source and destination");
  }
  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "call-boundary move mnemonic is not printable");
  }
  std::ostringstream out;
  out << mnemonic << " " << register_name(*move.destination_register) << ", "
      << register_name(*move.source_register);
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_call_boundary_abi_binding(
    const InstructionRecord& instruction,
    const CallBoundaryAbiBindingInstructionRecord& binding) {
  if (binding.source_bundle == nullptr || binding.source_binding == nullptr) {
    return target_unsupported(
        bad_header(instruction) +
        "call-boundary ABI binding node is missing prepared binding provenance");
  }
  return target_unsupported(
      bad_header(instruction) +
      "call-boundary ABI binding node requires later AArch64 move lowering");
}

mir::TargetInstructionPrintResult print_scalar_conversion(
    const InstructionRecord& instruction,
    const ScalarInstructionRecord& scalar,
    const ScalarCastRecord& cast) {
  if ((!cast.supported_float_integer_conversion &&
       !cast.supported_float_width_conversion) ||
      cast.operation == ScalarCastOperationKind::Deferred) {
    return target_unsupported(bad_header(instruction) +
                              "scalar conversion node is outside the printable subset");
  }
  const auto* source_register = std::get_if<RegisterOperand>(&cast.source.payload);
  if (cast.source.kind != OperandKind::Register || source_register == nullptr) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar conversion node requires a structured register source operand");
  }

  std::string_view mnemonic;
  std::optional<std::string> result;
  std::optional<std::string> source;
  switch (cast.operation) {
    case ScalarCastOperationKind::FloatExtend:
      mnemonic = "fcvt";
      result = fp_register_name_with_view(*scalar.result_register, abi::RegisterView::D);
      source = fp_register_name_with_view(*source_register, abi::RegisterView::S);
      break;
    case ScalarCastOperationKind::FloatTruncate:
      mnemonic = "fcvt";
      result = fp_register_name_with_view(*scalar.result_register, abi::RegisterView::S);
      source = fp_register_name_with_view(*source_register, abi::RegisterView::D);
      break;
    case ScalarCastOperationKind::SignedIntToFloat:
    case ScalarCastOperationKind::UnsignedIntToFloat: {
      const auto result_view = floating_register_view(cast.result_type);
      const auto source_bits = integer_type_bit_width(cast.source_type);
      if (!result_view.has_value() || !source_bits.has_value()) {
        return target_unsupported(bad_header(instruction) +
                                  "scalar int-to-float conversion has unsupported type width");
      }
      const auto source_view = integer_register_view(*source_bits);
      if (!source_view.has_value()) {
        return target_unsupported(bad_header(instruction) +
                                  "scalar int-to-float conversion has unsupported integer width");
      }
      mnemonic = cast.operation == ScalarCastOperationKind::SignedIntToFloat ? "scvtf" : "ucvtf";
      result = fp_register_name_with_view(*scalar.result_register, *result_view);
      source = register_name_with_view(*source_register, *source_view);
      break;
    }
    case ScalarCastOperationKind::FloatToSignedInt:
    case ScalarCastOperationKind::FloatToUnsignedInt: {
      const auto result_bits = integer_type_bit_width(cast.result_type);
      const auto source_view = floating_register_view(cast.source_type);
      if (!result_bits.has_value() || !source_view.has_value()) {
        return target_unsupported(bad_header(instruction) +
                                  "scalar float-to-int conversion has unsupported type width");
      }
      const auto result_view = integer_register_view(*result_bits);
      if (!result_view.has_value()) {
        return target_unsupported(bad_header(instruction) +
                                  "scalar float-to-int conversion has unsupported integer width");
      }
      mnemonic = cast.operation == ScalarCastOperationKind::FloatToSignedInt ? "fcvtzs"
                                                                             : "fcvtzu";
      result = register_name_with_view(*scalar.result_register, *result_view);
      source = fp_register_name_with_view(*source_register, *source_view);
      break;
    }
    case ScalarCastOperationKind::SignExtend:
    case ScalarCastOperationKind::ZeroExtend:
    case ScalarCastOperationKind::Truncate:
    case ScalarCastOperationKind::Deferred:
      return target_unsupported(bad_header(instruction) +
                                "scalar conversion node is outside the printable subset");
  }
  if (mnemonic.empty() || !result.has_value() || !source.has_value()) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar conversion node has incomplete printable register facts");
  }
  std::ostringstream out;
  out << mnemonic << " " << *result << ", " << *source;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_scalar(const InstructionRecord& instruction,
                                               const ScalarInstructionRecord& scalar) {
  if (!scalar.result_register.has_value()) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar node is missing a structured destination register operand");
  }
  if (scalar.scalar_cast.has_value()) {
    const auto& cast = *scalar.scalar_cast;
    if (cast.supported_float_integer_conversion || cast.supported_float_width_conversion) {
      return print_scalar_conversion(instruction, scalar, cast);
    }
    if (!cast.supported_simple_integer_cast ||
        cast.operation == ScalarCastOperationKind::Deferred) {
      return target_unsupported(bad_header(instruction) +
                                "scalar cast node is outside the printable simple integer subset");
    }
    const auto source_bits = integer_type_bit_width(cast.source_type);
    const auto result_bits = integer_type_bit_width(cast.result_type);
    if (!source_bits.has_value() || !result_bits.has_value() ||
        ((*source_bits >= *result_bits) &&
         cast.operation != ScalarCastOperationKind::Truncate) ||
        ((*source_bits <= *result_bits) &&
         cast.operation == ScalarCastOperationKind::Truncate)) {
      return target_unsupported(bad_header(instruction) +
                                "scalar cast node requires a supported integer source/result width");
    }
    const auto* source_register = std::get_if<RegisterOperand>(&cast.source.payload);
    if (cast.source.kind != OperandKind::Register || source_register == nullptr) {
      return target_unsupported(bad_header(instruction) +
                                "scalar cast node requires a structured register source operand");
    }
    const auto result_view = integer_register_view(*result_bits);
    if (!result_view.has_value()) {
      return target_unsupported(bad_header(instruction) +
                                "scalar cast node has an unsupported result register width");
    }
    const auto result = register_name_with_view(*scalar.result_register, *result_view);
    if (!result.has_value()) {
      return target_unsupported(bad_header(instruction) +
                                "scalar cast node destination is not a printable GPR register");
    }

    std::ostringstream out;
    switch (cast.operation) {
      case ScalarCastOperationKind::SignExtend: {
        if (*source_bits == 1U) {
          const auto source = register_name_with_view(
              *source_register,
              *result_bits <= 32U ? abi::RegisterView::W : abi::RegisterView::X);
          if (!source.has_value()) {
            return target_unsupported(
                bad_header(instruction) +
                "scalar sign-extend node source is not a printable GPR register");
          }
          out << "sbfx " << *result << ", " << *source << ", #0, #1";
        } else {
          std::string_view mnemonic;
          if (*source_bits == 8U) {
            mnemonic = "sxtb";
          } else if (*source_bits == 16U) {
            mnemonic = "sxth";
          } else if (*source_bits == 32U && *result_bits == 64U) {
            mnemonic = "sxtw";
          } else {
            return target_unsupported(bad_header(instruction) +
                                      "scalar sign-extend node has no printable width form");
          }
          const auto source = register_name_with_view(*source_register, abi::RegisterView::W);
          if (!source.has_value()) {
            return target_unsupported(
                bad_header(instruction) +
                "scalar sign-extend node source is not a printable GPR register");
          }
          out << mnemonic << " " << *result << ", " << *source;
        }
        return target_printed({out.str()});
      }
      case ScalarCastOperationKind::ZeroExtend: {
        const auto source = register_name_with_view(
            *source_register,
            *result_bits <= 32U ? abi::RegisterView::W : abi::RegisterView::X);
        if (!source.has_value()) {
          return target_unsupported(bad_header(instruction) +
                                    "scalar zero-extend node source is not a printable GPR register");
        }
        out << "ubfx " << *result << ", " << *source << ", #0, #" << *source_bits;
        return target_printed({out.str()});
      }
      case ScalarCastOperationKind::Truncate: {
        if (*result_bits > 32U) {
          return target_unsupported(bad_header(instruction) +
                                    "scalar truncate node has no printable width form");
        }
        const auto source = register_name_with_view(*source_register, abi::RegisterView::W);
        if (!source.has_value()) {
          return target_unsupported(bad_header(instruction) +
                                    "scalar truncate node source is not a printable GPR register");
        }
        if (*result_bits == 32U) {
          out << "mov " << *result << ", " << *source;
        } else {
          const std::uint64_t mask = (std::uint64_t{1} << *result_bits) - 1U;
          out << "and " << *result << ", " << *source << ", #" << mask;
        }
        return target_printed({out.str()});
      }
      case ScalarCastOperationKind::Deferred:
        break;
    }
    return target_unsupported(bad_header(instruction) +
                              "scalar cast node is outside the printable simple integer subset");
  }
  if (scalar.scalar_alu.has_value() && scalar.scalar_alu->supported_floating_operation) {
    const auto& alu = *scalar.scalar_alu;
    if (scalar.inputs.size() != 2) {
      return target_unsupported(
          bad_header(instruction) +
          "scalar FP node requires exactly two structured register operands");
    }
    const auto result_view = floating_register_view(alu.result_type);
    const auto operand_view = floating_register_view(alu.operand_type);
    if (!result_view.has_value() || !operand_view.has_value() ||
        result_view != operand_view) {
      return target_unsupported(bad_header(instruction) +
                                "scalar FP node requires matching F32/F64 operand and result widths");
    }
    const auto* lhs_register = std::get_if<RegisterOperand>(&scalar.inputs[0].payload);
    const auto* rhs_register = std::get_if<RegisterOperand>(&scalar.inputs[1].payload);
    if (scalar.inputs[0].kind != OperandKind::Register ||
        scalar.inputs[1].kind != OperandKind::Register ||
        lhs_register == nullptr || rhs_register == nullptr) {
      return target_unsupported(
          bad_header(instruction) +
          "scalar FP node requires structured FP/SIMD register operands");
    }
    const auto mnemonic = floating_alu_mnemonic(alu.operation);
    const auto result = fp_register_name_with_view(*scalar.result_register, *result_view);
    const auto lhs = fp_register_name_with_view(*lhs_register, *operand_view);
    const auto rhs = fp_register_name_with_view(*rhs_register, *operand_view);
    if (mnemonic.empty() || !result.has_value() || !lhs.has_value() || !rhs.has_value()) {
      return target_unsupported(bad_header(instruction) +
                                "scalar FP node has incomplete printable register facts");
    }
    std::ostringstream out;
    out << mnemonic << " " << *result << ", " << *lhs << ", " << *rhs;
    return target_printed({out.str()});
  }
  if (instruction.opcode != MachineOpcode::Add && instruction.opcode != MachineOpcode::Sub) {
    return target_unsupported(bad_header(instruction) +
                              "scalar node opcode is outside the printable add/sub subset");
  }
  if (scalar.inputs.size() != 2) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar add/sub node requires exactly two register or immediate operands");
  }

  const auto* lhs_register = std::get_if<RegisterOperand>(&scalar.inputs[0].payload);
  const auto* rhs_register = std::get_if<RegisterOperand>(&scalar.inputs[1].payload);
  const auto* lhs_immediate = std::get_if<ImmediateOperand>(&scalar.inputs[0].payload);
  const auto* rhs_immediate = std::get_if<ImmediateOperand>(&scalar.inputs[1].payload);
  const bool lhs_is_register = scalar.inputs[0].kind == OperandKind::Register &&
                               lhs_register != nullptr;
  const bool rhs_is_register = scalar.inputs[1].kind == OperandKind::Register &&
                               rhs_register != nullptr;
  const bool lhs_is_immediate = scalar.inputs[0].kind == OperandKind::Immediate &&
                                lhs_immediate != nullptr;
  const bool rhs_is_immediate = scalar.inputs[1].kind == OperandKind::Immediate &&
                                rhs_immediate != nullptr;
  if ((!lhs_is_register && !lhs_is_immediate) || (!rhs_is_register && !rhs_is_immediate)) {
    return target_unsupported(bad_header(instruction) +
                              "scalar add/sub node requires register or immediate operands");
  }
  if ((lhs_is_immediate && !is_plain_add_sub_immediate(*lhs_immediate)) ||
      (rhs_is_immediate && !is_plain_add_sub_immediate(*rhs_immediate))) {
    return target_unsupported(bad_header(instruction) +
                              "scalar add/sub immediate operand is outside the plain #imm "
                              "encoding range 0..4095");
  }

  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "scalar add/sub mnemonic is not printable");
  }

  std::vector<std::string> lines;
  const auto result = register_name(*scalar.result_register);
  if (lhs_is_register && rhs_is_register) {
    std::ostringstream out;
    out << mnemonic << " " << result << ", " << register_name(*lhs_register) << ", "
        << register_name(*rhs_register);
    lines.push_back(out.str());
  } else if (lhs_is_register && rhs_is_immediate) {
    std::ostringstream out;
    out << mnemonic << " " << result << ", " << register_name(*lhs_register) << ", "
        << immediate_name(*rhs_immediate);
    lines.push_back(out.str());
  } else if (lhs_is_immediate && rhs_is_register && instruction.opcode == MachineOpcode::Add) {
    std::ostringstream out;
    out << mnemonic << " " << result << ", " << register_name(*rhs_register) << ", "
        << immediate_name(*lhs_immediate);
    lines.push_back(out.str());
  } else if (lhs_is_immediate && rhs_is_immediate) {
    const auto move_mnemonic =
        machine_printer_mnemonic_kind_name(MachinePrinterMnemonicKind::Move);
    std::ostringstream move_line;
    move_line << move_mnemonic << " " << result << ", " << immediate_name(*lhs_immediate);
    lines.push_back(move_line.str());
    std::ostringstream add_line;
    add_line << mnemonic << " " << result << ", " << result << ", "
             << immediate_name(*rhs_immediate);
    lines.push_back(add_line.str());
  } else {
    return target_unsupported(
        bad_header(instruction) +
        "scalar sub with an immediate lhs and register rhs is not printable");
  }
  return target_printed(std::move(lines));
}

mir::TargetInstructionPrintResult print_scalar_fp_unary_intrinsic(
    const InstructionRecord& instruction,
    const ScalarFpUnaryIntrinsicRecord& intrinsic) {
  if (instruction.family != InstructionFamily::Intrinsic ||
      instruction.opcode != MachineOpcode::ScalarFpUnaryIntrinsic) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer requires an intrinsic machine opcode");
  }
  if (intrinsic.source_carrier == nullptr ||
      intrinsic.source_carrier->carrier_kind !=
          prepare::PreparedIntrinsicCarrierKind::Complete) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer requires complete prepared carrier provenance");
  }
  if (intrinsic.family != bir::IntrinsicFamilyKind::ScalarFpUnary ||
      intrinsic.operation != bir::IntrinsicOperationKind::FAbs) {
    return target_unsupported(
        bad_header(instruction) +
        "intrinsic operation is outside the printable scalar FP unary subset");
  }
  if ((intrinsic.operand_type != bir::TypeKind::F32 &&
       intrinsic.operand_type != bir::TypeKind::F64) ||
      intrinsic.operand_type != intrinsic.result_type) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer requires matching F32/F64 operand and result types");
  }
  if (!intrinsic.has_prepared_call_plan) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer requires prepared call-plan authority");
  }
  if (intrinsic.has_side_effects || intrinsic.requires_feature) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer only supports side-effect-free feature-free fabs");
  }
  const auto* operand_register = std::get_if<RegisterOperand>(&intrinsic.operand.payload);
  if (!intrinsic.result_register.has_value() ||
      intrinsic.operand.kind != OperandKind::Register ||
      operand_register == nullptr) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer requires explicit operand and result registers");
  }
  const auto view = floating_register_view(intrinsic.operand_type);
  if (!view.has_value()) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer has no register view for operand type");
  }
  const auto result = fp_register_name_with_view(*intrinsic.result_register, *view);
  const auto operand = fp_register_name_with_view(*operand_register, *view);
  if (!result.has_value() || !operand.has_value()) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer has incomplete printable FPR register facts");
  }
  std::ostringstream out;
  out << "fabs " << *result << ", " << *operand;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_crc32w_intrinsic(
    const InstructionRecord& instruction,
    const Crc32WIntrinsicRecord& intrinsic) {
  if (instruction.family != InstructionFamily::Intrinsic ||
      instruction.opcode != MachineOpcode::Crc32WIntrinsic) {
    return target_unsupported(bad_header(instruction) +
                              "CRC32W intrinsic printer requires a CRC32W machine opcode");
  }
  if (!has_complete_intrinsic_source(intrinsic.source_carrier)) {
    return target_unsupported(bad_header(instruction) +
                              "CRC32W intrinsic printer requires complete prepared carrier provenance");
  }
  if (intrinsic.family != bir::IntrinsicFamilyKind::Crc ||
      intrinsic.operation != bir::IntrinsicOperationKind::Crc32W ||
      intrinsic.required_feature != bir::IntrinsicFeatureKind::AArch64Crc ||
      !intrinsic.requires_feature ||
      intrinsic.operand_type != bir::TypeKind::I32 ||
      intrinsic.result_type != bir::TypeKind::I32 ||
      intrinsic.signedness != bir::IntrinsicSignedness::Unsigned ||
      !has_exact_intrinsic_roles(intrinsic.operand_roles,
                                 {bir::IntrinsicOperandRole::Accumulator,
                                  bir::IntrinsicOperandRole::Data})) {
    return target_unsupported(bad_header(instruction) +
                              "CRC32W intrinsic printer requires selected CRC32W carrier facts");
  }
  if (!intrinsic.has_prepared_call_plan || !intrinsic.result_register.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "CRC32W intrinsic printer requires prepared register authority");
  }
  const auto* accumulator = std::get_if<RegisterOperand>(&intrinsic.accumulator.payload);
  const auto* data = std::get_if<RegisterOperand>(&intrinsic.data.payload);
  if (intrinsic.accumulator.kind != OperandKind::Register ||
      intrinsic.data.kind != OperandKind::Register ||
      accumulator == nullptr || data == nullptr) {
    return target_unsupported(bad_header(instruction) +
                              "CRC32W intrinsic printer requires explicit operand registers");
  }
  const auto result_name =
      register_name_with_view(*intrinsic.result_register, abi::RegisterView::W);
  const auto accumulator_name = register_name_with_view(*accumulator, abi::RegisterView::W);
  const auto data_name = register_name_with_view(*data, abi::RegisterView::W);
  if (!result_name.has_value() || !accumulator_name.has_value() ||
      !data_name.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "CRC32W intrinsic printer has incomplete printable W-register facts");
  }
  std::ostringstream out;
  out << "crc32w " << *result_name << ", " << *accumulator_name << ", " << *data_name;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_vector_load_intrinsic(
    const InstructionRecord& instruction,
    const VectorLoadIntrinsicRecord& intrinsic) {
  if (instruction.family != InstructionFamily::Intrinsic ||
      instruction.opcode != MachineOpcode::VectorLoadIntrinsic) {
    return target_unsupported(bad_header(instruction) +
                              "vector-load intrinsic printer requires a vector-load machine opcode");
  }
  if (!has_complete_intrinsic_source(intrinsic.source_carrier)) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-load intrinsic printer requires complete prepared carrier provenance");
  }
  if (intrinsic.family != bir::IntrinsicFamilyKind::VectorMemory ||
      intrinsic.operation != bir::IntrinsicOperationKind::VectorLoad ||
      intrinsic.required_feature != bir::IntrinsicFeatureKind::AArch64Neon ||
      !intrinsic.requires_feature ||
      intrinsic.operand_type != bir::TypeKind::Ptr ||
      intrinsic.result_type != bir::TypeKind::I128 ||
      intrinsic.memory_access != bir::IntrinsicMemoryAccessKind::Read ||
      !has_exact_intrinsic_roles(intrinsic.operand_roles,
                                 {bir::IntrinsicOperandRole::Pointer}) ||
      !has_v16i8_unsigned_shape(intrinsic.vector_element_type,
                                intrinsic.vector_element_width_bytes,
                                intrinsic.vector_lane_count,
                                intrinsic.vector_total_width_bytes,
                                intrinsic.signedness)) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-load intrinsic printer requires selected v16i8 load carrier facts");
  }
  const auto* pointer = std::get_if<RegisterOperand>(&intrinsic.pointer.payload);
  if (!intrinsic.has_prepared_call_plan || !intrinsic.result_register.has_value() ||
      intrinsic.pointer.kind != OperandKind::Register || pointer == nullptr ||
      intrinsic.memory.base_kind != MemoryBaseKind::PointerValue ||
      !intrinsic.memory.base_register.has_value() ||
      intrinsic.memory.byte_offset != 0 ||
      intrinsic.memory.size_bytes != intrinsic.vector_total_width_bytes ||
      intrinsic.memory.address_space != bir::AddressSpace::Default) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-load intrinsic printer requires pointer memory and result register authority");
  }
  const auto result_name = f128_vector_register_name(*intrinsic.result_register);
  const auto pointer_name = register_name_with_view(*pointer, abi::RegisterView::X);
  const auto memory_base_name =
      register_name_with_view(*intrinsic.memory.base_register, abi::RegisterView::X);
  if (!result_name.has_value() || !pointer_name.has_value() ||
      !memory_base_name.has_value() || *pointer_name != *memory_base_name) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-load intrinsic printer has incomplete printable vector or pointer register facts");
  }
  std::ostringstream out;
  out << "ld1 {" << *result_name << "}, [" << *memory_base_name << "]";
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_vector_add_intrinsic(
    const InstructionRecord& instruction,
    const VectorAddIntrinsicRecord& intrinsic) {
  if (instruction.family != InstructionFamily::Intrinsic ||
      instruction.opcode != MachineOpcode::VectorAddIntrinsic) {
    return target_unsupported(bad_header(instruction) +
                              "vector-add intrinsic printer requires a vector-add machine opcode");
  }
  if (!has_complete_intrinsic_source(intrinsic.source_carrier)) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-add intrinsic printer requires complete prepared carrier provenance");
  }
  if (intrinsic.family != bir::IntrinsicFamilyKind::VectorOperation ||
      intrinsic.operation != bir::IntrinsicOperationKind::VectorAdd ||
      intrinsic.required_feature != bir::IntrinsicFeatureKind::AArch64Neon ||
      !intrinsic.requires_feature ||
      intrinsic.operand_type != bir::TypeKind::I128 ||
      intrinsic.result_type != bir::TypeKind::I128 ||
      intrinsic.memory_access != bir::IntrinsicMemoryAccessKind::None ||
      !has_exact_intrinsic_roles(intrinsic.operand_roles,
                                 {bir::IntrinsicOperandRole::VectorLhs,
                                  bir::IntrinsicOperandRole::VectorRhs}) ||
      !has_v16i8_unsigned_shape(intrinsic.vector_element_type,
                                intrinsic.vector_element_width_bytes,
                                intrinsic.vector_lane_count,
                                intrinsic.vector_total_width_bytes,
                                intrinsic.signedness)) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-add intrinsic printer requires selected v16i8 add carrier facts");
  }
  const auto* lhs = std::get_if<RegisterOperand>(&intrinsic.lhs.payload);
  const auto* rhs = std::get_if<RegisterOperand>(&intrinsic.rhs.payload);
  if (!intrinsic.has_prepared_call_plan || !intrinsic.result_register.has_value() ||
      intrinsic.lhs.kind != OperandKind::Register ||
      intrinsic.rhs.kind != OperandKind::Register ||
      lhs == nullptr || rhs == nullptr) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-add intrinsic printer requires explicit operand and result registers");
  }
  const auto result_name = f128_vector_register_name(*intrinsic.result_register);
  const auto lhs_name = f128_vector_register_name(*lhs);
  const auto rhs_name = f128_vector_register_name(*rhs);
  if (!result_name.has_value() || !lhs_name.has_value() || !rhs_name.has_value()) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-add intrinsic printer has incomplete printable vector register facts");
  }
  std::ostringstream out;
  out << "add " << *result_name << ", " << *lhs_name << ", " << *rhs_name;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_return(const InstructionRecord& instruction,
                                               const ReturnInstructionRecord& ret) {
  std::vector<std::string> lines;
  if (ret.value.has_value()) {
    const auto* immediate = std::get_if<ImmediateOperand>(&ret.value->payload);
    const auto* reg = std::get_if<RegisterOperand>(&ret.value->payload);
    if (ret.value->kind == OperandKind::Register && reg != nullptr) {
      const auto return_mnemonic = required_primary_mnemonic(instruction);
      if (return_mnemonic.empty()) {
        return target_unsupported(bad_header(instruction) + "return mnemonic is not printable");
      }
      return target_printed({std::string(return_mnemonic)});
    }
    if (ret.value->kind != OperandKind::Immediate || immediate == nullptr) {
      return target_unsupported(bad_header(instruction) +
                                "return value is not a printable immediate operand");
    }
    if (immediate->kind != ImmediateKind::SignedInteger ||
        immediate->signed_value < 0 || immediate->signed_value > 65535) {
      return target_unsupported(bad_header(instruction) +
                                "return immediate is outside the selected printable subset");
    }

    const char* result_register = nullptr;
    switch (ret.value_type) {
      case bir::TypeKind::I1:
      case bir::TypeKind::I8:
      case bir::TypeKind::I16:
      case bir::TypeKind::I32:
        result_register = "w0";
        break;
      case bir::TypeKind::I64:
        result_register = "x0";
        break;
      default:
        return target_unsupported(bad_header(instruction) +
                                  "return type is outside the selected printable subset");
    }
    const auto move_mnemonic = required_auxiliary_mnemonic(instruction);
    if (move_mnemonic.empty()) {
      return target_unsupported(bad_header(instruction) +
                                "return move mnemonic is not printable");
    }
    std::ostringstream move_line;
    move_line << move_mnemonic << " " << result_register << ", #" << immediate->signed_value;
    lines.push_back(move_line.str());
  }
  const auto return_mnemonic = required_primary_mnemonic(instruction);
  if (return_mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) + "return mnemonic is not printable");
  }
  lines.emplace_back(return_mnemonic);
  return target_printed(std::move(lines));
}

}  // namespace

mir::TargetInstructionPrintResult MachineInstructionPrinter::print_instruction(
    const mir::MachinePrintContext&,
    const mir::MachineInstruction<InstructionRecord>& instruction) const {
  return print_machine_instruction_line_payloads(instruction.target);
}

mir::TargetInstructionPrintResult print_machine_instruction_line_payloads(
    const InstructionRecord& instruction) {
  if (const auto invalid = validate_selected_machine_node(instruction); invalid.has_value()) {
    return target_unsupported(*invalid);
  }

  if (const auto* spill_reload =
          std::get_if<SpillReloadInstructionRecord>(&instruction.payload)) {
    return print_spill_reload(instruction, *spill_reload);
  }
  if (const auto* branch = std::get_if<BranchInstructionRecord>(&instruction.payload)) {
    return print_branch(instruction, *branch);
  }
  if (const auto* memory = std::get_if<MemoryInstructionRecord>(&instruction.payload)) {
    return print_memory(instruction, *memory);
  }
  if (const auto* atomic =
          std::get_if<AtomicMemoryInstructionRecord>(&instruction.payload)) {
    return print_atomic_memory(instruction, *atomic);
  }
  if (const auto* frame = std::get_if<FrameInstructionRecord>(&instruction.payload)) {
    return print_frame(instruction, *frame);
  }
  if (const auto* move =
          std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.payload)) {
    return print_call_boundary_move(instruction, *move);
  }
  if (const auto* binding =
          std::get_if<CallBoundaryAbiBindingInstructionRecord>(&instruction.payload)) {
    return print_call_boundary_abi_binding(instruction, *binding);
  }
  if (const auto* call = std::get_if<CallInstructionRecord>(&instruction.payload)) {
    return print_call(instruction, *call);
  }
  if (const auto* address = std::get_if<AddressMaterializationRecord>(&instruction.payload)) {
    return print_address_materialization(instruction, *address);
  }
  if (const auto* transport = std::get_if<I128TransportRecord>(&instruction.payload)) {
    return print_i128_transport(instruction, *transport);
  }
  if (const auto* transport = std::get_if<F128TransportRecord>(&instruction.payload)) {
    return print_f128_transport(instruction, *transport);
  }
  if (const auto* pair = std::get_if<I128PairOperationRecord>(&instruction.payload)) {
    return print_i128_pair_operation(instruction, *pair);
  }
  if (const auto* shift = std::get_if<I128ShiftRecord>(&instruction.payload)) {
    return print_i128_shift(instruction, *shift);
  }
  if (const auto* compare = std::get_if<I128CompareRecord>(&instruction.payload)) {
    return print_i128_compare(instruction, *compare);
  }
  if (const auto* helper =
          std::get_if<I128RuntimeHelperBoundaryRecord>(&instruction.payload)) {
    return print_i128_runtime_helper(instruction, *helper);
  }
  if (const auto* helper =
          std::get_if<F128RuntimeHelperBoundaryRecord>(&instruction.payload)) {
    return print_f128_runtime_helper(instruction, *helper);
  }
  if (const auto* scalar = std::get_if<ScalarInstructionRecord>(&instruction.payload)) {
    return print_scalar(instruction, *scalar);
  }
  if (const auto* intrinsic =
          std::get_if<ScalarFpUnaryIntrinsicRecord>(&instruction.payload)) {
    return print_scalar_fp_unary_intrinsic(instruction, *intrinsic);
  }
  if (const auto* intrinsic = std::get_if<Crc32WIntrinsicRecord>(&instruction.payload)) {
    return print_crc32w_intrinsic(instruction, *intrinsic);
  }
  if (const auto* intrinsic = std::get_if<VectorLoadIntrinsicRecord>(&instruction.payload)) {
    return print_vector_load_intrinsic(instruction, *intrinsic);
  }
  if (const auto* intrinsic = std::get_if<VectorAddIntrinsicRecord>(&instruction.payload)) {
    return print_vector_add_intrinsic(instruction, *intrinsic);
  }
  if (const auto* assembler = std::get_if<AssemblerInstructionRecord>(&instruction.payload)) {
    return print_assembler(instruction, *assembler);
  }
  if (const auto* ret = std::get_if<ReturnInstructionRecord>(&instruction.payload)) {
    return print_return(instruction, *ret);
  }
  return target_unsupported(bad_header(instruction) +
                            "instruction family is not in the printable subset");
}

}  // namespace c4c::backend::aarch64::codegen
