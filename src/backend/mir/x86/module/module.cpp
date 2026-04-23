#include "module.hpp"

#include "../abi/abi.hpp"
#include "../core/core.hpp"

namespace c4c::backend::x86::module {

namespace {

void append_function_stub(c4c::backend::x86::core::Text& out,
                          const c4c::backend::bir::Function& function,
                          const Data& data) {
  const auto symbol_name = data.render_asm_symbol_name(function.name);
  out.append_line(".globl " + symbol_name);
  out.append_line(".type " + symbol_name + ", @function");
  out.append_line(symbol_name + ":");
  out.append_line("    # x86 backend contract-first stub");
  out.append_line("    # behavior recovery will replace this body");
  if (function.return_type != c4c::backend::bir::TypeKind::Void) {
    out.append_line("    xor eax, eax");
  }
  out.append_line("    ret");
}

}  // namespace

std::string emit(const c4c::backend::prepare::PreparedBirModule& module) {
  const auto target_profile = c4c::backend::x86::abi::resolve_target_profile(module);
  if (!c4c::backend::x86::abi::is_x86_target(target_profile)) {
    throw std::invalid_argument("x86::module::emit requires an x86 target profile");
  }

  const auto target_triple = c4c::backend::x86::abi::resolve_target_triple(module);
  const auto data = make_data(module, target_triple);

  c4c::backend::x86::core::Text out;
  out.append_line(".intel_syntax noprefix");
  out.append_line(".text");
  out.append_line("# x86 backend contract-first module emitter");

  bool emitted_any_function = false;
  for (const auto& function : module.module.functions) {
    if (function.is_declaration) {
      continue;
    }
    emitted_any_function = true;
    append_function_stub(out, function, data);
  }

  if (!emitted_any_function) {
    out.append_line("# no defined functions");
  }

  out.append_raw(data.emit_data());
  return out.take_text();
}

}  // namespace c4c::backend::x86::module
