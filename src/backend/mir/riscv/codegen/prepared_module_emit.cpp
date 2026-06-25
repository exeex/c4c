#include "prepared_module_emit.hpp"

#include "emit.hpp"
#include "prepared_function_emit.hpp"
#include "prepared_global_memory_emit.hpp"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace c4c::backend::riscv::codegen {
namespace {

bool is_supported_rv64_runtime_external_callee(std::string_view callee) {
  return callee == "strlen";
}

bool is_inline_asm_callee(const c4c::backend::prepare::PreparedCallPlan& call) {
  return call.direct_callee_name == std::optional<std::string>{"llvm.inline_asm"};
}

std::optional<std::string_view> unsupported_external_call_reason(
    const c4c::backend::prepare::PreparedCallPlan& call) {
  namespace prepare = c4c::backend::prepare;

  if (is_inline_asm_callee(call)) {
    return std::nullopt;
  }

  switch (call.wrapper_kind) {
    case prepare::PreparedCallWrapperKind::SameModule:
    case prepare::PreparedCallWrapperKind::Indirect:
      return std::nullopt;
    case prepare::PreparedCallWrapperKind::DirectExternVariadic:
      return std::string_view{"variadic external calls"};
    case prepare::PreparedCallWrapperKind::DirectExternFixedArity:
      break;
  }

  if (call.is_indirect || call.indirect_callee.has_value()) {
    return std::string_view{"indirect external call plan"};
  }
  if (!call.direct_callee_name.has_value() || call.direct_callee_name->empty()) {
    return std::string_view{"missing direct external callee"};
  }
  if (!is_supported_rv64_runtime_external_callee(*call.direct_callee_name)) {
    return std::string_view{"unsupported runtime external symbol"};
  }
  if (call.variadic_fpr_arg_register_count != 0) {
    return std::string_view{"variadic floating-point register metadata"};
  }
  if (call.memory_return.has_value()) {
    return std::string_view{"external memory returns"};
  }
  if (call.outgoing_stack_argument_area.has_value()) {
    return std::string_view{"external stack arguments"};
  }
  if (call.arguments.size() > 8) {
    return std::string_view{"more than eight external arguments"};
  }

  for (const auto& argument : call.arguments) {
    if (argument.value_bank != prepare::PreparedRegisterBank::Gpr ||
        argument.destination_register_bank !=
            std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} ||
        !argument.destination_register_name.has_value() ||
        argument.destination_register_name->empty() ||
        argument.destination_contiguous_width != 1 ||
        argument.destination_stack_offset_bytes.has_value() ||
        argument.destination_stack_size_bytes.has_value() ||
        argument.aggregate_transport.has_value()) {
      return std::string_view{"non-scalar-GPR external arguments"};
    }
  }

  if (call.result.has_value()) {
    const auto& result = *call.result;
    if (result.value_bank != prepare::PreparedRegisterBank::Gpr ||
        result.source_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        result.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        result.source_register_bank !=
            std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} ||
        result.destination_register_bank !=
            std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} ||
        !result.source_register_name.has_value() ||
        result.source_register_name->empty() ||
        !result.destination_register_name.has_value() ||
        result.destination_register_name->empty() ||
        result.source_contiguous_width != 1 ||
        result.destination_contiguous_width != 1 ||
        result.source_stack_offset_bytes.has_value() ||
        result.destination_stack_offset_bytes.has_value()) {
      return std::string_view{"non-scalar-GPR external call result"};
    }
  }

  return std::nullopt;
}

void reject_unsupported_external_calls(
    const c4c::backend::prepare::PreparedFunctionLookups& lookups,
    std::string_view function_name) {
  for (const auto& [position, call] : lookups.call_plans.calls_by_position) {
    (void)position;
    if (call == nullptr) {
      continue;
    }
    const auto reason = unsupported_external_call_reason(*call);
    if (!reason.has_value()) {
      continue;
    }
    std::string diagnostic =
        "riscv prepared module emitter unsupported_external_call";
    diagnostic += " function='";
    diagnostic += function_name;
    diagnostic += "'";
    diagnostic += " callee='";
    diagnostic += call->direct_callee_name.value_or("<unknown>");
    diagnostic += "'";
    diagnostic += " wrapper_kind=";
    diagnostic += c4c::backend::prepare::prepared_call_wrapper_kind_name(
        call->wrapper_kind);
    diagnostic += " reason=";
    diagnostic += *reason;
    throw std::invalid_argument(diagnostic);
  }
}

void append_available_edge_publication_moves(
    std::string& out,
    const c4c::backend::prepare::PreparedFunctionLookups& lookups) {
  namespace prepare = c4c::backend::prepare;

  for (const auto& publication : lookups.edge_publications.publications) {
    if (publication.status != prepare::PreparedEdgePublicationLookupStatus::Available) {
      continue;
    }
    (void)append_edge_publication_move_instruction(out,
                                                   &lookups,
                                                   publication.predecessor_label,
                                                   publication.successor_label,
                                                   publication.destination_value_id);
  }
}

}  // namespace

std::string emit_prepared_module_text(
    const c4c::backend::prepare::PreparedBirModule& module) {
  namespace prepare = c4c::backend::prepare;

  std::string out;
  if (!append_prepared_global_storage_asm(out, module)) {
    throw std::invalid_argument(
        "riscv prepared module emitter does not support this prepared global storage layout");
  }
  out += "    .text\n";
  for (const auto& function : module.control_flow.functions) {
    const auto lookups = prepare::make_prepared_function_lookups(module, function);
    const auto function_name = prepare::prepared_function_name(module.names,
                                                              function.function_name);
    const auto function_it = std::find_if(
        module.module.functions.begin(),
        module.module.functions.end(),
        [&](const c4c::backend::bir::Function& candidate) {
          return candidate.name == function_name && !candidate.is_declaration;
        });
    if (function_it == module.module.functions.end()) {
      continue;
    }

    reject_unsupported_external_calls(lookups, function_name);

    if (!function_name.empty()) {
      out += "    .globl ";
      out += function_name;
      out += "\n";
      out += function_name;
      out += ":\n";
    }

    if (append_simple_prepared_bir_function_asm(out, module, &lookups, *function_it)) {
      append_available_edge_publication_moves(out, lookups);
      continue;
    }

    append_available_edge_publication_moves(out, lookups);
  }
  return out;
}

}  // namespace c4c::backend::riscv::codegen
