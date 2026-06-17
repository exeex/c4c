#include "prepared_call_emit.hpp"

#include "prepared_emit_context.hpp"
#include "prepared_frame_emit.hpp"
#include "prepared_scalar_emit.hpp"

#include "../../../prealloc/calls.hpp"

#include <cstdint>
#include <limits>

namespace c4c::backend::riscv::codegen {

std::optional<std::string> emit_riscv_simple_call(
    const c4c::backend::bir::CallInst& call,
    std::size_t block_index,
    const PreparedCurrentInstructionContext& context) {
  namespace prepare = c4c::backend::prepare;

  if (call.is_indirect || call.callee.empty() || call.args.size() > 8 ||
      context.lookups == nullptr) {
    return std::nullopt;
  }

  const auto* call_plan = prepare::find_indexed_prepared_call_plan(
      &context.lookups->call_plans,
      nullptr,
      block_index,
      context.instruction_index);
  if (call_plan == nullptr ||
      call_plan->is_indirect ||
      call_plan->indirect_callee.has_value() ||
      call_plan->direct_callee_name != std::optional<std::string>{call.callee} ||
      (call_plan->wrapper_kind != prepare::PreparedCallWrapperKind::SameModule &&
       call_plan->wrapper_kind != prepare::PreparedCallWrapperKind::DirectExternFixedArity) ||
      call_plan->variadic_fpr_arg_register_count != 0 ||
      call_plan->memory_return.has_value() ||
      call_plan->outgoing_stack_argument_area.has_value() ||
      call_plan->arguments.size() != call.args.size() ||
      call_plan->result.has_value() != call.result.has_value()) {
    return std::nullopt;
  }

  std::string out;
  for (std::size_t arg_index = 0; arg_index < call.args.size(); ++arg_index) {
    const auto* plan = prepare::find_indexed_prepared_immediate_call_argument(
        &context.lookups->call_plans,
        block_index,
        context.instruction_index,
        arg_index);
    if (plan == nullptr && arg_index < call_plan->arguments.size()) {
      plan = &call_plan->arguments[arg_index];
    }
    if (plan == nullptr ||
        plan->arg_index != arg_index ||
        plan->value_bank != prepare::PreparedRegisterBank::Gpr ||
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
          !fits_signed_12_bit_immediate(static_cast<std::int64_t>(
              *source_selection->source_stack_offset_bytes))) {
        return std::nullopt;
      }
      out += "    addi " + *plan->destination_register_name + ", sp, " +
             std::to_string(*source_selection->source_stack_offset_bytes) + "\n";
    } else if (plan->source_encoding ==
            prepare::PreparedStorageEncodingKind::Register &&
        plan->source_register_bank ==
            std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} &&
        plan->source_register_name.has_value() &&
        !plan->source_register_name->empty()) {
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

  out += "    call " + call.callee + "\n";

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

  return out;
}

}  // namespace c4c::backend::riscv::codegen
