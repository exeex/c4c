#pragma once

// Exported LIR call-operation helpers.
//
// This adapter binds the call-argument helpers to `LirCallOp`; include it from
// printer, verifier, HIR-to-LIR, or backend code that operates on LIR call ops.

#include "call_args.hpp"
#include "ir.hpp"

namespace c4c::codegen::lir {

inline bool lir_call_param_type_accepts_arg_type(std::string_view param_type,
                                                 std::string_view arg_type) {
  param_type = trim_lir_arg_text(param_type);
  arg_type = trim_lir_arg_text(arg_type);
  if (param_type == arg_type) return true;
  return param_type == "ptr" && arg_type.rfind("ptr byval(", 0) == 0;
}

inline bool lir_call_args_are_mirrorable(const FormattedLirTypedCall& formatted) {
  if (const auto parsed = parse_lir_typed_call_or_infer_params(
          formatted.callee_type_suffix, formatted.args_str);
      parsed.has_value()) {
    return true;
  }

  const auto param_types = parse_lir_call_param_types(formatted.callee_type_suffix);
  const auto args = parse_lir_typed_call_args(formatted.args_str);
  if (!param_types.has_value() || !args.has_value() ||
      param_types->size() != args->size()) {
    return false;
  }
  for (std::size_t index = 0; index < args->size(); ++index) {
    if (!lir_call_param_type_accepts_arg_type((*param_types)[index],
                                              (*args)[index].type)) {
      return false;
    }
  }
  return true;
}

inline std::vector<LirTypeRef> lir_call_arg_type_refs(
    const FormattedLirTypedCall& formatted,
    const std::vector<OwnedLirTypedCallArg>& args) {
  const bool has_explicit_mirror =
      std::any_of(args.begin(), args.end(), [](const OwnedLirTypedCallArg& arg) {
        return !arg.type_ref.empty();
      });
  if (!has_explicit_mirror) return {};

  if (!lir_call_args_are_mirrorable(formatted)) {
    return {};
  }

  std::vector<LirTypeRef> refs;
  refs.reserve(args.size());
  for (const auto& arg : args) {
    refs.push_back(arg.type_ref.empty() ? LirTypeRef(arg.type) : arg.type_ref);
  }
  return refs;
}

inline std::vector<LirCallArg> lir_call_structured_args(
    const std::vector<OwnedLirTypedCallArg>& args) {
  std::vector<LirCallArg> structured_args;
  structured_args.reserve(args.size());
  for (const auto& arg : args) {
    structured_args.push_back(
        {.type = std::string(trim_lir_arg_text(arg.type)),
         .operand = LirOperand(std::string(trim_lir_arg_text(arg.operand))),
         .type_ref = arg.type_ref,
         .aarch64_hfa_lane_count = arg.aarch64_hfa_lane_count,
         .aarch64_hfa_lane_index = arg.aarch64_hfa_lane_index,
         .aarch64_stack_align_bytes = arg.aarch64_stack_align_bytes});
  }
  return structured_args;
}

inline LirCallOp make_lir_call_op_with_return_type_ref(
    std::string result,
    LirTypeRef return_type,
    std::string callee,
    std::string_view callee_type_suffix,
    const std::vector<OwnedLirTypedCallArg>& args,
    LinkNameId direct_callee_link_name_id = kInvalidLinkName,
    std::optional<LirCallSignature> callee_signature = std::nullopt) {
  const auto formatted = format_lir_call_fields(callee_type_suffix, args);
  return LirCallOp{std::string(trim_lir_arg_text(result)),
                   std::move(return_type),
                   std::string(trim_lir_arg_text(callee)),
                   direct_callee_link_name_id,
                   formatted.callee_type_suffix,
                   formatted.args_str,
                   lir_call_arg_type_refs(formatted, args),
                   std::move(callee_signature),
                   lir_call_structured_args(args)};
}

inline LirCallOp make_lir_call_op(std::string result,
                                  std::string return_type,
                                  std::string callee,
                                  std::string_view callee_type_suffix,
                                  const std::vector<OwnedLirTypedCallArg>& args,
                                  LinkNameId direct_callee_link_name_id = kInvalidLinkName,
                                  std::optional<LirCallSignature> callee_signature = std::nullopt) {
  return make_lir_call_op_with_return_type_ref(
      std::move(result),
      LirTypeRef(std::string(trim_lir_arg_text(return_type))),
      std::move(callee),
      callee_type_suffix,
      args,
      direct_callee_link_name_id,
      std::move(callee_signature));
}

inline std::optional<ParsedLirTypedCallView> parse_lir_typed_call(
    const LirCallOp& call) {
  return parse_lir_typed_call(call.callee_type_suffix, call.args_str);
}

inline std::optional<ParsedLirTypedCallView> parse_lir_typed_call_or_infer_params(
    const LirCallOp& call) {
  if (call.callee_signature.has_value() &&
      !call.callee_signature->has_unspecified_params) {
    const auto args = parse_lir_typed_call_args(call.args_str);
    if (!args.has_value()) {
      return std::nullopt;
    }

    const LirCallSignature& sig = *call.callee_signature;
    if (sig.has_void_param_list) {
      if (!sig.fixed_param_types.empty() || sig.is_variadic || !args->empty()) {
        return std::nullopt;
      }
      return ParsedLirTypedCallView{{}, *args};
    }

    const std::size_t fixed_count = sig.fixed_param_types.size();
    if ((!sig.is_variadic && args->size() != fixed_count) ||
        (sig.is_variadic && args->size() < fixed_count)) {
      return std::nullopt;
    }

    std::vector<std::string_view> param_types;
    param_types.reserve(args->size());
    for (std::size_t index = 0; index < args->size(); ++index) {
      if (index < fixed_count) {
        if (!lir_call_param_type_accepts_arg_type(sig.fixed_param_types[index],
                                                  (*args)[index].type) &&
            trim_lir_arg_text(sig.fixed_param_types[index]) !=
                trim_lir_arg_text((*args)[index].type)) {
          return std::nullopt;
        }
        param_types.push_back(sig.fixed_param_types[index]);
      } else {
        param_types.push_back((*args)[index].type);
      }
    }
    return ParsedLirTypedCallView{std::move(param_types), *args};
  }
  return parse_lir_typed_call_or_infer_params(call.callee_type_suffix, call.args_str);
}

inline std::optional<ParsedLirDirectGlobalTypedCallView>
parse_lir_direct_global_typed_call(const LirCallOp& call) {
  return parse_lir_direct_global_typed_call(
      call.callee, call.callee_type_suffix, call.args_str);
}

inline bool lir_call_has_no_args(const LirCallOp& call) {
  return lir_call_has_no_args(call.callee_type_suffix, call.args_str);
}

inline void collect_lir_value_names_from_call(const LirCallOp& call,
                                              std::vector<std::string>& values) {
  collect_lir_value_names_from_call(call.callee, call.args_str, values);
}

template <typename Fn>
inline void collect_lir_global_symbol_refs_from_call(const LirCallOp& call, Fn&& visit) {
  collect_lir_global_symbol_refs_from_call(
      call.callee, call.args_str, std::forward<Fn>(visit));
}

template <typename Fn>
inline void rewrite_lir_call_operands(LirCallOp& call, Fn&& rewrite_operand) {
  rewrite_lir_call_operands(call.callee, call.args_str, std::forward<Fn>(rewrite_operand));
}

inline std::string format_lir_call_site(const LirCallOp& call) {
  return format_lir_call_site(call.callee, call.callee_type_suffix, call.args_str);
}

}  // namespace c4c::codegen::lir
