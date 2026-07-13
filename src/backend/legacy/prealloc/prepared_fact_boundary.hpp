#pragma once

#include "names.hpp"

#include <cstddef>
#include <string_view>

namespace c4c::backend::prepare {

// Status at the named-BIR-input/prepared-fact boundary.  Only Available is a
// positive prepared fact; every other state is deliberately fail closed.
enum class PreparedFactBoundaryStatus {
  Available,
  Missing,
  Incomplete,
  Ambiguous,
  Mismatched,
  Unsupported,
};

[[nodiscard]] constexpr std::string_view prepared_fact_boundary_status_name(
    PreparedFactBoundaryStatus status) {
  switch (status) {
    case PreparedFactBoundaryStatus::Available:
      return "available";
    case PreparedFactBoundaryStatus::Missing:
      return "missing";
    case PreparedFactBoundaryStatus::Incomplete:
      return "incomplete";
    case PreparedFactBoundaryStatus::Ambiguous:
      return "ambiguous";
    case PreparedFactBoundaryStatus::Mismatched:
      return "mismatched";
    case PreparedFactBoundaryStatus::Unsupported:
      return "unsupported";
  }
  return "unknown";
}

// Narrow source evidence for a prepared producer.  This binds evidence to
// stable prepared identity without carrying a BIR analysis record or index.
// It is evidence only: homes, moves, freshness and publication remain in their
// existing prepared owners.
struct PreparedFactBoundaryEvidence {
  PreparedFactBoundaryStatus status = PreparedFactBoundaryStatus::Missing;
  FunctionNameId function_name = kInvalidFunctionName;
  BlockLabelId block_label = kInvalidBlockLabel;
  ValueNameId value_name = kInvalidValueName;
  std::size_t instruction_index = 0;

  [[nodiscard]] constexpr explicit operator bool() const {
    return status == PreparedFactBoundaryStatus::Available;
  }
};

}  // namespace c4c::backend::prepare
