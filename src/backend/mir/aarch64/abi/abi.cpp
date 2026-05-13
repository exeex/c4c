#include "abi.hpp"

#include <cassert>

namespace c4c::backend::aarch64::abi {

namespace {

constexpr bool is_gp_view(RegisterView view) {
  return view == RegisterView::X || view == RegisterView::W;
}

constexpr bool is_fp_simd_view(RegisterView view) {
  return view == RegisterView::S || view == RegisterView::D || view == RegisterView::Q ||
         view == RegisterView::V;
}

template <std::size_t Count>
bool contains_register(const std::array<RegisterReference, Count>& registers,
                       RegisterReference reg) {
  for (const RegisterReference candidate : registers) {
    if (candidate == reg) {
      return true;
    }
  }
  return false;
}

constexpr char view_prefix(RegisterView view) {
  switch (view) {
    case RegisterView::X:
      return 'x';
    case RegisterView::W:
      return 'w';
    case RegisterView::S:
      return 's';
    case RegisterView::D:
      return 'd';
    case RegisterView::Q:
      return 'q';
    case RegisterView::V:
      return 'v';
    case RegisterView::Sp:
      break;
  }
  return '?';
}

std::string explicit_target_triple(const c4c::backend::prepare::PreparedBirModule& module) {
  if (!module.target_profile.triple.empty()) {
    return module.target_profile.triple;
  }
  if (!module.module.target_triple.empty()) {
    return module.module.target_triple;
  }
  return c4c::llvm_target_triple(module.target_profile);
}

HandoffError make_error(HandoffErrorKind kind,
                        const c4c::TargetProfile& target_profile,
                        const char* requirement) {
  return HandoffError{
      .kind = kind,
      .target_profile = target_profile,
      .message = std::string("AArch64 prepared-module handoff requires ") + requirement +
                 ", got arch=" + c4c::target_arch_name(target_profile.arch) +
                 " abi=" + c4c::backend_abi_name(target_profile.backend_abi),
  };
}

}  // namespace

bool operator==(const RegisterReference& lhs, const RegisterReference& rhs) {
  return lhs.bank == rhs.bank && lhs.view == rhs.view && lhs.index == rhs.index;
}

bool operator!=(const RegisterReference& lhs, const RegisterReference& rhs) {
  return !(lhs == rhs);
}

std::optional<RegisterReference> gp_register(std::uint8_t index, RegisterView view) {
  if (index > 30 || !is_gp_view(view)) {
    return std::nullopt;
  }
  return RegisterReference{.bank = RegisterBank::GeneralPurpose, .view = view, .index = index};
}

std::optional<RegisterReference> fp_simd_register(std::uint8_t index, RegisterView view) {
  if (index > 31 || !is_fp_simd_view(view)) {
    return std::nullopt;
  }
  return RegisterReference{.bank = RegisterBank::FpSimd, .view = view, .index = index};
}

RegisterReference x_register(std::uint8_t index) {
  auto reg = gp_register(index, RegisterView::X);
  assert(reg.has_value());
  return *reg;
}

RegisterReference w_register(std::uint8_t index) {
  auto reg = gp_register(index, RegisterView::W);
  assert(reg.has_value());
  return *reg;
}

RegisterReference s_register(std::uint8_t index) {
  auto reg = fp_simd_register(index, RegisterView::S);
  assert(reg.has_value());
  return *reg;
}

RegisterReference d_register(std::uint8_t index) {
  auto reg = fp_simd_register(index, RegisterView::D);
  assert(reg.has_value());
  return *reg;
}

RegisterReference q_register(std::uint8_t index) {
  auto reg = fp_simd_register(index, RegisterView::Q);
  assert(reg.has_value());
  return *reg;
}

RegisterReference v_register(std::uint8_t index) {
  auto reg = fp_simd_register(index, RegisterView::V);
  assert(reg.has_value());
  return *reg;
}

RegisterReference frame_pointer_register() {
  return x_register(29);
}

RegisterReference link_register() {
  return x_register(30);
}

RegisterReference stack_pointer_register() {
  return RegisterReference{.bank = RegisterBank::StackPointer, .view = RegisterView::Sp, .index = 31};
}

RegisterReference sret_register() {
  return x_register(8);
}

RegisterReference platform_reserved_register() {
  return x_register(18);
}

std::array<RegisterReference, 2> indirect_call_scratch_registers() {
  return {x_register(16), x_register(17)};
}

std::array<RegisterReference, 19> caller_saved_gp_registers() {
  return {x_register(0),  x_register(1),  x_register(2),  x_register(3),  x_register(4),
          x_register(5),  x_register(6),  x_register(7),  x_register(8),  x_register(9),
          x_register(10), x_register(11), x_register(12), x_register(13), x_register(14),
          x_register(15), x_register(16), x_register(17), x_register(30)};
}

std::array<RegisterReference, 11> callee_saved_gp_registers() {
  return {x_register(19), x_register(20), x_register(21), x_register(22), x_register(23),
          x_register(24), x_register(25), x_register(26), x_register(27), x_register(28),
          x_register(29)};
}

std::array<RegisterReference, 24> caller_saved_fp_simd_registers() {
  return {v_register(0),  v_register(1),  v_register(2),  v_register(3),  v_register(4),
          v_register(5),  v_register(6),  v_register(7),  v_register(16), v_register(17),
          v_register(18), v_register(19), v_register(20), v_register(21), v_register(22),
          v_register(23), v_register(24), v_register(25), v_register(26), v_register(27),
          v_register(28), v_register(29), v_register(30), v_register(31)};
}

std::array<RegisterReference, 8> callee_saved_fp_simd_registers() {
  return {v_register(8),  v_register(9),  v_register(10), v_register(11),
          v_register(12), v_register(13), v_register(14), v_register(15)};
}

bool is_valid_register_reference(RegisterReference reg) {
  switch (reg.bank) {
    case RegisterBank::GeneralPurpose:
      return reg.index <= 30 && is_gp_view(reg.view);
    case RegisterBank::StackPointer:
      return reg.index == 31 && reg.view == RegisterView::Sp;
    case RegisterBank::FpSimd:
      return reg.index <= 31 && is_fp_simd_view(reg.view);
  }
  return false;
}

bool is_gp_register(RegisterReference reg) {
  return reg.bank == RegisterBank::GeneralPurpose && is_valid_register_reference(reg);
}

bool is_stack_pointer(RegisterReference reg) {
  return reg.bank == RegisterBank::StackPointer && is_valid_register_reference(reg);
}

bool is_fp_simd_register(RegisterReference reg) {
  return reg.bank == RegisterBank::FpSimd && is_valid_register_reference(reg);
}

bool is_frame_pointer(RegisterReference reg) {
  return reg == frame_pointer_register();
}

bool is_link_register(RegisterReference reg) {
  return reg == link_register();
}

bool is_sret_register(RegisterReference reg) {
  return reg == sret_register();
}

bool is_platform_reserved(RegisterReference reg) {
  return reg == platform_reserved_register();
}

bool is_indirect_call_scratch(RegisterReference reg) {
  return contains_register(indirect_call_scratch_registers(), reg);
}

bool is_caller_saved(RegisterReference reg) {
  if (!is_valid_register_reference(reg)) {
    return false;
  }
  if (reg.bank == RegisterBank::GeneralPurpose) {
    return contains_register(caller_saved_gp_registers(), reg);
  }
  if (reg.bank == RegisterBank::FpSimd) {
    return contains_register(caller_saved_fp_simd_registers(), v_register(reg.index));
  }
  return false;
}

bool is_callee_saved(RegisterReference reg) {
  if (!is_valid_register_reference(reg)) {
    return false;
  }
  if (reg.bank == RegisterBank::GeneralPurpose) {
    return contains_register(callee_saved_gp_registers(), reg);
  }
  if (reg.bank == RegisterBank::FpSimd) {
    return contains_register(callee_saved_fp_simd_registers(), v_register(reg.index));
  }
  return false;
}

std::string register_name(RegisterReference reg) {
  if (is_stack_pointer(reg)) {
    return "sp";
  }
  if (!is_valid_register_reference(reg)) {
    return "<invalid-aarch64-register>";
  }
  return std::string(1, view_prefix(reg.view)) + std::to_string(reg.index);
}

std::string_view register_bank_name(RegisterBank bank) {
  switch (bank) {
    case RegisterBank::GeneralPurpose:
      return "gp";
    case RegisterBank::StackPointer:
      return "sp";
    case RegisterBank::FpSimd:
      return "fp_simd";
  }
  return "unknown";
}

std::string_view register_view_name(RegisterView view) {
  switch (view) {
    case RegisterView::X:
      return "x";
    case RegisterView::W:
      return "w";
    case RegisterView::Sp:
      return "sp";
    case RegisterView::S:
      return "s";
    case RegisterView::D:
      return "d";
    case RegisterView::Q:
      return "q";
    case RegisterView::V:
      return "v";
  }
  return "unknown";
}

c4c::TargetProfile resolve_target_profile(
    const c4c::backend::prepare::PreparedBirModule& module) {
  if (module.target_profile.arch != c4c::TargetArch::Unknown) {
    return module.target_profile;
  }
  const std::string target_triple = explicit_target_triple(module);
  if (!target_triple.empty()) {
    return c4c::target_profile_from_triple(target_triple);
  }
  return module.target_profile;
}

bool is_aarch64_target(const c4c::TargetProfile& target_profile) {
  return target_profile.arch == c4c::TargetArch::Aarch64;
}

bool is_aapcs64_abi(const c4c::TargetProfile& target_profile) {
  return target_profile.backend_abi == c4c::BackendAbiKind::Aapcs64;
}

std::optional<HandoffError> validate_prepared_module_handoff(
    const c4c::backend::prepare::PreparedBirModule& module) {
  const c4c::TargetProfile target_profile = resolve_target_profile(module);
  if (!is_aarch64_target(target_profile)) {
    return make_error(HandoffErrorKind::UnsupportedTargetArch, target_profile,
                      "TargetArch::Aarch64");
  }
  if (!is_aapcs64_abi(target_profile)) {
    return make_error(HandoffErrorKind::UnsupportedBackendAbi, target_profile,
                      "BackendAbiKind::Aapcs64");
  }
  return std::nullopt;
}

}  // namespace c4c::backend::aarch64::abi
