#pragma once

#include "call_args.hpp"
#include "ir.hpp"

namespace c4c::codegen::lir {

inline LirCallOp make_lir_call_op(std::string result,
                                  std::string return_type,
                                  std::string callee,
                                  std::string_view callee_type_suffix,
                                  const std::vector<OwnedLirTypedCallArg>& args,
                                  LinkNameId direct_callee_link_name_id = kInvalidLinkName) {
  const auto formatted = format_lir_call_fields(callee_type_suffix, args);
  return LirCallOp{std::string(trim_lir_arg_text(result)),
                   std::string(trim_lir_arg_text(return_type)),
                   std::string(trim_lir_arg_text(callee)),
                   direct_callee_link_name_id,
                   formatted.callee_type_suffix,
                   formatted.args_str};
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
