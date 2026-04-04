#pragma once

#include <string_view>

#include "../../src/codegen/lir/ir.hpp"

struct TestLirTargetProfile {
  std::string_view triple;
  std::string_view data_layout;
};

inline const TestLirTargetProfile& backend_test_riscv64_profile() {
  static const TestLirTargetProfile profile{
      "riscv64-unknown-linux-gnu",
      "e-m:e-p:64:64-i64:64-n32:64-S128",
  };
  return profile;
}

inline const TestLirTargetProfile& backend_test_aarch64_profile() {
  static const TestLirTargetProfile profile{
      "aarch64-unknown-linux-gnu",
      "e-m:e-i64:64-i128:128-n32:64-S128",
  };
  return profile;
}

inline const TestLirTargetProfile& backend_test_x86_64_profile() {
  static const TestLirTargetProfile profile{
      "x86_64-unknown-linux-gnu",
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-n8:16:32:64-S128",
  };
  return profile;
}

inline const TestLirTargetProfile& backend_test_x86_64_bir_pipeline_profile() {
  static const TestLirTargetProfile profile{
      "x86_64-unknown-linux-gnu",
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128",
  };
  return profile;
}

inline void apply_test_lir_target_profile(
    c4c::codegen::lir::LirModule& module,
    const TestLirTargetProfile& profile) {
  module.target_triple = profile.triple;
  module.data_layout = profile.data_layout;
}
