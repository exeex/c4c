#include "calls.hpp"

#include <algorithm>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
namespace abi = c4c::backend::aarch64::abi;

namespace {

constexpr std::size_t kStackPointerAlignmentBytes = 16;

[[nodiscard]] std::size_t align_to(std::size_t value, std::size_t alignment) {
  if (alignment == 0) {
    return value;
  }
  const auto remainder = value % alignment;
  return remainder == 0 ? value : value + (alignment - remainder);
}

}  // namespace

[[nodiscard]] std::size_t outgoing_stack_argument_bytes(
    const prepare::PreparedCallPlan& call_plan) {
  std::size_t bytes = 0;
  for (const auto& argument : call_plan.arguments) {
    if (!argument.destination_stack_offset_bytes.has_value() ||
        !argument.destination_stack_size_bytes.has_value()) {
      continue;
    }
    bytes = std::max(bytes,
                     *argument.destination_stack_offset_bytes +
                         *argument.destination_stack_size_bytes);
  }
  return align_to(bytes, kStackPointerAlignmentBytes);
}

[[nodiscard]] abi::RegisterReference outgoing_stack_argument_base_register() {
  return abi::x_register(16);
}

[[nodiscard]] std::optional<abi::RegisterView> scalar_integer_register_view_from_size(
    std::size_t size_bytes) {
  if (size_bytes > 0 && size_bytes <= 4) {
    return abi::RegisterView::W;
  }
  if (size_bytes == 8) {
    return abi::RegisterView::X;
  }
  return std::nullopt;
}

}  // namespace c4c::backend::aarch64::codegen
