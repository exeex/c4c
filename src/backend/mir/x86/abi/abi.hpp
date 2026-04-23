#pragma once

#include "../../../backend.hpp"

#include <string>
#include <string_view>

namespace c4c::backend::x86::abi {

[[nodiscard]] std::string resolve_target_triple(
    const c4c::backend::prepare::PreparedBirModule& module);
[[nodiscard]] c4c::TargetProfile resolve_target_profile(
    const c4c::backend::prepare::PreparedBirModule& module);
[[nodiscard]] bool is_x86_target(const c4c::TargetProfile& target_profile);
[[nodiscard]] bool is_apple_darwin_target(std::string_view target_triple);
[[nodiscard]] std::string narrow_i32_register_name(std::string_view wide_register);
[[nodiscard]] std::string render_asm_symbol_name(std::string_view target_triple,
                                                 std::string_view logical_name);
[[nodiscard]] std::string render_private_data_label(std::string_view target_triple,
                                                    std::string_view pool_name);

}  // namespace c4c::backend::x86::abi
