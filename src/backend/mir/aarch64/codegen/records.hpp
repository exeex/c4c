#pragma once

#include "../abi/abi.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <variant>

namespace c4c::backend::aarch64::codegen {

enum class OperandKind {
  Register,
  Immediate,
  PreparedValue,
  FrameSlot,
  Symbol,
  BranchTarget,
  Memory,
};

enum class RecordSurfaceKind {
  RecordOnly,
};

enum class RegisterOperandRole {
  Physical,
  PreparedAssignment,
  ValueHome,
  SpillAuthority,
  StoragePlan,
  CallAbi,
};

enum class ImmediateKind {
  SignedInteger,
  UnsignedInteger,
  Boolean,
  NullPointer,
};

enum class MemoryBaseKind {
  None,
  FrameSlot,
  Symbol,
  PointerValue,
  StringConstant,
  Register,
};

struct RegisterOperand {
  c4c::backend::aarch64::abi::RegisterReference reg{};
  RegisterOperandRole role = RegisterOperandRole::Physical;
  std::optional<c4c::backend::prepare::PreparedValueId> value_id;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  c4c::backend::prepare::PreparedRegisterClass prepared_class =
      c4c::backend::prepare::PreparedRegisterClass::None;
  c4c::backend::prepare::PreparedRegisterBank prepared_bank =
      c4c::backend::prepare::PreparedRegisterBank::None;
  std::optional<c4c::backend::aarch64::abi::RegisterView> expected_view;
  std::size_t contiguous_width = 1;
};

struct ImmediateOperand {
  ImmediateKind kind = ImmediateKind::SignedInteger;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  std::int64_t signed_value = 0;
  std::uint64_t unsigned_value = 0;
  std::optional<c4c::backend::prepare::PreparedValueId> source_value_id;
  c4c::ValueNameId source_value_name = c4c::kInvalidValueName;
};

struct PreparedValueOperand {
  c4c::backend::prepare::PreparedValueId value_id = 0;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  c4c::backend::prepare::PreparedValueKind value_kind =
      c4c::backend::prepare::PreparedValueKind::Temporary;
  c4c::backend::prepare::PreparedValueHomeKind home_kind =
      c4c::backend::prepare::PreparedValueHomeKind::None;
  c4c::backend::prepare::PreparedStorageEncodingKind storage_encoding =
      c4c::backend::prepare::PreparedStorageEncodingKind::None;
  c4c::backend::prepare::PreparedRegisterClass register_class =
      c4c::backend::prepare::PreparedRegisterClass::None;
  c4c::backend::prepare::PreparedRegisterBank storage_bank =
      c4c::backend::prepare::PreparedRegisterBank::None;
  std::size_t register_group_width = 1;
};

struct FrameSlotOperand {
  c4c::backend::prepare::PreparedFrameSlotId slot_id = 0;
  std::optional<c4c::backend::prepare::PreparedObjectId> object_id;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  std::optional<c4c::SlotNameId> slot_name;
  std::optional<c4c::ValueNameId> value_name;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  std::size_t offset_bytes = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  bool fixed_location = false;
};

struct SymbolOperand {
  c4c::LinkNameId link_name = c4c::kInvalidLinkName;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  std::int64_t byte_offset = 0;
  bool is_extern = false;
};

struct BranchTargetOperand {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  std::optional<c4c::backend::prepare::PreparedValueId> condition_value_id;
};

struct MemoryOperand {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  MemoryBaseKind base_kind = MemoryBaseKind::None;
  std::optional<RegisterOperand> base_register;
  std::optional<c4c::backend::prepare::PreparedFrameSlotId> frame_slot_id;
  std::optional<c4c::LinkNameId> symbol_name;
  std::optional<c4c::ValueNameId> pointer_value_name;
  std::optional<c4c::backend::prepare::PreparedValueId> pointer_value_id;
  std::optional<c4c::TextId> string_name;
  std::int64_t byte_offset = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  c4c::backend::bir::AddressSpace address_space = c4c::backend::bir::AddressSpace::Default;
  bool is_volatile = false;
  bool can_use_base_plus_offset = false;
};

using OperandPayload = std::variant<RegisterOperand,
                                    ImmediateOperand,
                                    PreparedValueOperand,
                                    FrameSlotOperand,
                                    SymbolOperand,
                                    BranchTargetOperand,
                                    MemoryOperand>;

struct OperandRecord {
  OperandKind kind = OperandKind::Register;
  OperandPayload payload = RegisterOperand{};
};

[[nodiscard]] std::string_view operand_kind_name(OperandKind kind);
[[nodiscard]] std::string_view record_surface_kind_name(RecordSurfaceKind kind);
[[nodiscard]] std::string_view register_operand_role_name(RegisterOperandRole role);
[[nodiscard]] std::string_view immediate_kind_name(ImmediateKind kind);
[[nodiscard]] std::string_view memory_base_kind_name(MemoryBaseKind kind);
[[nodiscard]] OperandRecord make_register_operand(RegisterOperand operand);
[[nodiscard]] OperandRecord make_immediate_operand(ImmediateOperand operand);
[[nodiscard]] OperandRecord make_prepared_value_operand(PreparedValueOperand operand);
[[nodiscard]] OperandRecord make_frame_slot_operand(FrameSlotOperand operand);
[[nodiscard]] OperandRecord make_symbol_operand(SymbolOperand operand);
[[nodiscard]] OperandRecord make_branch_target_operand(BranchTargetOperand operand);
[[nodiscard]] OperandRecord make_memory_operand(MemoryOperand operand);

}  // namespace c4c::backend::aarch64::codegen
