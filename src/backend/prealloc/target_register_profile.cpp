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

}  // namespace

std::optional<std::string> call_arg_destination_register_name(
    const c4c::TargetProfile& target_profile,
    const bir::CallArgAbiInfo& abi,
    std::size_t arg_index) {
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

std::vector<std::string_view> caller_saved_registers(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class) {
  if (reg_class != PreparedRegisterClass::General) {
    return {};
  }
  switch (target_profile.arch) {
    case c4c::TargetArch::X86_64:
      return {"r11"};
    case c4c::TargetArch::I686:
      return {"ecx"};
    case c4c::TargetArch::Aarch64:
      return {"x13"};
    case c4c::TargetArch::Riscv64:
      return {"t0"};
    case c4c::TargetArch::Unknown:
      return {};
  }
  return {};
}

std::vector<std::string_view> callee_saved_registers(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class) {
  if (reg_class != PreparedRegisterClass::General) {
    return {};
  }
  switch (target_profile.arch) {
    case c4c::TargetArch::X86_64:
      return {"rbx", "r12"};
    case c4c::TargetArch::I686:
      return {"ebx", "esi"};
    case c4c::TargetArch::Aarch64:
      return {"x20", "x21"};
    case c4c::TargetArch::Riscv64:
      return {"s1", "s2"};
    case c4c::TargetArch::Unknown:
      return {};
  }
  return {};
}

}  // namespace c4c::backend::prepare
