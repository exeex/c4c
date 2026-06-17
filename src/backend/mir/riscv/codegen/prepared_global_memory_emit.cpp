#include "prepared_global_memory_emit.hpp"

#include "prepared_emit_context.hpp"
#include "prepared_scalar_emit.hpp"

#include "../../../prealloc/addressing.hpp"
#include "../../../prealloc/names.hpp"

#include <algorithm>
#include <cstdint>

namespace c4c::backend::riscv::codegen {
namespace {

const c4c::backend::bir::Global* find_prepared_global(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::LinkNameId link_name,
    const std::string& fallback_name) {
  if (link_name != c4c::kInvalidLinkName) {
    const auto it = std::find_if(
        prepared.module.globals.begin(),
        prepared.module.globals.end(),
        [&](const c4c::backend::bir::Global& global) {
          return global.link_name_id == link_name;
        });
    if (it != prepared.module.globals.end()) {
      return &*it;
    }
  }
  if (fallback_name.empty()) {
    return nullptr;
  }
  const auto it = std::find_if(
      prepared.module.globals.begin(),
      prepared.module.globals.end(),
      [&](const c4c::backend::bir::Global& global) {
        return global.name == fallback_name;
      });
  return it == prepared.module.globals.end() ? nullptr : &*it;
}

bool is_simple_defined_i32_global(const c4c::backend::bir::Global& global) {
  namespace bir = c4c::backend::bir;

  if (global.is_extern ||
      global.is_thread_local ||
      global.type != bir::TypeKind::I32 ||
      global.size_bytes != 4 ||
      global.align_bytes < 4 ||
      !global.initializer_elements.empty() ||
      global.initializer_symbol_name.has_value() ||
      global.initializer_symbol_name_id != c4c::kInvalidLinkName) {
    return false;
  }
  if (!global.initializer.has_value()) {
    return true;
  }
  return global.initializer->kind == bir::Value::Kind::Immediate &&
         global.initializer->type == bir::TypeKind::I32;
}

std::string global_label(const c4c::backend::bir::Module& module,
                         const c4c::backend::bir::Global& global) {
  if (global.link_name_id != c4c::kInvalidLinkName) {
    const std::string_view spelling = module.names.link_names.spelling(global.link_name_id);
    if (!spelling.empty()) {
      return std::string{spelling};
    }
  }
  return global.name;
}

std::int32_t simple_i32_global_initializer(
    const c4c::backend::bir::Global& global) {
  if (!global.initializer.has_value()) {
    return 0;
  }
  return static_cast<std::int32_t>(global.initializer->immediate);
}

std::string prepared_link_name_spelling(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::LinkNameId link_name) {
  if (link_name == c4c::kInvalidLinkName) {
    return {};
  }
  const std::string_view spelling = prepared.names.link_names.spelling(link_name);
  return std::string{spelling};
}

const c4c::backend::prepare::PreparedMemoryAccess* simple_i32_global_access_for(
    const PreparedCurrentInstructionContext& context,
    std::optional<c4c::ValueNameId> result_value_name,
    std::optional<c4c::ValueNameId> stored_value_name) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (context.lookups == nullptr) {
    return nullptr;
  }
  const auto* access = prepare::find_indexed_prepared_memory_access(
      &context.lookups->memory_accesses,
      context.block_label,
      context.instruction_index);
  if (access == nullptr ||
      access->result_value_name != result_value_name ||
      access->stored_value_name != stored_value_name ||
      access->address_space != bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::GlobalSymbol ||
      access->address.byte_offset != 0 ||
      access->address.size_bytes != 4 ||
      access->address.align_bytes < 4 ||
      !access->address.can_use_base_plus_offset) {
    return nullptr;
  }
  return access;
}

}  // namespace

bool append_prepared_global_storage_asm(
    std::string& out,
    const c4c::backend::prepare::PreparedBirModule& prepared) {
  if (prepared.module.globals.empty()) {
    return true;
  }

  for (const auto& global : prepared.module.globals) {
    if (!is_simple_defined_i32_global(global) ||
        global_label(prepared.module, global).empty()) {
      return false;
    }
  }

  bool wrote_data = false;
  for (const auto& global : prepared.module.globals) {
    const auto label = global_label(prepared.module, global);
    const std::int32_t initializer = simple_i32_global_initializer(global);
    if (initializer == 0) {
      continue;
    }
    if (!wrote_data) {
      out += "    .data\n";
      wrote_data = true;
    }
    out += "    .balign 4\n";
    out += label;
    out += ":\n";
    out += "    .word ";
    out += std::to_string(initializer);
    out += "\n";
  }

  bool wrote_bss = false;
  for (const auto& global : prepared.module.globals) {
    const auto label = global_label(prepared.module, global);
    const std::int32_t initializer = simple_i32_global_initializer(global);
    if (initializer != 0) {
      continue;
    }
    if (!wrote_bss) {
      out += "    .bss\n";
      wrote_bss = true;
    }
    out += "    .balign 4\n";
    out += label;
    out += ":\n";
    out += "    .zero 4\n";
  }
  return true;
}

std::optional<std::string> emit_riscv_simple_load_global(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::LoadGlobalInst& load,
    const PreparedCurrentInstructionContext& context) {
  namespace bir = c4c::backend::bir;

  if (load.result.kind != bir::Value::Kind::Named ||
      load.result.type != bir::TypeKind::I32 ||
      load.result.name.empty() ||
      (load.global_name_id == c4c::kInvalidLinkName && load.global_name.empty()) ||
      load.byte_offset != 0) {
    return std::nullopt;
  }
  const auto result_name = context.names.value_names.find(load.result.name);
  if (result_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto* access = simple_i32_global_access_for(
      context,
      result_name,
      std::nullopt);
  std::string fallback_global_name = load.global_name;
  if (fallback_global_name.empty() &&
      access != nullptr &&
      access->address.symbol_name.has_value()) {
    fallback_global_name = prepared_link_name_spelling(
        prepared,
        *access->address.symbol_name);
  }
  const auto* global = find_prepared_global(
      prepared,
      load.global_name_id,
      fallback_global_name);
  if (global == nullptr || access == nullptr || !is_simple_defined_i32_global(*global)) {
    return std::nullopt;
  }
  const auto label = global_label(prepared.module, *global);
  const auto destination_register = prepared_register_for_value_name_id(
      context,
      result_name);
  if (label.empty() || !destination_register.has_value()) {
    return std::nullopt;
  }

  std::string out;
  out += "    lla " + *destination_register + ", " + label + "\n";
  out += "    lw " + *destination_register + ", 0(" + *destination_register + ")\n";
  return out;
}

std::optional<std::string> emit_riscv_simple_store_global(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::StoreGlobalInst& store,
    const PreparedCurrentInstructionContext& context) {
  namespace bir = c4c::backend::bir;

  if (store.value.type != bir::TypeKind::I32 ||
      (store.global_name_id == c4c::kInvalidLinkName && store.global_name.empty()) ||
      store.byte_offset != 0) {
    return std::nullopt;
  }

  std::optional<c4c::ValueNameId> stored_name;
  if (store.value.kind == bir::Value::Kind::Named) {
    if (store.value.name.empty()) {
      return std::nullopt;
    }
    stored_name = context.names.value_names.find(store.value.name);
    if (*stored_name == c4c::kInvalidValueName) {
      return std::nullopt;
    }
  } else if (store.value.kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }

  const auto* access = simple_i32_global_access_for(
      context,
      std::nullopt,
      stored_name);
  std::string fallback_global_name = store.global_name;
  if (fallback_global_name.empty() &&
      access != nullptr &&
      access->address.symbol_name.has_value()) {
    fallback_global_name = prepared_link_name_spelling(
        prepared,
        *access->address.symbol_name);
  }
  const auto* global = find_prepared_global(
      prepared,
      store.global_name_id,
      fallback_global_name);
  if (global == nullptr || access == nullptr || !is_simple_defined_i32_global(*global)) {
    return std::nullopt;
  }
  const auto label = global_label(prepared.module, *global);
  if (label.empty()) {
    return std::nullopt;
  }

  std::string out;
  if (!emit_move_to_register(
          out,
          "t1",
          context.names,
          context.lookups,
          store.value)) {
    return std::nullopt;
  }
  out += "    lla t0, " + label + "\n";
  out += "    sw t1, 0(t0)\n";
  return out;
}

}  // namespace c4c::backend::riscv::codegen
