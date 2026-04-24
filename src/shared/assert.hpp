#pragma once

#include <sstream>
#include <stdexcept>
#include <string_view>

namespace c4c::support {

[[noreturn]] inline void fail_internal(std::string_view kind,
                                       std::string_view detail,
                                       const char* expr,
                                       const char* file,
                                       int line) {
  std::ostringstream msg;
  msg << kind;
  if (!detail.empty()) msg << ": " << detail;
  if (expr && *expr) msg << " (" << expr << ")";
  msg << " @ " << file << ":" << line;
  throw std::runtime_error(msg.str());
}

[[noreturn]] inline void assertion_failed(const char* expr,
                                          const char* file,
                                          int line,
                                          std::string_view detail = {}) {
  fail_internal("C4C_ASSERT failed", detail, expr, file, line);
}

[[noreturn]] inline void unreachable(const char* file,
                                     int line,
                                     std::string_view detail = {}) {
  fail_internal("C4C_UNREACHABLE reached", detail, nullptr, file, line);
}

}  // namespace c4c::support

#define C4C_ASSERT(expr)                                                       \
  do {                                                                         \
    if (!(expr)) {                                                             \
      ::c4c::support::assertion_failed(#expr, __FILE__, __LINE__);             \
    }                                                                          \
  } while (false)

#define C4C_ASSERT_MSG(expr, detail)                                           \
  do {                                                                         \
    if (!(expr)) {                                                             \
      ::c4c::support::assertion_failed(#expr, __FILE__, __LINE__, (detail));   \
    }                                                                          \
  } while (false)

#define C4C_UNREACHABLE(detail)                                                \
  do {                                                                         \
    ::c4c::support::unreachable(__FILE__, __LINE__, (detail));                 \
  } while (false)
