#include "target_register_profile.hpp"

#include <array>
#include <string>

namespace c4c::backend::prepare {

namespace {

constexpr std::array<const char*, 6> kX86GeneralAbiRegisters = {
    "rdi", "rsi", "rdx", "rcx", "r8", "r9",
};
constexpr std::array<const char*, 8> kX86FloatAbiRegisters = {
    "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
};
constexpr std::array<const char*, 8> kAarch64GeneralAbiRegisters = {
    "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7",
};
constexpr std::array<const char*, 8> kRiscvGeneralAbiRegisters = {
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
};
constexpr std::array<const char*, 8> kRiscvFloatAbiRegisters = {
    "fa0", "fa1", "fa2", "fa3", "fa4", "fa5", "fa6", "fa7",
};

template <std::size_t N>
[[nodiscard]] std::optional<std::string> indexed_register_name(
    const std::array<const char*, N>& register_names,
    std::size_t index) {
  if (index >= register_names.size() || register_names[index] == nullptr ||
      register_names[index][0] == '\0') {
    return std::nullopt;
  }
  return std::string(register_names[index]);
}

[[nodiscard]] bool is_float_type(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return true;
    default:
      return false;
  }
}

[[nodiscard]] bool is_float_abi_class(bir::AbiValueClass abi_class) {
  switch (abi_class) {
    case bir::AbiValueClass::Sse:
    case bir::AbiValueClass::X87:
      return true;
    default:
      return false;
  }
}

[[nodiscard]] std::optional<std::string> aarch64_float_register_name(
    bir::TypeKind type,
    std::size_t index) {
  if (index >= 8) {
    return std::nullopt;
  }
  if (type == bir::TypeKind::F32) {
    return "s" + std::to_string(index);
  }
  if (type == bir::TypeKind::F64) {
    return "d" + std::to_string(index);
  }
  return std::nullopt;
}

[[nodiscard]] std::vector<std::string_view> x86_caller_saved_pool(PreparedRegisterClass reg_class) {
  switch (reg_class) {
    case PreparedRegisterClass::General:
      return {"r11"};
    case PreparedRegisterClass::Float:
      return {"xmm8"};
    case PreparedRegisterClass::Vector:
      return {"ymm8"};
    case PreparedRegisterClass::AggregateAddress:
    case PreparedRegisterClass::None:
      return {};
  }
  return {};
}

[[nodiscard]] std::vector<std::string_view> x86_callee_saved_pool(PreparedRegisterClass reg_class) {
  switch (reg_class) {
    case PreparedRegisterClass::General:
      return {"rbx", "r12"};
    case PreparedRegisterClass::Float:
      return {"xmm12", "xmm13"};
    case PreparedRegisterClass::Vector:
      return {"ymm12", "ymm13"};
    case PreparedRegisterClass::AggregateAddress:
    case PreparedRegisterClass::None:
      return {};
  }
  return {};
}

[[nodiscard]] std::vector<std::string_view> i686_caller_saved_pool(PreparedRegisterClass reg_class) {
  switch (reg_class) {
    case PreparedRegisterClass::General:
      return {"ecx"};
    case PreparedRegisterClass::Float:
      return {"xmm1"};
    case PreparedRegisterClass::Vector:
      return {"ymm1"};
    case PreparedRegisterClass::AggregateAddress:
    case PreparedRegisterClass::None:
      return {};
  }
  return {};
}

[[nodiscard]] std::vector<std::string_view> i686_callee_saved_pool(PreparedRegisterClass reg_class) {
  switch (reg_class) {
    case PreparedRegisterClass::General:
      return {"ebx", "esi"};
    case PreparedRegisterClass::Float:
      return {"xmm6", "xmm7"};
    case PreparedRegisterClass::Vector:
      return {"ymm6", "ymm7"};
    case PreparedRegisterClass::AggregateAddress:
    case PreparedRegisterClass::None:
      return {};
  }
  return {};
}

[[nodiscard]] std::vector<std::string_view> aarch64_caller_saved_pool(PreparedRegisterClass reg_class) {
  switch (reg_class) {
    case PreparedRegisterClass::General:
      return {"x13"};
    case PreparedRegisterClass::Float:
      return {"d13"};
    case PreparedRegisterClass::Vector:
      return {"v13"};
    case PreparedRegisterClass::AggregateAddress:
    case PreparedRegisterClass::None:
      return {};
  }
  return {};
}

[[nodiscard]] std::vector<std::string_view> aarch64_callee_saved_pool(PreparedRegisterClass reg_class) {
  switch (reg_class) {
    case PreparedRegisterClass::General:
      return {"x20", "x21"};
    case PreparedRegisterClass::Float:
      return {"d20", "d21"};
    case PreparedRegisterClass::Vector:
      return {"v20", "v21"};
    case PreparedRegisterClass::AggregateAddress:
    case PreparedRegisterClass::None:
      return {};
  }
  return {};
}

[[nodiscard]] std::vector<std::string_view> riscv_caller_saved_pool(PreparedRegisterClass reg_class) {
  switch (reg_class) {
    case PreparedRegisterClass::General:
      return {"t0"};
    case PreparedRegisterClass::Float:
      return {"ft0"};
    case PreparedRegisterClass::Vector:
      return {"v0"};
    case PreparedRegisterClass::AggregateAddress:
    case PreparedRegisterClass::None:
      return {};
  }
  return {};
}

[[nodiscard]] std::vector<std::string_view> riscv_callee_saved_pool(PreparedRegisterClass reg_class) {
  switch (reg_class) {
    case PreparedRegisterClass::General:
      return {"s1", "s2"};
    case PreparedRegisterClass::Float:
      return {"fs1", "fs2"};
    case PreparedRegisterClass::Vector:
      return {"v8", "v9"};
    case PreparedRegisterClass::AggregateAddress:
    case PreparedRegisterClass::None:
      return {};
  }
  return {};
}

[[nodiscard]] std::vector<std::string> contiguous_vector_units(std::string_view prefix,
                                                               std::size_t start_index,
                                                               std::size_t count) {
  std::vector<std::string> units;
  units.reserve(count);
  for (std::size_t index = 0; index < count; ++index) {
    units.push_back(std::string(prefix) + std::to_string(start_index + index));
  }
  return units;
}

[[nodiscard]] std::vector<PreparedRegisterCandidateSpan> singleton_spans(
    const std::vector<std::string_view>& register_names) {
  std::vector<PreparedRegisterCandidateSpan> spans;
  spans.reserve(register_names.size());
  for (const std::string_view register_name : register_names) {
    spans.push_back(PreparedRegisterCandidateSpan{
        .register_name = std::string(register_name),
        .contiguous_width = 1,
        .occupied_register_names = {std::string(register_name)},
    });
  }
  return spans;
}

[[nodiscard]] std::vector<PreparedRegisterCandidateSpan> contiguous_group_spans(
    const std::vector<std::string>& units,
    std::size_t contiguous_width) {
  if (contiguous_width == 0 || units.size() < contiguous_width) {
    return {};
  }
  std::vector<PreparedRegisterCandidateSpan> spans;
  for (std::size_t base_index = 0; base_index + contiguous_width <= units.size();
       base_index += contiguous_width) {
    PreparedRegisterCandidateSpan span{
        .register_name = units[base_index],
        .contiguous_width = contiguous_width,
        .occupied_register_names = {},
    };
    span.occupied_register_names.reserve(contiguous_width);
    for (std::size_t offset = 0; offset < contiguous_width; ++offset) {
      span.occupied_register_names.push_back(units[base_index + offset]);
    }
    spans.push_back(std::move(span));
  }
  return spans;
}

[[nodiscard]] std::vector<PreparedRegisterCandidateSpan> register_spans_for_pool(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class,
    bool caller_pool,
    std::size_t contiguous_width) {
  if (contiguous_width == 0) {
    return {};
  }
  if (reg_class == PreparedRegisterClass::Vector) {
    switch (target_profile.arch) {
      case c4c::TargetArch::X86_64:
        return contiguous_group_spans(
            caller_pool ? contiguous_vector_units("ymm", 0, 16)
                        : contiguous_vector_units("ymm", 16, 16),
            contiguous_width);
      case c4c::TargetArch::I686:
        return contiguous_group_spans(
            caller_pool ? contiguous_vector_units("ymm", 0, 8)
                        : contiguous_vector_units("ymm", 8, 8),
            contiguous_width);
      case c4c::TargetArch::Aarch64:
        return contiguous_group_spans(
            caller_pool ? contiguous_vector_units("v", 0, 16)
                        : contiguous_vector_units("v", 16, 16),
            contiguous_width);
      case c4c::TargetArch::Riscv64:
        return contiguous_group_spans(
            caller_pool ? contiguous_vector_units("v", 0, 16)
                        : contiguous_vector_units("v", 16, 16),
            contiguous_width);
      case c4c::TargetArch::Unknown:
        return {};
    }
  }

  const auto registers = caller_pool ? caller_saved_registers(target_profile, reg_class)
                                     : callee_saved_registers(target_profile, reg_class);
  if (contiguous_width != 1) {
    return {};
  }
  return singleton_spans(registers);
}

}  // namespace

std::optional<std::string> call_arg_destination_register_name(
    const c4c::TargetProfile& target_profile,
    const bir::CallArgAbiInfo& abi,
    std::size_t arg_index) {
  if (target_profile.arch == c4c::TargetArch::X86_64 &&
      abi.type == bir::TypeKind::Ptr &&
      abi.byval_copy &&
      !abi.sret_pointer) {
    return indexed_register_name(kX86GeneralAbiRegisters, arg_index);
  }
  if (!abi.passed_in_register) {
    return std::nullopt;
  }

  switch (target_profile.arch) {
    case c4c::TargetArch::X86_64:
      if (is_float_abi_class(abi.primary_class)) {
        return indexed_register_name(kX86FloatAbiRegisters, arg_index);
      }
      if (abi.primary_class == bir::AbiValueClass::Integer) {
        return indexed_register_name(kX86GeneralAbiRegisters, arg_index);
      }
      return std::nullopt;
    case c4c::TargetArch::I686:
      return std::nullopt;
    case c4c::TargetArch::Aarch64:
      if (is_float_abi_class(abi.primary_class)) {
        return aarch64_float_register_name(abi.type, arg_index);
      }
      if (abi.primary_class == bir::AbiValueClass::Integer) {
        return indexed_register_name(kAarch64GeneralAbiRegisters, arg_index);
      }
      return std::nullopt;
    case c4c::TargetArch::Riscv64:
      if (target_profile.has_float_arg_registers &&
          (is_float_abi_class(abi.primary_class) || is_float_type(abi.type))) {
        return indexed_register_name(kRiscvFloatAbiRegisters, arg_index);
      }
      if (abi.primary_class == bir::AbiValueClass::Integer) {
        return indexed_register_name(kRiscvGeneralAbiRegisters, arg_index);
      }
      return std::nullopt;
    case c4c::TargetArch::Unknown:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<std::string> call_result_destination_register_name(
    const c4c::TargetProfile& target_profile,
    const bir::CallResultAbiInfo& abi) {
  if (abi.returned_in_memory || abi.primary_class == bir::AbiValueClass::Memory ||
      abi.type == bir::TypeKind::Void) {
    return std::nullopt;
  }

  switch (target_profile.arch) {
    case c4c::TargetArch::X86_64:
      if (is_float_abi_class(abi.primary_class) || is_float_type(abi.type)) {
        return std::string("xmm0");
      }
      return std::string("rax");
    case c4c::TargetArch::I686:
      return is_float_type(abi.type) ? std::nullopt : std::optional<std::string>{"eax"};
    case c4c::TargetArch::Aarch64:
      if (is_float_abi_class(abi.primary_class) || is_float_type(abi.type)) {
        return aarch64_float_register_name(abi.type, 0);
      }
      return std::string("x0");
    case c4c::TargetArch::Riscv64:
      if (target_profile.has_float_return_registers &&
          (is_float_abi_class(abi.primary_class) || is_float_type(abi.type))) {
        return std::string("fa0");
      }
      return std::string("a0");
    case c4c::TargetArch::Unknown:
      return std::nullopt;
  }
  return std::nullopt;
}

std::vector<PreparedRegisterCandidateSpan> caller_saved_register_spans(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class,
    std::size_t contiguous_width) {
  return register_spans_for_pool(target_profile, reg_class, true, contiguous_width);
}

std::vector<PreparedRegisterCandidateSpan> callee_saved_register_spans(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class,
    std::size_t contiguous_width) {
  return register_spans_for_pool(target_profile, reg_class, false, contiguous_width);
}

std::vector<std::string_view> caller_saved_registers(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class) {
  switch (target_profile.arch) {
    case c4c::TargetArch::X86_64:
      return x86_caller_saved_pool(reg_class);
    case c4c::TargetArch::I686:
      return i686_caller_saved_pool(reg_class);
    case c4c::TargetArch::Aarch64:
      return aarch64_caller_saved_pool(reg_class);
    case c4c::TargetArch::Riscv64:
      return riscv_caller_saved_pool(reg_class);
    case c4c::TargetArch::Unknown:
      return {};
  }
  return {};
}

std::vector<std::string_view> callee_saved_registers(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class) {
  switch (target_profile.arch) {
    case c4c::TargetArch::X86_64:
      return x86_callee_saved_pool(reg_class);
    case c4c::TargetArch::I686:
      return i686_callee_saved_pool(reg_class);
    case c4c::TargetArch::Aarch64:
      return aarch64_callee_saved_pool(reg_class);
    case c4c::TargetArch::Riscv64:
      return riscv_callee_saved_pool(reg_class);
    case c4c::TargetArch::Unknown:
      return {};
  }
  return {};
}

}  // namespace c4c::backend::prepare
