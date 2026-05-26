#pragma once

#include "alu.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

[[nodiscard]] bool emit_prepared_value_home_to_register(
    const prepare::PreparedStackLayout* stack_layout,
    const prepare::PreparedValueHome& home,
    bir::TypeKind type,
    std::uint8_t target_index,
    std::vector<std::string>& lines,
    bool use_frame_pointer_base = false);

[[nodiscard]] bool emit_prepared_value_home_publication_to_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const prepare::PreparedValueHome& home,
    std::uint8_t target_index,
    std::vector<std::string>& lines);

}  // namespace c4c::backend::aarch64::codegen
