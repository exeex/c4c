#include "module_data_emit.hpp"

#include "../abi/x86_target_abi.hpp"
#include "../core/x86_codegen_output.hpp"

#include <cstdint>
#include <functional>
#include <unordered_set>

namespace c4c::backend::x86::module {

namespace {

std::optional<std::size_t> same_module_global_scalar_size(c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::I8: return 1;
    case c4c::backend::bir::TypeKind::I32: return 4;
    case c4c::backend::bir::TypeKind::Ptr: return 8;
    default: return std::nullopt;
  }
}

bool same_module_global_is_zero_initialized(const c4c::backend::bir::Global& global) {
  if (global.initializer_symbol_name.has_value()) {
    return false;
  }
  if (global.initializer.has_value()) {
    if (global.initializer->kind != c4c::backend::bir::Value::Kind::Immediate) {
      return false;
    }
    return global.initializer->immediate == 0 && global.initializer->immediate_bits == 0;
  }
  if (global.initializer_elements.empty()) {
    return false;
  }
  for (const auto& element : global.initializer_elements) {
    if (element.kind == c4c::backend::bir::Value::Kind::Named) {
      return false;
    }
    if (element.immediate != 0 || element.immediate_bits != 0) {
      return false;
    }
  }
  return true;
}

std::string escape_asm_bytes(std::string_view raw_bytes) {
  std::string escaped;
  escaped.reserve(raw_bytes.size());
  for (const unsigned char ch : raw_bytes) {
    switch (ch) {
      case '\\': escaped += "\\\\"; break;
      case '"': escaped += "\\\""; break;
      case '\n': escaped += "\\n"; break;
      case '\t': escaped += "\\t"; break;
      case '\r': escaped += "\\r"; break;
      case '\0': break;
      default: escaped.push_back(static_cast<char>(ch)); break;
    }
  }
  return escaped;
}

std::string emit_defined_global_prelude(std::string_view target_triple,
                                        std::string_view symbol_name,
                                        std::size_t align_bytes,
                                        bool is_zero_init) {
  std::string prelude = is_zero_init ? ".bss\n" : ".data\n";
  prelude += ".globl " + std::string(symbol_name) + "\n";
  if (target_triple.find("apple-darwin") == std::string_view::npos) {
    prelude += ".type " + std::string(symbol_name) + ", @object\n";
  }
  if (align_bytes > 1) {
    prelude += ".p2align " + std::to_string(align_bytes == 2 ? 1 : 2) + "\n";
  }
  prelude += std::string(symbol_name) + ":\n";
  return prelude;
}

bool append_initializer_value(std::string_view target_triple,
                              std::string* data,
                              const c4c::backend::bir::Value& value) {
  if (value.kind == c4c::backend::bir::Value::Kind::Named) {
    if (value.type != c4c::backend::bir::TypeKind::Ptr || value.name.empty() ||
        value.name.front() != '@') {
      return false;
    }
    *data += "    .quad " +
             c4c::backend::x86::abi::render_asm_symbol_name(target_triple,
                                                            value.name.substr(1)) +
             "\n";
    return true;
  }
  switch (value.type) {
    case c4c::backend::bir::TypeKind::I8:
      *data += "    .byte " +
               std::to_string(
                   static_cast<std::int32_t>(static_cast<std::int8_t>(value.immediate))) +
               "\n";
      return true;
    case c4c::backend::bir::TypeKind::I32:
      *data += "    .long " +
               std::to_string(static_cast<std::int32_t>(value.immediate)) + "\n";
      return true;
    case c4c::backend::bir::TypeKind::F32:
      *data += "    .long " +
               std::to_string(static_cast<std::uint32_t>(value.immediate_bits)) + "\n";
      return true;
    case c4c::backend::bir::TypeKind::F64:
      *data += "    .quad " + std::to_string(value.immediate_bits) + "\n";
      return true;
    case c4c::backend::bir::TypeKind::Ptr:
      if (value.immediate != 0 || value.immediate_bits != 0) {
        return false;
      }
      *data += "    .quad 0\n";
      return true;
    default: return false;
  }
}

const c4c::backend::bir::Global* lookup_same_module_global(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view name) {
  for (const auto& global : module.module.globals) {
    if (global.name == name) {
      if (global.is_extern || global.is_thread_local) {
        return nullptr;
      }
      return &global;
    }
  }
  return nullptr;
}

}  // namespace

std::string ModuleDataSupport::render_asm_symbol_name(std::string_view logical_name) const {
  return c4c::backend::x86::abi::render_asm_symbol_name(target_triple, logical_name);
}

std::string ModuleDataSupport::render_private_data_label(std::string_view pool_name) const {
  return c4c::backend::x86::abi::render_private_data_label(target_triple, pool_name);
}

const c4c::backend::bir::StringConstant* ModuleDataSupport::find_string_constant(
    std::string_view name) const {
  return c4c::backend::x86::module::find_string_constant(*module, name);
}

bool ModuleDataSupport::has_string_constant(std::string_view name) const {
  return find_string_constant(name) != nullptr;
}

const c4c::backend::bir::Global* ModuleDataSupport::find_same_module_global(
    std::string_view name) const {
  return c4c::backend::x86::module::find_same_module_global(*module, name);
}

bool ModuleDataSupport::has_same_module_global(std::string_view name) const {
  return find_same_module_global(name) != nullptr;
}

bool ModuleDataSupport::same_module_global_supports_scalar_load(
    const c4c::backend::bir::Global& global,
    c4c::backend::bir::TypeKind type,
    std::size_t byte_offset) const {
  return c4c::backend::x86::module::same_module_global_supports_scalar_load(global, type,
                                                                            byte_offset);
}

std::string ModuleDataSupport::emit_string_constant_data(
    const c4c::backend::bir::StringConstant& string_constant) const {
  return c4c::backend::x86::module::emit_string_constant_data(target_triple, string_constant);
}

std::optional<std::string> ModuleDataSupport::emit_same_module_global_data(
    const c4c::backend::bir::Global& global) const {
  return c4c::backend::x86::module::emit_same_module_global_data(target_triple, global);
}

std::string ModuleDataSupport::emit_missing_same_module_global_data(
    std::string_view asm_text) const {
  return c4c::backend::x86::module::emit_missing_same_module_global_data(*module, target_triple,
                                                                         asm_text);
}

std::string ModuleDataSupport::emit_direct_variadic_runtime_helpers(
    std::string_view asm_text) const {
  return c4c::backend::x86::module::emit_direct_variadic_runtime_helpers(target_triple, asm_text);
}

void ModuleDataSupport::add_referenced_same_module_globals(
    std::string_view asm_text,
    std::unordered_set<std::string_view>* used_same_module_global_names) const {
  c4c::backend::x86::module::add_referenced_same_module_globals(*module, target_triple, asm_text,
                                                                used_same_module_global_names);
}

std::string ModuleDataSupport::finalize_multi_defined_rendered_module_text(
    std::string_view rendered_text) const {
  const std::string rendered_variadic_runtime_helpers =
      emit_direct_variadic_runtime_helpers(rendered_text);
  const std::string finalized_text = std::string(rendered_text) + rendered_variadic_runtime_helpers;
  return finalized_text + emit_missing_same_module_global_data(finalized_text);
}

std::string ModuleDataSupport::finalize_selected_module_text(
    std::string_view rendered_text,
    const std::unordered_set<std::string_view>& used_string_names,
    std::unordered_set<std::string_view>* used_same_module_global_names) const {
  add_referenced_same_module_globals(rendered_text, used_same_module_global_names);
  const std::string rendered_variadic_runtime_helpers =
      emit_direct_variadic_runtime_helpers(rendered_text);
  const std::string rendered_data =
      emit_selected_module_data(used_string_names, *used_same_module_global_names);
  const std::string finalized_text =
      std::string(rendered_text) + rendered_variadic_runtime_helpers + rendered_data;
  return finalized_text + emit_missing_same_module_global_data(finalized_text);
}

std::string ModuleDataSupport::emit_selected_module_data(
    const std::unordered_set<std::string_view>& used_string_names,
    const std::unordered_set<std::string_view>& used_same_module_global_names) const {
  return c4c::backend::x86::module::emit_selected_module_data(*module, target_triple,
                                                              used_string_names,
                                                              used_same_module_global_names);
}

ModuleDataSupport make_module_data_support(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view target_triple) {
  return ModuleDataSupport{.module = &module, .target_triple = target_triple};
}

const c4c::backend::bir::StringConstant* find_string_constant(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view name) {
  for (const auto& string_constant : module.module.string_constants) {
    if (string_constant.name == name) {
      return &string_constant;
    }
  }
  return nullptr;
}

const c4c::backend::bir::Global* find_same_module_global(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view name) {
  return lookup_same_module_global(module, name);
}

bool same_module_global_supports_scalar_load(const c4c::backend::bir::Global& global,
                                             c4c::backend::bir::TypeKind type,
                                             std::size_t byte_offset) {
  const auto scalar_size = same_module_global_scalar_size(type);
  if (!scalar_size.has_value()) {
    return false;
  }
  if (global.initializer_symbol_name.has_value()) {
    return type == c4c::backend::bir::TypeKind::Ptr && byte_offset == 0;
  }
  if (same_module_global_is_zero_initialized(global) && byte_offset <= global.size_bytes &&
      *scalar_size <= global.size_bytes - byte_offset) {
    return true;
  }
  if (global.initializer.has_value()) {
    const auto init_size = same_module_global_scalar_size(global.initializer->type);
    return global.initializer->kind == c4c::backend::bir::Value::Kind::Immediate &&
           init_size.has_value() && global.initializer->type == type && byte_offset == 0;
  }
  std::size_t current_offset = 0;
  for (const auto& element : global.initializer_elements) {
    const auto element_size = same_module_global_scalar_size(element.type);
    if (!element_size.has_value()) {
      return false;
    }
    if (current_offset == byte_offset && element.type == type) {
      return true;
    }
    current_offset += *element_size;
  }
  return false;
}

std::string emit_string_constant_data(std::string_view target_triple,
                                      const c4c::backend::bir::StringConstant& string_constant) {
  const auto label =
      c4c::backend::x86::abi::render_private_data_label(target_triple, "@" + string_constant.name);
  return ".section .rodata\n" + label + ":\n    .asciz \"" +
         escape_asm_bytes(string_constant.bytes) + "\"\n";
}

std::optional<std::string> emit_same_module_global_data(
    std::string_view target_triple,
    const c4c::backend::bir::Global& global) {
  const auto symbol_name =
      c4c::backend::x86::abi::render_asm_symbol_name(target_triple, global.name);
  std::string data = emit_defined_global_prelude(
      target_triple, symbol_name, global.align_bytes > 0 ? global.align_bytes : 4,
      same_module_global_is_zero_initialized(global));
  if (global.initializer_symbol_name.has_value()) {
    data += "    .quad " +
            c4c::backend::x86::abi::render_asm_symbol_name(target_triple,
                                                           *global.initializer_symbol_name) +
            "\n";
    return data;
  }
  if (global.initializer.has_value()) {
    if (!append_initializer_value(target_triple, &data, *global.initializer)) {
      return std::nullopt;
    }
    return data;
  }
  if (global.initializer_elements.empty()) {
    return std::nullopt;
  }
  for (const auto& element : global.initializer_elements) {
    if (!append_initializer_value(target_triple, &data, element)) {
      return std::nullopt;
    }
  }
  return data;
}

std::string emit_missing_same_module_global_data(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view target_triple,
    std::string_view asm_text) {
  std::string missing_data;
  std::unordered_set<std::string_view> emitted_missing_globals;
  std::function<void(std::string_view)> add_same_module_global_closure =
      [&](std::string_view global_name) {
        const auto* same_module_global = lookup_same_module_global(module, global_name);
        if (same_module_global == nullptr ||
            emitted_missing_globals.find(same_module_global->name) !=
                emitted_missing_globals.end() ||
            c4c::backend::x86::core::asm_text_defines_symbol(
                asm_text,
                c4c::backend::x86::abi::render_asm_symbol_name(target_triple,
                                                               same_module_global->name))) {
          return;
        }
        emitted_missing_globals.insert(same_module_global->name);
        if (same_module_global->initializer_symbol_name.has_value()) {
          add_same_module_global_closure(*same_module_global->initializer_symbol_name);
        }
        if (same_module_global->initializer.has_value() &&
            same_module_global->initializer->kind == c4c::backend::bir::Value::Kind::Named &&
            same_module_global->initializer->type == c4c::backend::bir::TypeKind::Ptr &&
            !same_module_global->initializer->name.empty() &&
            same_module_global->initializer->name.front() == '@') {
          add_same_module_global_closure(same_module_global->initializer->name.substr(1));
        }
        for (const auto& element : same_module_global->initializer_elements) {
          if (element.kind != c4c::backend::bir::Value::Kind::Named ||
              element.type != c4c::backend::bir::TypeKind::Ptr || element.name.empty() ||
              element.name.front() != '@') {
            continue;
          }
          add_same_module_global_closure(element.name.substr(1));
        }
        const auto rendered_global_data = emit_same_module_global_data(target_triple,
                                                                       *same_module_global);
        if (!rendered_global_data.has_value()) {
          return;
        }
        missing_data += *rendered_global_data;
      };
  for (const auto& global : module.module.globals) {
    const auto rendered_name =
        c4c::backend::x86::abi::render_asm_symbol_name(target_triple, global.name);
    if (!c4c::backend::x86::core::asm_text_references_symbol(asm_text, rendered_name) ||
        c4c::backend::x86::core::asm_text_defines_symbol(asm_text, rendered_name)) {
      continue;
    }
    add_same_module_global_closure(global.name);
  }
  return missing_data;
}

std::string emit_direct_variadic_runtime_helpers(std::string_view target_triple,
                                                 std::string_view asm_text) {
  std::string helpers;
  const auto emit_function_prelude = [&](std::string_view symbol_name) {
    helpers += ".intel_syntax noprefix\n.text\n.globl " + std::string(symbol_name) + "\n";
    if (!c4c::backend::x86::abi::is_apple_darwin_target(target_triple)) {
      helpers += ".type " + std::string(symbol_name) + ", @function\n";
    }
    helpers += std::string(symbol_name) + ":\n";
  };
  if (c4c::backend::x86::core::asm_text_references_symbol(asm_text, "llvm.va_start.p0") &&
      !c4c::backend::x86::core::asm_text_defines_symbol(asm_text, "llvm.va_start.p0")) {
    emit_function_prelude("llvm.va_start.p0");
    helpers += "    mov DWORD PTR [rdi], 48\n";
    helpers += "    mov DWORD PTR [rdi + 4], 176\n";
    helpers += "    lea rax, [rsp + 8]\n";
    helpers += "    mov QWORD PTR [rdi + 8], rax\n";
    helpers += "    mov QWORD PTR [rdi + 16], rax\n";
    helpers += "    ret\n";
  }
  if (c4c::backend::x86::core::asm_text_references_symbol(asm_text, "llvm.va_end.p0") &&
      !c4c::backend::x86::core::asm_text_defines_symbol(asm_text, "llvm.va_end.p0")) {
    emit_function_prelude("llvm.va_end.p0");
    helpers += "    ret\n";
  }
  if (c4c::backend::x86::core::asm_text_references_symbol(asm_text, "llvm.va_copy.p0.p0") &&
      !c4c::backend::x86::core::asm_text_defines_symbol(asm_text, "llvm.va_copy.p0.p0")) {
    emit_function_prelude("llvm.va_copy.p0.p0");
    helpers += "    mov eax, DWORD PTR [rsi]\n";
    helpers += "    mov DWORD PTR [rdi], eax\n";
    helpers += "    mov eax, DWORD PTR [rsi + 4]\n";
    helpers += "    mov DWORD PTR [rdi + 4], eax\n";
    helpers += "    mov rax, QWORD PTR [rsi + 8]\n";
    helpers += "    mov QWORD PTR [rdi + 8], rax\n";
    helpers += "    mov rax, QWORD PTR [rsi + 16]\n";
    helpers += "    mov QWORD PTR [rdi + 16], rax\n";
    helpers += "    ret\n";
  }
  return helpers;
}

void add_referenced_same_module_globals(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view target_triple,
    std::string_view asm_text,
    std::unordered_set<std::string_view>* used_same_module_global_names) {
  std::function<void(std::string_view)> add_same_module_global_closure =
      [&](std::string_view global_name) {
        const auto* same_module_global = lookup_same_module_global(module, global_name);
        if (same_module_global == nullptr ||
            !used_same_module_global_names->insert(same_module_global->name).second) {
          return;
        }
        if (same_module_global->initializer_symbol_name.has_value()) {
          add_same_module_global_closure(*same_module_global->initializer_symbol_name);
        }
        if (same_module_global->initializer.has_value() &&
            same_module_global->initializer->kind == c4c::backend::bir::Value::Kind::Named &&
            same_module_global->initializer->type == c4c::backend::bir::TypeKind::Ptr &&
            !same_module_global->initializer->name.empty() &&
            same_module_global->initializer->name.front() == '@') {
          add_same_module_global_closure(same_module_global->initializer->name.substr(1));
        }
        for (const auto& element : same_module_global->initializer_elements) {
          if (element.kind != c4c::backend::bir::Value::Kind::Named ||
              element.type != c4c::backend::bir::TypeKind::Ptr || element.name.empty() ||
              element.name.front() != '@') {
            continue;
          }
          add_same_module_global_closure(element.name.substr(1));
        }
      };
  for (const auto& global : module.module.globals) {
    const auto rendered_name =
        c4c::backend::x86::abi::render_asm_symbol_name(target_triple, global.name);
    if (!c4c::backend::x86::core::asm_text_references_symbol(asm_text, rendered_name)) {
      continue;
    }
    add_same_module_global_closure(global.name);
  }
}

std::string emit_selected_module_data(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view target_triple,
    const std::unordered_set<std::string_view>& used_string_names,
    const std::unordered_set<std::string_view>& used_same_module_global_names) {
  std::string rendered_data;
  for (const auto& string_constant : module.module.string_constants) {
    if (used_string_names.find(string_constant.name) == used_string_names.end()) {
      continue;
    }
    rendered_data += emit_string_constant_data(target_triple, string_constant);
  }
  for (const auto& global : module.module.globals) {
    if (used_same_module_global_names.find(global.name) == used_same_module_global_names.end()) {
      continue;
    }
    const auto rendered_global_data = emit_same_module_global_data(target_triple, global);
    if (!rendered_global_data.has_value()) {
      throw std::invalid_argument(
          "x86 backend emitter requires same-module global data emission support through the canonical prepared-module handoff");
    }
    rendered_data += *rendered_global_data;
  }
  return rendered_data;
}

}  // namespace c4c::backend::x86::module
