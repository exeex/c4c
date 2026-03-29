#pragma once

#include "types.hpp"

#include <string>
#include <vector>

namespace c4c::backend::aarch64::assembler {

std::vector<AsmStatement> parse_asm(const std::string& text);

}  // namespace c4c::backend::aarch64::assembler
