#include "prepared_call_emit.hpp"

#include "prepared_emit_context.hpp"
#include "prepared_frame_emit.hpp"
#include "prepared_scalar_emit.hpp"

#include "../../../prealloc/addressing.hpp"
#include "../../../prealloc/calls.hpp"

#include <cstdint>
#include <limits>
#include <vector>

namespace c4c::backend::riscv::codegen {
namespace {

std::optional<std::string_view> riscv_gpr_argument_register(std::size_t index) {
  switch (index) {
    case 0: return std::string_view{"a0"};
    case 1: return std::string_view{"a1"};
    case 2: return std::string_view{"a2"};
    case 3: return std::string_view{"a3"};
    case 4: return std::string_view{"a4"};
    case 5: return std::string_view{"a5"};
    case 6: return std::string_view{"a6"};
    case 7: return std::string_view{"a7"};
    default: return std::nullopt;
  }
}

std::optional<std::int64_t> prepared_frame_slot_address_offset_for_value(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    const PreparedCurrentInstructionContext& context,
    c4c::backend::prepare::PreparedValueId value_id) {
  namespace prepare = c4c::backend::prepare;

  if (context.lookups == nullptr) {
    return std::nullopt;
  }
  const auto home_it = context.lookups->value_homes.homes_by_id.find(value_id);
  const auto* materializations =
      prepare::find_indexed_prepared_address_materializations(
          &context.lookups->address_materializations,
          context.block_label);
  if (home_it == context.lookups->value_homes.homes_by_id.end() ||
      home_it->second == nullptr ||
      home_it->second->value_name == c4c::kInvalidValueName ||
      materializations == nullptr) {
    return std::nullopt;
  }

  std::optional<std::int64_t> selected_offset;
  for (const auto* materialization : *materializations) {
    if (materialization == nullptr ||
        materialization->inst_index != context.instruction_index ||
        materialization->kind !=
            prepare::PreparedAddressMaterializationKind::FrameSlot ||
        materialization->result_value_name != home_it->second->value_name) {
      continue;
    }
    const auto stack_offset =
        simple_frame_slot_sp_offset_for(prepared, function_name, *materialization);
    if (!stack_offset.has_value()) {
      return std::nullopt;
    }
    if (selected_offset.has_value() && *selected_offset != *stack_offset) {
      return std::nullopt;
    }
    selected_offset = *stack_offset;
  }
  return selected_offset;
}

bool emit_riscv_frame_slot_address_argument(
    std::string& out,
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    const c4c::backend::prepare::PreparedCallArgumentPlan& plan,
    const PreparedCurrentInstructionContext& context,
    std::size_t active_stack_adjustment_bytes) {
  if (!plan.source_value_id.has_value() ||
      !plan.destination_register_name.has_value() ||
      plan.destination_register_name->empty()) {
    return false;
  }
  const auto stack_offset = prepared_frame_slot_address_offset_for_value(
      prepared,
      function_name,
      context,
      *plan.source_value_id);
  if (!stack_offset.has_value() || *stack_offset < 0 ||
      active_stack_adjustment_bytes >
          static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()) ||
      static_cast<std::size_t>(*stack_offset) >
          static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()) -
              active_stack_adjustment_bytes ||
      !fits_signed_12_bit_immediate(
          *stack_offset + static_cast<std::int64_t>(active_stack_adjustment_bytes))) {
    return false;
  }

  out += "    addi " + *plan.destination_register_name + ", sp, " +
         std::to_string(
             *stack_offset + static_cast<std::int64_t>(active_stack_adjustment_bytes)) +
         "\n";
  return true;
}

bool emit_riscv_prior_preserved_gpr_argument(
    std::string& out,
    const c4c::backend::prepare::PreparedCallArgumentPlan& plan) {
  namespace prepare = c4c::backend::prepare;

  if (!plan.source_selection.has_value() ||
      plan.source_selection->kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::PriorPreservation) {
    return false;
  }
  const auto& selection = *plan.source_selection;
  if (selection.preservation_route !=
          prepare::PreparedCallPreservationRoute::CalleeSavedRegister ||
      selection.preserved_register_bank !=
          std::optional<prepare::PreparedRegisterBank>{
              prepare::PreparedRegisterBank::Gpr} ||
      !selection.preserved_register_name.has_value() ||
      selection.preserved_register_name->empty() ||
      !selection.preserved_register_contiguous_width.has_value() ||
      *selection.preserved_register_contiguous_width != 1 ||
      selection.preserved_occupied_register_names.empty() ||
      !selection.preserved_register_placement.has_value()) {
    return false;
  }
  if (*selection.preserved_register_name != *plan.destination_register_name) {
    out += "    mv ";
    out += *plan.destination_register_name;
    out += ", ";
    out += *selection.preserved_register_name;
    out += "\n";
  }
  return true;
}

bool emit_riscv_callee_saved_gpr_preservation_effect(
    std::string& out,
    const c4c::backend::prepare::PreparedCallBoundaryEffectPlan& effect,
    c4c::backend::prepare::PreparedCallBoundaryEffectKind effect_kind,
    c4c::backend::prepare::PreparedMovePhase phase) {
  namespace prepare = c4c::backend::prepare;

  if (effect.effect_kind != effect_kind || effect.phase != phase ||
      effect.classification_status !=
          prepare::PreparedCallBoundaryMoveClassificationStatus::Available ||
      effect.preservation_route !=
          prepare::PreparedCallPreservationRoute::CalleeSavedRegister) {
    return false;
  }

  const auto& source = effect.source;
  const auto& destination = effect.destination;
  if (source.storage_kind != prepare::PreparedMoveStorageKind::Register ||
      destination.storage_kind != prepare::PreparedMoveStorageKind::Register ||
      source.register_bank !=
          std::optional<prepare::PreparedRegisterBank>{
              prepare::PreparedRegisterBank::Gpr} ||
      destination.register_bank !=
          std::optional<prepare::PreparedRegisterBank>{
              prepare::PreparedRegisterBank::Gpr} ||
      !source.register_name.has_value() ||
      source.register_name->empty() ||
      !destination.register_name.has_value() ||
      destination.register_name->empty() ||
      source.contiguous_width != 1 ||
      destination.contiguous_width != 1 ||
      source.occupied_register_names.size() > 1 ||
      destination.occupied_register_names.size() > 1) {
    return false;
  }

  if (*source.register_name != *destination.register_name) {
    out += "    mv ";
    out += *destination.register_name;
    out += ", ";
    out += *source.register_name;
    out += "\n";
  }
  return true;
}

bool emit_riscv_stack_copy_chunk(std::string& out,
                                 std::size_t source_offset,
                                 std::size_t destination_offset,
                                 std::size_t width_bytes) {
  if (source_offset >
          static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()) ||
      destination_offset >
          static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()) ||
      !fits_signed_12_bit_immediate(static_cast<std::int64_t>(source_offset)) ||
      !fits_signed_12_bit_immediate(
          static_cast<std::int64_t>(destination_offset))) {
    return false;
  }

  const auto source_text = std::to_string(source_offset);
  const auto destination_text = std::to_string(destination_offset);
  switch (width_bytes) {
    case 8:
      out += "    ld t3, " + source_text + "(sp)\n";
      out += "    sd t3, " + destination_text + "(sp)\n";
      return true;
    case 4:
      out += "    lw t3, " + source_text + "(sp)\n";
      out += "    sw t3, " + destination_text + "(sp)\n";
      return true;
    case 2:
      out += "    lh t3, " + source_text + "(sp)\n";
      out += "    sh t3, " + destination_text + "(sp)\n";
      return true;
    case 1:
      out += "    lb t3, " + source_text + "(sp)\n";
      out += "    sb t3, " + destination_text + "(sp)\n";
      return true;
    default:
      return false;
  }
}

bool emit_riscv_stack_copy_chunk_range(std::string& out,
                                       std::size_t source_offset,
                                       std::size_t destination_offset,
                                       std::size_t size_bytes) {
  std::size_t byte_offset = 0;
  while (byte_offset < size_bytes) {
    std::size_t width = 1;
    const std::size_t remaining = size_bytes - byte_offset;
    if (remaining >= 8 && (source_offset + byte_offset) % 8 == 0 &&
        (destination_offset + byte_offset) % 8 == 0) {
      width = 8;
    } else if (remaining >= 4 && (source_offset + byte_offset) % 4 == 0 &&
               (destination_offset + byte_offset) % 4 == 0) {
      width = 4;
    } else if (remaining >= 2 && (source_offset + byte_offset) % 2 == 0 &&
               (destination_offset + byte_offset) % 2 == 0) {
      width = 2;
    }
    if (!emit_riscv_stack_copy_chunk(out,
                                     source_offset + byte_offset,
                                     destination_offset + byte_offset,
                                     width)) {
      return false;
    }
    byte_offset += width;
  }
  return true;
}

bool emit_riscv_byval_aggregate_address_argument(
    std::string& out,
    const c4c::backend::prepare::PreparedCallArgumentPlan& plan,
    const c4c::backend::bir::CallArgAbiInfo* argument_abi,
    std::size_t& active_stack_adjustment_bytes) {
  namespace prepare = c4c::backend::prepare;

  const auto* selection =
      plan.source_selection.has_value() ? &*plan.source_selection : nullptr;
  const auto* transport =
      plan.aggregate_transport.has_value() ? &*plan.aggregate_transport : nullptr;
  const auto destination_register = riscv_gpr_argument_register(plan.arg_index);
  if (!destination_register.has_value() ||
      plan.value_bank != prepare::PreparedRegisterBank::AggregateAddress ||
      plan.source_encoding != prepare::PreparedStorageEncodingKind::Register ||
      plan.source_register_bank !=
          std::optional<prepare::PreparedRegisterBank>{
              prepare::PreparedRegisterBank::Gpr} ||
      !plan.source_register_name.has_value() ||
      plan.source_register_name->empty() ||
      plan.destination_register_bank.has_value() ||
      plan.destination_register_name.has_value() ||
      plan.destination_contiguous_width != 1 ||
      plan.destination_stack_offset_bytes.has_value() ||
      plan.destination_stack_size_bytes.has_value() ||
      selection == nullptr ||
      selection->kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::
              LocalFrameAddressMaterialization ||
      !selection->source_stack_offset_bytes.has_value() ||
      transport == nullptr ||
      transport->kind != prepare::PreparedAggregateTransportKind::StackCopy ||
      !transport->source_stack_offset_bytes.has_value() ||
      transport->source_stack_offset_bytes != selection->source_stack_offset_bytes ||
      transport->destination_stack_offset_bytes.has_value() ||
      transport->destination_stack_size_bytes.has_value() ||
      transport->payload_size_bytes == 0 ||
      transport->copy_size_bytes == 0 ||
      transport->copy_align_bytes == 0 ||
      transport->payload_align_bytes == 0 ||
      transport->copy_size_bytes > transport->payload_size_bytes ||
      transport->copy_size_bytes > 128 ||
      !transport->lanes.empty()) {
    return false;
  }

  if (argument_abi == nullptr ||
      argument_abi->type != c4c::backend::bir::TypeKind::Ptr ||
      !argument_abi->byval_copy ||
      argument_abi->primary_class != c4c::backend::bir::AbiValueClass::Memory ||
      argument_abi->size_bytes != transport->copy_size_bytes ||
      argument_abi->align_bytes != transport->copy_align_bytes) {
    return false;
  }

  const std::size_t stack_copy_size =
      align_riscv_stack_slot(transport->copy_size_bytes, 16);
  if (stack_copy_size == 0 ||
      stack_copy_size >
          static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()) ||
      active_stack_adjustment_bytes >
          std::numeric_limits<std::size_t>::max() - stack_copy_size ||
      !fits_signed_12_bit_immediate(-static_cast<std::int64_t>(stack_copy_size))) {
    return false;
  }
  const std::size_t pending_stack_adjustment =
      active_stack_adjustment_bytes + stack_copy_size;
  std::string payload_out;
  payload_out += "    addi sp, sp, -";
  payload_out += std::to_string(stack_copy_size);
  payload_out += "\n";
  std::vector<bool> covered(transport->copy_size_bytes, false);
  for (const auto& chunk : transport->chunks) {
    if (*transport->source_stack_offset_bytes >
        std::numeric_limits<std::size_t>::max() - transport->payload_size_bytes) {
      return false;
    }
    if (chunk.kind != prepare::PreparedAggregateTransportChunkKind::RequiredPayload ||
        chunk.size_bytes == 0 ||
        chunk.payload_offset_bytes > transport->copy_size_bytes ||
        chunk.destination_offset_bytes > transport->copy_size_bytes ||
        chunk.size_bytes > transport->copy_size_bytes - chunk.payload_offset_bytes ||
        chunk.size_bytes > transport->copy_size_bytes - chunk.destination_offset_bytes ||
        chunk.source_offset_bytes >
            std::numeric_limits<std::size_t>::max() - pending_stack_adjustment) {
      return false;
    }
    const std::size_t adjusted_source_offset =
        chunk.source_offset_bytes + pending_stack_adjustment;
    if (!emit_riscv_stack_copy_chunk_range(payload_out,
                                           adjusted_source_offset,
                                           chunk.destination_offset_bytes,
                                           chunk.size_bytes)) {
      return false;
    }
    for (std::size_t byte = 0; byte < chunk.size_bytes; ++byte) {
      const std::size_t destination_byte = chunk.destination_offset_bytes + byte;
      if (covered[destination_byte]) {
        return false;
      }
      covered[destination_byte] = true;
    }
  }
  for (bool byte_covered : covered) {
    if (!byte_covered) {
      return false;
    }
  }
  if (covered.empty()) {
    return false;
  }
  payload_out += "    mv ";
  payload_out += *destination_register;
  payload_out += ", sp\n";
  out += payload_out;
  active_stack_adjustment_bytes = pending_stack_adjustment;
  return true;
}

}  // namespace

std::optional<std::string> emit_riscv_simple_call(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    const c4c::backend::bir::CallInst& call,
    std::size_t block_index,
    const PreparedCurrentInstructionContext& context) {
  namespace prepare = c4c::backend::prepare;

  if (call.args.size() > 8 || context.lookups == nullptr) {
    return std::nullopt;
  }

  const auto* call_plan = prepare::find_indexed_prepared_call_plan(
      &context.lookups->call_plans,
      nullptr,
      block_index,
      context.instruction_index);
  if (call_plan == nullptr ||
      call_plan->variadic_fpr_arg_register_count != 0 ||
      call_plan->memory_return.has_value() ||
      call_plan->outgoing_stack_argument_area.has_value() ||
      call_plan->arguments.size() != call.args.size() ||
      call_plan->result.has_value() != call.result.has_value()) {
    return std::nullopt;
  }

  const bool direct_call =
      !call.is_indirect &&
      !call.callee.empty() &&
      !call_plan->is_indirect &&
      !call_plan->indirect_callee.has_value() &&
      call_plan->direct_callee_name == std::optional<std::string>{call.callee} &&
      (call_plan->wrapper_kind == prepare::PreparedCallWrapperKind::SameModule ||
       call_plan->wrapper_kind ==
           prepare::PreparedCallWrapperKind::DirectExternFixedArity);
  const bool indirect_call =
      call.is_indirect &&
      call_plan->is_indirect &&
      call_plan->wrapper_kind == prepare::PreparedCallWrapperKind::Indirect &&
      !call_plan->direct_callee_name.has_value() &&
      call_plan->indirect_callee.has_value() &&
      call_plan->indirect_callee->encoding ==
          prepare::PreparedStorageEncodingKind::Register &&
      call_plan->indirect_callee->bank == prepare::PreparedRegisterBank::Gpr &&
      call_plan->indirect_callee->register_name.has_value() &&
      !call_plan->indirect_callee->register_name->empty();
  if (!direct_call && !indirect_call) {
    return std::nullopt;
  }

  std::string out;
  std::size_t active_stack_adjustment_bytes = 0;
  const auto* before_call_bundle = prepare::find_indexed_prepared_move_bundle(
      &context.lookups->move_bundles,
      nullptr,
      prepare::PreparedMovePhase::BeforeCall,
      block_index,
      context.instruction_index);
  const auto before_call_effects =
      prepare::plan_prepared_call_boundary_effects(
          *call_plan,
          before_call_bundle,
          nullptr);
  for (const auto& effect : before_call_effects) {
    if (effect.effect_kind !=
        prepare::PreparedCallBoundaryEffectKind::PreservationHomePopulation) {
      continue;
    }
    if (!emit_riscv_callee_saved_gpr_preservation_effect(
            out,
            effect,
            prepare::PreparedCallBoundaryEffectKind::PreservationHomePopulation,
            prepare::PreparedMovePhase::BeforeCall)) {
      return std::nullopt;
    }
  }

  for (std::size_t arg_index = 0; arg_index < call.args.size(); ++arg_index) {
    const auto* plan = prepare::find_indexed_prepared_immediate_call_argument(
        &context.lookups->call_plans,
        block_index,
        context.instruction_index,
        arg_index);
    if (plan == nullptr && arg_index < call_plan->arguments.size()) {
      plan = &call_plan->arguments[arg_index];
    }
    if (plan == nullptr || plan->arg_index != arg_index) {
      return std::nullopt;
    }
    if (emit_riscv_byval_aggregate_address_argument(
            out,
            *plan,
            arg_index < call.arg_abi.size() ? &call.arg_abi[arg_index] : nullptr,
            active_stack_adjustment_bytes)) {
      continue;
    }
    if (plan->value_bank != prepare::PreparedRegisterBank::Gpr ||
        plan->destination_register_bank !=
            std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} ||
        !plan->destination_register_name.has_value() ||
        plan->destination_register_name->empty() ||
        plan->destination_contiguous_width != 1 ||
        plan->destination_stack_offset_bytes.has_value() ||
        plan->destination_stack_size_bytes.has_value() ||
        plan->aggregate_transport.has_value()) {
      return std::nullopt;
    }
    const auto* source_selection =
        plan->source_selection.has_value() ? &*plan->source_selection : nullptr;
    if (source_selection != nullptr &&
        source_selection->kind ==
            prepare::PreparedCallArgumentSourceSelectionKind::
                LocalFrameAddressMaterialization) {
      if (!source_selection->source_stack_offset_bytes.has_value() ||
          *source_selection->source_stack_offset_bytes >
              static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()) ||
          *source_selection->source_stack_offset_bytes >
              static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()) -
                  active_stack_adjustment_bytes ||
          !fits_signed_12_bit_immediate(static_cast<std::int64_t>(
              *source_selection->source_stack_offset_bytes +
              active_stack_adjustment_bytes))) {
        return std::nullopt;
      }
      out += "    addi " + *plan->destination_register_name + ", sp, " +
             std::to_string(*source_selection->source_stack_offset_bytes +
                            active_stack_adjustment_bytes) + "\n";
    } else if (source_selection != nullptr &&
               source_selection->kind ==
                   prepare::PreparedCallArgumentSourceSelectionKind::
                       PriorPreservation) {
      if (!emit_riscv_prior_preserved_gpr_argument(out, *plan)) {
        return std::nullopt;
      }
    } else if (plan->source_encoding ==
            prepare::PreparedStorageEncodingKind::Register &&
        plan->source_register_bank ==
            std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} &&
        plan->source_register_name.has_value() &&
        !plan->source_register_name->empty()) {
      if (emit_riscv_frame_slot_address_argument(out,
                                                 prepared,
                                                 function_name,
                                                 *plan,
                                                 context,
                                                 active_stack_adjustment_bytes)) {
        continue;
      }
      if (*plan->source_register_name != *plan->destination_register_name) {
        out += "    mv " + *plan->destination_register_name + ", " +
               *plan->source_register_name + "\n";
      }
    } else {
      if (!emit_move_to_register(out,
                                 *plan->destination_register_name,
                                 context.names,
                                 context.lookups,
                                 call.args[arg_index])) {
        return std::nullopt;
      }
    }
  }

  if (direct_call) {
    out += "    call " + call.callee + "\n";
  } else {
    out += "    jalr ra, 0(" + *call_plan->indirect_callee->register_name + ")\n";
  }

  if (active_stack_adjustment_bytes != 0) {
    if (!fits_signed_12_bit_immediate(
            static_cast<std::int64_t>(active_stack_adjustment_bytes))) {
      return std::nullopt;
    }
    out += "    addi sp, sp, ";
    out += std::to_string(active_stack_adjustment_bytes);
    out += "\n";
  }

  if (call.result.has_value()) {
    if (!call_plan->result.has_value() ||
        call_plan->result->value_bank != prepare::PreparedRegisterBank::Gpr ||
        call_plan->result->source_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        call_plan->result->destination_storage_kind !=
            prepare::PreparedMoveStorageKind::Register ||
        call_plan->result->source_register_bank !=
            std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} ||
        call_plan->result->destination_register_bank !=
            std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} ||
        !call_plan->result->source_register_name.has_value() ||
        call_plan->result->source_register_name->empty() ||
        !call_plan->result->destination_register_name.has_value() ||
        call_plan->result->destination_register_name->empty() ||
        call_plan->result->source_contiguous_width != 1 ||
        call_plan->result->destination_contiguous_width != 1 ||
        call_plan->result->source_stack_offset_bytes.has_value() ||
        call_plan->result->destination_stack_offset_bytes.has_value()) {
      return std::nullopt;
    }
    const std::string& destination_register = *call_plan->result->destination_register_name;
    const std::string& source_register = *call_plan->result->source_register_name;
    if (destination_register != source_register) {
      out += "    mv " + destination_register + ", " + source_register + "\n";
    }
  }

  const auto* after_call_bundle = prepare::find_indexed_prepared_move_bundle(
      &context.lookups->move_bundles,
      nullptr,
      prepare::PreparedMovePhase::AfterCall,
      block_index,
      context.instruction_index);
  const auto after_call_effects =
      prepare::plan_prepared_call_boundary_effects(
          *call_plan,
          nullptr,
          after_call_bundle);
  for (const auto& effect : after_call_effects) {
    if (effect.effect_kind !=
        prepare::PreparedCallBoundaryEffectKind::PreservationRepublication) {
      continue;
    }
    if (!emit_riscv_callee_saved_gpr_preservation_effect(
            out,
            effect,
            prepare::PreparedCallBoundaryEffectKind::PreservationRepublication,
            prepare::PreparedMovePhase::AfterCall)) {
      return std::nullopt;
    }
  }

  return out;
}

}  // namespace c4c::backend::riscv::codegen
