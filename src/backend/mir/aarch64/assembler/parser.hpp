#pragma once

#include "types.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::assembler {

// Parses external assembler text for the staged compatibility assembler path.
// Internal backend codegen, encoder, object writer, and linker work must not
// recover semantics by feeding printed machine_printer.cpp --codegen asm output
// through this parser; future compile-through input belongs in structured
// machine instruction or asm/encoding records.
std::vector<AsmStatement> parse_asm(const std::string& text);
std::string trim_asm(std::string_view text);

}  // namespace c4c::backend::aarch64::assembler
