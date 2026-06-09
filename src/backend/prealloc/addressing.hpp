#pragma once

#include "frame.hpp"
#include "names.hpp"

#include "../../target_profile.hpp"
#include "../bir/bir.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace c4c::backend::prepare {

struct PreparedBirModule;
struct PreparedEdgePublicationSourceProducer;
struct PreparedEdgePublicationSourceProducerLookups;
struct PreparedSameBlockScalarProducer;
struct PreparedValueHomeLookups;

enum class PreparedAddressBaseKind {
  None,
  FrameSlot,
  GlobalSymbol,
  PointerValue,
  StringConstant,
};

[[nodiscard]] constexpr std::string_view prepared_address_base_kind_name(
    PreparedAddressBaseKind kind) {
  switch (kind) {
    case PreparedAddressBaseKind::None:
      return "none";
    case PreparedAddressBaseKind::FrameSlot:
      return "frame_slot";
    case PreparedAddressBaseKind::GlobalSymbol:
      return "global_symbol";
    case PreparedAddressBaseKind::PointerValue:
      return "pointer_value";
    case PreparedAddressBaseKind::StringConstant:
      return "string_constant";
  }
  return "unknown";
}

struct PreparedAddress {
  PreparedAddressBaseKind base_kind = PreparedAddressBaseKind::None;
  std::optional<PreparedFrameSlotId> frame_slot_id;
  std::optional<LinkNameId> symbol_name;
  std::optional<ValueNameId> pointer_value_name;
  bir::GlobalAddressMaterializationPolicy global_address_materialization_policy =
      bir::GlobalAddressMaterializationPolicy::Unspecified;
  std::int64_t byte_offset = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  bool can_use_base_plus_offset = false;
};

[[nodiscard]] inline std::optional<bir::GlobalAddressMaterializationPolicy>
prepared_global_symbol_address_policy(
    const PreparedAddress& address,
    const c4c::TargetProfile* target_profile) {
  if (address.base_kind != PreparedAddressBaseKind::GlobalSymbol ||
      !address.symbol_name.has_value()) {
    return std::nullopt;
  }
  if (address.global_address_materialization_policy !=
      bir::GlobalAddressMaterializationPolicy::Unspecified) {
    return address.global_address_materialization_policy;
  }
  if (target_profile != nullptr &&
      target_profile->relocation_model == c4c::TargetRelocationModel::Static) {
    return bir::GlobalAddressMaterializationPolicy::Direct;
  }
  return std::nullopt;
}

enum class PreparedValueHomeKind;

enum class PreparedAddressMaterializationKind {
  None,
  FrameSlot,
  DirectGlobal,
  StringConstant,
  Label,
  GotGlobal,
  TlsGlobal,
};

[[nodiscard]] constexpr std::string_view prepared_address_materialization_kind_name(
    PreparedAddressMaterializationKind kind) {
  switch (kind) {
    case PreparedAddressMaterializationKind::None:
      return "none";
    case PreparedAddressMaterializationKind::FrameSlot:
      return "frame_slot";
    case PreparedAddressMaterializationKind::DirectGlobal:
      return "direct_global";
    case PreparedAddressMaterializationKind::StringConstant:
      return "string_constant";
    case PreparedAddressMaterializationKind::Label:
      return "label";
    case PreparedAddressMaterializationKind::GotGlobal:
      return "got_global";
    case PreparedAddressMaterializationKind::TlsGlobal:
      return "tls_global";
  }
  return "unknown";
}

enum class PreparedTlsMaterializationModel {
  None,
  LocalExecThreadPointerRelative,
};

[[nodiscard]] constexpr std::string_view prepared_tls_materialization_model_name(
    PreparedTlsMaterializationModel model) {
  switch (model) {
    case PreparedTlsMaterializationModel::None:
      return "none";
    case PreparedTlsMaterializationModel::LocalExecThreadPointerRelative:
      return "local_exec_thread_pointer_relative";
  }
  return "unknown";
}

enum class PreparedTlsThreadPointerRegister {
  None,
  Aarch64TpidrEl0,
};

[[nodiscard]] constexpr std::string_view prepared_tls_thread_pointer_register_name(
    PreparedTlsThreadPointerRegister reg) {
  switch (reg) {
    case PreparedTlsThreadPointerRegister::None:
      return "none";
    case PreparedTlsThreadPointerRegister::Aarch64TpidrEl0:
      return "aarch64_tpidr_el0";
  }
  return "unknown";
}

enum class PreparedTlsRelocationKind {
  None,
  Aarch64TprelHi12,
  Aarch64TprelLo12Nc,
};

[[nodiscard]] constexpr std::string_view prepared_tls_relocation_kind_name(
    PreparedTlsRelocationKind kind) {
  switch (kind) {
    case PreparedTlsRelocationKind::None:
      return "none";
    case PreparedTlsRelocationKind::Aarch64TprelHi12:
      return "aarch64_tprel_hi12";
    case PreparedTlsRelocationKind::Aarch64TprelLo12Nc:
      return "aarch64_tprel_lo12_nc";
  }
  return "unknown";
}

struct PreparedAddressMaterialization {
  FunctionNameId function_name = kInvalidFunctionName;
  BlockLabelId block_label = kInvalidBlockLabel;
  std::size_t inst_index = 0;
  PreparedAddressMaterializationKind kind = PreparedAddressMaterializationKind::None;
  std::optional<ValueNameId> result_value_name;
  std::optional<PreparedValueId> result_value_id;
  std::optional<PreparedValueHomeKind> result_home_kind;
  std::optional<PreparedFrameSlotId> frame_slot_id;
  std::optional<LinkNameId> symbol_name;
  std::optional<TextId> text_name;
  std::optional<BlockLabelId> target_label;
  bir::GlobalAddressMaterializationPolicy address_materialization_policy =
      bir::GlobalAddressMaterializationPolicy::Unspecified;
  std::int64_t byte_offset = 0;
  bir::AddressSpace address_space = bir::AddressSpace::Default;
  bool is_thread_local = false;
  bool has_tls_address_space = false;
  PreparedTlsMaterializationModel tls_model = PreparedTlsMaterializationModel::None;
  PreparedTlsThreadPointerRegister tls_thread_pointer_register =
      PreparedTlsThreadPointerRegister::None;
  PreparedTlsRelocationKind tls_high_relocation = PreparedTlsRelocationKind::None;
  PreparedTlsRelocationKind tls_low_relocation = PreparedTlsRelocationKind::None;
};

struct PreparedMemoryAccess {
  FunctionNameId function_name = kInvalidFunctionName;
  BlockLabelId block_label = kInvalidBlockLabel;
  std::size_t inst_index = 0;
  std::optional<ValueNameId> result_value_name;
  std::optional<ValueNameId> stored_value_name;
  bir::AddressSpace address_space = bir::AddressSpace::Default;
  bool is_volatile = false;
  PreparedAddress address;
};

struct PreparedSameBlockGlobalLoadAccess {
  const bir::LoadGlobalInst* load_global = nullptr;
  const PreparedMemoryAccess* access = nullptr;
};

struct PreparedSameBlockLoadLocalStoredValueSource {
  bir::Value stored_value;
  std::size_t store_instruction_index = 0;
  const PreparedEdgePublicationSourceProducer* load_producer = nullptr;
  const PreparedMemoryAccess* load_access = nullptr;
  const PreparedMemoryAccess* store_access = nullptr;
};

struct PreparedAddressingFunction {
  FunctionNameId function_name = kInvalidFunctionName;
  std::size_t frame_size_bytes = 0;
  std::size_t frame_alignment_bytes = 0;
  std::vector<PreparedMemoryAccess> accesses;
  std::vector<PreparedAddressMaterialization> address_materializations;
};

struct PreparedAddressing {
  std::vector<PreparedAddressingFunction> functions;
};

[[nodiscard]] inline const PreparedAddressingFunction* find_prepared_addressing_function(
    const PreparedAddressing& addressing,
    FunctionNameId function_name) {
  for (const auto& function_addressing : addressing.functions) {
    if (function_addressing.function_name == function_name) {
      return &function_addressing;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedMemoryAccess* find_prepared_memory_access(
    const PreparedAddressingFunction& function_addressing,
    BlockLabelId block_label,
    std::size_t inst_index) {
  for (const auto& access : function_addressing.accesses) {
    if (access.block_label == block_label && access.inst_index == inst_index) {
      return &access;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedMemoryAccess* find_prepared_memory_access_by_result_name(
    const PreparedNameTables& names,
    const PreparedAddressingFunction& function_addressing,
    std::string_view result_value_name) {
  const ValueNameId value_name_id = names.value_names.find(result_value_name);
  if (value_name_id == kInvalidValueName) {
    return nullptr;
  }

  const PreparedMemoryAccess* matched = nullptr;
  for (const auto& access : function_addressing.accesses) {
    if (access.result_value_name != value_name_id) {
      continue;
    }
    if (matched != nullptr) {
      return nullptr;
    }
    matched = &access;
  }
  return matched;
}

[[nodiscard]] inline const PreparedMemoryAccess*
find_prepared_memory_access_by_result_value_name(
    const PreparedAddressingFunction& function_addressing,
    ValueNameId result_value_name) {
  if (result_value_name == kInvalidValueName) {
    return nullptr;
  }

  const PreparedMemoryAccess* matched = nullptr;
  for (const auto& access : function_addressing.accesses) {
    if (access.result_value_name != result_value_name) {
      continue;
    }
    if (matched != nullptr) {
      return nullptr;
    }
    matched = &access;
  }
  return matched;
}

[[nodiscard]] inline const PreparedMemoryAccess*
find_prepared_memory_access_before_by_result_value_name(
    const PreparedAddressingFunction& function_addressing,
    BlockLabelId block_label,
    ValueNameId result_value_name,
    std::size_t before_instruction_index) {
  if (block_label == kInvalidBlockLabel ||
      result_value_name == kInvalidValueName) {
    return nullptr;
  }

  const PreparedMemoryAccess* matched = nullptr;
  for (const auto& access : function_addressing.accesses) {
    if (access.block_label != block_label ||
        access.inst_index >= before_instruction_index ||
        access.result_value_name != result_value_name) {
      continue;
    }
    if (matched != nullptr) {
      return nullptr;
    }
    matched = &access;
  }
  return matched;
}

[[nodiscard]] inline const PreparedAddressMaterialization*
find_prepared_address_materialization(
    const PreparedAddressingFunction& function_addressing,
    BlockLabelId block_label,
    std::size_t inst_index) {
  for (const auto& materialization : function_addressing.address_materializations) {
    if (materialization.block_label == block_label &&
        materialization.inst_index == inst_index) {
      return &materialization;
    }
  }
  return nullptr;
}

struct PreparedAddressMaterializationLookups {
  std::unordered_map<BlockLabelId, std::vector<const PreparedAddressMaterialization*>>
      materializations_by_block;
};

struct PreparedMemoryAccessLookups {
  std::unordered_map<std::size_t, const PreparedMemoryAccess*> accesses_by_position;
  std::unordered_map<ValueNameId, std::vector<const PreparedMemoryAccess*>>
      accesses_by_result_value_name;
  std::unordered_map<PreparedValueId, std::vector<const PreparedMemoryAccess*>>
      accesses_by_result_value_id;
};

[[nodiscard]] std::size_t prepared_memory_access_position_key(
    BlockLabelId block_label,
    std::size_t instruction_index);

[[nodiscard]] PreparedAddressMaterializationLookups
make_prepared_address_materialization_lookups(const PreparedBirModule& prepared,
                                              FunctionNameId function_name);

[[nodiscard]] PreparedMemoryAccessLookups make_prepared_memory_access_lookups(
    const PreparedAddressingFunction* addressing,
    const PreparedValueHomeLookups* value_home_lookups = nullptr);

[[nodiscard]] const std::vector<const PreparedAddressMaterialization*>*
find_indexed_prepared_address_materializations(
    const PreparedAddressMaterializationLookups* lookups,
    BlockLabelId block_label);

[[nodiscard]] const std::vector<const PreparedMemoryAccess*>*
find_indexed_prepared_memory_accesses_by_result_value_name(
    const PreparedMemoryAccessLookups* lookups,
    ValueNameId result_value_name);

[[nodiscard]] const PreparedMemoryAccess* find_indexed_prepared_memory_access(
    const PreparedMemoryAccessLookups* lookups,
    BlockLabelId block_label,
    std::size_t instruction_index);

[[nodiscard]] const PreparedMemoryAccess*
find_unique_indexed_prepared_memory_access_by_result_value_name(
    const PreparedMemoryAccessLookups* lookups,
    ValueNameId result_value_name);

[[nodiscard]] const std::vector<const PreparedMemoryAccess*>*
find_indexed_prepared_memory_accesses_by_result_value_id(
    const PreparedMemoryAccessLookups* lookups,
    PreparedValueId result_value_id);

[[nodiscard]] const PreparedMemoryAccess*
find_unique_indexed_prepared_memory_access_by_result_value_id(
    const PreparedMemoryAccessLookups* lookups,
    PreparedValueId result_value_id);

[[nodiscard]] std::vector<const PreparedAddressMaterialization*>
collect_prepared_address_materializations_for_block(
    const PreparedAddressingFunction& addressing,
    BlockLabelId block_label);

[[nodiscard]] std::optional<PreparedSameBlockGlobalLoadAccess>
find_prepared_global_load_access(
    const PreparedNameTables& names,
    const PreparedAddressingFunction* addressing,
    BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::LoadGlobalInst& load_global);

[[nodiscard]] std::optional<PreparedSameBlockGlobalLoadAccess>
find_prepared_same_block_global_load_access(
    const PreparedNameTables& names,
    const PreparedAddressingFunction* addressing,
    const PreparedSameBlockScalarProducer& producer);

[[nodiscard]] std::optional<PreparedSameBlockLoadLocalStoredValueSource>
find_prepared_same_block_load_local_stored_value_source(
    const PreparedNameTables& names,
    const PreparedStackLayout& stack_layout,
    const PreparedAddressingFunction* addressing,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index);

}  // namespace c4c::backend::prepare
