#include "src/backend/mir/aarch64/abi/abi.hpp"

#include <iostream>
#include <optional>
#include <string>

namespace {

namespace aarch64_abi = c4c::backend::aarch64::abi;
namespace prepare = c4c::backend::prepare;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

bool failed_with(const aarch64_abi::PreparedRegisterConversionResult& result,
                 aarch64_abi::PreparedRegisterConversionErrorKind kind) {
  return !result.has_value() && result.error.has_value() && result.error->kind == kind;
}

int converts_representative_gp_sp_and_fp_simd_spellings() {
  const auto x9 = aarch64_abi::convert_prepared_register(
      "x9",
      prepare::PreparedRegisterBank::Gpr,
      prepare::PreparedRegisterClass::General,
      aarch64_abi::RegisterView::X);
  const auto w4 = aarch64_abi::convert_prepared_register(
      "w4",
      prepare::PreparedRegisterBank::Gpr,
      prepare::PreparedRegisterClass::General,
      aarch64_abi::RegisterView::W);
  const auto sp = aarch64_abi::convert_prepared_register(
      "sp",
      std::nullopt,
      std::nullopt,
      aarch64_abi::RegisterView::Sp);
  const auto d13 = aarch64_abi::convert_prepared_register(
      "d13",
      prepare::PreparedRegisterBank::Fpr,
      prepare::PreparedRegisterClass::Float,
      aarch64_abi::RegisterView::D);
  const auto v21 = aarch64_abi::convert_prepared_register(
      "v21",
      prepare::PreparedRegisterBank::Vreg,
      prepare::PreparedRegisterClass::Vector,
      aarch64_abi::RegisterView::V);

  if (!x9.has_value() || x9.reg != aarch64_abi::gp_register(9, aarch64_abi::RegisterView::X)) {
    return fail("expected x9 prepared metadata to convert to a typed GP register");
  }
  if (!w4.has_value() || w4.reg != aarch64_abi::gp_register(4, aarch64_abi::RegisterView::W)) {
    return fail("expected w4 prepared metadata to convert to a typed GP register");
  }
  if (!sp.has_value() || sp.reg != aarch64_abi::stack_pointer_register()) {
    return fail("expected sp spelling to parse as the distinct stack pointer register");
  }
  if (!d13.has_value() ||
      d13.reg != aarch64_abi::fp_simd_register(13, aarch64_abi::RegisterView::D)) {
    return fail("expected d13 prepared metadata to convert to a typed FP register");
  }
  if (!v21.has_value() ||
      v21.reg != aarch64_abi::fp_simd_register(21, aarch64_abi::RegisterView::V)) {
    return fail("expected v21 prepared metadata to convert to a typed vector register");
  }
  return 0;
}

int converts_prepared_assignment_and_saved_register_carriers() {
  const prepare::PreparedPhysicalRegisterAssignment assignment{
      .reg_class = prepare::PreparedRegisterClass::Float,
      .register_name = "s1",
      .contiguous_width = 1,
      .occupied_register_names = {"s1"},
  };
  const auto assigned =
      aarch64_abi::convert_prepared_register(assignment, aarch64_abi::RegisterView::S);
  if (!assigned.has_value() ||
      assigned.reg != aarch64_abi::fp_simd_register(1, aarch64_abi::RegisterView::S)) {
    return fail("expected prepared assignment carrier to convert through its class metadata");
  }

  const prepare::PreparedSavedRegister saved{
      .bank = prepare::PreparedRegisterBank::Gpr,
      .register_name = "x20",
      .contiguous_width = 1,
      .occupied_register_names = {"x20"},
      .save_index = 0,
  };
  const auto saved_reg = aarch64_abi::convert_prepared_register(
      saved,
      prepare::PreparedRegisterClass::General,
      aarch64_abi::RegisterView::X);
  if (!saved_reg.has_value() || saved_reg.reg != aarch64_abi::x_register(20)) {
    return fail("expected prepared saved-register carrier to convert through its bank metadata");
  }
  return 0;
}

int fails_closed_on_unknown_spellings_and_metadata_mismatches() {
  const auto unknown = aarch64_abi::convert_prepared_register(
      "x31",
      prepare::PreparedRegisterBank::Gpr,
      prepare::PreparedRegisterClass::General,
      aarch64_abi::RegisterView::X);
  const auto bank_mismatch = aarch64_abi::convert_prepared_register(
      "x3",
      prepare::PreparedRegisterBank::Fpr,
      std::nullopt,
      aarch64_abi::RegisterView::X);
  const auto class_mismatch = aarch64_abi::convert_prepared_register(
      "d7",
      std::nullopt,
      prepare::PreparedRegisterClass::General,
      aarch64_abi::RegisterView::D);
  const auto view_mismatch = aarch64_abi::convert_prepared_register(
      "q2",
      prepare::PreparedRegisterBank::Vreg,
      prepare::PreparedRegisterClass::Vector,
      aarch64_abi::RegisterView::V);
  const auto unsupported_class_view = aarch64_abi::convert_prepared_register(
      "q8",
      std::nullopt,
      prepare::PreparedRegisterClass::Float,
      std::nullopt);
  const auto aggregate_ok = aarch64_abi::convert_prepared_register(
      "x8",
      prepare::PreparedRegisterBank::AggregateAddress,
      prepare::PreparedRegisterClass::AggregateAddress,
      aarch64_abi::RegisterView::X);

  if (!failed_with(unknown,
                   aarch64_abi::PreparedRegisterConversionErrorKind::UnknownRegisterName)) {
    return fail("expected x31 to fail closed as an unknown AArch64 prepared register");
  }
  if (!failed_with(bank_mismatch,
                   aarch64_abi::PreparedRegisterConversionErrorKind::RegisterBankMismatch)) {
    return fail("expected FPR bank metadata on x3 to fail closed as a bank mismatch");
  }
  if (!failed_with(class_mismatch,
                   aarch64_abi::PreparedRegisterConversionErrorKind::RegisterClassMismatch)) {
    return fail("expected general class metadata on d7 to fail closed as a class mismatch");
  }
  if (!failed_with(view_mismatch,
                   aarch64_abi::PreparedRegisterConversionErrorKind::RegisterViewMismatch)) {
    return fail("expected q2 with expected v view to fail closed");
  }
  if (!failed_with(unsupported_class_view,
                   aarch64_abi::PreparedRegisterConversionErrorKind::UnsupportedRegisterView)) {
    return fail("expected float class metadata on q8 to fail closed as an unsupported view");
  }
  if (!aggregate_ok.has_value() || aggregate_ok.reg != aarch64_abi::x_register(8)) {
    return fail("expected aggregate-address metadata to accept a GP physical register");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = converts_representative_gp_sp_and_fp_simd_spellings(); status != 0) {
    return status;
  }
  if (const int status = converts_prepared_assignment_and_saved_register_carriers();
      status != 0) {
    return status;
  }
  if (const int status = fails_closed_on_unknown_spellings_and_metadata_mismatches();
      status != 0) {
    return status;
  }
  return 0;
}
