#include "backend_bir_test_support.hpp"

#include "../../src/backend/bir_printer.hpp"
#include "../../src/backend/lowering/lir_to_bir.hpp"

namespace {

using I8ModuleFactory = c4c::codegen::lir::LirModule (*)();

c4c::codegen::lir::LirModule make_unsupported_local_array_gep_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.arr", "[2 x i32]", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{"%t0", "[2 x i32]", "%lv.arr", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::SExt, "i32", "0", "i64"});
  entry.insts.push_back(LirGepOp{"%t2", "i32", "%t0", false, {"i64 %t1"}});
  entry.insts.push_back(LirStoreOp{"i32", "4", "%t2"});
  entry.insts.push_back(LirGepOp{"%t3", "[2 x i32]", "%lv.arr", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t4", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t5", "i32", "%t3", false, {"i64 %t4"}});
  entry.insts.push_back(LirStoreOp{"i32", "3", "%t5"});
  entry.insts.push_back(LirLoadOp{"%t6", "i32", "%t2"});
  entry.insts.push_back(LirLoadOp{"%t7", "i32", "%t5"});
  entry.insts.push_back(LirBinOp{"%t8", "add", "i32", "%t6", "%t7"});
  entry.terminator = LirRet{std::string("%t8"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_unsupported_x86_double_return_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define double @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.terminator = LirRet{std::string("0.0"), "double"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_unsupported_aarch64_double_return_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define double @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.terminator = LirRet{std::string("0.0"), "double"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

void expect_i8_bir_route(I8ModuleFactory make_module,
                         std::string_view signature,
                         std::string_view op_text,
                         std::string_view message_prefix) {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, std::string(signature),
                  std::string(message_prefix) +
                      " should preserve the widened i8 signature on the BIR text path");
  expect_contains(rendered, std::string(op_text),
                  std::string(message_prefix) +
                      " should expose the widened i8 operation on the BIR text path");
}

void expect_i8_bir_immediate_route(I8ModuleFactory make_module,
                                   std::string_view signature,
                                   std::string_view ret_text,
                                   std::string_view message_prefix) {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, std::string(signature),
                  std::string(message_prefix) +
                      " should preserve the widened i8 signature on the BIR text path");
  expect_contains(rendered, std::string(ret_text),
                  std::string(message_prefix) +
                      " should keep the widened i8 immediate return on the BIR text path");
}

void test_backend_default_route_stays_on_bir_lir_entry_target_neutral() {
  const auto route = c4c::backend::select_lowering_route(
      c4c::backend::BackendModuleInput{make_bir_return_add_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});

  expect_true(route == c4c::backend::BackendLoweringRoute::BirFromLirEntry,
              "default backend entry should now start from the BIR lowering seam instead of routing fresh LIR input through legacy backend IR");
}

void test_backend_bir_pipeline_options_keep_bir_lir_entry_route_target_neutral() {
  const auto route = c4c::backend::select_lowering_route(
      c4c::backend::BackendModuleInput{make_bir_return_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_true(route == c4c::backend::BackendLoweringRoute::BirFromLirEntry,
              "BIR-focused backend options should keep the LIR entry seam on the BIR lowering route before any target-specific emission is observed");
}

void test_backend_direct_bir_module_route_ignores_legacy_pipeline_default_target_neutral() {
  const auto route = c4c::backend::select_lowering_route(
      c4c::backend::BackendModuleInput{make_return_immediate_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_true(route == c4c::backend::BackendLoweringRoute::BirPreloweredModule,
              "direct BIR input should remain observable as a BIR-owned route even when callers do not opt into the LIR-entry BIR pipeline");
}

void test_backend_default_path_uses_bir_when_bir_pipeline_is_not_selected() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_add_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Riscv64});

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "default backend flow should expose the supported tiny add slice on the BIR text path");
  expect_contains(rendered, "%t0 = bir.add i32 2, 3",
                  "default backend flow should lower supported LIR input directly through the BIR seam");
}

void test_backend_riscv64_path_keeps_unsupported_lir_on_llvm_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_unsupported_local_array_gep_lir_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Riscv64});

  expect_contains(rendered, "define i32 @main()",
                  "riscv64 backend text flow should keep unsupported LIR input on the LLVM text surface when shared BIR lowering cannot represent it");
  expect_not_contains(rendered, "bir.func",
                      "unsupported LIR input should not pretend to have lowered through the BIR scaffold");
}

void test_backend_riscv64_path_keeps_unsupported_direct_lir_on_llvm_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_unsupported_x86_double_return_lir_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Riscv64});

  expect_contains(rendered, "define double @main()",
                  "riscv64 backend text flow should keep unsupported direct-LIR input on the LLVM text surface even when native x86 entry rejects the same module");
  expect_not_contains(rendered, "bir.func",
                      "unsupported direct LIR should not be rendered as BIR on the riscv64 text path");
}

void test_backend_entry_rejects_unsupported_direct_lir_input_on_x86() {
  try {
    (void)c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{make_unsupported_x86_double_return_lir_module()},
        c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  } catch (const std::invalid_argument& ex) {
    expect_contains(ex.what(), "direct LIR module",
                    "backend entry should now expose the native x86 direct-LIR subset rejection instead of rescuing unsupported input through LLVM text");
    expect_not_contains(ex.what(), "legacy backend IR",
                        "backend entry should not mention the deleted legacy backend IR route when x86 rejects unsupported direct LIR");
    return;
  }

  fail("backend entry should reject unsupported x86 direct LIR input once the backend.cpp LLVM rescue seam is removed");
}

void test_backend_entry_rejects_unsupported_direct_lir_input_on_aarch64() {
  try {
    (void)c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{make_unsupported_aarch64_double_return_lir_module()},
        c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  } catch (const std::invalid_argument& ex) {
    expect_contains(ex.what(), "direct LIR module",
                    "backend entry should now expose the native aarch64 direct-LIR subset rejection instead of rescuing unsupported input through LLVM text");
    expect_not_contains(ex.what(), "legacy backend IR",
                        "backend entry should not mention the deleted legacy backend IR route when aarch64 rejects unsupported direct LIR");
    return;
  }

  fail("backend entry should reject unsupported aarch64 direct LIR input once the backend.cpp LLVM rescue seam is removed");
}

void test_backend_bir_pipeline_is_opt_in_through_backend_options() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit backend options should select the BIR pipeline");
  expect_contains(rendered, "%t0 = bir.add i32 2, 3",
                  "explicit BIR selection should route the tiny add slice through the BIR lowering seam");
}

void test_backend_bir_pipeline_accepts_direct_bir_module_input() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_return_immediate_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @tiny_ret() -> i32 {",
                  "backend input should accept a direct BIR module without forcing a legacy LIR wrapper");
  expect_contains(rendered, "bir.ret i32 7",
                  "direct BIR input should flow through the BIR text surface unchanged");
}

void test_backend_bir_pipeline_routes_sub_cluster_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should keep the narrow sub slice on the BIR path");
  expect_contains(rendered, "%t0 = bir.sub i32 3, 3",
                  "explicit BIR selection should expose sub through the BIR-specific route surface");
}

void test_backend_bir_pipeline_routes_sext_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_sext_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @widen_signed(i32 %p.x) -> i64 {",
                  "explicit BIR selection should preserve the widened sext-return signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.sext i32 %p.x to i64",
                  "explicit BIR selection should expose the straight-line sext slice on the BIR text path");
  expect_contains(rendered, "bir.ret i64 %t0",
                  "explicit BIR selection should keep the sext result on the BIR text path");
}

void test_backend_bir_pipeline_routes_zext_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_zext_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @widen_unsigned(i8 %p.x) -> i32 {",
                  "explicit BIR selection should preserve the bounded zext-return signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.zext i8 %p.x to i32",
                  "explicit BIR selection should expose the straight-line zext slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the zext result on the BIR text path");
}

void test_backend_bir_pipeline_routes_trunc_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_trunc_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @narrow_signed(i64 %p.x) -> i32 {",
                  "explicit BIR selection should preserve the bounded trunc-return signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.trunc i64 %p.x to i32",
                  "explicit BIR selection should expose the straight-line trunc slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the trunc result on the BIR text path");
}

void test_backend_bir_pipeline_routes_straight_line_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "%t0 = bir.add i32 2, 3",
                  "explicit BIR selection should preserve the head of the straight-line arithmetic chain");
  expect_contains(rendered, "%t1 = bir.sub i32 %t0, 1",
                  "explicit BIR selection should let later chain instructions consume prior BIR values");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should expose the chain tail on the BIR route surface");
}

void test_backend_bir_pipeline_routes_zero_param_staged_constant_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_staged_constant_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the zero-parameter staged constant signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.add i32 2, 3",
                  "explicit BIR selection should expose the staged constant head on the BIR text path");
  expect_contains(rendered, "%t1 = bir.sub i32 %t0, 1",
                  "explicit BIR selection should preserve the middle subtraction of the staged constant chain");
  expect_contains(rendered, "%t2 = bir.add i32 %t1, 4",
                  "explicit BIR selection should preserve the tail add of the staged constant chain");
}

void test_backend_bir_pipeline_routes_mul_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_mul_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny mul signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.mul i32 6, 7",
                  "explicit BIR selection should expose the widened mul slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the mul result on the BIR text path");
}

void test_backend_bir_pipeline_routes_and_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_and_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny and signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.and i32 14, 11",
                  "explicit BIR selection should expose the widened and slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the and result on the BIR text path");
}

void test_backend_bir_pipeline_routes_or_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_or_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny or signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.or i32 12, 3",
                  "explicit BIR selection should expose the widened or slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the or result on the BIR text path");
}

void test_backend_bir_pipeline_routes_xor_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_xor_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny xor signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.xor i32 12, 10",
                  "explicit BIR selection should expose the widened xor slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the xor result on the BIR text path");
}

void test_backend_bir_pipeline_routes_shl_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_shl_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny shl signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.shl i32 3, 4",
                  "explicit BIR selection should expose the widened shl slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the shl result on the BIR text path");
}

void test_backend_bir_pipeline_routes_lshr_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_lshr_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny lshr signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.lshr i32 16, 2",
                  "explicit BIR selection should expose the widened lshr slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the lshr result on the BIR text path");
}

void test_backend_bir_pipeline_routes_ashr_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_ashr_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny ashr signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.ashr i32 -16, 2",
                  "explicit BIR selection should expose the widened ashr slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the ashr result on the BIR text path");
}

void test_backend_bir_pipeline_routes_sdiv_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_sdiv_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny signed-division signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.sdiv i32 12, 3",
                  "explicit BIR selection should expose the widened signed-division slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the signed-division result on the BIR text path");
}

void test_backend_bir_pipeline_routes_udiv_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_udiv_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny unsigned-division signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.udiv i32 12, 3",
                  "explicit BIR selection should expose the widened unsigned-division slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the unsigned-division result on the BIR text path");
}

void test_backend_bir_pipeline_routes_srem_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_srem_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny signed-remainder signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.srem i32 14, 5",
                  "explicit BIR selection should expose the widened signed-remainder slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the signed-remainder result on the BIR text path");
}

void test_backend_bir_pipeline_routes_urem_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_urem_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny unsigned-remainder signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.urem i32 14, 5",
                  "explicit BIR selection should expose the widened unsigned-remainder slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the unsigned-remainder result on the BIR text path");
}

void test_backend_bir_pipeline_routes_eq_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_eq_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny compare-return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.eq i32 7, 7",
                  "explicit BIR selection should expose the bounded compare materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the widened compare result on the BIR text path");
}

void test_backend_bir_pipeline_routes_ne_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_ne_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny inequality-return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.ne i32 7, 3",
                  "explicit BIR selection should expose the bounded inequality materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the widened inequality result on the BIR text path");
}

void test_backend_bir_pipeline_routes_slt_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_slt_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny signed relational compare-return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.slt i32 3, 7",
                  "explicit BIR selection should expose the bounded signed relational compare materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the widened signed relational compare result on the BIR text path");
}

void test_backend_bir_pipeline_routes_sle_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_sle_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny signed less-than-or-equal compare-return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.sle i32 7, 7",
                  "explicit BIR selection should expose the bounded signed less-than-or-equal compare materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the widened signed less-than-or-equal compare result on the BIR text path");
}

void test_backend_bir_pipeline_routes_sgt_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_sgt_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny signed greater-than compare-return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.sgt i32 7, 3",
                  "explicit BIR selection should expose the bounded signed greater-than compare materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the widened signed greater-than compare result on the BIR text path");
}

void test_backend_bir_pipeline_routes_sge_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_sge_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny signed greater-than-or-equal compare-return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.sge i32 7, 7",
                  "explicit BIR selection should expose the bounded signed greater-than-or-equal compare materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the widened signed greater-than-or-equal compare result on the BIR text path");
}

void test_backend_bir_pipeline_routes_ult_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_ult_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny unsigned relational compare-return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.ult i32 3, 7",
                  "explicit BIR selection should expose the bounded unsigned relational compare materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the widened unsigned relational compare result on the BIR text path");
}

void test_backend_bir_pipeline_routes_ule_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_ule_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny unsigned less-than-or-equal compare-return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.ule i32 7, 7",
                  "explicit BIR selection should expose the bounded unsigned less-than-or-equal compare materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the widened unsigned less-than-or-equal compare result on the BIR text path");
}

void test_backend_bir_pipeline_routes_ugt_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_ugt_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny unsigned greater-than compare-return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.ugt i32 7, 3",
                  "explicit BIR selection should expose the bounded unsigned greater-than compare materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the widened unsigned greater-than compare result on the BIR text path");
}

void test_backend_bir_pipeline_routes_uge_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_uge_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny unsigned greater-than-or-equal compare-return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.uge i32 7, 7",
                  "explicit BIR selection should expose the bounded unsigned greater-than-or-equal compare materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the widened unsigned greater-than-or-equal compare result on the BIR text path");
}

void test_backend_bir_pipeline_routes_select_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_select_eq_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny compare-fed select return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.select eq i32 7, 7, 11, 4",
                  "explicit BIR selection should expose the bounded select materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the fused select result on the BIR text path");
}

void test_backend_bir_pipeline_routes_single_param_select_branch_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_single_param_select_eq_branch_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose(i32 %p.x) -> i32 {",
                  "explicit BIR selection should preserve the one-parameter ternary signature on the BIR text path");
  expect_contains(rendered, "%t.select = bir.select eq i32 %p.x, 7, 11, 4",
                  "explicit BIR selection should collapse the bounded branch-return ternary into the BIR select surface");
  expect_contains(rendered, "bir.ret i32 %t.select",
                  "explicit BIR selection should keep the fused select result on the BIR text path");
}

void test_backend_bir_pipeline_routes_single_param_select_phi_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_single_param_select_eq_phi_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose(i32 %p.x) -> i32 {",
                  "explicit BIR selection should preserve the one-parameter phi-join ternary signature on the BIR text path");
  expect_contains(rendered, "%t8 = bir.select eq i32 %p.x, 7, 11, 4",
                  "explicit BIR selection should collapse the empty branch-only goto chain plus phi join into the BIR select surface");
  expect_contains(rendered, "bir.ret i32 %t8",
                  "explicit BIR selection should keep the fused select result on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_phi_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_select_eq_phi_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose2(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the two-parameter phi-join ternary signature on the BIR text path");
  expect_contains(rendered, "%t8 = bir.select eq i32 %p.x, %p.y, %p.x, %p.y",
                  "explicit BIR selection should collapse the two-parameter phi-join ternary into the BIR select surface");
  expect_contains(rendered, "bir.ret i32 %t8",
                  "explicit BIR selection should keep the fused two-parameter select result on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_predecessor_add_phi_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_select_eq_predecessor_add_phi_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose2_add(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the widened two-parameter predecessor-compute ternary signature on the BIR text path");
  expect_contains(rendered, "%t3 = bir.add i32 %p.x, 5",
                  "explicit BIR selection should keep the then-arm predecessor arithmetic on the BIR text path");
  expect_contains(rendered, "%t4 = bir.add i32 %p.y, 9",
                  "explicit BIR selection should keep the else-arm predecessor arithmetic on the BIR text path");
  expect_contains(rendered, "%t5 = bir.select eq i32 %p.x, %p.y, %t3, %t4",
                  "explicit BIR selection should collapse the predecessor-compute phi join into the bounded BIR select surface");
  expect_contains(rendered, "bir.ret i32 %t5",
                  "explicit BIR selection should keep the fused predecessor-compute select result on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_add_phi_post_join_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_add_phi_post_join_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose2_add_post(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the widened split-predecessor ternary signature on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 5",
                  "explicit BIR selection should keep the then-arm predecessor arithmetic on the BIR text path even when it is separated from the phi by an empty end block");
  expect_contains(rendered, "%t9 = bir.add i32 %p.y, 9",
                  "explicit BIR selection should keep the else-arm predecessor arithmetic on the BIR text path even when it is separated from the phi by an empty end block");
  expect_contains(rendered, "%t10 = bir.select eq i32 %p.x, %p.y, %t8, %t9",
                  "explicit BIR selection should collapse the split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t11 = bir.add i32 %t10, 6",
                  "explicit BIR selection should preserve the bounded post-join add after the fused select for the split-predecessor form");
  expect_contains(rendered, "bir.ret i32 %t11",
                  "explicit BIR selection should keep the split-predecessor join-local arithmetic result on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_add_phi_post_join_add_sub_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_add_phi_post_join_add_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered,
                  "bir.func @choose2_add_post_chain(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the widened simple split-predecessor ternary signature while extending the join-local arithmetic tail to the nearby + 6 - 2 parity shape on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 5",
                  "explicit BIR selection should keep the then-arm predecessor arithmetic for the simple split-predecessor family on the BIR text path");
  expect_contains(rendered, "%t9 = bir.add i32 %p.y, 9",
                  "explicit BIR selection should keep the else-arm predecessor arithmetic for the simple split-predecessor family on the BIR text path");
  expect_contains(rendered, "%t10 = bir.select eq i32 %p.x, %p.y, %t8, %t9",
                  "explicit BIR selection should collapse the simple split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t11 = bir.add i32 %t10, 6",
                  "explicit BIR selection should preserve the first join-local arithmetic step after the simple split-predecessor form");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 2",
                  "explicit BIR selection should preserve the trailing join-local subtraction after the fused select for the simple split-predecessor family");
  expect_contains(rendered, "bir.ret i32 %t12",
                  "explicit BIR selection should keep the widened simple split-predecessor join-local add/sub chain on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_add_phi_post_join_add_sub_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_add_phi_post_join_add_sub_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered,
                  "bir.func @choose2_add_post_chain_tail(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the simple split-predecessor ternary signature while extending the join-local arithmetic tail to the nearby + 6 - 2 + 9 parity shape on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 5",
                  "explicit BIR selection should keep the then-arm predecessor arithmetic for the simple split-predecessor tail-extension slice on the BIR text path");
  expect_contains(rendered, "%t9 = bir.add i32 %p.y, 9",
                  "explicit BIR selection should keep the else-arm predecessor arithmetic for the simple split-predecessor tail-extension slice on the BIR text path");
  expect_contains(rendered, "%t10 = bir.select eq i32 %p.x, %p.y, %t8, %t9",
                  "explicit BIR selection should collapse the simple split-predecessor phi join into the bounded BIR select surface before the longer join-local tail");
  expect_contains(rendered, "%t11 = bir.add i32 %t10, 6",
                  "explicit BIR selection should preserve the first join-local arithmetic step after the simple split-predecessor form");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 2",
                  "explicit BIR selection should preserve the middle join-local subtraction after the fused select for the simple split-predecessor family");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 9",
                  "explicit BIR selection should preserve the trailing join-local add after the fused select for the simple split-predecessor family");
  expect_contains(rendered, "bir.ret i32 %t13",
                  "explicit BIR selection should keep the widened simple split-predecessor join-local add/sub/add chain on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_affine_phi_post_join_add_sub_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_deeper_affine_phi_post_join_add_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered,
                  "bir.func @choose2_deeper_both_post_chain(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the symmetric deeper split-predecessor ternary signature while widening the join-local arithmetic tail on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the deeper split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the deeper split then-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %t9, 5",
                  "explicit BIR selection should keep the deeper split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t11 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the deeper split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 4",
                  "explicit BIR selection should keep the deeper split else-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 7",
                  "explicit BIR selection should keep the deeper split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t14 = bir.select eq i32 %p.x, %p.y, %t10, %t13",
                  "explicit BIR selection should collapse the symmetric deeper split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t15 = bir.add i32 %t14, 6",
                  "explicit BIR selection should preserve the first join-local arithmetic step after the symmetric deeper split-predecessor form");
  expect_contains(rendered, "%t16 = bir.sub i32 %t15, 2",
                  "explicit BIR selection should preserve the trailing join-local subtraction after the fused select");
  expect_contains(rendered, "bir.ret i32 %t16",
                  "explicit BIR selection should keep the widened join-local add/sub chain on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_affine_phi_post_join_add_sub_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_mixed_affine_phi_post_join_add_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose2_mixed_post_chain(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the split-predecessor mixed-affine ternary signature while widening the join-local arithmetic tail on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t11 = bir.sub i32 %t10, 4",
                  "explicit BIR selection should keep the split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t12 = bir.select eq i32 %p.x, %p.y, %t9, %t11",
                  "explicit BIR selection should collapse the mixed split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 6",
                  "explicit BIR selection should preserve the first join-local arithmetic step after the mixed split-predecessor form");
  expect_contains(rendered, "%t14 = bir.sub i32 %t13, 2",
                  "explicit BIR selection should preserve the trailing join-local subtraction after the fused select");
  expect_contains(rendered, "bir.ret i32 %t14",
                  "explicit BIR selection should keep the widened mixed split-predecessor join-local add/sub chain on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_affine_phi_post_join_add_sub_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_mixed_affine_phi_post_join_add_sub_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered,
                  "bir.func @choose2_mixed_post_chain_tail(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the split-predecessor mixed-affine ternary signature while extending the join-local arithmetic tail to the nearby + 6 - 2 + 9 parity shape on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t11 = bir.sub i32 %t10, 4",
                  "explicit BIR selection should keep the split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t12 = bir.select eq i32 %p.x, %p.y, %t9, %t11",
                  "explicit BIR selection should collapse the mixed split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 6",
                  "explicit BIR selection should preserve the first join-local arithmetic step after the mixed split-predecessor form");
  expect_contains(rendered, "%t14 = bir.sub i32 %t13, 2",
                  "explicit BIR selection should preserve the middle join-local subtraction after the fused select");
  expect_contains(rendered, "%t15 = bir.add i32 %t14, 9",
                  "explicit BIR selection should preserve the trailing join-local add after the fused select");
  expect_contains(rendered, "bir.ret i32 %t15",
                  "explicit BIR selection should keep the widened mixed split-predecessor join-local add/sub/add chain on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_affine_phi_post_join_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_mixed_affine_phi_post_join_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose2_mixed_post(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the richer split-predecessor ternary signature on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t11 = bir.sub i32 %t10, 4",
                  "explicit BIR selection should keep the split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t12 = bir.select eq i32 %p.x, %p.y, %t9, %t11",
                  "explicit BIR selection should collapse the richer split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 6",
                  "explicit BIR selection should preserve the bounded post-join add after the richer split-predecessor form");
  expect_contains(rendered, "bir.ret i32 %t13",
                  "explicit BIR selection should keep the richer split-predecessor join-local arithmetic result on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose2_deeper_post(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the next richer split-predecessor ternary signature on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the deeper split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the deeper split then-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %t9, 5",
                  "explicit BIR selection should keep the deeper split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t11 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the mixed split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 4",
                  "explicit BIR selection should keep the mixed split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t13 = bir.select eq i32 %p.x, %p.y, %t10, %t12",
                  "explicit BIR selection should collapse the next richer split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t14 = bir.add i32 %t13, 6",
                  "explicit BIR selection should preserve the bounded post-join add after the next richer split-predecessor form");
  expect_contains(rendered, "bir.ret i32 %t14",
                  "explicit BIR selection should keep the next richer split-predecessor join-local arithmetic result on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered,
                  "bir.func @choose2_deeper_post_chain(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the asymmetric deeper-then split-predecessor ternary signature while widening the join-local arithmetic tail on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the deeper split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the deeper split then-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %t9, 5",
                  "explicit BIR selection should keep the deeper split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t11 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the mixed split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 4",
                  "explicit BIR selection should keep the mixed split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t13 = bir.select eq i32 %p.x, %p.y, %t10, %t12",
                  "explicit BIR selection should collapse the asymmetric deeper-then split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t14 = bir.add i32 %t13, 6",
                  "explicit BIR selection should preserve the first join-local arithmetic step after the asymmetric deeper-then split-predecessor form");
  expect_contains(rendered, "%t15 = bir.sub i32 %t14, 2",
                  "explicit BIR selection should preserve the trailing join-local subtraction after the fused select");
  expect_contains(rendered, "bir.ret i32 %t15",
                  "explicit BIR selection should keep the widened asymmetric deeper-then join-local add/sub chain on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(
      rendered,
      "bir.func @choose2_deeper_post_chain_tail(i32 %p.x, i32 %p.y) -> i32 {",
      "explicit BIR selection should preserve the asymmetric deeper-then split-predecessor ternary signature while extending the join-local affine tail by one more bounded step on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the deeper split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the deeper split then-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %t9, 5",
                  "explicit BIR selection should keep the deeper split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t11 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the mixed split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 4",
                  "explicit BIR selection should keep the mixed split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t13 = bir.select eq i32 %p.x, %p.y, %t10, %t12",
                  "explicit BIR selection should collapse the asymmetric deeper-then split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t14 = bir.add i32 %t13, 6",
                  "explicit BIR selection should preserve the first join-local arithmetic step after the asymmetric deeper-then split-predecessor form");
  expect_contains(rendered, "%t15 = bir.sub i32 %t14, 2",
                  "explicit BIR selection should preserve the second join-local arithmetic step after the fused select");
  expect_contains(rendered, "%t16 = bir.add i32 %t15, 9",
                  "explicit BIR selection should preserve the added third join-local arithmetic step after the fused select");
  expect_contains(rendered, "bir.ret i32 %t16",
                  "explicit BIR selection should keep the extended asymmetric deeper-then join-local arithmetic result on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(
      rendered,
      "bir.func @choose2_mixed_then_deeper_post(i32 %p.x, i32 %p.y) -> i32 {",
      "explicit BIR selection should preserve the mirrored asymmetric deeper-else split-predecessor ternary signature on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the mixed split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the mixed split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the deeper split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t11 = bir.sub i32 %t10, 4",
                  "explicit BIR selection should keep the deeper split else-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t12 = bir.add i32 %t11, 7",
                  "explicit BIR selection should keep the deeper split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t13 = bir.select eq i32 %p.x, %p.y, %t9, %t12",
                  "explicit BIR selection should collapse the mirrored asymmetric split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t14 = bir.add i32 %t13, 6",
                  "explicit BIR selection should preserve the bounded post-join add after the mirrored asymmetric split-predecessor form");
  expect_contains(rendered, "bir.ret i32 %t14",
                  "explicit BIR selection should keep the mirrored asymmetric deeper-else join-local arithmetic result on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(
      rendered,
      "bir.func @choose2_mixed_then_deeper_post_chain(i32 %p.x, i32 %p.y) -> i32 {",
      "explicit BIR selection should preserve the mirrored asymmetric deeper-else split-predecessor ternary signature while widening the join-local arithmetic tail on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the mixed split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the mixed split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the deeper split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t11 = bir.sub i32 %t10, 4",
                  "explicit BIR selection should keep the deeper split else-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t12 = bir.add i32 %t11, 7",
                  "explicit BIR selection should keep the deeper split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t13 = bir.select eq i32 %p.x, %p.y, %t9, %t12",
                  "explicit BIR selection should collapse the mirrored asymmetric split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t14 = bir.add i32 %t13, 6",
                  "explicit BIR selection should preserve the first join-local arithmetic step after the mirrored asymmetric split-predecessor form");
  expect_contains(rendered, "%t15 = bir.sub i32 %t14, 2",
                  "explicit BIR selection should preserve the trailing join-local subtraction after the fused select");
  expect_contains(rendered, "bir.ret i32 %t15",
                  "explicit BIR selection should keep the widened mirrored asymmetric deeper-else join-local add/sub chain on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(
      rendered,
      "bir.func @choose2_mixed_then_deeper_post_chain_tail(i32 %p.x, i32 %p.y) -> i32 {",
      "explicit BIR selection should preserve the mirrored asymmetric deeper-else split-predecessor ternary signature while extending the join-local affine tail by one more bounded step on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the mixed split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the mixed split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the deeper split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t11 = bir.sub i32 %t10, 4",
                  "explicit BIR selection should keep the deeper split else-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t12 = bir.add i32 %t11, 7",
                  "explicit BIR selection should keep the deeper split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t13 = bir.select eq i32 %p.x, %p.y, %t9, %t12",
                  "explicit BIR selection should collapse the mirrored asymmetric split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t14 = bir.add i32 %t13, 6",
                  "explicit BIR selection should preserve the first join-local arithmetic step after the mirrored asymmetric split-predecessor form");
  expect_contains(rendered, "%t15 = bir.sub i32 %t14, 2",
                  "explicit BIR selection should preserve the second join-local arithmetic step after the fused select");
  expect_contains(rendered, "%t16 = bir.add i32 %t15, 9",
                  "explicit BIR selection should preserve the added third join-local arithmetic step after the fused select");
  expect_contains(rendered, "bir.ret i32 %t16",
                  "explicit BIR selection should keep the extended mirrored asymmetric deeper-else join-local arithmetic result on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_affine_phi_post_join_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_deeper_affine_phi_post_join_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose2_deeper_both_post(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the symmetric deeper split-predecessor ternary signature on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the deeper split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the deeper split then-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %t9, 5",
                  "explicit BIR selection should keep the deeper split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t11 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the deeper split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 4",
                  "explicit BIR selection should keep the deeper split else-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 7",
                  "explicit BIR selection should keep the deeper split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t14 = bir.select eq i32 %p.x, %p.y, %t10, %t13",
                  "explicit BIR selection should collapse the symmetric deeper split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t15 = bir.add i32 %t14, 6",
                  "explicit BIR selection should preserve the bounded post-join add after the symmetric deeper split-predecessor form");
  expect_contains(rendered, "bir.ret i32 %t15",
                  "explicit BIR selection should keep the symmetric deeper split-predecessor join-local arithmetic result on the BIR text path");
}

void test_backend_bir_pipeline_routes_mixed_predecessor_select_post_join_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_mixed_predecessor_add_phi_post_join_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_mixed_add() -> i32 {",
                  "explicit BIR selection should preserve the bounded mixed predecessor/immediate conditional signature on the BIR text path");
  expect_contains(rendered, "%t3 = bir.add i32 7, 5",
                  "explicit BIR selection should keep the computed predecessor input on the fused BIR block");
  expect_contains(rendered, "%t4 = bir.select slt i32 2, 3, %t3, 9",
                  "explicit BIR selection should collapse the mixed predecessor/immediate phi join into a bounded BIR select");
  expect_contains(rendered, "%t5 = bir.add i32 %t4, 6",
                  "explicit BIR selection should preserve the bounded post-join add on the fused select result");
  expect_contains(rendered, "bir.ret i32 %t5",
                  "explicit BIR selection should keep the join-local arithmetic result on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_affine_phi_post_join_add_sub_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_deeper_affine_phi_post_join_add_sub_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(
      rendered,
      "bir.func @choose2_deeper_both_post_chain_tail(i32 %p.x, i32 %p.y) -> i32 {",
      "explicit BIR selection should preserve the symmetric deeper split-predecessor ternary signature while extending the join-local affine tail by one more bounded step on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the deeper split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the deeper split then-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %t9, 5",
                  "explicit BIR selection should keep the deeper split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t11 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the deeper split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 4",
                  "explicit BIR selection should keep the deeper split else-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 7",
                  "explicit BIR selection should keep the deeper split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t14 = bir.select eq i32 %p.x, %p.y, %t10, %t13",
                  "explicit BIR selection should collapse the symmetric deeper split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t15 = bir.add i32 %t14, 6",
                  "explicit BIR selection should preserve the first join-local arithmetic step after the fused select");
  expect_contains(rendered, "%t16 = bir.sub i32 %t15, 2",
                  "explicit BIR selection should preserve the second join-local arithmetic step after the fused select");
  expect_contains(rendered, "%t17 = bir.add i32 %t16, 9",
                  "explicit BIR selection should preserve the added third join-local arithmetic step after the fused select");
  expect_contains(rendered, "bir.ret i32 %t17",
                  "explicit BIR selection should keep the extended join-local arithmetic result on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @tiny_char(i8 %p.x) -> i8 {",
                  "explicit BIR selection should preserve widened i8 function signatures on the BIR text path");
  expect_contains(rendered, "%t0 = bir.add i8 %p.x, 2",
                  "explicit BIR selection should route i8 arithmetic through the BIR text surface");
  expect_contains(rendered, "%t1 = bir.sub i8 %t0, 1",
                  "explicit BIR selection should preserve the trailing i8 adjustment on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_two_param_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_two_param_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @add_pair_u(i8 %p.x, i8 %p.y) -> i8 {",
                  "explicit BIR selection should preserve the widened i8 two-parameter signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.add i8 %p.x, %p.y",
                  "explicit BIR selection should route widened i8 two-parameter arithmetic through the BIR surface");
  expect_contains(rendered, "bir.ret i8 %t0",
                  "explicit BIR selection should keep the widened i8 two-parameter result on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_eq_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_eq_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_eq_u() -> i8 {",
                  "explicit BIR selection should preserve the widened i8 equality signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.eq i8 7, 7",
                  "explicit BIR selection should expose the widened i8 equality compare materialization on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_ne_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_ne_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_ne_u() -> i8 {",
                  "explicit BIR selection should preserve the widened i8 inequality signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.ne i8 7, 3",
                  "explicit BIR selection should expose the widened i8 inequality compare materialization on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_ult_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_ult_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_ult_u() -> i8 {",
                  "explicit BIR selection should preserve the widened i8 unsigned-less-than signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.ult i8 3, 7",
                  "explicit BIR selection should expose the widened i8 unsigned-less-than compare materialization on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_ule_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_ule_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_ule_u() -> i8 {",
                  "explicit BIR selection should preserve the widened i8 unsigned-less-than-or-equal signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.ule i8 7, 7",
                  "explicit BIR selection should expose the widened i8 unsigned-less-than-or-equal compare materialization on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_ugt_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_ugt_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_ugt_u() -> i8 {",
                  "explicit BIR selection should preserve the widened i8 unsigned-greater-than signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.ugt i8 7, 3",
                  "explicit BIR selection should expose the widened i8 unsigned-greater-than compare materialization on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_uge_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_uge_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_uge_u() -> i8 {",
                  "explicit BIR selection should preserve the widened i8 unsigned-greater-than-or-equal signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.uge i8 7, 7",
                  "explicit BIR selection should expose the widened i8 unsigned-greater-than-or-equal compare materialization on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_slt_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_slt_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_slt_u() -> i8 {",
                  "explicit BIR selection should preserve the widened i8 signed-less-than signature on the BIR text path");
  expect_contains(rendered, "%t5 = bir.slt i8 3, 7",
                  "explicit BIR selection should expose the widened i8 signed-less-than compare materialization on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_sle_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_sle_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_sle_u() -> i8 {",
                  "explicit BIR selection should preserve the widened i8 signed-less-than-or-equal signature on the BIR text path");
  expect_contains(rendered, "%t5 = bir.sle i8 7, 7",
                  "explicit BIR selection should expose the widened i8 signed-less-than-or-equal compare materialization on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_sgt_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_sgt_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_sgt_u() -> i8 {",
                  "explicit BIR selection should preserve the widened i8 signed-greater-than signature on the BIR text path");
  expect_contains(rendered, "%t5 = bir.sgt i8 7, 3",
                  "explicit BIR selection should expose the widened i8 signed-greater-than compare materialization on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_sge_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_sge_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_sge_u() -> i8 {",
                  "explicit BIR selection should preserve the widened i8 signed-greater-than-or-equal signature on the BIR text path");
  expect_contains(rendered, "%t5 = bir.sge i8 7, 7",
                  "explicit BIR selection should expose the widened i8 signed-greater-than-or-equal compare materialization on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_add_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_add_module,
                      "bir.func @choose_add_u() -> i8 {",
                      "%t0 = bir.add i8 2, 3",
                      "explicit BIR selection for widened i8 add");
}

void test_backend_bir_pipeline_routes_i8_sub_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_sub_module,
                      "bir.func @choose_sub_u() -> i8 {",
                      "%t0 = bir.sub i8 9, 4",
                      "explicit BIR selection for widened i8 sub");
}

void test_backend_bir_pipeline_routes_i8_mul_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_mul_module,
                      "bir.func @choose_mul_u() -> i8 {",
                      "%t0 = bir.mul i8 6, 7",
                      "explicit BIR selection for widened i8 mul");
}

void test_backend_bir_pipeline_routes_i8_and_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_and_module,
                      "bir.func @choose_and_u() -> i8 {",
                      "%t0 = bir.and i8 14, 11",
                      "explicit BIR selection for widened i8 and");
}

void test_backend_bir_pipeline_routes_i8_or_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_or_module,
                      "bir.func @choose_or_u() -> i8 {",
                      "%t0 = bir.or i8 12, 3",
                      "explicit BIR selection for widened i8 or");
}

void test_backend_bir_pipeline_routes_i8_xor_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_xor_module,
                      "bir.func @choose_xor_u() -> i8 {",
                      "%t0 = bir.xor i8 12, 10",
                      "explicit BIR selection for widened i8 xor");
}

void test_backend_bir_pipeline_routes_i8_shl_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_shl_module,
                      "bir.func @choose_shl_u() -> i8 {",
                      "%t0 = bir.shl i8 3, 4",
                      "explicit BIR selection for widened i8 shl");
}

void test_backend_bir_pipeline_routes_i8_lshr_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_lshr_module,
                      "bir.func @choose_lshr_u() -> i8 {",
                      "%t0 = bir.lshr i8 16, 2",
                      "explicit BIR selection for widened i8 lshr");
}

void test_backend_bir_pipeline_routes_i8_ashr_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_ashr_module,
                      "bir.func @choose_ashr_u() -> i8 {",
                      "%t0 = bir.ashr i8 -16, 2",
                      "explicit BIR selection for widened i8 ashr");
}

void test_backend_bir_pipeline_routes_i8_sdiv_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_sdiv_module,
                      "bir.func @choose_sdiv_u() -> i8 {",
                      "%t0 = bir.sdiv i8 12, 3",
                      "explicit BIR selection for widened i8 sdiv");
}

void test_backend_bir_pipeline_routes_i8_udiv_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_udiv_module,
                      "bir.func @choose_udiv_u() -> i8 {",
                      "%t0 = bir.udiv i8 12, 3",
                      "explicit BIR selection for widened i8 udiv");
}

void test_backend_bir_pipeline_routes_i8_srem_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_srem_module,
                      "bir.func @choose_srem_u() -> i8 {",
                      "%t0 = bir.srem i8 14, 5",
                      "explicit BIR selection for widened i8 srem");
}

void test_backend_bir_pipeline_routes_i8_urem_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_urem_module,
                      "bir.func @choose_urem_u() -> i8 {",
                      "%t0 = bir.urem i8 14, 5",
                      "explicit BIR selection for widened i8 urem");
}

void test_backend_bir_pipeline_routes_i8_immediate_through_bir_text_surface() {
  expect_i8_bir_immediate_route(make_bir_i8_return_immediate_module,
                                "bir.func @choose_const_u() -> i8 {",
                                "bir.ret i8 11",
                                "explicit BIR selection for widened i8 immediate");
}

void test_backend_bir_pipeline_routes_i64_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i64_return_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @wide_add(i64 %p.x) -> i64 {",
                  "explicit BIR selection should preserve widened i64 function signatures on the BIR text path");
  expect_contains(rendered, "%t0 = bir.add i64 %p.x, 2",
                  "explicit BIR selection should route i64 arithmetic through the BIR text surface");
  expect_contains(rendered, "%t1 = bir.sub i64 %t0, 1",
                  "explicit BIR selection should preserve the trailing i64 adjustment on the BIR text path");
}

void test_backend_bir_pipeline_routes_single_param_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_single_param_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @add_one(i32 %p.x) -> i32 {",
                  "explicit BIR selection should preserve the bounded one-parameter signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.add i32 %p.x, 2",
                  "explicit BIR selection should route parameter-fed arithmetic through the BIR surface");
  expect_contains(rendered, "%t1 = bir.sub i32 %t0, 1",
                  "explicit BIR selection should keep the bounded parameter-fed chain in BIR text form");
}

void test_backend_bir_pipeline_routes_two_param_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @add_pair(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the bounded two-parameter signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.add i32 %p.x, %p.y",
                  "explicit BIR selection should route two-parameter affine arithmetic through the BIR surface");
}

void test_backend_bir_pipeline_routes_two_param_add_sub_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @add_pair(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the bounded two-parameter signature for the affine chain");
  expect_contains(rendered, "%t0 = bir.add i32 %p.x, %p.y",
                  "explicit BIR selection should keep the two-parameter add head of the bounded affine chain on the BIR text path");
  expect_contains(rendered, "%t1 = bir.sub i32 %t0, 1",
                  "explicit BIR selection should expose the trailing immediate adjustment on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_staged_affine_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_staged_affine_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @add_pair(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the two-parameter signature for the staged affine chain");
  expect_contains(rendered, "%t0 = bir.add i32 %p.x, 2",
                  "explicit BIR selection should expose the staged immediate head on the BIR text path");
  expect_contains(rendered, "%t1 = bir.add i32 %t0, %p.y",
                  "explicit BIR selection should keep the later second-parameter add on the BIR text path");
  expect_contains(rendered, "%t2 = bir.sub i32 %t1, 1",
                  "explicit BIR selection should expose the trailing staged subtraction on the BIR text path");
}

void test_backend_bir_pipeline_selection_only_applies_at_lir_entry_input() {
  const auto lir_module = make_bir_return_add_module();
  const auto bir_module = c4c::backend::lower_to_bir(lir_module);
  const auto expected = c4c::backend::bir::print(bir_module);

  const auto rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{bir_module},
                                make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_true(rendered == expected,
              "explicit prelowered BIR emission should stay on the BIR-owned passthrough text surface");
  expect_contains(rendered, "bir.func",
                  "explicit prelowered BIR emission should stay on the BIR text path");
}

void test_backend_lowered_riscv_passthrough_is_detached_from_lir_metadata() {
  const auto bir_module = c4c::backend::lower_to_bir(make_bir_return_add_module());
  const auto expected = c4c::backend::bir::print(bir_module);

  const auto rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{bir_module},
                                make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_true(rendered == expected,
              "once callers emit an explicit prelowered RISC-V BIR module, the output should come only from the BIR-owned passthrough text surface");
  expect_contains(rendered, "bir.func",
                  "explicit prelowered RISC-V BIR emission should stay on the BIR text surface");
}

void test_backend_direct_bir_i686_target_uses_x86_stack_arg_emitter() {
  auto bir_module = c4c::backend::lower_to_bir(make_bir_two_param_add_module());
  bir_module.target_triple = "i686-unknown-linux-gnu";

  const auto rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{bir_module},
                                make_bir_pipeline_options(c4c::backend::Target::I686));

  expect_contains(rendered, "DWORD PTR [esp + 4]",
                  "direct i686 BIR emission should still dispatch through the x86 stack-arg path");
  expect_contains(rendered, "DWORD PTR [esp + 8]",
                  "direct i686 BIR emission should keep the second stack-arg load");
}

}  // namespace

void run_backend_bir_pipeline_tests() {
  RUN_TEST(test_backend_default_route_stays_on_bir_lir_entry_target_neutral);
  RUN_TEST(test_backend_bir_pipeline_options_keep_bir_lir_entry_route_target_neutral);
  RUN_TEST(test_backend_direct_bir_module_route_ignores_legacy_pipeline_default_target_neutral);
  RUN_TEST(test_backend_default_path_uses_bir_when_bir_pipeline_is_not_selected);
  RUN_TEST(test_backend_riscv64_path_keeps_unsupported_lir_on_llvm_text_surface);
  RUN_TEST(test_backend_riscv64_path_keeps_unsupported_direct_lir_on_llvm_text_surface);
  RUN_TEST(test_backend_entry_rejects_unsupported_direct_lir_input_on_x86);
  RUN_TEST(test_backend_entry_rejects_unsupported_direct_lir_input_on_aarch64);
  RUN_TEST(test_backend_bir_pipeline_is_opt_in_through_backend_options);
  RUN_TEST(test_backend_bir_pipeline_accepts_direct_bir_module_input);
  RUN_TEST(test_backend_bir_pipeline_routes_sub_cluster_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_sext_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_zext_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_trunc_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_straight_line_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_zero_param_staged_constant_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_mul_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_and_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_or_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_xor_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_shl_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_lshr_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_ashr_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_sdiv_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_udiv_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_srem_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_urem_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_eq_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_ne_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_slt_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_sle_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_sgt_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_sge_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_ult_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_ule_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_ugt_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_uge_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_select_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_single_param_select_branch_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_single_param_select_phi_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_phi_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_predecessor_add_phi_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_add_phi_post_join_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_add_phi_post_join_add_sub_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_add_phi_post_join_add_sub_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_affine_phi_post_join_add_sub_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_affine_phi_post_join_add_sub_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_affine_phi_post_join_add_sub_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_affine_phi_post_join_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_affine_phi_post_join_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_affine_phi_post_join_add_sub_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_mixed_predecessor_select_post_join_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_two_param_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_eq_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_ne_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_ult_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_ule_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_ugt_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_uge_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_slt_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_sle_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_sgt_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_sge_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_sub_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_mul_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_and_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_or_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_xor_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_shl_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_lshr_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_ashr_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_sdiv_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_udiv_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_srem_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_urem_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_immediate_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i64_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_single_param_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_add_sub_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_staged_affine_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_selection_only_applies_at_lir_entry_input);
  RUN_TEST(test_backend_lowered_riscv_passthrough_is_detached_from_lir_metadata);
  RUN_TEST(test_backend_direct_bir_i686_target_uses_x86_stack_arg_emitter);
}
