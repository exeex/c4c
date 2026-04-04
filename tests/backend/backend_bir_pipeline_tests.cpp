#include "backend_bir_test_support.hpp"

#include "../../src/backend/ir_printer.hpp"
#include "../../src/backend/lowering/lir_to_backend_ir.hpp"
#include "../../src/backend/lowering/lir_to_bir.hpp"

namespace {

void test_backend_default_route_stays_on_legacy_lir_entry_target_neutral() {
  const auto route = c4c::backend::select_lowering_route(
      c4c::backend::BackendModuleInput{make_bir_return_add_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::X86_64});

  expect_true(route == c4c::backend::BackendLoweringRoute::LegacyFromLirEntry,
              "default backend entry should stay on the legacy LIR route until callers explicitly select the BIR pipeline");
}

void test_backend_bir_pipeline_selects_bir_lir_entry_route_target_neutral() {
  const auto route = c4c::backend::select_lowering_route(
      c4c::backend::BackendModuleInput{make_bir_return_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_true(route == c4c::backend::BackendLoweringRoute::BirFromLirEntry,
              "explicit BIR backend options should switch the entry seam onto the BIR lowering route before any target-specific emission is observed");
}

void test_backend_prelowered_legacy_module_route_ignores_pipeline_override_target_neutral() {
  const auto lir_module = make_bir_return_add_module();
  const auto lowered = c4c::backend::lower_lir_to_backend_module(lir_module);
  const auto route = c4c::backend::select_lowering_route(
      c4c::backend::BackendModuleInput{lowered, &lir_module},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_true(route == c4c::backend::BackendLoweringRoute::LegacyPreloweredModule,
              "once callers provide a pre-lowered legacy backend module, pipeline selection should stop at that structured route seam instead of re-entering LIR routing");
}

void test_backend_direct_bir_module_route_ignores_legacy_pipeline_default_target_neutral() {
  const auto route = c4c::backend::select_lowering_route(
      c4c::backend::BackendModuleInput{make_return_immediate_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_true(route == c4c::backend::BackendLoweringRoute::BirPreloweredModule,
              "direct BIR input should remain observable as a BIR-owned route even when callers do not opt into the LIR-entry BIR pipeline");
}

void test_backend_default_path_remains_legacy_when_bir_pipeline_is_not_selected() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_add_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Riscv64});

  expect_contains(rendered, "define i32 @main()",
                  "default backend flow should keep returning legacy LLVM text when the BIR path is not selected");
  expect_not_contains(rendered, "bir.func",
                      "default backend flow should not silently switch into the BIR scaffold");
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
  const c4c::backend::BackendModule lowered_backend =
      c4c::backend::lower_lir_to_backend_module(lir_module);
  const auto expected = c4c::backend::print_backend_module(lowered_backend);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered_backend, &lir_module},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_true(rendered == expected,
              "once callers provide a lowered backend module, backend pipeline selection should stop at the LIR entry seam");
  expect_not_contains(rendered, "bir.func",
                      "pre-lowered backend-module entry should not re-enter the BIR text path");
}

void test_backend_lowered_riscv_passthrough_ignores_broken_legacy_fallback() {
  auto legacy_module = make_bir_return_add_module();
  const c4c::backend::BackendModule lowered_backend =
      c4c::backend::lower_lir_to_backend_module(legacy_module);
  const auto expected = c4c::backend::print_backend_module(lowered_backend);

  for (auto& function : legacy_module.functions) {
    if (function.name == "main") {
      function.name = "legacy_main_broken";
      function.signature_text = "define i32 @legacy_main_broken()\n";
      break;
    }
  }

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered_backend, &legacy_module},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_true(rendered == expected,
              "once callers provide a lowered RISC-V backend module, malformed attached legacy fallback metadata should not override the backend-owned passthrough text surface");
  expect_not_contains(rendered, "define i32 @legacy_main_broken()",
                      "pre-lowered RISC-V backend-module emission should ignore broken legacy fallback LLVM text");
}

}  // namespace

void run_backend_bir_pipeline_tests() {
  RUN_TEST(test_backend_default_route_stays_on_legacy_lir_entry_target_neutral);
  RUN_TEST(test_backend_bir_pipeline_selects_bir_lir_entry_route_target_neutral);
  RUN_TEST(test_backend_prelowered_legacy_module_route_ignores_pipeline_override_target_neutral);
  RUN_TEST(test_backend_direct_bir_module_route_ignores_legacy_pipeline_default_target_neutral);
  RUN_TEST(test_backend_default_path_remains_legacy_when_bir_pipeline_is_not_selected);
  RUN_TEST(test_backend_bir_pipeline_is_opt_in_through_backend_options);
  RUN_TEST(test_backend_bir_pipeline_accepts_direct_bir_module_input);
  RUN_TEST(test_backend_bir_pipeline_routes_sub_cluster_through_bir_text_surface);
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
  RUN_TEST(test_backend_bir_pipeline_routes_i64_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_single_param_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_add_sub_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_staged_affine_chain_through_bir_text_surface);
}
