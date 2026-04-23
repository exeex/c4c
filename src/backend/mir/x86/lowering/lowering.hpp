#pragma once

#include "../../../bir/bir.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace c4c::backend::x86::lowering {

// Lowering helpers are encoding helpers only. They may translate prepared
// frame/call/storage plans into x86 operand and instruction text, but they
// must not derive new regalloc, frame, or call-placement policy. GPR, FPR,
// and VREG selection belongs to `prealloc/`, not to x86 lowering. When VLA or
// dynamic alloca is present, fixed-slot references must follow the published
// frame/dynamic-stack contract rather than guessing from transient `rsp`.

[[nodiscard]] std::string_view atomics_summary();
[[nodiscard]] std::string_view call_summary();
[[nodiscard]] std::string_view compare_summary();
[[nodiscard]] std::string_view float_summary();
[[nodiscard]] std::string_view ret_summary();
[[nodiscard]] std::string_view scalar_summary();
[[nodiscard]] std::string frame_comment(std::string_view function_name);
[[nodiscard]] std::optional<std::string_view> memory_size(c4c::backend::bir::TypeKind type);
[[nodiscard]] std::string stack_mem(std::size_t byte_offset, std::string_view size_name);

}  // namespace c4c::backend::x86::lowering
