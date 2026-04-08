#pragma once

#include "../ir.hpp"

#include "../../codegen/lir/call_args.hpp"
#include "../../codegen/lir/ir.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend {

class LirAdapterError : public std::invalid_argument {
 public:
  static LirAdapterError unsupported(const std::string& message) {
    return LirAdapterError(true, message);
  }

  static LirAdapterError malformed(const std::string& message) {
    return LirAdapterError(false, message);
  }

  bool is_unsupported() const noexcept { return is_unsupported_; }

 private:
  explicit LirAdapterError(bool is_unsupported, const std::string& message)
      : std::invalid_argument(message), is_unsupported_(is_unsupported) {}

  bool is_unsupported_;
};

}  // namespace c4c::backend
