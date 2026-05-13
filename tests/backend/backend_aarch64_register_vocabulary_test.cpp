#include "src/backend/mir/aarch64/abi/abi.hpp"

#include <iostream>
#include <optional>
#include <string>

namespace {

namespace aarch64_abi = c4c::backend::aarch64::abi;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

int representative_gp_and_sp_registers_are_typed() {
  const auto x0 = aarch64_abi::gp_register(0, aarch64_abi::RegisterView::X);
  const auto w9 = aarch64_abi::gp_register(9, aarch64_abi::RegisterView::W);
  const auto invalid_x31 = aarch64_abi::gp_register(31, aarch64_abi::RegisterView::X);
  const auto invalid_sp_view = aarch64_abi::gp_register(3, aarch64_abi::RegisterView::Sp);

  if (!x0.has_value() || !w9.has_value()) {
    return fail("expected representative GP registers to construct");
  }
  if (invalid_x31.has_value() || invalid_sp_view.has_value()) {
    return fail("expected GP helper to reject x31 and non-GP views");
  }
  if (!aarch64_abi::is_gp_register(*x0) || !aarch64_abi::is_gp_register(*w9)) {
    return fail("expected x and w views to classify as GP registers");
  }
  if (aarch64_abi::register_name(*x0) != "x0" ||
      aarch64_abi::register_name(*w9) != "w9") {
    return fail("expected GP register spelling to preserve view and index");
  }

  const auto sp = aarch64_abi::stack_pointer_register();
  if (!aarch64_abi::is_stack_pointer(sp) || aarch64_abi::is_gp_register(sp)) {
    return fail("expected stack pointer to be a distinct typed register");
  }
  if (aarch64_abi::register_name(sp) != "sp") {
    return fail("expected stack pointer spelling to be sp");
  }
  return 0;
}

int representative_fp_simd_registers_are_typed() {
  const auto s1 = aarch64_abi::fp_simd_register(1, aarch64_abi::RegisterView::S);
  const auto d2 = aarch64_abi::fp_simd_register(2, aarch64_abi::RegisterView::D);
  const auto q3 = aarch64_abi::fp_simd_register(3, aarch64_abi::RegisterView::Q);
  const auto v31 = aarch64_abi::fp_simd_register(31, aarch64_abi::RegisterView::V);
  const auto invalid_v32 = aarch64_abi::fp_simd_register(32, aarch64_abi::RegisterView::V);
  const auto invalid_x_view = aarch64_abi::fp_simd_register(0, aarch64_abi::RegisterView::X);

  if (!s1.has_value() || !d2.has_value() || !q3.has_value() || !v31.has_value()) {
    return fail("expected representative FP/SIMD registers to construct");
  }
  if (invalid_v32.has_value() || invalid_x_view.has_value()) {
    return fail("expected FP/SIMD helper to reject invalid index and GP view");
  }
  if (!aarch64_abi::is_fp_simd_register(*s1) || !aarch64_abi::is_fp_simd_register(*d2) ||
      !aarch64_abi::is_fp_simd_register(*q3) || !aarch64_abi::is_fp_simd_register(*v31)) {
    return fail("expected scalar and vector views to classify as FP/SIMD registers");
  }
  if (aarch64_abi::register_name(*s1) != "s1" ||
      aarch64_abi::register_name(*d2) != "d2" ||
      aarch64_abi::register_name(*q3) != "q3" ||
      aarch64_abi::register_name(*v31) != "v31") {
    return fail("expected FP/SIMD spelling to preserve view and index");
  }
  return 0;
}

int special_gp_roles_are_classified() {
  if (!aarch64_abi::is_frame_pointer(aarch64_abi::x_register(29)) ||
      !aarch64_abi::is_link_register(aarch64_abi::x_register(30))) {
    return fail("expected x29 and x30 to classify as FP and LR");
  }
  if (!aarch64_abi::is_sret_register(aarch64_abi::x_register(8))) {
    return fail("expected x8 to classify as the sret register");
  }
  if (!aarch64_abi::is_platform_reserved(aarch64_abi::x_register(18))) {
    return fail("expected x18 to classify as the platform-reserved register");
  }
  if (!aarch64_abi::is_indirect_call_scratch(aarch64_abi::x_register(16)) ||
      !aarch64_abi::is_indirect_call_scratch(aarch64_abi::x_register(17)) ||
      aarch64_abi::is_indirect_call_scratch(aarch64_abi::x_register(15))) {
    return fail("expected only x16/x17 to classify as indirect-call scratch");
  }
  if (!aarch64_abi::is_caller_saved(aarch64_abi::x_register(0)) ||
      !aarch64_abi::is_caller_saved(aarch64_abi::link_register()) ||
      aarch64_abi::is_caller_saved(aarch64_abi::platform_reserved_register())) {
    return fail("expected caller-saved GP classification to exclude x18 and include LR");
  }
  if (!aarch64_abi::is_callee_saved(aarch64_abi::x_register(19)) ||
      !aarch64_abi::is_callee_saved(aarch64_abi::frame_pointer_register()) ||
      aarch64_abi::is_callee_saved(aarch64_abi::link_register())) {
    return fail("expected callee-saved GP classification to include x19 and FP only");
  }
  return 0;
}

int fp_simd_preservation_roles_are_classified_by_register_number() {
  if (!aarch64_abi::is_caller_saved(aarch64_abi::s_register(0)) ||
      !aarch64_abi::is_caller_saved(aarch64_abi::q_register(16)) ||
      aarch64_abi::is_caller_saved(aarch64_abi::d_register(8))) {
    return fail("expected v0-v7 and v16-v31 to classify as FP/SIMD caller-saved");
  }
  if (!aarch64_abi::is_callee_saved(aarch64_abi::d_register(8)) ||
      !aarch64_abi::is_callee_saved(aarch64_abi::q_register(15)) ||
      aarch64_abi::is_callee_saved(aarch64_abi::v_register(16))) {
    return fail("expected v8-v15 to classify as FP/SIMD callee-saved");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = representative_gp_and_sp_registers_are_typed(); status != 0) {
    return status;
  }
  if (const int status = representative_fp_simd_registers_are_typed(); status != 0) {
    return status;
  }
  if (const int status = special_gp_roles_are_classified(); status != 0) {
    return status;
  }
  if (const int status = fp_simd_preservation_roles_are_classified_by_register_number();
      status != 0) {
    return status;
  }
  return 0;
}
