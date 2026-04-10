#include "x86_codegen.hpp"

#include "../../bir.hpp"

#include <stdexcept>

namespace c4c::backend::x86 {

namespace {

struct MinimalScalarGlobalLoadSlice {
  std::string function_name;
  std::string global_name;
  std::int64_t init_imm = 0;
  std::size_t align_bytes = 4;
  bool zero_initializer = false;
};

struct MinimalExternScalarGlobalLoadSlice {
  std::string function_name;
  std::string global_name;
};

std::optional<MinimalScalarGlobalLoadSlice> parse_minimal_scalar_global_load_slice(
    const c4c::backend::bir::Module& module) {
  using namespace c4c::backend::bir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_extern || global.type != TypeKind::I32 || !global.initializer.has_value() ||
      global.initializer->kind != c4c::backend::bir::Value::Kind::Immediate ||
      global.initializer->type != TypeKind::I32) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.return_type != TypeKind::I32 ||
      !function.params.empty() || !function.local_slots.empty() || function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* load =
      entry.insts.size() == 1 ? std::get_if<LoadGlobalInst>(&entry.insts.front()) : nullptr;
  if (entry.label != "entry" || load == nullptr ||
      load->result.kind != c4c::backend::bir::Value::Kind::Named ||
      load->result.type != TypeKind::I32 || load->global_name != global.name ||
      load->byte_offset != 0 || entry.terminator.kind != TerminatorKind::Return ||
      !entry.terminator.value.has_value() || *entry.terminator.value != load->result) {
    return std::nullopt;
  }

  return MinimalScalarGlobalLoadSlice{
      .function_name = function.name,
      .global_name = global.name,
      .init_imm = global.initializer->immediate,
      .align_bytes = load->align_bytes > 0 ? load->align_bytes : 4,
      .zero_initializer = global.initializer->immediate == 0,
  };
}

std::optional<MinimalExternScalarGlobalLoadSlice> parse_minimal_extern_scalar_global_load_slice(
    const c4c::backend::bir::Module& module) {
  using namespace c4c::backend::bir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (!global.is_extern || global.type != TypeKind::I32 || global.initializer.has_value()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.return_type != TypeKind::I32 ||
      !function.params.empty() || !function.local_slots.empty() || function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* load =
      entry.insts.size() == 1 ? std::get_if<LoadGlobalInst>(&entry.insts.front()) : nullptr;
  if (entry.label != "entry" || load == nullptr ||
      load->result.kind != c4c::backend::bir::Value::Kind::Named ||
      load->result.type != TypeKind::I32 || load->global_name != global.name ||
      load->byte_offset != 0 || entry.terminator.kind != TerminatorKind::Return ||
      !entry.terminator.value.has_value() || *entry.terminator.value != load->result) {
    return std::nullopt;
  }

  return MinimalExternScalarGlobalLoadSlice{
      .function_name = function.name,
      .global_name = global.name,
  };
}

[[noreturn]] void throw_unwired_translated_globals_owner() {
  throw std::logic_error(
      "x86 translated globals owner methods are compiled for symbol/link coverage, but the exporter-backed X86Codegen state is not wired yet");
}

}  // namespace

std::optional<std::string> try_emit_minimal_scalar_global_load_module(
    const c4c::backend::bir::Module& module) {
  const auto slice = parse_minimal_scalar_global_load_slice(module);
  if (!slice.has_value()) {
    return std::nullopt;
  }
  return emit_minimal_scalar_global_load_slice_asm(module.target_triple,
                                                   slice->function_name,
                                                   slice->global_name,
                                                   slice->init_imm,
                                                   slice->align_bytes,
                                                   slice->zero_initializer);
}

std::optional<std::string> try_emit_minimal_extern_scalar_global_load_module(
    const c4c::backend::bir::Module& module) {
  const auto slice = parse_minimal_extern_scalar_global_load_slice(module);
  if (!slice.has_value()) {
    return std::nullopt;
  }
  return emit_minimal_extern_scalar_global_load_slice_asm(module.target_triple,
                                                          slice->function_name,
                                                          slice->global_name);
}

void X86Codegen::emit_global_addr_impl(const Value& dest, const std::string& name) {
  (void)dest;
  (void)name;
  throw_unwired_translated_globals_owner();
}

void X86Codegen::emit_tls_global_addr_impl(const Value& dest, const std::string& name) {
  (void)dest;
  (void)name;
  throw_unwired_translated_globals_owner();
}

void X86Codegen::emit_global_addr_absolute_impl(const Value& dest, const std::string& name) {
  (void)dest;
  (void)name;
  throw_unwired_translated_globals_owner();
}

void X86Codegen::emit_global_load_rip_rel_impl(const Value& dest,
                                                const std::string& sym,
                                                IrType ty) {
  (void)dest;
  (void)sym;
  (void)ty;
  throw_unwired_translated_globals_owner();
}

void X86Codegen::emit_global_store_rip_rel_impl(const Operand& val,
                                                 const std::string& sym,
                                                 IrType ty) {
  (void)val;
  (void)sym;
  (void)ty;
  throw_unwired_translated_globals_owner();
}

void X86Codegen::emit_label_addr_impl(const Value& dest, const std::string& label) {
  (void)dest;
  (void)label;
  throw_unwired_translated_globals_owner();
}

void X86Codegen::emit_store_result_impl(const Value& dest) {
  (void)dest;
  throw_unwired_translated_globals_owner();
}

void X86Codegen::emit_load_operand_impl(const Operand& op) {
  (void)op;
  throw_unwired_translated_globals_owner();
}

}  // namespace c4c::backend::x86
