#include "dispatch.hpp"

#include "../abi/abi.hpp"
#include "alu.hpp"
#include "atomics.hpp"
#include "calls.hpp"
#include "cast_ops.hpp"
#include "comparison.hpp"
#include "f128.hpp"
#include "float_ops.hpp"
#include "globals.hpp"
#include "i128_ops.hpp"
#include "inline_asm.hpp"
#include "intrinsics.hpp"
#include "memory.hpp"
#include "returns.hpp"
#include "variadic.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace c4c::backend::aarch64::codegen {
namespace {

namespace abi = c4c::backend::aarch64::abi;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

[[nodiscard]] bool registers_alias(const RegisterOperand& lhs,
                                   const RegisterOperand& rhs) {
  return lhs.reg.bank == rhs.reg.bank && lhs.reg.index == rhs.reg.index;
}

void append_block_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
                             module::ModuleLoweringDiagnosticKind kind,
                             const module::BlockLoweringContext& context,
                             std::string message) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = kind,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .message = std::move(message),
  });
}

[[nodiscard]] std::string unsupported_terminator_message(
    c4c::backend::bir::TerminatorKind kind) {
  switch (kind) {
    case c4c::backend::bir::TerminatorKind::Return:
      return "AArch64 block dispatch visited unsupported prepared return terminator";
    case c4c::backend::bir::TerminatorKind::Branch:
      return "AArch64 block dispatch visited prepared branch terminator; semantic lowering is not implemented";
    case c4c::backend::bir::TerminatorKind::CondBranch:
      return "AArch64 block dispatch visited prepared conditional branch terminator; semantic lowering is not implemented";
  }
  return "AArch64 block dispatch visited unsupported prepared terminator";
}

[[nodiscard]] const bir::Block* find_bir_block(
    const module::FunctionLoweringContext& function,
    const prepare::PreparedControlFlowBlock& block) {
  if (function.bir_function == nullptr) {
    return nullptr;
  }

  if (function.prepared == nullptr || block.block_label == c4c::kInvalidBlockLabel) {
    return nullptr;
  }

  const std::string_view prepared_block_label =
      prepare::prepared_block_label(function.prepared->names, block.block_label);
  if (prepared_block_label.empty()) {
    return nullptr;
  }

  for (const auto& bir_block : function.bir_function->blocks) {
    if (bir_block.label_id != c4c::kInvalidBlockLabel &&
        function.prepared->module.names.block_labels.spelling(bir_block.label_id) ==
            prepared_block_label) {
      return &bir_block;
    }
    if (std::string_view{bir_block.label} == prepared_block_label) {
      return &bir_block;
    }
  }
  return nullptr;
}

[[nodiscard]] bool binary_result_matches_value(const bir::Inst& inst,
                                               std::string_view value_name) {
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  return binary != nullptr &&
         binary->result.kind == bir::Value::Kind::Named &&
         binary->result.name == value_name;
}

[[nodiscard]] bool binary_uses_named_value(const bir::Inst& inst,
                                           std::string_view value_name) {
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  if (binary == nullptr) {
    return false;
  }
  const auto matches = [&](const bir::Value& value) {
    return value.kind == bir::Value::Kind::Named && value.name == value_name;
  };
  return matches(binary->lhs) || matches(binary->rhs);
}

[[nodiscard]] std::optional<unsigned> integer_bit_width(bir::TypeKind type) {
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
    case bir::TypeKind::Ptr:
      return 64U;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string_view> branch_condition_suffix(
    bir::BinaryOpcode predicate) {
  switch (predicate) {
    case bir::BinaryOpcode::Eq:
      return std::string_view{"eq"};
    case bir::BinaryOpcode::Ne:
      return std::string_view{"ne"};
    case bir::BinaryOpcode::Slt:
      return std::string_view{"lt"};
    case bir::BinaryOpcode::Sle:
      return std::string_view{"le"};
    case bir::BinaryOpcode::Sgt:
      return std::string_view{"gt"};
    case bir::BinaryOpcode::Sge:
      return std::string_view{"ge"};
    case bir::BinaryOpcode::Ult:
      return std::string_view{"lo"};
    case bir::BinaryOpcode::Ule:
      return std::string_view{"ls"};
    case bir::BinaryOpcode::Ugt:
      return std::string_view{"hi"};
    case bir::BinaryOpcode::Uge:
      return std::string_view{"hs"};
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::string machine_block_label(c4c::FunctionNameId function_name,
                                              c4c::BlockLabelId block_label) {
  return ".LBB" + std::to_string(function_name) + "_" + std::to_string(block_label);
}

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

[[nodiscard]] std::optional<RegisterOperand> make_named_prepared_result_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.function.value_locations == nullptr) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  const auto* home =
      prepare::find_prepared_value_home(*context.function.value_locations, *value_name);
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

[[nodiscard]] bool emitted_scalar_value_available(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const BlockScalarLoweringState& scalar_state) {
  const auto value_name = prepared_named_value_id(context, value);
  return value_name.has_value() &&
         find_emitted_scalar_register(scalar_state, *value_name).has_value();
}

[[nodiscard]] bool is_scalar_call_argument_producer_opcode(bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::SRem:
      return true;
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      return false;
  }
  return false;
}

[[nodiscard]] std::optional<std::size_t> find_same_block_scalar_producer(
    const module::BlockLoweringContext& context,
    std::string_view value_name,
    std::size_t before_instruction_index) {
  if (context.bir_block == nullptr) {
    return std::nullopt;
  }
  for (std::size_t index = before_instruction_index; index > 0; --index) {
    const std::size_t candidate_index = index - 1;
    const auto* binary =
        std::get_if<bir::BinaryInst>(&context.bir_block->insts[candidate_index]);
    if (binary == nullptr) {
      continue;
    }
    if (binary->result.kind == bir::Value::Kind::Named &&
        binary->result.name == value_name &&
        is_scalar_call_argument_producer_opcode(binary->opcode)) {
      return candidate_index;
    }
  }
  return std::nullopt;
}

[[nodiscard]] bool materialize_scalar_call_argument_value(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics,
    std::vector<module::MachineInstruction>& lowered,
    std::vector<std::string_view>& active_values) {
  if (value.kind != bir::Value::Kind::Named || value.name.empty() ||
      emitted_scalar_value_available(context, value, scalar_state)) {
    return true;
  }
  for (const auto active : active_values) {
    if (active == value.name) {
      return false;
    }
  }
  const auto producer_index =
      find_same_block_scalar_producer(context, value.name, before_instruction_index);
  if (!producer_index.has_value() || context.bir_block == nullptr) {
    if (auto prepared_register = make_named_prepared_result_register(context, value);
        prepared_register.has_value()) {
      record_emitted_scalar_register(scalar_state,
                                     prepared_register->value_name,
                                     *prepared_register);
    }
    return true;
  }
  const auto* binary = std::get_if<bir::BinaryInst>(&context.bir_block->insts[*producer_index]);
  if (binary == nullptr ||
      !is_scalar_call_argument_producer_opcode(binary->opcode)) {
    return true;
  }

  active_values.push_back(value.name);
  const bool lhs_ready =
      materialize_scalar_call_argument_value(context,
                                             binary->lhs,
                                             *producer_index,
                                             scalar_state,
                                             diagnostics,
                                             lowered,
                                             active_values);
  const bool rhs_ready =
      materialize_scalar_call_argument_value(context,
                                             binary->rhs,
                                             *producer_index,
                                             scalar_state,
                                             diagnostics,
                                             lowered,
                                             active_values);
  active_values.pop_back();
  if (!lhs_ready || !rhs_ready) {
    return false;
  }
  if (emitted_scalar_value_available(context, value, scalar_state)) {
    return true;
  }
  if (auto instruction = lower_scalar_instruction(context,
                                                  context.bir_block->insts[*producer_index],
                                                  *producer_index,
                                                  scalar_state,
                                                  diagnostics)) {
    const auto expected_result =
        make_named_prepared_result_register(context, binary->result);
    if (expected_result.has_value()) {
      if (auto* scalar =
              std::get_if<ScalarInstructionRecord>(&instruction->target.payload)) {
        scalar->result_register = *expected_result;
        if (scalar->scalar_alu.has_value()) {
          scalar->scalar_alu->result_register = *expected_result;
          if (const auto lhs_name = prepared_named_value_id(context, binary->lhs);
              lhs_name.has_value()) {
            if (const auto lhs_register =
                find_emitted_scalar_register(scalar_state, *lhs_name);
                lhs_register.has_value()) {
              auto lhs_operand = make_register_operand(*lhs_register);
              scalar->scalar_alu->lhs = lhs_operand;
              if (!scalar->inputs.empty()) {
                scalar->inputs[0] = std::move(lhs_operand);
              }
            }
          }
          if (const auto rhs_name = prepared_named_value_id(context, binary->rhs);
              rhs_name.has_value()) {
            if (const auto rhs_register =
                    find_emitted_scalar_register(scalar_state, *rhs_name);
                rhs_register.has_value()) {
              auto rhs_operand = make_register_operand(*rhs_register);
              scalar->scalar_alu->rhs = rhs_operand;
              if (scalar->inputs.size() > 1) {
                scalar->inputs[1] = std::move(rhs_operand);
              }
            }
          }
        }
        record_emitted_scalar_register(scalar_state,
                                       expected_result->value_name,
                                       *expected_result);
      }
    }
    lowered.push_back(std::move(*instruction));
    return true;
  }
  return false;
}

[[nodiscard]] std::vector<module::MachineInstruction>
lower_scalar_call_argument_producers(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  for (const auto& argument : call.args) {
    std::vector<std::string_view> active_values;
    if (!materialize_scalar_call_argument_value(context,
                                                argument,
                                                instruction_index,
                                                scalar_state,
                                                diagnostics,
                                                lowered,
                                                active_values)) {
      return {};
    }
  }
  return lowered;
}

[[nodiscard]] bool prepared_edge_select_source_is_destination_register(
    const prepare::PreparedValueHome& source_home,
    const prepare::PreparedValueHome& destination_home) {
  return source_home.kind == prepare::PreparedValueHomeKind::Register &&
         destination_home.kind == prepare::PreparedValueHomeKind::Register &&
         source_home.register_name.has_value() &&
         destination_home.register_name.has_value() &&
         *source_home.register_name == *destination_home.register_name;
}

[[nodiscard]] bool is_current_block_join_parallel_copy_source(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst) {
  if (context.function.value_locations == nullptr ||
      context.control_flow_block == nullptr) {
    return false;
  }
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  if (binary == nullptr ||
      binary->result.kind != bir::Value::Kind::Named ||
      binary->result.name.empty()) {
    return false;
  }
  const auto result_value_name = prepared_named_value_id(context, binary->result);
  if (!result_value_name.has_value()) {
    return false;
  }
  const auto* result_home =
      prepare::find_prepared_value_home(*context.function.value_locations,
                                        *result_value_name);
  if (result_home == nullptr || result_home->value_name == c4c::kInvalidValueName) {
    return false;
  }
  for (const auto& bundle : context.function.value_locations->move_bundles) {
    if (bundle.phase != prepare::PreparedMovePhase::BlockEntry ||
        bundle.authority_kind != prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
        bundle.source_parallel_copy_successor_label !=
            std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label}) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
          move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
          move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
          move.source_immediate_i32.has_value() ||
          move.from_value_id != result_home->value_id ||
          move.from_value_id == move.to_value_id) {
        continue;
      }
      const auto* destination_home =
          prepare::find_prepared_value_home(*context.function.value_locations,
                                            move.to_value_id);
      if (destination_home != nullptr &&
          prepared_edge_select_source_is_destination_register(*result_home,
                                                             *destination_home)) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] std::vector<module::MachineInstruction>
lower_predecessor_select_parallel_copy_sources(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.bir_function == nullptr ||
      context.control_flow_block == nullptr ||
      context.bir_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::Branch ||
      context.bir_block->terminator.kind != bir::TerminatorKind::Branch) {
    return lowered;
  }

  const auto* bundle = prepare::find_prepared_move_bundle(
      *context.function.value_locations,
      prepare::PreparedMovePhase::BlockEntry,
      context.block_index,
      0);
  if (bundle == nullptr ||
      bundle->authority_kind != prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
      bundle->source_parallel_copy_predecessor_label !=
          std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label} ||
      !bundle->source_parallel_copy_successor_label.has_value()) {
    return lowered;
  }

  const auto successor_label = prepare::prepared_block_label(
      context.function.prepared->names,
      *bundle->source_parallel_copy_successor_label);
  if (successor_label.empty() ||
      successor_label != context.bir_block->terminator.target_label) {
    return lowered;
  }
  const auto* successor =
      prepare::find_block_in_function(*context.function.bir_function, successor_label);
  if (successor == nullptr) {
    return lowered;
  }

  for (const auto& move : bundle->moves) {
    if (move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
        move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
        move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        move.source_immediate_i32.has_value() ||
        move.from_value_id == move.to_value_id) {
      continue;
    }
    const auto* source_home =
        prepare::find_prepared_value_home(*context.function.value_locations,
                                          move.from_value_id);
    const auto* destination_home =
        prepare::find_prepared_value_home(*context.function.value_locations,
                                          move.to_value_id);
    if (source_home == nullptr ||
        destination_home == nullptr ||
        source_home->value_name == c4c::kInvalidValueName ||
        destination_home->kind != prepare::PreparedValueHomeKind::Register) {
      continue;
    }
    const auto source_name =
        prepare::prepared_value_name(context.function.prepared->names, source_home->value_name);
    if (source_name.empty()) {
      continue;
    }
    if (!prepared_edge_select_source_is_destination_register(*source_home,
                                                            *destination_home)) {
      continue;
    }
    for (std::size_t source_index = 0; source_index < successor->insts.size(); ++source_index) {
      if (!binary_result_matches_value(successor->insts[source_index], source_name)) {
        continue;
      }
      auto edge_context = context;
      edge_context.bir_block = successor;
      BlockScalarLoweringState edge_state = scalar_state;
      if (source_index > 0) {
        const auto& previous = successor->insts[source_index - 1];
        const auto* previous_binary = std::get_if<bir::BinaryInst>(&previous);
        if (previous_binary != nullptr &&
            previous_binary->result.kind == bir::Value::Kind::Named &&
            binary_uses_named_value(successor->insts[source_index],
                                    previous_binary->result.name)) {
          if (auto previous_lowered =
                  lower_scalar_instruction(edge_context,
                                           previous,
                                           source_index - 1,
                                           edge_state,
                                           diagnostics)) {
            lowered.push_back(std::move(*previous_lowered));
          }
        }
      }
      auto source_lowered =
          lower_scalar_control_value_instruction(edge_context,
                                                 successor->insts[source_index],
                                                 source_index,
                                                 edge_state,
                                                 diagnostics,
                                                 true);
      if (!source_lowered.has_value()) {
        lowered.clear();
        return lowered;
      }
      lowered.push_back(std::move(*source_lowered));
      scalar_state = std::move(edge_state);
      return lowered;
    }
  }
  return lowered;
}

[[nodiscard]] module::MachineInstruction make_branch_compare_assembler_instruction(
    const module::BlockLoweringContext& context,
    std::vector<std::string> lines);

[[nodiscard]] const bir::CastInst* find_same_block_cast_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.bir_block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return nullptr;
  }
  for (const auto& inst : context.bir_block->insts) {
    const auto* cast = std::get_if<bir::CastInst>(&inst);
    if (cast != nullptr &&
        cast->result.kind == bir::Value::Kind::Named &&
        cast->result.name == value.name) {
      return cast;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<std::string> emitted_register_name(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const BlockScalarLoweringState& scalar_state,
    abi::RegisterView view) {
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  const auto emitted = find_emitted_scalar_register(scalar_state, *value_name);
  if (!emitted.has_value() || !abi::is_gp_register(emitted->reg)) {
    return std::nullopt;
  }
  const auto viewed = abi::gp_register(emitted->reg.index, view);
  if (!viewed.has_value()) {
    return std::nullopt;
  }
  return abi::register_name(*viewed);
}

[[nodiscard]] std::optional<std::string> compare_operand_name(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const BlockScalarLoweringState& scalar_state,
    abi::RegisterView view) {
  if (value.kind == bir::Value::Kind::Immediate) {
    return "#" + std::to_string(value.immediate);
  }
  return emitted_register_name(context, value, scalar_state, view);
}

struct SameBlockLoadProducer {
  const bir::LoadLocalInst* load = nullptr;
  std::size_t instruction_index = 0;
};

[[nodiscard]] SameBlockLoadProducer find_same_block_load_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.bir_block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return {};
  }
  for (std::size_t index = 0; index < context.bir_block->insts.size(); ++index) {
    const auto& inst = context.bir_block->insts[index];
    const auto* load = std::get_if<bir::LoadLocalInst>(&inst);
    if (load != nullptr &&
        load->result.kind == bir::Value::Kind::Named &&
        load->result.name == value.name) {
      return SameBlockLoadProducer{.load = load, .instruction_index = index};
    }
  }
  return {};
}

[[nodiscard]] const prepare::PreparedFrameSlot* find_frame_slot(
    const prepare::PreparedStackLayout& stack_layout,
    prepare::PreparedFrameSlotId slot_id) {
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.slot_id == slot_id) {
      return &slot;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<std::string> prepared_frame_slot_load_address(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return std::nullopt;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  const auto* access =
      addressing != nullptr
          ? prepare::find_prepared_memory_access(
                *addressing, context.control_flow_block->block_label, instruction_index)
          : nullptr;
  if (access == nullptr ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value() ||
      !access->address.can_use_base_plus_offset) {
    return std::nullopt;
  }
  const auto* slot =
      find_frame_slot(context.function.prepared->stack_layout, *access->address.frame_slot_id);
  if (slot == nullptr) {
    return std::nullopt;
  }
  const auto offset =
      static_cast<std::int64_t>(slot->offset_bytes) + access->address.byte_offset;
  std::string address = "[sp";
  if (offset != 0) {
    address += ", #";
    address += std::to_string(offset);
  }
  address += "]";
  return address;
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_fused_compare_branch_from_emitted_cast(
    const module::BlockLoweringContext& context,
    const BlockScalarLoweringState& scalar_state) {
  if (context.function.control_flow == nullptr ||
      context.function.prepared == nullptr ||
      context.control_flow_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch) {
    return std::nullopt;
  }
  const auto* branch_condition = prepare::find_prepared_branch_condition(
      *context.function.control_flow, context.control_flow_block->block_label);
  if (branch_condition == nullptr ||
      branch_condition->kind != prepare::PreparedBranchConditionKind::FusedCompare ||
      !branch_condition->can_fuse_with_branch ||
      !branch_condition->predicate.has_value() ||
      !branch_condition->compare_type.has_value() ||
      !branch_condition->lhs.has_value() ||
      !branch_condition->rhs.has_value()) {
    return std::nullopt;
  }
  const auto condition = branch_condition_suffix(*branch_condition->predicate);
  if (!condition.has_value()) {
    return std::nullopt;
  }

  const bir::Value* cast_value = &*branch_condition->lhs;
  const bir::Value* other_value = &*branch_condition->rhs;
  const bir::CastInst* cast = find_same_block_cast_producer(context, *cast_value);
  if (cast == nullptr) {
    cast_value = &*branch_condition->rhs;
    other_value = &*branch_condition->lhs;
    cast = find_same_block_cast_producer(context, *cast_value);
  }
  if (cast == nullptr ||
      cast->opcode != bir::CastOpcode::SExt ||
      cast->operand.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }

  const auto source_bits = integer_bit_width(cast->operand.type);
  const auto result_bits = integer_bit_width(cast->result.type);
  const auto result_view = scalar_register_view(cast->result.type);
  if (!source_bits.has_value() || !result_bits.has_value() ||
      !result_view.has_value() || *source_bits >= *result_bits) {
    return std::nullopt;
  }
  auto source_name =
      emitted_register_name(context, cast->operand, scalar_state, abi::RegisterView::W);
  const auto rhs_name =
      compare_operand_name(context, *other_value, scalar_state, *result_view);
  std::vector<std::string> lines;
  if (!source_name.has_value()) {
    const auto load_producer = find_same_block_load_producer(context, cast->operand);
    const auto load_address =
        load_producer.load != nullptr
            ? prepared_frame_slot_load_address(context, load_producer.instruction_index)
            : std::optional<std::string>{};
    if (!load_producer.load ||
        load_producer.load->result.type != bir::TypeKind::I8 ||
        !load_address.has_value()) {
      return std::nullopt;
    }
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    if (scratches.size() < 2U) {
      return std::nullopt;
    }
    source_name = abi::register_name(abi::w_register(scratches[1].index));
    lines.push_back("ldrb " + *source_name + ", " + *load_address);
  }
  if (!rhs_name.has_value()) {
    return std::nullopt;
  }

  std::string scratch_name;
  if (auto result_register = make_named_prepared_result_register(context, cast->result);
      result_register.has_value()) {
    const auto scratch = abi::gp_register(result_register->reg.index, *result_view);
    if (!scratch.has_value()) {
      return std::nullopt;
    }
    scratch_name = abi::register_name(*scratch);
  } else {
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    if (scratches.empty()) {
      return std::nullopt;
    }
    const auto scratch = abi::gp_register(scratches.front().index, *result_view);
    if (!scratch.has_value()) {
      return std::nullopt;
    }
    scratch_name = abi::register_name(*scratch);
  }

  if (*source_bits == 8U) {
    lines.push_back("sxtb " + scratch_name + ", " + *source_name);
  } else if (*source_bits == 16U) {
    lines.push_back("sxth " + scratch_name + ", " + *source_name);
  } else if (*source_bits == 32U && *result_bits == 64U) {
    lines.push_back("sxtw " + scratch_name + ", " + *source_name);
  } else {
    return std::nullopt;
  }
  lines.push_back("cmp " + scratch_name + ", " + *rhs_name);
  lines.push_back("b." + std::string{*condition} + " " +
                  machine_block_label(branch_condition->function_name,
                                      branch_condition->true_label));
  lines.push_back("b " + machine_block_label(branch_condition->function_name,
                                             branch_condition->false_label));
  return make_branch_compare_assembler_instruction(context, std::move(lines));
}

[[nodiscard]] bool is_fused_compare_branch_support_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    const BlockScalarLoweringState& scalar_state) {
  if (!lower_fused_compare_branch_from_emitted_cast(context, scalar_state).has_value() ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return false;
  }
  const auto* branch_condition = prepare::find_prepared_branch_condition(
      *context.function.control_flow, context.control_flow_block->block_label);
  if (branch_condition == nullptr ||
      !branch_condition->lhs.has_value() ||
      !branch_condition->rhs.has_value()) {
    return false;
  }
  if (const auto* cast = std::get_if<bir::CastInst>(&inst);
      cast != nullptr &&
      cast->result.kind == bir::Value::Kind::Named) {
    const auto matches = [&](const bir::Value& value) {
      return value.kind == bir::Value::Kind::Named &&
             value.name == cast->result.name;
    };
    return matches(*branch_condition->lhs) || matches(*branch_condition->rhs);
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(&inst);
      binary != nullptr &&
      binary->result.kind == bir::Value::Kind::Named &&
      branch_condition->condition_value.kind == bir::Value::Kind::Named) {
    return binary->result.name == branch_condition->condition_value.name;
  }
  return false;
}

[[nodiscard]] module::InstructionLoweringFamily classify_instruction(
    const bir::Inst& inst) {
  return std::visit(
      [](const auto& typed_inst) -> module::InstructionLoweringFamily {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::PhiInst>) {
          return module::InstructionLoweringFamily::Phi;
        } else if constexpr (std::is_same_v<T, bir::BinaryInst> ||
                             std::is_same_v<T, bir::CastInst>) {
          return module::InstructionLoweringFamily::Scalar;
        } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
          return module::InstructionLoweringFamily::Select;
        } else if constexpr (std::is_same_v<T, bir::CallInst>) {
          return module::InstructionLoweringFamily::Call;
        } else if constexpr (std::is_same_v<T, bir::LoadLocalInst> ||
                             std::is_same_v<T, bir::LoadGlobalInst> ||
                             std::is_same_v<T, bir::StoreLocalInst> ||
                             std::is_same_v<T, bir::StoreGlobalInst>) {
          return module::InstructionLoweringFamily::Memory;
        }
        return module::InstructionLoweringFamily::Unknown;
      },
      inst);
}

[[nodiscard]] bool is_store_local_instruction(const bir::Inst& inst) {
  return std::get_if<bir::StoreLocalInst>(&inst) != nullptr;
}

void record_address_materialization_result(
    BlockScalarLoweringState& scalar_state,
    const module::MachineInstruction& instruction) {
  const auto* address_record =
      std::get_if<AddressMaterializationRecord>(&instruction.target.payload);
  if (address_record == nullptr || !address_record->result_register.has_value()) {
    return;
  }
  record_emitted_scalar_register(scalar_state,
                                 address_record->result_value_name,
                                 *address_record->result_register);
}

void record_memory_result(BlockScalarLoweringState& scalar_state,
                          const module::MachineInstruction& instruction) {
  const auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr || !memory_record->result_register.has_value()) {
    return;
  }
  record_emitted_scalar_register(scalar_state,
                                 memory_record->result_value_name,
                                 *memory_record->result_register);
}

void retarget_pointer_store_value_to_materialized_address(
    module::MachineInstruction& instruction,
    const RegisterOperand& materialized_address) {
  auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store ||
      memory_record->value_type != bir::TypeKind::Ptr) {
    return;
  }
  memory_record->value = make_register_operand(materialized_address);
}

void retarget_pointer_store_value_to_emitted_scalar(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    const BlockScalarLoweringState& scalar_state,
    module::MachineInstruction& instruction) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr ||
      store->value.kind != bir::Value::Kind::Named ||
      store->value.type != bir::TypeKind::Ptr) {
    return;
  }
  auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store ||
      memory_record->value_type != bir::TypeKind::Ptr) {
    return;
  }
  const auto value_name = prepared_named_value_id(context, store->value);
  if (!value_name.has_value()) {
    return;
  }
  const auto emitted = find_emitted_scalar_register(scalar_state, *value_name);
  if (!emitted.has_value()) {
    return;
  }
  memory_record->value = make_register_operand(*emitted);
}

[[nodiscard]] std::string relocation_operand(std::string_view label,
                                             std::size_t byte_offset) {
  std::string operand{label};
  if (byte_offset != 0) {
    operand += "+";
    operand += std::to_string(byte_offset);
  }
  return operand;
}

[[nodiscard]] std::string frame_slot_address(std::size_t offset_bytes) {
  std::string address{"[sp"};
  if (offset_bytes != 0) {
    address += ", #";
    address += std::to_string(offset_bytes);
  }
  address += "]";
  return address;
}

[[nodiscard]] std::optional<abi::RegisterView> scalar_view_for_type(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
      return abi::RegisterView::W;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return abi::RegisterView::X;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string> gp_register_name(std::uint8_t index,
                                                          abi::RegisterView view) {
  auto reg = abi::gp_register(index, view);
  if (!reg.has_value()) {
    return std::nullopt;
  }
  return abi::register_name(*reg);
}

[[nodiscard]] const bir::Inst* find_same_block_named_producer(
    const module::BlockLoweringContext& context,
    std::string_view value_name,
    std::size_t before_instruction_index) {
  if (context.bir_block == nullptr || value_name.empty()) {
    return nullptr;
  }
  for (std::size_t index = before_instruction_index; index > 0; --index) {
    const auto& candidate = context.bir_block->insts[index - 1];
    bool matches = false;
    std::visit(
        [&](const auto& typed_inst) {
          using T = std::decay_t<decltype(typed_inst)>;
          if constexpr (std::is_same_v<T, bir::BinaryInst> ||
                        std::is_same_v<T, bir::CastInst> ||
                        std::is_same_v<T, bir::LoadLocalInst> ||
                        std::is_same_v<T, bir::LoadGlobalInst>) {
            matches = typed_inst.result.kind == bir::Value::Kind::Named &&
                      typed_inst.result.name == value_name;
          }
        },
        candidate);
    if (matches) {
      return &candidate;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<std::size_t> prepared_local_load_offset(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return std::nullopt;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  const auto* access =
      addressing != nullptr
          ? prepare::find_prepared_memory_access(
                *addressing, context.control_flow_block->block_label, instruction_index)
          : nullptr;
  if (access == nullptr ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value() ||
      !access->address.can_use_base_plus_offset) {
    return std::nullopt;
  }
  const auto* slot =
      find_frame_slot(context.function.prepared->stack_layout,
                      *access->address.frame_slot_id);
  if (slot == nullptr) {
    return std::nullopt;
  }
  return slot->offset_bytes + static_cast<std::size_t>(access->address.byte_offset);
}

[[nodiscard]] std::optional<std::size_t> producer_instruction_index(
    const module::BlockLoweringContext& context,
    const bir::Inst* producer) {
  if (context.bir_block == nullptr || producer == nullptr) {
    return std::nullopt;
  }
  for (std::size_t index = 0; index < context.bir_block->insts.size(); ++index) {
    if (&context.bir_block->insts[index] == producer) {
      return index;
    }
  }
  return std::nullopt;
}

[[nodiscard]] bool emit_value_publication_to_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines) {
  const auto target_view = scalar_view_for_type(value.type);
  if (!target_view.has_value()) {
    return false;
  }
  const auto target = gp_register_name(target_index, *target_view);
  if (!target.has_value()) {
    return false;
  }
  if (value.kind == bir::Value::Kind::Immediate) {
    lines.push_back("mov " + *target + ", #" + std::to_string(value.immediate));
    return true;
  }
  if (value.kind != bir::Value::Kind::Named) {
    return false;
  }
  const auto* producer =
      find_same_block_named_producer(context, value.name, before_instruction_index);
  if (producer == nullptr) {
    return false;
  }

  if (const auto* load_local = std::get_if<bir::LoadLocalInst>(producer);
      load_local != nullptr) {
    const auto index = producer_instruction_index(context, producer);
    const auto offset =
        index.has_value() ? prepared_local_load_offset(context, *index) : std::nullopt;
    if (!offset.has_value()) {
      return false;
    }
    lines.push_back("ldr " + *target + ", " + frame_slot_address(*offset));
    return true;
  }

  if (const auto* load_global = std::get_if<bir::LoadGlobalInst>(producer);
      load_global != nullptr) {
    const auto address = gp_register_name(scratch_index, abi::RegisterView::X);
    if (!address.has_value() || load_global->global_name.empty()) {
      return false;
    }
    const auto symbol = relocation_operand(load_global->global_name,
                                           load_global->byte_offset);
    lines.push_back("adrp " + *address + ", " + symbol);
    lines.push_back("add " + *address + ", " + *address + ", :lo12:" + symbol);
    lines.push_back("ldr " + *target + ", [" + *address + "]");
    return true;
  }

  if (const auto* cast = std::get_if<bir::CastInst>(producer); cast != nullptr) {
    if (cast->opcode == bir::CastOpcode::ZExt) {
      bir::Value source = cast->operand;
      source.type = cast->operand.type;
      if (!emit_value_publication_to_register(
              context, source, before_instruction_index, target_index, scratch_index, lines)) {
        return false;
      }
      const auto source_bits = integer_bit_width(cast->operand.type);
      const auto result_bits = integer_bit_width(cast->result.type);
      if (!source_bits.has_value() || !result_bits.has_value()) {
        return false;
      }
      if (*source_bits < 32U || *source_bits < *result_bits) {
        const auto widened = gp_register_name(target_index, abi::RegisterView::X);
        if (!widened.has_value()) {
          return false;
        }
        if (*source_bits < 32U) {
          lines.push_back("ubfx " + *widened + ", " + *widened +
                          ", #0, #" + std::to_string(*source_bits));
        }
      }
      return true;
    }
    if (cast->opcode == bir::CastOpcode::Trunc) {
      bir::Value source = cast->operand;
      source.type = cast->operand.type;
      return emit_value_publication_to_register(
          context, source, before_instruction_index, target_index, scratch_index, lines);
    }
    return false;
  }

  const auto* binary = std::get_if<bir::BinaryInst>(producer);
  if (binary == nullptr ||
      (binary->opcode != bir::BinaryOpcode::Add &&
       binary->opcode != bir::BinaryOpcode::Sub)) {
    return false;
  }
  auto lhs = binary->lhs;
  lhs.type = binary->operand_type;
  auto rhs = binary->rhs;
  rhs.type = binary->operand_type;
  if (!emit_value_publication_to_register(
          context, lhs, before_instruction_index, target_index, scratch_index, lines)) {
    return false;
  }
  const std::uint8_t nested_scratch_index = scratch_index == 9 ? 10 : 9;
  if (!emit_value_publication_to_register(
          context, rhs, before_instruction_index, scratch_index, nested_scratch_index, lines)) {
    return false;
  }
  const auto result = gp_register_name(target_index, abi::RegisterView::X);
  const auto rhs_name = gp_register_name(scratch_index, abi::RegisterView::X);
  if (!result.has_value() || !rhs_name.has_value()) {
    return false;
  }
  lines.push_back(std::string{binary->opcode == bir::BinaryOpcode::Add ? "add " : "sub "} +
                  *result + ", " + *result + ", " + *rhs_name);
  return true;
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_store_global_value_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    const module::MachineInstruction& lowered_memory) {
  const auto* store = std::get_if<bir::StoreGlobalInst>(&inst);
  if (store == nullptr || store->value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&lowered_memory.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store ||
      !memory_record->value.has_value() ||
      memory_record->value->kind != OperandKind::Register) {
    return std::nullopt;
  }
  const auto* target_register =
      std::get_if<RegisterOperand>(&memory_record->value->payload);
  if (target_register == nullptr ||
      !abi::is_gp_register(target_register->reg) ||
      abi::is_reserved_mir_scratch(target_register->reg)) {
    return std::nullopt;
  }

  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  std::vector<std::string> lines;
  auto value = store->value;
  if (!emit_value_publication_to_register(context,
                                          value,
                                          instruction_index,
                                          target_register->reg.index,
                                          scratches.front().index,
                                          lines) ||
      lines.empty()) {
    return std::nullopt;
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects = {MachineSideEffectKind::MemoryRead},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = [&] {
            std::string text;
            for (std::size_t index = 0; index < lines.size(); ++index) {
              if (index != 0) {
                text += '\n';
              }
              text += lines[index];
            }
            return text;
          }(),
      },
  };
  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = instruction_index,
          },
  };
}

[[nodiscard]] bool lower_store_local_with_address_materialization(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::MachineBlock& block,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (!is_store_local_instruction(inst)) {
    return false;
  }

  auto materialized = lower_address_materialization(context, instruction_index, diagnostics);
  if (!materialized.has_value()) {
    return false;
  }
  std::optional<RegisterOperand> materialized_address;
  if (const auto* address_record =
          std::get_if<AddressMaterializationRecord>(&materialized->target.payload);
      address_record != nullptr && address_record->result_register.has_value()) {
    materialized_address = *address_record->result_register;
  }

  record_address_materialization_result(scalar_state, *materialized);
  block.instructions.push_back(std::move(*materialized));

  auto lowered_memory =
      lower_memory_instruction(context, inst, instruction_index, diagnostics);
  if (lowered_memory.handled && lowered_memory.instruction.has_value()) {
    if (materialized_address.has_value()) {
      retarget_pointer_store_value_to_materialized_address(
          *lowered_memory.instruction, *materialized_address);
    }
    record_memory_result(scalar_state, *lowered_memory.instruction);
    block.instructions.push_back(std::move(*lowered_memory.instruction));
  }
  return lowered_memory.handled;
}

[[nodiscard]] bool lower_scalar_with_address_materialization(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::MachineBlock& block,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  if (binary == nullptr ||
      binary->result.kind != bir::Value::Kind::Named ||
      binary->result.type != bir::TypeKind::Ptr ||
      context.bir_block == nullptr ||
      instruction_index + 1 >= context.bir_block->insts.size()) {
    return false;
  }
  const auto* next_store =
      std::get_if<bir::StoreLocalInst>(&context.bir_block->insts[instruction_index + 1]);
  if (next_store == nullptr ||
      next_store->value.kind != bir::Value::Kind::Named ||
      next_store->value.type != bir::TypeKind::Ptr ||
      next_store->value.name != binary->result.name) {
    return false;
  }

  auto materialized = lower_address_materialization(context, instruction_index, diagnostics);
  if (!materialized.has_value()) {
    return false;
  }

  record_address_materialization_result(scalar_state, *materialized);
  auto lowered_scalar =
      lower_scalar_instruction(context, inst, instruction_index, scalar_state, diagnostics);
  if (!lowered_scalar.has_value()) {
    return false;
  }

  block.instructions.push_back(std::move(*materialized));
  block.instructions.push_back(std::move(*lowered_scalar));
  return true;
}

void append_unsupported_instruction_diagnostic(
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .instruction_family = classify_instruction(inst),
      .message =
          "AArch64 block dispatch visited unsupported prepared BIR instruction family",
  });
}

void append_call_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
                            module::ModuleLoweringDiagnosticKind kind,
                            const module::BlockLoweringContext& context,
                            std::size_t instruction_index,
                            std::string message) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = kind,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .instruction_family = module::InstructionLoweringFamily::Call,
      .message = std::move(message),
  });
}

[[nodiscard]] bool is_dynamic_alloca_helper(std::string_view callee) {
  constexpr std::string_view kPrefix = "llvm.dynamic_alloca.";
  return callee.substr(0, kPrefix.size()) == kPrefix;
}

[[nodiscard]] std::optional<prepare::PreparedDynamicStackOpKind>
dynamic_stack_helper_kind(std::string_view callee) {
  if (callee == "llvm.stacksave") {
    return prepare::PreparedDynamicStackOpKind::StackSave;
  }
  if (is_dynamic_alloca_helper(callee)) {
    return prepare::PreparedDynamicStackOpKind::DynamicAlloca;
  }
  if (callee == "llvm.stackrestore") {
    return prepare::PreparedDynamicStackOpKind::StackRestore;
  }
  return std::nullopt;
}

[[nodiscard]] const prepare::PreparedDynamicStackOp* find_dynamic_stack_op(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    prepare::PreparedDynamicStackOpKind kind) {
  if (context.function.dynamic_stack_plan == nullptr) {
    return nullptr;
  }
  const c4c::BlockLabelId block_label =
      context.control_flow_block != nullptr ? context.control_flow_block->block_label
                                            : c4c::kInvalidBlockLabel;
  for (const auto& op : context.function.dynamic_stack_plan->operations) {
    if (op.kind == kind && op.instruction_index == instruction_index &&
        (op.block_label == block_label || op.block_label == c4c::kInvalidBlockLabel ||
         block_label == c4c::kInvalidBlockLabel)) {
      return &op;
    }
  }
  return nullptr;
}

[[nodiscard]] const prepare::PreparedValueHome* find_dynamic_stack_value_home(
    const module::FunctionLoweringContext& context,
    c4c::ValueNameId value_name) {
  if (context.value_locations == nullptr || value_name == c4c::kInvalidValueName) {
    return nullptr;
  }
  for (const auto& home : context.value_locations->value_homes) {
    if (home.value_name == value_name) {
      return &home;
    }
  }
  return nullptr;
}

[[nodiscard]] const prepare::PreparedValueHome* find_dynamic_stack_value_home(
    const module::FunctionLoweringContext& context,
    const std::optional<c4c::ValueNameId>& value_name) {
  if (!value_name.has_value()) {
    return nullptr;
  }
  return find_dynamic_stack_value_home(context, *value_name);
}

[[nodiscard]] std::optional<std::string> dynamic_stack_register_home_name(
    const prepare::PreparedValueHome& home) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      !home.register_name.has_value() || home.register_name->empty()) {
    return std::nullopt;
  }
  return home.register_name;
}

[[nodiscard]] std::optional<std::string> dynamic_stack_stack_home_address(
    const module::FunctionLoweringContext& context,
    const prepare::PreparedValueHome& home) {
  if (home.kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home.offset_bytes.has_value()) {
    return std::nullopt;
  }
  if (context.frame_plan == nullptr ||
      !context.frame_plan->uses_frame_pointer_for_fixed_slots) {
    return std::nullopt;
  }
  return std::string{"[x29, #"} + std::to_string(*home.offset_bytes) + "]";
}

[[nodiscard]] bool dynamic_stack_home_requires_stable_frame_pointer(
    const module::FunctionLoweringContext& context,
    const prepare::PreparedValueHome& home) {
  return home.kind == prepare::PreparedValueHomeKind::StackSlot &&
         (context.frame_plan == nullptr ||
          !context.frame_plan->uses_frame_pointer_for_fixed_slots);
}

[[nodiscard]] std::optional<std::size_t> dynamic_stack_element_size_bytes(
    const prepare::PreparedDynamicStackOp& op) {
  if (op.element_size_bytes != 0) {
    return op.element_size_bytes;
  }
  if (op.allocation_type_text == "i8") {
    return std::size_t{1};
  }
  if (op.allocation_type_text == "i16") {
    return std::size_t{2};
  }
  if (op.allocation_type_text == "i32") {
    return std::size_t{4};
  }
  if (op.allocation_type_text == "i64" || op.allocation_type_text == "ptr") {
    return std::size_t{8};
  }
  return std::nullopt;
}

[[nodiscard]] module::MachineInstruction make_bir_machine_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    InstructionRecord target) {
  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = instruction_index,
          },
  };
}

[[nodiscard]] module::MachineInstruction make_branch_compare_assembler_instruction(
    const module::BlockLoweringContext& context,
    std::vector<std::string> lines) {
  std::string asm_text;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (index != 0) {
      asm_text += '\n';
    }
    asm_text += lines[index];
  }

  const std::size_t instruction_index =
      context.bir_block != nullptr ? context.bir_block->insts.size() : 0;
  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection =
          MachineNodeStatusRecord{
              .status = MachineNodeSelectionStatus::Selected,
          },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects = {MachineSideEffectKind::ControlFlowTransfer},
      .payload =
          AssemblerInstructionRecord{
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  return make_bir_machine_instruction(context, instruction_index, std::move(target));
}

[[nodiscard]] InstructionRecord make_dynamic_stack_rejection_record(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::string_view diagnostic) {
  return InstructionRecord{
      .family = InstructionFamily::Frame,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::FrameSetup,
      .selection =
          MachineNodeStatusRecord{
              .status = MachineNodeSelectionStatus::DeferredUnsupported,
              .diagnostic = diagnostic,
          },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects = {MachineSideEffectKind::FrameSetup},
      .payload = FrameInstructionRecord{},
  };
}

[[nodiscard]] module::MachineInstruction make_dynamic_stack_assembler_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::vector<std::string> lines) {
  std::string asm_text;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (index != 0) {
      asm_text += '\n';
    }
    asm_text += lines[index];
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects = {MachineSideEffectKind::FrameSetup},
      .payload =
          AssemblerInstructionRecord{
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  return make_bir_machine_instruction(context, instruction_index, std::move(target));
}

[[nodiscard]] std::optional<module::MachineInstruction> make_dynamic_stack_failure(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::string_view message,
    module::ModuleLoweringDiagnostics& diagnostics) {
  append_call_diagnostic(diagnostics,
                         module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
                         context,
                         instruction_index,
                         std::string{message});
  return make_bir_machine_instruction(
      context, instruction_index, make_dynamic_stack_rejection_record(
                                      context, instruction_index, message));
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_dynamic_stack_save(
    const module::BlockLoweringContext& context,
    const prepare::PreparedDynamicStackOp& op,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* result_home =
      find_dynamic_stack_value_home(context.function, op.result_value_name);
  if (result_home == nullptr) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack stack-save lowering requires a prepared result home",
        diagnostics);
  }
  if (const auto reg = dynamic_stack_register_home_name(*result_home); reg.has_value()) {
    return make_dynamic_stack_assembler_instruction(
        context, instruction_index, std::vector<std::string>{"mov " + *reg + ", sp"});
  }
  if (const auto address =
          dynamic_stack_stack_home_address(context.function, *result_home);
      address.has_value()) {
    return make_dynamic_stack_assembler_instruction(
        context,
        instruction_index,
        std::vector<std::string>{"mov x16, sp", "str x16, " + *address});
  }
  if (dynamic_stack_home_requires_stable_frame_pointer(context.function, *result_home)) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack stack-save lowering requires stack-slot homes to use a stable frame-pointer base",
        diagnostics);
  }
  return make_dynamic_stack_failure(
      context,
      instruction_index,
      "AArch64 dynamic-stack stack-save lowering requires a register or stack-slot result home",
      diagnostics);
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_dynamic_alloca(
    const module::BlockLoweringContext& context,
    const prepare::PreparedDynamicStackOp& op,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto element_size = dynamic_stack_element_size_bytes(op);
  if (!element_size.has_value() || *element_size > 65535) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack allocation lowering requires a printable nonzero element size",
        diagnostics);
  }

  const auto* count_home =
      find_dynamic_stack_value_home(context.function, op.operand_value_name);
  const auto* result_home =
      find_dynamic_stack_value_home(context.function, op.result_value_name);
  if (count_home == nullptr || result_home == nullptr) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack allocation lowering requires prepared operand and result homes",
        diagnostics);
  }

  std::vector<std::string> lines;
  std::string count_register;
  if (const auto reg = dynamic_stack_register_home_name(*count_home); reg.has_value()) {
    count_register = *reg;
  } else if (const auto address =
                 dynamic_stack_stack_home_address(context.function, *count_home);
             address.has_value()) {
    lines.push_back("ldr x16, " + *address);
    count_register = "x16";
  } else if (dynamic_stack_home_requires_stable_frame_pointer(context.function, *count_home)) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack allocation lowering requires stack-slot count homes to use a stable frame-pointer base",
        diagnostics);
  } else {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack allocation lowering requires a register or stack-slot count home",
        diagnostics);
  }

  std::string byte_count_register = count_register;
  if (byte_count_register == "x17") {
    lines.push_back("mov x16, x17");
    byte_count_register = "x16";
  }
  if (*element_size != 1) {
    if (byte_count_register != "x16") {
      lines.push_back("mov x16, " + byte_count_register);
      byte_count_register = "x16";
    }
    lines.push_back("mov x17, #" + std::to_string(*element_size));
    lines.push_back("mul x16, x16, x17");
  }

  lines.push_back("mov x17, sp");
  lines.push_back("sub x17, x17, " + byte_count_register);
  lines.push_back("and x17, x17, #-16");
  lines.push_back("mov sp, x17");

  if (const auto reg = dynamic_stack_register_home_name(*result_home); reg.has_value()) {
    lines.push_back("mov " + *reg + ", sp");
  } else if (const auto address =
                 dynamic_stack_stack_home_address(context.function, *result_home);
             address.has_value()) {
    lines.push_back("str x17, " + *address);
  } else if (dynamic_stack_home_requires_stable_frame_pointer(context.function, *result_home)) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack allocation lowering requires stack-slot result homes to use a stable frame-pointer base",
        diagnostics);
  } else {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack allocation lowering requires a register or stack-slot result home",
        diagnostics);
  }

  return make_dynamic_stack_assembler_instruction(
      context, instruction_index, std::move(lines));
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_dynamic_stack_restore(
    const module::BlockLoweringContext& context,
    const prepare::PreparedDynamicStackOp& op,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* operand_home =
      find_dynamic_stack_value_home(context.function, op.operand_value_name);
  if (operand_home == nullptr) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack restore lowering requires a prepared operand home",
        diagnostics);
  }
  if (const auto reg = dynamic_stack_register_home_name(*operand_home); reg.has_value()) {
    return make_dynamic_stack_assembler_instruction(
        context, instruction_index, std::vector<std::string>{"mov sp, " + *reg});
  }
  if (const auto address =
          dynamic_stack_stack_home_address(context.function, *operand_home);
      address.has_value()) {
    return make_dynamic_stack_assembler_instruction(
        context,
        instruction_index,
        std::vector<std::string>{"ldr x16, " + *address, "mov sp, x16"});
  }
  if (dynamic_stack_home_requires_stable_frame_pointer(context.function, *operand_home)) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack restore lowering requires stack-slot operand homes to use a stable frame-pointer base",
        diagnostics);
  }
  return make_dynamic_stack_failure(
      context,
      instruction_index,
      "AArch64 dynamic-stack restore lowering requires a register or stack-slot operand home",
      diagnostics);
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_dynamic_stack_helper_call(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto kind = dynamic_stack_helper_kind(call_inst.callee);
  if (!kind.has_value()) {
    return std::nullopt;
  }

  const auto* op = find_dynamic_stack_op(context, instruction_index, *kind);
  if (op == nullptr) {
    constexpr std::string_view message =
        "AArch64 dynamic-stack helper lowering requires prepared dynamic-stack operation authority";
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        std::string{message});
    return make_bir_machine_instruction(
        context, instruction_index, make_dynamic_stack_rejection_record(
                                        context, instruction_index, message));
  }

  switch (*kind) {
    case prepare::PreparedDynamicStackOpKind::StackSave:
      return lower_dynamic_stack_save(context, *op, instruction_index, diagnostics);
    case prepare::PreparedDynamicStackOpKind::DynamicAlloca:
      return lower_dynamic_alloca(context, *op, instruction_index, diagnostics);
    case prepare::PreparedDynamicStackOpKind::StackRestore:
      return lower_dynamic_stack_restore(context, *op, instruction_index, diagnostics);
  }
  return make_dynamic_stack_failure(
      context,
      instruction_index,
      "AArch64 dynamic-stack helper lowering visited an unsupported operation kind",
      diagnostics);
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_call_instruction(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (auto dynamic_stack = lower_dynamic_stack_helper_call(
          context, call_inst, instruction_index, diagnostics);
      dynamic_stack.has_value()) {
    return dynamic_stack;
  }

  const auto variadic_helper = variadic_entry_helper_kind(call_inst.callee);
  const prepare::PreparedVariadicEntryPlanFunction* variadic_entry_plan = nullptr;
  if (variadic_helper.has_value()) {
    variadic_entry_plan =
        require_prepared_variadic_entry_plan(context, instruction_index, diagnostics);
    if (variadic_entry_plan == nullptr) {
      return std::nullopt;
    }
  }

  const auto* call_plan =
      require_prepared_call_plan(context, instruction_index, diagnostics);
  if (call_plan == nullptr) {
    return std::nullopt;
  }

  const prepare::PreparedVariadicEntryHelperOperandHomes* variadic_helper_operand_homes =
      nullptr;
  if (variadic_entry_plan != nullptr) {
    variadic_helper_operand_homes =
        prepare::find_prepared_variadic_entry_helper_operand_homes(
            *variadic_entry_plan, context.block_index, instruction_index);
    if (variadic_helper_operand_homes == nullptr ||
        !variadic_helper_operand_homes_complete(*variadic_helper_operand_homes)) {
      std::string message =
          "AArch64 variadic entry helper lowering requires prepared helper operand-home facts";
      const auto missing_consumption_fact =
          variadic_helper_missing_consumption_fact_message(*variadic_helper);
      if (!missing_consumption_fact.empty()) {
        message = missing_consumption_fact;
      }
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          std::move(message));
      return std::nullopt;
    }
  }

  return lower_prepared_call_instruction(context,
                                         call_inst,
                                         *call_plan,
                                         instruction_index,
                                         variadic_entry_plan,
                                         variadic_helper_operand_homes,
                                         variadic_helper,
                                         diagnostics);
}

}  // namespace

module::BlockLoweringContext make_block_lowering_context(
    module::FunctionLoweringContext function,
    const prepare::PreparedControlFlowBlock& block,
    std::size_t block_index) {
  return module::BlockLoweringContext{
      .function = function,
      .control_flow_block = &block,
      .bir_block = find_bir_block(function, block),
      .block_index = block_index,
  };
}

InstructionDispatchResult dispatch_prepared_block(
    const module::BlockLoweringContext& context,
    module::MachineBlock& block,
    module::ModuleLoweringDiagnostics& diagnostics) {
  InstructionDispatchResult result;

  if (context.function.control_flow == nullptr || context.control_flow_block == nullptr) {
    append_block_diagnostic(diagnostics,
                            module::ModuleLoweringDiagnosticKind::MissingBlockContext,
                            context,
                            "AArch64 block dispatch requires prepared function and block context");
    return result;
  }

  block.block_label = context.control_flow_block->block_label;
  block.index = context.block_index;
  block.successors.clear();

  if (context.bir_block == nullptr && context.function.bir_function != nullptr) {
    append_block_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingInstructionBlockMapping,
        context,
        "AArch64 block dispatch could not map prepared block to retained BIR instructions");
  }

  BlockScalarLoweringState scalar_state;
  auto record_call_boundary_destination =
      [&](const module::MachineInstruction& instruction) {
    const auto* move_record =
        std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
    if (move_record != nullptr && move_record->destination_register.has_value()) {
      record_emitted_scalar_register(scalar_state,
                                     move_record->destination_register->value_name,
                                     *move_record->destination_register);
    }
  };
  auto retarget_call_boundary_source_to_emitted_scalar =
      [&](module::MachineInstruction& instruction) {
    auto* move_record =
        std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
    if (move_record == nullptr) {
      return;
    }
    std::optional<c4c::ValueNameId> source_value_name;
    if (move_record->source_memory.has_value() &&
        move_record->source_memory->result_value_name.has_value()) {
      source_value_name = *move_record->source_memory->result_value_name;
    } else if (move_record->source_register.has_value() &&
               move_record->source_register->value_name != c4c::kInvalidValueName) {
      source_value_name = move_record->source_register->value_name;
    }
    if (!source_value_name.has_value()) {
      return;
    }
    const auto emitted = find_emitted_scalar_register(scalar_state, *source_value_name);
    if (!emitted.has_value()) {
      return;
    }
    move_record->source_register = *emitted;
    move_record->source_memory.reset();
    if (move_record->destination_register.has_value() &&
        emitted->reg.bank == abi::RegisterBank::GeneralPurpose &&
        move_record->destination_register->reg.bank == abi::RegisterBank::GeneralPurpose &&
        emitted->expected_view.has_value()) {
      const auto retargeted_destination =
          abi::gp_register(move_record->destination_register->reg.index,
                           *emitted->expected_view);
      if (retargeted_destination.has_value()) {
        move_record->destination_register->reg = *retargeted_destination;
        move_record->destination_register->expected_view = emitted->expected_view;
      }
    }
  };
  auto source_value_is_materialized_address =
      [](const CallBoundaryMoveInstructionRecord& move_record,
         const std::vector<module::MachineInstruction>& materialized_addresses) {
    for (const auto& materialized : materialized_addresses) {
      const auto* address_record =
          std::get_if<AddressMaterializationRecord>(&materialized.target.payload);
      if (address_record == nullptr) {
        continue;
      }
      if (address_record->result_value_id.has_value() &&
          *address_record->result_value_id == move_record.move.from_value_id) {
        return true;
      }
      if (move_record.source_register.has_value() &&
          address_record->result_value_name != c4c::kInvalidValueName &&
          move_record.source_register->value_name == address_record->result_value_name) {
        return true;
      }
    }
    return false;
  };
  auto source_register_conflicts_with_materialized_address =
      [&](const CallBoundaryMoveInstructionRecord& move_record,
          const std::vector<module::MachineInstruction>& materialized_addresses) {
    if (!move_record.source_register.has_value() ||
        source_value_is_materialized_address(move_record, materialized_addresses)) {
      return false;
    }
    for (const auto& materialized : materialized_addresses) {
      const auto* address_record =
          std::get_if<AddressMaterializationRecord>(&materialized.target.payload);
      if (address_record == nullptr || !address_record->result_register.has_value()) {
        continue;
      }
      if (registers_alias(*move_record.source_register,
                          *address_record->result_register)) {
        return true;
      }
    }
    return false;
  };
  for (auto& block_entry_move : lower_value_moves(
           context,
           prepare::PreparedMovePhase::BlockEntry,
           0,
           diagnostics)) {
    record_call_boundary_destination(block_entry_move);
    block.instructions.push_back(std::move(block_entry_move));
  }
  if (context.bir_block != nullptr) {
    for (std::size_t instruction_index = 0;
         instruction_index < context.bir_block->insts.size();
         ++instruction_index) {
      const auto& inst = context.bir_block->insts[instruction_index];
      if (const auto* call = std::get_if<bir::CallInst>(&inst)) {
        if (auto dynamic_stack = lower_dynamic_stack_helper_call(
                context, *call, instruction_index, diagnostics)) {
          block.instructions.push_back(std::move(*dynamic_stack));
          ++result.visited_operations;
          continue;
        }
        if (call->inline_asm.has_value() ||
            has_prepared_inline_asm_carrier(context, instruction_index)) {
          if (auto lowered = lower_inline_asm_instruction(
                  context, *call, instruction_index, diagnostics)) {
            block.instructions.push_back(std::move(*lowered));
          }
          ++result.visited_operations;
          continue;
        }
        if (has_prepared_intrinsic_carrier(context, instruction_index)) {
          if (auto lowered = lower_intrinsic_instruction(
                  context, instruction_index, diagnostics)) {
            block.instructions.push_back(std::move(*lowered));
          }
          ++result.visited_operations;
          continue;
        }
        const auto* call_plan = find_prepared_call_plan(context, instruction_index);
        if (call_plan != nullptr) {
          auto argument_producers =
              lower_scalar_call_argument_producers(context,
                                                   *call,
                                                   instruction_index,
                                                   scalar_state,
                                                   diagnostics);
          for (auto& argument_producer : argument_producers) {
            block.instructions.push_back(std::move(argument_producer));
          }
          auto materialized_addresses =
              lower_address_materializations(context, instruction_index, diagnostics);
          auto before_call_moves =
              lower_before_call_moves(context, *call_plan, instruction_index, diagnostics);
          for (auto& before_call_move : before_call_moves) {
            retarget_call_boundary_source_to_emitted_scalar(before_call_move);
          }
          std::vector<module::MachineInstruction> deferred_before_call_moves;
          for (auto& before_call_move : before_call_moves) {
            const auto* move_record =
                std::get_if<CallBoundaryMoveInstructionRecord>(
                    &before_call_move.target.payload);
            if (move_record != nullptr &&
                source_register_conflicts_with_materialized_address(
                    *move_record, materialized_addresses)) {
              block.instructions.push_back(std::move(before_call_move));
            } else {
              deferred_before_call_moves.push_back(std::move(before_call_move));
            }
          }
          for (auto& materialized : materialized_addresses) {
            if (const auto* address_record =
                    std::get_if<AddressMaterializationRecord>(&materialized.target.payload);
                address_record != nullptr && address_record->result_register.has_value()) {
              record_emitted_scalar_register(scalar_state,
                                             address_record->result_value_name,
                                             *address_record->result_register);
            }
            block.instructions.push_back(std::move(materialized));
          }
          for (auto& before_call_move : deferred_before_call_moves) {
            retarget_call_boundary_source_to_emitted_scalar(before_call_move);
            block.instructions.push_back(std::move(before_call_move));
          }
        }
        if (auto lowered = lower_call_instruction(
                context, *call, instruction_index, diagnostics)) {
          block.instructions.push_back(std::move(*lowered));
          clear_call_clobbered_emitted_scalar_registers(scalar_state);
          if (call_plan != nullptr) {
            auto after_call_moves =
                lower_after_call_moves(context, *call_plan, instruction_index, diagnostics);
            for (auto& after_call_move : after_call_moves) {
              record_call_boundary_destination(after_call_move);
              block.instructions.push_back(std::move(after_call_move));
            }
          }
        }
      } else if (lower_store_local_with_address_materialization(
                     context,
                     inst,
                     instruction_index,
                     scalar_state,
                     block,
                     diagnostics)) {
        ++result.visited_operations;
        continue;
      } else if (lower_scalar_with_address_materialization(
                     context,
                     inst,
                     instruction_index,
                     scalar_state,
                     block,
                     diagnostics)) {
        ++result.visited_operations;
        continue;
      } else if (auto lowered = lower_address_materialization(
                     context, instruction_index, diagnostics)) {
        record_address_materialization_result(scalar_state, *lowered);
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered_i128_pair =
                     lower_i128_pair_operation_instruction(
                         context, inst, instruction_index, diagnostics);
                 lowered_i128_pair.handled) {
        if (lowered_i128_pair.instruction.has_value()) {
          block.instructions.push_back(std::move(*lowered_i128_pair.instruction));
        }
        ++result.visited_operations;
        continue;
      } else if (auto lowered_i128_copy =
                     lower_i128_copy_instruction(context, inst, instruction_index, diagnostics);
                 lowered_i128_copy.handled) {
        if (lowered_i128_copy.instruction.has_value()) {
          block.instructions.push_back(std::move(*lowered_i128_copy.instruction));
        }
        ++result.visited_operations;
        continue;
      } else if (auto lowered_f128_helper =
                     lower_f128_runtime_helper_instruction(
                         context, inst, instruction_index, diagnostics);
                 lowered_f128_helper.handled) {
        if (lowered_f128_helper.instruction.has_value()) {
          block.instructions.push_back(std::move(*lowered_f128_helper.instruction));
        }
        ++result.visited_operations;
        continue;
      } else if (auto lowered = lower_prepared_scalar_float_alu_instruction(
              context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (is_fused_compare_branch_support_instruction(context, inst, scalar_state)) {
        continue;
      } else if (auto lowered = lower_scalar_cast_instruction(
              context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (is_current_block_join_parallel_copy_source(context, inst)) {
        continue;
      } else if (auto lowered = lower_scalar_instruction(
              context, inst, instruction_index, scalar_state, diagnostics)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered = lower_scalar_control_value_instruction(
              context, inst, instruction_index, scalar_state, diagnostics)) {
        block.instructions.push_back(std::move(*lowered));
      } else {
        auto lowered_i128_transport =
            lower_i128_transport_instruction(context, inst, instruction_index, diagnostics);
        if (lowered_i128_transport.handled) {
          if (lowered_i128_transport.instruction.has_value()) {
            block.instructions.push_back(std::move(*lowered_i128_transport.instruction));
          }
          ++result.visited_operations;
          continue;
        }
        auto lowered_memory =
            lower_f128_transport_instruction(context, inst, instruction_index, diagnostics);
        if (lowered_memory.handled) {
          if (lowered_memory.instruction.has_value()) {
            record_memory_result(scalar_state, *lowered_memory.instruction);
            block.instructions.push_back(std::move(*lowered_memory.instruction));
          }
        } else if (auto lowered_ordinary_memory =
                       lower_memory_instruction(context, inst, instruction_index, diagnostics);
                   lowered_ordinary_memory.handled) {
          if (lowered_ordinary_memory.instruction.has_value()) {
            retarget_pointer_store_value_to_emitted_scalar(
                context, inst, scalar_state, *lowered_ordinary_memory.instruction);
            if (auto value_publication =
                    lower_store_global_value_publication(
                        context, inst, instruction_index, *lowered_ordinary_memory.instruction)) {
              block.instructions.push_back(std::move(*value_publication));
            }
            record_memory_result(scalar_state, *lowered_ordinary_memory.instruction);
            block.instructions.push_back(std::move(*lowered_ordinary_memory.instruction));
          }
        } else {
          append_unsupported_instruction_diagnostic(
              diagnostics, context, inst, instruction_index);
        }
      }
      ++result.visited_operations;
    }
  }

  auto lowered_atomic_operations =
      lower_atomic_memory_operations_for_block(context, diagnostics);
  result.visited_operations += lowered_atomic_operations.size();
  for (auto& atomic_instruction : lowered_atomic_operations) {
    block.instructions.push_back(std::move(atomic_instruction));
  }

  result.visited_terminator = true;
  if (context.control_flow_block->terminator_kind ==
      c4c::backend::bir::TerminatorKind::Return) {
    if (auto lowered =
            lower_prepared_return_terminator(context, scalar_state, diagnostics)) {
      block.instructions.push_back(std::move(*lowered));
    }
  } else if (context.control_flow_block->terminator_kind ==
             c4c::backend::bir::TerminatorKind::Branch) {
    for (auto& edge_source :
         lower_predecessor_select_parallel_copy_sources(context, scalar_state, diagnostics)) {
      block.instructions.push_back(std::move(edge_source));
    }
    if (auto lowered = lower_prepared_branch_terminator(context, diagnostics)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors.push_back(make_unconditional_branch_successor(context));
    }
  } else if (context.control_flow_block->terminator_kind ==
             c4c::backend::bir::TerminatorKind::CondBranch) {
    if (auto lowered =
            lower_fused_compare_branch_from_emitted_cast(context, scalar_state)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors = make_conditional_branch_successors(context);
    } else if (auto lowered =
            lower_prepared_conditional_branch_terminator(context, diagnostics)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors = make_conditional_branch_successors(context);
    }
  } else {
    append_block_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily,
        context,
        unsupported_terminator_message(context.control_flow_block->terminator_kind));
  }

  result.emitted_instructions = block.instructions.size();
  return result;
}

}  // namespace c4c::backend::aarch64::codegen
