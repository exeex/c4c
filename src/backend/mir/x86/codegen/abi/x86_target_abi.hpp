#pragma once

#include "../../../../backend.hpp"

#include <string>
#include <string_view>

namespace c4c::backend::x86::abi {

[[nodiscard]] bool is_x86_target(const c4c::TargetProfile& target_profile);
[[nodiscard]] std::string render_asm_symbol_name(std::string_view target_triple,
                                                 std::string_view logical_name);
[[nodiscard]] std::string render_private_data_label(std::string_view target_triple,
                                                    std::string_view pool_name);

}  // namespace c4c::backend::x86::abi
