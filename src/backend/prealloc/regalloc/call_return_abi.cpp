#include "call_return_abi.hpp"

#include "../module.hpp"
#include "../target_register_profile.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <system_error>
#include <utility>

namespace c4c::backend::prepare::regalloc_detail {

namespace {

[[nodiscard]] std::optional<std::pair<std::string, std::size_t>> split_trailing_register_index(
    std::string_view register_name) {
  if (register_name.empty()) {
    return std::nullopt;
  }
  std::size_t digit_start = register_name.size();
  while (digit_start > 0 && register_name[digit_start - 1] >= '0' &&
         register_name[digit_start - 1] <= '9') {
    --digit_start;
  }
  if (digit_start == register_name.size()) {
    return std::nullopt;
  }

  std::size_t parsed_index = 0;
  const char* const begin = register_name.data() + digit_start;
  const char* const end = register_name.data() + register_name.size();
  const auto result = std::from_chars(begin, end, parsed_index);
  if (result.ec != std::errc{} || result.ptr != end) {
    return std::nullopt;
  }
  return std::pair<std::string, std::size_t>{std::string(register_name.substr(0, digit_start)),
                                              parsed_index};
}

template <std::size_t N>
[[nodiscard]] std::vector<std::string> abi_register_names_from_sequence(
    const std::array<std::string_view, N>& sequence,
    std::size_t start_index,
    std::size_t contiguous_width) {
  if (start_index + contiguous_width > sequence.size()) {
    return {};
  }

  std::vector<std::string> names;
  names.reserve(contiguous_width);
  for (std::size_t index = 0; index < contiguous_width; ++index) {
    names.emplace_back(sequence[start_index + index]);
  }
  return names;
}

[[nodiscard]] std::vector<std::string> contiguous_numeric_register_names(
    std::string_view register_name,
    std::size_t contiguous_width) {
  if (const auto split = split_trailing_register_index(register_name); split.has_value()) {
    std::vector<std::string> names;
    names.reserve(contiguous_width);
    for (std::size_t index = 0; index < contiguous_width; ++index) {
      names.push_back(split->first + std::to_string(split->second + index));
    }
    return names;
  }
  return {};
}

[[nodiscard]] std::size_t align_up(std::size_t value, std::size_t alignment) {
  if (alignment == 0) {
    return value;
  }
  const std::size_t remainder = value % alignment;
  return remainder == 0 ? value : value + (alignment - remainder);
}

[[nodiscard]] std::size_t scalar_type_size_bytes(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return 1;
    case bir::TypeKind::I32:
    case bir::TypeKind::F32:
      return 4;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F64:
      return 8;
    case bir::TypeKind::I128:
    case bir::TypeKind::F128:
      return 16;
    case bir::TypeKind::Void:
      return 0;
  }
  return 0;
}

[[nodiscard]] std::size_t call_stack_argument_size_bytes(const bir::CallArgAbiInfo& abi) {
  if (abi.aarch64_hfa_lane_count > 0) {
    return std::max<std::size_t>(scalar_type_size_bytes(abi.type), 1);
  }
  return align_up(std::max<std::size_t>(abi.size_bytes, 8), 8);
}

[[nodiscard]] std::size_t call_stack_argument_alignment_bytes(const bir::CallArgAbiInfo& abi) {
  if (abi.aarch64_hfa_lane_count > 0) {
    if (abi.aarch64_hfa_lane_index > 0) {
      return 1;
    }
    return std::min<std::size_t>(
        std::max<std::size_t>(scalar_type_size_bytes(abi.type), 8), 16);
  }
  const std::size_t abi_alignment = abi.align_bytes == 0 ? abi.size_bytes : abi.align_bytes;
  return std::min<std::size_t>(std::max<std::size_t>(abi_alignment, 8), 16);
}

[[nodiscard]] bool call_arg_requires_stack_destination(const c4c::TargetProfile& target_profile,
                                                       const bir::CallInst& call,
                                                       std::size_t arg_index) {
  const auto abi = resolve_call_arg_abi(call, arg_index);
  if (!abi.has_value()) {
    return false;
  }
  if (call_arg_storage_kind(target_profile, call, arg_index) == PreparedMoveStorageKind::StackSlot) {
    return true;
  }
  return target_profile.arch == c4c::TargetArch::X86_64 &&
         abi->type == bir::TypeKind::Ptr &&
         abi->byval_copy &&
         !abi->sret_pointer &&
         [&]() {
           const auto abi_register_index =
               call_arg_abi_register_index(target_profile, call, arg_index);
           return abi_register_index.has_value() &&
                  call_arg_destination_register_name(target_profile, *abi, *abi_register_index)
                      .has_value();
         }();
}

[[nodiscard]] bool aarch64_register_passed_byval_aggregate(
    const c4c::TargetProfile& target_profile,
    const bir::CallArgAbiInfo& abi,
    std::optional<std::size_t> abi_register_index) {
  if (target_profile.arch != c4c::TargetArch::Aarch64 ||
      abi.type != bir::TypeKind::Ptr ||
      !abi.byval_copy ||
      abi.sret_pointer ||
      !abi.passed_in_register ||
      abi.passed_on_stack ||
      abi.primary_class != bir::AbiValueClass::Integer ||
      abi.size_bytes == 0 ||
      abi.size_bytes > 16 ||
      !abi_register_index.has_value()) {
    return false;
  }
  const auto register_name =
      call_arg_destination_register_name(target_profile, abi, *abi_register_index);
  if (!register_name.has_value()) {
    return false;
  }
  const std::size_t register_width = std::max<std::size_t>((abi.size_bytes + 7) / 8, 1);
  return !call_arg_destination_register_names(target_profile,
                                             PreparedRegisterClass::General,
                                             *abi_register_index,
                                             *register_name,
                                             register_width)
              .empty();
}

[[nodiscard]] bool aarch64_indirect_byval_aggregate_pointer(
    const c4c::TargetProfile& target_profile,
    const bir::CallArgAbiInfo& abi,
    std::optional<std::size_t> abi_register_index) {
  return target_profile.arch == c4c::TargetArch::Aarch64 &&
         abi.type == bir::TypeKind::Ptr &&
         abi.byval_copy &&
         !abi.sret_pointer &&
         abi.passed_in_register &&
         !abi.passed_on_stack &&
         abi.primary_class == bir::AbiValueClass::Integer &&
         abi.size_bytes > 16 &&
         abi_register_index.has_value() &&
         call_arg_destination_register_name(target_profile, abi, *abi_register_index).has_value();
}

[[nodiscard]] bool aarch64_same_call_arg_register_bank(const bir::CallArgAbiInfo& lhs,
                                                       const bir::CallArgAbiInfo& rhs) {
  const auto is_float_bank = [](const bir::CallArgAbiInfo& abi) {
    return abi.primary_class == bir::AbiValueClass::Sse ||
           abi.primary_class == bir::AbiValueClass::X87;
  };
  if (is_float_bank(lhs) || is_float_bank(rhs)) {
    return is_float_bank(lhs) == is_float_bank(rhs);
  }
  return lhs.primary_class == bir::AbiValueClass::Integer &&
         rhs.primary_class == bir::AbiValueClass::Integer;
}

}  // namespace

std::vector<std::string> call_arg_destination_register_names(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class,
    std::size_t arg_index,
    std::string_view register_name,
    std::size_t contiguous_width) {
  if (contiguous_width <= 1) {
    return std::vector<std::string>{std::string(register_name)};
  }

  switch (target_profile.arch) {
    case c4c::TargetArch::X86_64:
      if (reg_class == PreparedRegisterClass::General) {
        constexpr std::array<std::string_view, 6> kX86GeneralArgRegisters = {
            "rdi", "rsi", "rdx", "rcx", "r8", "r9"};
        if (const auto names =
                abi_register_names_from_sequence(kX86GeneralArgRegisters, arg_index, contiguous_width);
            !names.empty()) {
          return names;
        }
      }
      break;
    case c4c::TargetArch::I686:
      return {};
    case c4c::TargetArch::Aarch64:
      if (reg_class == PreparedRegisterClass::General) {
        constexpr std::array<std::string_view, 8> kAarch64GeneralArgRegisters = {
            "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7"};
        if (const auto names =
                abi_register_names_from_sequence(kAarch64GeneralArgRegisters,
                                                 arg_index,
                                                 contiguous_width);
            !names.empty()) {
          return names;
        }
        return {};
      }
      break;
    case c4c::TargetArch::Riscv64:
    case c4c::TargetArch::Unknown:
      break;
  }

  return contiguous_numeric_register_names(register_name, contiguous_width);
}

std::optional<PreparedRegisterPlacement> f128_call_arg_destination_placement(
    std::optional<PreparedRegisterPlacement> placement,
    bir::TypeKind arg_type) {
  if (arg_type == bir::TypeKind::F128 && placement.has_value()) {
    placement->bank = PreparedRegisterBank::Vreg;
  }
  return placement;
}

std::vector<std::string> call_result_destination_register_names(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class,
    std::string_view register_name,
    std::size_t contiguous_width) {
  if (contiguous_width <= 1) {
    return std::vector<std::string>{std::string(register_name)};
  }

  switch (target_profile.arch) {
    case c4c::TargetArch::X86_64:
      if (reg_class == PreparedRegisterClass::General) {
        constexpr std::array<std::string_view, 2> kX86GeneralReturnRegisters = {"rax", "rdx"};
        for (std::size_t index = 0; index < kX86GeneralReturnRegisters.size(); ++index) {
          if (kX86GeneralReturnRegisters[index] != register_name) {
            continue;
          }
          if (const auto names = abi_register_names_from_sequence(
                  kX86GeneralReturnRegisters, index, contiguous_width);
              !names.empty()) {
            return names;
          }
          break;
        }
      }
      break;
    case c4c::TargetArch::I686:
      return {};
    case c4c::TargetArch::Aarch64:
    case c4c::TargetArch::Riscv64:
    case c4c::TargetArch::Unknown:
      break;
  }

  return contiguous_numeric_register_names(register_name, contiguous_width);
}

std::optional<bir::CallArgAbiInfo> resolve_call_arg_abi(
    const bir::CallInst& call,
    std::size_t arg_index) {
  if (arg_index < call.arg_abi.size()) {
    return call.arg_abi[arg_index];
  }
  return std::nullopt;
}

[[nodiscard]] bool scalar_i16_call_arg_abi_needs_repair(
    const bir::CallArgAbiInfo& abi) {
  return abi.type == bir::TypeKind::I16 &&
         (abi.primary_class != bir::AbiValueClass::Integer ||
          abi.passed_on_stack ||
          abi.byval_copy ||
          abi.sret_pointer ||
          !abi.passed_in_register);
}

[[nodiscard]] bir::CallArgAbiInfo repair_scalar_i16_call_arg_abi(
    const bir::CallArgAbiInfo& abi) {
  bir::CallArgAbiInfo repaired = abi;
  repaired.type = bir::TypeKind::I16;
  repaired.size_bytes = 2;
  repaired.align_bytes = 2;
  repaired.primary_class = bir::AbiValueClass::Integer;
  repaired.secondary_class = bir::AbiValueClass::None;
  repaired.passed_in_register = true;
  repaired.passed_on_stack = false;
  repaired.byval_copy = false;
  repaired.sret_pointer = false;
  repaired.aarch64_hfa_lane_count = 0;
  repaired.aarch64_hfa_lane_index = 0;
  return repaired;
}

std::optional<bir::CallArgAbiInfo> resolve_call_arg_abi(
    const c4c::TargetProfile& target_profile,
    const bir::CallInst& call,
    std::size_t arg_index) {
  (void)target_profile;
  auto abi = resolve_call_arg_abi(call, arg_index);
  if (abi.has_value() && scalar_i16_call_arg_abi_needs_repair(*abi)) {
    return repair_scalar_i16_call_arg_abi(*abi);
  }
  return abi;
}

std::optional<std::size_t> call_arg_abi_register_index(
    const c4c::TargetProfile& target_profile,
    const bir::CallInst& call,
    std::size_t arg_index) {
  const auto abi = resolve_call_arg_abi(target_profile, call, arg_index);
  if (!abi.has_value() || !abi->passed_in_register) {
    if (target_profile.arch == c4c::TargetArch::X86_64 &&
        abi.has_value() &&
        abi->type == bir::TypeKind::Ptr &&
        abi->byval_copy &&
        !abi->sret_pointer) {
      return arg_index;
    }
    if (target_profile.arch == c4c::TargetArch::Aarch64 &&
        abi.has_value() &&
        abi->type == bir::TypeKind::Ptr &&
        abi->sret_pointer) {
      return 0;
    }
    return std::nullopt;
  }
  if (target_profile.arch != c4c::TargetArch::Aarch64) {
    return arg_index;
  }
  if (abi->type == bir::TypeKind::Ptr && abi->sret_pointer) {
    return 0;
  }

  std::size_t register_index = 0;
  for (std::size_t candidate_index = 0; candidate_index < arg_index; ++candidate_index) {
    const auto candidate_abi = resolve_call_arg_abi(target_profile, call, candidate_index);
    if (!candidate_abi.has_value() || !candidate_abi->passed_in_register) {
      continue;
    }
    if (candidate_abi->type == bir::TypeKind::Ptr && candidate_abi->sret_pointer) {
      continue;
    }
    if (aarch64_same_call_arg_register_bank(*candidate_abi, *abi)) {
      if (candidate_abi->type == bir::TypeKind::Ptr &&
          candidate_abi->byval_copy &&
          !candidate_abi->sret_pointer &&
          candidate_abi->primary_class == bir::AbiValueClass::Integer &&
          candidate_abi->size_bytes > 0 &&
          candidate_abi->size_bytes <= 16) {
        register_index += std::max<std::size_t>((candidate_abi->size_bytes + 7) / 8, 1);
      } else {
        ++register_index;
      }
    }
  }
  return register_index;
}

PreparedMoveStorageKind call_arg_storage_kind(const c4c::TargetProfile& target_profile,
                                              const bir::CallInst& call,
                                              std::size_t arg_index) {
  const auto abi = resolve_call_arg_abi(target_profile, call, arg_index);
  if (!abi.has_value()) {
    return PreparedMoveStorageKind::None;
  }

    if (target_profile.arch == c4c::TargetArch::X86_64 &&
        abi->type == bir::TypeKind::Ptr &&
        abi->byval_copy &&
        !abi->sret_pointer &&
        [&]() {
          const auto abi_register_index =
              call_arg_abi_register_index(target_profile, call, arg_index);
          return abi_register_index.has_value() &&
                 call_arg_destination_register_name(target_profile, *abi, *abi_register_index)
                     .has_value();
        }()) {
      return PreparedMoveStorageKind::Register;
    }
    const auto aarch64_register_index =
        call_arg_abi_register_index(target_profile, call, arg_index);
    if (target_profile.arch == c4c::TargetArch::Aarch64 &&
        abi->type == bir::TypeKind::Ptr &&
        abi->sret_pointer &&
        aarch64_register_index.has_value() &&
        call_arg_destination_register_name(target_profile, *abi, *aarch64_register_index)
            .has_value()) {
      return PreparedMoveStorageKind::Register;
    }
    if (aarch64_register_passed_byval_aggregate(
            target_profile, *abi, aarch64_register_index)) {
      return PreparedMoveStorageKind::Register;
    }
    if (aarch64_indirect_byval_aggregate_pointer(
            target_profile, *abi, aarch64_register_index)) {
      return PreparedMoveStorageKind::Register;
    }

    if (abi->passed_on_stack || abi->byval_copy || abi->sret_pointer ||
        abi->primary_class == bir::AbiValueClass::Memory) {
    return PreparedMoveStorageKind::StackSlot;
  }
  const auto destination_register_index =
      call_arg_abi_register_index(target_profile, call, arg_index);
  if (destination_register_index.has_value() &&
      call_arg_destination_register_name(target_profile, *abi, *destination_register_index)
          .has_value()) {
    return PreparedMoveStorageKind::Register;
  }
  if (abi->type != bir::TypeKind::Void) {
    return PreparedMoveStorageKind::StackSlot;
  }
  return PreparedMoveStorageKind::None;
}

std::optional<std::size_t> call_arg_destination_stack_offset_bytes(
    const c4c::TargetProfile& target_profile,
    const bir::CallInst& call,
    std::size_t arg_index) {
  if ((target_profile.arch != c4c::TargetArch::X86_64 &&
       target_profile.arch != c4c::TargetArch::Aarch64) ||
      !call_arg_requires_stack_destination(target_profile, call, arg_index)) {
    return std::nullopt;
  }

  std::size_t next_offset_bytes = 0;
  for (std::size_t candidate_index = 0; candidate_index < call.args.size(); ++candidate_index) {
    if (!call_arg_requires_stack_destination(target_profile, call, candidate_index)) {
      continue;
    }
    const auto abi = resolve_call_arg_abi(call, candidate_index);
    if (!abi.has_value()) {
      return std::nullopt;
    }
    next_offset_bytes =
        align_up(next_offset_bytes, call_stack_argument_alignment_bytes(*abi));
    if (candidate_index == arg_index) {
      return next_offset_bytes;
    }
    next_offset_bytes += call_stack_argument_size_bytes(*abi);
  }
  return std::nullopt;
}

PreparedMoveStorageKind call_result_storage_kind(const bir::CallInst& call) {
  if (!call.result.has_value() || !call.result_abi.has_value() || call.result->kind != bir::Value::Kind::Named) {
    return PreparedMoveStorageKind::None;
  }

  const auto& abi = *call.result_abi;
  if (abi.returned_in_memory || abi.primary_class == bir::AbiValueClass::Memory) {
    return PreparedMoveStorageKind::StackSlot;
  }
  if (abi.type != bir::TypeKind::Void) {
    return PreparedMoveStorageKind::Register;
  }
  return PreparedMoveStorageKind::None;
}

std::optional<bir::CallResultAbiInfo> direct_bir_function_return_move_repair(
    const bir::Function& function) {
  if (function.return_type == bir::TypeKind::Void) {
    return std::nullopt;
  }

  bir::CallResultAbiInfo abi{
      .type = function.return_type,
      .primary_class = bir::AbiValueClass::None,
      .secondary_class = bir::AbiValueClass::None,
      .returned_in_memory = false,
  };
  switch (function.return_type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      abi.primary_class = bir::AbiValueClass::Integer;
      return abi;
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
      abi.primary_class = bir::AbiValueClass::Sse;
      return abi;
    case bir::TypeKind::I128:
      abi.primary_class = bir::AbiValueClass::Memory;
      abi.returned_in_memory = true;
      return abi;
    case bir::TypeKind::Void:
      return std::nullopt;
  }
  return std::nullopt;
}

PreparedMoveStorageKind function_return_storage_kind(const bir::Function& function) {
  const auto return_abi = function.return_abi;
  if (!return_abi.has_value()) {
    return PreparedMoveStorageKind::None;
  }
  const auto& abi = *return_abi;
  if (abi.returned_in_memory || abi.primary_class == bir::AbiValueClass::Memory) {
    return PreparedMoveStorageKind::StackSlot;
  }
  if (abi.type != bir::TypeKind::Void) {
    return PreparedMoveStorageKind::Register;
  }
  return PreparedMoveStorageKind::None;
}

}  // namespace c4c::backend::prepare::regalloc_detail
