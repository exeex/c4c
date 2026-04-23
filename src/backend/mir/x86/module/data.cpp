#include "module.hpp"

#include "../abi/abi.hpp"
#include "../core/core.hpp"

namespace c4c::backend::x86::module {

std::string Data::render_asm_symbol_name(std::string_view logical_name) const {
  return c4c::backend::x86::abi::render_asm_symbol_name(target_triple, logical_name);
}

std::string Data::render_private_data_label(std::string_view pool_name) const {
  return c4c::backend::x86::abi::render_private_data_label(target_triple, pool_name);
}

std::string Data::emit_data() const {
  if (module == nullptr) {
    return {};
  }

  c4c::backend::x86::core::Text out;
  if (!module->module.string_constants.empty()) {
    out.append_line(".section .rodata");
    for (const auto& constant : module->module.string_constants) {
      out.append_line(render_private_data_label(constant.name) + ":");
      out.append_line("    # string constant deferred to behavior-recovery packet");
    }
  }

  if (!module->module.globals.empty()) {
    out.append_line(".data");
    for (const auto& global : module->module.globals) {
      out.append_line(render_asm_symbol_name(global.name) + ":");
      out.append_line("    # global data emission deferred to behavior-recovery packet");
    }
  }

  return out.take_text();
}

Data make_data(const c4c::backend::prepare::PreparedBirModule& module,
               std::string_view target_triple) {
  return Data{
      .module = &module,
      .target_triple = target_triple,
  };
}

}  // namespace c4c::backend::x86::module
