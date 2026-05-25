#pragma once

#include "../abi/abi.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] std::vector<std::string> materialize_integer_constant_lines(
    abi::RegisterReference destination,
    std::uint64_t value,
    unsigned width_bits);

}  // namespace c4c::backend::aarch64::codegen
