#pragma once

#include "types.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::assembler {

std::vector<AsmStatement> parse_asm(const std::string& text);
std::string trim_asm(std::string_view text);

}  // namespace c4c::backend::aarch64::assembler
