#include "lowering.hpp"

namespace c4c::backend {

namespace {

bool use_float_return_registers(const c4c::TargetProfile& target_profile,
                                bir::TypeKind type) {
  if (!target_profile.has_float_return_registers) {
    return false;
  }
  return type == bir::TypeKind::F32 || type == bir::TypeKind::F64 ||
         type == bir::TypeKind::F128;
}

bool use_float_arg_registers(const c4c::TargetProfile& target_profile,
                             bir::TypeKind type) {
  if (!target_profile.has_float_arg_registers) {
    return false;
  }
  return type == bir::TypeKind::F32 || type == bir::TypeKind::F64 ||
         type == bir::TypeKind::F128;
}

std::optional<bir::CallResultAbiInfo> lower_function_return_abi(
    const c4c::TargetProfile& target_profile,
    bir::TypeKind type,
                                                                bool returned_via_sret) {
  if (returned_via_sret) {
    return bir::CallResultAbiInfo{
        .type = bir::TypeKind::Void,
        .primary_class = bir::AbiValueClass::Memory,
        .returned_in_memory = true,
    };
  }

  bir::CallResultAbiInfo abi{
      .type = type,
  };
  switch (type) {
    case bir::TypeKind::Void:
      return std::nullopt;
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      abi.primary_class = use_float_return_registers(target_profile, type)
                              ? bir::AbiValueClass::Sse
                              : bir::AbiValueClass::Integer;
      return abi;
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      abi.primary_class = bir::AbiValueClass::Integer;
      return abi;
    case bir::TypeKind::I128:
      abi.primary_class = bir::AbiValueClass::Memory;
      abi.returned_in_memory = true;
      return abi;
  }
  return std::nullopt;
}

std::optional<bir::CallArgAbiInfo> lower_call_arg_abi(
    const c4c::TargetProfile& target_profile,
    bir::TypeKind type) {
  if (type == bir::TypeKind::Void) {
    return std::nullopt;
  }

  bir::CallArgAbiInfo abi{
      .type = type,
      .size_bytes = lir_to_bir_detail::type_size_bytes(type),
      .align_bytes = lir_to_bir_detail::type_size_bytes(type),
      .passed_in_register = true,
  };
  switch (type) {
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
      abi.primary_class = use_float_arg_registers(target_profile, type)
                              ? bir::AbiValueClass::Sse
                              : bir::AbiValueClass::Integer;
      return abi;
    case bir::TypeKind::F128:
      if (target_profile.arch == c4c::TargetArch::X86_64) {
        abi.primary_class = bir::AbiValueClass::Memory;
        abi.passed_in_register = false;
        abi.passed_on_stack = true;
        return abi;
      }
      abi.primary_class = use_float_arg_registers(target_profile, type)
                              ? bir::AbiValueClass::Sse
                              : bir::AbiValueClass::Integer;
      return abi;
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      abi.primary_class = bir::AbiValueClass::Integer;
      return abi;
    case bir::TypeKind::I128:
      abi.primary_class = bir::AbiValueClass::Memory;
      abi.passed_in_register = false;
      abi.passed_on_stack = true;
      return abi;
    case bir::TypeKind::Void:
      return std::nullopt;
  }
  return std::nullopt;
}

}  // namespace

std::optional<bir::CallArgAbiInfo> lir_to_bir_detail::compute_call_arg_abi(
    const c4c::TargetProfile& target_profile,
    bir::TypeKind type) {
  return lower_call_arg_abi(target_profile, type);
}

std::optional<bir::CallResultAbiInfo> lir_to_bir_detail::compute_function_return_abi(
    const c4c::TargetProfile& target_profile,
    bir::TypeKind type,
    bool returned_via_sret) {
  return lower_function_return_abi(target_profile, type, returned_via_sret);
}

}  // namespace c4c::backend
