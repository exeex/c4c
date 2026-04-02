#pragma once

#include "../ir.hpp"

#include "../../codegen/lir/call_args.hpp"
#include "../../codegen/lir/ir.hpp"

#include <optional>
#include <string_view>
#include <utility>

namespace c4c::backend {

using BackendTypedCallArgView = c4c::codegen::lir::LirTypedCallArgView;
using ParsedBackendTypedCallView = c4c::codegen::lir::ParsedLirTypedCallView;

struct ParsedBackendDirectGlobalTypedCallView {
  std::string_view symbol_name;
  ParsedBackendTypedCallView typed_call;
};

inline std::optional<ParsedBackendTypedCallView> parse_backend_typed_call(
    const c4c::codegen::lir::LirCallOp& call) {
  return c4c::codegen::lir::parse_lir_typed_call_or_infer_params(call);
}

std::optional<c4c::codegen::lir::ParsedLirDirectGlobalTypedCallView>
parse_backend_direct_global_typed_call(const c4c::codegen::lir::LirCallOp& call);
std::optional<ParsedBackendTypedCallView> parse_backend_typed_call(
    const BackendCallInst& call);
std::optional<ParsedBackendDirectGlobalTypedCallView> parse_backend_direct_global_typed_call(
    const BackendCallInst& call);

}  // namespace c4c::backend
