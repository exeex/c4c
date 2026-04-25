#pragma once

// Exported LIR call-operation helpers.
//
// This adapter binds the call-argument helpers to `LirCallOp`; include it from
// printer, verifier, HIR-to-LIR, or backend code that operates on LIR call ops.

#include "call_args.hpp"
#include "ir.hpp"

namespace c4c::codegen::lir {

inline std::vector<LirTypeRef> lir_call_arg_type_refs(
    const std::vector<OwnedLirTypedCallArg>& args) {
  const bool has_explicit_mirror =
      std::any_of(args.begin(), args.end(), [](const OwnedLirTypedCallArg& arg) {
        return !arg.type_ref.empty();
      });
  if (!has_explicit_mirror) return {};

  std::vector<LirTypeRef> refs;
  refs.reserve(args.size());
  for (const auto& arg : args) {
    refs.push_back(arg.type_ref.empty() ? LirTypeRef(arg.type) : arg.type_ref);
  }
  return refs;
}

inline LirCallOp make_lir_call_op_with_return_type_ref(
    std::string result,
    LirTypeRef return_type,
    std::string callee,
    std::string_view callee_type_suffix,
    const std::vector<OwnedLirTypedCallArg>& args,
    LinkNameId direct_callee_link_name_id = kInvalidLinkName) {
  const auto formatted = format_lir_call_fields(callee_type_suffix, args);
  return LirCallOp{std::string(trim_lir_arg_text(result)),
                   std::move(return_type),
                   std::string(trim_lir_arg_text(callee)),
                   direct_callee_link_name_id,
                   formatted.callee_type_suffix,
                   formatted.args_str,
                   lir_call_arg_type_refs(args)};
}

inline LirCallOp make_lir_call_op(std::string result,
                                  std::string return_type,
                                  std::string callee,
                                  std::string_view callee_type_suffix,
                                  const std::vector<OwnedLirTypedCallArg>& args,
                                  LinkNameId direct_callee_link_name_id = kInvalidLinkName) {
  return make_lir_call_op_with_return_type_ref(
      std::move(result),
      LirTypeRef(std::string(trim_lir_arg_text(return_type))),
      std::move(callee),
      callee_type_suffix,
      args,
      direct_callee_link_name_id);
}

inline std::optional<ParsedLirTypedCallView> parse_lir_typed_call(
    const LirCallOp& call) {
  return parse_lir_typed_call(call.callee_type_suffix, call.args_str);
}

inline std::optional<ParsedLirTypedCallView> parse_lir_typed_call_or_infer_params(
    const LirCallOp& call) {
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
