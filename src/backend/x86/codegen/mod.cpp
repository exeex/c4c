#include <string_view>

namespace c4c::backend::x86 {

namespace {

[[maybe_unused]] constexpr std::string_view kCodegenModuleOverview =
    "Mechanical translation of ref/claudes-c-compiler/src/backend/x86/codegen";

}  // namespace

void x86_codegen_module_anchor() {
  // The real implementation is split across alu/calls/memory/comparison and
  // the other target-local translation units in this directory.
}

}  // namespace c4c::backend::x86
