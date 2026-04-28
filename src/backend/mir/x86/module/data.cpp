#include "module.hpp"

#include "../abi/abi.hpp"
#include "../core/core.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>

namespace c4c::backend::x86::module {

namespace {

std::size_t p2align_for_alignment(std::size_t align_bytes) {
  if (align_bytes <= 1) {
    return 0;
  }
  std::size_t p2align = 0;
  std::size_t value = 1;
  while (value < align_bytes) {
    value <<= 1;
    ++p2align;
  }
  return p2align;
}

bool is_zero_immediate(const c4c::backend::bir::Value& value) {
  return value.kind == c4c::backend::bir::Value::Kind::Immediate && value.immediate == 0;
}

bool is_zero_initialized_global(const c4c::backend::bir::Global& global) {
  if (global.initializer.has_value()) {
    return is_zero_immediate(*global.initializer);
  }
  if (!global.initializer_elements.empty()) {
    for (const auto& element : global.initializer_elements) {
      if (!is_zero_immediate(element)) {
        return false;
      }
    }
    return true;
  }
  return global.initializer_symbol_name.has_value() ? false : global.size_bytes != 0;
}

bool has_named_pointer_initializer_element(const c4c::backend::bir::Global& global) {
  return std::any_of(global.initializer_elements.begin(),
                     global.initializer_elements.end(),
                     [](const c4c::backend::bir::Value& element) {
                       return element.kind == c4c::backend::bir::Value::Kind::Named &&
                              element.type == c4c::backend::bir::TypeKind::Ptr;
                     });
}

std::string_view logical_symbol_name(std::string_view symbol_name) {
  if (!symbol_name.empty() && symbol_name.front() == '@') {
    symbol_name.remove_prefix(1);
  }
  return symbol_name;
}

const c4c::backend::bir::Global* find_defined_same_module_global(
    const c4c::backend::bir::Module& module,
    std::string_view symbol_name) {
  const auto logical_name = logical_symbol_name(symbol_name);
  for (const auto& global : module.globals) {
    if (global.name == logical_name && !global.is_extern && !global.is_thread_local) {
      return &global;
    }
  }
  return nullptr;
}

bool symbol_name_initializer_targets_mixed_pointer_global(
    const c4c::backend::bir::Module& module,
    const c4c::backend::bir::Global& global) {
  if (!global.initializer_symbol_name.has_value()) {
    return false;
  }
  const auto* target = find_defined_same_module_global(module, *global.initializer_symbol_name);
  return target != nullptr && has_named_pointer_initializer_element(*target);
}

std::optional<std::string> render_global_initializer_directive(
    const c4c::backend::bir::Value& value,
    const c4c::backend::bir::Module& module,
    std::string_view target_triple) {
  if (value.kind == c4c::backend::bir::Value::Kind::Named) {
    if (value.type != c4c::backend::bir::TypeKind::Ptr || value.name.empty() ||
        find_defined_same_module_global(module, value.name) == nullptr) {
      return std::nullopt;
    }
    return ".quad " + c4c::backend::x86::abi::render_asm_symbol_name(
                         target_triple, logical_symbol_name(value.name));
  }
  if (value.kind != c4c::backend::bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  switch (value.type) {
    case c4c::backend::bir::TypeKind::I8:
      return ".byte " + std::to_string(static_cast<std::int8_t>(value.immediate));
    case c4c::backend::bir::TypeKind::I16:
      return ".short " + std::to_string(static_cast<std::int16_t>(value.immediate));
    case c4c::backend::bir::TypeKind::I32:
      return ".long " + std::to_string(static_cast<std::int32_t>(value.immediate));
    case c4c::backend::bir::TypeKind::I64:
    case c4c::backend::bir::TypeKind::Ptr:
      return ".quad " + std::to_string(value.immediate);
    case c4c::backend::bir::TypeKind::Void:
    case c4c::backend::bir::TypeKind::I1:
    case c4c::backend::bir::TypeKind::I128:
    case c4c::backend::bir::TypeKind::F32:
    case c4c::backend::bir::TypeKind::F64:
    case c4c::backend::bir::TypeKind::F128:
      break;
  }
  return std::nullopt;
}

bool emit_global_initializer(c4c::backend::x86::core::Text& out,
                             const c4c::backend::bir::Module& module,
                             const c4c::backend::bir::Global& global,
                             std::string_view target_triple) {
  if (global.initializer.has_value()) {
    const auto directive = render_global_initializer_directive(*global.initializer,
                                                               module,
                                                               target_triple);
    if (!directive.has_value()) {
      return false;
    }
    out.append_line("    " + *directive);
    return true;
  }
  if (!global.initializer_elements.empty()) {
    for (const auto& element : global.initializer_elements) {
      const auto directive = render_global_initializer_directive(element, module, target_triple);
      if (!directive.has_value()) {
        return false;
      }
      out.append_line("    " + *directive);
    }
    return true;
  }
  if (global.initializer_symbol_name.has_value()) {
    if (!symbol_name_initializer_targets_mixed_pointer_global(module, global)) {
      return false;
    }
    out.append_line("    .quad " +
                    c4c::backend::x86::abi::render_asm_symbol_name(
                        target_triple, logical_symbol_name(*global.initializer_symbol_name)));
    return true;
  }
  if (global.size_bytes != 0) {
    out.append_line("    .zero " + std::to_string(global.size_bytes));
    return true;
  }
  return false;
}

}  // namespace

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

  const auto has_zero_global =
      std::any_of(module->module.globals.begin(),
                  module->module.globals.end(),
                  [](const c4c::backend::bir::Global& global) {
                    return is_zero_initialized_global(global);
                  });
  const auto has_nonzero_global =
      std::any_of(module->module.globals.begin(),
                  module->module.globals.end(),
                  [](const c4c::backend::bir::Global& global) {
                    return !is_zero_initialized_global(global);
                  });

  const auto emit_globals_in_section = [&](bool zero_section) {
    for (const auto& global : module->module.globals) {
      if (is_zero_initialized_global(global) != zero_section) {
        continue;
      }
      if (!global.is_extern) {
        out.append_line(".globl " + render_asm_symbol_name(global.name));
      }
      out.append_line(".type " + render_asm_symbol_name(global.name) + ", @object");
      if (global.align_bytes > 1) {
        out.append_line(".p2align " + std::to_string(p2align_for_alignment(global.align_bytes)));
      }
      out.append_line(render_asm_symbol_name(global.name) + ":");
      if (!emit_global_initializer(out, module->module, global, target_triple)) {
        out.append_line("    # global data emission deferred to behavior-recovery packet");
      }
    }
  };

  if (has_nonzero_global) {
    out.append_line(".data");
    emit_globals_in_section(false);
  }
  if (has_zero_global) {
    out.append_line(".bss");
    emit_globals_in_section(true);
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
