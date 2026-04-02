#pragma once

#include "ir.hpp"
#include "lowering/call_decode.hpp"

#include "../codegen/lir/call_args.hpp"
#include "../codegen/lir/ir.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend {

enum class LirAdapterErrorKind {
  Unsupported,
  Malformed,
};

class LirAdapterError : public std::invalid_argument {
 public:
  LirAdapterError(LirAdapterErrorKind kind, const std::string& message)
      : std::invalid_argument(message), kind_(kind) {}

  LirAdapterErrorKind kind() const noexcept { return kind_; }

 private:
  LirAdapterErrorKind kind_;
};

BackendModule lower_to_backend_ir(const c4c::codegen::lir::LirModule& module);
inline BackendModule adapt_minimal_module(const c4c::codegen::lir::LirModule& module) {
  return lower_to_backend_ir(module);
}
std::string render_module(const BackendModule& module);

}  // namespace c4c::backend
