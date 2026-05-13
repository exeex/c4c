#pragma once

#include "../../../backend.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace c4c::backend::aarch64::abi {

enum class RegisterBank {
  GeneralPurpose,
  StackPointer,
  FpSimd,
};

enum class RegisterView {
  X,
  W,
  Sp,
  S,
  D,
  Q,
  V,
};

enum class AllocationRegisterPool {
  ArgumentReturn,
  CallerSavedTemp,
  CalleeSaved,
  ReservedMirScratch,
  SpecialOrForbidden,
};

enum class PreparedRegisterConversionErrorKind {
  EmptyRegisterName,
  UnknownRegisterName,
  UnsupportedRegisterView,
  RegisterBankMismatch,
  RegisterClassMismatch,
  RegisterViewMismatch,
};

struct RegisterReference {
  RegisterBank bank = RegisterBank::GeneralPurpose;
  RegisterView view = RegisterView::X;
  std::uint8_t index = 0;
};

struct PreparedRegisterConversionError {
  PreparedRegisterConversionErrorKind kind =
      PreparedRegisterConversionErrorKind::UnknownRegisterName;
  std::string register_name;
  std::optional<c4c::backend::prepare::PreparedRegisterBank> prepared_bank;
  std::optional<c4c::backend::prepare::PreparedRegisterClass> prepared_class;
  std::optional<RegisterView> expected_view;
  std::string message;
};

struct PreparedRegisterConversionResult {
  std::optional<RegisterReference> reg;
  std::optional<PreparedRegisterConversionError> error;

  [[nodiscard]] bool has_value() const { return reg.has_value(); }
};

enum class HandoffErrorKind {
  UnsupportedTargetArch,
  UnsupportedBackendAbi,
};

struct HandoffError {
  HandoffErrorKind kind = HandoffErrorKind::UnsupportedTargetArch;
  c4c::TargetProfile target_profile{};
  std::string message;
};

[[nodiscard]] bool operator==(const RegisterReference& lhs,
                              const RegisterReference& rhs);
[[nodiscard]] bool operator!=(const RegisterReference& lhs,
                              const RegisterReference& rhs);
[[nodiscard]] std::optional<RegisterReference> gp_register(std::uint8_t index,
                                                           RegisterView view);
[[nodiscard]] std::optional<RegisterReference> fp_simd_register(std::uint8_t index,
                                                                RegisterView view);
[[nodiscard]] RegisterReference x_register(std::uint8_t index);
[[nodiscard]] RegisterReference w_register(std::uint8_t index);
[[nodiscard]] RegisterReference s_register(std::uint8_t index);
[[nodiscard]] RegisterReference d_register(std::uint8_t index);
[[nodiscard]] RegisterReference q_register(std::uint8_t index);
[[nodiscard]] RegisterReference v_register(std::uint8_t index);
[[nodiscard]] RegisterReference frame_pointer_register();
[[nodiscard]] RegisterReference link_register();
[[nodiscard]] RegisterReference stack_pointer_register();
[[nodiscard]] RegisterReference sret_register();
[[nodiscard]] RegisterReference platform_reserved_register();
[[nodiscard]] std::array<RegisterReference, 2> indirect_call_scratch_registers();
[[nodiscard]] std::array<RegisterReference, 2> reserved_mir_scratch_gp_registers();
[[nodiscard]] std::array<RegisterReference, 2> reserved_mir_scratch_fp_simd_registers();
[[nodiscard]] std::array<RegisterReference, 19> caller_saved_gp_registers();
[[nodiscard]] std::array<RegisterReference, 11> callee_saved_gp_registers();
[[nodiscard]] std::array<RegisterReference, 24> caller_saved_fp_simd_registers();
[[nodiscard]] std::array<RegisterReference, 8> callee_saved_fp_simd_registers();
[[nodiscard]] bool is_valid_register_reference(RegisterReference reg);
[[nodiscard]] bool is_gp_register(RegisterReference reg);
[[nodiscard]] bool is_stack_pointer(RegisterReference reg);
[[nodiscard]] bool is_fp_simd_register(RegisterReference reg);
[[nodiscard]] bool is_frame_pointer(RegisterReference reg);
[[nodiscard]] bool is_link_register(RegisterReference reg);
[[nodiscard]] bool is_sret_register(RegisterReference reg);
[[nodiscard]] bool is_platform_reserved(RegisterReference reg);
[[nodiscard]] bool is_indirect_call_scratch(RegisterReference reg);
[[nodiscard]] bool is_reserved_mir_scratch(RegisterReference reg);
[[nodiscard]] bool is_special_or_forbidden(RegisterReference reg,
                                           bool frame_pointer_reserved = true);
[[nodiscard]] bool is_caller_saved(RegisterReference reg);
[[nodiscard]] bool is_callee_saved(RegisterReference reg);
[[nodiscard]] bool is_long_lived_allocatable_candidate(RegisterReference reg,
                                                       bool frame_pointer_reserved = true);
[[nodiscard]] AllocationRegisterPool allocation_register_pool(
    RegisterReference reg,
    bool frame_pointer_reserved = true);
[[nodiscard]] std::string register_name(RegisterReference reg);
[[nodiscard]] std::string_view register_bank_name(RegisterBank bank);
[[nodiscard]] std::string_view register_view_name(RegisterView view);
[[nodiscard]] std::string_view allocation_register_pool_name(AllocationRegisterPool pool);
[[nodiscard]] std::string_view prepared_register_conversion_error_kind_name(
    PreparedRegisterConversionErrorKind kind);
[[nodiscard]] std::optional<RegisterReference> parse_aarch64_register_name(
    std::string_view register_name);
[[nodiscard]] PreparedRegisterConversionResult convert_prepared_register(
    std::string_view register_name,
    std::optional<c4c::backend::prepare::PreparedRegisterBank> prepared_bank,
    std::optional<c4c::backend::prepare::PreparedRegisterClass> prepared_class,
    std::optional<RegisterView> expected_view);
[[nodiscard]] PreparedRegisterConversionResult convert_prepared_register(
    const c4c::backend::prepare::PreparedPhysicalRegisterAssignment& assignment,
    std::optional<RegisterView> expected_view);
[[nodiscard]] PreparedRegisterConversionResult convert_prepared_register(
    const c4c::backend::prepare::PreparedSavedRegister& saved_register,
    std::optional<c4c::backend::prepare::PreparedRegisterClass> prepared_class,
    std::optional<RegisterView> expected_view);
[[nodiscard]] c4c::TargetProfile resolve_target_profile(
    const c4c::backend::prepare::PreparedBirModule& module);
[[nodiscard]] bool is_aarch64_target(const c4c::TargetProfile& target_profile);
[[nodiscard]] bool is_aapcs64_abi(const c4c::TargetProfile& target_profile);
[[nodiscard]] std::optional<HandoffError> validate_prepared_module_handoff(
    const c4c::backend::prepare::PreparedBirModule& module);

}  // namespace c4c::backend::aarch64::abi
