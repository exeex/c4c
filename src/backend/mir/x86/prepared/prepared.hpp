#pragma once

#include "../../../bir/bir.hpp"
#include "../../../prealloc/prealloc.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace c4c::backend::x86::prepared {

struct Query {
  const c4c::backend::prepare::PreparedBirModule* module = nullptr;
  std::optional<c4c::FunctionNameId> function_name;

  [[nodiscard]] const c4c::backend::prepare::PreparedAddressingFunction* addressing() const {
    if (module == nullptr || !function_name.has_value()) {
      return nullptr;
    }
    return c4c::backend::prepare::find_prepared_addressing(*module, *function_name);
  }

  [[nodiscard]] const c4c::backend::prepare::PreparedValueLocationFunction* locations() const {
    if (module == nullptr || !function_name.has_value()) {
      return nullptr;
    }
    return c4c::backend::prepare::find_prepared_value_location_function(*module, *function_name);
  }
};

[[nodiscard]] inline Query make_query(const c4c::backend::prepare::PreparedBirModule& module,
                                      std::string_view function_name) {
  return Query{
      .module = &module,
      .function_name =
          c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function_name),
  };
}

struct FastPath {
  bool accepted = false;
  std::string lane;
  std::string reason;
};

struct Operand {
  std::string text;
  bool materialize = false;
};

[[nodiscard]] FastPath classify_module_fast_path(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function = std::nullopt);
[[nodiscard]] std::optional<Operand> render_immediate_operand(
    const c4c::backend::bir::Value& value);

}  // namespace c4c::backend::x86::prepared
