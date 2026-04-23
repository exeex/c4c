#include "route_debug.hpp"
#include "debug/prepared_route_debug.hpp"

namespace c4c::backend::x86 {

std::string summarize_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function,
    std::optional<std::string_view> focus_block) {
  return c4c::backend::x86::debug::summarize_prepared_module_routes(
      module, focus_function, focus_block);
}

std::string trace_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function,
    std::optional<std::string_view> focus_block) {
  return c4c::backend::x86::debug::trace_prepared_module_routes(
      module, focus_function, focus_block);
}

}  // namespace c4c::backend::x86
