#pragma once

#include <stdexcept>
#include <string>

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
