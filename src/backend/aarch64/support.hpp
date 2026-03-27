#pragma once

#include <stdexcept>
#include <string>

namespace c4c::backend::aarch64 {

[[noreturn]] inline void fail_unsupported(const std::string& detail) {
  throw std::invalid_argument("aarch64 backend emitter does not support " + detail);
}

}  // namespace c4c::backend::aarch64
