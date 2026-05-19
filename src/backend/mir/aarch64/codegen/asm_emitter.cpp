#include "asm_emitter.hpp"

#include "machine_printer.hpp"
#include "mir/printer.hpp"

#include <cstdint>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace c4c::backend::aarch64::codegen {

namespace {

std::string byte_directive(std::string_view bytes) {
  std::ostringstream out;
  out << "    .byte ";
  for (std::size_t i = 0; i < bytes.size(); ++i) {
    if (i != 0) {
      out << ", ";
    }
    out << static_cast<unsigned int>(static_cast<unsigned char>(bytes[i]));
  }
  if (bytes.empty()) {
    out << "0";
  } else if (bytes.back() != '\0') {
    out << ", 0";
  }
  return out.str();
}

void append_string_constants(std::ostringstream& assembly,
                             const c4c::backend::prepare::PreparedBirModule& prepared) {
  if (prepared.module.string_constants.empty()) {
    return;
  }

  assembly << "    .section .rodata\n";
  for (const auto& constant : prepared.module.string_constants) {
    if (constant.align_bytes > 1) {
      assembly << "    .balign " << constant.align_bytes << "\n";
    }
    assembly << constant.name << ":\n";
    assembly << byte_directive(constant.bytes) << "\n";
  }
}

std::string global_label(const c4c::backend::bir::Module& module,
                         const c4c::backend::bir::Global& global) {
  const std::string_view structured = module.names.link_names.spelling(global.link_name_id);
  if (!structured.empty()) {
    return std::string{structured};
  }
  return global.name;
}

std::string logical_symbol_name(const c4c::backend::bir::Module& module,
                                const c4c::backend::bir::Value& value) {
  if (value.pointer_symbol_link_name_id != c4c::kInvalidLinkName) {
    const std::string_view structured =
        module.names.link_names.spelling(value.pointer_symbol_link_name_id);
    if (!structured.empty()) {
      return std::string{structured};
    }
  }
  if (!value.name.empty() && value.name.front() == '@') {
    return value.name.substr(1);
  }
  return value.name;
}

std::optional<std::string> initializer_symbol_name(
    const c4c::backend::bir::Module& module,
    const c4c::backend::bir::Global& global) {
  if (global.initializer_symbol_name_id != c4c::kInvalidLinkName) {
    const std::string_view structured =
        module.names.link_names.spelling(global.initializer_symbol_name_id);
    if (!structured.empty()) {
      return std::string{structured};
    }
  }
  if (!global.initializer_symbol_name.has_value() ||
      global.initializer_symbol_name->empty()) {
    return std::nullopt;
  }
  std::string symbol = *global.initializer_symbol_name;
  if (!symbol.empty() && symbol.front() == '@') {
    symbol.erase(symbol.begin());
  }
  return symbol;
}

std::int64_t scalar_global_initializer(const c4c::backend::bir::Global& global) {
  if (global.initializer.has_value() &&
      global.initializer->kind == c4c::backend::bir::Value::Kind::Immediate) {
    return global.initializer->immediate;
  }
  return 0;
}

bool is_supported_scalar_global(const c4c::backend::bir::Global& global) {
  switch (global.type) {
    case c4c::backend::bir::TypeKind::I8:
    case c4c::backend::bir::TypeKind::I16:
    case c4c::backend::bir::TypeKind::I32:
    case c4c::backend::bir::TypeKind::I64:
    case c4c::backend::bir::TypeKind::Ptr:
    case c4c::backend::bir::TypeKind::F64:
      return true;
    case c4c::backend::bir::TypeKind::F32:
      return true;
    default:
      return false;
  }
}

std::string scalar_global_directive(const c4c::backend::bir::Global& global) {
  switch (global.type) {
    case c4c::backend::bir::TypeKind::I8:
      return ".byte";
    case c4c::backend::bir::TypeKind::I16:
      return ".hword";
    case c4c::backend::bir::TypeKind::I32:
      return ".word";
    case c4c::backend::bir::TypeKind::I64:
    case c4c::backend::bir::TypeKind::Ptr:
    case c4c::backend::bir::TypeKind::F64:
      return ".xword";
    case c4c::backend::bir::TypeKind::F32:
      return ".word";
    default:
      return {};
  }
}

std::optional<std::string> global_initializer_directive(
    const c4c::backend::bir::Module& module,
    const c4c::backend::bir::Value& value) {
  if (value.kind == c4c::backend::bir::Value::Kind::Named) {
    if (value.type != c4c::backend::bir::TypeKind::Ptr) {
      return std::nullopt;
    }
    return ".xword " + logical_symbol_name(module, value);
  }
  if (value.kind != c4c::backend::bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  switch (value.type) {
    case c4c::backend::bir::TypeKind::I8:
      return ".byte " + std::to_string(static_cast<std::int8_t>(value.immediate));
    case c4c::backend::bir::TypeKind::I16:
      return ".hword " + std::to_string(static_cast<std::int16_t>(value.immediate));
    case c4c::backend::bir::TypeKind::I32:
      return ".word " + std::to_string(static_cast<std::int32_t>(value.immediate));
    case c4c::backend::bir::TypeKind::I64:
    case c4c::backend::bir::TypeKind::Ptr:
      return ".xword " + std::to_string(value.immediate);
    case c4c::backend::bir::TypeKind::F32:
      return ".word " + std::to_string(static_cast<std::uint32_t>(value.immediate_bits));
    case c4c::backend::bir::TypeKind::F64:
      return ".xword " + std::to_string(value.immediate_bits);
    default:
      return std::nullopt;
  }
}

bool emit_global_initializer(std::ostringstream& assembly,
                             const c4c::backend::bir::Module& module,
                             const c4c::backend::bir::Global& global) {
  if (global.initializer.has_value()) {
    const auto directive = global_initializer_directive(module, *global.initializer);
    if (!directive.has_value()) {
      return false;
    }
    assembly << "    " << *directive << "\n";
    return true;
  }
  if (!global.initializer_elements.empty()) {
    for (const auto& element : global.initializer_elements) {
      const auto directive = global_initializer_directive(module, element);
      if (!directive.has_value()) {
        return false;
      }
      assembly << "    " << *directive << "\n";
    }
    return true;
  }
  if (global.initializer_symbol_name_id != c4c::kInvalidLinkName ||
      global.initializer_symbol_name.has_value()) {
    const auto symbol = initializer_symbol_name(module, global);
    if (symbol.has_value()) {
      assembly << "    .xword " << *symbol << "\n";
      return true;
    }
  }
  if (is_supported_scalar_global(global)) {
    assembly << "    " << scalar_global_directive(global) << " "
             << scalar_global_initializer(global) << "\n";
    return true;
  }
  if (global.size_bytes != 0) {
    assembly << "    .zero " << global.size_bytes << "\n";
    return true;
  }
  return false;
}

bool is_emit_supported_global(const c4c::backend::bir::Global& global) {
  return is_supported_scalar_global(global) || global.initializer.has_value() ||
         !global.initializer_elements.empty() ||
         global.initializer_symbol_name_id != c4c::kInvalidLinkName ||
         global.size_bytes != 0;
}

void append_global_objects(std::ostringstream& assembly,
                           const c4c::backend::prepare::PreparedBirModule& prepared) {
  bool wrote_section = false;
  for (const auto& global : prepared.module.globals) {
    if (global.is_extern || !is_emit_supported_global(global)) {
      continue;
    }
    if (!wrote_section) {
      assembly << "    .data\n";
      wrote_section = true;
    }
    if (global.align_bytes > 1) {
      assembly << "    .balign " << global.align_bytes << "\n";
    }
    assembly << global_label(prepared.module, global) << ":\n";
    if (!emit_global_initializer(assembly, prepared.module, global)) {
      assembly << "    # global data emission deferred to behavior-recovery packet\n";
    }
  }
}

}  // namespace

std::string print_prepared_machine_nodes(
    const c4c::backend::prepare::PreparedBirModule& prepared) {
  const auto built = compile_prepared_module(prepared);
  if (!built.module.has_value()) {
    std::string message = "AArch64 backend assembly route could not build a prepared module";
    if (built.error.has_value() && !built.error->message.empty()) {
      message += ": ";
      message += built.error->message;
    }
    throw std::invalid_argument(message);
  }

  std::ostringstream assembly;
  assembly << "    .text\n";
  std::size_t machine_node_count = 0;
  for (const auto& function : built.module->functions) {
    if (c4c::backend::mir::empty(function.mir)) {
      continue;
    }
    ++machine_node_count;
    assembly << "    .globl " << function.label << "\n"
             << "    .type " << function.label << ", %function\n"
             << function.label << ":\n";
    const MachineInstructionPrinter target_printer;
    const auto printed = c4c::backend::mir::print_machine_function(function.mir,
                                                                   target_printer);
    if (!printed.ok) {
      throw std::invalid_argument("AArch64 backend assembly route reached the machine-node printer, "
                                  "but printing failed: " +
                                  printed.diagnostic);
    }
    assembly << printed.assembly
             << "    .size " << function.label << ", .-" << function.label << "\n";
  }
  if (machine_node_count == 0) {
    throw std::invalid_argument(
        "AArch64 backend assembly route reached the machine-node printer, but no selected printable machine nodes are available for this source input");
  }
  append_global_objects(assembly, prepared);
  append_string_constants(assembly, prepared);
  assembly << "    .section .note.GNU-stack,\"\",@progbits\n";
  return assembly.str();
}

}  // namespace c4c::backend::aarch64::codegen
