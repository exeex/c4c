#include "abi.hpp"

#include <cassert>
#include <charconv>
#include <system_error>

namespace c4c::backend::aarch64::abi {

namespace {

namespace prepare = c4c::backend::prepare;

constexpr bool is_gp_view(RegisterView view) {
  return view == RegisterView::X || view == RegisterView::W;
}

constexpr bool is_fp_simd_view(RegisterView view) {
  return view == RegisterView::S || view == RegisterView::D || view == RegisterView::Q ||
         view == RegisterView::V;
}

constexpr bool is_float_view(RegisterView view) {
  return view == RegisterView::S || view == RegisterView::D;
}

constexpr bool is_vector_view(RegisterView view) {
  return view == RegisterView::Q || view == RegisterView::V;
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

std::optional<std::uint8_t> parse_register_index(std::string_view digits,
                                                 std::uint8_t max_index) {
  if (digits.empty()) {
    return std::nullopt;
  }
  if (digits.size() > 1 && digits.front() == '0') {
    return std::nullopt;
  }
  unsigned parsed = 0;
  const char* begin = digits.data();
  const char* end = digits.data() + digits.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc{} || result.ptr != end || parsed > max_index) {
    return std::nullopt;
  }
  return static_cast<std::uint8_t>(parsed);
}

std::string conversion_message(PreparedRegisterConversionErrorKind kind,
                               std::string_view register_name) {
  return std::string(prepared_register_conversion_error_kind_name(kind)) +
         " for prepared register `" + std::string(register_name) + "`";
}

PreparedRegisterConversionResult conversion_failure(
    PreparedRegisterConversionErrorKind kind,
    std::string_view register_name,
    std::optional<prepare::PreparedRegisterBank> prepared_bank,
    std::optional<prepare::PreparedRegisterClass> prepared_class,
    std::optional<RegisterView> expected_view) {
  return PreparedRegisterConversionResult{
      .reg = std::nullopt,
      .error = PreparedRegisterConversionError{
          .kind = kind,
          .register_name = std::string(register_name),
          .prepared_bank = prepared_bank,
          .prepared_class = prepared_class,
          .expected_view = expected_view,
          .message = conversion_message(kind, register_name),
      },
  };
}

std::string placement_name(const prepare::PreparedRegisterPlacement& placement) {
  return std::string("placement=") +
         std::string(prepare::prepared_register_bank_name(placement.bank)) + ":" +
         std::string(prepare::prepared_register_slot_pool_name(placement.pool)) + "#" +
         std::to_string(placement.slot_index) + "/w" +
         std::to_string(placement.contiguous_width);
}

bool prepared_bank_group_matches_register(prepare::PreparedRegisterBank bank,
                                          RegisterReference reg) {
  switch (bank) {
    case prepare::PreparedRegisterBank::Gpr:
    case prepare::PreparedRegisterBank::AggregateAddress:
      return reg.bank == RegisterBank::GeneralPurpose;
    case prepare::PreparedRegisterBank::Fpr:
    case prepare::PreparedRegisterBank::Vreg:
      return reg.bank == RegisterBank::FpSimd;
    case prepare::PreparedRegisterBank::None:
      return false;
  }
  return false;
}

bool prepared_class_group_matches_register(prepare::PreparedRegisterClass reg_class,
                                           RegisterReference reg) {
  switch (reg_class) {
    case prepare::PreparedRegisterClass::General:
    case prepare::PreparedRegisterClass::AggregateAddress:
      return reg.bank == RegisterBank::GeneralPurpose;
    case prepare::PreparedRegisterClass::Float:
    case prepare::PreparedRegisterClass::Vector:
      return reg.bank == RegisterBank::FpSimd;
    case prepare::PreparedRegisterClass::None:
      return false;
  }
  return false;
}

bool bank_supports_view(prepare::PreparedRegisterBank bank, RegisterView view) {
  switch (bank) {
    case prepare::PreparedRegisterBank::Gpr:
    case prepare::PreparedRegisterBank::AggregateAddress:
      return view == RegisterView::X || view == RegisterView::W;
    case prepare::PreparedRegisterBank::Fpr:
      return is_float_view(view);
    case prepare::PreparedRegisterBank::Vreg:
      return is_vector_view(view);
    case prepare::PreparedRegisterBank::None:
      return false;
  }
  return false;
}

bool class_supports_view(prepare::PreparedRegisterClass reg_class, RegisterView view) {
  switch (reg_class) {
    case prepare::PreparedRegisterClass::General:
    case prepare::PreparedRegisterClass::AggregateAddress:
      return view == RegisterView::X || view == RegisterView::W;
    case prepare::PreparedRegisterClass::Float:
      return is_float_view(view);
    case prepare::PreparedRegisterClass::Vector:
      return is_vector_view(view);
    case prepare::PreparedRegisterClass::None:
      return false;
  }
  return false;
}

template <std::size_t Count>
std::optional<RegisterReference> indexed_register(
    const std::array<RegisterReference, Count>& registers,
    std::size_t index) {
  if (index >= registers.size()) {
    return std::nullopt;
  }
  return registers[index];
}

std::optional<RegisterReference> call_abi_register(
    const prepare::PreparedRegisterPlacement& placement) {
  if (placement.slot_index > 7) {
    return std::nullopt;
  }
  switch (placement.bank) {
    case prepare::PreparedRegisterBank::Gpr:
    case prepare::PreparedRegisterBank::AggregateAddress:
      return x_register(static_cast<std::uint8_t>(placement.slot_index));
    case prepare::PreparedRegisterBank::Fpr:
    case prepare::PreparedRegisterBank::Vreg:
      return v_register(static_cast<std::uint8_t>(placement.slot_index));
    case prepare::PreparedRegisterBank::None:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<RegisterReference> prepared_caller_saved_register(
    const prepare::PreparedRegisterPlacement& placement) {
  if (placement.slot_index != 0 || placement.contiguous_width != 1) {
    return std::nullopt;
  }

  switch (placement.bank) {
    case prepare::PreparedRegisterBank::Gpr:
      return x_register(13);
    case prepare::PreparedRegisterBank::Fpr:
      return d_register(13);
    case prepare::PreparedRegisterBank::Vreg:
      return v_register(13);
    case prepare::PreparedRegisterBank::AggregateAddress:
    case prepare::PreparedRegisterBank::None:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<RegisterReference> placement_register(
    const prepare::PreparedRegisterPlacement& placement) {
  switch (placement.pool) {
    case prepare::PreparedRegisterSlotPool::CallArgument:
      return call_abi_register(placement);
    case prepare::PreparedRegisterSlotPool::CallResult:
      if (placement.slot_index != 0) {
        return std::nullopt;
      }
      return call_abi_register(placement);
    case prepare::PreparedRegisterSlotPool::CallerSaved:
      return prepared_caller_saved_register(placement);
      break;
    case prepare::PreparedRegisterSlotPool::CalleeSaved:
      switch (placement.bank) {
        case prepare::PreparedRegisterBank::Gpr:
        case prepare::PreparedRegisterBank::AggregateAddress:
          return indexed_register(callee_saved_gp_registers(), placement.slot_index);
        case prepare::PreparedRegisterBank::Fpr:
        case prepare::PreparedRegisterBank::Vreg:
          return indexed_register(callee_saved_fp_simd_registers(), placement.slot_index);
        case prepare::PreparedRegisterBank::None:
          return std::nullopt;
      }
      break;
    case prepare::PreparedRegisterSlotPool::ReservedScratch:
      switch (placement.bank) {
        case prepare::PreparedRegisterBank::Gpr:
        case prepare::PreparedRegisterBank::AggregateAddress:
          return indexed_register(reserved_mir_scratch_gp_registers(), placement.slot_index);
        case prepare::PreparedRegisterBank::Fpr:
        case prepare::PreparedRegisterBank::Vreg:
          return indexed_register(reserved_mir_scratch_fp_simd_registers(), placement.slot_index);
        case prepare::PreparedRegisterBank::None:
          return std::nullopt;
      }
      break;
    case prepare::PreparedRegisterSlotPool::None:
      return std::nullopt;
  }
  return std::nullopt;
}

std::string explicit_target_triple(const prepare::PreparedBirModule& module) {
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

std::array<RegisterReference, 2> reserved_mir_scratch_gp_registers() {
  return {x_register(9), x_register(10)};
}

std::array<RegisterReference, 2> reserved_mir_scratch_fp_simd_registers() {
  return {v_register(16), v_register(17)};
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

bool is_reserved_mir_scratch(RegisterReference reg) {
  if (!is_valid_register_reference(reg)) {
    return false;
  }
  if (reg.bank == RegisterBank::GeneralPurpose) {
    return contains_register(reserved_mir_scratch_gp_registers(), reg);
  }
  if (reg.bank == RegisterBank::FpSimd) {
    return contains_register(reserved_mir_scratch_fp_simd_registers(), v_register(reg.index));
  }
  return false;
}

bool is_special_or_forbidden(RegisterReference reg, bool frame_pointer_reserved) {
  if (!is_valid_register_reference(reg)) {
    return true;
  }
  if (is_stack_pointer(reg) || is_sret_register(reg) || is_platform_reserved(reg) ||
      is_link_register(reg) || is_indirect_call_scratch(reg)) {
    return true;
  }
  return frame_pointer_reserved && is_frame_pointer(reg);
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

bool is_long_lived_allocatable_candidate(RegisterReference reg, bool frame_pointer_reserved) {
  return is_valid_register_reference(reg) && !is_reserved_mir_scratch(reg) &&
         !is_special_or_forbidden(reg, frame_pointer_reserved) &&
         (reg.bank == RegisterBank::GeneralPurpose || reg.bank == RegisterBank::FpSimd);
}

AllocationRegisterPool allocation_register_pool(RegisterReference reg,
                                                bool frame_pointer_reserved) {
  if (is_reserved_mir_scratch(reg)) {
    return AllocationRegisterPool::ReservedMirScratch;
  }
  if (is_special_or_forbidden(reg, frame_pointer_reserved)) {
    return AllocationRegisterPool::SpecialOrForbidden;
  }
  if ((reg.bank == RegisterBank::GeneralPurpose && reg.index <= 7) ||
      (reg.bank == RegisterBank::FpSimd && reg.index <= 7)) {
    return AllocationRegisterPool::ArgumentReturn;
  }
  if (is_callee_saved(reg)) {
    return AllocationRegisterPool::CalleeSaved;
  }
  if (is_caller_saved(reg)) {
    return AllocationRegisterPool::CallerSavedTemp;
  }
  return AllocationRegisterPool::SpecialOrForbidden;
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

std::string_view allocation_register_pool_name(AllocationRegisterPool pool) {
  switch (pool) {
    case AllocationRegisterPool::ArgumentReturn:
      return "argument_return";
    case AllocationRegisterPool::CallerSavedTemp:
      return "caller_saved_temp";
    case AllocationRegisterPool::CalleeSaved:
      return "callee_saved";
    case AllocationRegisterPool::ReservedMirScratch:
      return "reserved_mir_scratch";
    case AllocationRegisterPool::SpecialOrForbidden:
      return "special_or_forbidden";
  }
  return "unknown";
}

std::string_view prepared_register_conversion_error_kind_name(
    PreparedRegisterConversionErrorKind kind) {
  switch (kind) {
    case PreparedRegisterConversionErrorKind::EmptyRegisterName:
      return "empty_register_name";
    case PreparedRegisterConversionErrorKind::UnknownRegisterName:
      return "unknown_register_name";
    case PreparedRegisterConversionErrorKind::UnsupportedRegisterView:
      return "unsupported_register_view";
    case PreparedRegisterConversionErrorKind::RegisterBankMismatch:
      return "register_bank_mismatch";
    case PreparedRegisterConversionErrorKind::RegisterClassMismatch:
      return "register_class_mismatch";
    case PreparedRegisterConversionErrorKind::RegisterViewMismatch:
      return "register_view_mismatch";
  }
  return "unknown";
}

std::optional<RegisterReference> parse_aarch64_register_name(std::string_view register_name) {
  if (register_name == "sp") {
    return stack_pointer_register();
  }
  if (register_name.size() < 2) {
    return std::nullopt;
  }

  const char prefix = register_name.front();
  const std::string_view digits = register_name.substr(1);
  switch (prefix) {
    case 'x':
      if (const auto index = parse_register_index(digits, 30); index.has_value()) {
        return gp_register(*index, RegisterView::X);
      }
      return std::nullopt;
    case 'w':
      if (const auto index = parse_register_index(digits, 30); index.has_value()) {
        return gp_register(*index, RegisterView::W);
      }
      return std::nullopt;
    case 's':
      if (const auto index = parse_register_index(digits, 31); index.has_value()) {
        return fp_simd_register(*index, RegisterView::S);
      }
      return std::nullopt;
    case 'd':
      if (const auto index = parse_register_index(digits, 31); index.has_value()) {
        return fp_simd_register(*index, RegisterView::D);
      }
      return std::nullopt;
    case 'q':
      if (const auto index = parse_register_index(digits, 31); index.has_value()) {
        return fp_simd_register(*index, RegisterView::Q);
      }
      return std::nullopt;
    case 'v':
      if (const auto index = parse_register_index(digits, 31); index.has_value()) {
        return fp_simd_register(*index, RegisterView::V);
      }
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

PreparedRegisterConversionResult convert_prepared_register(
    std::string_view register_name,
    std::optional<prepare::PreparedRegisterBank> prepared_bank,
    std::optional<prepare::PreparedRegisterClass> prepared_class,
    std::optional<RegisterView> expected_view) {
  if (register_name.empty()) {
    return conversion_failure(PreparedRegisterConversionErrorKind::EmptyRegisterName,
                              register_name,
                              prepared_bank,
                              prepared_class,
                              expected_view);
  }

  const auto parsed = parse_aarch64_register_name(register_name);
  if (!parsed.has_value()) {
    return conversion_failure(PreparedRegisterConversionErrorKind::UnknownRegisterName,
                              register_name,
                              prepared_bank,
                              prepared_class,
                              expected_view);
  }

  if (expected_view.has_value() && parsed->view != *expected_view) {
    return conversion_failure(PreparedRegisterConversionErrorKind::RegisterViewMismatch,
                              register_name,
                              prepared_bank,
                              prepared_class,
                              expected_view);
  }

  if (prepared_bank.has_value()) {
    if (!prepared_bank_group_matches_register(*prepared_bank, *parsed)) {
      return conversion_failure(PreparedRegisterConversionErrorKind::RegisterBankMismatch,
                                register_name,
                                prepared_bank,
                                prepared_class,
                                expected_view);
    }
    if (!bank_supports_view(*prepared_bank, parsed->view)) {
      return conversion_failure(PreparedRegisterConversionErrorKind::UnsupportedRegisterView,
                                register_name,
                                prepared_bank,
                                prepared_class,
                                expected_view);
    }
  }

  if (prepared_class.has_value()) {
    if (!prepared_class_group_matches_register(*prepared_class, *parsed)) {
      return conversion_failure(PreparedRegisterConversionErrorKind::RegisterClassMismatch,
                                register_name,
                                prepared_bank,
                                prepared_class,
                                expected_view);
    }
    if (!class_supports_view(*prepared_class, parsed->view)) {
      return conversion_failure(PreparedRegisterConversionErrorKind::UnsupportedRegisterView,
                                register_name,
                                prepared_bank,
                                prepared_class,
                                expected_view);
    }
  }

  return PreparedRegisterConversionResult{
      .reg = *parsed,
      .error = std::nullopt,
  };
}

PreparedRegisterConversionResult convert_prepared_register(
    const prepare::PreparedRegisterPlacement& placement,
    std::optional<prepare::PreparedRegisterClass> prepared_class,
    std::optional<RegisterView> expected_view) {
  const std::string display_name = placement_name(placement);
  const auto mapped = placement_register(placement);
  if (!mapped.has_value()) {
    return conversion_failure(PreparedRegisterConversionErrorKind::UnknownRegisterName,
                              display_name,
                              placement.bank,
                              prepared_class,
                              expected_view);
  }

  RegisterReference reg = *mapped;
  if (expected_view.has_value()) {
    reg.view = *expected_view;
  }

  if (!prepared_bank_group_matches_register(placement.bank, reg)) {
    return conversion_failure(PreparedRegisterConversionErrorKind::RegisterBankMismatch,
                              display_name,
                              placement.bank,
                              prepared_class,
                              expected_view);
  }
  if (!bank_supports_view(placement.bank, reg.view)) {
    return conversion_failure(PreparedRegisterConversionErrorKind::UnsupportedRegisterView,
                              display_name,
                              placement.bank,
                              prepared_class,
                              expected_view);
  }

  if (prepared_class.has_value()) {
    if (!prepared_class_group_matches_register(*prepared_class, reg)) {
      return conversion_failure(PreparedRegisterConversionErrorKind::RegisterClassMismatch,
                                display_name,
                                placement.bank,
                                prepared_class,
                                expected_view);
    }
    if (!class_supports_view(*prepared_class, reg.view)) {
      return conversion_failure(PreparedRegisterConversionErrorKind::UnsupportedRegisterView,
                                display_name,
                                placement.bank,
                                prepared_class,
                                expected_view);
    }
  }

  if (!is_valid_register_reference(reg)) {
    return conversion_failure(PreparedRegisterConversionErrorKind::UnsupportedRegisterView,
                              display_name,
                              placement.bank,
                              prepared_class,
                              expected_view);
  }

  return PreparedRegisterConversionResult{
      .reg = reg,
      .error = std::nullopt,
  };
}

PreparedRegisterConversionResult convert_prepared_register(
    const prepare::PreparedPhysicalRegisterAssignment& assignment,
    std::optional<RegisterView> expected_view) {
  if (assignment.placement.has_value()) {
    return convert_prepared_register(*assignment.placement,
                                     assignment.reg_class,
                                     expected_view);
  }
  return convert_prepared_register(assignment.register_name,
                                   std::nullopt,
                                   assignment.reg_class,
                                   expected_view);
}

PreparedRegisterConversionResult convert_prepared_register(
    const prepare::PreparedSavedRegister& saved_register,
    std::optional<prepare::PreparedRegisterClass> prepared_class,
    std::optional<RegisterView> expected_view) {
  if (saved_register.placement.has_value()) {
    return convert_prepared_register(*saved_register.placement,
                                     prepared_class,
                                     expected_view);
  }
  return convert_prepared_register(saved_register.register_name,
                                   saved_register.bank,
                                   prepared_class,
                                   expected_view);
}

c4c::TargetProfile resolve_target_profile(
    const prepare::PreparedBirModule& module) {
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
    const prepare::PreparedBirModule& module) {
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
