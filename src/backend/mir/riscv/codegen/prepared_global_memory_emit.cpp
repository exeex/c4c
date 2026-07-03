#include "prepared_global_memory_emit.hpp"

#include "prepared_emit_context.hpp"
#include "prepared_frame_emit.hpp"
#include "prepared_scalar_emit.hpp"
#include "rv64_line_assembler.hpp"

#include "../../../prealloc/addressing.hpp"
#include "../../../prealloc/names.hpp"
#include "../../../prealloc/prepared_contract_verifier.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string_view>
#include <vector>

namespace c4c::backend::riscv::codegen {
namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

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

const c4c::backend::bir::Function* find_prepared_function(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::LinkNameId link_name,
    const std::string& fallback_name) {
  if (!fallback_name.empty()) {
    const auto it = std::find_if(
        prepared.module.functions.begin(),
        prepared.module.functions.end(),
        [&](const c4c::backend::bir::Function& function) {
          return function.name == fallback_name;
        });
    if (it != prepared.module.functions.end()) {
      return &*it;
    }
  }
  if (link_name != c4c::kInvalidLinkName) {
    const auto it = std::find_if(
        prepared.module.functions.begin(),
        prepared.module.functions.end(),
        [&](const c4c::backend::bir::Function& function) {
          return function.link_name_id == link_name;
        });
    if (it != prepared.module.functions.end()) {
      return &*it;
    }
  }
  return nullptr;
}

std::optional<std::vector<std::int32_t>> simple_defined_i32_global_words(
    const c4c::backend::bir::Global& global) {
  namespace bir = c4c::backend::bir;

  if (global.is_extern ||
      global.is_thread_local ||
      global.type != bir::TypeKind::I32 ||
      global.size_bytes == 0 ||
      global.size_bytes % 4 != 0 ||
      global.align_bytes < 4 ||
      global.initializer_symbol_name.has_value() ||
      global.initializer_symbol_name_id != c4c::kInvalidLinkName) {
    return std::nullopt;
  }

  const std::size_t word_count = global.size_bytes / 4;
  std::vector<std::int32_t> words(word_count, 0);

  if (!global.initializer_elements.empty()) {
    if (global.initializer.has_value() ||
        global.initializer_elements.size() > word_count) {
      return std::nullopt;
    }
    for (std::size_t index = 0; index < global.initializer_elements.size(); ++index) {
      const auto& element = global.initializer_elements[index];
      if (element.kind != bir::Value::Kind::Immediate ||
          element.type != bir::TypeKind::I32) {
        return std::nullopt;
      }
      words[index] = static_cast<std::int32_t>(element.immediate);
    }
    return words;
  }

  if (global.size_bytes != 4) {
    return std::nullopt;
  }
  if (!global.initializer.has_value()) {
    return words;
  }
  if (global.initializer->kind != bir::Value::Kind::Immediate ||
      global.initializer->type != bir::TypeKind::I32) {
    return std::nullopt;
  }
  words[0] = static_cast<std::int32_t>(global.initializer->immediate);
  return words;
}

bool is_simple_defined_i32_global(const c4c::backend::bir::Global& global) {
  return simple_defined_i32_global_words(global).has_value();
}

bool is_simple_zero_storage_element(const c4c::backend::bir::Value& element) {
  namespace bir = c4c::backend::bir;

  if (element.kind != bir::Value::Kind::Immediate || element.immediate != 0) {
    return false;
  }
  switch (element.type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::I128:
      return true;
    default:
      return false;
  }
}

bool is_simple_zero_initialized_global_storage(
    const c4c::backend::bir::Global& global) {
  return !global.is_extern &&
         !global.is_thread_local &&
         global.size_bytes != 0 &&
         !global.initializer.has_value() &&
         !global.initializer_symbol_name.has_value() &&
         global.initializer_symbol_name_id == c4c::kInvalidLinkName &&
         !global.initializer_elements.empty() &&
         std::all_of(global.initializer_elements.begin(),
                     global.initializer_elements.end(),
                     is_simple_zero_storage_element);
}

bool supports_simple_i32_global_field_access(
    const c4c::backend::bir::Global& global) {
  return is_simple_defined_i32_global(global) ||
         is_simple_zero_initialized_global_storage(global);
}

bool is_no_storage_extern_global(const c4c::backend::bir::Global& global) {
  return global.is_extern &&
         !global.is_thread_local &&
         !global.is_constant &&
         !global.initializer.has_value() &&
         !global.initializer_symbol_name.has_value() &&
         global.initializer_symbol_name_id == c4c::kInvalidLinkName &&
         global.initializer_elements.empty();
}

std::size_t string_constant_storage_size(
    const c4c::backend::bir::StringConstant& constant) {
  return constant.bytes.empty() || constant.bytes.back() != '\0'
             ? constant.bytes.size() + 1
             : constant.bytes.size();
}

const c4c::backend::bir::StringConstant* find_string_constant(
    const c4c::backend::bir::Module& module,
    std::string_view name) {
  const auto it = std::find_if(
      module.string_constants.begin(),
      module.string_constants.end(),
      [&](const c4c::backend::bir::StringConstant& constant) {
        return constant.name == name;
      });
  return it == module.string_constants.end() ? nullptr : &*it;
}

const c4c::backend::bir::StringConstant* string_constant_for_global(
    const c4c::backend::bir::Module& module,
    const c4c::backend::bir::Global& global) {
  namespace bir = c4c::backend::bir;

  if (!global.is_extern ||
      global.is_thread_local ||
      !global.is_constant ||
      global.type != bir::TypeKind::I8 ||
      global.size_bytes == 0 ||
      global.initializer.has_value() ||
      global.initializer_symbol_name.has_value() ||
      global.initializer_symbol_name_id != c4c::kInvalidLinkName ||
      !global.initializer_elements.empty()) {
    return nullptr;
  }

  const auto* constant = find_string_constant(module, global.name);
  if (constant == nullptr ||
      constant->align_bytes > std::max<std::size_t>(global.align_bytes, 1) ||
      string_constant_storage_size(*constant) != global.size_bytes) {
    return nullptr;
  }
  return constant;
}

bool is_supported_prepared_global_storage(
    const c4c::backend::bir::Module& module,
    const c4c::backend::bir::Global& global) {
  return is_no_storage_extern_global(global) ||
         is_simple_defined_i32_global(global) ||
         is_simple_zero_initialized_global_storage(global) ||
         string_constant_for_global(module, global) != nullptr;
}

std::string byte_directive(std::string_view bytes) {
  std::string out = "    .byte ";
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    if (index != 0) {
      out += ", ";
    }
    out += std::to_string(static_cast<unsigned int>(
        static_cast<unsigned char>(bytes[index])));
  }
  if (bytes.empty()) {
    out += "0";
  } else if (bytes.back() != '\0') {
    out += ", 0";
  }
  out += "\n";
  return out;
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

std::string function_label(const c4c::backend::bir::Module& module,
                           const c4c::backend::bir::Function& function) {
  if (!function.name.empty()) {
    return function.name;
  }
  if (function.link_name_id != c4c::kInvalidLinkName) {
    const std::string_view spelling =
        module.names.link_names.spelling(function.link_name_id);
    if (!spelling.empty()) {
      return std::string{spelling};
    }
  }
  return function.name;
}

bool all_zero_words(const std::vector<std::int32_t>& words) {
  return std::all_of(words.begin(), words.end(), [](std::int32_t word) {
    return word == 0;
  });
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

std::string symbol_pointer_fallback_name(const c4c::backend::bir::Value& value) {
  if (!value.name.empty() && value.name.front() == '@') {
    return value.name.substr(1);
  }
  return value.name;
}

const c4c::backend::prepare::PreparedMemoryAccess* simple_i32_global_access_for(
    const PreparedCurrentInstructionContext& context,
    std::optional<c4c::ValueNameId> result_value_name,
    std::optional<c4c::ValueNameId> stored_value_name,
    std::int64_t expected_byte_offset) {
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
      access->address.byte_offset != expected_byte_offset ||
      access->address.size_bytes != 4 ||
      access->address.align_bytes < 4 ||
      !access->address.can_use_base_plus_offset) {
    return nullptr;
  }
  return access;
}

void append_fragment_global(RiscvEncodedFragment& destination,
                            RiscvEncodedFragment source) {
  const auto base_offset = destination.bytes.size();
  destination.bytes.insert(destination.bytes.end(),
                           source.bytes.begin(),
                           source.bytes.end());
  for (auto& label : source.labels) {
    label.offset_bytes += base_offset;
    destination.labels.push_back(std::move(label));
  }
  for (auto& fixup : source.fixups) {
    fixup.offset_bytes += base_offset;
    destination.fixups.push_back(std::move(fixup));
  }
}

std::optional<std::int64_t> integer_immediate_for_value_global(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  switch (value.type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
      break;
    default:
      return std::nullopt;
  }
  if (value.kind == bir::Value::Kind::Immediate) {
    return value.immediate;
  }
  const auto* home = prepared_value_home_for(names, lookups, value);
  if (home == nullptr) {
    return std::nullopt;
  }
  const auto report =
      prepare::verify_prepared_rematerializable_integer_immediate_contract(home);
  if (report.owner_class != prepare::PreparedContractOwnerClass::Coherent) {
    return std::nullopt;
  }
  const auto fact = prepare::as_rematerializable_integer_immediate_fact(*home);
  return fact.has_value() ? std::optional<std::int64_t>{fact->signed_value}
                          : std::nullopt;
}

bool is_rv64_null_pointer_value_global(const c4c::backend::bir::Value& value) {
  return value.kind == bir::Value::Kind::Immediate &&
         value.type == bir::TypeKind::Ptr &&
         value.immediate == 0 && value.immediate_bits == 0;
}

std::optional<std::uint32_t> gpr_register_number_for_value_global(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  const auto* home = prepared_value_home_for(names, lookups, value);
  return home == nullptr ? std::nullopt
                         : rv64_prepared_gpr_register_number_for_home(*home);
}

std::optional<std::uint32_t> fpr_register_number_for_home_global(
    const c4c::backend::prepare::PreparedValueHome& home) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      !home.target_register_identity.has_value()) {
    return std::nullopt;
  }
  const auto& identity = *home.target_register_identity;
  if (identity.target_arch != c4c::TargetArch::Riscv64 ||
      identity.bank != prepare::PreparedRegisterBank::Fpr ||
      identity.register_class != prepare::PreparedRegisterClass::Float ||
      identity.physical_index > 31) {
    return std::nullopt;
  }
  return static_cast<std::uint32_t>(identity.physical_index);
}

std::optional<std::uint32_t> fpr_register_number_for_value_global(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  const auto* home = prepared_value_home_for(names, lookups, value);
  return home == nullptr ? std::nullopt
                         : fpr_register_number_for_home_global(*home);
}

std::optional<std::size_t> prepared_stack_slot_home_absolute_offset_for_value_global(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value,
    std::size_t stack_frame_bytes) {
  const auto* home = prepared_value_home_for(names, lookups, value);
  if (home == nullptr) {
    return std::nullopt;
  }
  const auto size_bytes = rv64_global_scalar_memory_size_for_type(value.type);
  if (!size_bytes.has_value()) {
    return std::nullopt;
  }
  return rv64_prepared_stack_slot_home_absolute_offset(stack_layout,
                                                       *home,
                                                       stack_frame_bytes,
                                                       *size_bytes);
}

std::optional<std::uint32_t> rv64_global_load_funct3_for_size(
    std::size_t size_bytes) {
  switch (size_bytes) {
    case 1:
      return 0;
    case 2:
      return 1;
    case 4:
      return 2;
    case 8:
      return 3;
    default:
      return std::nullopt;
  }
}

std::optional<std::uint32_t> rv64_global_store_funct3_for_size(
    std::size_t size_bytes) {
  switch (size_bytes) {
    case 1:
      return 0;
    case 2:
      return 1;
    case 4:
      return 2;
    case 8:
      return 3;
    default:
      return std::nullopt;
  }
}

bool append_rv64_load_global_base_to_register(RiscvEncodedFragment& fragment,
                                              std::uint32_t destination_register,
                                              std::uint32_t base_register,
                                              std::int32_t offset,
                                              std::size_t size_bytes) {
  if (!fits_signed_12_bit_immediate(offset)) {
    return false;
  }
  const auto funct3 = rv64_global_load_funct3_for_size(size_bytes);
  if (!funct3.has_value()) {
    return false;
  }
  rv64_append_le32(fragment.bytes,
                   rv64_encode_i_type(0x03,
                                       destination_register,
                                       *funct3,
                                       base_register,
                                       offset));
  return true;
}

bool append_rv64_load_global_base_to_fpr(RiscvEncodedFragment& fragment,
                                         std::uint32_t destination_register,
                                         std::uint32_t base_register,
                                         std::int32_t offset,
                                         c4c::backend::bir::TypeKind type) {
  if (!fits_signed_12_bit_immediate(offset)) {
    return false;
  }
  std::optional<std::uint32_t> funct3;
  if (type == bir::TypeKind::F32) {
    funct3 = 2;
  } else if (type == bir::TypeKind::F64) {
    funct3 = 3;
  }
  if (!funct3.has_value()) {
    return false;
  }
  rv64_append_le32(fragment.bytes,
                   rv64_encode_i_type(0x07,
                                       destination_register,
                                       *funct3,
                                       base_register,
                                       offset));
  return true;
}

bool append_rv64_store_register_to_global_base(RiscvEncodedFragment& fragment,
                                               std::uint32_t source_register,
                                               std::uint32_t base_register,
                                               std::int32_t offset,
                                               std::size_t size_bytes) {
  if (!fits_signed_12_bit_immediate(offset)) {
    return false;
  }
  const auto funct3 = rv64_global_store_funct3_for_size(size_bytes);
  if (!funct3.has_value()) {
    return false;
  }
  rv64_append_le32(fragment.bytes,
                   rv64_encode_s_type(0x23,
                                       *funct3,
                                       base_register,
                                       source_register,
                                       offset));
  return true;
}

bool append_rv64_move_value_to_register_global(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value,
    std::size_t stack_frame_bytes) {
  if (is_rv64_null_pointer_value_global(value)) {
    append_rv64_prepared_load_immediate(fragment, destination, 0);
    return true;
  }
  const auto immediate = integer_immediate_for_value_global(names, lookups, value);
  if (immediate.has_value()) {
    append_rv64_prepared_load_immediate(fragment, destination, *immediate);
    return true;
  }
  const auto source = gpr_register_number_for_value_global(names, lookups, value);
  if (source.has_value()) {
    append_rv64_prepared_move(fragment, destination, *source);
    return true;
  }
  const auto stack_offset =
      prepared_stack_slot_home_absolute_offset_for_value_global(stack_layout,
                                                                names,
                                                                lookups,
                                                                value,
                                                                stack_frame_bytes);
  if (!stack_offset.has_value()) {
    return false;
  }
  const auto size_bytes = rv64_global_scalar_memory_size_for_type(value.type);
  if (!size_bytes.has_value()) {
    return false;
  }
  return append_rv64_prepared_load_stack_offset_to_register(
      fragment, destination, *stack_offset, *size_bytes);
}

std::optional<std::pair<std::string, RiscvObjectFixupTargetKind>>
prepared_direct_global_materialization_symbol(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedAddressMaterialization& materialization) {
  if (materialization.kind != prepare::PreparedAddressMaterializationKind::DirectGlobal ||
      materialization.address_space != bir::AddressSpace::Default ||
      materialization.is_thread_local || materialization.has_tls_address_space ||
      materialization.address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::Direct ||
      !materialization.symbol_name.has_value()) {
    return std::nullopt;
  }

  const std::string_view prepared_symbol =
      prepare::prepared_link_name(prepared.names, *materialization.symbol_name);
  if (prepared_symbol.empty()) {
    return std::nullopt;
  }

  for (const auto& global : prepared.module.globals) {
    std::string label = global.name;
    if (global.link_name_id != c4c::kInvalidLinkName) {
      const std::string_view spelling =
          prepared.module.names.link_names.spelling(global.link_name_id);
      if (!spelling.empty()) {
        label = std::string{spelling};
      }
    }
    if (label == prepared_symbol) {
      return std::pair<std::string, RiscvObjectFixupTargetKind>{
          std::string{prepared_symbol}, RiscvObjectFixupTargetKind::Object};
    }
  }

  for (const auto& function : prepared.module.functions) {
    if (function.name == prepared_symbol) {
      return std::pair<std::string, RiscvObjectFixupTargetKind>{
          std::string{prepared_symbol}, RiscvObjectFixupTargetKind::Function};
    }
  }

  return std::pair<std::string, RiscvObjectFixupTargetKind>{
      std::string{prepared_symbol}, RiscvObjectFixupTargetKind::Object};
}

std::optional<std::pair<std::string, RiscvObjectFixupTargetKind>>
prepared_address_materialization_symbol(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedAddressMaterialization& materialization) {
  if (materialization.kind ==
      prepare::PreparedAddressMaterializationKind::DirectGlobal) {
    return prepared_direct_global_materialization_symbol(prepared, materialization);
  }
  if (materialization.kind ==
          prepare::PreparedAddressMaterializationKind::StringConstant &&
      materialization.address_space == bir::AddressSpace::Default &&
      !materialization.is_thread_local && !materialization.has_tls_address_space &&
      materialization.text_name.has_value()) {
    const std::string_view label =
        prepared.names.texts.lookup(*materialization.text_name);
    if (!label.empty()) {
      return std::pair<std::string, RiscvObjectFixupTargetKind>{
          std::string{label}, RiscvObjectFixupTargetKind::Object};
    }
  }
  return std::nullopt;
}

const c4c::backend::bir::Global* prepared_global_for_access_symbol(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedMemoryAccess& access) {
  if (!access.address.symbol_name.has_value()) {
    return nullptr;
  }
  const std::string_view symbol =
      prepare::prepared_link_name(prepared.names, *access.address.symbol_name);
  if (symbol.empty()) {
    return nullptr;
  }
  for (const auto& global : prepared.module.globals) {
    std::string label = global.name;
    if (global.link_name_id != c4c::kInvalidLinkName) {
      const std::string_view spelling =
          prepared.module.names.link_names.spelling(global.link_name_id);
      if (!spelling.empty()) {
        label = std::string{spelling};
      }
    }
    if (label == symbol) {
      return &global;
    }
  }
  return nullptr;
}

bool prepared_global_access_is_supported(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::optional<c4c::ValueNameId> result_value_name,
    std::optional<c4c::ValueNameId> stored_value_name,
    std::size_t size_bytes) {
  if (access == nullptr ||
      access->result_value_name != result_value_name ||
      access->stored_value_name != stored_value_name ||
      access->address_space != bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !access->address.symbol_name.has_value() ||
      access->address.size_bytes != size_bytes ||
      access->address.align_bytes < size_bytes ||
      !access->address.can_use_base_plus_offset ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return false;
  }
  const auto policy =
      prepare::prepared_global_symbol_address_policy(access->address,
                                                     &prepared.target_profile);
  if (!policy.has_value() ||
      *policy != bir::GlobalAddressMaterializationPolicy::Direct) {
    return false;
  }
  const auto* global = prepared_global_for_access_symbol(prepared, *access);
  if (global == nullptr ||
      access->address.byte_offset < 0 ||
      static_cast<std::uint64_t>(access->address.byte_offset) > global->size_bytes ||
      size_bytes > global->size_bytes -
                       static_cast<std::uint64_t>(access->address.byte_offset)) {
    return false;
  }
  return true;
}

}  // namespace

std::optional<std::size_t> rv64_global_scalar_memory_size_for_type(
    c4c::backend::bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I8:
      return std::size_t{1};
    case bir::TypeKind::I16:
      return std::size_t{2};
    case bir::TypeKind::I32:
      return std::size_t{4};
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return std::size_t{8};
    default:
      return std::nullopt;
  }
}

std::optional<std::size_t> rv64_global_floating_memory_size_for_type(
    c4c::backend::bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::F64:
      return std::size_t{8};
    default:
      return std::nullopt;
  }
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_symbol_address_materialization(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::BinaryInst& binary) {
  if (lookups == nullptr ||
      binary.opcode != bir::BinaryOpcode::Add ||
      binary.result.type != bir::TypeKind::Ptr ||
      binary.result.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto destination =
      gpr_register_number_for_value_global(names, lookups, binary.result);
  if (!destination.has_value()) {
    return std::nullopt;
  }
  const auto result_value_name = names.value_names.find(binary.result.name);
  if (result_value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }

  std::vector<const c4c::backend::prepare::PreparedAddressMaterialization*>
      materialization_candidates;
  if (const auto* materializations =
          prepare::find_indexed_prepared_address_materializations(
              &lookups->address_materializations,
              block_label)) {
    materialization_candidates = *materializations;
  } else {
    for (const auto& entry : lookups->address_materializations.materializations_by_block) {
      materialization_candidates.insert(materialization_candidates.end(),
                                        entry.second.begin(),
                                        entry.second.end());
    }
  }

  const c4c::backend::prepare::PreparedAddressMaterialization* selected = nullptr;
  for (const auto* materialization : materialization_candidates) {
    if (materialization == nullptr ||
        materialization->inst_index != instruction_index ||
        materialization->result_value_name != result_value_name) {
      continue;
    }
    if (!prepared_address_materialization_symbol(prepared, *materialization).has_value()) {
      continue;
    }
    if (selected != nullptr) {
      return std::nullopt;
    }
    selected = materialization;
  }
  if (selected == nullptr) {
    return std::nullopt;
  }
  auto symbol = prepared_address_materialization_symbol(prepared, *selected);
  if (!symbol.has_value()) {
    return std::nullopt;
  }

  const std::string auipc_label = ".Lpcrel_hi_" +
                                  std::to_string(selected->function_name) + "_" +
                                  std::to_string(selected->block_label) + "_" +
                                  std::to_string(selected->inst_index);
  return make_rv64_pcrel_address_fragment(*destination,
                                           std::move(symbol->first),
                                           auipc_label,
                                           symbol->second,
                                           selected->byte_offset);
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_load_global(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::LoadGlobalInst& load,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes) {
  if (load.result.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto size_bytes = rv64_global_scalar_memory_size_for_type(load.result.type);
  const auto floating_size_bytes =
      rv64_global_floating_memory_size_for_type(load.result.type);
  if (floating_size_bytes.has_value()) {
    const auto result_value_name = names.value_names.find(load.result.name);
    if (result_value_name == c4c::kInvalidValueName ||
        !prepared_global_access_is_supported(prepared,
                                             access,
                                             result_value_name,
                                             std::nullopt,
                                             *floating_size_bytes)) {
      return std::nullopt;
    }
    const std::string_view symbol =
        prepare::prepared_link_name(prepared.names, *access->address.symbol_name);
    const auto destination =
        fpr_register_number_for_value_global(names, lookups, load.result);
    if (!destination.has_value()) {
      return std::nullopt;
    }
    const std::uint32_t address_register = 6;
    RiscvEncodedFragment fragment = make_rv64_pcrel_address_fragment(
        address_register,
        std::string{symbol},
        ".Lpcrel_hi_global_load_" + std::to_string(access->function_name) + "_" +
            std::to_string(access->block_label) + "_" +
            std::to_string(access->inst_index),
        RiscvObjectFixupTargetKind::Object,
        0);
    if (!append_rv64_load_global_base_to_fpr(
            fragment,
            *destination,
            address_register,
            static_cast<std::int32_t>(access->address.byte_offset),
            load.result.type)) {
      return std::nullopt;
    }
    return fragment;
  }
  if (!size_bytes.has_value()) {
    return std::nullopt;
  }
  const auto result_value_name = names.value_names.find(load.result.name);
  if (result_value_name == c4c::kInvalidValueName ||
      !prepared_global_access_is_supported(prepared,
                                           access,
                                           result_value_name,
                                           std::nullopt,
                                           *size_bytes)) {
    return std::nullopt;
  }
  const std::string_view symbol =
      prepare::prepared_link_name(prepared.names, *access->address.symbol_name);
  const auto destination =
      gpr_register_number_for_value_global(names, lookups, load.result);
  const auto destination_offset =
      prepared_stack_slot_home_absolute_offset_for_value_global(stack_layout,
                                                                names,
                                                                lookups,
                                                                load.result,
                                                                stack_frame_bytes);
  if (!destination.has_value() && !destination_offset.has_value()) {
    return std::nullopt;
  }
  const std::uint32_t destination_register = destination.value_or(6);
  RiscvEncodedFragment fragment = make_rv64_pcrel_address_fragment(
      destination_register,
      std::string{symbol},
      ".Lpcrel_hi_global_load_" + std::to_string(access->function_name) + "_" +
          std::to_string(access->block_label) + "_" +
          std::to_string(access->inst_index),
      RiscvObjectFixupTargetKind::Object,
      0);
  if (!append_rv64_load_global_base_to_register(fragment,
                                               destination_register,
                                               destination_register,
                                               static_cast<std::int32_t>(
                                                   access->address.byte_offset),
                                               *size_bytes)) {
    return std::nullopt;
  }
  if (destination_offset.has_value() &&
      !append_rv64_prepared_store_register_to_stack_offset(fragment,
                                                          destination_register,
                                                          *destination_offset,
                                                          *size_bytes)) {
    return std::nullopt;
  }
  return fragment;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_store_global(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::StoreGlobalInst& store,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes) {
  const auto size_bytes = rv64_global_scalar_memory_size_for_type(store.value.type);
  if (!size_bytes.has_value()) {
    return std::nullopt;
  }
  std::optional<c4c::ValueNameId> stored_value_name;
  if (store.value.kind == bir::Value::Kind::Named) {
    stored_value_name = names.value_names.find(store.value.name);
    if (*stored_value_name == c4c::kInvalidValueName) {
      return std::nullopt;
    }
  }
  if (!prepared_global_access_is_supported(prepared,
                                           access,
                                           std::nullopt,
                                           stored_value_name,
                                           *size_bytes)) {
    return std::nullopt;
  }
  const std::string_view symbol =
      prepare::prepared_link_name(prepared.names, *access->address.symbol_name);
  RiscvEncodedFragment fragment;
  if (!append_rv64_move_value_to_register_global(fragment,
                                                 6,
                                                 stack_layout,
                                                 names,
                                                 lookups,
                                                 store.value,
                                                 stack_frame_bytes)) {
    return std::nullopt;
  }
  append_fragment_global(
      fragment,
      make_rv64_pcrel_address_fragment(
          5,
          std::string{symbol},
          ".Lpcrel_hi_global_store_" + std::to_string(access->function_name) +
              "_" + std::to_string(access->block_label) + "_" +
              std::to_string(access->inst_index),
          RiscvObjectFixupTargetKind::Object,
          0));
  if (!append_rv64_store_register_to_global_base(
          fragment,
          6,
          5,
          static_cast<std::int32_t>(access->address.byte_offset),
          *size_bytes)) {
    return std::nullopt;
  }
  return fragment;
}

bool append_prepared_global_storage_asm(
    std::string& out,
    const c4c::backend::prepare::PreparedBirModule& prepared) {
  if (prepared.module.globals.empty()) {
    return true;
  }

  for (const auto& global : prepared.module.globals) {
    if (is_no_storage_extern_global(global)) {
      continue;
    }
    if (global_label(prepared.module, global).empty()) {
      return false;
    }
    if (!is_supported_prepared_global_storage(prepared.module, global)) {
      return false;
    }
  }

  bool wrote_rodata = false;
  for (const auto& global : prepared.module.globals) {
    const auto* constant = string_constant_for_global(prepared.module, global);
    if (constant == nullptr) {
      continue;
    }
    if (!wrote_rodata) {
      out += "    .section .rodata\n";
      wrote_rodata = true;
    }
    const auto align_bytes = std::max<std::size_t>(global.align_bytes, constant->align_bytes);
    if (align_bytes > 1) {
      out += "    .balign ";
      out += std::to_string(align_bytes);
      out += "\n";
    }
    out += global_label(prepared.module, global);
    out += ":\n";
    out += byte_directive(constant->bytes);
  }

  bool wrote_data = false;
  for (const auto& global : prepared.module.globals) {
    if (is_no_storage_extern_global(global) ||
        string_constant_for_global(prepared.module, global) != nullptr) {
      continue;
    }
    const auto label = global_label(prepared.module, global);
    const auto words = simple_defined_i32_global_words(global);
    if (!words.has_value() || all_zero_words(*words)) {
      continue;
    }
    if (!wrote_data) {
      out += "    .data\n";
      wrote_data = true;
    }
    out += "    .balign 4\n";
    out += label;
    out += ":\n";
    for (const std::int32_t word : *words) {
      out += "    .word ";
      out += std::to_string(word);
      out += "\n";
    }
  }

  bool wrote_bss = false;
  for (const auto& global : prepared.module.globals) {
    if (is_no_storage_extern_global(global) ||
        string_constant_for_global(prepared.module, global) != nullptr) {
      continue;
    }
    const auto label = global_label(prepared.module, global);
    const auto words = simple_defined_i32_global_words(global);
    if ((!words.has_value() || !all_zero_words(*words)) &&
        !is_simple_zero_initialized_global_storage(global)) {
      continue;
    }
    if (!wrote_bss) {
      out += "    .bss\n";
      wrote_bss = true;
    }
    const std::size_t align_bytes =
        std::max<std::size_t>(global.align_bytes, words.has_value() ? 4 : 1);
    if (align_bytes > 1) {
      out += "    .balign ";
      out += std::to_string(align_bytes);
      out += "\n";
    }
    out += label;
    out += ":\n";
    out += "    .zero ";
    out += std::to_string(global.size_bytes);
    out += "\n";
  }
  return true;
}

std::optional<std::string> emit_riscv_direct_global_address_materialization(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedAddressMaterialization& materialization,
    std::string_view destination_register) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (destination_register.empty() ||
      materialization.kind != prepare::PreparedAddressMaterializationKind::DirectGlobal ||
      materialization.address_space != bir::AddressSpace::Default ||
      materialization.is_thread_local ||
      materialization.has_tls_address_space ||
      materialization.address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::Direct ||
      !materialization.symbol_name.has_value() ||
      materialization.byte_offset != 0) {
    return std::nullopt;
  }

  const auto* global = find_prepared_global(
      prepared,
      *materialization.symbol_name,
      prepared_link_name_spelling(prepared, *materialization.symbol_name));
  if (global != nullptr) {
    if (!is_simple_defined_i32_global(*global)) {
      return std::nullopt;
    }
    const auto label = global_label(prepared.module, *global);
    if (label.empty()) {
      return std::nullopt;
    }

    return "    lla " + std::string{destination_register} + ", " + label + "\n";
  }

  const auto* function = find_prepared_function(
      prepared,
      *materialization.symbol_name,
      prepared_link_name_spelling(prepared, *materialization.symbol_name));
  if (function == nullptr || function->is_declaration) {
    return std::nullopt;
  }
  const auto label = function_label(prepared.module, *function);
  if (label.empty()) {
    return std::nullopt;
  }

  return "    lla " + std::string{destination_register} + ", " + label + "\n";
}

std::optional<std::string> emit_riscv_direct_function_address_materialization(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::Value& value,
    std::string_view destination_register) {
  namespace bir = c4c::backend::bir;

  if (destination_register.empty() ||
      value.kind != bir::Value::Kind::Named ||
      value.type != bir::TypeKind::Ptr ||
      (value.pointer_symbol_link_name_id == c4c::kInvalidLinkName &&
       value.name.empty())) {
    return std::nullopt;
  }

  const auto* function = find_prepared_function(
      prepared,
      value.pointer_symbol_link_name_id,
      symbol_pointer_fallback_name(value));
  if (function == nullptr || function->is_declaration) {
    return std::nullopt;
  }
  const auto label = function_label(prepared.module, *function);
  if (label.empty()) {
    return std::nullopt;
  }

  return "    lla " + std::string{destination_register} + ", " + label + "\n";
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
      load.byte_offset % 4 != 0 ||
      load.byte_offset > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    return std::nullopt;
  }
  const auto byte_offset = static_cast<std::int64_t>(load.byte_offset);
  if (!fits_signed_12_bit_immediate(byte_offset)) {
    return std::nullopt;
  }
  const auto result_name = context.names.value_names.find(load.result.name);
  if (result_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto* access = simple_i32_global_access_for(
      context,
      result_name,
      std::nullopt,
      byte_offset);
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
  if (global == nullptr || access == nullptr ||
      !supports_simple_i32_global_field_access(*global) ||
      load.byte_offset > global->size_bytes ||
      4 > global->size_bytes - load.byte_offset) {
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
  out += "    lw " + *destination_register + ", " + std::to_string(byte_offset) + "(" +
         *destination_register + ")\n";
  return out;
}

std::optional<std::string> emit_riscv_simple_store_global(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::StoreGlobalInst& store,
    const PreparedCurrentInstructionContext& context) {
  namespace bir = c4c::backend::bir;

  if (store.value.type != bir::TypeKind::I32 ||
      (store.global_name_id == c4c::kInvalidLinkName && store.global_name.empty()) ||
      store.byte_offset % 4 != 0 ||
      store.byte_offset > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    return std::nullopt;
  }
  const auto byte_offset = static_cast<std::int64_t>(store.byte_offset);
  if (!fits_signed_12_bit_immediate(byte_offset)) {
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
      stored_name,
      byte_offset);
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
  if (global == nullptr || access == nullptr ||
      !supports_simple_i32_global_field_access(*global) ||
      store.byte_offset > global->size_bytes ||
      4 > global->size_bytes - store.byte_offset) {
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
  out += "    sw t1, " + std::to_string(byte_offset) + "(t0)\n";
  return out;
}

}  // namespace c4c::backend::riscv::codegen
