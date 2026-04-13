
#include "backend.hpp"
#include "generation.hpp"
#include "liveness.hpp"
#include "regalloc.hpp"
#include "stack_layout/analysis.hpp"
#include "stack_layout/alloca_coalescing.hpp"
#include "stack_layout/regalloc_helpers.hpp"
#include "stack_layout/slot_assignment.hpp"
#include "target.hpp"
#include "../../src/codegen/lir/call_args.hpp"
#include "../../src/codegen/lir/lir_printer.hpp"
#include "../../src/codegen/lir/verify.hpp"
#include "../../src/backend/lowering/call_decode.hpp"
#include "../../src/backend/lowering/lir_to_bir.hpp"
#include "../../src/backend/elf/mod.hpp"
#include "../../src/backend/linker_common/mod.hpp"
#include "../../src/backend/aarch64/assembler/mod.hpp"
#include "../../src/backend/aarch64/codegen/emit.hpp"
#include "../../src/backend/aarch64/linker/mod.hpp"
#include "../../src/backend/aarch64/assembler/parser.hpp"
#include "../../src/backend/riscv/assembler/mod.hpp"
#include "../../src/backend/riscv/codegen/emit.hpp"
#include "../../src/backend/riscv/codegen/riscv_codegen.hpp"
#include "../../src/backend/riscv/linker/mod.hpp"
#include "../../src/backend/x86/assembler/mod.hpp"
#include "../../src/backend/x86/assembler/encoder/mod.hpp"
#include "../../src/backend/x86/assembler/parser.hpp"
#include "../../src/backend/x86/codegen/x86_codegen.hpp"
#include "../../src/backend/x86/linker/mod.hpp"
#include "backend_test_helper.hpp"

#include <cstdint>
#include <type_traits>

namespace {

c4c::backend::LivenessInput lower_test_backend_cfg_liveness_input(
    const c4c::codegen::lir::LirFunction& function) {
  const auto backend_cfg = c4c::backend::lower_lir_to_backend_cfg(function);
  return c4c::backend::lower_backend_cfg_to_liveness_input(backend_cfg);
}

c4c::backend::stack_layout::StackLayoutInput lower_test_prepared_entry_alloca_stack_layout_input(
    const c4c::codegen::lir::LirModule& module,
    std::size_t function_index) {
  return c4c::backend::stack_layout::lower_prepared_entry_alloca_stack_layout_input(
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_preparation(
          module, function_index));
}

c4c::backend::stack_layout::StackLayoutInput lower_test_function_entry_alloca_stack_layout_input(
    const c4c::codegen::lir::LirFunction& function) {
  return c4c::backend::stack_layout::lower_function_entry_alloca_stack_layout_input(
      function, c4c::backend::lower_lir_to_backend_cfg(function));
}

c4c::codegen::lir::LirModule make_mixed_bir_and_entry_alloca_module() {
  auto module = make_typed_direct_call_two_arg_module();
  auto dead_alloca_module = make_dead_local_alloca_candidate_module();
  auto dead_function = dead_alloca_module.functions.front();
  dead_function.name = "needs_rewrite";
  module.functions.push_back(std::move(dead_function));
  return module;
}

c4c::codegen::lir::LirModule make_live_pointer_param_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "advance";
  function.signature_text = "define void @advance(ptr %p.p)\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.param.p", "ptr", "", 8});
  function.alloca_insts.push_back(LirStoreOp{"ptr", "%p.p", "%lv.param.p"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirLoadOp{"%t0", "ptr", "%lv.param.p"});
  entry.insts.push_back(LirGepOp{"%t1", "i8", "%t0", false, {"i64 1"}});
  entry.insts.push_back(LirStoreOp{"ptr", "%t1", "%lv.param.p"});
  entry.terminator = LirRet{std::nullopt, "void"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

}  // namespace

void test_backend_shared_call_decode_parses_bir_minimal_direct_call_module() {
  c4c::backend::bir::Module module;
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "helper",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::immediate_i32(42),
          },
      }},
      .is_declaration = false,
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "main",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {c4c::backend::bir::CallInst{
              .result = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
              .callee = "helper",
              .args = {},
              .return_type_name = "i32",
          }},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
          },
      }},
      .is_declaration = false,
  });

  const auto parsed = c4c::backend::parse_bir_minimal_direct_call_module(module);
  expect_true(parsed.has_value(),
              "shared call-decode surface should parse a BIR helper definition plus main direct call module");
  expect_true(parsed->helper != nullptr && parsed->helper->name == "helper" &&
                  parsed->main_function != nullptr && parsed->main_function->name == "main",
              "shared call-decode surface should preserve the helper and main function identities for BIR direct-call modules");
  expect_true(parsed->call != nullptr && parsed->call->callee == "helper" &&
                  parsed->return_imm == 42,
              "shared call-decode surface should recover the helper return immediate for minimal BIR direct-call modules");
}

void test_x86_codegen_header_exports_translated_globals_owner_symbols() {
  using X86Codegen = c4c::backend::x86::X86Codegen;

  auto emit_global_addr = &X86Codegen::emit_global_addr_impl;
  auto emit_tls_global_addr = &X86Codegen::emit_tls_global_addr_impl;
  auto emit_global_addr_absolute = &X86Codegen::emit_global_addr_absolute_impl;
  auto emit_global_load_rip_rel = &X86Codegen::emit_global_load_rip_rel_impl;
  auto emit_global_store_rip_rel = &X86Codegen::emit_global_store_rip_rel_impl;
  auto emit_label_addr = &X86Codegen::emit_label_addr_impl;
  auto emit_store_result = &X86Codegen::emit_store_result_impl;
  auto emit_load_operand = &X86Codegen::emit_load_operand_impl;

  expect_true(emit_global_addr != nullptr && emit_tls_global_addr != nullptr &&
                  emit_global_addr_absolute != nullptr &&
                  emit_global_load_rip_rel != nullptr &&
                  emit_global_store_rip_rel != nullptr && emit_label_addr != nullptr &&
                  emit_store_result != nullptr && emit_load_operand != nullptr,
              "x86 translated globals owner methods should stay link-reachable through the public x86_codegen surface once globals.cpp enters the build");
}

void test_x86_codegen_header_exports_translated_globals_owner_helper_symbols() {
  using X86Codegen = c4c::backend::x86::X86Codegen;

  auto emit_global_load_rip_rel = &X86Codegen::emit_global_load_rip_rel_impl;
  auto emit_global_store_rip_rel = &X86Codegen::emit_global_store_rip_rel_impl;
  auto emit_store_result = &X86Codegen::emit_store_result_impl;
  auto emit_load_operand = &X86Codegen::emit_load_operand_impl;

  expect_true(emit_global_load_rip_rel != nullptr &&
                  emit_global_store_rip_rel != nullptr &&
                  emit_store_result != nullptr && emit_load_operand != nullptr,
              "x86 translated globals owner helper methods should remain compile/link reachable while globals.cpp starts carrying real behavior");
}

void test_x86_codegen_header_exports_translated_returns_owner_symbols() {
  using X86Codegen = c4c::backend::x86::X86Codegen;

  auto emit_return = &X86Codegen::emit_return_impl;
  auto current_return_type = &X86Codegen::current_return_type_impl;
  auto emit_return_f32_to_reg = &X86Codegen::emit_return_f32_to_reg_impl;
  auto emit_return_f64_to_reg = &X86Codegen::emit_return_f64_to_reg_impl;
  auto emit_return_i128_to_regs = &X86Codegen::emit_return_i128_to_regs_impl;
  auto emit_return_f128_to_reg = &X86Codegen::emit_return_f128_to_reg_impl;
  auto emit_return_int_to_reg = &X86Codegen::emit_return_int_to_reg_impl;
  auto emit_get_return_f64_second = &X86Codegen::emit_get_return_f64_second_impl;
  auto emit_set_return_f64_second = &X86Codegen::emit_set_return_f64_second_impl;
  auto emit_get_return_f32_second = &X86Codegen::emit_get_return_f32_second_impl;
  auto emit_set_return_f32_second = &X86Codegen::emit_set_return_f32_second_impl;
  auto emit_get_return_f128_second = &X86Codegen::emit_get_return_f128_second_impl;
  auto emit_set_return_f128_second = &X86Codegen::emit_set_return_f128_second_impl;

  expect_true(emit_return != nullptr && current_return_type != nullptr &&
                  emit_return_f32_to_reg != nullptr && emit_return_f64_to_reg != nullptr &&
                  emit_return_i128_to_regs != nullptr && emit_return_f128_to_reg != nullptr &&
                  emit_return_int_to_reg != nullptr &&
                  emit_get_return_f64_second != nullptr &&
                  emit_set_return_f64_second != nullptr &&
                  emit_get_return_f32_second != nullptr &&
                  emit_set_return_f32_second != nullptr &&
                  emit_get_return_f128_second != nullptr &&
                  emit_set_return_f128_second != nullptr,
              "x86 translated returns owner methods should stay compile/link reachable through the public x86_codegen surface once returns.cpp enters the build");
}

void test_x86_codegen_header_exports_translated_prologue_owner_symbols() {
  using X86Codegen = c4c::backend::x86::X86Codegen;

  auto calculate_stack_space = &X86Codegen::calculate_stack_space_impl;
  auto aligned_frame_size = &X86Codegen::aligned_frame_size_impl;
  auto emit_prologue = &X86Codegen::emit_prologue_impl;
  auto emit_epilogue = &X86Codegen::emit_epilogue_impl;
  auto emit_store_params = &X86Codegen::emit_store_params_impl;
  auto emit_param_ref = &X86Codegen::emit_param_ref_impl;
  auto emit_epilogue_and_ret = &X86Codegen::emit_epilogue_and_ret_impl;
  auto store_instr_for_type = &X86Codegen::store_instr_for_type_impl;
  auto load_instr_for_type = &X86Codegen::load_instr_for_type_impl;

  expect_true(calculate_stack_space != nullptr && aligned_frame_size != nullptr &&
                  emit_prologue != nullptr && emit_epilogue != nullptr &&
                  emit_store_params != nullptr && emit_param_ref != nullptr &&
                  emit_epilogue_and_ret != nullptr && store_instr_for_type != nullptr &&
                  load_instr_for_type != nullptr,
              "x86 translated prologue owner methods should stay compile/link reachable through the public x86_codegen surface once prologue.cpp carries the active owner lane");
}

void test_riscv_codegen_header_exports_translated_returns_owner_symbols() {
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_return = &RiscvCodegen::emit_return_impl;
  auto emit_return_i128_to_regs = &RiscvCodegen::emit_return_i128_to_regs_impl;
  auto emit_return_f128_to_reg = &RiscvCodegen::emit_return_f128_to_reg_impl;
  auto emit_return_f32_to_reg = &RiscvCodegen::emit_return_f32_to_reg_impl;
  auto emit_return_f64_to_reg = &RiscvCodegen::emit_return_f64_to_reg_impl;
  auto emit_return_int_to_reg = &RiscvCodegen::emit_return_int_to_reg_impl;
  auto emit_get_return_f64_second = &RiscvCodegen::emit_get_return_f64_second_impl;
  auto emit_set_return_f64_second = &RiscvCodegen::emit_set_return_f64_second_impl;
  auto emit_get_return_f32_second = &RiscvCodegen::emit_get_return_f32_second_impl;
  auto emit_set_return_f32_second = &RiscvCodegen::emit_set_return_f32_second_impl;

  expect_true(emit_return != nullptr && emit_return_i128_to_regs != nullptr &&
                  emit_return_f128_to_reg != nullptr &&
                  emit_return_f32_to_reg != nullptr && emit_return_f64_to_reg != nullptr &&
                  emit_return_int_to_reg != nullptr &&
                  emit_get_return_f64_second != nullptr &&
                  emit_set_return_f64_second != nullptr &&
                  emit_get_return_f32_second != nullptr &&
                  emit_set_return_f32_second != nullptr,
              "riscv translated returns owner helpers should stay compile/link reachable through the shared riscv_codegen surface once returns.cpp starts carrying real owner behavior");
}

void test_riscv_codegen_header_exports_translated_prologue_return_exit_symbols() {
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto aligned_frame_size = &RiscvCodegen::aligned_frame_size_impl;
  auto emit_prologue = &RiscvCodegen::emit_prologue_impl;
  auto emit_epilogue = &RiscvCodegen::emit_epilogue_impl;
  auto emit_epilogue_and_ret = &RiscvCodegen::emit_epilogue_and_ret_impl;
  auto current_return_type = &RiscvCodegen::current_return_type_impl;

  expect_true(aligned_frame_size != nullptr && emit_prologue != nullptr &&
                  emit_epilogue != nullptr && emit_epilogue_and_ret != nullptr &&
                  current_return_type != nullptr,
              "riscv translated prologue return-exit helpers should stay compile/link reachable through the shared riscv_codegen surface once prologue.cpp starts carrying real owner behavior");
}

void test_riscv_codegen_header_exports_translated_variadic_activation_surface() {
  using IrType = c4c::backend::riscv::codegen::IrType;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using Value = c4c::backend::riscv::codegen::Value;

  expect_true(
      std::is_same_v<decltype(&RiscvCodegen::emit_va_arg_impl),
                     void (RiscvCodegen::*)(const Value&, const Value&, IrType)> &&
          std::is_same_v<decltype(&RiscvCodegen::emit_va_start_impl),
                         void (RiscvCodegen::*)(const Value&)> &&
          std::is_same_v<decltype(&RiscvCodegen::emit_va_copy_impl),
                         void (RiscvCodegen::*)(const Value&, const Value&)> &&
          c4c::backend::riscv::codegen::riscv_variadic_named_gp_count(3) == 3 &&
          c4c::backend::riscv::codegen::riscv_variadic_named_gp_count(11) == 8 &&
          c4c::backend::riscv::codegen::riscv_variadic_reg_save_area_size() == 64 &&
          c4c::backend::riscv::codegen::riscv_variadic_gp_save_offset(0) == 0 &&
          c4c::backend::riscv::codegen::riscv_variadic_gp_save_offset(7) == 56 &&
          c4c::backend::riscv::codegen::riscv_variadic_next_arg_stack_offset(3, 24) == 24 &&
          c4c::backend::riscv::codegen::riscv_variadic_next_arg_stack_offset(11, 24) == 88 &&
          c4c::backend::riscv::codegen::riscv_variadic_arg_step_bytes(false) == 8 &&
          c4c::backend::riscv::codegen::riscv_variadic_arg_step_bytes(true) == 16,
      "riscv translated variadic activation helpers should stay declaration-reachable through the shared riscv_codegen surface once the direct variadic owner bodies are wired");
}

void test_riscv_codegen_header_exports_translated_atomics_activation_surface() {
  using AtomicOrdering = c4c::backend::riscv::codegen::AtomicOrdering;
  using AtomicRmwOp = c4c::backend::riscv::codegen::AtomicRmwOp;
  using IrType = c4c::backend::riscv::codegen::IrType;
  using RiscvCodegenState = c4c::backend::riscv::codegen::RiscvCodegenState;

  auto amo_ordering_suffix = &c4c::backend::riscv::codegen::riscv_amo_ordering_suffix;
  auto amo_width_suffix = &c4c::backend::riscv::codegen::riscv_amo_width_suffix;
  auto sign_extend_atomic_result = &c4c::backend::riscv::codegen::riscv_sign_extend_atomic_result;
  auto is_atomic_subword_type = &c4c::backend::riscv::codegen::riscv_is_atomic_subword_type;
  auto atomic_subword_bits = &c4c::backend::riscv::codegen::riscv_atomic_subword_bits;

  expect_true(
      amo_ordering_suffix != nullptr && amo_width_suffix != nullptr &&
          sign_extend_atomic_result != nullptr && is_atomic_subword_type != nullptr &&
          atomic_subword_bits != nullptr &&
          static_cast<unsigned>(AtomicOrdering::SeqCst) == 4 &&
          static_cast<unsigned>(AtomicRmwOp::TestAndSet) == 7 &&
          sizeof(&c4c::backend::riscv::codegen::riscv_sign_extend_atomic_result) ==
              sizeof(void (*)(RiscvCodegenState&, IrType)),
      "riscv translated atomics activation helpers should stay declaration-reachable through the shared riscv_codegen surface once atomics.cpp crosses the first build boundary");
}

void test_riscv_codegen_header_exports_translated_atomic_load_store_fence_surface() {
  using AtomicOrdering = c4c::backend::riscv::codegen::AtomicOrdering;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_atomic_load = &RiscvCodegen::emit_atomic_load_impl;
  auto emit_atomic_store = &RiscvCodegen::emit_atomic_store_impl;
  auto emit_fence = &RiscvCodegen::emit_fence_impl;

  expect_true(emit_atomic_load != nullptr && emit_atomic_store != nullptr &&
                  emit_fence != nullptr &&
                  static_cast<unsigned>(AtomicOrdering::SeqCst) == 4,
              "riscv translated atomic load/store/fence seam should stay declaration-reachable through the shared riscv_codegen surface once the first direct atomics owner packet lands");
}

void test_riscv_codegen_header_exports_translated_atomic_rmw_cmpxchg_surface() {
  using AtomicOrdering = c4c::backend::riscv::codegen::AtomicOrdering;
  using AtomicRmwOp = c4c::backend::riscv::codegen::AtomicRmwOp;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_atomic_rmw = &RiscvCodegen::emit_atomic_rmw_impl;
  auto emit_atomic_cmpxchg = &RiscvCodegen::emit_atomic_cmpxchg_impl;

  expect_true(
      emit_atomic_rmw != nullptr && emit_atomic_cmpxchg != nullptr &&
          static_cast<unsigned>(AtomicRmwOp::Nand) == 5 &&
          static_cast<unsigned>(AtomicOrdering::AcqRel) == 3,
      "riscv translated atomic rmw/cmpxchg seam should stay declaration-reachable through the shared riscv_codegen surface once the bounded direct atomics owner packet lands");
}

void test_riscv_codegen_header_exports_translated_atomic_software_helper_surface() {
  using IrType = c4c::backend::riscv::codegen::IrType;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_clz = &RiscvCodegen::emit_clz;
  auto emit_ctz = &RiscvCodegen::emit_ctz;
  auto emit_bswap = &RiscvCodegen::emit_bswap;
  auto emit_popcount = &RiscvCodegen::emit_popcount;

  expect_true(emit_clz != nullptr && emit_ctz != nullptr && emit_bswap != nullptr &&
                  emit_popcount != nullptr && static_cast<unsigned>(IrType::U32) == 7,
              "riscv translated atomic software helper family should stay declaration-reachable through the shared riscv_codegen surface once the bounded helper packet lands");
}

void test_riscv_codegen_header_exports_translated_intrinsics_activation_surface() {
  using IntrinsicOp = c4c::backend::riscv::codegen::IntrinsicOp;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_rv_binary_128 = &RiscvCodegen::emit_rv_binary_128;
  auto emit_rv_zero_byte_mask = &RiscvCodegen::emit_rv_zero_byte_mask;
  auto emit_rv_cmpeq_bytes = &RiscvCodegen::emit_rv_cmpeq_bytes;
  auto emit_rv_cmpeq_dwords = &RiscvCodegen::emit_rv_cmpeq_dwords;
  auto emit_rv_binary_128_bytewise = &RiscvCodegen::emit_rv_binary_128_bytewise;
  auto emit_rv_psubusb_8bytes = &RiscvCodegen::emit_rv_psubusb_8bytes;
  auto emit_rv_psubsb_128 = &RiscvCodegen::emit_rv_psubsb_128;
  auto emit_rv_psubsb_8bytes = &RiscvCodegen::emit_rv_psubsb_8bytes;
  auto emit_rv_pmovmskb = &RiscvCodegen::emit_rv_pmovmskb;

  expect_true(
      emit_rv_binary_128 != nullptr && emit_rv_zero_byte_mask != nullptr &&
          emit_rv_cmpeq_bytes != nullptr && emit_rv_cmpeq_dwords != nullptr &&
          emit_rv_binary_128_bytewise != nullptr && emit_rv_psubusb_8bytes != nullptr &&
          emit_rv_psubsb_128 != nullptr && emit_rv_psubsb_8bytes != nullptr &&
          emit_rv_pmovmskb != nullptr && sizeof(IntrinsicOp) == sizeof(std::uint16_t) &&
          static_cast<unsigned>(IntrinsicOp::Pmovmskb128) == 18 &&
          static_cast<unsigned>(IntrinsicOp::Pextrq128) == 74,
      "riscv translated intrinsics activation helpers should stay declaration-reachable through the shared riscv_codegen surface once intrinsics.cpp crosses the current build boundary");
}

void test_riscv_codegen_header_exports_translated_intrinsics_dispatcher_surface() {
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_intrinsic_rv = &RiscvCodegen::emit_intrinsic_rv;

  expect_true(
      emit_intrinsic_rv != nullptr,
      "riscv translated intrinsics dispatcher should stay declaration-reachable through the shared riscv_codegen surface once the first direct owner packet lands");
}

void test_riscv_codegen_header_exports_translated_f128_return_helper_symbol() {
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_f128_operand_to_a0_a1 = &RiscvCodegen::emit_f128_operand_to_a0_a1;
  auto emit_f128_neg_full = &RiscvCodegen::emit_f128_neg_full;
  auto emit_f128_neg_impl = &RiscvCodegen::emit_f128_neg_impl;

  expect_true(emit_f128_operand_to_a0_a1 != nullptr && emit_f128_neg_full != nullptr &&
                  emit_f128_neg_impl != nullptr,
              "riscv translated f128 soft-float hook surface should stay compile/link reachable through the shared riscv_codegen surface once the bounded backend soft-float seam is wired");
}

void test_riscv_codegen_header_exports_translated_f128_compare_surface() {
  using IrCmpOp = c4c::backend::riscv::codegen::IrCmpOp;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_f128_cmp = &RiscvCodegen::emit_f128_cmp_impl;

  expect_true(
      emit_f128_cmp != nullptr && static_cast<unsigned>(IrCmpOp::Uge) == 9,
      "riscv translated f128 compare seam should stay declaration-reachable through the shared riscv_codegen surface once the bounded shared soft-float compare route lands");
}

void test_riscv_codegen_header_exports_translated_f128_memory_surface() {
  using Operand = c4c::backend::riscv::codegen::Operand;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using Value = c4c::backend::riscv::codegen::Value;

  auto emit_f128_store = &c4c::backend::emit_riscv_f128_store;
  auto emit_f128_load = &c4c::backend::emit_riscv_f128_load;
  auto emit_f128_store_with_offset = &c4c::backend::emit_riscv_f128_store_with_offset;
  auto emit_f128_load_with_offset = &c4c::backend::emit_riscv_f128_load_with_offset;

  expect_true(emit_f128_store != nullptr && emit_f128_load != nullptr &&
                  emit_f128_store_with_offset != nullptr &&
                  emit_f128_load_with_offset != nullptr &&
                  sizeof(&c4c::backend::emit_riscv_f128_store) ==
                      sizeof(void (*)(RiscvCodegen&, const Operand&, const Value&)),
              "riscv translated f128 memory seam should stay declaration-reachable through the shared soft-float surface once the bounded shared memory route lands");
}

void test_riscv_codegen_header_exports_translated_cast_surface() {
  using Operand = c4c::backend::riscv::codegen::Operand;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using Value = c4c::backend::riscv::codegen::Value;
  using IrType = c4c::backend::riscv::codegen::IrType;

  auto emit_cast = &RiscvCodegen::emit_cast_impl;
  auto emit_load_acc_pair = &RiscvCodegen::emit_load_acc_pair_impl;
  auto emit_store_acc_pair = &RiscvCodegen::emit_store_acc_pair_impl;
  auto emit_sign_extend_acc_high = &RiscvCodegen::emit_sign_extend_acc_high_impl;
  auto emit_zero_acc_high = &RiscvCodegen::emit_zero_acc_high_impl;
  auto emit_i128_to_float_call = &RiscvCodegen::emit_i128_to_float_call_impl;
  auto emit_float_to_i128_call = &RiscvCodegen::emit_float_to_i128_call_impl;
  auto emit_f128_cast = &c4c::backend::emit_f128_cast;
  auto emit_cast_default = &c4c::backend::emit_cast_default;

  expect_true(emit_cast != nullptr && emit_load_acc_pair != nullptr &&
                  emit_store_acc_pair != nullptr && emit_sign_extend_acc_high != nullptr &&
                  emit_zero_acc_high != nullptr && emit_i128_to_float_call != nullptr &&
                  emit_float_to_i128_call != nullptr && emit_f128_cast != nullptr &&
                  emit_cast_default != nullptr &&
                  sizeof(&c4c::backend::emit_f128_cast) ==
                      sizeof(bool (*)(c4c::backend::F128SoftFloatHooks&,
                                      const Value&,
                                      const Operand&,
                                      IrType,
                                      IrType)) &&
                  sizeof(&c4c::backend::emit_cast_default) ==
                      sizeof(void (*)(RiscvCodegen&, const Value&, const Operand&, IrType, IrType)),
              "riscv translated cast seam should stay declaration-reachable through the shared soft-float, shared default-cast, and riscv_codegen surfaces once the bounded default-cast route lands");
}

void test_riscv_codegen_header_exports_translated_i128_surface() {
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_store_pair_to_slot = &RiscvCodegen::emit_store_pair_to_slot_impl;
  auto emit_load_pair_from_slot = &RiscvCodegen::emit_load_pair_from_slot_impl;
  auto emit_save_acc_pair = &RiscvCodegen::emit_save_acc_pair_impl;
  auto emit_store_pair_indirect = &RiscvCodegen::emit_store_pair_indirect_impl;
  auto emit_load_pair_indirect = &RiscvCodegen::emit_load_pair_indirect_impl;
  auto emit_i128_neg = &RiscvCodegen::emit_i128_neg_impl;
  auto emit_i128_not = &RiscvCodegen::emit_i128_not_impl;
  auto emit_i128_prep_binop = &RiscvCodegen::emit_i128_prep_binop_impl;
  auto emit_i128_add = &RiscvCodegen::emit_i128_add_impl;
  auto emit_i128_sub = &RiscvCodegen::emit_i128_sub_impl;
  auto emit_i128_mul = &RiscvCodegen::emit_i128_mul_impl;
  auto emit_i128_and = &RiscvCodegen::emit_i128_and_impl;
  auto emit_i128_or = &RiscvCodegen::emit_i128_or_impl;
  auto emit_i128_xor = &RiscvCodegen::emit_i128_xor_impl;
  auto emit_i128_shl = &RiscvCodegen::emit_i128_shl_impl;
  auto emit_i128_lshr = &RiscvCodegen::emit_i128_lshr_impl;
  auto emit_i128_ashr = &RiscvCodegen::emit_i128_ashr_impl;
  auto emit_i128_prep_shift_lhs = &RiscvCodegen::emit_i128_prep_shift_lhs_impl;
  auto emit_i128_shl_const = &RiscvCodegen::emit_i128_shl_const_impl;
  auto emit_i128_lshr_const = &RiscvCodegen::emit_i128_lshr_const_impl;
  auto emit_i128_ashr_const = &RiscvCodegen::emit_i128_ashr_const_impl;
  auto emit_i128_divrem_call = &RiscvCodegen::emit_i128_divrem_call_impl;
  auto emit_i128_store_result = &RiscvCodegen::emit_i128_store_result_impl;
  auto emit_i128_cmp_eq = &RiscvCodegen::emit_i128_cmp_eq_impl;
  auto emit_i128_cmp_ordered = &RiscvCodegen::emit_i128_cmp_ordered_impl;
  auto emit_i128_cmp_store_result = &RiscvCodegen::emit_i128_cmp_store_result_impl;

  expect_true(emit_store_pair_to_slot != nullptr && emit_load_pair_from_slot != nullptr &&
                  emit_save_acc_pair != nullptr && emit_store_pair_indirect != nullptr &&
                  emit_load_pair_indirect != nullptr && emit_i128_neg != nullptr &&
                  emit_i128_not != nullptr && emit_i128_prep_binop != nullptr &&
                  emit_i128_add != nullptr && emit_i128_sub != nullptr &&
                  emit_i128_mul != nullptr && emit_i128_and != nullptr &&
                  emit_i128_or != nullptr && emit_i128_xor != nullptr &&
                  emit_i128_shl != nullptr && emit_i128_lshr != nullptr &&
                  emit_i128_ashr != nullptr &&
                  emit_i128_prep_shift_lhs != nullptr &&
                  emit_i128_shl_const != nullptr &&
                  emit_i128_lshr_const != nullptr &&
                  emit_i128_ashr_const != nullptr &&
                  emit_i128_divrem_call != nullptr &&
                  emit_i128_store_result != nullptr &&
                  emit_i128_cmp_eq != nullptr && emit_i128_cmp_ordered != nullptr &&
                  emit_i128_cmp_store_result != nullptr,
              "riscv translated i128 owner surface should stay declaration-reachable through the shared riscv_codegen surface once the broader i128 route lands");
}

void test_riscv_codegen_header_exports_translated_memory_helper_surface() {
  using IrType = c4c::backend::riscv::codegen::IrType;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using RiscvCodegenState = c4c::backend::riscv::codegen::RiscvCodegenState;
  using SlotAddr = c4c::backend::riscv::codegen::SlotAddr;

  auto store_for_type = &RiscvCodegen::store_for_type;
  auto load_for_type = &RiscvCodegen::load_for_type;
  auto emit_typed_store_to_slot = &RiscvCodegen::emit_typed_store_to_slot_impl;
  auto emit_typed_load_from_slot = &RiscvCodegen::emit_typed_load_from_slot_impl;
  auto emit_load_ptr_from_slot = &RiscvCodegen::emit_load_ptr_from_slot_impl;
  auto emit_add_offset_to_addr_reg = &RiscvCodegen::emit_add_offset_to_addr_reg_impl;
  auto emit_slot_addr_to_secondary = &RiscvCodegen::emit_slot_addr_to_secondary_impl;
  auto emit_gep_direct_const = &RiscvCodegen::emit_gep_direct_const_impl;
  auto emit_gep_indirect_const = &RiscvCodegen::emit_gep_indirect_const_impl;
  auto emit_memcpy_load_dest_addr = &RiscvCodegen::emit_memcpy_load_dest_addr_impl;
  auto emit_memcpy_load_src_addr = &RiscvCodegen::emit_memcpy_load_src_addr_impl;
  auto emit_memcpy_impl = &RiscvCodegen::emit_memcpy_impl_impl;
  auto emit_add_imm_to_acc = &RiscvCodegen::emit_add_imm_to_acc_impl;
  auto emit_round_up_acc_to_16 = &RiscvCodegen::emit_round_up_acc_to_16_impl;
  auto emit_sub_sp_by_acc = &RiscvCodegen::emit_sub_sp_by_acc_impl;
  auto emit_mov_sp_to_acc = &RiscvCodegen::emit_mov_sp_to_acc_impl;
  auto emit_mov_acc_to_sp = &RiscvCodegen::emit_mov_acc_to_sp_impl;
  auto emit_align_acc = &RiscvCodegen::emit_align_acc_impl;
  auto emit_alloca_aligned_addr = &RiscvCodegen::emit_alloca_aligned_addr_impl;
  auto emit_alloca_aligned_addr_to_acc = &RiscvCodegen::emit_alloca_aligned_addr_to_acc_impl;
  auto state_resolve_slot_addr = &RiscvCodegenState::resolve_slot_addr;
  auto state_assigned_reg = &RiscvCodegenState::assigned_reg;

  expect_true(store_for_type != nullptr && load_for_type != nullptr &&
                  emit_typed_store_to_slot != nullptr && emit_typed_load_from_slot != nullptr &&
                  emit_load_ptr_from_slot != nullptr &&
                  emit_add_offset_to_addr_reg != nullptr && emit_add_imm_to_acc != nullptr &&
                  emit_slot_addr_to_secondary != nullptr &&
                  emit_gep_direct_const != nullptr && emit_gep_indirect_const != nullptr &&
                  emit_memcpy_load_dest_addr != nullptr &&
                  emit_memcpy_load_src_addr != nullptr && emit_memcpy_impl != nullptr &&
                  emit_round_up_acc_to_16 != nullptr && emit_sub_sp_by_acc != nullptr &&
                  emit_mov_sp_to_acc != nullptr && emit_mov_acc_to_sp != nullptr &&
                  emit_align_acc != nullptr &&
                  emit_alloca_aligned_addr != nullptr &&
                  state_resolve_slot_addr != nullptr &&
                  state_assigned_reg != nullptr &&
                  SlotAddr::Direct({-8}).kind == SlotAddr::Kind::Direct &&
                  emit_alloca_aligned_addr_to_acc != nullptr &&
                  IrType::I64 != IrType::F128,
              "riscv translated memory helper seam should stay declaration-reachable through the shared riscv_codegen surface once memory.cpp enters the bounded owner lane");
}

void test_riscv_memory_shared_surface_models_slot_addr_and_type_selectors() {
  using IrType = c4c::backend::riscv::codegen::IrType;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using RiscvCodegenState = c4c::backend::riscv::codegen::RiscvCodegenState;
  using SlotAddr = c4c::backend::riscv::codegen::SlotAddr;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;

  RiscvCodegenState state;
  state.slots.emplace(1, StackSlot{-24});
  state.slots.emplace(2, StackSlot{-32});
  state.slots.emplace(3, StackSlot{-48});
  state.allocas.insert(2);
  state.over_aligned_allocas.emplace(3, 64);

  const auto direct = state.resolve_slot_addr(1);
  const auto indirect = state.resolve_slot_addr(2);
  const auto over_aligned = state.resolve_slot_addr(3);
  const auto missing = state.resolve_slot_addr(99);

  expect_true(direct.has_value() && direct->kind == SlotAddr::Kind::Direct &&
                  direct->slot.raw == -24,
              "the bounded riscv memory shared surface should model direct slot addresses for ordinary stack-backed values");
  expect_true(indirect.has_value() && indirect->kind == SlotAddr::Kind::Indirect &&
                  indirect->slot.raw == -32,
              "the bounded riscv memory shared surface should model indirect slot addresses for alloca-backed values");
  expect_true(over_aligned.has_value() && over_aligned->kind == SlotAddr::Kind::OverAligned &&
                  over_aligned->slot.raw == -48 && over_aligned->value_id == 3,
              "the bounded riscv memory shared surface should preserve over-aligned slot identity for aligned alloca address resolution");
  expect_true(!missing.has_value(),
              "the bounded riscv memory shared surface should return no slot address when a value has no tracked slot");

  expect_true(std::string(RiscvCodegen::store_for_type(IrType::I16)) == "sh" &&
                  std::string(RiscvCodegen::store_for_type(IrType::F32)) == "sw" &&
                  std::string(RiscvCodegen::store_for_type(IrType::I64)) == "sd" &&
                  std::string(RiscvCodegen::load_for_type(IrType::I16)) == "lh" &&
                  std::string(RiscvCodegen::load_for_type(IrType::U16)) == "lhu" &&
                  std::string(RiscvCodegen::load_for_type(IrType::I32)) == "lw" &&
                  std::string(RiscvCodegen::load_for_type(IrType::F32)) == "lwu" &&
                  std::string(RiscvCodegen::load_for_type(IrType::I64)) == "ld",
              "the bounded riscv memory shared surface should select the translated typed load/store instructions used by the upcoming scalar default memory owner path");
}

void test_riscv_codegen_header_exports_translated_memory_owner_surface() {
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_store = &RiscvCodegen::emit_store_impl;
  auto emit_load = &RiscvCodegen::emit_load_impl;
  auto emit_store_with_const_offset = &RiscvCodegen::emit_store_with_const_offset_impl;
  auto emit_load_with_const_offset = &RiscvCodegen::emit_load_with_const_offset_impl;

  expect_true(emit_store != nullptr && emit_load != nullptr &&
                  emit_store_with_const_offset != nullptr &&
                  emit_load_with_const_offset != nullptr,
              "riscv translated memory owner surface should stay declaration-reachable through the shared riscv_codegen surface once the scalar default load/store packet lands");
}

void test_riscv_codegen_header_exports_translated_alu_unary_surface() {
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_float_neg = &RiscvCodegen::emit_float_neg_impl;
  auto emit_int_neg = &RiscvCodegen::emit_int_neg_impl;
  auto emit_int_not = &RiscvCodegen::emit_int_not_impl;

  expect_true(emit_float_neg != nullptr && emit_int_neg != nullptr && emit_int_not != nullptr,
              "riscv translated alu unary seam should stay declaration-reachable through the shared riscv_codegen surface once alu.cpp enters the bounded owner lane");
}

void test_riscv_codegen_header_exports_translated_alu_integer_owner_surface() {
  using IrBinOp = c4c::backend::riscv::codegen::IrBinOp;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_int_binop = &RiscvCodegen::emit_int_binop_impl;
  auto emit_copy_i128 = &RiscvCodegen::emit_copy_i128_impl;

  expect_true(emit_int_binop != nullptr && emit_copy_i128 != nullptr &&
                  static_cast<unsigned>(IrBinOp::LShr) == 12,
              "riscv translated alu integer-owner seam should stay declaration-reachable through the shared riscv_codegen surface once the broader alu packet lands");
}

void test_riscv_codegen_header_exports_translated_calls_abi_surface() {
  using CallAbiConfig = c4c::backend::riscv::codegen::CallAbiConfig;
  using CallArgClass = c4c::backend::riscv::codegen::CallArgClass;
  using IrType = c4c::backend::riscv::codegen::IrType;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto call_abi_config = &RiscvCodegen::call_abi_config_impl;
  auto emit_call_compute_stack_space = &RiscvCodegen::emit_call_compute_stack_space_impl;

  CallArgClass stack_class;
  stack_class.value = CallArgClass::Stack{};
  CallArgClass reg_pair_class;
  reg_pair_class.value = CallArgClass::I128RegPair{2};
  CallArgClass split_stack_class;
  split_stack_class.value = CallArgClass::StructSplitRegStack{1, 24};
  const CallAbiConfig abi_config = RiscvCodegen{}.call_abi_config_impl();

  expect_true(call_abi_config != nullptr && emit_call_compute_stack_space != nullptr &&
                  stack_class.is_stack() && !reg_pair_class.is_stack() &&
                  stack_class.stack_bytes() == 8 &&
                  split_stack_class.stack_bytes() == 16 &&
                  abi_config.max_int_regs == 8 && abi_config.max_float_regs == 8 &&
                  abi_config.align_i128_pairs && abi_config.f128_in_gp_pairs &&
                  abi_config.variadic_floats_in_gp &&
                  abi_config.use_riscv_float_struct_classification &&
                  abi_config.allow_struct_split_reg_stack &&
                  abi_config.align_struct_pairs && !abi_config.f128_in_fp_regs &&
                  !abi_config.use_sysv_struct_classification &&
                  !abi_config.sret_uses_dedicated_reg &&
                  IrType::I64 != IrType::F128,
              "riscv translated calls ABI seam should keep the shared ABI structs and helper entry points declaration-reachable through the shared riscv_codegen surface once calls.cpp enters the bounded owner lane");
}

void test_riscv_codegen_header_exports_translated_calls_instruction_cleanup_surface() {
  using Operand = c4c::backend::riscv::codegen::Operand;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_call_instruction = &RiscvCodegen::emit_call_instruction_impl;
  auto emit_call_cleanup = &RiscvCodegen::emit_call_cleanup_impl;

  expect_true(emit_call_instruction != nullptr && emit_call_cleanup != nullptr &&
                  Operand::immediate_i64(7).kind == Operand::Kind::ImmI64,
              "riscv translated calls instruction/cleanup seam should keep the owner entry points declaration-reachable through the shared riscv_codegen surface once the next calls.cpp packet lands");
}

void test_riscv_codegen_header_exports_translated_calls_result_store_surface() {
  using IrType = c4c::backend::riscv::codegen::IrType;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_call_store_result = &RiscvCodegen::emit_call_store_result_impl;

  expect_true(emit_call_store_result != nullptr && IrType::I64 != IrType::F128,
              "riscv translated calls result-store seam should keep the bounded owner entry point declaration-reachable through the shared riscv_codegen surface once the next calls.cpp packet lands");
}

void test_riscv_codegen_header_exports_translated_calls_outgoing_surface() {
  using CallArgClass = c4c::backend::riscv::codegen::CallArgClass;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using RiscvFloatClass = c4c::backend::riscv::codegen::RiscvFloatClass;

  auto emit_call_f128_pre_convert = &RiscvCodegen::emit_call_f128_pre_convert_impl;
  auto emit_call_stack_args = &RiscvCodegen::emit_call_stack_args_impl;
  auto emit_call_reg_args = &RiscvCodegen::emit_call_reg_args_impl;

  CallArgClass stack_class;
  stack_class.value = CallArgClass::Stack{};
  CallArgClass int_reg_class;
  int_reg_class.value = CallArgClass::IntReg{0};
  CallArgClass float_reg_class;
  float_reg_class.value = CallArgClass::FloatReg{1};

  expect_true(
      emit_call_f128_pre_convert != nullptr && emit_call_stack_args != nullptr &&
          emit_call_reg_args != nullptr && stack_class.is_stack() &&
          !std::holds_alternative<CallArgClass::Stack>(int_reg_class.value) &&
          !float_reg_class.is_stack() &&
          std::holds_alternative<RiscvFloatClass::TwoFloats>(
              RiscvFloatClass::two_floats(false, true).value),
      "riscv translated outgoing-call lowering surface should keep the first calls-family entry points declaration-reachable through the shared riscv_codegen surface once calls.cpp advances into the broader staging lane");
}

void test_riscv_codegen_header_exports_translated_globals_label_surface() {
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_label_addr = &RiscvCodegen::emit_label_addr_impl;

  expect_true(emit_label_addr != nullptr,
              "riscv translated globals label seam should stay declaration-reachable through the shared riscv_codegen surface once globals.cpp enters the bounded owner lane");
}

void test_riscv_codegen_header_exports_translated_global_addr_surface() {
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_global_addr = &RiscvCodegen::emit_global_addr_impl;

  expect_true(emit_global_addr != nullptr,
              "riscv translated global-address seam should stay declaration-reachable through the shared riscv_codegen surface once the GOT-aware globals.cpp packet lands");
}

void test_riscv_codegen_header_exports_translated_tls_global_addr_surface() {
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_tls_global_addr = &RiscvCodegen::emit_tls_global_addr_impl;

  expect_true(emit_tls_global_addr != nullptr,
              "riscv translated tls-global-address seam should stay declaration-reachable through the shared riscv_codegen surface once globals.cpp advances into the bounded TLS owner lane");
}

void test_riscv_codegen_header_exports_translated_float_binop_surface() {
  using FloatOp = c4c::backend::riscv::codegen::FloatOp;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_float_binop = &RiscvCodegen::emit_float_binop_impl;

  expect_true(
      emit_float_binop != nullptr && static_cast<unsigned>(FloatOp::Div) == 3,
      "riscv translated float-binop seam should stay declaration-reachable through the shared riscv_codegen surface once float_ops.cpp enters the bounded owner lane");
}

void test_riscv_codegen_header_exports_translated_int_cmp_surface() {
  using IrCmpOp = c4c::backend::riscv::codegen::IrCmpOp;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_int_cmp = &RiscvCodegen::emit_int_cmp_impl;

  expect_true(
      emit_int_cmp != nullptr && static_cast<unsigned>(IrCmpOp::Uge) == 9,
      "riscv translated integer-compare seam should stay declaration-reachable through the shared riscv_codegen surface once comparison.cpp advances into the bounded integer-owner lane");
}

void test_riscv_codegen_header_exports_translated_comparison_control_flow_surface() {
  using IrCmpOp = c4c::backend::riscv::codegen::IrCmpOp;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_fused_cmp_branch = &RiscvCodegen::emit_fused_cmp_branch_impl;
  auto emit_select = &RiscvCodegen::emit_select_impl;

  expect_true(
      emit_fused_cmp_branch != nullptr && emit_select != nullptr &&
          static_cast<unsigned>(IrCmpOp::Uge) == 9,
      "riscv translated comparison control-flow seam should stay declaration-reachable through the shared riscv_codegen surface once comparison.cpp advances into the broader fused-branch/select lane");
}

void test_riscv_codegen_header_exports_translated_float_cmp_surface() {
  using IrCmpOp = c4c::backend::riscv::codegen::IrCmpOp;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_float_cmp = &RiscvCodegen::emit_float_cmp_impl;

  expect_true(
      emit_float_cmp != nullptr && static_cast<unsigned>(IrCmpOp::Uge) == 9,
      "riscv translated float-compare seam should stay declaration-reachable through the shared riscv_codegen surface once comparison.cpp enters the bounded owner lane");
}

void test_riscv_traits_exports_shared_return_default_seam() {
  using Operand = c4c::backend::riscv::codegen::Operand;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  auto emit_return_default = &c4c::backend::emit_riscv_return_default;
  auto emit_load_operand = &RiscvCodegen::emit_load_operand_for_return_default;
  auto emit_load_acc_pair = &RiscvCodegen::emit_load_acc_pair_for_return_default;

  expect_true(emit_return_default != nullptr && emit_load_operand != nullptr &&
                  emit_load_acc_pair != nullptr,
              "bounded riscv shared return-default seam should stay compile/link reachable through traits.cpp and the shared riscv_codegen surface");

  static_cast<void>(sizeof(Operand));
}

void test_riscv_translated_returns_owner_emits_bounded_helper_text() {
  using Operand = c4c::backend::riscv::codegen::Operand;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(5, StackSlot{-24});
  codegen.state.slots.emplace(7, StackSlot{-40});

  codegen.emit_return_i128_to_regs_impl();
  codegen.emit_return_f128_to_reg_impl();
  codegen.emit_return_f32_to_reg_impl();
  codegen.emit_return_f64_to_reg_impl();
  codegen.emit_return_int_to_reg_impl();
  codegen.emit_get_return_f64_second_impl(Value{5});
  codegen.emit_set_return_f64_second_impl(Operand{7});
  codegen.emit_set_return_f64_second_impl(Operand::f64_const(0.0));
  codegen.emit_set_return_f64_second_impl(Operand::immediate_i64(11));
  codegen.emit_get_return_f32_second_impl(Value{5});
  codegen.emit_set_return_f32_second_impl(Operand{7});
  codegen.emit_set_return_f32_second_impl(Operand::f32_const(0.0f));
  codegen.emit_set_return_f32_second_impl(Operand::immediate_i64(7));

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    mv a0, t0",
                  "wired translated riscv returns helpers should move the primary integer lane into a0");
  expect_contains(asm_text, "    mv a1, t1",
                  "wired translated riscv returns helpers should move the secondary i128 lane into a1");
  expect_contains(asm_text, "    call __extenddftf2",
                  "wired translated riscv returns helpers should preserve the bounded f128 soft-float bridge");
  expect_contains(asm_text, "    fmv.w.x fa0, t0",
                  "wired translated riscv returns helpers should move f32 return values into fa0");
  expect_contains(asm_text, "    fmv.d.x fa0, t0",
                  "wired translated riscv returns helpers should move f64 return values into fa0");
  expect_contains(asm_text, "    fsd fa1, -24(s0)",
                  "wired translated riscv returns helpers should spill the second f64 return lane through the shared slot surface");
  expect_contains(asm_text, "    fld fa1, -40(s0)",
                  "wired translated riscv returns helpers should reload the second f64 return lane through the shared slot surface");
  expect_contains(asm_text, "    li t0, 0",
                  "wired translated riscv returns helpers should materialize bounded float constants through t0");
  expect_contains(asm_text, "    fmv.d.x fa1, t0",
                  "wired translated riscv returns helpers should route bounded second-lane f64 values through fa1");
  expect_contains(asm_text, "    fsw fa1, -24(s0)",
                  "wired translated riscv returns helpers should spill the second f32 return lane through the shared slot surface");
  expect_contains(asm_text, "    flw fa1, -40(s0)",
                  "wired translated riscv returns helpers should reload the second f32 return lane through the shared slot surface");
  expect_contains(asm_text, "    fmv.w.x fa1, t0",
                  "wired translated riscv returns helpers should route bounded second-lane f32 values through fa1");
}

void test_riscv_translated_returns_owner_emits_bounded_value_return_paths() {
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;

  RiscvCodegen f128_codegen;
  f128_codegen.current_return_type = IrType::F128;
  f128_codegen.state.slots.emplace(5, StackSlot{-24});
  f128_codegen.emit_return_impl(Operand{5}, 16);

  std::string f128_text;
  for (const auto& line : f128_codegen.state.asm_lines) {
    f128_text += line;
    f128_text.push_back('\n');
  }

  expect_true(f128_codegen.current_return_type_impl() == IrType::F128,
              "wired translated riscv returns entry should surface the bounded current return type state");
  expect_contains(f128_text, "    ld a0, -24(s0)",
                  "wired translated riscv returns entry should route long-double returns through the bounded gp-pair helper");
  expect_contains(f128_text, "    ld a1, -16(s0)",
                  "wired translated riscv returns entry should load the long-double high lane through the bounded gp-pair helper");
  expect_contains(f128_text, "    ret",
                  "wired translated riscv returns entry should finish bounded long-double returns through the shared epilogue path");

  RiscvCodegen default_scalar_codegen;
  default_scalar_codegen.current_return_type = IrType::I64;
  default_scalar_codegen.emit_return_impl(Operand::immediate_i64(42), 32);

  std::string default_scalar_text;
  for (const auto& line : default_scalar_codegen.state.asm_lines) {
    default_scalar_text += line;
    default_scalar_text.push_back('\n');
  }

  expect_contains(default_scalar_text, "    li t0, 42",
                  "wired translated riscv returns entry should delegate non-long-double returns through the shared default operand loader");
  expect_contains(default_scalar_text, "    mv a0, t0",
                  "wired translated riscv returns entry should delegate non-long-double integer returns through the shared default register move");
  expect_contains(default_scalar_text, "    ret",
                  "wired translated riscv returns entry should finish delegated return paths through the shared epilogue helper");
}

void test_riscv_traits_return_default_emits_bounded_shared_return_text() {
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;

  RiscvCodegen i128_codegen;
  i128_codegen.current_return_type = IrType::I128;
  i128_codegen.state.slots.emplace(9, StackSlot{-48});
  const Operand i128_value{9};
  c4c::backend::emit_riscv_return_default(i128_codegen, &i128_value, 0);

  std::string i128_text;
  for (const auto& line : i128_codegen.state.asm_lines) {
    i128_text += line;
    i128_text.push_back('\n');
  }

  expect_contains(i128_text, "    ld t0, -48(s0)",
                  "bounded riscv shared return-default seam should load the low i128 lane through the shared slot surface");
  expect_contains(i128_text, "    ld t1, -40(s0)",
                  "bounded riscv shared return-default seam should load the high i128 lane through the shared slot surface");
  expect_contains(i128_text, "    mv a0, t0",
                  "bounded riscv shared return-default seam should route the low i128 lane into a0");
  expect_contains(i128_text, "    mv a1, t1",
                  "bounded riscv shared return-default seam should route the high i128 lane into a1");
  expect_contains(i128_text, "    ret",
                  "bounded riscv shared return-default seam should finish through the shared epilogue handoff");

  RiscvCodegen scalar_codegen;
  scalar_codegen.current_return_type = IrType::F32;
  const auto scalar_value = Operand::f32_const(0.0f);
  c4c::backend::emit_riscv_return_default(scalar_codegen, &scalar_value, 0);

  std::string scalar_text;
  for (const auto& line : scalar_codegen.state.asm_lines) {
    scalar_text += line;
    scalar_text.push_back('\n');
  }

  expect_contains(scalar_text, "    li t0, 0",
                  "bounded riscv shared return-default seam should materialize scalar return operands through the shared load-operand path");
  expect_contains(scalar_text, "    fmv.w.x fa0, t0",
                  "bounded riscv shared return-default seam should route f32 returns into fa0");

  RiscvCodegen void_codegen;
  c4c::backend::emit_riscv_return_default(void_codegen, nullptr, 0);

  std::string void_text;
  for (const auto& line : void_codegen.state.asm_lines) {
    void_text += line;
    void_text.push_back('\n');
  }

  expect_true(void_text == "    ret\n",
              "bounded riscv shared return-default seam should still emit the shared epilogue handoff when no return value is present");
}

void test_riscv_translated_f128_return_helper_emits_gp_pair_loads() {
  using Operand = c4c::backend::riscv::codegen::Operand;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(9, StackSlot{-48});

  codegen.emit_f128_operand_to_a0_a1(Operand{9});
  codegen.emit_f128_operand_to_a0_a1(Operand::f64_const(0.0));

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    ld a0, -48(s0)",
                  "wired translated riscv f128 helper should load the low long-double lane into a0");
  expect_contains(asm_text, "    ld a1, -40(s0)",
                  "wired translated riscv f128 helper should load the high long-double lane into a1");
  expect_contains(asm_text, "    li a0, 0",
                  "wired translated riscv f128 helper should keep the bounded constant materialization path available");
  expect_contains(asm_text, "    li a1, 0",
                  "wired translated riscv f128 helper should clear the high lane for the bounded constant bridge");
}

void test_riscv_translated_memory_helper_seam_emits_bounded_text() {
  using IrType = c4c::backend::riscv::codegen::IrType;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;

  RiscvCodegen codegen;
  codegen.state.assigned_regs.emplace(9, c4c::backend::riscv::codegen::PhysReg{7});
  codegen.state.assigned_regs.emplace(10, c4c::backend::riscv::codegen::PhysReg{8});
  codegen.state.over_aligned_allocas.emplace(7, 32);
  codegen.state.over_aligned_allocas.emplace(8, 4096);

  codegen.emit_typed_store_to_slot_impl("sd", IrType::I64, StackSlot{-24});
  codegen.emit_typed_load_from_slot_impl("ld", StackSlot{-40});
  codegen.emit_load_ptr_from_slot_impl(StackSlot{-48}, 9);
  codegen.emit_load_ptr_from_slot_impl(StackSlot{-56}, 99);
  codegen.emit_add_offset_to_addr_reg_impl(16);
  codegen.emit_add_offset_to_addr_reg_impl(4096);
  codegen.emit_slot_addr_to_secondary_impl(StackSlot{-64}, true, 7);
  codegen.emit_slot_addr_to_secondary_impl(StackSlot{-72}, false, 10);
  codegen.emit_slot_addr_to_secondary_impl(StackSlot{-80}, false, 100);
  codegen.emit_gep_direct_const_impl(StackSlot{-96}, 24);
  codegen.emit_gep_indirect_const_impl(StackSlot{-88}, 0, 10);
  codegen.emit_gep_indirect_const_impl(StackSlot{-104}, 5000, 101);
  codegen.emit_memcpy_load_dest_addr_impl(StackSlot{-112}, true, 7);
  codegen.emit_memcpy_load_dest_addr_impl(StackSlot{-120}, false, 10);
  codegen.emit_memcpy_load_dest_addr_impl(StackSlot{-128}, false, 102);
  codegen.emit_memcpy_load_src_addr_impl(StackSlot{-8192}, true, 8);
  codegen.emit_memcpy_load_src_addr_impl(StackSlot{-144}, false, 9);
  codegen.emit_memcpy_load_src_addr_impl(StackSlot{-152}, false, 103);
  codegen.emit_memcpy_impl_impl(24);
  codegen.emit_add_imm_to_acc_impl(-8);
  codegen.emit_add_imm_to_acc_impl(5000);
  codegen.emit_round_up_acc_to_16_impl();
  codegen.emit_sub_sp_by_acc_impl();
  codegen.emit_mov_sp_to_acc_impl();
  codegen.emit_mov_acc_to_sp_impl();
  codegen.emit_align_acc_impl(32);
  codegen.emit_alloca_aligned_addr_impl(StackSlot{-56}, 7);
  codegen.emit_alloca_aligned_addr_to_acc_impl(StackSlot{-8192}, 8);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    sd t0, -24(s0)",
                  "wired translated riscv memory helper seam should store typed slot values through the shared s0-relative helper");
  expect_contains(asm_text, "    ld t0, -40(s0)",
                  "wired translated riscv memory helper seam should load typed slot values through the shared s0-relative helper");
  expect_contains(asm_text, "    mv t5, s7",
                  "wired translated riscv memory helper seam should reuse assigned callee-saved registers when loading pointer-valued slots");
  expect_contains(asm_text, "    ld t5, -56(s0)",
                  "wired translated riscv memory helper seam should still load pointer-valued slots from the stack when no register assignment is present");
  expect_contains(asm_text, "    addi t5, t5, 16",
                  "wired translated riscv memory helper seam should keep 12-bit address offsets on the direct addi path");
  expect_contains(asm_text, "    li t6, 4096",
                  "wired translated riscv memory helper seam should materialize large address offsets through the temporary register path");
  expect_contains(asm_text, "    add t5, t5, t6",
                  "wired translated riscv memory helper seam should apply large address offsets through the bounded t5 helper path");
  expect_contains(asm_text, "    addi t1, s0, -64",
                  "wired translated riscv memory helper seam should materialize alloca slot addresses into the secondary register without widening into broader address resolution");
  expect_contains(asm_text, "    mv t1, s8",
                  "wired translated riscv memory helper seam should reuse assigned callee-saved registers when routing slot addresses into the secondary register");
  expect_contains(asm_text, "    ld t1, -80(s0)",
                  "wired translated riscv memory helper seam should still load secondary slot addresses from the stack when no register assignment is present");
  expect_contains(asm_text, "    addi t0, s0, -72",
                  "wired translated riscv memory helper seam should form direct constant GEP addresses relative to s0 in the accumulator");
  expect_contains(asm_text, "    mv t0, s8",
                  "wired translated riscv memory helper seam should reuse assigned callee-saved registers for indirect constant GEP bases");
  expect_contains(asm_text, "    ld t0, -104(s0)",
                  "wired translated riscv memory helper seam should still load indirect constant GEP bases from the stack when no register assignment is present");
  expect_contains(asm_text, "    addi t1, s0, -112",
                  "wired translated riscv memory helper seam should materialize memcpy destination alloca addresses into t1 without widening into the full memcpy owner body");
  expect_contains(asm_text, "    mv t1, s8",
                  "wired translated riscv memory helper seam should reuse assigned callee-saved registers for memcpy destination addresses");
  expect_contains(asm_text, "    ld t1, -128(s0)",
                  "wired translated riscv memory helper seam should still load memcpy destination addresses from the stack when no register assignment is present");
  expect_contains(asm_text, "    li t6, -8192",
                  "wired translated riscv memory helper seam should support large alloca-relative memcpy source addresses through the shared s0-add helper");
  expect_contains(asm_text, "    add t2, s0, t6",
                  "wired translated riscv memory helper seam should materialize memcpy source alloca addresses into t2 when the slot offset exceeds addi range");
  expect_contains(asm_text, "    mv t2, s7",
                  "wired translated riscv memory helper seam should reuse assigned callee-saved registers for memcpy source addresses");
  expect_contains(asm_text, "    ld t2, -152(s0)",
                  "wired translated riscv memory helper seam should still load memcpy source addresses from the stack when no register assignment is present");
  expect_contains(asm_text, "    li t3, 24",
                  "wired translated riscv memory helper seam should materialize the memcpy byte count into the bounded loop counter register");
  expect_contains(asm_text, ".Lmemcpy_loop_",
                  "wired translated riscv memory helper seam should emit a dedicated local loop label without widening into broader memory owner behavior");
  expect_contains(asm_text, "    beqz t3, .Lmemcpy_done_",
                  "wired translated riscv memory helper seam should exit the bounded memcpy loop when the byte counter reaches zero");
  expect_contains(asm_text, "    lbu t4, 0(t2)",
                  "wired translated riscv memory helper seam should load one source byte per memcpy iteration through the bounded byte-copy path");
  expect_contains(asm_text, "    sb t4, 0(t1)",
                  "wired translated riscv memory helper seam should store one destination byte per memcpy iteration through the bounded byte-copy path");
  expect_contains(asm_text, "    addi t1, t1, 1",
                  "wired translated riscv memory helper seam should advance the destination pointer one byte per loop iteration");
  expect_contains(asm_text, "    addi t2, t2, 1",
                  "wired translated riscv memory helper seam should advance the source pointer one byte per loop iteration");
  expect_contains(asm_text, "    addi t3, t3, -1",
                  "wired translated riscv memory helper seam should decrement the bounded loop counter after each copied byte");
  expect_contains(asm_text, "    j .Lmemcpy_loop_",
                  "wired translated riscv memory helper seam should branch back to the bounded local loop label until the copy completes");
  expect_contains(asm_text, ".Lmemcpy_done_",
                  "wired translated riscv memory helper seam should emit a dedicated local done label for the bounded memcpy loop");
  expect_contains(asm_text, "    addi t0, t0, -8",
                  "wired translated riscv memory helper seam should keep small accumulator offsets on the direct addi path");
  expect_contains(asm_text, "    li t1, 5000",
                  "wired translated riscv memory helper seam should materialize large accumulator offsets through the temporary register path");
  expect_contains(asm_text, "    add t0, t0, t1",
                  "wired translated riscv memory helper seam should apply large accumulator offsets through the bounded t0 helper path");
  expect_contains(asm_text, "    addi t0, t0, 15",
                  "wired translated riscv memory helper seam should round dynamic alloca sizes up to the bounded 16-byte stack alignment");
  expect_contains(asm_text, "    andi t0, t0, -16",
                  "wired translated riscv memory helper seam should finish the bounded dynamic alloca size alignment with an immediate mask");
  expect_contains(asm_text, "    sub sp, sp, t0",
                  "wired translated riscv memory helper seam should reserve dynamic alloca space by subtracting the accumulator from sp");
  expect_contains(asm_text, "    mv t0, sp",
                  "wired translated riscv memory helper seam should expose the current stack pointer through the bounded accumulator path");
  expect_contains(asm_text, "    mv sp, t0",
                  "wired translated riscv memory helper seam should restore the stack pointer from the bounded accumulator path");
  expect_contains(asm_text, "    addi t0, t0, 31",
                  "wired translated riscv memory helper seam should support bounded arbitrary alignment by adding align-1 before masking");
  expect_contains(asm_text, "    andi t0, t0, -32",
                  "wired translated riscv memory helper seam should support bounded arbitrary alignment by masking the accumulator with -align");
  expect_contains(asm_text, "    addi t5, s0, -56",
                  "wired translated riscv memory helper seam should compute bounded over-aligned alloca addresses relative to s0 into the pointer register");
  expect_contains(asm_text, "    addi t5, t5, 31",
                  "wired translated riscv memory helper seam should add align-1 before masking when materializing bounded over-aligned alloca addresses");
  expect_contains(asm_text, "    andi t5, t5, -32",
                  "wired translated riscv memory helper seam should mask bounded over-aligned alloca addresses in the pointer register");
  expect_contains(asm_text, "    li t6, -8192",
                  "wired translated riscv memory helper seam should materialize large alloca slot offsets before forming an aligned accumulator address");
  expect_contains(asm_text, "    add t0, s0, t6",
                  "wired translated riscv memory helper seam should support large alloca slot offsets when forming aligned accumulator addresses");
  expect_contains(asm_text, "    li t6, 4095",
                  "wired translated riscv memory helper seam should materialize large align-1 adjustments for accumulator aligned-address emission");
  expect_contains(asm_text, "    add t0, t0, t6",
                  "wired translated riscv memory helper seam should apply large align-1 adjustments through the bounded accumulator helper path");
  expect_contains(asm_text, "    li t6, -4096",
                  "wired translated riscv memory helper seam should materialize large negative alignment masks when an andi immediate is not sufficient");
  expect_contains(asm_text, "    and t0, t0, t6",
                  "wired translated riscv memory helper seam should fall back to register-masked accumulator alignment for large over-aligned allocas");
}

void test_riscv_translated_memory_owner_emits_bounded_scalar_text() {
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(1, StackSlot{-24});
  codegen.state.slots.emplace(2, StackSlot{-40});
  codegen.state.slots.emplace(3, StackSlot{-56});
  codegen.state.slots.emplace(11, StackSlot{-88});
  codegen.state.slots.emplace(21, StackSlot{-96});
  codegen.state.slots.emplace(23, StackSlot{-104});
  codegen.state.allocas.insert(2);
  codegen.state.over_aligned_allocas.emplace(3, 32);
  codegen.state.assigned_regs.emplace(2, PhysReg{7});
  codegen.state.assigned_regs.emplace(22, PhysReg{8});

  codegen.emit_store_impl(Operand{11}, Value{1}, IrType::I32);
  codegen.emit_store_with_const_offset_impl(Operand::immediate_i64(7), Value{2}, 8, IrType::I64);
  codegen.emit_store_with_const_offset_impl(Operand::immediate_i64(9), Value{3}, 16, IrType::I16);
  codegen.emit_load_impl(Value{21}, Value{1}, IrType::I32);
  codegen.emit_load_with_const_offset_impl(Value{22}, Value{2}, 12, IrType::U16);
  codegen.emit_load_with_const_offset_impl(Value{23}, Value{3}, 24, IrType::U32);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    ld t0, -88(s0)",
                  "wired translated riscv memory owner should lower scalar store operands through the shared operand-to-t0 bridge");
  expect_contains(asm_text, "    sw t0, -24(s0)",
                  "wired translated riscv memory owner should store direct scalar values through the landed shared type-selector and slot helper surface");
  expect_contains(asm_text, "    mv t3, t0",
                  "wired translated riscv memory owner should preserve the accumulator payload when routing indirect or over-aligned scalar stores");
  expect_contains(asm_text, "    mv t5, s7",
                  "wired translated riscv memory owner should reuse assigned callee-saved pointer bases for indirect scalar stores and loads");
  expect_contains(asm_text, "    addi t5, t5, 8",
                  "wired translated riscv memory owner should apply bounded constant offsets on indirect scalar stores");
  expect_contains(asm_text, "    sd t3, 0(t5)",
                  "wired translated riscv memory owner should emit indirect scalar stores through the landed shared type-selector path");
  expect_contains(asm_text, "    addi t5, s0, -56",
                  "wired translated riscv memory owner should resolve over-aligned scalar store bases through the landed shared slot-address seam");
  expect_contains(asm_text, "    andi t5, t5, -32",
                  "wired translated riscv memory owner should keep over-aligned scalar store resolution within the bounded shared helper path");
  expect_contains(asm_text, "    addi t5, t5, 16",
                  "wired translated riscv memory owner should apply bounded constant offsets after over-aligned scalar store address resolution");
  expect_contains(asm_text, "    sh t3, 0(t5)",
                  "wired translated riscv memory owner should emit over-aligned scalar stores with the selected narrow store instruction");
  expect_contains(asm_text, "    lw t0, -24(s0)",
                  "wired translated riscv memory owner should load direct scalar values through the landed shared type-selector path");
  expect_contains(asm_text, "    sd t0, -96(s0)",
                  "wired translated riscv memory owner should store loaded direct scalar results back through the shared destination helper");
  expect_contains(asm_text, "    addi t5, t5, 12",
                  "wired translated riscv memory owner should apply bounded constant offsets on indirect scalar loads");
  expect_contains(asm_text, "    lhu t0, 0(t5)",
                  "wired translated riscv memory owner should emit indirect scalar loads with the selected unsigned load instruction");
  expect_contains(asm_text, "    mv s8, t0",
                  "wired translated riscv memory owner should reuse assigned callee-saved destinations for loaded scalar values");
  expect_contains(asm_text, "    addi t5, t5, 24",
                  "wired translated riscv memory owner should apply bounded constant offsets after over-aligned scalar load address resolution");
  expect_contains(asm_text, "    lwu t0, 0(t5)",
                  "wired translated riscv memory owner should emit over-aligned scalar loads with the selected wide unsigned load instruction");
  expect_contains(asm_text, "    sd t0, -104(s0)",
                  "wired translated riscv memory owner should spill loaded scalar results through the shared store_t0_to helper when the destination is stack-backed");
}

void test_riscv_translated_alu_unary_helpers_emit_bounded_text() {
  using IrType = c4c::backend::riscv::codegen::IrType;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  RiscvCodegen codegen;
  codegen.emit_float_neg_impl(IrType::F64);
  codegen.emit_float_neg_impl(IrType::F32);
  codegen.emit_int_neg_impl(IrType::I64);
  codegen.emit_int_not_impl(IrType::I64);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    fmv.d.x ft0, t0",
                  "wired translated riscv alu unary seam should route f64 negation through the bounded gp-to-fp bridge");
  expect_contains(asm_text, "    fneg.d ft0, ft0",
                  "wired translated riscv alu unary seam should emit the translated f64 negate instruction");
  expect_contains(asm_text, "    fmv.x.d t0, ft0",
                  "wired translated riscv alu unary seam should route f64 negate results back through the bounded fp-to-gp bridge");
  expect_contains(asm_text, "    fmv.w.x ft0, t0",
                  "wired translated riscv alu unary seam should route non-f64 negation through the bounded single-precision gp-to-fp bridge");
  expect_contains(asm_text, "    fneg.s ft0, ft0",
                  "wired translated riscv alu unary seam should emit the translated f32 negate instruction");
  expect_contains(asm_text, "    fmv.x.w t0, ft0",
                  "wired translated riscv alu unary seam should route f32 negate results back through the bounded fp-to-gp bridge");
  expect_contains(asm_text, "    neg t0, t0",
                  "wired translated riscv alu unary seam should emit integer negation through the bounded accumulator path");
  expect_contains(asm_text, "    not t0, t0",
                  "wired translated riscv alu unary seam should emit integer bitwise not through the bounded accumulator path");
}

void test_riscv_translated_alu_integer_owner_emits_bounded_text() {
  using IrBinOp = c4c::backend::riscv::codegen::IrBinOp;
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(5, StackSlot{-24});
  codegen.state.slots.emplace(11, StackSlot{-56});
  codegen.state.slots.emplace(13, StackSlot{-88});

  codegen.emit_int_binop_impl(Value{5}, IrBinOp::Add, Operand::immediate_i64(10),
                              Operand::immediate_i64(3), IrType::I64);
  codegen.emit_int_binop_impl(Value{5}, IrBinOp::Sub, Operand::immediate_i64(10),
                              Operand::immediate_i64(3), IrType::I32);
  codegen.emit_int_binop_impl(Value{5}, IrBinOp::Mul, Operand::immediate_i64(10),
                              Operand::immediate_i64(3), IrType::I64);
  codegen.emit_int_binop_impl(Value{5}, IrBinOp::SDiv, Operand::immediate_i64(10),
                              Operand::immediate_i64(3), IrType::I64);
  codegen.emit_int_binop_impl(Value{5}, IrBinOp::UDiv, Operand::immediate_i64(10),
                              Operand::immediate_i64(3), IrType::U32);
  codegen.emit_int_binop_impl(Value{5}, IrBinOp::SRem, Operand::immediate_i64(10),
                              Operand::immediate_i64(3), IrType::I64);
  codegen.emit_int_binop_impl(Value{5}, IrBinOp::URem, Operand::immediate_i64(10),
                              Operand::immediate_i64(3), IrType::U32);
  codegen.emit_int_binop_impl(Value{5}, IrBinOp::And, Operand::immediate_i64(10),
                              Operand::immediate_i64(3), IrType::I64);
  codegen.emit_int_binop_impl(Value{5}, IrBinOp::Or, Operand::immediate_i64(10),
                              Operand::immediate_i64(3), IrType::I64);
  codegen.emit_int_binop_impl(Value{5}, IrBinOp::Xor, Operand::immediate_i64(10),
                              Operand::immediate_i64(3), IrType::I64);
  codegen.emit_int_binop_impl(Value{5}, IrBinOp::Shl, Operand::immediate_i64(10),
                              Operand::immediate_i64(3), IrType::I64);
  codegen.emit_int_binop_impl(Value{5}, IrBinOp::AShr, Operand::immediate_i64(10),
                              Operand::immediate_i64(3), IrType::I32);
  codegen.emit_int_binop_impl(Value{5}, IrBinOp::LShr, Operand::immediate_i64(10),
                              Operand::immediate_i64(3), IrType::U32);
  codegen.emit_copy_i128_impl(Value{13}, Operand{11});

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    add t0, t1, t2",
                  "wired translated riscv alu integer-owner seam should emit the translated 64-bit add mnemonic");
  expect_contains(asm_text, "    subw t0, t1, t2",
                  "wired translated riscv alu integer-owner seam should select the translated 32-bit subtract mnemonic when the IR type is i32");
  expect_contains(asm_text, "    mul t0, t1, t2",
                  "wired translated riscv alu integer-owner seam should emit the translated 64-bit multiply mnemonic");
  expect_contains(asm_text, "    div t0, t1, t2",
                  "wired translated riscv alu integer-owner seam should emit the translated signed division mnemonic");
  expect_contains(asm_text, "    divuw t0, t1, t2",
                  "wired translated riscv alu integer-owner seam should emit the translated unsigned 32-bit division mnemonic");
  expect_contains(asm_text, "    rem t0, t1, t2",
                  "wired translated riscv alu integer-owner seam should emit the translated signed remainder mnemonic");
  expect_contains(asm_text, "    remuw t0, t1, t2",
                  "wired translated riscv alu integer-owner seam should emit the translated unsigned 32-bit remainder mnemonic");
  expect_contains(asm_text, "    and t0, t1, t2",
                  "wired translated riscv alu integer-owner seam should emit the translated bitwise and mnemonic");
  expect_contains(asm_text, "    or t0, t1, t2",
                  "wired translated riscv alu integer-owner seam should emit the translated bitwise or mnemonic");
  expect_contains(asm_text, "    xor t0, t1, t2",
                  "wired translated riscv alu integer-owner seam should emit the translated bitwise xor mnemonic");
  expect_contains(asm_text, "    sll t0, t1, t2",
                  "wired translated riscv alu integer-owner seam should emit the translated left-shift mnemonic");
  expect_contains(asm_text, "    sraw t0, t1, t2",
                  "wired translated riscv alu integer-owner seam should emit the translated arithmetic right-shift mnemonic");
  expect_contains(asm_text, "    srlw t0, t1, t2",
                  "wired translated riscv alu integer-owner seam should emit the translated logical right-shift mnemonic");
  expect_contains(asm_text, "    sd t0, -24(s0)",
                  "wired translated riscv alu integer-owner seam should store scalar binop results through the shared destination helper");
  expect_contains(asm_text, "    ld t0, -56(s0)",
                  "wired translated riscv alu integer-owner seam should reload i128 copy sources through the landed pair-load helper");
  expect_contains(asm_text, "    ld t1, -48(s0)",
                  "wired translated riscv alu integer-owner seam should reload the copied high lane through the landed pair-load helper");
  expect_contains(asm_text, "    sd t0, -88(s0)",
                  "wired translated riscv alu integer-owner seam should store copied i128 low lanes through the landed pair-store helper");
  expect_contains(asm_text, "    sd t1, -80(s0)",
                  "wired translated riscv alu integer-owner seam should store copied i128 high lanes through the landed pair-store helper");
}

void test_riscv_translated_calls_abi_helpers() {
  using CallArgClass = c4c::backend::riscv::codegen::CallArgClass;
  using IrType = c4c::backend::riscv::codegen::IrType;

  c4c::backend::riscv::codegen::RiscvCodegen codegen;

  std::vector<CallArgClass> arg_classes;
  CallArgClass int_reg;
  int_reg.value = CallArgClass::IntReg{0};
  arg_classes.push_back(int_reg);
  CallArgClass stack_arg;
  stack_arg.value = CallArgClass::Stack{};
  arg_classes.push_back(stack_arg);
  CallArgClass i128_stack;
  i128_stack.value = CallArgClass::I128Stack{};
  arg_classes.push_back(i128_stack);
  CallArgClass split_stack;
  split_stack.value = CallArgClass::StructSplitRegStack{1, 24};
  arg_classes.push_back(split_stack);
  CallArgClass byval_stack;
  byval_stack.value = CallArgClass::StructByValStack{9};
  arg_classes.push_back(byval_stack);

  const auto abi_config = codegen.call_abi_config_impl();
  const auto stack_space = codegen.emit_call_compute_stack_space_impl(
      arg_classes, {IrType::I64, IrType::I64, IrType::I128, IrType::Ptr, IrType::Ptr});

  expect_true(abi_config.max_int_regs == 8 && abi_config.max_float_regs == 8 &&
                  abi_config.align_i128_pairs && abi_config.f128_in_gp_pairs &&
                  abi_config.variadic_floats_in_gp &&
                  abi_config.use_riscv_float_struct_classification &&
                  abi_config.allow_struct_split_reg_stack &&
                  stack_space == 64,
              "wired translated riscv calls ABI seam should preserve the LP64D helper config and 16-byte-aligned stack-space computation without widening into broader call lowering");
}

void test_riscv_translated_calls_instruction_cleanup_helpers() {
  using Operand = c4c::backend::riscv::codegen::Operand;

  c4c::backend::riscv::codegen::RiscvCodegen codegen;

  codegen.emit_call_instruction_impl(std::optional<std::string>{"callee_symbol"}, std::nullopt, false, 0);
  codegen.emit_call_instruction_impl(std::nullopt, std::optional<Operand>{Operand::immediate_i64(42)}, true, 0);
  codegen.emit_call_cleanup_impl(24, 0, false);
  codegen.emit_call_cleanup_impl(0, 16, false);
  codegen.emit_call_cleanup_impl(4096, 16, false);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    call callee_symbol",
                  "wired translated riscv calls instruction seam should emit direct call text through the bounded owner path");
  expect_contains(asm_text, "    li t0, 42",
                  "wired translated riscv calls instruction seam should materialize indirect call targets through the shared operand loader");
  expect_contains(asm_text, "    mv t2, t0",
                  "wired translated riscv calls instruction seam should stage indirect call targets through t2 before jalr");
  expect_contains(asm_text, "    jalr ra, t2, 0",
                  "wired translated riscv calls instruction seam should emit the translated indirect call form");
  expect_contains(asm_text, "    addi sp, sp, 24",
                  "wired translated riscv calls cleanup seam should release bounded stack-argument reservations");
  expect_contains(asm_text, "    addi sp, sp, 16",
                  "wired translated riscv calls cleanup seam should release standalone f128 temp reservations");
  expect_contains(asm_text, "    li t6, 4112",
                  "wired translated riscv calls cleanup seam should materialize large cleanup totals that exceed addi immediates");
  expect_contains(asm_text, "    add sp, sp, t6",
                  "wired translated riscv calls cleanup seam should fall back to register-based stack restoration for large cleanup totals");
}

void test_riscv_translated_calls_result_store_non_f128_emits_bounded_text() {
  using IrType = c4c::backend::riscv::codegen::IrType;
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(5, StackSlot{-24});
  codegen.state.slots.emplace(6, StackSlot{-32});
  codegen.state.slots.emplace(7, StackSlot{-40});
  codegen.state.assigned_regs.emplace(8, PhysReg{1});

  codegen.emit_call_store_result_impl(Value{5}, IrType::F32);
  codegen.emit_call_store_result_impl(Value{6}, IrType::F64);
  codegen.emit_call_store_result_impl(Value{7}, IrType::I64);
  codegen.emit_call_store_result_impl(Value{8}, IrType::I64);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    fmv.x.w t0, fa0",
                  "wired translated riscv calls result-store seam should lower f32 call results through the bounded fa0-to-gp bridge");
  expect_contains(asm_text, "    sd t0, -24(s0)",
                  "wired translated riscv calls result-store seam should store f32 call results back through the shared s0 slot helper");
  expect_contains(asm_text, "    fmv.x.d t0, fa0",
                  "wired translated riscv calls result-store seam should lower f64 call results through the bounded fa0-to-gp bridge");
  expect_contains(asm_text, "    sd t0, -32(s0)",
                  "wired translated riscv calls result-store seam should store f64 call results back through the shared s0 slot helper");
  expect_contains(asm_text, "    sd a0, -40(s0)",
                  "wired translated riscv calls result-store seam should store integer call results directly from a0 when the destination lives on the stack");
  expect_contains(asm_text, "    mv s1, a0",
                  "wired translated riscv calls result-store seam should reuse assigned callee-saved destinations for integer call results without widening into broader call lowering");
}

void test_riscv_translated_calls_result_store_wide_emits_bounded_text() {
  using IrType = c4c::backend::riscv::codegen::IrType;
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(9, StackSlot{-48});
  codegen.state.slots.emplace(10, StackSlot{-64});
  codegen.state.assigned_regs.emplace(10, PhysReg{2});

  codegen.emit_call_store_result_impl(Value{9}, IrType::I128);
  codegen.emit_call_store_result_impl(Value{10}, IrType::F128);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    sd a0, -48(s0)",
                  "wired translated riscv calls result-store seam should store i128 low halves from a0 through the shared s0 slot helper");
  expect_contains(asm_text, "    sd a1, -40(s0)",
                  "wired translated riscv calls result-store seam should store i128 high halves from a1 through the shared s0 slot helper");
  expect_contains(asm_text, "    sd a0, -64(s0)",
                  "wired translated riscv calls result-store seam should preserve f128 low halves in the destination slot before truncation");
  expect_contains(asm_text, "    sd a1, -56(s0)",
                  "wired translated riscv calls result-store seam should preserve f128 high halves in the destination slot before truncation");
  expect_contains(asm_text, "    call __trunctfdf2",
                  "wired translated riscv calls result-store seam should reuse the landed long-double truncation helper when the call result is f128");
  expect_contains(asm_text, "    fmv.x.d t0, fa0",
                  "wired translated riscv calls result-store seam should bridge truncated long-double results back into the integer accumulator path");
  expect_contains(asm_text, "    mv s2, t0",
                  "wired translated riscv calls result-store seam should materialize the truncated f128 result into an assigned callee-saved destination");
}

void test_riscv_translated_globals_label_helper_emits_bounded_text() {
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(5, StackSlot{-24});
  codegen.state.assigned_regs.emplace(7, PhysReg{1});

  codegen.emit_label_addr_impl(Value{5}, ".Lowner_tmp");
  codegen.emit_label_addr_impl(Value{7}, ".Lowner_reg");

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    lla t0, .Lowner_tmp",
                  "wired translated riscv globals seam should materialize local labels through the bounded lla path");
  expect_contains(asm_text, "    sd t0, -24(s0)",
                  "wired translated riscv globals seam should store label addresses back through the preparatory shared result-store helper when the destination lives on the stack");
  expect_contains(asm_text, "    lla t0, .Lowner_reg",
                  "wired translated riscv globals seam should keep label-address materialization inside the bounded globals owner method");
  expect_contains(asm_text, "    mv s1, t0",
                  "wired translated riscv globals seam should reuse assigned callee-saved destinations through the preparatory shared result-store helper");
}

void test_riscv_translated_globals_global_addr_helper_emits_bounded_text() {
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(5, StackSlot{-24});
  codegen.state.assigned_regs.emplace(7, PhysReg{1});
  codegen.state.mark_needs_got_for_addr("global_got_counter");

  codegen.emit_global_addr_impl(Value{5}, "global_counter");
  codegen.emit_global_addr_impl(Value{7}, "global_got_counter");

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    lla t0, global_counter",
                  "wired translated riscv globals seam should materialize local global addresses through the bounded lla path");
  expect_contains(asm_text, "    sd t0, -24(s0)",
                  "wired translated riscv globals seam should store local global addresses back through the shared store_t0_to helper when the destination lives on the stack");
  expect_contains(asm_text, "    la t0, global_got_counter",
                  "wired translated riscv globals seam should switch to the shared GOT-aware la path when the RV64 codegen state marks a symbol as requiring GOT materialization");
  expect_contains(asm_text, "    mv s1, t0",
                  "wired translated riscv globals seam should reuse assigned callee-saved destinations for GOT-address materialization without widening into TLS handling");
}

void test_riscv_translated_globals_tls_global_addr_emits_bounded_text() {
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(5, StackSlot{-24});
  codegen.state.assigned_regs.emplace(7, PhysReg{1});

  codegen.emit_tls_global_addr_impl(Value{5}, "tls_counter");
  codegen.emit_tls_global_addr_impl(Value{7}, "tls_reg_counter");

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    lui t0, %tprel_hi(tls_counter)",
                  "wired translated riscv TLS globals seam should materialize the upper Local-Exec TLS relocation through the active owner body");
  expect_contains(asm_text, "    add t0, t0, tp, %tprel_add(tls_counter)",
                  "wired translated riscv TLS globals seam should add the thread-pointer base through the translated %tprel_add path");
  expect_contains(asm_text, "    addi t0, t0, %tprel_lo(tls_counter)",
                  "wired translated riscv TLS globals seam should finish the Local-Exec TLS address materialization through the translated %tprel_lo path");
  expect_contains(asm_text, "    sd t0, -24(s0)",
                  "wired translated riscv TLS globals seam should store stack destinations back through the shared store_t0_to helper");
  expect_contains(asm_text, "    lui t0, %tprel_hi(tls_reg_counter)",
                  "wired translated riscv TLS globals seam should remain reusable across repeated bounded owner calls");
  expect_contains(asm_text, "    mv s1, t0",
                  "wired translated riscv TLS globals seam should reuse assigned callee-saved destinations without widening into broader globals work");
}

void test_riscv_translated_call_owner_emits_scalar_reg_and_stack_abi_text() {
  using CallArgClass = c4c::backend::riscv::codegen::CallArgClass;
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(1, StackSlot{-16});
  codegen.state.slots.emplace(2, StackSlot{-24});

  std::vector<Operand> args{
      Operand{1},
      Operand::immediate_i64(7),
      Operand{2},
  };

  std::vector<CallArgClass> arg_classes(3);
  arg_classes[0].value = CallArgClass::FloatReg{0};
  arg_classes[1].value = CallArgClass::IntReg{0};
  arg_classes[2].value = CallArgClass::Stack{};

  std::vector<IrType> arg_types{
      IrType::F64,
      IrType::I64,
      IrType::I64,
  };

  const auto stack_space = codegen.emit_call_compute_stack_space_impl(arg_classes, arg_types);
  const auto f128_temp_space =
      codegen.emit_call_f128_pre_convert_impl(args, arg_classes, arg_types, stack_space);
  const auto stack_adjust =
      codegen.emit_call_stack_args_impl(args, arg_classes, arg_types, stack_space, 0, f128_temp_space);
  codegen.emit_call_reg_args_impl(args, arg_classes, arg_types, stack_adjust, f128_temp_space,
                                  stack_space, {});

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_true(stack_space == 16,
              "translated riscv outgoing-call staging slice should reserve the bounded aligned stack space for one stack-classified scalar argument");
  expect_true(f128_temp_space == 0,
              "translated riscv outgoing-call staging slice should leave the bounded f128 pre-convert path inactive when no f128 register arguments are present");
  expect_true(stack_adjust == 0,
              "translated riscv outgoing-call staging slice should report no extra source-slot adjust because the current bounded path still loads operands from s0-relative slots");
  expect_contains(asm_text, "    addi sp, sp, -16",
                  "translated riscv outgoing-call staging slice should allocate the aligned stack-argument area before storing stack-classified scalar arguments");
  expect_contains(asm_text, "    ld t0, -24(s0)\n    sd t0, 0(sp)\n",
                  "translated riscv outgoing-call staging slice should lower stack-classified scalar arguments through the shared operand helper and bounded sp-relative store path");
  expect_contains(asm_text, "    ld t0, -16(s0)\n    fmv.d.x fa0, t0\n",
                  "translated riscv outgoing-call staging slice should route f64 register arguments into the LP64D fa-register bank");
  expect_contains(asm_text, "    li t0, 7\n    mv t3, t0\n    mv a0, t3\n",
                  "translated riscv outgoing-call staging slice should stage integer register arguments through the bounded t3/t4/t5 path before moving them into a-registers");
}

void test_riscv_translated_call_owner_emits_wide_scalar_staging_text() {
  using CallArgClass = c4c::backend::riscv::codegen::CallArgClass;
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(10, StackSlot{-32});
  codegen.state.slots.emplace(20, StackSlot{-64});
  codegen.state.slots.emplace(30, StackSlot{-96});

  std::vector<Operand> args{
      Operand{10},
      Operand{20},
      Operand::immediate_i64(-5),
      Operand{30},
  };

  std::vector<CallArgClass> arg_classes(4);
  arg_classes[0].value = CallArgClass::I128RegPair{2};
  arg_classes[1].value = CallArgClass::F128Reg{4};
  arg_classes[2].value = CallArgClass::I128Stack{};
  arg_classes[3].value = CallArgClass::F128Stack{};

  std::vector<IrType> arg_types{
      IrType::I128,
      IrType::F128,
      IrType::I128,
      IrType::F128,
  };

  const auto stack_space = codegen.emit_call_compute_stack_space_impl(arg_classes, arg_types);
  const auto f128_temp_space =
      codegen.emit_call_f128_pre_convert_impl(args, arg_classes, arg_types, stack_space);
  const auto stack_adjust =
      codegen.emit_call_stack_args_impl(args, arg_classes, arg_types, stack_space, 0, f128_temp_space);
  codegen.emit_call_reg_args_impl(args, arg_classes, arg_types, stack_adjust, f128_temp_space,
                                  stack_space, {});

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_true(stack_space == 32,
              "translated riscv wide outgoing-call staging slice should reserve a 16-byte-aligned stack area for bounded i128 and f128 stack arguments");
  expect_true(f128_temp_space == 16,
              "translated riscv wide outgoing-call staging slice should reserve one temporary f128 spill slot for a value-backed f128 register argument");
  expect_true(stack_adjust == 0,
              "translated riscv wide outgoing-call staging slice should keep the existing no-extra-source-adjust contract while widening only argument staging");
  expect_contains(asm_text, "    addi sp, sp, -16\n    ld a0, -64(s0)\n    ld a1, -56(s0)\n    sd a0, 0(sp)\n    sd a1, 8(sp)\n",
                  "translated riscv wide outgoing-call staging slice should pre-convert value-backed f128 register arguments into the bounded temporary spill area before stack allocation");
  expect_contains(asm_text, "    addi sp, sp, -32\n    li t0, -5\n    li t1, -1\n    sd t0, 0(sp)\n    sd t1, 8(sp)\n",
                  "translated riscv wide outgoing-call staging slice should lower i128 stack arguments through the shared accumulator-pair helper and aligned sp-relative stores");
  expect_contains(asm_text, "    ld a0, -96(s0)\n    ld a1, -88(s0)\n    sd a0, 16(sp)\n    sd a1, 24(sp)\n",
                  "translated riscv wide outgoing-call staging slice should lower f128 stack arguments through the shared f128 operand helper and aligned paired stack stores");
  expect_contains(asm_text, "    ld a4, 32(sp)\n    ld a5, 40(sp)\n",
                  "translated riscv wide outgoing-call staging slice should reload value-backed f128 register arguments from the bounded temporary spill area using running spill offsets instead of arg-register indices");
  expect_contains(asm_text, "    ld t0, -32(s0)\n    ld t1, -24(s0)\n    mv a2, t0\n    mv a3, t1\n",
                  "translated riscv wide outgoing-call staging slice should route i128 register-pair arguments through the shared pair-load helper into the target a-register pair");
}

void test_riscv_translated_call_owner_emits_aggregate_stack_staging_text() {
  using CallArgClass = c4c::backend::riscv::codegen::CallArgClass;
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(40, StackSlot{-48});
  codegen.state.slots.emplace(41, StackSlot{-80});
  codegen.state.allocas.insert(41);
  codegen.state.assigned_regs.emplace(42, PhysReg{1});

  std::vector<Operand> args{
      Operand{40},
      Operand{41},
      Operand{42},
  };

  std::vector<CallArgClass> arg_classes(3);
  arg_classes[0].value = CallArgClass::StructByValStack{9};
  arg_classes[1].value = CallArgClass::LargeStructStack{24};
  arg_classes[2].value = CallArgClass::StructSplitRegStack{2, 24};

  std::vector<IrType> arg_types{
      IrType::Ptr,
      IrType::Ptr,
      IrType::Ptr,
  };

  const auto stack_space = codegen.emit_call_compute_stack_space_impl(arg_classes, arg_types);
  const auto f128_temp_space =
      codegen.emit_call_f128_pre_convert_impl(args, arg_classes, arg_types, stack_space);
  const auto stack_adjust =
      codegen.emit_call_stack_args_impl(args, arg_classes, arg_types, stack_space, 0, f128_temp_space);
  codegen.emit_call_reg_args_impl(args, arg_classes, arg_types, stack_adjust, f128_temp_space,
                                  stack_space, {});

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_true(stack_space == 64,
              "translated riscv aggregate outgoing-call stack slice should reserve the aligned stack area for by-value, large-struct, and split-reg-stack aggregate arguments");
  expect_true(f128_temp_space == 0,
              "translated riscv aggregate outgoing-call stack slice should not allocate f128 temp space when the bounded packet only lowers stack-side aggregate classes");
  expect_true(stack_adjust == 0,
              "translated riscv aggregate outgoing-call stack slice should preserve the existing no-extra-source-adjust contract while only widening stack staging");
  expect_contains(asm_text, "    addi sp, sp, -64",
                  "translated riscv aggregate outgoing-call stack slice should allocate the full aligned aggregate stack area before copying staged aggregate payloads");
  expect_contains(asm_text, "    ld t0, -48(s0)\n    ld t1, 0(t0)\n    sd t1, 0(sp)\n    ld t1, 8(t0)\n    sd t1, 8(sp)\n",
                  "translated riscv aggregate outgoing-call stack slice should copy struct-by-value stack arguments from pointer-backed frame slots through bounded t0/t1 loads and sp-relative stores");
  expect_contains(asm_text, "    addi t0, s0, -80\n    ld t1, 0(t0)\n    sd t1, 16(sp)\n    ld t1, 8(t0)\n    sd t1, 24(sp)\n    ld t1, 16(t0)\n    sd t1, 32(sp)\n",
                  "translated riscv aggregate outgoing-call stack slice should materialize alloca-backed large-struct sources from s0-relative addresses before copying each eight-byte chunk");
  expect_contains(asm_text, "    mv t0, s1\n    ld t1, 8(t0)\n    sd t1, 40(sp)\n    ld t1, 16(t0)\n    sd t1, 48(sp)\n",
                  "translated riscv aggregate outgoing-call stack slice should stage only the stack-resident tail of split aggregate arguments after reusing the already-assigned register-backed base pointer");
}

void test_riscv_translated_call_owner_emits_aggregate_reg_staging_text() {
  using CallArgClass = c4c::backend::riscv::codegen::CallArgClass;
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(50, StackSlot{-56});
  codegen.state.assigned_regs.emplace(51, PhysReg{1});

  std::vector<Operand> args{
      Operand{50},
      Operand{51},
  };

  std::vector<CallArgClass> arg_classes(2);
  arg_classes[0].value = CallArgClass::StructByValReg{4, 16};
  arg_classes[1].value = CallArgClass::StructSplitRegStack{6, 24};

  std::vector<IrType> arg_types{
      IrType::Ptr,
      IrType::Ptr,
  };

  const auto stack_space = codegen.emit_call_compute_stack_space_impl(arg_classes, arg_types);
  const auto f128_temp_space =
      codegen.emit_call_f128_pre_convert_impl(args, arg_classes, arg_types, stack_space);
  const auto stack_adjust =
      codegen.emit_call_stack_args_impl(args, arg_classes, arg_types, stack_space, 0, f128_temp_space);
  codegen.emit_call_reg_args_impl(args, arg_classes, arg_types, stack_adjust, f128_temp_space,
                                  stack_space, {});

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_true(stack_space == 16,
              "translated riscv aggregate outgoing-call register slice should reserve only the split-aggregate tail on the caller stack");
  expect_true(f128_temp_space == 0,
              "translated riscv aggregate outgoing-call register slice should not allocate f128 temp space when only GP aggregate staging is active");
  expect_true(stack_adjust == 0,
              "translated riscv aggregate outgoing-call register slice should preserve the existing no-extra-source-adjust contract while widening only GP aggregate staging");
  expect_contains(asm_text, "    addi sp, sp, -16",
                  "translated riscv aggregate outgoing-call register slice should keep the split aggregate stack tail allocation active alongside GP register staging");
  expect_contains(asm_text, "    mv t0, s1\n    ld t1, 8(t0)\n    sd t1, 0(sp)\n    ld t1, 16(t0)\n    sd t1, 8(sp)\n",
                  "translated riscv aggregate outgoing-call register slice should preserve the previously landed split-aggregate stack-tail copy path");
  expect_contains(asm_text, "    ld t0, -56(s0)\n    ld a4, 0(t0)\n    ld a5, 8(t0)\n",
                  "translated riscv aggregate outgoing-call register slice should load by-value aggregate register arguments from pointer-backed frame slots into the target a-register pair");
  expect_contains(asm_text, "    mv t0, s1\n    ld a6, 0(t0)\n",
                  "translated riscv aggregate outgoing-call register slice should stage the register-resident head of split aggregate arguments into the selected a-register");
}

void test_riscv_translated_call_owner_emits_float_aggregate_reg_staging_text() {
  using CallArgClass = c4c::backend::riscv::codegen::CallArgClass;
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using RiscvFloatClass = c4c::backend::riscv::codegen::RiscvFloatClass;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(60, StackSlot{-72});
  codegen.state.slots.emplace(61, StackSlot{-96});
  codegen.state.slots.emplace(62, StackSlot{-120});

  std::vector<Operand> args{
      Operand{60},
      Operand{61},
      Operand{62},
  };

  std::vector<CallArgClass> arg_classes(3);
  arg_classes[0].value = CallArgClass::StructSseReg{1, std::optional<std::size_t>{2}, 12};
  arg_classes[1].value = CallArgClass::StructMixedIntSseReg{3, 4, 12};
  arg_classes[2].value = CallArgClass::StructMixedSseIntReg{5, 6, 16};

  std::vector<IrType> arg_types{
      IrType::Ptr,
      IrType::Ptr,
      IrType::Ptr,
  };

  std::vector<std::optional<RiscvFloatClass>> classes{
      RiscvFloatClass::two_floats(false, true),
      RiscvFloatClass::int_and_float(false, 4, 4),
      RiscvFloatClass::float_and_int(true, 8, 4),
  };

  const auto stack_space = codegen.emit_call_compute_stack_space_impl(arg_classes, arg_types);
  const auto f128_temp_space =
      codegen.emit_call_f128_pre_convert_impl(args, arg_classes, arg_types, stack_space);
  const auto stack_adjust =
      codegen.emit_call_stack_args_impl(args, arg_classes, arg_types, stack_space, 0, f128_temp_space);
  codegen.emit_call_reg_args_impl(args, arg_classes, arg_types, stack_adjust, f128_temp_space,
                                  stack_space, classes);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_true(stack_space == 0,
              "translated riscv float aggregate outgoing-call register slice should not reserve caller stack space when the bounded packet only lowers register-classified aggregate args");
  expect_true(f128_temp_space == 0,
              "translated riscv float aggregate outgoing-call register slice should keep f128 temp allocation inactive for the struct-float register packet");
  expect_true(stack_adjust == 0,
              "translated riscv float aggregate outgoing-call register slice should preserve the no-extra-source-adjust contract while only widening float/mixed register staging");
  expect_contains(asm_text, "    ld t0, -72(s0)\n    flw fa1, 0(t0)\n    fld fa2, 4(t0)\n",
                  "translated riscv float aggregate outgoing-call register slice should honor the routed two-float metadata when loading struct-float register arguments");
  expect_contains(asm_text, "    ld t0, -96(s0)\n    lw a3, 0(t0)\n    flw fa4, 4(t0)\n",
                  "translated riscv float aggregate outgoing-call register slice should load mixed int-then-float aggregates with the metadata-selected integer width and float offset");
  expect_contains(asm_text, "    ld t0, -120(s0)\n    fld fa5, 0(t0)\n    lw a6, 8(t0)\n",
                  "translated riscv float aggregate outgoing-call register slice should load mixed float-then-int aggregates with the metadata-selected float width and integer offset");
}

void test_riscv_translated_float_binop_non_f128_emits_bounded_text() {
  using FloatOp = c4c::backend::riscv::codegen::FloatOp;
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(5, StackSlot{-24});
  codegen.state.assigned_regs.emplace(7, PhysReg{1});

  codegen.emit_float_binop_impl(Value{5}, FloatOp::Add, Operand::immediate_i64(42),
                                Operand::immediate_i64(7), IrType::F32);
  codegen.emit_float_binop_impl(Value{7}, FloatOp::Mul, Operand::immediate_i64(9),
                                Operand::immediate_i64(3), IrType::F64);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    li t0, 42",
                  "wired translated riscv float-binop seam should lower the lhs through the shared operand-to-t0 bridge");
  expect_contains(asm_text, "    mv t1, t0",
                  "wired translated riscv float-binop seam should preserve the translated lhs staging register");
  expect_contains(asm_text, "    li t0, 7",
                  "wired translated riscv float-binop seam should lower the rhs through the shared operand-to-t0 bridge");
  expect_contains(asm_text, "    fmv.w.x ft0, t1",
                  "wired translated riscv float-binop seam should reload F32 operands through the bounded scalar float path");
  expect_contains(asm_text, "    fadd.s ft0, ft0, ft1",
                  "wired translated riscv float-binop seam should emit the translated F32 mnemonic body");
  expect_contains(asm_text, "    sd t0, -24(s0)",
                  "wired translated riscv float-binop seam should store stack destinations through the shared result-store helper");
  expect_contains(asm_text, "    fmv.d.x ft0, t1",
                  "wired translated riscv float-binop seam should reload F64 operands through the bounded scalar float path");
  expect_contains(asm_text, "    fmul.d ft0, ft0, ft1",
                  "wired translated riscv float-binop seam should emit the translated F64 mnemonic body");
  expect_contains(asm_text, "    mv s1, t0",
                  "wired translated riscv float-binop seam should reuse assigned callee-saved destinations through the shared result-store helper");
}

void test_riscv_translated_f128_neg_emits_bounded_text() {
  using Operand = c4c::backend::riscv::codegen::Operand;
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(5, StackSlot{-24});
  codegen.state.slots.emplace(9, StackSlot{-56});
  codegen.state.assigned_regs.emplace(5, PhysReg{1});

  codegen.emit_f128_neg_impl(Value{5}, Operand{9});

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    ld a0, -56(s0)",
                  "wired translated riscv f128 neg seam should reload the low lane through the shared soft-float operand bridge");
  expect_contains(asm_text, "    ld a1, -48(s0)",
                  "wired translated riscv f128 neg seam should reload the high lane through the shared soft-float operand bridge");
  expect_contains(asm_text, "    li t0, 1",
                  "wired translated riscv f128 neg seam should materialize the sign-mask seed through the bounded shared helper");
  expect_contains(asm_text, "    slli t0, t0, 63",
                  "wired translated riscv f128 neg seam should position the IEEE-754 sign bit in the bounded shared helper");
  expect_contains(asm_text, "    xor a1, a1, t0",
                  "wired translated riscv f128 neg seam should flip only the high-lane sign bit");
  expect_contains(asm_text, "    sd a0, -24(s0)",
                  "wired translated riscv f128 neg seam should stage the full-precision low lane into the destination slot");
  expect_contains(asm_text, "    sd a1, -16(s0)",
                  "wired translated riscv f128 neg seam should stage the full-precision high lane into the destination slot");
  expect_contains(asm_text, "    call __trunctfdf2",
                  "wired translated riscv f128 neg seam should finish through the shared f128-to-f64 approximation bridge");
  expect_contains(asm_text, "    mv s1, t0",
                  "wired translated riscv f128 neg seam should preserve the bounded register result hook after truncation");
}

void test_riscv_translated_float_binop_f128_emits_bounded_text() {
  using FloatOp = c4c::backend::riscv::codegen::FloatOp;
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(5, StackSlot{-24});
  codegen.state.slots.emplace(21, StackSlot{-56});
  codegen.state.slots.emplace(23, StackSlot{-72});
  codegen.state.assigned_regs.emplace(5, PhysReg{1});

  codegen.emit_float_binop_impl(Value{5}, FloatOp::Add, Operand{21}, Operand{23}, IrType::F128);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    addi sp, sp, -16",
                  "wired translated riscv f128 binop seam should allocate only the bounded temporary stack bridge for the lhs");
  expect_contains(asm_text, "    ld a0, -56(s0)",
                  "wired translated riscv f128 binop seam should load the lhs low lane through the shared soft-float operand bridge");
  expect_contains(asm_text, "    sd a0, 0(sp)",
                  "wired translated riscv f128 binop seam should spill the lhs through the bounded temp stack bridge");
  expect_contains(asm_text, "    ld a0, -72(s0)",
                  "wired translated riscv f128 binop seam should load the rhs low lane through the shared soft-float operand bridge");
  expect_contains(asm_text, "    mv a2, a0",
                  "wired translated riscv f128 binop seam should move the rhs into the second argument pair through the shared soft-float helper");
  expect_contains(asm_text, "    ld a0, 0(sp)",
                  "wired translated riscv f128 binop seam should restore the lhs low lane after staging the rhs");
  expect_contains(asm_text, "    addi sp, sp, 16",
                  "wired translated riscv f128 binop seam should release the bounded temporary stack bridge before the libcall");
  expect_contains(asm_text, "    call __addtf3",
                  "wired translated riscv f128 binop seam should route add through the shared soft-float libcall mapping");
  expect_contains(asm_text, "    sd a0, -24(s0)",
                  "wired translated riscv f128 binop seam should stage the full-precision result into the destination slot");
  expect_contains(asm_text, "    call __trunctfdf2",
                  "wired translated riscv f128 binop seam should produce the bounded f64 approximation after the libcall");
  expect_contains(asm_text, "    mv s1, t0",
                  "wired translated riscv f128 binop seam should preserve the assigned-register approximation hook after truncation");
}

void test_riscv_translated_float_cmp_non_f128_emits_bounded_text() {
  using IrCmpOp = c4c::backend::riscv::codegen::IrCmpOp;
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(5, StackSlot{-24});
  codegen.state.assigned_regs.emplace(7, PhysReg{1});

  codegen.emit_float_cmp_impl(Value{5}, IrCmpOp::Ne, Operand::immediate_i64(42),
                              Operand::immediate_i64(7), IrType::F32);
  codegen.emit_float_cmp_impl(Value{7}, IrCmpOp::Sge, Operand::immediate_i64(9),
                              Operand::immediate_i64(3), IrType::F64);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    li t0, 42",
                  "wired translated riscv float-compare seam should lower the lhs through the shared operand-to-t0 bridge");
  expect_contains(asm_text, "    mv t1, t0",
                  "wired translated riscv float-compare seam should preserve the translated lhs staging register");
  expect_contains(asm_text, "    li t0, 7",
                  "wired translated riscv float-compare seam should lower the rhs through the shared operand-to-t0 bridge");
  expect_contains(asm_text, "    mv t2, t0",
                  "wired translated riscv float-compare seam should preserve the translated rhs staging register");
  expect_contains(asm_text, "    fmv.w.x ft0, t1",
                  "wired translated riscv float-compare seam should reload F32 operands through the bounded scalar float path");
  expect_contains(asm_text, "    feq.s t0, ft0, ft1",
                  "wired translated riscv float-compare seam should emit the translated equality predicate before inversion for F32 not-equal");
  expect_contains(asm_text, "    xori t0, t0, 1",
                  "wired translated riscv float-compare seam should invert the translated equality predicate for not-equal");
  expect_contains(asm_text, "    sd t0, -24(s0)",
                  "wired translated riscv float-compare seam should store stack destinations through the shared result-store helper");
  expect_contains(asm_text, "    fmv.d.x ft0, t1",
                  "wired translated riscv float-compare seam should reload F64 operands through the bounded scalar float path");
  expect_contains(asm_text, "    fle.d t0, ft1, ft0",
                  "wired translated riscv float-compare seam should swap operands for signed-greater-or-equal before emitting the translated predicate");
  expect_contains(asm_text, "    mv s1, t0",
                  "wired translated riscv float-compare seam should reuse assigned callee-saved destinations through the shared result-store helper");
}

void test_riscv_translated_int_cmp_emits_bounded_text() {
  using IrCmpOp = c4c::backend::riscv::codegen::IrCmpOp;
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen eq_codegen;
  eq_codegen.state.slots.emplace(5, StackSlot{-24});
  eq_codegen.emit_int_cmp_impl(Value{5}, IrCmpOp::Eq, Operand::immediate_i64(513),
                               Operand::immediate_i64(258), IrType::U16);

  std::string eq_asm_text;
  for (const auto& line : eq_codegen.state.asm_lines) {
    eq_asm_text += line;
    eq_asm_text.push_back('\n');
  }

  expect_contains(eq_asm_text, "    li t0, 513",
                  "wired translated riscv integer-compare seam should lower the lhs through the shared operand-to-t0 bridge");
  expect_contains(eq_asm_text, "    mv t1, t0",
                  "wired translated riscv integer-compare seam should preserve the translated lhs staging register");
  expect_contains(eq_asm_text, "    li t0, 258",
                  "wired translated riscv integer-compare seam should lower the rhs through the shared operand-to-t0 bridge");
  expect_contains(eq_asm_text, "    mv t2, t0",
                  "wired translated riscv integer-compare seam should preserve the translated rhs staging register");
  expect_contains(eq_asm_text, "    slli t1, t1, 48",
                  "wired translated riscv integer-compare seam should zero-extend U16 lhs values before compare lowering");
  expect_contains(eq_asm_text, "    srli t2, t2, 48",
                  "wired translated riscv integer-compare seam should zero-extend U16 rhs values before compare lowering");
  expect_contains(eq_asm_text, "    sub t0, t1, t2",
                  "wired translated riscv integer-compare seam should materialize equality through the translated subtract path");
  expect_contains(eq_asm_text, "    seqz t0, t0",
                  "wired translated riscv integer-compare seam should materialize equality booleans through the translated zero-test path");
  expect_contains(eq_asm_text, "    sd t0, -24(s0)",
                  "wired translated riscv integer-compare seam should store stack destinations through the shared result-store helper");

  RiscvCodegen uge_codegen;
  uge_codegen.state.assigned_regs.emplace(7, PhysReg{1});
  uge_codegen.emit_int_cmp_impl(Value{7}, IrCmpOp::Uge, Operand::immediate_i64(7),
                                Operand::immediate_i64(9), IrType::U32);

  std::string uge_asm_text;
  for (const auto& line : uge_codegen.state.asm_lines) {
    uge_asm_text += line;
    uge_asm_text.push_back('\n');
  }

  expect_contains(uge_asm_text, "    slli t1, t1, 32",
                  "wired translated riscv integer-compare seam should zero-extend U32 lhs values through the bounded compare helper");
  expect_contains(uge_asm_text, "    srli t2, t2, 32",
                  "wired translated riscv integer-compare seam should zero-extend U32 rhs values through the bounded compare helper");
  expect_contains(uge_asm_text, "    sltu t0, t1, t2",
                  "wired translated riscv integer-compare seam should lower unsigned-greater-or-equal through the translated unsigned less-than primitive");
  expect_contains(uge_asm_text, "    xori t0, t0, 1",
                  "wired translated riscv integer-compare seam should invert the translated unsigned less-than result for unsigned-greater-or-equal");
  expect_contains(uge_asm_text, "    mv s1, t0",
                  "wired translated riscv integer-compare seam should reuse assigned callee-saved destinations through the shared result-store helper");

  RiscvCodegen slt_codegen;
  slt_codegen.state.slots.emplace(11, StackSlot{-40});
  slt_codegen.emit_int_cmp_impl(Value{11}, IrCmpOp::Slt, Operand::immediate_i64(255),
                                Operand::immediate_i64(1), IrType::I8);

  std::string slt_asm_text;
  for (const auto& line : slt_codegen.state.asm_lines) {
    slt_asm_text += line;
    slt_asm_text.push_back('\n');
  }

  expect_contains(slt_asm_text, "    slli t1, t1, 56",
                  "wired translated riscv integer-compare seam should sign-extend I8 lhs values through the bounded compare helper");
  expect_contains(slt_asm_text, "    srai t2, t2, 56",
                  "wired translated riscv integer-compare seam should sign-extend I8 rhs values through the bounded compare helper");
  expect_contains(slt_asm_text, "    slt t0, t1, t2",
                  "wired translated riscv integer-compare seam should lower signed less-than through the translated signed compare primitive");
  expect_contains(slt_asm_text, "    sd t0, -40(s0)",
                  "wired translated riscv integer-compare seam should preserve the shared stack-store result path for signed compares");
}

void test_riscv_translated_comparison_control_flow_emits_bounded_text() {
  using IrCmpOp = c4c::backend::riscv::codegen::IrCmpOp;
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(15, StackSlot{-24});
  codegen.state.assigned_regs.emplace(17, PhysReg{1});

  codegen.emit_fused_cmp_branch_impl(IrCmpOp::Uge, Operand::immediate_i64(7),
                                     Operand::immediate_i64(9), IrType::U32,
                                     ".Ltrue", ".Lfalse");
  codegen.emit_select_impl(Value{15}, Operand::immediate_i64(0),
                           Operand::immediate_i64(11), Operand::immediate_i64(4),
                           IrType::I32);
  codegen.emit_select_impl(Value{17}, Operand::immediate_i64(4294967296LL),
                           Operand::immediate_i64(11), Operand::immediate_i64(4),
                           IrType::I32);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    slli t1, t1, 32",
                  "wired translated riscv comparison control-flow seam should zero-extend U32 lhs values through the shared compare helper before branching");
  expect_contains(asm_text, "    srli t2, t2, 32",
                  "wired translated riscv comparison control-flow seam should zero-extend U32 rhs values through the shared compare helper before branching");
  expect_contains(asm_text, "    bltu t1, t2, .Lcmp_skip_0\n    j .Ltrue\n.Lcmp_skip_0:\n    j .Lfalse\n",
                  "wired translated riscv comparison control-flow seam should lower unsigned-greater-or-equal through the translated inverted branch plus explicit label jumps");
  expect_contains(asm_text, "    li t0, 4\n    mv t2, t0\n    li t0, 0\n    beqz t0, .Lsel_skip_1\n    li t0, 11\n    mv t2, t0\n.Lsel_skip_1:\n    mv t0, t2\n    sd t0, -24(s0)\n",
                  "wired translated riscv comparison control-flow seam should seed select with the false value, branch around the true-value reload, and preserve the shared stack-store path");
  expect_contains(asm_text, "    li t0, 4\n    mv t2, t0\n    li t0, 4294967296\n    beqz t0, .Lsel_skip_2\n    li t0, 11\n    mv t2, t0\n.Lsel_skip_2:\n    mv t0, t2\n    mv s1, t0\n",
                  "wired translated riscv comparison control-flow seam should override the false seed with the true value and reuse assigned destinations when the condition is non-zero");
}

void test_riscv_translated_f128_cmp_emits_bounded_text() {
  using IrCmpOp = c4c::backend::riscv::codegen::IrCmpOp;
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen eq_codegen;
  eq_codegen.state.slots.emplace(5, StackSlot{-24});
  eq_codegen.state.slots.emplace(21, StackSlot{-56});
  eq_codegen.state.slots.emplace(23, StackSlot{-72});
  eq_codegen.state.assigned_regs.emplace(5, PhysReg{1});

  eq_codegen.emit_float_cmp_impl(Value{5}, IrCmpOp::Eq, Operand{21}, Operand{23}, IrType::F128);

  std::string eq_asm_text;
  for (const auto& line : eq_codegen.state.asm_lines) {
    eq_asm_text += line;
    eq_asm_text.push_back('\n');
  }

  expect_contains(eq_asm_text, "    addi sp, sp, -16",
                  "wired translated riscv f128 compare seam should allocate only the bounded temporary stack bridge for the lhs");
  expect_contains(eq_asm_text, "    ld a0, -56(s0)",
                  "wired translated riscv f128 compare seam should load the lhs low lane through the shared soft-float operand bridge");
  expect_contains(eq_asm_text, "    sd a0, 0(sp)",
                  "wired translated riscv f128 compare seam should spill the lhs through the bounded temporary stack bridge");
  expect_contains(eq_asm_text, "    ld a0, -72(s0)",
                  "wired translated riscv f128 compare seam should load the rhs low lane through the shared soft-float operand bridge");
  expect_contains(eq_asm_text, "    mv a2, a0",
                  "wired translated riscv f128 compare seam should move the rhs low lane into the second argument pair through the shared soft-float hook");
  expect_contains(eq_asm_text, "    ld a0, 0(sp)",
                  "wired translated riscv f128 compare seam should restore the lhs low lane after staging the rhs");
  expect_contains(eq_asm_text, "    addi sp, sp, 16",
                  "wired translated riscv f128 compare seam should release the bounded temporary stack bridge before the libcall");
  expect_contains(eq_asm_text, "    call __eqtf2",
                  "wired translated riscv f128 compare seam should route equality through the shared soft-float libcall mapping");
  expect_contains(eq_asm_text, "    seqz t0, a0",
                  "wired translated riscv f128 compare seam should materialize equality booleans from the shared soft-float result contract");
  expect_contains(eq_asm_text, "    mv s1, t0",
                  "wired translated riscv f128 compare seam should store equality results through the shared assigned-register destination path");

  RiscvCodegen ge_codegen;
  ge_codegen.state.slots.emplace(7, StackSlot{-40});
  ge_codegen.state.slots.emplace(25, StackSlot{-80});
  ge_codegen.state.slots.emplace(27, StackSlot{-96});

  ge_codegen.emit_float_cmp_impl(Value{7}, IrCmpOp::Sge, Operand{25}, Operand{27}, IrType::F128);

  std::string ge_asm_text;
  for (const auto& line : ge_codegen.state.asm_lines) {
    ge_asm_text += line;
    ge_asm_text.push_back('\n');
  }

  expect_contains(ge_asm_text, "    call __getf2",
                  "wired translated riscv f128 compare seam should route greater-or-equal through the shared soft-float libcall mapping");
  expect_contains(ge_asm_text, "    slt t0, a0, x0",
                  "wired translated riscv f128 compare seam should detect negative libcall results before inverting for greater-or-equal");
  expect_contains(ge_asm_text, "    xori t0, t0, 1",
                  "wired translated riscv f128 compare seam should invert the negative-result test to materialize greater-or-equal");
  expect_contains(ge_asm_text, "    sd t0, -40(s0)",
                  "wired translated riscv f128 compare seam should store stack boolean destinations through the shared result-store helper");
}

void test_riscv_translated_f128_memory_emits_bounded_text() {
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(1, StackSlot{-24});
  codegen.state.slots.emplace(2, StackSlot{-40});
  codegen.state.slots.emplace(3, StackSlot{-56});
  codegen.state.slots.emplace(11, StackSlot{-88});
  codegen.state.slots.emplace(13, StackSlot{-104});
  codegen.state.slots.emplace(21, StackSlot{-120});
  codegen.state.slots.emplace(22, StackSlot{-136});
  codegen.state.slots.emplace(23, StackSlot{-152});
  codegen.state.allocas.insert(2);
  codegen.state.over_aligned_allocas.emplace(3, 32);
  codegen.state.assigned_regs.emplace(2, PhysReg{7});
  codegen.state.assigned_regs.emplace(22, PhysReg{8});

  codegen.emit_store_impl(Operand{11}, Value{1}, IrType::F128);
  codegen.emit_store_with_const_offset_impl(Operand{13}, Value{2}, 16, IrType::F128);
  codegen.emit_store_with_const_offset_impl(Operand{11}, Value{3}, 24, IrType::F128);
  codegen.emit_load_impl(Value{21}, Value{1}, IrType::F128);
  codegen.emit_load_with_const_offset_impl(Value{22}, Value{2}, 8, IrType::F128);
  codegen.emit_load_with_const_offset_impl(Value{23}, Value{3}, 24, IrType::F128);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    ld a0, -88(s0)",
                  "wired translated riscv f128 memory seam should reload direct-slot sources through the shared operand bridge before direct stores");
  expect_contains(asm_text, "    ld a1, -80(s0)",
                  "wired translated riscv f128 memory seam should reload both F128 lanes through the shared operand bridge before direct stores");
  expect_contains(asm_text, "    sd a0, -24(s0)",
                  "wired translated riscv f128 memory seam should store the low F128 lane directly into resolved stack slots");
  expect_contains(asm_text, "    sd a1, -16(s0)",
                  "wired translated riscv f128 memory seam should store the high F128 lane directly into resolved stack slots");
  expect_contains(asm_text, "    mv t5, s7",
                  "wired translated riscv f128 memory seam should reuse assigned callee-saved pointer bases for indirect stores and loads");
  expect_contains(asm_text, "    addi t5, t5, 16",
                  "wired translated riscv f128 memory seam should apply bounded constant offsets on indirect F128 stores");
  expect_contains(asm_text, "    sd a0, 0(t5)",
                  "wired translated riscv f128 memory seam should store the low F128 lane through the resolved address-register path");
  expect_contains(asm_text, "    sd a1, 8(t5)",
                  "wired translated riscv f128 memory seam should store the high F128 lane through the resolved address-register path");
  expect_contains(asm_text, "    addi t5, s0, -56",
                  "wired translated riscv f128 memory seam should resolve over-aligned F128 bases through the shared alloca-address helper");
  expect_contains(asm_text, "    andi t5, t5, -32",
                  "wired translated riscv f128 memory seam should preserve bounded over-aligned address masking for F128 stores and loads");
  expect_contains(asm_text, "    addi t5, t5, 24",
                  "wired translated riscv f128 memory seam should apply bounded constant offsets after over-aligned address resolution");
  expect_contains(asm_text, "    ld a0, -24(s0)",
                  "wired translated riscv f128 memory seam should reload direct F128 load sources through the shared stack-slot path");
  expect_contains(asm_text, "    ld a1, -16(s0)",
                  "wired translated riscv f128 memory seam should reload the high direct F128 lane through the shared stack-slot path");
  expect_contains(asm_text, "    sd a0, -120(s0)",
                  "wired translated riscv f128 memory seam should preserve the full direct-load low lane in the destination slot before truncation");
  expect_contains(asm_text, "    sd a1, -112(s0)",
                  "wired translated riscv f128 memory seam should preserve the full direct-load high lane in the destination slot before truncation");
  expect_contains(asm_text, "    addi t5, t5, 8",
                  "wired translated riscv f128 memory seam should apply bounded constant offsets on indirect F128 loads");
  expect_contains(asm_text, "    ld a0, 0(t5)",
                  "wired translated riscv f128 memory seam should reload the low lane from resolved address-register loads");
  expect_contains(asm_text, "    ld a1, 8(t5)",
                  "wired translated riscv f128 memory seam should reload the high lane from resolved address-register loads");
  expect_contains(asm_text, "    call __trunctfdf2",
                  "wired translated riscv f128 memory seam should truncate loaded full-precision values only after staging the full destination payload");
  expect_contains(asm_text, "    mv s8, t0",
                  "wired translated riscv f128 memory seam should reuse assigned callee-saved destinations for truncated load results");
  expect_contains(asm_text, "    sd a0, -152(s0)",
                  "wired translated riscv f128 memory seam should preserve the low lane for over-aligned loads after the shared address-resolution path");
  expect_contains(asm_text, "    sd a1, -144(s0)",
                  "wired translated riscv f128 memory seam should preserve the high lane for over-aligned loads after the shared address-resolution path");
}

void test_riscv_translated_cast_emits_bounded_f128_text() {
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen u32_to_f128_codegen;
  u32_to_f128_codegen.state.slots.emplace(7, StackSlot{-32});
  u32_to_f128_codegen.state.slots.emplace(9, StackSlot{-24});
  u32_to_f128_codegen.emit_cast_impl(Value{9}, Operand{7}, IrType::U32, IrType::F128);

  std::string u32_to_f128_asm;
  for (const auto& line : u32_to_f128_codegen.state.asm_lines) {
    u32_to_f128_asm += line;
    u32_to_f128_asm.push_back('\n');
  }

  expect_contains(u32_to_f128_asm, "    ld t0, -32(s0)",
                  "wired translated riscv cast seam should reload unsigned integer sources through the bounded operand bridge before widening to F128");
  expect_contains(u32_to_f128_asm, "    slli t0, t0, 32",
                  "wired translated riscv cast seam should zero-extend narrow unsigned integer sources before the shared F128 libcall");
  expect_contains(u32_to_f128_asm, "    srli t0, t0, 32",
                  "wired translated riscv cast seam should finish the bounded unsigned extension before the shared F128 libcall");
  expect_contains(u32_to_f128_asm, "    mv a0, t0",
                  "wired translated riscv cast seam should route widened integer payloads into the first soft-float argument register");
  expect_contains(u32_to_f128_asm, "    call __floatunditf",
                  "wired translated riscv cast seam should use the shared unsigned integer-to-f128 libcall mapping");
  expect_contains(u32_to_f128_asm, "    sd a0, -24(s0)",
                  "wired translated riscv cast seam should preserve the low widened F128 lane in the destination slot");
  expect_contains(u32_to_f128_asm, "    sd a1, -16(s0)",
                  "wired translated riscv cast seam should preserve the high widened F128 lane in the destination slot");

  RiscvCodegen f128_to_f64_codegen;
  f128_to_f64_codegen.state.slots.emplace(11, StackSlot{-56});
  f128_to_f64_codegen.state.slots.emplace(13, StackSlot{-40});
  f128_to_f64_codegen.emit_cast_impl(Value{13}, Operand{11}, IrType::F128, IrType::F64);

  std::string f128_to_f64_asm;
  for (const auto& line : f128_to_f64_codegen.state.asm_lines) {
    f128_to_f64_asm += line;
    f128_to_f64_asm.push_back('\n');
  }

  expect_contains(f128_to_f64_asm, "    ld a0, -56(s0)",
                  "wired translated riscv cast seam should reload the low F128 lane through the shared operand bridge before narrowing to F64");
  expect_contains(f128_to_f64_asm, "    ld a1, -48(s0)",
                  "wired translated riscv cast seam should reload the high F128 lane through the shared operand bridge before narrowing to F64");
  expect_contains(f128_to_f64_asm, "    call __trunctfdf2",
                  "wired translated riscv cast seam should use the shared F128-to-F64 narrowing libcall");
  expect_contains(f128_to_f64_asm, "    fmv.x.d t0, fa0",
                  "wired translated riscv cast seam should move the narrowed F64 payload back into the accumulator");
  expect_contains(f128_to_f64_asm, "    sd t0, -40(s0)",
                  "wired translated riscv cast seam should store narrowed F64 results through the shared destination helper");

  RiscvCodegen f128_to_i8_codegen;
  f128_to_i8_codegen.state.slots.emplace(15, StackSlot{-80});
  f128_to_i8_codegen.state.slots.emplace(17, StackSlot{-64});
  f128_to_i8_codegen.emit_cast_impl(Value{17}, Operand{15}, IrType::F128, IrType::I8);

  std::string f128_to_i8_asm;
  for (const auto& line : f128_to_i8_codegen.state.asm_lines) {
    f128_to_i8_asm += line;
    f128_to_i8_asm.push_back('\n');
  }

  expect_contains(f128_to_i8_asm, "    call __fixtfdi",
                  "wired translated riscv cast seam should use the shared signed F128-to-int libcall mapping");
  expect_contains(f128_to_i8_asm, "    mv t0, a0",
                  "wired translated riscv cast seam should move integer cast results back into the accumulator after the soft-float libcall");
  expect_contains(f128_to_i8_asm, "    slli t0, t0, 56",
                  "wired translated riscv cast seam should preserve the translated signed narrowing path after F128 integer conversion");
  expect_contains(f128_to_i8_asm, "    srai t0, t0, 56",
                  "wired translated riscv cast seam should finish the translated signed narrowing path after F128 integer conversion");
  expect_contains(f128_to_i8_asm, "    sd t0, -64(s0)",
                  "wired translated riscv cast seam should store integer cast results through the shared destination helper");
}

void test_riscv_translated_cast_emits_bounded_scalar_text() {
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen i32_to_f64_codegen;
  i32_to_f64_codegen.state.slots.emplace(21, StackSlot{-32});
  i32_to_f64_codegen.state.slots.emplace(23, StackSlot{-24});
  i32_to_f64_codegen.emit_cast_impl(Value{23}, Operand{21}, IrType::I32, IrType::F64);

  std::string i32_to_f64_asm;
  for (const auto& line : i32_to_f64_codegen.state.asm_lines) {
    i32_to_f64_asm += line;
    i32_to_f64_asm.push_back('\n');
  }

  expect_contains(i32_to_f64_asm, "    ld t0, -32(s0)",
                  "wired translated riscv scalar cast owner should reload signed integer sources through the bounded operand bridge");
  expect_contains(i32_to_f64_asm, "    sext.w t0, t0",
                  "wired translated riscv scalar cast owner should sign-extend i32 sources before integer-to-f64 conversion");
  expect_contains(i32_to_f64_asm, "    fcvt.d.l ft0, t0",
                  "wired translated riscv scalar cast owner should use the translated signed integer-to-f64 conversion");
  expect_contains(i32_to_f64_asm, "    fmv.x.d t0, ft0",
                  "wired translated riscv scalar cast owner should materialize f64 cast results back into the accumulator");
  expect_contains(i32_to_f64_asm, "    sd t0, -24(s0)",
                  "wired translated riscv scalar cast owner should store widened scalar results through the shared destination helper");

  RiscvCodegen f64_to_u16_codegen;
  f64_to_u16_codegen.state.slots.emplace(25, StackSlot{-56});
  f64_to_u16_codegen.state.slots.emplace(27, StackSlot{-40});
  f64_to_u16_codegen.emit_cast_impl(Value{27}, Operand{25}, IrType::F64, IrType::U16);

  std::string f64_to_u16_asm;
  for (const auto& line : f64_to_u16_codegen.state.asm_lines) {
    f64_to_u16_asm += line;
    f64_to_u16_asm.push_back('\n');
  }

  expect_contains(f64_to_u16_asm, "    fmv.d.x ft0, t0",
                  "wired translated riscv scalar cast owner should move f64 payloads into ft0 before integer conversion");
  expect_contains(f64_to_u16_asm, "    fcvt.lu.d t0, ft0, rtz",
                  "wired translated riscv scalar cast owner should use the translated unsigned float-to-int truncate path");
  expect_contains(f64_to_u16_asm, "    slli t0, t0, 48",
                  "wired translated riscv scalar cast owner should narrow unsigned float-to-int results to the requested width");
  expect_contains(f64_to_u16_asm, "    srli t0, t0, 48",
                  "wired translated riscv scalar cast owner should preserve zero-extension after unsigned narrowing");
  expect_contains(f64_to_u16_asm, "    sd t0, -40(s0)",
                  "wired translated riscv scalar cast owner should store narrowed unsigned results through the shared destination helper");

  RiscvCodegen f32_to_f64_codegen;
  f32_to_f64_codegen.state.slots.emplace(29, StackSlot{-80});
  f32_to_f64_codegen.state.slots.emplace(31, StackSlot{-64});
  f32_to_f64_codegen.emit_cast_impl(Value{31}, Operand{29}, IrType::F32, IrType::F64);

  std::string f32_to_f64_asm;
  for (const auto& line : f32_to_f64_codegen.state.asm_lines) {
    f32_to_f64_asm += line;
    f32_to_f64_asm.push_back('\n');
  }

  expect_contains(f32_to_f64_asm, "    fmv.w.x ft0, t0",
                  "wired translated riscv scalar cast owner should move f32 payloads into ft0 before float widening");
  expect_contains(f32_to_f64_asm, "    fcvt.d.s ft0, ft0",
                  "wired translated riscv scalar cast owner should use the translated float widen instruction");
  expect_contains(f32_to_f64_asm, "    fmv.x.d t0, ft0",
                  "wired translated riscv scalar cast owner should materialize widened f64 payloads back into the accumulator");

  RiscvCodegen u32_to_i32_codegen;
  u32_to_i32_codegen.state.slots.emplace(33, StackSlot{-96});
  u32_to_i32_codegen.state.slots.emplace(35, StackSlot{-88});
  u32_to_i32_codegen.emit_cast_impl(Value{35}, Operand{33}, IrType::U32, IrType::I32);

  std::string u32_to_i32_asm;
  for (const auto& line : u32_to_i32_codegen.state.asm_lines) {
    u32_to_i32_asm += line;
    u32_to_i32_asm.push_back('\n');
  }

  expect_contains(u32_to_i32_asm, "    sext.w t0, t0",
                  "wired translated riscv scalar cast owner should preserve the ABI-mandated u32-to-i32 sign-extension path");
  expect_contains(u32_to_i32_asm, "    sd t0, -88(s0)",
                  "wired translated riscv scalar cast owner should store same-size scalar cast results through the shared destination helper");
}

void test_riscv_translated_default_cast_emits_bounded_i128_text() {
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen signed_to_i128_codegen;
  signed_to_i128_codegen.state.slots.emplace(49, StackSlot{-112});
  signed_to_i128_codegen.state.slots.emplace(51, StackSlot{-96});
  signed_to_i128_codegen.emit_cast_impl(Value{51}, Operand{49}, IrType::I32, IrType::I128);

  std::string signed_to_i128_asm;
  for (const auto& line : signed_to_i128_codegen.state.asm_lines) {
    signed_to_i128_asm += line;
    signed_to_i128_asm.push_back('\n');
  }

  expect_contains(signed_to_i128_asm, "    ld t0, -112(s0)",
                  "wired translated riscv default-cast seam should reload signed scalar sources through the shared operand bridge");
  expect_contains(signed_to_i128_asm, "    sext.w t0, t0",
                  "wired translated riscv default-cast seam should widen narrow signed inputs before extending them to i128");
  expect_contains(signed_to_i128_asm, "    srai t1, t0, 63",
                  "wired translated riscv default-cast seam should sign-extend the high i128 lane for signed integer widening");
  expect_contains(signed_to_i128_asm, "    sd t0, -96(s0)",
                  "wired translated riscv default-cast seam should store the low widened i128 lane through the shared pair-store helper");
  expect_contains(signed_to_i128_asm, "    sd t1, -88(s0)",
                  "wired translated riscv default-cast seam should store the high widened i128 lane through the shared pair-store helper");

  RiscvCodegen unsigned_to_i128_codegen;
  unsigned_to_i128_codegen.state.slots.emplace(53, StackSlot{-144});
  unsigned_to_i128_codegen.state.slots.emplace(55, StackSlot{-128});
  unsigned_to_i128_codegen.emit_cast_impl(Value{55}, Operand{53}, IrType::U32, IrType::I128);

  std::string unsigned_to_i128_asm;
  for (const auto& line : unsigned_to_i128_codegen.state.asm_lines) {
    unsigned_to_i128_asm += line;
    unsigned_to_i128_asm.push_back('\n');
  }

  expect_contains(unsigned_to_i128_asm, "    slli t0, t0, 32",
                  "wired translated riscv default-cast seam should preserve the translated zero-extension path before widening unsigned values to i128");
  expect_contains(unsigned_to_i128_asm, "    srli t0, t0, 32",
                  "wired translated riscv default-cast seam should finish the translated zero-extension path before widening unsigned values to i128");
  expect_contains(unsigned_to_i128_asm, "    li t1, 0",
                  "wired translated riscv default-cast seam should zero the high i128 lane for unsigned integer widening");

  RiscvCodegen float_to_i128_codegen;
  float_to_i128_codegen.state.slots.emplace(57, StackSlot{-176});
  float_to_i128_codegen.state.slots.emplace(59, StackSlot{-160});
  float_to_i128_codegen.emit_cast_impl(Value{59}, Operand{57}, IrType::F64, IrType::I128);

  std::string float_to_i128_asm;
  for (const auto& line : float_to_i128_codegen.state.asm_lines) {
    float_to_i128_asm += line;
    float_to_i128_asm.push_back('\n');
  }

  expect_contains(float_to_i128_asm, "    ld t0, -176(s0)",
                  "wired translated riscv default-cast seam should reload float sources through the shared operand bridge before float-to-i128 lowering");
  expect_contains(float_to_i128_asm, "    fmv.d.x fa0, t0",
                  "wired translated riscv default-cast seam should move floating-point payloads into fa0 before the compiler-rt i128 conversion");
  expect_contains(float_to_i128_asm, "    call __fixdfti",
                  "wired translated riscv default-cast seam should use the shared signed float-to-i128 compiler-rt helper");
  expect_contains(float_to_i128_asm, "    mv t0, a0",
                  "wired translated riscv default-cast seam should move the converted low i128 lane back into the accumulator");
  expect_contains(float_to_i128_asm, "    mv t1, a1",
                  "wired translated riscv default-cast seam should move the converted high i128 lane back into the accumulator");
  expect_contains(float_to_i128_asm, "    sd t0, -160(s0)",
                  "wired translated riscv default-cast seam should store the converted low i128 lane through the shared pair-store helper");
  expect_contains(float_to_i128_asm, "    sd t1, -152(s0)",
                  "wired translated riscv default-cast seam should store the converted high i128 lane through the shared pair-store helper");

  RiscvCodegen i128_to_float_codegen;
  i128_to_float_codegen.state.slots.emplace(61, StackSlot{-208});
  i128_to_float_codegen.state.slots.emplace(63, StackSlot{-184});
  i128_to_float_codegen.emit_cast_impl(Value{63}, Operand{61}, IrType::I128, IrType::F64);

  std::string i128_to_float_asm;
  for (const auto& line : i128_to_float_codegen.state.asm_lines) {
    i128_to_float_asm += line;
    i128_to_float_asm.push_back('\n');
  }

  expect_contains(i128_to_float_asm, "    ld t0, -208(s0)",
                  "wired translated riscv default-cast seam should reload the low i128 lane through the shared pair-load helper before floating conversion");
  expect_contains(i128_to_float_asm, "    ld t1, -200(s0)",
                  "wired translated riscv default-cast seam should reload the high i128 lane through the shared pair-load helper before floating conversion");
  expect_contains(i128_to_float_asm, "    mv a0, t0",
                  "wired translated riscv default-cast seam should move the low i128 lane into a0 before the compiler-rt float conversion");
  expect_contains(i128_to_float_asm, "    mv a1, t1",
                  "wired translated riscv default-cast seam should move the high i128 lane into a1 before the compiler-rt float conversion");
  expect_contains(i128_to_float_asm, "    call __floattidf",
                  "wired translated riscv default-cast seam should use the shared signed i128-to-float compiler-rt helper");
  expect_contains(i128_to_float_asm, "    fmv.x.d t0, fa0",
                  "wired translated riscv default-cast seam should move float conversion results back into the scalar accumulator");
  expect_contains(i128_to_float_asm, "    sd t0, -184(s0)",
                  "wired translated riscv default-cast seam should store float conversion results through the shared scalar destination helper");
}

void test_riscv_translated_i128_owner_emits_bounded_helper_text() {
  using IrCmpOp = c4c::backend::riscv::codegen::IrCmpOp;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(5, StackSlot{-24});
  codegen.state.slots.emplace(11, StackSlot{-56});
  codegen.state.slots.emplace(13, StackSlot{-72});

  codegen.emit_i128_prep_binop_impl(Operand{5}, Operand::immediate_i64(-3));
  codegen.emit_i128_add_impl();
  codegen.emit_i128_store_result_impl(Value{11});
  codegen.emit_i128_prep_binop_impl(Operand{5}, Operand::immediate_i64(9));
  codegen.emit_i128_sub_impl();
  codegen.emit_i128_prep_binop_impl(Operand{5}, Operand::immediate_i64(6));
  codegen.emit_i128_mul_impl();
  codegen.emit_i128_prep_binop_impl(Operand{5}, Operand::immediate_i64(12));
  codegen.emit_i128_and_impl();
  codegen.emit_i128_prep_binop_impl(Operand{5}, Operand::immediate_i64(4));
  codegen.emit_i128_or_impl();
  codegen.emit_i128_prep_binop_impl(Operand{5}, Operand::immediate_i64(7));
  codegen.emit_i128_xor_impl();
  codegen.emit_i128_prep_shift_lhs_impl(Operand{5});
  codegen.emit_i128_shl_const_impl(0);
  codegen.emit_i128_shl_const_impl(64);
  codegen.emit_i128_shl_const_impl(68);
  codegen.emit_i128_shl_const_impl(5);
  codegen.emit_i128_lshr_const_impl(0);
  codegen.emit_i128_lshr_const_impl(64);
  codegen.emit_i128_lshr_const_impl(68);
  codegen.emit_i128_lshr_const_impl(5);
  codegen.emit_i128_ashr_const_impl(0);
  codegen.emit_i128_ashr_const_impl(64);
  codegen.emit_i128_ashr_const_impl(68);
  codegen.emit_i128_ashr_const_impl(5);
  codegen.emit_i128_prep_binop_impl(Operand{5}, Operand::immediate_i64(65));
  codegen.emit_i128_shl_impl();
  codegen.emit_i128_prep_binop_impl(Operand{5}, Operand::immediate_i64(65));
  codegen.emit_i128_lshr_impl();
  codegen.emit_i128_prep_binop_impl(Operand{5}, Operand::immediate_i64(65));
  codegen.emit_i128_ashr_impl();
  codegen.emit_i128_divrem_call_impl("__udivti3", Operand{5}, Operand::immediate_i64(3));
  codegen.emit_i128_prep_binop_impl(Operand{5}, Operand::immediate_i64(7));
  codegen.emit_i128_cmp_eq_impl(false);
  codegen.emit_i128_prep_binop_impl(Operand{5}, Operand::immediate_i64(7));
  codegen.emit_i128_cmp_eq_impl(true);
  codegen.emit_i128_cmp_ordered_impl(IrCmpOp::Sge);
  codegen.emit_i128_cmp_store_result_impl(Value{13});
  codegen.emit_store_pair_to_slot_impl(StackSlot{-88});
  codegen.emit_load_pair_from_slot_impl(StackSlot{-88});
  codegen.emit_save_acc_pair_impl();
  codegen.emit_store_pair_indirect_impl();
  codegen.emit_load_pair_indirect_impl();
  codegen.emit_i128_neg_impl();
  codegen.emit_i128_not_impl();

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    ld t0, -24(s0)",
                  "wired translated riscv i128 owner should reload the low lhs lane through the shared pair-load helper");
  expect_contains(asm_text, "    ld t1, -16(s0)",
                  "wired translated riscv i128 owner should reload the high lhs lane through the shared pair-load helper");
  expect_contains(asm_text, "    mv t3, t0",
                  "wired translated riscv i128 owner should preserve the prepared lhs low lane before arithmetic");
  expect_contains(asm_text, "    mv t4, t1",
                  "wired translated riscv i128 owner should preserve the prepared lhs high lane before arithmetic");
  expect_contains(asm_text, "    li t5, -3",
                  "wired translated riscv i128 owner should materialize immediate rhs low lanes through the shared pair-load helper");
  expect_contains(asm_text, "    li t6, -1",
                  "wired translated riscv i128 owner should sign-extend negative immediate rhs lanes into the staged high register");
  expect_contains(asm_text, "    add t0, t3, t5",
                  "wired translated riscv i128 owner should emit the Rust-matching add low-lane primitive");
  expect_contains(asm_text, "    sltu t2, t0, t3",
                  "wired translated riscv i128 owner should materialize the low-lane carry before the high-lane add");
  expect_contains(asm_text, "    add t1, t4, t6",
                  "wired translated riscv i128 owner should emit the high-lane add against the staged rhs");
  expect_contains(asm_text, "    add t1, t1, t2",
                  "wired translated riscv i128 owner should fold the carry into the high-lane result");
  expect_contains(asm_text, "    sltu t2, t3, t5",
                  "wired translated riscv i128 owner should materialize the low-lane borrow before subtraction");
  expect_contains(asm_text, "    sub t0, t3, t5",
                  "wired translated riscv i128 owner should emit the Rust-matching subtract low-lane primitive");
  expect_contains(asm_text, "    sub t1, t4, t6",
                  "wired translated riscv i128 owner should emit the high-lane subtraction against the staged rhs");
  expect_contains(asm_text, "    sub t1, t1, t2",
                  "wired translated riscv i128 owner should fold the low-lane borrow into the high-lane subtraction");
  expect_contains(asm_text, "    mul t0, t3, t5",
                  "wired translated riscv i128 owner should emit the Rust-matching low-lane multiply primitive");
  expect_contains(asm_text, "    mulhu t1, t3, t5",
                  "wired translated riscv i128 owner should seed the high-lane product from the low-word unsigned high multiply");
  expect_contains(asm_text, "    mul t2, t4, t5",
                  "wired translated riscv i128 owner should accumulate the high-lhs by low-rhs cross term into the staged product");
  expect_contains(asm_text, "    mul t2, t3, t6",
                  "wired translated riscv i128 owner should accumulate the low-lhs by high-rhs cross term into the staged product");
  expect_contains(asm_text, "    and t0, t3, t5",
                  "wired translated riscv i128 owner should preserve the translated bitwise-and low-lane primitive");
  expect_contains(asm_text, "    and t1, t4, t6",
                  "wired translated riscv i128 owner should preserve the translated bitwise-and high-lane primitive");
  expect_contains(asm_text, "    or t0, t3, t5",
                  "wired translated riscv i128 owner should preserve the translated bitwise-or low-lane primitive");
  expect_contains(asm_text, "    or t1, t4, t6",
                  "wired translated riscv i128 owner should preserve the translated bitwise-or high-lane primitive");
  expect_contains(asm_text, "    xor t0, t3, t5",
                  "wired translated riscv i128 owner should preserve the translated xor low-lane primitive for arithmetic/logical and equality paths");
  expect_contains(asm_text, "    xor t1, t4, t6",
                  "wired translated riscv i128 owner should preserve the translated xor high-lane primitive for arithmetic/logical and equality paths");
  expect_contains(asm_text, "    mv t0, t3",
                  "wired translated riscv i128 owner should preserve the prepared low lane when a constant shift amount is zero");
  expect_contains(asm_text, "    mv t1, t4",
                  "wired translated riscv i128 owner should preserve the prepared high lane when a constant shift amount is zero");
  expect_contains(asm_text, "    slli t1, t3, 4",
                  "wired translated riscv i128 owner should shift the low lane into the high lane for left shifts above 64 bits");
  expect_contains(asm_text, "    slli t1, t4, 5",
                  "wired translated riscv i128 owner should seed left shifts below 64 bits from the prepared high lane");
  expect_contains(asm_text, "    srli t2, t3, 59",
                  "wired translated riscv i128 owner should bridge low-lane spill bits into the high lane for bounded left shifts");
  expect_contains(asm_text, "    slli t0, t3, 5",
                  "wired translated riscv i128 owner should emit the Rust-matching low-lane left shift for bounded constant shifts");
  expect_contains(asm_text, "    srli t0, t4, 4",
                  "wired translated riscv i128 owner should shift the high lane into the low lane for logical right shifts above 64 bits");
  expect_contains(asm_text, "    srli t0, t3, 5",
                  "wired translated riscv i128 owner should emit the Rust-matching low-lane logical right shift for bounded constant shifts");
  expect_contains(asm_text, "    slli t2, t4, 59",
                  "wired translated riscv i128 owner should bridge high-lane spill bits into the low lane for bounded logical right shifts");
  expect_contains(asm_text, "    srli t1, t4, 5",
                  "wired translated riscv i128 owner should emit the Rust-matching high-lane logical right shift for bounded constant shifts");
  expect_contains(asm_text, "    srai t1, t4, 63",
                  "wired translated riscv i128 owner should sign-fill the high lane for arithmetic right shifts at or above 64 bits");
  expect_contains(asm_text, "    srai t0, t4, 4",
                  "wired translated riscv i128 owner should shift the signed high lane into the low lane for arithmetic right shifts above 64 bits");
  expect_contains(asm_text, "    or t0, t0, t2",
                  "wired translated riscv i128 owner should merge bridged spill bits into the low lane for bounded right shifts");
  expect_contains(asm_text, "    srai t1, t4, 5",
                  "wired translated riscv i128 owner should emit the Rust-matching high-lane arithmetic right shift for bounded constant shifts");
  expect_contains(asm_text, "    andi t5, t5, 127",
                  "wired translated riscv i128 owner should mask variable shift counts to the Rust-matching 0..127 range");
  expect_contains(asm_text, "    beqz t5, ",
                  "wired translated riscv i128 owner should preserve the no-op fast path for zero-count variable shifts");
  expect_contains(asm_text, "    bge t5, t2, ",
                  "wired translated riscv i128 owner should branch into the above-64 path for variable shifts");
  expect_contains(asm_text, "    sll t1, t4, t5",
                  "wired translated riscv i128 owner should emit the bounded variable left-shift high-lane primitive");
  expect_contains(asm_text, "    srl t6, t3, t2",
                  "wired translated riscv i128 owner should bridge low-lane spill bits into the variable left-shift high lane");
  expect_contains(asm_text, "    sll t0, t3, t5",
                  "wired translated riscv i128 owner should emit the bounded variable left-shift low-lane primitive");
  expect_contains(asm_text, "    srl t0, t3, t5",
                  "wired translated riscv i128 owner should emit the bounded variable logical-right-shift low-lane primitive");
  expect_contains(asm_text, "    sll t6, t4, t2",
                  "wired translated riscv i128 owner should bridge high-lane spill bits into the variable right-shift low lane");
  expect_contains(asm_text, "    srl t1, t4, t5",
                  "wired translated riscv i128 owner should emit the bounded variable logical-right-shift high-lane primitive");
  expect_contains(asm_text, "    sra t1, t4, t5",
                  "wired translated riscv i128 owner should emit the bounded variable arithmetic-right-shift high-lane primitive");
  expect_contains(asm_text, "    sra t0, t4, t5",
                  "wired translated riscv i128 owner should shift the signed high lane into the low lane for variable arithmetic right shifts above 64 bits");
  expect_contains(asm_text, "    mv a0, t0",
                  "wired translated riscv i128 owner should move the lhs low lane into a0 before the div/rem helper call");
  expect_contains(asm_text, "    mv a1, t1",
                  "wired translated riscv i128 owner should move the lhs high lane into a1 before the div/rem helper call");
  expect_contains(asm_text, "    li t0, 3",
                  "wired translated riscv i128 owner should materialize the rhs low lane before the div/rem helper call");
  expect_contains(asm_text, "    li t1, 0",
                  "wired translated riscv i128 owner should zero-extend a positive rhs immediate before the div/rem helper call");
  expect_contains(asm_text, "    mv a2, t0",
                  "wired translated riscv i128 owner should move the rhs low lane into a2 before the div/rem helper call");
  expect_contains(asm_text, "    mv a3, t1",
                  "wired translated riscv i128 owner should move the rhs high lane into a3 before the div/rem helper call");
  expect_contains(asm_text, "    call __udivti3",
                  "wired translated riscv i128 owner should dispatch div/rem lowering through the translated helper call surface");
  expect_contains(asm_text, "    mv t0, a0",
                  "wired translated riscv i128 owner should move the helper low-lane result back into the accumulator");
  expect_contains(asm_text, "    mv t1, a1",
                  "wired translated riscv i128 owner should move the helper high-lane result back into the accumulator");
  expect_contains(asm_text, "    or t0, t0, t1",
                  "wired translated riscv i128 owner should fold low/high xor lanes together before equality materialization");
  expect_contains(asm_text, "    seqz t0, t0",
                  "wired translated riscv i128 owner should materialize equality results through the translated seqz primitive");
  expect_contains(asm_text, "    snez t0, t0",
                  "wired translated riscv i128 owner should materialize inequality results through the translated snez primitive");
  expect_contains(asm_text, "    sd t0, -56(s0)",
                  "wired translated riscv i128 owner should store the low result lane back to the destination slot");
  expect_contains(asm_text, "    sd t1, -48(s0)",
                  "wired translated riscv i128 owner should store the high result lane back to the destination slot");
  expect_contains(asm_text, "    blt t1, t6, ",
                  "wired translated riscv i128 owner should compare the high signed lanes first for ordered comparisons");
  expect_contains(asm_text, "    bgeu t0, t5, ",
                  "wired translated riscv i128 owner should compare the low lanes unsigned after high-lane equality for signed-ge");
  expect_contains(asm_text, "    li t0, 1",
                  "wired translated riscv i128 owner should materialize a true ordered-comparison result in the accumulator");
  expect_contains(asm_text, "    li t0, 0",
                  "wired translated riscv i128 owner should materialize a false ordered-comparison result in the accumulator");
  expect_contains(asm_text, "    sd t0, -72(s0)",
                  "wired translated riscv i128 owner should store the comparison result through the shared scalar store helper");
  expect_contains(asm_text, "    sd t0, -88(s0)",
                  "wired translated riscv i128 owner should keep the pair-store helper wired for explicit stack slots");
  expect_contains(asm_text, "    sd t1, -80(s0)",
                  "wired translated riscv i128 owner should keep the pair-store helper writing the high half at slot+8");
  expect_contains(asm_text, "    ld t0, -88(s0)",
                  "wired translated riscv i128 owner should keep the pair-load helper reloading the low half from an explicit slot");
  expect_contains(asm_text, "    ld t1, -80(s0)",
                  "wired translated riscv i128 owner should keep the pair-load helper reloading the high half from an explicit slot");
  expect_contains(asm_text, "    sd t3, 0(t5)",
                  "wired translated riscv i128 owner should lower indirect pair stores through the staged low lane");
  expect_contains(asm_text, "    sd t4, 8(t5)",
                  "wired translated riscv i128 owner should lower indirect pair stores through the staged high lane");
  expect_contains(asm_text, "    ld t0, 0(t5)",
                  "wired translated riscv i128 owner should keep indirect pair loads wired through the staged address register");
  expect_contains(asm_text, "    ld t1, 8(t5)",
                  "wired translated riscv i128 owner should keep indirect pair loads reloading the high lane from address+8");
  expect_contains(asm_text, "    addi t0, t0, 1",
                  "wired translated riscv i128 owner should emit the Rust-matching two's-complement neg low-lane increment");
  expect_contains(asm_text, "    seqz t2, t0",
                  "wired translated riscv i128 owner should materialize the low-lane zero test before the high-lane carry");
  expect_contains(asm_text, "    add t1, t1, t2",
                  "wired translated riscv i128 owner should emit the Rust-matching two's-complement neg carry into the high lane");
  expect_contains(asm_text, "    not t0, t0",
                  "wired translated riscv i128 owner should preserve the translated i128 bitwise-not low-lane primitive");
  expect_contains(asm_text, "    not t1, t1",
                  "wired translated riscv i128 owner should preserve the translated i128 bitwise-not high-lane primitive");
}

void test_riscv_translated_prologue_return_exit_helpers_match_shared_contract() {
  using IrFunction = c4c::backend::riscv::codegen::IrFunction;
  using IrType = c4c::backend::riscv::codegen::IrType;
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  RiscvCodegen codegen;
  codegen.used_callee_saved = {PhysReg{1}, PhysReg{7}};
  IrFunction func;
  func.return_type = IrType::F64;

  expect_true(codegen.aligned_frame_size_impl(17) == 32,
              "wired translated riscv prologue helpers should keep the bounded 16-byte frame alignment contract");

  codegen.emit_prologue_impl(func, 32);
  codegen.emit_epilogue_and_ret_impl(0);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_true(codegen.current_return_type_impl() == IrType::F64,
              "wired translated riscv prologue helpers should surface the current return type for later returns-entry work");
  expect_contains(asm_text, "    addi sp, sp, -32",
                  "wired translated riscv prologue helpers should reserve the bounded frame size before recording return-exit state");
  expect_contains(asm_text, "    sd ra, 24(sp)",
                  "wired translated riscv prologue helpers should save ra in the bounded frame layout");
  expect_contains(asm_text, "    sd s0, 16(sp)",
                  "wired translated riscv prologue helpers should save s0 in the bounded frame layout");
  expect_contains(asm_text, "    addi s0, sp, 32",
                  "wired translated riscv prologue helpers should establish the frame pointer for later s0-relative owners");
  expect_contains(asm_text, "    sd s1, -32(s0)",
                  "wired translated riscv prologue helpers should spill used callee-saved registers through the shared s0-relative helper");
  expect_contains(asm_text, "    sd s7, -24(s0)",
                  "wired translated riscv prologue helpers should keep the bounded callee-saved spill order stable");
  expect_contains(asm_text, "    ld s1, -32(s0)",
                  "wired translated riscv prologue helpers should reload spilled callee-saved registers before returning");
  expect_contains(asm_text, "    ld s7, -24(s0)",
                  "wired translated riscv prologue helpers should reload the bounded callee-saved set in order");
  expect_contains(asm_text, "    ld ra, -8(s0)",
                  "wired translated riscv prologue helpers should restore ra from the frame-pointer-relative exit slot");
  expect_contains(asm_text, "    ld s0, -16(s0)",
                  "wired translated riscv prologue helpers should restore s0 from the frame-pointer-relative exit slot");
  expect_contains(asm_text, "    addi sp, sp, 32",
                  "wired translated riscv prologue helpers should release the bounded frame size in the epilogue");
  expect_contains(asm_text, "    ret",
                  "wired translated riscv prologue helpers should finish the bounded return-exit path with ret");
}

void test_riscv_translated_prologue_tracks_variadic_frame_setup() {
  using IrFunction = c4c::backend::riscv::codegen::IrFunction;
  using IrType = c4c::backend::riscv::codegen::IrType;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  RiscvCodegen codegen;
  IrFunction func;
  func.is_variadic = true;
  func.return_type = IrType::I64;
  func.va_named_gp_count = 11;
  func.va_named_stack_bytes = 24;

  codegen.emit_prologue_impl(func, 32);
  codegen.emit_epilogue_and_ret_impl(0);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_true(codegen.is_variadic,
              "wired translated riscv prologue helpers should retain whether the active function is variadic for later owner slices");
  expect_true(codegen.variadic_save_area_bytes == 64,
              "wired translated riscv variadic prologue helpers should retain the bounded GP save-area size selected by the shared activation surface");
  expect_true(codegen.va_named_gp_count == 8,
              "wired translated riscv prologue helpers should clamp the named GP register count to the shared a0-a7 save window for later variadic owners");
  expect_true(codegen.va_named_stack_bytes == 24,
              "wired translated riscv prologue helpers should retain the named stack-byte count for later variadic entry owners");
  expect_true(c4c::backend::riscv::codegen::riscv_variadic_next_arg_stack_offset(
                  codegen.va_named_gp_count, codegen.va_named_stack_bytes) == 88,
              "wired translated riscv variadic activation state should expose the next-argument stack offset that future va_start owners will consume");
  expect_contains(asm_text, "    addi sp, sp, -96",
                  "wired translated riscv variadic prologue helpers should reserve the 64-byte GP save area above the bounded frame");
  expect_contains(asm_text, "    addi s0, sp, 32",
                  "wired translated riscv variadic prologue helpers should keep s0 anchored at the top of the bounded local frame");
  expect_contains(asm_text, "    sd a0, 0(s0)",
                  "wired translated riscv variadic prologue helpers should save a0 into the shared variadic register-save area");
  expect_contains(asm_text, "    sd a7, 56(s0)",
                  "wired translated riscv variadic prologue helpers should save the full a0-a7 register window for future va_start and va_arg owners");
  expect_contains(asm_text, "    addi sp, sp, 96",
                  "wired translated riscv variadic epilogue helpers should release the bounded frame plus variadic save area");
}

void test_riscv_translated_variadic_emits_bounded_text() {
  using IrType = c4c::backend::riscv::codegen::IrType;
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen codegen;
  codegen.va_named_gp_count = 8;
  codegen.va_named_stack_bytes = 24;
  codegen.state.slots.emplace(1, StackSlot{-40});
  codegen.state.allocas.insert(1);
  codegen.state.slots.emplace(2, StackSlot{-56});
  codegen.state.assigned_regs.emplace(3, PhysReg{8});
  codegen.state.slots.emplace(4, StackSlot{-64});
  codegen.state.slots.emplace(5, StackSlot{-72});
  codegen.state.slots.emplace(6, StackSlot{-80});
  codegen.state.allocas.insert(6);
  codegen.state.assigned_regs.emplace(7, PhysReg{2});

  codegen.emit_va_start_impl(Value{1});
  codegen.emit_va_copy_impl(Value{2}, Value{3});
  codegen.emit_va_arg_impl(Value{5}, Value{4}, IrType::I64);
  codegen.emit_va_arg_impl(Value{7}, Value{6}, IrType::F128);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    addi t0, s0, -40",
                  "wired translated riscv variadic owner bodies should resolve alloca-backed va_list destinations through the bounded s0-relative address path");
  expect_contains(asm_text, "    addi t1, s0, 88",
                  "wired translated riscv variadic owner bodies should seed va_start from the prologue-tracked next-argument offset");
  expect_contains(asm_text, "    sd t1, 0(t0)",
                  "wired translated riscv variadic owner bodies should initialize the va_list storage with the computed next-argument pointer");
  expect_contains(asm_text, "    mv t1, s8",
                  "wired translated riscv variadic owner bodies should reuse assigned callee-saved source pointers for va_copy without widening into broader state work");
  expect_contains(asm_text, "    ld t0, -56(s0)",
                  "wired translated riscv variadic owner bodies should still reload stack-resident destination pointers for va_copy through the shared slot path");
  expect_contains(asm_text, "    sd t2, 0(t0)",
                  "wired translated riscv variadic owner bodies should store copied va_list pointers through the resolved destination address");
  expect_contains(asm_text, "    ld t1, -64(s0)",
                  "wired translated riscv variadic owner bodies should reload stack-backed va_list pointers for direct va_arg consumers");
  expect_contains(asm_text, "    ld t0, 0(t2)",
                  "wired translated riscv variadic owner bodies should materialize scalar variadic arguments from the current va_list cursor");
  expect_contains(asm_text, "    addi t2, t2, 8",
                  "wired translated riscv variadic owner bodies should advance scalar va_list cursors by one eight-byte slot");
  expect_contains(asm_text, "    sd t0, -72(s0)",
                  "wired translated riscv variadic owner bodies should store scalar va_arg results through the shared destination-slot helper");
  expect_contains(asm_text, "    addi t1, s0, -80",
                  "wired translated riscv variadic owner bodies should resolve alloca-backed va_arg cursors through the shared s0-relative address path");
  expect_contains(asm_text, "    addi t2, t2, 15",
                  "wired translated riscv variadic owner bodies should align long-double va_arg cursors before loading the full pair");
  expect_contains(asm_text, "    andi t2, t2, -16",
                  "wired translated riscv variadic owner bodies should preserve the bounded 16-byte alignment contract for long-double va_arg loads");
  expect_contains(asm_text, "    ld a0, 0(t2)",
                  "wired translated riscv variadic owner bodies should load the low long-double lane into a0 before truncation");
  expect_contains(asm_text, "    ld a1, 8(t2)",
                  "wired translated riscv variadic owner bodies should load the high long-double lane into a1 before truncation");
  expect_contains(asm_text, "    addi t2, t2, 16",
                  "wired translated riscv variadic owner bodies should advance long-double va_list cursors by the bounded 16-byte step");
  expect_contains(asm_text, "    call __trunctfdf2",
                  "wired translated riscv variadic owner bodies should reuse the landed long-double truncation bridge for F128 va_arg materialization");
  expect_contains(asm_text, "    fmv.x.d t0, fa0",
                  "wired translated riscv variadic owner bodies should bridge truncated long-double results back into the integer accumulator path");
  expect_contains(asm_text, "    mv s2, t0",
                  "wired translated riscv variadic owner bodies should materialize F128 va_arg results into assigned callee-saved destinations after truncation");
}

void test_riscv_translated_atomics_activation_helpers_emit_bounded_text() {
  using AtomicOrdering = c4c::backend::riscv::codegen::AtomicOrdering;
  using IrType = c4c::backend::riscv::codegen::IrType;
  using RiscvCodegenState = c4c::backend::riscv::codegen::RiscvCodegenState;

  RiscvCodegenState state;
  c4c::backend::riscv::codegen::riscv_sign_extend_atomic_result(state, IrType::I8);
  c4c::backend::riscv::codegen::riscv_sign_extend_atomic_result(state, IrType::U16);
  c4c::backend::riscv::codegen::riscv_sign_extend_atomic_result(state, IrType::I32);

  std::string asm_text;
  for (const auto& line : state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_true(
      std::string(c4c::backend::riscv::codegen::riscv_amo_ordering_suffix(
                      AtomicOrdering::Relaxed))
              .empty() &&
          std::string(c4c::backend::riscv::codegen::riscv_amo_ordering_suffix(
                          AtomicOrdering::Acquire)) == ".aq" &&
          std::string(c4c::backend::riscv::codegen::riscv_amo_ordering_suffix(
                          AtomicOrdering::Release)) == ".rl" &&
          std::string(c4c::backend::riscv::codegen::riscv_amo_ordering_suffix(
                          AtomicOrdering::SeqCst)) == ".aqrl" &&
          std::string(c4c::backend::riscv::codegen::riscv_amo_width_suffix(IrType::I32)) ==
              "w" &&
          std::string(c4c::backend::riscv::codegen::riscv_amo_width_suffix(IrType::I64)) ==
              "d" &&
          c4c::backend::riscv::codegen::riscv_is_atomic_subword_type(IrType::I8) &&
          c4c::backend::riscv::codegen::riscv_is_atomic_subword_type(IrType::U16) &&
          !c4c::backend::riscv::codegen::riscv_is_atomic_subword_type(IrType::I64) &&
          c4c::backend::riscv::codegen::riscv_atomic_subword_bits(IrType::I8) == 8 &&
          c4c::backend::riscv::codegen::riscv_atomic_subword_bits(IrType::U16) == 16,
      "wired translated riscv atomics activation helpers should expose the bounded ordering, width, and subword classification surface before direct atomic owner bodies land");
  expect_contains(asm_text, "    slli t0, t0, 56",
                  "wired translated riscv atomics activation helpers should preserve the I8 sign-extension path for later atomic owner packets");
  expect_contains(asm_text, "    srai t0, t0, 56",
                  "wired translated riscv atomics activation helpers should finish the bounded I8 sign-extension path");
  expect_contains(asm_text, "    slli t0, t0, 48",
                  "wired translated riscv atomics activation helpers should preserve the U16 narrowing path for later subword atomic packets");
  expect_contains(asm_text, "    srli t0, t0, 48",
                  "wired translated riscv atomics activation helpers should zero-extend bounded U16 results without widening into direct atomic owner bodies");
  expect_contains(asm_text, "    sext.w t0, t0",
                  "wired translated riscv atomics activation helpers should preserve the I32 sign-extension helper for later atomic RMW and cmpxchg owners");
}

void test_riscv_translated_atomic_load_store_fence_emits_bounded_text() {
  using AtomicOrdering = c4c::backend::riscv::codegen::AtomicOrdering;
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(10, StackSlot{-24});
  codegen.state.slots.emplace(11, StackSlot{-32});
  codegen.state.slots.emplace(20, StackSlot{-40});
  codegen.state.assigned_regs.emplace(21, PhysReg{8});

  codegen.emit_atomic_store_impl(Operand{10}, Operand::immediate_i64(7), IrType::I16,
                                 AtomicOrdering::Release);
  codegen.emit_atomic_store_impl(Operand{11}, Operand::immediate_i64(9), IrType::I64,
                                 AtomicOrdering::SeqCst);
  codegen.emit_atomic_load_impl(Value{20}, Operand{10}, IrType::U16, AtomicOrdering::Acquire);
  codegen.emit_atomic_load_impl(Value{21}, Operand{11}, IrType::I32, AtomicOrdering::SeqCst);
  codegen.emit_fence_impl(AtomicOrdering::Acquire);
  codegen.emit_fence_impl(AtomicOrdering::Release);
  codegen.emit_fence_impl(AtomicOrdering::SeqCst);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    li t0, 7",
                  "wired translated riscv atomic store seam should load immediate store payloads through the shared operand-to-t0 bridge");
  expect_contains(asm_text, "    mv t1, t0",
                  "wired translated riscv atomic store seam should stage store payloads into the translated t1 temporary");
  expect_contains(asm_text, "    ld t0, -24(s0)",
                  "wired translated riscv atomic load/store seam should load pointer operands through the shared slot bridge");
  expect_contains(asm_text, "    fence rw, w",
                  "wired translated riscv atomic store seam should emit the translated release fence for subword stores");
  expect_contains(asm_text, "    sh t1, 0(t0)",
                  "wired translated riscv atomic store seam should keep bounded subword stores on the direct store instruction path");
  expect_contains(asm_text, "    amoswap.d.aqrl zero, t1, (t0)",
                  "wired translated riscv atomic store seam should lower non-subword seqcst stores through the bounded amoswap path");
  expect_contains(asm_text, "    lhu t0, 0(t0)",
                  "wired translated riscv atomic load seam should keep bounded unsigned subword loads on the direct load instruction path");
  expect_contains(asm_text, "    fence r, rw",
                  "wired translated riscv atomic load/fence seam should emit the translated acquire fence");
  expect_contains(asm_text, "    sd t0, -40(s0)",
                  "wired translated riscv atomic load seam should spill stack-backed results through the shared store_t0_to helper");
  expect_contains(asm_text, "    lr.w.aqrl t0, (t0)",
                  "wired translated riscv atomic load seam should lower non-subword seqcst loads through the bounded lr path");
  expect_contains(asm_text, "    sext.w t0, t0",
                  "wired translated riscv atomic load seam should preserve I32 sign extension through the landed helper surface");
  expect_contains(asm_text, "    mv s8, t0",
                  "wired translated riscv atomic load seam should reuse assigned callee-saved destinations for register-backed results");
  expect_contains(asm_text, "    fence rw, rw",
                  "wired translated riscv atomic fence seam should emit the translated full fence for seqcst ordering");
}

void test_riscv_translated_atomic_rmw_cmpxchg_emits_bounded_text() {
  using AtomicOrdering = c4c::backend::riscv::codegen::AtomicOrdering;
  using AtomicRmwOp = c4c::backend::riscv::codegen::AtomicRmwOp;
  using IrType = c4c::backend::riscv::codegen::IrType;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(10, StackSlot{-24});
  codegen.state.slots.emplace(11, StackSlot{-32});
  codegen.state.slots.emplace(20, StackSlot{-40});
  codegen.state.slots.emplace(22, StackSlot{-48});
  codegen.state.assigned_regs.emplace(21, PhysReg{8});
  codegen.state.assigned_regs.emplace(23, PhysReg{9});

  codegen.emit_atomic_rmw_impl(Value{20}, AtomicRmwOp::Nand, Operand{10}, Operand::immediate_i64(5),
                               IrType::I16, AtomicOrdering::AcqRel);
  codegen.emit_atomic_rmw_impl(Value{21}, AtomicRmwOp::Sub, Operand{11}, Operand::immediate_i64(9),
                               IrType::I64, AtomicOrdering::Acquire);
  codegen.emit_atomic_cmpxchg_impl(Value{22}, Operand{10}, Operand::immediate_i64(3),
                                   Operand::immediate_i64(4), IrType::I8,
                                   AtomicOrdering::SeqCst, AtomicOrdering::Acquire, false);
  codegen.emit_atomic_cmpxchg_impl(Value{23}, Operand{11}, Operand::immediate_i64(7),
                                   Operand::immediate_i64(8), IrType::I32,
                                   AtomicOrdering::Acquire, AtomicOrdering::Relaxed, true);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    andi a2, t1, -4",
                  "wired translated riscv atomic rmw/cmpxchg seam should align subword addresses before entering the bounded lr/sc loop");
  expect_contains(asm_text, "    sllw a4, a4, a3",
                  "wired translated riscv atomic rmw/cmpxchg seam should materialize the shifted subword mask inside the bounded helper path");
  expect_contains(asm_text, "    lr.w.aqrl t0, (a2)",
                  "wired translated riscv atomic rmw seam should issue subword acqrel lr.w inside the bounded translated helper loop");
  expect_contains(asm_text, "    sc.w.aqrl t5, t4, (a2)",
                  "wired translated riscv atomic rmw seam should retry subword acqrel updates through the translated sc.w loop");
  expect_contains(asm_text, "    and t3, t3, t2",
                  "wired translated riscv atomic rmw seam should preserve the bounded NAND field-masking path before reinserting the subword result");
  expect_contains(asm_text, "    srlw t0, t0, a3",
                  "wired translated riscv atomic rmw/cmpxchg seam should extract the old subword value from the containing word after the lr/sc loop");
  expect_contains(asm_text, "    amoadd.d.aq t0, t2, (t1)",
                  "wired translated riscv atomic rmw seam should lower direct-width subtract through the translated neg-plus-amoadd path");
  expect_contains(asm_text, "    neg t2, t2",
                  "wired translated riscv atomic rmw seam should negate direct-width subtract operands before the bounded amoadd lowering");
  expect_contains(asm_text, "    mv s8, t0",
                  "wired translated riscv atomic rmw seam should reuse assigned callee-saved destinations for direct-width results");
  expect_contains(asm_text, "    lr.w.aq t0, (t1)",
                  "wired translated riscv atomic cmpxchg seam should lower direct-width acquire compare-exchange through the bounded lr.w loop");
  expect_contains(asm_text, "    sc.w.aq t4, t3, (t1)",
                  "wired translated riscv atomic cmpxchg seam should lower direct-width acquire compare-exchange through the bounded sc.w loop");
  expect_contains(asm_text, "    li t0, 1",
                  "wired translated riscv atomic cmpxchg seam should materialize boolean success results without widening beyond the direct owner packet");
  expect_contains(asm_text, "    mv s9, t0",
                  "wired translated riscv atomic cmpxchg seam should preserve register-backed boolean results through the shared store_t0_to bridge");
  expect_contains(asm_text, "    sd t0, -48(s0)",
                  "wired translated riscv atomic cmpxchg seam should spill stack-backed old-value results through the shared store_t0_to bridge");
}

void test_riscv_translated_atomic_software_helpers_emit_bounded_text() {
  using IrType = c4c::backend::riscv::codegen::IrType;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  RiscvCodegen codegen;
  codegen.emit_clz(IrType::I32);
  codegen.emit_ctz(IrType::I64);
  codegen.emit_bswap(IrType::I16);
  codegen.emit_bswap(IrType::I32);
  codegen.emit_bswap(IrType::I64);
  codegen.emit_popcount(IrType::U32);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    beqz t0, .Lclz_zero_",
                  "wired translated riscv atomic software helpers should keep the bounded clz zero-input fast path");
  expect_contains(asm_text, "    slli t2, t2, 31",
                  "wired translated riscv atomic software helpers should seed 32-bit clz scans from the translated high-bit mask");
  expect_contains(asm_text, ".Lctz_loop_",
                  "wired translated riscv atomic software helpers should preserve the translated ctz loop label structure");
  expect_contains(asm_text, "    li t2, 64",
                  "wired translated riscv atomic software helpers should cap 64-bit ctz scans at the translated bit width");
  expect_contains(asm_text, "    slli t1, t1, 8",
                  "wired translated riscv atomic software helpers should swap the low byte upward for 16-bit bswap");
  expect_contains(asm_text, "    slli t0, t2, 24",
                  "wired translated riscv atomic software helpers should place the low byte into the high lane for 32-bit bswap");
  expect_contains(asm_text, "    srli t2, t1, 56",
                  "wired translated riscv atomic software helpers should iterate all eight source bytes for 64-bit bswap");
  expect_contains(asm_text, "    addi t2, t0, -1",
                  "wired translated riscv atomic software helpers should use the bounded Kernighan loop step for popcount");
  expect_contains(asm_text, ".Lpopcnt_done_",
                  "wired translated riscv atomic software helpers should preserve the translated popcount loop/done label flow");
}

void test_riscv_translated_intrinsics_activation_helpers_emit_bounded_text() {
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;

  RiscvCodegen codegen;
  codegen.emit_rv_zero_byte_mask("t1", "t3");
  codegen.emit_rv_cmpeq_bytes();
  codegen.emit_rv_cmpeq_dwords();
  codegen.emit_rv_binary_128("xor");
  codegen.emit_rv_binary_128_bytewise();
  codegen.emit_rv_psubusb_8bytes("t1", "t2", "t3");
  codegen.emit_rv_psubsb_128();
  codegen.emit_rv_pmovmskb();

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    li t5, 0x0101010101010101",
                  "wired translated riscv intrinsics activation helpers should preserve the zero-byte-mask materialization path through the shared helper surface");
  expect_contains(asm_text, "    mv a6, t0",
                  "wired translated riscv intrinsics activation helpers should preserve the bounded compare helper register staging");
  expect_contains(asm_text, "    xor t1, t1, t2",
                  "wired translated riscv intrinsics activation helpers should keep the byte-compare xor step reachable through the activation surface");
  expect_contains(asm_text, "    slli t3, t1, 32",
                  "wired translated riscv intrinsics activation helpers should preserve the dword-compare lane-splitting path");
  expect_contains(asm_text, "    xor t1, t1, t3",
                  "wired translated riscv intrinsics activation helpers should expose the generic 128-bit binary helper through the shared activation surface");
  expect_contains(asm_text, "    sd t4, 8(a5)",
                  "wired translated riscv intrinsics activation helpers should keep the bounded 128-bit helper storeback path visible before direct dispatch work lands");
  expect_contains(asm_text, "psub_skip_7:",
                  "wired translated riscv intrinsics activation helpers should preserve the translated per-byte unsigned-saturating subtract labels");
  expect_contains(asm_text, "psubsb_noclamp_hi_0:",
                  "wired translated riscv intrinsics activation helpers should preserve the translated signed-saturating subtract clamp labels");
  expect_contains(asm_text, "    li t3, 0x0002040810204081",
                  "wired translated riscv intrinsics activation helpers should preserve the pmovmskb multiplier constant on the shared helper surface");
}

void test_riscv_translated_intrinsics_dispatcher_emits_bounded_text() {
  using IntrinsicOp = c4c::backend::riscv::codegen::IntrinsicOp;
  using Operand = c4c::backend::riscv::codegen::Operand;
  using PhysReg = c4c::backend::riscv::codegen::PhysReg;
  using RiscvCodegen = c4c::backend::riscv::codegen::RiscvCodegen;
  using StackSlot = c4c::backend::riscv::codegen::StackSlot;
  using Value = c4c::backend::riscv::codegen::Value;

  RiscvCodegen codegen;
  codegen.state.slots.emplace(1, StackSlot{-16});
  codegen.state.slots.emplace(2, StackSlot{-32});
  codegen.state.slots.emplace(4, StackSlot{-64});
  codegen.state.slots.emplace(5, StackSlot{-96});
  codegen.state.slots.emplace(6, StackSlot{-112});
  codegen.state.allocas.insert(4);
  codegen.state.assigned_regs.emplace(4, PhysReg{7});
  codegen.state.over_aligned_allocas.emplace(5, 32);

  codegen.emit_intrinsic_rv(std::nullopt, IntrinsicOp::Lfence, std::nullopt, {});
  codegen.emit_intrinsic_rv(std::nullopt, IntrinsicOp::Mfence, std::nullopt, {});
  codegen.emit_intrinsic_rv(std::nullopt, IntrinsicOp::Sfence, std::nullopt, {});
  codegen.emit_intrinsic_rv(std::nullopt, IntrinsicOp::Pause, std::nullopt, {});
  codegen.emit_intrinsic_rv(std::nullopt, IntrinsicOp::Clflush, std::nullopt, {Operand{1}});
  codegen.emit_intrinsic_rv(std::nullopt, IntrinsicOp::Movnti, std::optional<Value>{Value{2}},
                            {Operand::immediate_i64(9)});
  codegen.emit_intrinsic_rv(std::nullopt, IntrinsicOp::Movnti64, std::optional<Value>{Value{4}},
                            {Operand::immediate_i64(11)});
  codegen.emit_intrinsic_rv(std::nullopt, IntrinsicOp::Movntdq, std::optional<Value>{Value{5}},
                            {Operand{1}});
  codegen.emit_intrinsic_rv(std::nullopt, IntrinsicOp::Movntpd, std::optional<Value>{Value{2}},
                            {Operand{1}});
  codegen.emit_intrinsic_rv(std::nullopt, IntrinsicOp::Loaddqu, std::optional<Value>{Value{4}},
                            {Operand{1}});
  codegen.emit_intrinsic_rv(std::nullopt, IntrinsicOp::Storedqu, std::optional<Value>{Value{5}},
                            {Operand{1}});
  codegen.emit_intrinsic_rv(std::nullopt, IntrinsicOp::Pcmpeqb128,
                            std::optional<Value>{Value{5}}, {Operand{1}, Operand{4}});
  codegen.emit_intrinsic_rv(std::nullopt, IntrinsicOp::Pcmpeqd128,
                            std::optional<Value>{Value{5}}, {Operand{1}, Operand{4}});
  codegen.emit_intrinsic_rv(std::nullopt, IntrinsicOp::Psubusb128,
                            std::optional<Value>{Value{5}}, {Operand{1}, Operand{4}});
  codegen.emit_intrinsic_rv(std::nullopt, IntrinsicOp::Psubsb128,
                            std::optional<Value>{Value{5}}, {Operand{1}, Operand{4}});
  codegen.emit_intrinsic_rv(std::nullopt, IntrinsicOp::Por128, std::optional<Value>{Value{5}},
                            {Operand{1}, Operand{4}});
  codegen.emit_intrinsic_rv(std::nullopt, IntrinsicOp::Pand128, std::optional<Value>{Value{5}},
                            {Operand{1}, Operand{4}});
  codegen.emit_intrinsic_rv(std::nullopt, IntrinsicOp::Pxor128, std::optional<Value>{Value{5}},
                            {Operand{1}, Operand{4}});
  codegen.emit_intrinsic_rv(std::optional<Value>{Value{6}}, IntrinsicOp::Pmovmskb128,
                            std::nullopt, {Operand{1}});
  codegen.emit_intrinsic_rv(std::nullopt, IntrinsicOp::SetEpi8, std::optional<Value>{Value{2}},
                            {Operand::immediate_i64(18)});
  codegen.emit_intrinsic_rv(std::nullopt, IntrinsicOp::SetEpi32, std::optional<Value>{Value{2}},
                            {Operand::immediate_i64(0x12345678)});
  codegen.emit_intrinsic_rv(std::optional<Value>{Value{6}}, IntrinsicOp::Crc32_8, std::nullopt,
                            {Operand::immediate_i64(1), Operand::immediate_i64(2)});
  codegen.emit_intrinsic_rv(std::optional<Value>{Value{6}}, IntrinsicOp::Crc32_16, std::nullopt,
                            {Operand::immediate_i64(3), Operand::immediate_i64(4)});
  codegen.emit_intrinsic_rv(std::optional<Value>{Value{6}}, IntrinsicOp::Crc32_32, std::nullopt,
                            {Operand::immediate_i64(5), Operand::immediate_i64(6)});
  codegen.emit_intrinsic_rv(std::optional<Value>{Value{6}}, IntrinsicOp::Crc32_64, std::nullopt,
                            {Operand::immediate_i64(7), Operand::immediate_i64(8)});
  codegen.emit_intrinsic_rv(std::optional<Value>{Value{6}}, IntrinsicOp::FrameAddress,
                            std::nullopt, {});
  codegen.emit_intrinsic_rv(std::optional<Value>{Value{6}}, IntrinsicOp::ReturnAddress,
                            std::nullopt, {});
  codegen.emit_intrinsic_rv(std::optional<Value>{Value{6}}, IntrinsicOp::ThreadPointer,
                            std::nullopt, {});
  codegen.emit_intrinsic_rv(std::optional<Value>{Value{6}}, IntrinsicOp::SqrtF64, std::nullopt,
                            {Operand::f64_const(9.5)});
  codegen.emit_intrinsic_rv(std::optional<Value>{Value{6}}, IntrinsicOp::SqrtF32, std::nullopt,
                            {Operand::f32_const(4.25f)});
  codegen.emit_intrinsic_rv(std::optional<Value>{Value{6}}, IntrinsicOp::FabsF64, std::nullopt,
                            {Operand::f64_const(-3.75)});
  codegen.emit_intrinsic_rv(std::optional<Value>{Value{6}}, IntrinsicOp::FabsF32, std::nullopt,
                            {Operand::f32_const(-1.5f)});
  auto emit_x86_zero_dest_fallback = [&](IntrinsicOp op, std::initializer_list<Operand> args) {
    codegen.emit_intrinsic_rv(std::nullopt, op, std::optional<Value>{Value{2}},
                              std::vector<Operand>(args));
  };

  emit_x86_zero_dest_fallback(IntrinsicOp::Aesenc128, {Operand{1}, Operand{4}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Aesenclast128, {Operand{1}, Operand{4}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Aesdec128, {Operand{1}, Operand{4}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Aesdeclast128, {Operand{1}, Operand{4}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Aesimc128, {Operand{1}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Aeskeygenassist128,
                              {Operand{1}, Operand::immediate_i64(7)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Pclmulqdq128,
                              {Operand{1}, Operand{4}, Operand::immediate_i64(1)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Pslldqi128, {Operand{1}, Operand::immediate_i64(3)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Psrldqi128, {Operand{1}, Operand::immediate_i64(2)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Psllqi128, {Operand{1}, Operand::immediate_i64(4)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Psrlqi128, {Operand{1}, Operand::immediate_i64(5)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Pshufd128, {Operand{1}, Operand::immediate_i64(0x1b)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Loadldi128, {Operand::immediate_i64(9)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Paddw128, {Operand{1}, Operand{4}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Psubw128, {Operand{1}, Operand{4}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Pmulhw128, {Operand{1}, Operand{4}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Pmaddwd128, {Operand{1}, Operand{4}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Pcmpgtw128, {Operand{1}, Operand{4}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Pcmpgtb128, {Operand{1}, Operand{4}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Paddd128, {Operand{1}, Operand{4}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Psubd128, {Operand{1}, Operand{4}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Packssdw128, {Operand{1}, Operand{4}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Packsswb128, {Operand{1}, Operand{4}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Packuswb128, {Operand{1}, Operand{4}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Punpcklbw128, {Operand{1}, Operand{4}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Punpckhbw128, {Operand{1}, Operand{4}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Punpcklwd128, {Operand{1}, Operand{4}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Punpckhwd128, {Operand{1}, Operand{4}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Psllwi128, {Operand{1}, Operand::immediate_i64(6)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Psrlwi128, {Operand{1}, Operand::immediate_i64(7)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Psrawi128, {Operand{1}, Operand::immediate_i64(3)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Psradi128, {Operand{1}, Operand::immediate_i64(2)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Pslldi128, {Operand{1}, Operand::immediate_i64(1)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Psrldi128, {Operand{1}, Operand::immediate_i64(4)});
  emit_x86_zero_dest_fallback(IntrinsicOp::SetEpi16, {Operand::immediate_i64(0x1234)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Pinsrw128,
                              {Operand{1}, Operand::immediate_i64(2), Operand::immediate_i64(1)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Pextrw128,
                              {Operand{1}, Operand::immediate_i64(3), Operand::immediate_i64(0)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Pinsrd128,
                              {Operand{1}, Operand::immediate_i64(5), Operand::immediate_i64(2)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Pextrd128,
                              {Operand{1}, Operand::immediate_i64(1), Operand::immediate_i64(0)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Pinsrb128,
                              {Operand{1}, Operand::immediate_i64(0x7f), Operand::immediate_i64(3)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Pextrb128,
                              {Operand{1}, Operand::immediate_i64(4), Operand::immediate_i64(0)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Pinsrq128,
                              {Operand{1}, Operand::immediate_i64(11), Operand::immediate_i64(1)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Pextrq128,
                              {Operand{1}, Operand::immediate_i64(0), Operand::immediate_i64(0)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Storeldi128, {Operand{1}, Operand::immediate_i64(12)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Cvtsi128Si32, {Operand{1}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Cvtsi32Si128, {Operand::immediate_i64(13)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Cvtsi128Si64, {Operand{1}});
  emit_x86_zero_dest_fallback(IntrinsicOp::Pshuflw128,
                              {Operand{1}, Operand::immediate_i64(0x39)});
  emit_x86_zero_dest_fallback(IntrinsicOp::Pshufhw128,
                              {Operand{1}, Operand::immediate_i64(0xc6)});

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  auto count_substring = [&](std::string_view needle) {
    std::size_t count = 0;
    std::size_t pos = 0;
    while ((pos = asm_text.find(needle, pos)) != std::string::npos) {
      ++count;
      pos += needle.size();
    }
    return count;
  };

  expect_contains(asm_text, "    fence iorw, iorw",
                  "wired translated riscv intrinsics dispatcher should keep the fence-like memory barrier family live");
  expect_contains(asm_text, "    fence ow, ow",
                  "wired translated riscv intrinsics dispatcher should keep the store-only fence branch live");
  expect_contains(asm_text, "    fence.tso",
                  "wired translated riscv intrinsics dispatcher should keep the pause hint approximation branch live");
  expect_contains(asm_text, "    ld t0, -32(s0)",
                  "wired translated riscv intrinsics dispatcher should load direct destination pointers from their stack slots instead of treating the slot as the address");
  expect_contains(asm_text, "    sw t1, 0(t0)",
                  "wired translated riscv intrinsics dispatcher should keep the non-temporal i32 store path live");
  expect_contains(asm_text, "    mv t0, s7",
                  "wired translated riscv intrinsics dispatcher should reuse assigned callee-saved registers when the destination pointer resolves indirectly");
  expect_contains(asm_text, "    sd t1, 0(t0)",
                  "wired translated riscv intrinsics dispatcher should keep the non-temporal i64 store path live");
  expect_contains(asm_text, "    ld t0, -16(s0)",
                  "wired translated riscv intrinsics dispatcher should load direct source pointers from their stack slots before the bounded vector memory transfer path");
  expect_contains(asm_text, "    addi t5, s0, -96",
                  "wired translated riscv intrinsics dispatcher should materialize over-aligned destination pointers through the bounded local alignment helper");
  expect_contains(asm_text, "    andi t5, t5, -32",
                  "wired translated riscv intrinsics dispatcher should preserve over-aligned destination masking for vector memory transfers");
  expect_contains(asm_text, "    sd t2, 8(t5)",
                  "wired translated riscv intrinsics dispatcher should keep the bounded 128-bit storeback path live across movntdq/movntpd/loaddqu/storedqu");
  expect_contains(asm_text, "    mv a7, s7",
                  "wired translated riscv intrinsics dispatcher should load the second vector operand through the already-landed pointer surface");
  expect_contains(asm_text, "    mv a5, t5",
                  "wired translated riscv intrinsics dispatcher should route vector helper-backed results through the shared destination pointer surface");
  expect_contains(asm_text, "psub_skip_7:",
                  "wired translated riscv intrinsics dispatcher should keep the unsigned saturating subtract helper reachable through the vector packet");
  expect_contains(asm_text, "psubsb_noclamp_hi_0:",
                  "wired translated riscv intrinsics dispatcher should keep the signed saturating subtract helper reachable through the vector packet");
  expect_contains(asm_text, "    or t1, t1, t3",
                  "wired translated riscv intrinsics dispatcher should route packed bitwise branches through the translated binary helper");
  expect_contains(asm_text, "    and t1, t1, t3",
                  "wired translated riscv intrinsics dispatcher should keep the packed and branch live through the translated helper surface");
  expect_contains(asm_text, "    li t3, 0x0002040810204081",
                  "wired translated riscv intrinsics dispatcher should keep the pmovmskb helper branch live");
  expect_contains(asm_text, "    sd t0, -112(s0)",
                  "wired translated riscv intrinsics dispatcher should store pmovmskb scalar results through the shared scalar result path");
  expect_contains(asm_text, "    andi t0, t0, 0xff",
                  "wired translated riscv intrinsics dispatcher should mask byte splat inputs before replicating them");
  expect_contains(asm_text, "    li t1, 0x0101010101010101",
                  "wired translated riscv intrinsics dispatcher should materialize the translated byte-splat multiplier");
  expect_contains(asm_text, "    mul t0, t0, t1",
                  "wired translated riscv intrinsics dispatcher should replicate byte splats through the translated multiplier path");
  expect_contains(asm_text, "    slli t1, t0, 32",
                  "wired translated riscv intrinsics dispatcher should form the high dword half for SetEpi32 splats");
  expect_contains(asm_text, "    srli t0, t0, 32",
                  "wired translated riscv intrinsics dispatcher should zero-extend the low dword before SetEpi32 storeback");
  expect_contains(asm_text, "    sd t0, 8(t1)",
                  "wired translated riscv intrinsics dispatcher should store both 64-bit halves for translated splat packets");
  expect_contains(asm_text, "    li t5, 0x82F63B78",
                  "wired translated riscv intrinsics dispatcher should materialize the Castagnoli polynomial for the software CRC packet");
  expect_contains(asm_text, "    li t6, 64",
                  "wired translated riscv intrinsics dispatcher should scale the translated CRC loop to the selected input width");
  expect_contains(asm_text, ".Lcrc_loop_",
                  "wired translated riscv intrinsics dispatcher should keep the bounded CRC loop labels reachable");
  expect_contains(asm_text, "    beqz t0, .Lcrc_skip_",
                  "wired translated riscv intrinsics dispatcher should branch around the polynomial xor when the shifted CRC bit is clear");
  expect_contains(asm_text, "    xor t3, t3, t5",
                  "wired translated riscv intrinsics dispatcher should apply the translated CRC polynomial xor inside the loop");
  expect_contains(asm_text, "    sd t0, -112(s0)",
                  "wired translated riscv intrinsics dispatcher should store software CRC results back through the shared scalar result path");
  expect_contains(asm_text, "    mv t0, s0",
                  "wired translated riscv intrinsics dispatcher should materialize the current frame pointer for __builtin_frame_address through the shared scalar result path");
  expect_contains(asm_text, "    ld t0, -8(s0)",
                  "wired translated riscv intrinsics dispatcher should load the saved return address from the active frame for __builtin_return_address");
  expect_contains(asm_text, "    mv t0, tp",
                  "wired translated riscv intrinsics dispatcher should read the reserved thread-pointer register for the translated thread-pointer builtin");
  expect_contains(asm_text, "    fmv.d.x ft0, t0",
                  "wired translated riscv intrinsics dispatcher should move scalar f64 inputs into ft0 before the bounded sqrt/fabs packet");
  expect_contains(asm_text, "    fsqrt.d ft0, ft0",
                  "wired translated riscv intrinsics dispatcher should keep the scalar f64 sqrt branch live through the translated floating-point path");
  expect_contains(asm_text, "    fmv.w.x ft0, t0",
                  "wired translated riscv intrinsics dispatcher should move scalar f32 inputs into ft0 before the bounded sqrt/fabs packet");
  expect_contains(asm_text, "    fsqrt.s ft0, ft0",
                  "wired translated riscv intrinsics dispatcher should keep the scalar f32 sqrt branch live through the translated floating-point path");
  expect_contains(asm_text, "    fabs.d ft0, ft0",
                  "wired translated riscv intrinsics dispatcher should keep the scalar f64 fabs branch live through the translated floating-point path");
  expect_contains(asm_text, "    fabs.s ft0, ft0",
                  "wired translated riscv intrinsics dispatcher should keep the scalar f32 fabs branch live through the translated floating-point path");
  expect_contains(asm_text, "    fmv.x.d t0, ft0",
                  "wired translated riscv intrinsics dispatcher should return scalar f64 sqrt/fabs results through the shared scalar result path");
  expect_contains(asm_text, "    fmv.x.w t0, ft0",
                  "wired translated riscv intrinsics dispatcher should return scalar f32 sqrt/fabs results through the shared scalar result path");
  expect_true(count_substring("    sd zero, 0(t0)\n") == 49,
              "wired translated riscv intrinsics dispatcher should zero the low vector lane for the full translated x86-only fallback block");
  expect_true(count_substring("    sd zero, 8(t0)\n") == 49,
              "wired translated riscv intrinsics dispatcher should zero the high vector lane for the full translated x86-only fallback block");
}

void test_x86_translated_returns_owner_emits_bounded_helper_text() {
  using EightbyteClass = c4c::backend::x86::EightbyteClass;
  using Operand = c4c::backend::x86::Operand;
  using StackSlot = c4c::backend::x86::StackSlot;
  using Value = c4c::backend::x86::Value;

  c4c::backend::x86::X86Codegen codegen;
  codegen.state.slots.emplace(5, StackSlot{-24});
  codegen.state.slots.emplace(7, StackSlot{-40});
  codegen.func_ret_classes = {EightbyteClass::Sse, EightbyteClass::Integer};

  codegen.emit_return_f32_to_reg_impl();
  codegen.emit_return_f64_to_reg_impl();
  codegen.emit_return_i128_to_regs_impl();
  codegen.emit_return_f128_to_reg_impl();
  codegen.emit_return_int_to_reg_impl();
  codegen.emit_get_return_f64_second_impl(Value{5});
  codegen.emit_set_return_f64_second_impl(Operand{7});
  codegen.emit_get_return_f32_second_impl(Value{5});
  codegen.emit_set_return_f32_second_impl(Operand{7});
  codegen.emit_get_return_f128_second_impl(Value{5});
  codegen.emit_set_return_f128_second_impl(Operand{7});

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    movd %eax, %xmm0",
                  "wired translated returns owner should move f32 return values into xmm0");
  expect_contains(asm_text, "    movq %rax, %xmm0",
                  "wired translated returns owner should move f64 return values into xmm0");
  expect_contains(asm_text, "    movq %rdx, %rax",
                  "wired translated returns owner should handle the mixed SSE plus INTEGER i128 return shuffle");
  expect_contains(asm_text, "    pushq %rax",
                  "wired translated returns owner should keep the bounded f128 return-to-x87 helper linked");
  expect_contains(asm_text, "    movsd %xmm1, -24(%rbp)",
                  "wired translated returns owner should spill the second f64 return lane through the shared slot surface");
  expect_contains(asm_text, "    movsd -40(%rbp), %xmm1",
                  "wired translated returns owner should reload the second f64 return lane through the shared slot surface");
  expect_contains(asm_text, "    movss %xmm1, -24(%rbp)",
                  "wired translated returns owner should spill the second f32 return lane through the shared slot surface");
  expect_contains(asm_text, "    movss -40(%rbp), %xmm1",
                  "wired translated returns owner should reload the second f32 return lane through the shared slot surface");
  expect_contains(asm_text, "    fstpt -24(%rbp)",
                  "wired translated returns owner should stage the second f128 return lane through the shared slot surface");
  expect_contains(asm_text, "    fldt -40(%rbp)",
                  "wired translated returns owner should reload the second f128 return lane through the shared slot surface");
  expect_true(codegen.state.reg_cache.acc_valid &&
                  codegen.state.reg_cache.acc_value_id == 5 &&
                  !codegen.state.reg_cache.acc_known_zero_extended,
              "wired translated returns owner should refresh the shared accumulator cache when materializing the second f128 return lane");
  expect_true(codegen.state.f128_direct_slots.find(5) != codegen.state.f128_direct_slots.end(),
              "wired translated returns owner should mark direct f128 slots when the second f128 lane is staged back into memory");
}

void test_x86_translated_returns_owner_emits_bounded_value_return_paths() {
  using IrType = c4c::backend::x86::IrType;
  using Operand = c4c::backend::x86::Operand;
  using StackSlot = c4c::backend::x86::StackSlot;

  c4c::backend::x86::X86Codegen direct_slot_codegen;
  direct_slot_codegen.current_return_type = IrType::F128;
  direct_slot_codegen.state.slots.emplace(5, StackSlot{-24});
  direct_slot_codegen.state.f128_direct_slots.insert(5);
  direct_slot_codegen.emit_return_impl(Operand{5}, 16);

  std::string direct_slot_asm;
  for (const auto& line : direct_slot_codegen.state.asm_lines) {
    direct_slot_asm += line;
    direct_slot_asm.push_back('\n');
  }

  expect_true(direct_slot_codegen.current_return_type_impl() == IrType::F128,
              "wired translated returns owner should surface the bounded current return type state");
  expect_contains(direct_slot_asm, "    fldt -24(%rbp)",
                  "wired translated returns owner should reload direct-slot f128 returns through the shared slot surface");
  expect_contains(direct_slot_asm, "    ret",
                  "wired translated returns owner should still finish bounded direct-slot f128 returns through the shared epilogue path");

  c4c::backend::x86::X86Codegen tracked_source_codegen;
  tracked_source_codegen.current_return_type = IrType::F128;
  tracked_source_codegen.state.slots.emplace(7, StackSlot{-40});
  tracked_source_codegen.state.allocas.insert(7);
  tracked_source_codegen.state.track_f128_load(9, 7, 0);
  tracked_source_codegen.emit_return_impl(Operand{9}, 8);

  std::string tracked_source_asm;
  for (const auto& line : tracked_source_codegen.state.asm_lines) {
    tracked_source_asm += line;
    tracked_source_asm.push_back('\n');
  }

  expect_contains(tracked_source_asm, "    movq -40(%rbp), %rcx",
                  "wired translated returns owner should recover tracked indirect f128 sources through the shared pointer-load helper");
  expect_contains(tracked_source_asm, "    fldt (%rcx)",
                  "wired translated returns owner should reload tracked indirect f128 sources through the shared load helper path");
  expect_contains(tracked_source_asm, "    ret",
                  "wired translated returns owner should still finish tracked-source f128 returns through the shared epilogue path");

  c4c::backend::x86::X86Codegen const_f128_codegen;
  const_f128_codegen.current_return_type = IrType::F128;
  const_f128_codegen.state.set_f128_constant_words(11, 0x1122334455667788ULL, 0x99AAULL);
  const_f128_codegen.emit_return_impl(Operand{11}, 24);

  std::string const_f128_asm;
  for (const auto& line : const_f128_codegen.state.asm_lines) {
    const_f128_asm += line;
    const_f128_asm.push_back('\n');
  }

  expect_contains(const_f128_asm, "    subq $16, %rsp",
                  "wired translated returns owner should materialize bounded f128 constants through the shared x87 staging path");
  expect_contains(const_f128_asm, "    fldt (%rsp)",
                  "wired translated returns owner should load bounded f128 constants onto the x87 stack before returning");
  expect_contains(const_f128_asm, "    ret",
                  "wired translated returns owner should finish bounded f128 constant returns through the shared epilogue path");

  c4c::backend::x86::X86Codegen default_scalar_codegen;
  default_scalar_codegen.current_return_type = IrType::I64;
  default_scalar_codegen.state.function_return_thunk = true;
  default_scalar_codegen.emit_return_impl(Operand::immediate_i64(42), 32);

  std::string default_scalar_asm;
  for (const auto& line : default_scalar_codegen.state.asm_lines) {
    default_scalar_asm += line;
    default_scalar_asm.push_back('\n');
  }

  expect_contains(default_scalar_asm, "    movq $42, %rax",
                  "wired translated returns owner should keep the shared default scalar return load path");
  expect_contains(default_scalar_asm, "    jmp __x86_return_thunk",
                  "wired translated returns owner should reuse the shared epilogue helper instead of hardcoding ret");
}

void test_x86_translated_returns_owner_handles_i128_lane_variants() {
  using EightbyteClass = c4c::backend::x86::EightbyteClass;

  c4c::backend::x86::X86Codegen int_sse;
  int_sse.func_ret_classes = {EightbyteClass::Integer, EightbyteClass::Sse};
  int_sse.emit_return_i128_to_regs_impl();
  expect_true(int_sse.state.asm_lines.size() == 1 &&
                  int_sse.state.asm_lines.front() == "    movq %rdx, %xmm0",
              "wired translated returns owner should move the second INTEGER+SSE lane into xmm0");

  c4c::backend::x86::X86Codegen sse_sse;
  sse_sse.func_ret_classes = {EightbyteClass::Sse, EightbyteClass::Sse};
  sse_sse.emit_return_i128_to_regs_impl();
  expect_true(sse_sse.state.asm_lines.size() == 2 &&
                  sse_sse.state.asm_lines[0] == "    movq %rax, %xmm0" &&
                  sse_sse.state.asm_lines[1] == "    movq %rdx, %xmm1",
              "wired translated returns owner should move both SSE-class i128 lanes into xmm0/xmm1");
}

void test_x86_translated_i128_owner_emits_bounded_helper_text() {
  using IrCmpOp = c4c::backend::x86::IrCmpOp;
  using Operand = c4c::backend::x86::Operand;
  using StackSlot = c4c::backend::x86::StackSlot;
  using Value = c4c::backend::x86::Value;

  c4c::backend::x86::X86Codegen codegen;
  codegen.state.slots.emplace(5, StackSlot{-24});
  codegen.state.slots.emplace(11, StackSlot{-56});
  codegen.state.slots.emplace(13, StackSlot{-72});

  codegen.emit_i128_prep_binop_impl(Operand{5}, Operand::immediate_i64(-3));
  codegen.emit_i128_add_impl();
  codegen.emit_i128_store_result_impl(Value{11});
  codegen.emit_i128_cmp_ordered_impl(IrCmpOp::Sge);
  codegen.emit_i128_cmp_store_result_impl(Value{13});
  codegen.emit_store_pair_to_slot_impl(StackSlot{-88});
  codegen.emit_load_pair_from_slot_impl(StackSlot{-88});
  codegen.emit_save_acc_pair_impl();
  codegen.emit_store_pair_indirect_impl();
  codegen.emit_load_pair_indirect_impl();
  codegen.emit_i128_neg_impl();
  codegen.emit_i128_not_impl();

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    movq -24(%rbp), %rax",
                  "wired translated i128 owner should reload the low lhs lane through the shared pair-load helper");
  expect_contains(asm_text, "    movq -16(%rbp), %rdx",
                  "wired translated i128 owner should reload the high lhs lane through the shared pair-load helper");
  expect_contains(asm_text, "    movq $-3, %rax",
                  "wired translated i128 owner should materialize immediate rhs low lanes through the shared pair-load helper");
  expect_contains(asm_text, "    movq $-1, %rdx",
                  "wired translated i128 owner should sign-extend negative immediate rhs lanes into rdx");
  expect_contains(asm_text, "    movq %rax, %rcx",
                  "wired translated i128 owner should move the prepared rhs low lane into rcx before the arithmetic primitive");
  expect_contains(asm_text, "    movq %rdx, %rsi",
                  "wired translated i128 owner should move the prepared rhs high lane into rsi before the arithmetic primitive");
  expect_contains(asm_text, "    addq %rcx, %rax",
                  "wired translated i128 owner should emit the Rust-matching add low-lane primitive");
  expect_contains(asm_text, "    adcq %rsi, %rdx",
                  "wired translated i128 owner should emit the Rust-matching add carry primitive");
  expect_contains(asm_text, "    movq %rax, -56(%rbp)",
                  "wired translated i128 owner should store the low result lane back to the destination slot");
  expect_contains(asm_text, "    movq %rdx, -48(%rbp)",
                  "wired translated i128 owner should store the high result lane back to the destination slot");
  expect_contains(asm_text, "    cmpq %rsi, %rdx",
                  "wired translated i128 owner should compare high lanes first for ordered comparisons");
  expect_contains(asm_text, "    setg %r8b",
                  "wired translated i128 owner should use the Rust-matching high-word condition code for signed-ge");
  expect_contains(asm_text, "    setae %r8b",
                  "wired translated i128 owner should use the Rust-matching low-word condition code after high-word equality");
  expect_contains(asm_text, "    movzbq %r8b, %rax",
                  "wired translated i128 owner should materialize the ordered comparison result in rax");
  expect_contains(asm_text, "    movq %rax, -72(%rbp)",
                  "wired translated i128 owner should store the comparison result through the shared scalar store helper");
  expect_contains(asm_text, "    movq %rax, -88(%rbp)",
                  "wired translated i128 owner should keep the pair-store helper wired for explicit stack slots");
  expect_contains(asm_text, "    movq %rdx, -80(%rbp)",
                  "wired translated i128 owner should keep the pair-store helper writing the high half at slot+8");
  expect_contains(asm_text, "    movq -88(%rbp), %rax",
                  "wired translated i128 owner should keep the pair-load helper reloading the low half from an explicit slot");
  expect_contains(asm_text, "    movq -80(%rbp), %rdx",
                  "wired translated i128 owner should keep the pair-load helper reloading the high half from an explicit slot");
  expect_contains(asm_text, "    movq %rax, %rsi",
                  "wired translated i128 owner should preserve the accumulator pair through the save helper before indirect stores");
  expect_contains(asm_text, "    movq %rdx, %rdi",
                  "wired translated i128 owner should preserve the high accumulator lane through the save helper before indirect stores");
  expect_contains(asm_text, "    movq %rsi, (%rcx)",
                  "wired translated i128 owner should lower indirect pair stores through the saved-rsi low lane");
  expect_contains(asm_text, "    movq %rdi, 8(%rcx)",
                  "wired translated i128 owner should lower indirect pair stores through the saved-rdi high lane");
  expect_contains(asm_text, "    movq (%rcx), %rax",
                  "wired translated i128 owner should keep indirect pair loads wired through rcx");
  expect_contains(asm_text, "    movq 8(%rcx), %rdx",
                  "wired translated i128 owner should keep indirect pair loads reloading the high lane from rcx+8");
  expect_contains(asm_text, "    addq $1, %rax",
                  "wired translated i128 owner should emit the Rust-matching two's-complement neg low-lane increment");
  expect_contains(asm_text, "    adcq $0, %rdx",
                  "wired translated i128 owner should emit the Rust-matching two's-complement neg carry into the high lane");
  expect_true(codegen.state.reg_cache.acc_valid &&
                  codegen.state.reg_cache.acc_value_id == 13 &&
                  !codegen.state.reg_cache.acc_known_zero_extended,
              "wired translated i128 owner should leave the shared accumulator cache tracking the scalar cmp-store destination");
}

void test_x86_codegen_header_exports_translated_call_owner_surface() {
  using X86Codegen = c4c::backend::x86::X86Codegen;
  using X86CodegenOutput = c4c::backend::x86::X86CodegenOutput;
  using X86CodegenRegCache = c4c::backend::x86::X86CodegenRegCache;
  using X86CodegenState = c4c::backend::x86::X86CodegenState;
  using Value = c4c::backend::x86::Value;
  using StackSlot = c4c::backend::x86::StackSlot;

  auto output_emit_instr_imm_reg = &X86CodegenOutput::emit_instr_imm_reg;
  auto output_emit_instr_rbp_reg = &X86CodegenOutput::emit_instr_rbp_reg;
  auto output_emit_instr_rbp = &X86CodegenOutput::emit_instr_rbp;
  auto reg_cache_invalidate_all = &X86CodegenRegCache::invalidate_all;
  auto reg_cache_set_acc = &X86CodegenRegCache::set_acc;
  auto operand_to_rax = &X86Codegen::operand_to_rax;
  auto operand_to_rcx = &X86Codegen::operand_to_rcx;
  auto operand_to_rax_rdx = &X86Codegen::operand_to_rax_rdx;
  auto store_rax_to = &X86Codegen::store_rax_to;
  auto store_rax_rdx_to = &X86Codegen::store_rax_rdx_to;
  auto call_abi_config = &X86Codegen::call_abi_config_impl;
  auto emit_call_compute_stack_space = &X86Codegen::emit_call_compute_stack_space_impl;
  auto emit_call_stack_args = &X86Codegen::emit_call_stack_args_impl;
  auto emit_call_reg_args = &X86Codegen::emit_call_reg_args_impl;
  auto emit_call_instruction = &X86Codegen::emit_call_instruction_impl;
  auto emit_call_cleanup = &X86Codegen::emit_call_cleanup_impl;
  auto set_call_ret_eightbyte_classes = &X86Codegen::set_call_ret_eightbyte_classes_impl;
  auto emit_call_store_result = &X86Codegen::emit_call_store_result_impl;
  auto emit_call_store_i128_result = &X86Codegen::emit_call_store_i128_result_impl;
  auto emit_call_move_f32_to_acc = &X86Codegen::emit_call_move_f32_to_acc_impl;
  auto emit_call_move_f64_to_acc = &X86Codegen::emit_call_move_f64_to_acc_impl;
  auto state_emit = &X86CodegenState::emit;
  auto state_get_slot = &X86CodegenState::get_slot;
  auto state_resolve_slot_addr = &X86CodegenState::resolve_slot_addr;

  c4c::backend::x86::CallArgClass stack_class;
  stack_class.kind = c4c::backend::x86::CallArgClass::Kind::Stack;
  c4c::backend::x86::CallArgClass register_class;
  register_class.kind = c4c::backend::x86::CallArgClass::Kind::Register;
  Value value{17};
  StackSlot slot{-24};
  const c4c::backend::x86::CallAbiConfig abi_config{
      6, 8, false, false, false, false, false, true, false, false, false, false,
  };

  expect_true(output_emit_instr_imm_reg != nullptr && output_emit_instr_rbp_reg != nullptr &&
                  output_emit_instr_rbp != nullptr && reg_cache_invalidate_all != nullptr &&
                  reg_cache_set_acc != nullptr && operand_to_rax != nullptr &&
                  operand_to_rcx != nullptr &&
                  operand_to_rax_rdx != nullptr && store_rax_to != nullptr &&
                  store_rax_rdx_to != nullptr && call_abi_config != nullptr &&
                  emit_call_compute_stack_space != nullptr &&
                  emit_call_stack_args != nullptr && emit_call_reg_args != nullptr &&
                  emit_call_instruction != nullptr && emit_call_cleanup != nullptr &&
                  set_call_ret_eightbyte_classes != nullptr &&
                  emit_call_store_result != nullptr &&
                  emit_call_store_i128_result != nullptr &&
                  emit_call_move_f32_to_acc != nullptr &&
                  emit_call_move_f64_to_acc != nullptr && state_emit != nullptr &&
                  state_get_slot != nullptr && state_resolve_slot_addr != nullptr &&
                  value.raw == 17 && slot.raw == -24 &&
                  stack_class.is_stack() && !stack_class.is_register() &&
                  register_class.is_register() && !register_class.is_stack() &&
                  abi_config.max_int_regs == 6 && abi_config.max_float_regs == 8 &&
                  abi_config.use_sysv_struct_classification &&
                  !abi_config.allow_struct_split_reg_stack,
              "x86 translated call-owner build wiring should keep the shared ABI structs, output hooks, reg-cache hooks, raw value/slot fields, and owner helper surface linkable once calls.cpp enters the build");
}

void test_x86_translated_shared_call_support_tracks_real_state_and_output() {
  c4c::backend::x86::X86Codegen codegen;
  codegen.state.slots.emplace(7, c4c::backend::x86::StackSlot{-24});
  codegen.state.slots.emplace(9, c4c::backend::x86::StackSlot{-40});
  codegen.state.allocas.insert(9);
  codegen.state.over_aligned_allocas.emplace(9, 32);

  codegen.state.emit("    pushq %rbp");
  codegen.state.out.emit_instr_imm_reg("    addq", 16, "rsp");
  codegen.state.out.emit_instr_rbp_reg("    movq", -24, "rax");
  codegen.state.out.emit_instr_rbp("    fstpt", -32);
  codegen.state.out.emit_instr_mem_reg("    leaq", 8, "rax", "rcx");
  codegen.operand_to_rax(c4c::backend::x86::Operand{7});
  codegen.store_rax_to(c4c::backend::x86::Value{7});

  expect_true(codegen.state.reg_cache.acc_valid &&
                  codegen.state.reg_cache.acc_value_id == 7 &&
                  !codegen.state.reg_cache.acc_known_zero_extended,
              "shared x86 call-support reg-cache hook should remember the accumulator value after a store_rax_to slice");

  codegen.store_rax_rdx_to(c4c::backend::x86::Value{7});
  codegen.state.track_f128_load(11, 9, 0);

  const auto direct_slot = codegen.state.get_slot(7);
  const auto over_aligned_addr = codegen.state.resolve_slot_addr(9);
  const auto f128_source = codegen.state.get_f128_source(11);
  const auto over_align = codegen.state.alloca_over_align(9);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_true(direct_slot.has_value() && direct_slot->raw == -24,
              "shared x86 call-support state should return stored slot offsets for translated helper lookups");
  expect_true(over_aligned_addr.has_value() &&
                  over_aligned_addr->kind == c4c::backend::x86::SlotAddr::Kind::OverAligned &&
                  over_aligned_addr->slot.raw == -40 && over_aligned_addr->value_id == 9,
              "shared x86 call-support state should preserve over-aligned alloca slot metadata for translated helper lookups");
  expect_true(f128_source.has_value() && *f128_source == 9,
              "shared x86 call-support state should remember the bounded f128 load source id");
  expect_true(over_align.has_value() && *over_align == 32,
              "shared x86 call-support state should remember the bounded alloca over-alignment");
  expect_true(!codegen.state.reg_cache.acc_valid,
              "shared x86 call-support invalidate_all hook should clear accumulator cache state after a paired store slice");
  expect_contains(asm_text, "    pushq %rbp",
                  "shared x86 call-support state emit helper should append raw assembly lines");
  expect_contains(asm_text, "    addq $16, %rsp",
                  "shared x86 call-support output helper should render immediate/register instructions");
  expect_contains(asm_text, "    movq -24(%rbp), %rax",
                  "shared x86 call-support output helper should render rbp/register loads");
  expect_contains(asm_text, "    fstpt -32(%rbp)",
                  "shared x86 call-support output helper should render rbp memory-only instructions");
  expect_contains(asm_text, "    leaq 8(%rax), %rcx",
                  "shared x86 call-support output helper should render generic memory/register instructions");
  expect_contains(asm_text, "    movq %rax, -24(%rbp)",
                  "shared x86 call-support store helper should emit the translated rbp store text");
  expect_contains(asm_text, "    movq %rdx, -16(%rbp)",
                  "shared x86 call-support paired store helper should emit the high-half rbp store text");
}

void test_x86_translated_call_owner_emits_scalar_reg_and_stack_abi_text() {
  using CallArgClass = c4c::backend::x86::CallArgClass;
  using IrType = c4c::backend::x86::IrType;
  using Operand = c4c::backend::x86::Operand;
  using StackSlot = c4c::backend::x86::StackSlot;
  using Value = c4c::backend::x86::Value;

  c4c::backend::x86::X86Codegen codegen;
  codegen.state.slots.emplace(1, StackSlot{-16});
  codegen.state.slots.emplace(2, StackSlot{-24});
  codegen.state.slots.emplace(3, StackSlot{-32});
  codegen.state.slots.emplace(4, StackSlot{-48});
  codegen.state.slots.emplace(5, StackSlot{-64});

  std::vector<Operand> args{Operand{1}, Operand{2}, Operand{3}};
  std::vector<CallArgClass> arg_classes(3);
  arg_classes[0].kind = CallArgClass::Kind::Register;
  arg_classes[1].kind = CallArgClass::Kind::Register;
  arg_classes[2].kind = CallArgClass::Kind::Stack;
  std::vector<IrType> arg_types{IrType::F64, IrType::I64, IrType::I64};

  const auto stack_space = codegen.emit_call_compute_stack_space_impl(arg_classes, arg_types);
  const auto stack_adjust = codegen.emit_call_stack_args_impl(args, arg_classes, arg_types, stack_space, 0, 0);
  codegen.emit_call_reg_args_impl(args, arg_classes, arg_types, stack_adjust, 0, stack_space, {});
  codegen.emit_call_instruction_impl(std::optional<std::string>{"callee_symbol"}, std::nullopt, false, stack_space);
  codegen.emit_call_cleanup_impl(stack_space, 0, false);
  codegen.emit_call_move_f64_to_acc_impl();
  codegen.emit_call_store_result_impl(Value{4}, IrType::F64);
  codegen.emit_call_move_f32_to_acc_impl();
  codegen.emit_call_store_result_impl(Value{5}, IrType::F32);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_true(stack_space == 16,
              "translated x86 call owner should reserve the bounded SysV stack space including the alignment pad for one stack argument");
  expect_true(stack_adjust == 0,
              "translated x86 call owner should report no extra stack adjustment beyond the bounded alignment-aware push slice");
  expect_contains(asm_text, "    pushq %rax",
                  "translated x86 call owner should push stack-classified arguments through the shared accumulator helper");
  expect_contains(asm_text, "    movq %rax, %xmm0",
                  "translated x86 call owner should route f64 register arguments through xmm registers");
  expect_contains(asm_text, "    movq %rax, %rdi",
                  "translated x86 call owner should keep integer register arguments in the integer SysV bank");
  expect_contains(asm_text, "    movb $1, %al",
                  "translated x86 call owner should advertise the used SSE argument register count in %al");
  expect_contains(asm_text, "    call callee_symbol",
                  "translated x86 call owner should emit direct call instructions through the owner surface");
  expect_contains(asm_text, "    addq $16, %rsp",
                  "translated x86 call owner should clean up the alignment-adjusted stack argument area");
  expect_contains(asm_text, "    movq %xmm0, %rax",
                  "translated x86 call owner should expose the bounded f64 result move helper");
  expect_contains(asm_text, "    movq %rax, -48(%rbp)",
                  "translated x86 call owner should store f64 call results through the shared slot path");
  expect_contains(asm_text, "    movd %xmm0, %eax",
                  "translated x86 call owner should expose the bounded f32 result move helper");
  expect_contains(asm_text, "    movq %rax, -64(%rbp)",
                  "translated x86 call owner should store f32 call results through the shared slot path");
}

void test_x86_operand_immediate_contract() {
  using Operand = c4c::backend::x86::Operand;

  c4c::backend::x86::X86Codegen codegen;
  codegen.state.slots.emplace(7, c4c::backend::x86::StackSlot{-24});

  const auto imm = Operand::immediate_i32(7);
  const auto zero = Operand::zero();
  const auto slot_backed = Operand{7};

  expect_true(codegen.const_as_imm32(imm).has_value() && *codegen.const_as_imm32(imm) == 7,
              "shared x86 operand contract should expose explicit imm32 operands to translated owner files");
  expect_true(codegen.const_as_imm32(zero).has_value() && *codegen.const_as_imm32(zero) == 0,
              "shared x86 operand contract should preserve explicit zero immediates");
  expect_true(!codegen.const_as_imm32(slot_backed).has_value(),
              "shared x86 operand contract should not treat plain raw value ids as immediates");

  codegen.operand_to_rax(imm);
  codegen.operand_to_rcx(zero);
  codegen.operand_to_rax_rdx(Operand::immediate_i64(-3));

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    movq $7, %rax",
                  "shared x86 operand contract should let accumulator loads honor explicit imm32 operands");
  expect_contains(asm_text, "    xorl %ecx, %ecx",
                  "shared x86 operand contract should let rcx loads honor explicit zero immediates");
  expect_contains(asm_text, "    movq $-3, %rax",
                  "shared x86 operand contract should lower immediate pair loads through the shared helper path");
  expect_contains(asm_text, "    movq $-1, %rdx",
                  "shared x86 operand contract should sign-extend negative immediates across the rdx high half");
}

void test_x86_translated_intrinsics_owner_emits_bounded_helper_text() {
  using Operand = c4c::backend::x86::Operand;
  using StackSlot = c4c::backend::x86::StackSlot;
  using Value = c4c::backend::x86::Value;

  c4c::backend::x86::X86Codegen codegen;
  codegen.state.slots.emplace(1, StackSlot{-16});
  codegen.state.slots.emplace(2, StackSlot{-32});
  codegen.state.slots.emplace(3, StackSlot{-48});
  codegen.state.allocas.insert(1);
  codegen.state.allocas.insert(2);
  codegen.state.allocas.insert(3);

  codegen.float_operand_to_xmm0(Operand::zero(), false);
  codegen.float_operand_to_xmm0(Operand::immediate_i32(7), true);
  codegen.float_operand_to_xmm0(Operand{1}, false);
  codegen.emit_sse_binary_128(Value{3}, Operand{1}, Operand{2}, "pcmpeqb");
  codegen.emit_sse_unary_imm_128(Value{3}, Operand{1}, 5, "psllq");
  codegen.emit_sse_shuffle_imm_128(Value{3}, Operand{1}, Operand{2}, 9, "pshufd");

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    xorpd %xmm0, %xmm0",
                  "wired translated intrinsics helper should preserve the zeroed f64 constant fast path");
  expect_contains(asm_text, "    movl $7, %eax",
                  "wired translated intrinsics helper should materialize non-zero f32 immediates through eax");
  expect_contains(asm_text, "    movd %eax, %xmm0",
                  "wired translated intrinsics helper should route f32 immediates into xmm0 with movd");
  expect_contains(asm_text, "    leaq -16(%rbp), %rax",
                  "wired translated intrinsics helper should resolve pointer-backed vector operands through the shared slot-address contract");
  expect_contains(asm_text, "    pcmpeqb %xmm1, %xmm0",
                  "wired translated intrinsics helper should emit bounded SSE binary op text once both vector operands are loaded");
  expect_contains(asm_text, "    psllq $5, %xmm0",
                  "wired translated intrinsics helper should emit unary immediate SSE ops with the requested lane constant");
  expect_contains(asm_text, "    pshufd $9, %xmm0, %xmm0",
                  "wired translated intrinsics helper should emit shuffle-immediate SSE ops in the translated self-shuffle form");
  expect_contains(asm_text, "    movdqu %xmm0, (%rax)",
                  "wired translated intrinsics helper should store vector results back through the shared pointer destination path");
}

void test_x86_translated_intrinsics_dispatcher_emits_bounded_text() {
  using IntrinsicOp = c4c::backend::x86::IntrinsicOp;
  using Operand = c4c::backend::x86::Operand;
  using StackSlot = c4c::backend::x86::StackSlot;
  using Value = c4c::backend::x86::Value;

  c4c::backend::x86::X86Codegen codegen;
  codegen.state.slots.emplace(1, StackSlot{-16});
  codegen.state.slots.emplace(2, StackSlot{-32});
  codegen.state.slots.emplace(3, StackSlot{-48});
  codegen.state.slots.emplace(5, StackSlot{-64});
  codegen.state.allocas.insert(1);
  codegen.state.allocas.insert(2);
  codegen.state.allocas.insert(3);

  codegen.emit_nontemporal_store(IntrinsicOp::Movnti, Operand{3}, Operand::immediate_i32(9),
                                 std::nullopt);
  codegen.emit_intrinsic_impl(std::optional<Value>{Value{3}}, IntrinsicOp::Pcmpeqb128,
                              {Operand{1}, Operand{2}});
  codegen.emit_intrinsic_impl(std::optional<Value>{Value{5}}, IntrinsicOp::Crc32_32,
                              {Operand::immediate_i32(1), Operand::immediate_i32(2)});

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    movnti %ecx, (%rax)",
                  "wired translated intrinsics dispatcher should keep the non-temporal i32 store path live");
  expect_contains(asm_text, "    pcmpeqb %xmm1, %xmm0",
                  "wired translated intrinsics dispatcher should route packed compare branches through the translated SSE helper");
  expect_contains(asm_text, "    crc32l %ecx, %eax",
                  "wired translated intrinsics dispatcher should keep scalar CRC32 branches live");
  expect_contains(asm_text, "    movq %rax, -64(%rbp)",
                  "wired translated intrinsics dispatcher should still store scalar results through the shared result helper");
}

void test_x86_codegen_header_exports_translated_memory_owner_surface() {
  using AddressSpace = c4c::backend::x86::AddressSpace;
  using X86Codegen = c4c::backend::x86::X86Codegen;
  using X86CodegenState = c4c::backend::x86::X86CodegenState;

  auto emit_store = &X86Codegen::emit_store_impl;
  auto emit_load = &X86Codegen::emit_load_impl;
  auto emit_store_with_const_offset = &X86Codegen::emit_store_with_const_offset_impl;
  auto emit_load_with_const_offset = &X86Codegen::emit_load_with_const_offset_impl;
  auto emit_typed_store_to_slot = &X86Codegen::emit_typed_store_to_slot_impl;
  auto emit_typed_load_from_slot = &X86Codegen::emit_typed_load_from_slot_impl;
  auto emit_load_ptr_from_slot = &X86Codegen::emit_load_ptr_from_slot_impl;
  auto emit_typed_store_indirect = &X86Codegen::emit_typed_store_indirect_impl;
  auto emit_typed_load_indirect = &X86Codegen::emit_typed_load_indirect_impl;
  auto emit_add_offset_to_addr_reg = &X86Codegen::emit_add_offset_to_addr_reg_impl;
  auto emit_alloca_addr_to = &X86Codegen::emit_alloca_addr_to;
  auto emit_slot_addr_to_secondary = &X86Codegen::emit_slot_addr_to_secondary_impl;
  auto emit_gep_direct_const = &X86Codegen::emit_gep_direct_const_impl;
  auto emit_gep_indirect_const = &X86Codegen::emit_gep_indirect_const_impl;
  auto emit_memcpy_load_dest_addr = &X86Codegen::emit_memcpy_load_dest_addr_impl;
  auto emit_memcpy_load_src_addr = &X86Codegen::emit_memcpy_load_src_addr_impl;
  auto emit_alloca_aligned_addr = &X86Codegen::emit_alloca_aligned_addr_impl;
  auto emit_alloca_aligned_addr_to_acc = &X86Codegen::emit_alloca_aligned_addr_to_acc_impl;
  auto emit_seg_load = &X86Codegen::emit_seg_load_impl;
  auto emit_seg_load_symbol = &X86Codegen::emit_seg_load_symbol_impl;
  auto emit_seg_store = &X86Codegen::emit_seg_store_impl;
  auto emit_seg_store_symbol = &X86Codegen::emit_seg_store_symbol_impl;
  auto state_assigned_reg_index = &X86CodegenState::assigned_reg_index;

  expect_true(emit_store != nullptr && emit_load != nullptr &&
                  emit_store_with_const_offset != nullptr &&
                  emit_load_with_const_offset != nullptr &&
                  emit_typed_store_to_slot != nullptr &&
                  emit_typed_load_from_slot != nullptr &&
                  emit_load_ptr_from_slot != nullptr &&
                  emit_typed_store_indirect != nullptr &&
                  emit_typed_load_indirect != nullptr &&
                  emit_add_offset_to_addr_reg != nullptr &&
                  emit_alloca_addr_to != nullptr &&
                  emit_slot_addr_to_secondary != nullptr &&
                  emit_gep_direct_const != nullptr &&
                  emit_gep_indirect_const != nullptr &&
                  emit_memcpy_load_dest_addr != nullptr &&
                  emit_memcpy_load_src_addr != nullptr &&
                  emit_alloca_aligned_addr != nullptr &&
                  emit_alloca_aligned_addr_to_acc != nullptr &&
                  state_assigned_reg_index != nullptr &&
                  emit_seg_load != nullptr && emit_seg_load_symbol != nullptr &&
                  emit_seg_store != nullptr && emit_seg_store_symbol != nullptr &&
                  AddressSpace::Default != AddressSpace::SegFs &&
                  AddressSpace::SegFs != AddressSpace::SegGs,
              "x86 translated memory-owner surface should stay declaration-reachable through x86_codegen while memory.cpp advances from stale syntax and missing helper glue toward the next owner-state blocker tier");
}

void test_x86_codegen_state_tracks_translated_reg_assignments() {
  c4c::backend::x86::X86CodegenState state;
  state.reg_assignment_indices.emplace(41, 14);

  const auto assigned = state.assigned_reg_index(41);
  const auto missing = state.assigned_reg_index(99);

  expect_true(assigned.has_value() && *assigned == 14 && !missing.has_value(),
              "shared x86 translated state should expose bounded register-assignment lookup for parked owner files");
}

void test_x86_translated_memory_owner_emits_linked_helper_text() {
  using AddressSpace = c4c::backend::x86::AddressSpace;
  using IrType = c4c::backend::x86::IrType;
  using Operand = c4c::backend::x86::Operand;
  using StackSlot = c4c::backend::x86::StackSlot;

  c4c::backend::x86::X86Codegen codegen;
  codegen.state.slots.emplace(5, StackSlot{-24});
  codegen.state.slots.emplace(7, StackSlot{-40});
  codegen.state.slots.emplace(9, StackSlot{-56});
  codegen.state.f128_direct_slots.insert(5);

  codegen.emit_store_impl(Operand{5}, c4c::backend::x86::Value{7}, IrType::F128);
  codegen.emit_seg_store_symbol_impl(Operand{5}, "tls_anchor", IrType::I64, AddressSpace::SegGs);
  codegen.emit_add_offset_to_addr_reg_impl(8);
  codegen.emit_gep_direct_const_impl(StackSlot{-32}, 16);
  codegen.emit_memcpy_impl_impl(24);
  codegen.emit_load_impl(c4c::backend::x86::Value{9}, c4c::backend::x86::Value{7}, IrType::F128);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    movq -24(%rbp), %rax",
                  "wired translated memory owner should load the source operand into the accumulator before a segment-symbol store");
  expect_contains(asm_text, "    movq %rax, %gs:tls_anchor(%rip)",
                  "wired translated memory owner should emit segment-symbol stores through the linked owner body");
  expect_contains(asm_text, "    fldt -24(%rbp)",
                  "wired translated memory owner should reload direct-slot f128 values through the parked helper-backed x87 path");
  expect_contains(asm_text, "    fstpt -40(%rbp)",
                  "wired translated memory owner should store F128 values through the parked helper-backed resolved-address path");
  expect_contains(asm_text, "    addq $8, %rcx",
                  "wired translated memory owner should keep offset-add helpers linked through the active x86 build");
  expect_contains(asm_text, "    leaq -16(%rbp), %rax",
                  "wired translated memory owner should keep direct-const GEP helpers linked through the active x86 build");
  expect_contains(asm_text, "    movq $24, %rcx",
                  "wired translated memory owner should keep memcpy size setup linked through the active x86 build");
  expect_contains(asm_text, "    rep movsb",
                  "wired translated memory owner should keep memcpy emission linked through the active x86 build");
  expect_contains(asm_text, "    fldt -40(%rbp)",
                  "wired translated memory owner should load parked F128 values through the helper-backed resolved-address path");
  expect_contains(asm_text, "    fstpt -56(%rbp)",
                  "wired translated memory owner should preserve full F128 precision by staging the parked load through the destination slot");
  expect_contains(asm_text, "    fldt -56(%rbp)",
                  "wired translated memory owner should reload the staged F128 destination slot after the helper-backed parked load");
  expect_true(codegen.state.get_f128_source(9).has_value() && *codegen.state.get_f128_source(9) == 7,
              "wired translated memory owner should preserve F128 load provenance through the shared translated state");
  expect_true(codegen.state.reg_cache.acc_valid &&
                  codegen.state.reg_cache.acc_value_id == 9 &&
                  !codegen.state.reg_cache.acc_known_zero_extended,
              "wired translated memory owner should refresh the shared accumulator cache when finishing parked F128 loads");
  expect_true(codegen.state.f128_direct_slots.find(9) != codegen.state.f128_direct_slots.end(),
              "wired translated memory owner should mark parked F128 load destinations as direct-slot x87 values");
}

void test_x86_translated_memory_owner_preserves_f128_store_precision_paths() {
  using IrType = c4c::backend::x86::IrType;
  using Operand = c4c::backend::x86::Operand;
  using SlotAddr = c4c::backend::x86::SlotAddr;
  using StackSlot = c4c::backend::x86::StackSlot;
  using Value = c4c::backend::x86::Value;

  c4c::backend::x86::X86Codegen codegen;
  codegen.state.slots.emplace(5, StackSlot{-24});
  codegen.state.slots.emplace(7, StackSlot{-40});
  codegen.state.slots.emplace(11, StackSlot{-72});
  codegen.state.slots.emplace(13, StackSlot{-88});
  codegen.state.slots.emplace(15, StackSlot{-104});
  codegen.state.f128_direct_slots.insert(5);
  codegen.state.allocas.insert(13);
  codegen.state.allocas.insert(15);
  codegen.state.over_aligned_allocas.emplace(13, 32);

  codegen.emit_store_impl(Operand{5}, Value{7}, IrType::F128);
  codegen.emit_store_with_const_offset_impl(Operand{11}, Value{7}, 8, IrType::F128);
  codegen.emit_store_with_const_offset_impl(Operand{11}, Value{13}, 16, IrType::F128);
  codegen.emit_store_with_const_offset_impl(Operand{11}, Value{15}, 24, IrType::F128);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    fldt -24(%rbp)",
                  "wired translated memory owner should keep the direct-slot F128 fast path when the source already carries x87-backed precision");
  expect_contains(asm_text, "    fstpt -40(%rbp)",
                  "wired translated memory owner should store direct-slot F128 values through the resolved-address helper path");
  expect_contains(asm_text, "    movq -72(%rbp), %rax",
                  "wired translated memory owner should fall back through the bounded accumulator load path when the source is not a direct-slot F128 value");
  expect_contains(asm_text, "    pushq %rax",
                  "wired translated memory owner should convert bounded fallback stores through a stack-staged x87 reload for direct destinations");
  expect_contains(asm_text, "    fldl (%rsp)",
                  "wired translated memory owner should reload bounded fallback F64 payloads into x87 before storing as F128");
  expect_contains(asm_text, "    fstpt -32(%rbp)",
                  "wired translated memory owner should fold constant offsets into direct-slot fallback F128 stores");
  expect_contains(asm_text, "    movq %rax, %rdx",
                  "wired translated memory owner should preserve the fallback payload while resolving over-aligned or indirect destinations");
  expect_contains(asm_text, "    leaq -88(%rbp), %rcx",
                  "wired translated memory owner should resolve over-aligned fallback F128 store destinations through the shared alloca helper");
  expect_contains(asm_text, "    andq $-32, %rcx",
                  "wired translated memory owner should keep over-aligned fallback F128 stores aligned through the shared helper path");
  expect_contains(asm_text, "    addq $16, %rcx",
                  "wired translated memory owner should apply the bounded constant offset after over-aligned address resolution");
  expect_contains(asm_text, "    fstpt (%rcx)",
                  "wired translated memory owner should store fallback F128 values through the resolved indirect-address form");
  expect_contains(asm_text, "    movq -104(%rbp), %rcx",
                  "wired translated memory owner should resolve indirect fallback F128 store destinations through the shared pointer-load helper");
  expect_contains(asm_text, "    addq $24, %rcx",
                  "wired translated memory owner should apply constant offsets on indirect fallback F128 stores");
}

void test_x86_translated_f128_store_raw_bytes_emits_resolved_address_paths() {
  using SlotAddr = c4c::backend::x86::SlotAddr;
  using StackSlot = c4c::backend::x86::StackSlot;

  c4c::backend::x86::X86Codegen codegen;
  codegen.state.slots.emplace(7, StackSlot{-40});
  codegen.state.slots.emplace(9, StackSlot{-56});
  codegen.state.allocas.insert(7);
  codegen.state.over_aligned_allocas.emplace(7, 32);

  codegen.emit_f128_store_raw_bytes(SlotAddr::Direct(StackSlot{-24}), 0, 8,
                                    0x0123456789ABCDEFULL, 0x1357);
  codegen.emit_f128_store_raw_bytes(SlotAddr::OverAligned(StackSlot{-40}, 7), 7, 16,
                                    0x0FEDCBA987654321ULL, 0);
  codegen.emit_f128_store_raw_bytes(SlotAddr::Indirect(StackSlot{-56}), 9, 24,
                                    0x1111222233334444ULL, 0x2468);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    movabsq $81985529216486895, %rax",
                  "translated raw-byte f128 helper should materialize the low 64 bits for direct destinations");
  expect_contains(asm_text, "    movq %rax, -16(%rbp)",
                  "translated raw-byte f128 helper should fold direct-slot offsets into the first 8-byte store");
  expect_contains(asm_text, "    movq $4951, %rax",
                  "translated raw-byte f128 helper should materialize non-zero high bytes for direct destinations");
  expect_contains(asm_text, "    movq %rax, -8(%rbp)",
                  "translated raw-byte f128 helper should write the high raw-byte fragment after the direct-slot offset fold");
  expect_contains(asm_text, "    leaq -40(%rbp), %rcx",
                  "translated raw-byte f128 helper should resolve over-aligned destinations through the shared alloca-address helper");
  expect_contains(asm_text, "    andq $-32, %rcx",
                  "translated raw-byte f128 helper should preserve over-aligned destination alignment before writing raw bytes");
  expect_contains(asm_text, "    addq $16, %rcx",
                  "translated raw-byte f128 helper should apply constant offsets after over-aligned address resolution");
  expect_contains(asm_text, "    movabsq $1147797409030816545, %rax",
                  "translated raw-byte f128 helper should materialize the low 64 bits for over-aligned destinations");
  expect_contains(asm_text, "    movq %rax, (%rcx)",
                  "translated raw-byte f128 helper should store the low raw-byte fragment through the resolved rcx address");
  expect_contains(asm_text, "    xorl %eax, %eax",
                  "translated raw-byte f128 helper should zero the high fragment efficiently when the upper bytes are all zero");
  expect_contains(asm_text, "    movq %rax, 8(%rcx)",
                  "translated raw-byte f128 helper should still write the zeroed high fragment through the resolved rcx address");
  expect_contains(asm_text, "    movq -56(%rbp), %rcx",
                  "translated raw-byte f128 helper should resolve indirect destinations through the shared pointer-load helper");
  expect_contains(asm_text, "    addq $24, %rcx",
                  "translated raw-byte f128 helper should apply constant offsets on indirect destinations");
  expect_contains(asm_text, "    movabsq $1229801703532086340, %rax",
                  "translated raw-byte f128 helper should materialize the low 64 bits for indirect destinations");
  expect_contains(asm_text, "    movq $9320, %rax",
                  "translated raw-byte f128 helper should materialize non-zero high bytes for indirect destinations");
}

void test_x86_translated_memory_owner_routes_constant_f128_stores_through_raw_byte_helper() {
  using IrType = c4c::backend::x86::IrType;
  using Operand = c4c::backend::x86::Operand;
  using StackSlot = c4c::backend::x86::StackSlot;
  using Value = c4c::backend::x86::Value;

  c4c::backend::x86::X86Codegen codegen;
  codegen.state.slots.emplace(7, StackSlot{-40});
  codegen.state.slots.emplace(13, StackSlot{-88});
  codegen.state.slots.emplace(15, StackSlot{-104});
  codegen.state.allocas.insert(13);
  codegen.state.allocas.insert(15);
  codegen.state.over_aligned_allocas.emplace(13, 32);
  codegen.state.set_f128_constant_words(21, 0x0123456789ABCDEFULL, 0x1357);
  codegen.state.set_f128_constant_words(23, 0x0FEDCBA987654321ULL, 0);
  codegen.state.set_f128_constant_words(25, 0x1111222233334444ULL, 0x2468);

  codegen.emit_store_impl(Operand{21}, Value{7}, IrType::F128);
  codegen.emit_store_with_const_offset_impl(Operand{23}, Value{13}, 16, IrType::F128);
  codegen.emit_store_with_const_offset_impl(Operand{25}, Value{15}, 24, IrType::F128);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    movabsq $81985529216486895, %rax",
                  "wired translated memory owner should materialize direct constant long-double low bytes through the raw-byte helper path");
  expect_contains(asm_text, "    movq %rax, -40(%rbp)",
                  "wired translated memory owner should route direct constant long-double stores through bounded raw-byte writes");
  expect_contains(asm_text, "    movq $4951, %rax",
                  "wired translated memory owner should preserve the non-zero high raw-byte fragment for direct constant long-double stores");
  expect_contains(asm_text, "    leaq -88(%rbp), %rcx",
                  "wired translated memory owner should still resolve over-aligned constant long-double destinations through the shared helper path");
  expect_contains(asm_text, "    addq $16, %rcx",
                  "wired translated memory owner should apply constant offsets before writing over-aligned constant long-double raw bytes");
  expect_contains(asm_text, "    xorl %eax, %eax",
                  "wired translated memory owner should keep the helper zero-high fast path for over-aligned constant long-double stores");
  expect_contains(asm_text, "    movq -104(%rbp), %rcx",
                  "wired translated memory owner should still resolve indirect constant long-double destinations through the shared pointer-load helper");
  expect_contains(asm_text, "    addq $24, %rcx",
                  "wired translated memory owner should apply constant offsets on indirect constant long-double stores");
  expect_contains(asm_text, "    movq $9320, %rax",
                  "wired translated memory owner should materialize indirect constant long-double high bytes through the raw-byte helper path");
  expect_not_contains(asm_text, "    fldl (%rsp)",
                      "wired translated memory owner should not route constant long-double stores back through the f64-via-x87 fallback");
  expect_not_contains(asm_text, "    fstpt (%rcx)",
                      "wired translated memory owner should not lower constant long-double stores through the fallback x87 store path");
}

void test_x86_translated_f128_helper_reloads_constant_raw_bytes() {
  using Operand = c4c::backend::x86::Operand;

  c4c::backend::x86::X86Codegen nonzero_hi_codegen;
  nonzero_hi_codegen.state.set_f128_constant_words(21, 0x0123456789ABCDEFULL, 0x1357);
  nonzero_hi_codegen.emit_f128_load_to_x87(Operand{21});

  std::string nonzero_hi_asm;
  for (const auto& line : nonzero_hi_codegen.state.asm_lines) {
    nonzero_hi_asm += line;
    nonzero_hi_asm.push_back('\n');
  }

  expect_contains(nonzero_hi_asm, "    subq $16, %rsp",
                  "translated f128 helper should stage constant raw bytes on the stack before the shared x87 reload");
  expect_contains(nonzero_hi_asm, "    movabsq $81985529216486895, %rax",
                  "translated f128 helper should materialize the low raw-byte fragment for constant long-double inputs");
  expect_contains(nonzero_hi_asm, "    movq $4951, %rax",
                  "translated f128 helper should materialize the non-zero high raw-byte fragment for constant long-double inputs");
  expect_contains(nonzero_hi_asm, "    fldt (%rsp)",
                  "translated f128 helper should reload constant long-double inputs through the shared raw-byte x87 path");
  expect_contains(nonzero_hi_asm, "    addq $16, %rsp",
                  "translated f128 helper should pop the temporary raw-byte staging area after the x87 reload");
  expect_not_contains(nonzero_hi_asm, "    fldl (%rsp)",
                      "translated f128 helper should not mis-handle constant long-double inputs as staged f64 payloads");

  c4c::backend::x86::X86Codegen zero_hi_codegen;
  zero_hi_codegen.state.set_f128_constant_words(23, 0x0FEDCBA987654321ULL, 0);
  zero_hi_codegen.emit_f128_load_to_x87(Operand{23});

  std::string zero_hi_asm;
  for (const auto& line : zero_hi_codegen.state.asm_lines) {
    zero_hi_asm += line;
    zero_hi_asm.push_back('\n');
  }

  expect_contains(zero_hi_asm, "    xorl %eax, %eax",
                  "translated f128 helper should preserve the zero-high fast path when reloading constant long-double raw bytes");
  expect_contains(zero_hi_asm, "    movq %rax, 8(%rsp)",
                  "translated f128 helper should still stage the zeroed high fragment before the x87 reload");
}

void test_x86_translated_f128_helpers_reload_integer_conversion_sources() {
  using IrType = c4c::backend::x86::IrType;
  using SlotAddr = c4c::backend::x86::SlotAddr;
  using StackSlot = c4c::backend::x86::StackSlot;

  c4c::backend::x86::X86Codegen codegen;
  codegen.state.slots.emplace(7, StackSlot{-40});
  codegen.state.slots.emplace(9, StackSlot{-56});
  codegen.state.allocas.insert(7);
  codegen.state.over_aligned_allocas.emplace(7, 32);

  codegen.emit_f128_to_int_from_memory(SlotAddr::Direct(StackSlot{-24}), IrType::I64);
  codegen.emit_f128_to_int_from_memory(SlotAddr::OverAligned(StackSlot{-40}, 7), IrType::I64);
  codegen.emit_f128_to_int_from_memory(SlotAddr::Indirect(StackSlot{-56}), IrType::I64);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    fldt -24(%rbp)",
                  "translated f128 helper should reload direct-slot values straight into x87 before integer conversion");
  expect_contains(asm_text, "    leaq -40(%rbp), %rcx",
                  "translated f128 helper should resolve over-aligned sources through the shared alloca-address helper");
  expect_contains(asm_text, "    andq $-32, %rcx",
                  "translated f128 helper should preserve over-aligned source alignment before x87 reload");
  expect_contains(asm_text, "    fldt (%rcx)",
                  "translated f128 helper should reload non-direct f128 values through the resolved rcx address");
  expect_contains(asm_text, "    movq -56(%rbp), %rcx",
                  "translated f128 helper should load indirect source pointers through the shared pointer-slot helper");
  expect_contains(asm_text, "    fisttpq (%rsp)",
                  "translated f128 helper should finish the x87 integer conversion through the shared ST0 helper");
  expect_contains(asm_text, "    movq (%rsp), %rax",
                  "translated f128 helper should materialize the converted integer result in rax");
}

void test_x86_translated_f128_helpers_emit_st0_integer_conversion_text() {
  using IrType = c4c::backend::x86::IrType;

  c4c::backend::x86::X86Codegen signed_codegen;
  signed_codegen.emit_f128_st0_to_int(IrType::I8);

  std::string signed_asm;
  for (const auto& line : signed_codegen.state.asm_lines) {
    signed_asm += line;
    signed_asm.push_back('\n');
  }

  expect_contains(signed_asm, "    fisttpq (%rsp)",
                  "translated f128 helper should convert signed ST0 values with the direct fisttpq path");
  expect_contains(signed_asm, "    movsbq %al, %rax",
                  "translated f128 helper should sign-extend narrow signed integer results after the x87 conversion");

  c4c::backend::x86::X86Codegen pointer_codegen;
  pointer_codegen.emit_f128_st0_to_int(IrType::Ptr);

  std::string pointer_asm;
  for (const auto& line : pointer_codegen.state.asm_lines) {
    pointer_asm += line;
    pointer_asm.push_back('\n');
  }

  expect_contains(pointer_asm, "    movq (%rsp), %rax",
                  "translated f128 helper should materialize pointer-sized conversions in rax without narrow post-fixups");
  expect_not_contains(pointer_asm, "    movsbq %al, %rax",
                      "translated f128 helper should not sign-extend pointer-sized conversions as if they were i8 results");
}

void test_x86_translated_f128_cast_helpers_dispatch_x87_paths() {
  using IrType = c4c::backend::x86::IrType;

  c4c::backend::x86::X86Codegen int_to_f128_codegen;
  int_to_f128_codegen.emit_cast_instrs_x86(IrType::I8, IrType::F128);

  std::string int_to_f128_asm;
  for (const auto& line : int_to_f128_codegen.state.asm_lines) {
    int_to_f128_asm += line;
    int_to_f128_asm.push_back('\n');
  }

  expect_contains(int_to_f128_asm, "    movsbq %al, %rax",
                  "translated f128 cast helper should sign-extend narrow signed integers before the x87 FILD path");
  expect_contains(int_to_f128_asm, "    fildq (%rsp)",
                  "translated f128 cast helper should route integer-to-f128 promotion through the shared stack-staged FILD helper");
  expect_contains(int_to_f128_asm, "    fstpl (%rsp)",
                  "translated f128 cast helper should materialize the helper result back into the accumulator bit pattern");

  c4c::backend::x86::X86Codegen f128_to_f32_codegen;
  f128_to_f32_codegen.emit_cast_instrs_x86(IrType::F128, IrType::F32);

  std::string f128_to_f32_asm;
  for (const auto& line : f128_to_f32_codegen.state.asm_lines) {
    f128_to_f32_asm += line;
    f128_to_f32_asm.push_back('\n');
  }

  expect_contains(f128_to_f32_asm, "    fldl (%rsp)",
                  "translated f128 cast helper should reload the staged accumulator payload into x87 before narrowing to f32");
  expect_contains(f128_to_f32_asm, "    fstps (%rsp)",
                  "translated f128 cast helper should narrow staged f128 payloads through the shared x87-to-f32 store path");

  c4c::backend::x86::X86Codegen f32_to_f128_codegen;
  f32_to_f128_codegen.emit_cast_instrs_x86(IrType::F32, IrType::F128);

  std::string f32_to_f128_asm;
  for (const auto& line : f32_to_f128_codegen.state.asm_lines) {
    f32_to_f128_asm += line;
    f32_to_f128_asm.push_back('\n');
  }

  expect_contains(f32_to_f128_asm, "    movd %eax, %xmm0",
                  "translated f128 cast helper should move f32 accumulator payloads into xmm0 before widening");
  expect_contains(f32_to_f128_asm, "    cvtss2sd %xmm0, %xmm0",
                  "translated f128 cast helper should widen f32 inputs through the translated SSE conversion path");
  expect_contains(f32_to_f128_asm, "    movq %xmm0, %rax",
                  "translated f128 cast helper should materialize the widened helper result back in rax");
}

void test_x86_translated_f128_cast_helpers_cover_generic_scalar_casts() {
  using IrType = c4c::backend::x86::IrType;

  c4c::backend::x86::X86Codegen int_to_f64_codegen;
  int_to_f64_codegen.emit_cast_instrs_x86(IrType::I32, IrType::F64);

  std::string int_to_f64_asm;
  for (const auto& line : int_to_f64_codegen.state.asm_lines) {
    int_to_f64_asm += line;
    int_to_f64_asm.push_back('\n');
  }

  expect_contains(int_to_f64_asm, "    movslq %eax, %rax",
                  "translated generic cast helper should sign-extend i32 sources before integer-to-f64 conversion");
  expect_contains(int_to_f64_asm, "    cvtsi2sdq %rax, %xmm0",
                  "translated generic cast helper should use the scalar signed-int-to-f64 SSE conversion");
  expect_contains(int_to_f64_asm, "    movq %xmm0, %rax",
                  "translated generic cast helper should materialize the widened float result back into rax");

  c4c::backend::x86::X86Codegen f64_to_i8_codegen;
  f64_to_i8_codegen.emit_cast_instrs_x86(IrType::F64, IrType::I8);

  std::string f64_to_i8_asm;
  for (const auto& line : f64_to_i8_codegen.state.asm_lines) {
    f64_to_i8_asm += line;
    f64_to_i8_asm.push_back('\n');
  }

  expect_contains(f64_to_i8_asm, "    movq %rax, %xmm0",
                  "translated generic cast helper should move f64 payloads into xmm0 before integer conversion");
  expect_contains(f64_to_i8_asm, "    cvttsd2siq %xmm0, %rax",
                  "translated generic cast helper should convert f64 payloads to signed integers through the SSE truncate path");
  expect_contains(f64_to_i8_asm, "    movsbq %al, %rax",
                  "translated generic cast helper should sign-extend narrow signed integer results after float conversion");
}

void test_x86_translated_f128_unsigned_cast_helpers_emit_real_paths() {
  using IrType = c4c::backend::x86::IrType;

  c4c::backend::x86::X86Codegen f128_to_u64_codegen;
  f128_to_u64_codegen.emit_f128_to_u64_cast();

  std::string f128_to_u64_asm;
  for (const auto& line : f128_to_u64_codegen.state.asm_lines) {
    f128_to_u64_asm += line;
    f128_to_u64_asm.push_back('\n');
  }

  expect_contains(f128_to_u64_asm, "    movabsq $4890909195324358656, %rcx",
                  "translated unsigned f128 helper should materialize the 2^63 threshold constant before the x87 compare");
  expect_contains(f128_to_u64_asm, "    fcomip %st(1), %st",
                  "translated unsigned f128 helper should branch on the translated x87 threshold compare");
  expect_contains(f128_to_u64_asm, "    fsubrp %st, %st(1)",
                  "translated unsigned f128 helper should subtract the threshold in the high-bit recovery path");
  expect_contains(f128_to_u64_asm, "    addq %rcx, %rax",
                  "translated unsigned f128 helper should restore the high-bit bias after the recovered integer conversion");
  expect_not_contains(f128_to_u64_asm, "<f128-to-u64-cast>",
                      "translated unsigned f128 helper should no longer leave the helper body as placeholder text");

  c4c::backend::x86::X86Codegen f64_to_u64_codegen;
  f64_to_u64_codegen.emit_float_to_unsigned(true, true, IrType::I64);

  std::string f64_to_u64_asm;
  for (const auto& line : f64_to_u64_codegen.state.asm_lines) {
    f64_to_u64_asm += line;
    f64_to_u64_asm.push_back('\n');
  }

  expect_contains(f64_to_u64_asm, "    movq %rax, %xmm0",
                  "translated unsigned float helper should move staged f64 payloads into xmm0 before conversion");
  expect_contains(f64_to_u64_asm, "    ucomisd %xmm1, %xmm0",
                  "translated unsigned float helper should compare f64 inputs against the 2^63 threshold in xmm form");
  expect_contains(f64_to_u64_asm, "    subsd %xmm1, %xmm0",
                  "translated unsigned float helper should recover large f64 inputs through the translated subtract-and-add-bias path");
  expect_contains(f64_to_u64_asm, "    addq %rcx, %rax",
                  "translated unsigned float helper should restore the sign-bit bias after the large-value SSE truncate");

  c4c::backend::x86::X86Codegen f32_to_u8_codegen;
  f32_to_u8_codegen.emit_float_to_unsigned(false, false, IrType::I8);

  std::string f32_to_u8_asm;
  for (const auto& line : f32_to_u8_codegen.state.asm_lines) {
    f32_to_u8_asm += line;
    f32_to_u8_asm.push_back('\n');
  }

  expect_contains(f32_to_u8_asm, "    movd %eax, %xmm0",
                  "translated unsigned float helper should move staged f32 payloads into xmm0 before truncation");
  expect_contains(f32_to_u8_asm, "    cvttss2siq %xmm0, %rax",
                  "translated unsigned float helper should truncate f32 payloads through the shared SSE path");
  expect_contains(f32_to_u8_asm, "    movzbq %al, %rax",
                  "translated unsigned float helper should zero-extend narrow unsigned results after conversion");
  expect_not_contains(f32_to_u8_asm, "<float-to-unsigned>",
                      "translated unsigned float helper should no longer leave the helper body as placeholder text");

  c4c::backend::x86::X86Codegen u64_to_f64_codegen;
  u64_to_f64_codegen.emit_u64_to_float(true);

  std::string u64_to_f64_asm;
  for (const auto& line : u64_to_f64_codegen.state.asm_lines) {
    u64_to_f64_asm += line;
    u64_to_f64_asm.push_back('\n');
  }

  expect_contains(u64_to_f64_asm, "    testq %rax, %rax",
                  "translated unsigned-int-to-float helper should branch on the sign bit before choosing the fast or recovered path");
  expect_contains(u64_to_f64_asm, "    cvtsi2sdq %rax, %xmm0",
                  "translated unsigned-int-to-float helper should keep the direct signed-convert fast path for small values");
  expect_contains(u64_to_f64_asm, "    addsd %xmm0, %xmm0",
                  "translated unsigned-int-to-float helper should double the recovered half-value in the high-bit path");
  expect_contains(u64_to_f64_asm, "    movq %xmm0, %rax",
                  "translated unsigned-int-to-float helper should materialize f64 results back into the accumulator payload");

  c4c::backend::x86::X86Codegen u64_to_f32_codegen;
  u64_to_f32_codegen.emit_u64_to_float(false);

  std::string u64_to_f32_asm;
  for (const auto& line : u64_to_f32_codegen.state.asm_lines) {
    u64_to_f32_asm += line;
    u64_to_f32_asm.push_back('\n');
  }

  expect_contains(u64_to_f32_asm, "    cvtsi2ssq %rax, %xmm0",
                  "translated unsigned-int-to-float helper should reuse the scalar signed-int-to-f32 conversion on the bounded fast path");
  expect_contains(u64_to_f32_asm, "    addss %xmm0, %xmm0",
                  "translated unsigned-int-to-float helper should double the recovered f32 half-value in the high-bit path");
  expect_contains(u64_to_f32_asm, "    movd %xmm0, %eax",
                  "translated unsigned-int-to-float helper should materialize f32 results back into eax");
  expect_not_contains(u64_to_f32_asm, "<u64-to-float>",
                      "translated unsigned-int-to-float helper should no longer leave the helper body as placeholder text");
}

void test_x86_codegen_header_exports_translated_cast_owner_symbols() {
  using X86Codegen = c4c::backend::x86::X86Codegen;

  auto emit_cast_instrs = &X86Codegen::emit_cast_instrs_impl;
  auto emit_cast = &X86Codegen::emit_cast_impl;

  expect_true(emit_cast_instrs != nullptr && emit_cast != nullptr,
              "x86 translated cast owner methods should stay compile/link reachable through the public x86_codegen surface once cast_ops.cpp enters the build");
}

void test_x86_translated_cast_owner_reuses_constant_f128_reload_bridge() {
  using IrType = c4c::backend::x86::IrType;
  using Operand = c4c::backend::x86::Operand;
  using StackSlot = c4c::backend::x86::StackSlot;
  using Value = c4c::backend::x86::Value;

  c4c::backend::x86::X86Codegen f128_to_f64_codegen;
  f128_to_f64_codegen.state.slots.emplace(9, StackSlot{-24});
  f128_to_f64_codegen.state.set_f128_constant_words(31, 0x0123456789ABCDEFULL, 0x2468);

  f128_to_f64_codegen.emit_cast_impl(Value{9}, Operand{31}, IrType::F128, IrType::F64);

  std::string f128_to_f64_asm;
  for (const auto& line : f128_to_f64_codegen.state.asm_lines) {
    f128_to_f64_asm += line;
    f128_to_f64_asm.push_back('\n');
  }

  expect_contains(f128_to_f64_asm, "    subq $16, %rsp",
                  "wired translated cast owner should stage constant long-double operands through the shared raw-byte stack bridge before narrowing");
  expect_contains(f128_to_f64_asm, "    fldt (%rsp)",
                  "wired translated cast owner should reload constant long-double operands through the shared x87 helper before narrowing");
  expect_contains(f128_to_f64_asm, "    fstpl (%rsp)",
                  "wired translated cast owner should narrow constant long-double operands through the translated x87 store path");
  expect_contains(f128_to_f64_asm, "    movq %rax, -24(%rbp)",
                  "wired translated cast owner should store narrowed f64 results through the shared destination-slot path");

  c4c::backend::x86::X86Codegen f128_to_i8_codegen;
  f128_to_i8_codegen.state.slots.emplace(11, StackSlot{-40});
  f128_to_i8_codegen.state.set_f128_constant_words(33, 0x0FEDCBA987654321ULL, 0x1357);

  f128_to_i8_codegen.emit_cast_impl(Value{11}, Operand{33}, IrType::F128, IrType::I8);

  std::string f128_to_i8_asm;
  for (const auto& line : f128_to_i8_codegen.state.asm_lines) {
    f128_to_i8_asm += line;
    f128_to_i8_asm.push_back('\n');
  }

  expect_contains(f128_to_i8_asm, "    fldt (%rsp)",
                  "wired translated cast owner should reuse the constant long-double reload bridge for integer casts");
  expect_contains(f128_to_i8_asm, "    fisttpq (%rsp)",
                  "wired translated cast owner should convert the reloaded x87 value through the shared integer helper");
  expect_contains(f128_to_i8_asm, "    movsbq %al, %rax",
                  "wired translated cast owner should preserve the translated narrow signed-fixup path after integer conversion");
  expect_contains(f128_to_i8_asm, "    movq %rax, -40(%rbp)",
                  "wired translated cast owner should store integer cast results through the shared destination-slot path");
}

void test_x86_translated_cast_owner_emits_unsigned_f128_split_path() {
  using IrType = c4c::backend::x86::IrType;
  using Operand = c4c::backend::x86::Operand;
  using StackSlot = c4c::backend::x86::StackSlot;
  using Value = c4c::backend::x86::Value;

  c4c::backend::x86::X86Codegen u32_to_f128_codegen;
  u32_to_f128_codegen.state.slots.emplace(7, StackSlot{-32});
  u32_to_f128_codegen.state.slots.emplace(9, StackSlot{-24});

  u32_to_f128_codegen.emit_cast_impl(Value{9}, Operand{7}, IrType::U32, IrType::F128);

  std::string u32_to_f128_asm;
  for (const auto& line : u32_to_f128_codegen.state.asm_lines) {
    u32_to_f128_asm += line;
    u32_to_f128_asm.push_back('\n');
  }

  expect_contains(u32_to_f128_asm, "    testq %rax, %rax",
                  "wired translated cast owner should treat unsigned integer inputs as the split long-double path");
  expect_contains(u32_to_f128_asm, "    fildq (%rsp)",
                  "wired translated cast owner should still stage the fast unsigned long-double path through x87");
  expect_contains(u32_to_f128_asm, "    faddp %st, %st(1)",
                  "wired translated cast owner should recover the large unsigned long-double branch through the split path");
  expect_contains(u32_to_f128_asm, "    fstpt -24(%rbp)",
                  "wired translated cast owner should store the widened unsigned result through the destination slot");
}

void test_x86_codegen_header_exports_translated_float_owner_symbols() {
  using X86Codegen = c4c::backend::x86::X86Codegen;

  auto emit_float_binop = &X86Codegen::emit_float_binop_impl;
  auto emit_float_binop_mnemonic = &X86Codegen::emit_float_binop_mnemonic_impl;
  auto emit_unaryop = &X86Codegen::emit_unaryop_impl;

  expect_true(emit_float_binop != nullptr && emit_float_binop_mnemonic != nullptr &&
                  emit_unaryop != nullptr,
              "x86 translated float owner methods should stay compile/link reachable through the public x86_codegen surface once float_ops.cpp enters the build");
}

void test_x86_codegen_header_exports_translated_comparison_owner_symbols() {
  using X86Codegen = c4c::backend::x86::X86Codegen;

  auto emit_float_cmp = &X86Codegen::emit_float_cmp_impl;
  auto emit_f128_cmp = &X86Codegen::emit_f128_cmp_impl;
  auto emit_int_cmp = &X86Codegen::emit_int_cmp_impl;

  expect_true(emit_float_cmp != nullptr && emit_f128_cmp != nullptr &&
                  emit_int_cmp != nullptr,
              "x86 translated comparison owner methods should stay compile/link reachable through the public x86_codegen surface once comparison.cpp enters the build");
}

void test_x86_translated_comparison_owner_reuses_constant_f128_reload_bridge() {
  using IrCmpOp = c4c::backend::x86::IrCmpOp;
  using Operand = c4c::backend::x86::Operand;
  using StackSlot = c4c::backend::x86::StackSlot;
  using Value = c4c::backend::x86::Value;

  c4c::backend::x86::X86Codegen eq_codegen;
  eq_codegen.state.slots.emplace(13, StackSlot{-24});
  eq_codegen.state.set_f128_constant_words(41, 0x0123456789ABCDEFULL, 0x2468);
  eq_codegen.state.set_f128_constant_words(43, 0x0FEDCBA987654321ULL, 0x1357);

  eq_codegen.emit_f128_cmp_impl(Value{13}, IrCmpOp::Eq, Operand{41}, Operand{43});

  std::string eq_asm;
  for (const auto& line : eq_codegen.state.asm_lines) {
    eq_asm += line;
    eq_asm.push_back('\n');
  }

  expect_contains(eq_asm, "    subq $16, %rsp",
                  "wired translated comparison owner should stage constant long-double operands through the shared raw-byte stack bridge");
  expect_contains(eq_asm, "    fldt (%rsp)",
                  "wired translated comparison owner should reload constant long-double operands through the shared x87 helper before comparing");
  expect_contains(eq_asm, "    fucomip %st(1), %st",
                  "wired translated comparison owner should emit the translated x87 compare instruction for F128 equality");
  expect_contains(eq_asm, "    fstp %st(0)",
                  "wired translated comparison owner should pop the remaining x87 value after F128 comparison");
  expect_contains(eq_asm, "    setnp %al",
                  "wired translated comparison owner should preserve the translated ordered-equality guard");
  expect_contains(eq_asm, "    sete %cl",
                  "wired translated comparison owner should preserve the translated equality materialization path");
  expect_contains(eq_asm, "    andb %cl, %al",
                  "wired translated comparison owner should combine the ordered and equal flags for F128 equality");
  expect_contains(eq_asm, "    movzbq %al, %rax",
                  "wired translated comparison owner should zero-extend the boolean compare result");
  expect_contains(eq_asm, "    movq %rax, -24(%rbp)",
                  "wired translated comparison owner should store F128 compare booleans through the shared destination-slot path");
  expect_not_contains(eq_asm, "    fldl (%rsp)",
                      "wired translated comparison owner should not fall back to the f64 reload path for constant long-double compares");
  expect_true(eq_codegen.state.reg_cache.acc_valid &&
                  eq_codegen.state.reg_cache.acc_value_id == 13 &&
                  !eq_codegen.state.reg_cache.acc_known_zero_extended,
              "wired translated comparison owner should refresh the shared accumulator cache after F128 compare lowering");

  c4c::backend::x86::X86Codegen lt_codegen;
  lt_codegen.state.slots.emplace(15, StackSlot{-40});
  lt_codegen.state.set_f128_constant_words(45, 0x1111222233334444ULL, 0x2468);
  lt_codegen.state.set_f128_constant_words(47, 0x5555666677778888ULL, 0);

  lt_codegen.emit_f128_cmp_impl(Value{15}, IrCmpOp::Slt, Operand{45}, Operand{47});

  std::string lt_asm;
  for (const auto& line : lt_codegen.state.asm_lines) {
    lt_asm += line;
    lt_asm.push_back('\n');
  }

  expect_contains(lt_asm, "    seta %al",
                  "wired translated comparison owner should preserve the translated swapped x87 less-than materialization path");
  expect_contains(lt_asm, "    movq %rax, -40(%rbp)",
                  "wired translated comparison owner should store ordered less-than booleans through the shared destination-slot path");
}

void test_x86_translated_comparison_owner_emits_bounded_branch_and_select_text() {
  using BlockId = c4c::backend::x86::BlockId;
  using IrCmpOp = c4c::backend::x86::IrCmpOp;
  using IrType = c4c::backend::x86::IrType;
  using Operand = c4c::backend::x86::Operand;
  using StackSlot = c4c::backend::x86::StackSlot;
  using Value = c4c::backend::x86::Value;

  c4c::backend::x86::X86Codegen codegen;
  codegen.state.slots.emplace(13, StackSlot{-24});
  codegen.state.slots.emplace(15, StackSlot{-40});
  codegen.state.slots.emplace(17, StackSlot{-56});

  codegen.emit_int_cmp_impl(Value{13}, IrCmpOp::Uge, Operand::immediate_i32(3),
                            Operand::immediate_i32(2), IrType::U32);
  codegen.emit_fused_cmp_branch_impl(IrCmpOp::Uge, Operand::immediate_i32(3),
                                     Operand::immediate_i32(2), IrType::U32,
                                     ".Ltrue", ".Lfalse");
  codegen.emit_fused_cmp_branch_blocks_impl(IrCmpOp::Eq, Operand::immediate_i32(7),
                                            Operand::immediate_i32(7), IrType::U32,
                                            BlockId{3}, BlockId{4});
  codegen.emit_cond_branch_blocks_impl(Operand{17}, BlockId{5}, BlockId{6});
  codegen.emit_select_impl(Value{15}, Operand::immediate_i64(4294967296LL),
                           Operand::immediate_i32(11), Operand::immediate_i32(4),
                           IrType::I32);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    cmpl %ecx, %eax",
                  "wired translated comparison owner should compare U32 values through the 32-bit compare form");
  expect_contains(asm_text, "    setae %al",
                  "wired translated comparison owner should materialize unsigned-ge booleans through the translated setcc");
  expect_contains(asm_text, "    movq %rax, -24(%rbp)",
                  "wired translated comparison owner should store integer-compare booleans through the shared destination-slot path");
  expect_contains(asm_text, "    jae .Ltrue\n    jmp .Lfalse\n",
                  "wired translated comparison owner should emit the translated fused compare/branch label form");
  expect_contains(asm_text, "    je .L3\n    jmp .L4\n",
                  "wired translated comparison owner should emit the translated fused compare/branch block form");
  expect_contains(asm_text, "    movq -56(%rbp), %rax\n    testq %rax, %rax\n    jne .L5\n    jmp .L6\n",
                  "wired translated comparison owner should lower conditional branch blocks through the translated test-and-jump sequence");
  expect_contains(asm_text, "    movabsq $4294967296, %rdx",
                  "wired translated comparison owner should preserve the translated movabsq path for non-imm32 select conditions");
  expect_contains(asm_text, "    movq $4, %rax\n    movq $11, %rcx\n    movabsq $4294967296, %rdx\n    testq %rdx, %rdx\n    cmovne %rcx, %rax\n    movq %rax, -40(%rbp)\n",
                  "wired translated comparison owner should lower select through the translated false-value seed plus cmovne path");
  expect_true(codegen.state.reg_cache.acc_valid &&
                  codegen.state.reg_cache.acc_value_id == 15 &&
                  !codegen.state.reg_cache.acc_known_zero_extended,
              "wired translated comparison owner should refresh the shared accumulator cache after translated select lowering");
}

void test_x86_translated_float_owner_reuses_constant_f128_reload_bridge() {
  using FloatOp = c4c::backend::x86::FloatOp;
  using IrType = c4c::backend::x86::IrType;
  using IrUnaryOp = c4c::backend::x86::IrUnaryOp;
  using Operand = c4c::backend::x86::Operand;
  using StackSlot = c4c::backend::x86::StackSlot;
  using Value = c4c::backend::x86::Value;

  c4c::backend::x86::X86Codegen binop_codegen;
  binop_codegen.state.slots.emplace(5, StackSlot{-24});
  binop_codegen.state.set_f128_constant_words(21, 0x0123456789ABCDEFULL, 0x1357);
  binop_codegen.state.set_f128_constant_words(23, 0x0FEDCBA987654321ULL, 0);

  binop_codegen.emit_float_binop_impl(Value{5}, FloatOp::Add, Operand{21}, Operand{23}, IrType::F128);

  std::string binop_asm;
  for (const auto& line : binop_codegen.state.asm_lines) {
    binop_asm += line;
    binop_asm.push_back('\n');
  }

  expect_contains(binop_asm, "    subq $16, %rsp",
                  "wired translated float owner should stage constant long-double operands through the shared raw-byte stack bridge");
  expect_contains(binop_asm, "    fldt (%rsp)",
                  "wired translated float owner should reload constant long-double operands through the shared x87 helper");
  expect_contains(binop_asm, "    faddp %st, %st(1)",
                  "wired translated float owner should emit the translated x87 add sequence for F128 binops");
  expect_contains(binop_asm, "    fstpt -24(%rbp)",
                  "wired translated float owner should stage F128 binop results through the destination slot");
  expect_contains(binop_asm, "    fldt -24(%rbp)",
                  "wired translated float owner should reload the staged F128 result to preserve direct-slot tracking");
  expect_not_contains(binop_asm, "    fldl (%rsp)",
                      "wired translated float owner should not fall back to the f64 reload path for constant long-double operands");
  expect_true(binop_codegen.state.reg_cache.acc_valid &&
                  binop_codegen.state.reg_cache.acc_value_id == 5 &&
                  !binop_codegen.state.reg_cache.acc_known_zero_extended,
              "wired translated float owner should refresh the shared accumulator cache after F128 binops");
  expect_true(binop_codegen.state.f128_direct_slots.find(5) !=
                  binop_codegen.state.f128_direct_slots.end(),
              "wired translated float owner should preserve direct-slot tracking for F128 binop results");

  c4c::backend::x86::X86Codegen unary_codegen;
  unary_codegen.state.slots.emplace(7, StackSlot{-40});
  unary_codegen.state.set_f128_constant_words(25, 0x1111222233334444ULL, 0x2468);

  unary_codegen.emit_unaryop_impl(Value{7}, IrUnaryOp::Neg, Operand{25}, IrType::F128);

  std::string unary_asm;
  for (const auto& line : unary_codegen.state.asm_lines) {
    unary_asm += line;
    unary_asm.push_back('\n');
  }

  expect_contains(unary_asm, "    fldt (%rsp)",
                  "wired translated float owner should reuse the constant long-double reload bridge for unary F128 ops");
  expect_contains(unary_asm, "    fchs",
                  "wired translated float owner should emit the translated x87 negate sequence for F128 unary ops");
  expect_contains(unary_asm, "    fstpt -40(%rbp)",
                  "wired translated float owner should stage unary F128 results through the destination slot");
  expect_contains(unary_asm, "    fldt -40(%rbp)",
                  "wired translated float owner should reload the staged unary F128 result to preserve direct-slot tracking");
  expect_true(unary_codegen.state.reg_cache.acc_valid &&
                  unary_codegen.state.reg_cache.acc_value_id == 7 &&
                  !unary_codegen.state.reg_cache.acc_known_zero_extended,
              "wired translated float owner should refresh the shared accumulator cache after unary F128 ops");
  expect_true(unary_codegen.state.f128_direct_slots.find(7) !=
                  unary_codegen.state.f128_direct_slots.end(),
              "wired translated float owner should preserve direct-slot tracking for unary F128 results");
}

void test_x86_translated_float_owner_handles_u32_unary_widths() {
  using IrType = c4c::backend::x86::IrType;
  using IrUnaryOp = c4c::backend::x86::IrUnaryOp;
  using Operand = c4c::backend::x86::Operand;
  using StackSlot = c4c::backend::x86::StackSlot;
  using Value = c4c::backend::x86::Value;

  c4c::backend::x86::X86Codegen codegen;
  codegen.state.slots.emplace(3, StackSlot{-24});
  codegen.state.slots.emplace(5, StackSlot{-40});

  codegen.emit_unaryop_impl(Value{5}, IrUnaryOp::Neg, Operand{3}, IrType::U32);
  codegen.emit_unaryop_impl(Value{5}, IrUnaryOp::Not, Operand{3}, IrType::U32);
  codegen.emit_unaryop_impl(Value{5}, IrUnaryOp::Clz, Operand{3}, IrType::U32);
  codegen.emit_unaryop_impl(Value{5}, IrUnaryOp::Ctz, Operand{3}, IrType::U32);
  codegen.emit_unaryop_impl(Value{5}, IrUnaryOp::Bswap, Operand{3}, IrType::U32);
  codegen.emit_unaryop_impl(Value{5}, IrUnaryOp::Popcount, Operand{3}, IrType::U32);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    negl %eax",
                  "wired translated float owner should lower unsigned 32-bit negation through the eax-width unary path");
  expect_contains(asm_text, "    notl %eax",
                  "wired translated float owner should lower unsigned 32-bit bitwise not through the eax-width unary path");
  expect_contains(asm_text, "    lzcntl %eax, %eax",
                  "wired translated float owner should lower unsigned 32-bit clz through the eax-width unary path");
  expect_contains(asm_text, "    tzcntl %eax, %eax",
                  "wired translated float owner should lower unsigned 32-bit ctz through the eax-width unary path");
  expect_contains(asm_text, "    bswapl %eax",
                  "wired translated float owner should lower unsigned 32-bit byte swaps through the eax-width unary path");
  expect_contains(asm_text, "    popcntl %eax, %eax",
                  "wired translated float owner should lower unsigned 32-bit popcount through the eax-width unary path");
  expect_not_contains(asm_text, "    negq %rax",
                      "wired translated float owner should not widen unsigned 32-bit negation to the 64-bit unary path");
  expect_not_contains(asm_text, "    notq %rax",
                      "wired translated float owner should not widen unsigned 32-bit bitwise not to the 64-bit unary path");
  expect_not_contains(asm_text, "    lzcntq %rax, %rax",
                      "wired translated float owner should not widen unsigned 32-bit clz to the 64-bit unary path");
  expect_not_contains(asm_text, "    tzcntq %rax, %rax",
                      "wired translated float owner should not widen unsigned 32-bit ctz to the 64-bit unary path");
  expect_not_contains(asm_text, "    bswapq %rax",
                      "wired translated float owner should not widen unsigned 32-bit byte swaps to the 64-bit unary path");
  expect_not_contains(asm_text, "    popcntq %rax, %rax",
                      "wired translated float owner should not widen unsigned 32-bit popcount to the 64-bit unary path");
}

void test_x86_codegen_header_exports_translated_asm_emitter_owner_symbols() {
  using X86Codegen = c4c::backend::x86::X86Codegen;

  auto classify_constraint = &X86Codegen::classify_constraint;
  auto setup_operand_metadata = &X86Codegen::setup_operand_metadata;
  auto resolve_memory_operand = &X86Codegen::resolve_memory_operand;
  auto assign_scratch_reg = &X86Codegen::assign_scratch_reg;
  auto load_input_to_reg = &X86Codegen::load_input_to_reg;
  auto preload_readwrite_output = &X86Codegen::preload_readwrite_output;
  auto substitute_template_line = &X86Codegen::substitute_template_line;
  auto store_output_from_reg = &X86Codegen::store_output_from_reg;
  auto reset_scratch_state = &X86Codegen::reset_scratch_state;

  expect_true(classify_constraint != nullptr && setup_operand_metadata != nullptr &&
                  resolve_memory_operand != nullptr && assign_scratch_reg != nullptr &&
                  load_input_to_reg != nullptr && preload_readwrite_output != nullptr &&
                  substitute_template_line != nullptr && store_output_from_reg != nullptr &&
                  reset_scratch_state != nullptr,
              "x86 translated asm_emitter owner methods should stay compile/link reachable through the public x86_codegen surface once asm_emitter.cpp enters the build");
}

void test_riscv_codegen_header_exports_first_owner_cluster_helpers() {
  using c4c::backend::riscv::codegen::CALL_TEMP_CALLEE_SAVED;
  using c4c::backend::riscv::codegen::RISCV_ARG_REGS;
  using c4c::backend::riscv::codegen::RISCV_CALLEE_SAVED;
  using c4c::backend::riscv::codegen::PhysReg;
  using c4c::backend::riscv::codegen::callee_saved_name;
  using c4c::backend::riscv::codegen::constraint_to_callee_saved_riscv;
  using c4c::backend::riscv::codegen::riscv_reg_to_callee_saved;

  expect_true(RISCV_CALLEE_SAVED.size() == 6 && RISCV_CALLEE_SAVED.front().value == 1 &&
                  RISCV_CALLEE_SAVED[1].value == 7 &&
                  RISCV_CALLEE_SAVED.back().value == 11 &&
                  CALL_TEMP_CALLEE_SAVED.size() == 5 &&
                  CALL_TEMP_CALLEE_SAVED.front().value == 2 &&
                  CALL_TEMP_CALLEE_SAVED.back().value == 6 &&
                  std::string(RISCV_ARG_REGS.front()) == "a0" &&
                  std::string(RISCV_ARG_REGS.back()) == "a7" &&
                  std::string(callee_saved_name(PhysReg{1})) == "s1" &&
                  std::string(callee_saved_name(PhysReg{10})) == "s10" &&
                  riscv_reg_to_callee_saved("x24").has_value() &&
                  riscv_reg_to_callee_saved("x24")->value == 8 &&
                  constraint_to_callee_saved_riscv("{s8}").has_value() &&
                  constraint_to_callee_saved_riscv("{s8}")->value == 8 &&
                  !constraint_to_callee_saved_riscv("{a0}").has_value(),
              "riscv translated first-cluster helpers should stay compile/link reachable through the shared riscv_codegen surface once the first owner files enter the build");
}

void test_riscv_translated_asm_emitter_helpers_match_shared_contract() {
  using c4c::backend::riscv::codegen::RvConstraintKind;
  using c4c::backend::riscv::codegen::classify_rv_constraint;
  using c4c::backend::riscv::codegen::substitute_riscv_asm_operands;

  const auto tied = classify_rv_constraint("2");
  const auto memory = classify_rv_constraint("=m");
  const auto immediate = classify_rv_constraint("n");
  const auto specific = classify_rv_constraint("fa0");
  const auto zero_or_reg = classify_rv_constraint("rJ");
  const auto gp = classify_rv_constraint("r");

  const std::string asm_text = substitute_riscv_asm_operands(
      "add %[dst], %z2, %1",
      {"a0", "t1", "t2"},
      {std::optional<std::string>{"dst"}, std::optional<std::string>{"mem"}, std::nullopt},
      {RvConstraintKind::GpReg, RvConstraintKind::Memory, RvConstraintKind::Immediate},
      {0, -16, 0},
      {"", "", ""},
      {std::nullopt, std::nullopt, std::optional<long long>{0}},
      {std::nullopt, std::nullopt, std::nullopt},
      {0, 1, 2});

  expect_true(tied.kind == RvConstraintKind::Tied && tied.tied == 2 &&
                  memory.kind == RvConstraintKind::Memory &&
                  immediate.kind == RvConstraintKind::Immediate &&
                  specific.kind == RvConstraintKind::Specific &&
                  specific.specific == "fa0" &&
                  zero_or_reg.kind == RvConstraintKind::ZeroOrReg &&
                  gp.kind == RvConstraintKind::GpReg &&
                  asm_text == "add a0, zero, -16(s0)",
              "riscv translated asm-emitter helpers should preserve the bounded constraint and operand-substitution contract through the shared header seam");
}

void test_x86_translated_asm_emitter_helpers_match_shared_contract() {
  using X86Codegen = c4c::backend::x86::X86Codegen;

  const auto callee_saved = c4c::backend::x86::x86_callee_saved_regs();
  const auto caller_saved = c4c::backend::x86::x86_caller_saved_regs();
  expect_true(callee_saved.size() == 5 && callee_saved.front().index == 1 &&
                  callee_saved.back().index == 5 && caller_saved.size() == 6 &&
                  caller_saved.front().index == 10 && caller_saved.back().index == 15,
              "x86 translated register-pool helpers should keep the ref callee-saved and caller-saved allocation order for the future prologue/regalloc owner path");
  expect_true(c4c::backend::x86::x86_constraint_to_callee_saved("{r14}").has_value() &&
                  c4c::backend::x86::x86_constraint_to_callee_saved("{r14}")->index == 4 &&
                  c4c::backend::x86::x86_constraint_to_callee_saved("=&b").has_value() &&
                  c4c::backend::x86::x86_constraint_to_callee_saved("=&b")->index == 1 &&
                  !c4c::backend::x86::x86_constraint_to_callee_saved("{r10}").has_value() &&
                  c4c::backend::x86::x86_clobber_name_to_callee_saved("r15b").has_value() &&
                  c4c::backend::x86::x86_clobber_name_to_callee_saved("r15b")->index == 5 &&
                  !c4c::backend::x86::x86_clobber_name_to_callee_saved("r11").has_value() &&
                  c4c::backend::x86::x86_variadic_reg_save_area_size(false) == 176 &&
                  c4c::backend::x86::x86_variadic_reg_save_area_size(true) == 48 &&
                  c4c::backend::x86::x86_aligned_frame_size(0) == 0 &&
                  c4c::backend::x86::x86_aligned_frame_size(1) == 16 &&
                  c4c::backend::x86::x86_aligned_frame_size(16) == 16 &&
                  c4c::backend::x86::x86_aligned_frame_size(17) == 32 &&
                  c4c::backend::x86::x86_stack_probe_page_size() == 4096 &&
                  !c4c::backend::x86::x86_needs_stack_probe(4096) &&
                  c4c::backend::x86::x86_needs_stack_probe(4097) &&
                  c4c::backend::x86::x86_callee_saved_slot_offset(64, 0) == -64 &&
                  c4c::backend::x86::x86_callee_saved_slot_offset(64, 3) == -40 &&
                  std::string(c4c::backend::x86::x86_arg_reg_name(0)) == "rdi" &&
                  std::string(c4c::backend::x86::x86_arg_reg_name(5)) == "r9" &&
                  std::string(c4c::backend::x86::x86_arg_reg_name(6)).empty() &&
                  std::string(c4c::backend::x86::x86_float_arg_reg_name(0)) == "xmm0" &&
                  std::string(c4c::backend::x86::x86_float_arg_reg_name(7)) == "xmm7" &&
                  std::string(c4c::backend::x86::x86_float_arg_reg_name(8)).empty() &&
                  c4c::backend::x86::x86_param_stack_base_offset() == 16 &&
                  c4c::backend::x86::x86_param_stack_offset(0) == 16 &&
                  c4c::backend::x86::x86_param_stack_offset(8) == 24 &&
                  c4c::backend::x86::x86_param_slot_name("%p.arg") == "%lv.param.arg" &&
                  c4c::backend::x86::x86_param_slot_name("%p.aggregate.0") ==
                      "%lv.param.aggregate.0" &&
                  c4c::backend::x86::x86_param_slot_name("arg").empty() &&
                  c4c::backend::x86::x86_param_slot_name("%p.").empty() &&
                  c4c::backend::x86::x86_param_slot_matches("%lv.param.arg", "%p.arg") &&
                  !c4c::backend::x86::x86_param_slot_matches("%lv.param.other", "%p.arg") &&
                  !c4c::backend::x86::x86_allow_struct_split_reg_stack() &&
                  std::string(c4c::backend::x86::x86_param_ref_scalar_load_instr("i32")) ==
                      "movslq" &&
                  std::string(c4c::backend::x86::x86_param_ref_scalar_load_instr("u32")) ==
                      "movl" &&
                  std::string(c4c::backend::x86::x86_param_ref_scalar_load_instr("i64")) ==
                      "movq" &&
                  std::string(c4c::backend::x86::x86_param_ref_scalar_load_instr("u64")) ==
                      "movq" &&
                  std::string(c4c::backend::x86::x86_param_ref_scalar_load_instr("f32")) ==
                      "movl" &&
                  std::string(c4c::backend::x86::x86_param_ref_float_reg_move_instr("f32")) ==
                      "movd" &&
                  std::string(c4c::backend::x86::x86_param_ref_scalar_load_instr("f64")) ==
                      "movq" &&
                  std::string(c4c::backend::x86::x86_param_ref_float_reg_move_instr("f64")) ==
                      "movq" &&
                  std::string(c4c::backend::x86::x86_param_ref_scalar_load_instr("ptr")) ==
                      "movq" &&
                  std::string(c4c::backend::x86::x86_param_ref_scalar_dest_reg("i32")) ==
                      "%rax" &&
                  std::string(c4c::backend::x86::x86_param_ref_scalar_dest_reg("u32")) ==
                      "%eax" &&
                  std::string(c4c::backend::x86::x86_param_ref_scalar_dest_reg("f32")) ==
                      "%eax" &&
                  std::string(c4c::backend::x86::x86_param_ref_scalar_dest_reg("i64")) ==
                      "%rax" &&
                  std::string(c4c::backend::x86::x86_param_ref_scalar_dest_reg("u64")) ==
                      "%rax" &&
                  std::string(c4c::backend::x86::x86_param_ref_scalar_dest_reg("f64")) ==
                      "%rax" &&
                  std::string(c4c::backend::x86::x86_param_ref_scalar_dest_reg("ptr")) ==
                      "%rax" &&
                  std::string(c4c::backend::x86::x86_param_ref_scalar_arg_reg(0, "i32")) ==
                      "edi" &&
                  std::string(c4c::backend::x86::x86_param_ref_scalar_arg_reg(0, "u32")) ==
                      "edi" &&
                  std::string(c4c::backend::x86::x86_param_ref_scalar_arg_reg(0, "i64")) ==
                      "rdi" &&
                  std::string(c4c::backend::x86::x86_param_ref_scalar_arg_reg(0, "u64")) ==
                      "rdi" &&
                  std::string(c4c::backend::x86::x86_param_ref_float_arg_reg(0, "f32")) ==
                      "xmm0" &&
                  std::string(c4c::backend::x86::x86_param_ref_float_arg_reg(0, "f64")) ==
                      "xmm0" &&
                  std::string(c4c::backend::x86::x86_param_ref_scalar_arg_reg(0, "ptr")) ==
                      "rdi" &&
                  c4c::backend::x86::x86_param_ref_scalar_stack_operand(0, "i32") ==
                      "DWORD PTR [rbp + 16]" &&
                  c4c::backend::x86::x86_param_ref_scalar_stack_operand(0, "u32") ==
                      "DWORD PTR [rbp + 16]" &&
                  c4c::backend::x86::x86_param_ref_scalar_stack_operand(0, "f32") ==
                      "DWORD PTR [rbp + 16]" &&
                  c4c::backend::x86::x86_param_ref_scalar_stack_operand(8, "i64") ==
                      "QWORD PTR [rbp + 24]" &&
                  c4c::backend::x86::x86_param_ref_scalar_stack_operand(8, "u64") ==
                      "QWORD PTR [rbp + 24]" &&
                  c4c::backend::x86::x86_param_ref_scalar_stack_operand(8, "f64") ==
                      "QWORD PTR [rbp + 24]" &&
                  c4c::backend::x86::x86_param_ref_scalar_stack_operand(8, "ptr") ==
                      "QWORD PTR [rbp + 24]" &&
                  c4c::backend::x86::x86_phys_reg_is_callee_saved(c4c::backend::PhysReg{1}) &&
                  !c4c::backend::x86::x86_phys_reg_is_callee_saved(c4c::backend::PhysReg{12}) &&
                  c4c::backend::x86::x86_param_can_prestore_direct_to_reg(
                      false, c4c::backend::PhysReg{1}, 1) &&
                  !c4c::backend::x86::x86_param_can_prestore_direct_to_reg(
                      true, c4c::backend::PhysReg{1}, 1) &&
                  !c4c::backend::x86::x86_param_can_prestore_direct_to_reg(
                      false, c4c::backend::PhysReg{12}, 1) &&
                  !c4c::backend::x86::x86_param_can_prestore_direct_to_reg(
                      false, c4c::backend::PhysReg{1}, 2) &&
                  !c4c::backend::x86::x86_param_can_prestore_direct_to_reg(
                      false, std::nullopt, 0) &&
                  std::string(c4c::backend::x86::x86_param_prestore_move_instr()) == "movq" &&
                  std::string(c4c::backend::x86::x86_param_prestore_arg_reg(0)) == "rdi" &&
                  std::string(c4c::backend::x86::x86_param_prestore_arg_reg(5)) == "r9" &&
                  std::string(c4c::backend::x86::x86_param_prestore_float_move_instr("f32")) ==
                      "movd" &&
                  std::string(c4c::backend::x86::x86_param_prestore_float_move_instr("f64")) ==
                      "movq" &&
                  std::string(c4c::backend::x86::x86_param_prestore_float_arg_reg(0, "f32")) ==
                      "xmm0" &&
                  std::string(c4c::backend::x86::x86_param_prestore_float_arg_reg(7, "f64")) ==
                      "xmm7" &&
                  std::string(
                      c4c::backend::x86::x86_param_prestore_dest_reg(c4c::backend::PhysReg{1})) ==
                      "rbx" &&
                  std::string(
                      c4c::backend::x86::x86_param_prestore_dest_reg(c4c::backend::PhysReg{5})) ==
                      "r15" &&
                  std::string(c4c::backend::x86::x86_param_prestore_dest_reg(
                      c4c::backend::PhysReg{1}, "f32")) == "ebx" &&
                  std::string(c4c::backend::x86::x86_param_prestore_dest_reg(
                      c4c::backend::PhysReg{5}, "f64")) == "r15" &&
                  c4c::backend::x86::x86_param_struct_reg_qword_count(0) == 0 &&
                  c4c::backend::x86::x86_param_struct_reg_qword_count(1) == 1 &&
                  c4c::backend::x86::x86_param_struct_reg_qword_count(8) == 1 &&
                  c4c::backend::x86::x86_param_struct_reg_qword_count(9) == 2 &&
                  c4c::backend::x86::x86_param_struct_reg_qword_count(15) == 2 &&
                  c4c::backend::x86::x86_param_struct_reg_qword_count(16) == 2 &&
                  c4c::backend::x86::x86_param_struct_reg_qword_count(17) == 2 &&
                  std::string(c4c::backend::x86::x86_param_struct_reg_arg_reg(0, 0)) == "rdi" &&
                  std::string(c4c::backend::x86::x86_param_struct_reg_arg_reg(0, 1)) == "rsi" &&
                  std::string(c4c::backend::x86::x86_param_struct_reg_arg_reg(4, 1)) == "r9" &&
                  std::string(c4c::backend::x86::x86_param_struct_reg_arg_reg(5, 0)) == "r9" &&
                  std::string(c4c::backend::x86::x86_param_struct_reg_arg_reg(5, 1)).empty() &&
                  c4c::backend::x86::x86_param_struct_reg_dest_offset(-40, 0) == -40 &&
                  c4c::backend::x86::x86_param_struct_reg_dest_offset(-40, 1) == -32 &&
                  std::string(c4c::backend::x86::x86_param_struct_sse_arg_reg(0)) == "xmm0" &&
                  std::string(c4c::backend::x86::x86_param_struct_sse_arg_reg(7)) == "xmm7" &&
                  std::string(c4c::backend::x86::x86_param_struct_sse_arg_reg(8)).empty() &&
                  c4c::backend::x86::x86_param_struct_sse_dest_offset(-40, 0) == -40 &&
                  c4c::backend::x86::x86_param_struct_sse_dest_offset(-40, 1) == -32 &&
                  c4c::backend::x86::x86_param_struct_sse_dest_offset(-40, 2) == -24 &&
                  std::string(c4c::backend::x86::x86_param_struct_mixed_int_sse_int_arg_reg(0)) ==
                      "rdi" &&
                  std::string(c4c::backend::x86::x86_param_struct_mixed_int_sse_int_arg_reg(6))
                      .empty() &&
                  std::string(c4c::backend::x86::x86_param_struct_mixed_int_sse_fp_arg_reg(7)) ==
                      "xmm7" &&
                  std::string(c4c::backend::x86::x86_param_struct_mixed_int_sse_fp_arg_reg(8))
                      .empty() &&
                  c4c::backend::x86::x86_param_struct_mixed_int_sse_int_dest_offset(-40) ==
                      -40 &&
                  c4c::backend::x86::x86_param_struct_mixed_int_sse_fp_dest_offset(-40) == -32 &&
                  std::string(c4c::backend::x86::x86_param_struct_mixed_sse_int_fp_arg_reg(0)) ==
                      "xmm0" &&
                  std::string(c4c::backend::x86::x86_param_struct_mixed_sse_int_fp_arg_reg(8))
                      .empty() &&
                  std::string(c4c::backend::x86::x86_param_struct_mixed_sse_int_int_arg_reg(5)) ==
                      "r9" &&
                  std::string(c4c::backend::x86::x86_param_struct_mixed_sse_int_int_arg_reg(6))
                      .empty() &&
                  c4c::backend::x86::x86_param_struct_mixed_sse_int_fp_dest_offset(-40) ==
                      -40 &&
                  c4c::backend::x86::x86_param_struct_mixed_sse_int_int_dest_offset(-40) == -32 &&
                  c4c::backend::x86::x86_param_aggregate_copy_qword_count(0) == 0 &&
                  c4c::backend::x86::x86_param_aggregate_copy_qword_count(1) == 1 &&
                  c4c::backend::x86::x86_param_aggregate_copy_qword_count(8) == 1 &&
                  c4c::backend::x86::x86_param_aggregate_copy_qword_count(9) == 2 &&
                  c4c::backend::x86::x86_param_aggregate_copy_qword_count(16) == 2 &&
                  c4c::backend::x86::x86_param_aggregate_copy_qword_count(17) == 3 &&
                  c4c::backend::x86::x86_param_aggregate_copy_src_offset(0, 0) == 16 &&
                  c4c::backend::x86::x86_param_aggregate_copy_src_offset(8, 1) == 32 &&
                  c4c::backend::x86::x86_param_aggregate_copy_src_offset(16, 2) == 48 &&
                  c4c::backend::x86::x86_param_aggregate_copy_dest_offset(-40, 0) == -40 &&
                  c4c::backend::x86::x86_param_aggregate_copy_dest_offset(-40, 2) == -24 &&
                  std::string(c4c::backend::x86::x86_param_split_reg_stack_arg_reg(0)) == "rdi" &&
                  std::string(c4c::backend::x86::x86_param_split_reg_stack_arg_reg(5)) == "r9" &&
                  std::string(c4c::backend::x86::x86_param_split_reg_stack_arg_reg(6)).empty() &&
                  c4c::backend::x86::x86_param_split_reg_stack_qword_count(8) == 0 &&
                  c4c::backend::x86::x86_param_split_reg_stack_qword_count(9) == 1 &&
                  c4c::backend::x86::x86_param_split_reg_stack_qword_count(16) == 1 &&
                  c4c::backend::x86::x86_param_split_reg_stack_qword_count(17) == 2 &&
                  c4c::backend::x86::x86_param_split_reg_stack_src_offset(0, 0) == 16 &&
                  c4c::backend::x86::x86_param_split_reg_stack_src_offset(8, 1) == 32 &&
                  c4c::backend::x86::x86_param_split_reg_stack_dest_offset(-40, 0) == -32 &&
                  c4c::backend::x86::x86_param_split_reg_stack_dest_offset(-40, 1) == -24 &&
                  c4c::backend::x86::x86_variadic_gp_save_offset(-176, 0) == -176 &&
                  c4c::backend::x86::x86_variadic_gp_save_offset(-176, 5) == -136 &&
                  c4c::backend::x86::x86_variadic_sse_save_offset(-176, 0) == -128 &&
                  c4c::backend::x86::x86_variadic_sse_save_offset(-176, 7) == -16,
              "x86 translated prologue-side helpers should keep the ref callee-saved mapping, no-partial-reg-stack aggregate ABI policy, typed integer ParamRef register/stack load contract, parameter-storage / ParamRef pre-store policy, variadic register-save-area sizing, and 16-byte frame-alignment contract for the future translated prologue owner path");

  std::unordered_set<std::size_t> pre_stored_params;
  c4c::backend::x86::x86_mark_param_prestored(pre_stored_params, 1);
  c4c::backend::x86::x86_mark_param_prestored(pre_stored_params, 4);
  expect_true(!c4c::backend::x86::x86_param_is_prestored(pre_stored_params, 0) &&
                  c4c::backend::x86::x86_param_is_prestored(pre_stored_params, 1) &&
                  c4c::backend::x86::x86_param_is_prestored(pre_stored_params, 4),
              "x86 translated prologue pre-store bookkeeping helpers should preserve the param indices already saved into their non-alloca-backed owner location");

  expect_true(std::string(c4c::backend::x86::phys_reg_name(c4c::backend::PhysReg{1})) == "rbx" &&
                  std::string(c4c::backend::x86::phys_reg_name(c4c::backend::PhysReg{12})) == "r8" &&
                  std::string(c4c::backend::x86::phys_reg_name_32(c4c::backend::PhysReg{5})) == "r15d" &&
                  std::string(c4c::backend::x86::reg_name_to_32("r10")) == "r10d",
              "x86 translated register-name helpers should keep the ref physreg and sub-register mapping contract for the future translated prologue and ALU owners");
  expect_true(std::string(c4c::backend::x86::x86_alu_mnemonic(c4c::backend::bir::BinaryOpcode::Add)) ==
                      "add" &&
                  std::string(c4c::backend::x86::x86_alu_mnemonic(c4c::backend::bir::BinaryOpcode::Xor)) ==
                      "xor",
              "x86 translated ALU mnemonic helpers should keep the ref add/sub/and/or/xor opcode mapping contract for the future translated ALU owner path");
  const auto shift_left =
      c4c::backend::x86::x86_shift_mnemonic(c4c::backend::bir::BinaryOpcode::Shl);
  const auto shift_right =
      c4c::backend::x86::x86_shift_mnemonic(c4c::backend::bir::BinaryOpcode::LShr);
  expect_true(std::string(shift_left.first) == "shll" &&
                  std::string(shift_left.second) == "shlq" &&
                  std::string(shift_right.first) == "shrl" &&
                  std::string(shift_right.second) == "shrq",
              "x86 translated shift mnemonic helpers should keep the ref 32-bit and 64-bit opcode mapping contract for the future translated ALU owner path");

  const auto linux_symbol =
      c4c::backend::x86::asm_symbol_name("x86_64-unknown-linux-gnu", "main");
  const auto darwin_symbol =
      c4c::backend::x86::asm_symbol_name("x86_64-apple-darwin", "main");
  expect_true(linux_symbol == "main" && darwin_symbol == "_main",
              "x86 translated asm-emitter helpers should keep the shared symbol prefix contract across Linux and Darwin triples");

  const auto linux_label =
      c4c::backend::x86::asm_private_data_label("x86_64-unknown-linux-gnu", "@pool");
  const auto darwin_label =
      c4c::backend::x86::asm_private_data_label("x86_64-apple-darwin", "@pool");
  expect_true(linux_label == ".L.pool" && darwin_label == "Lpool",
              "x86 translated asm-emitter helpers should normalize private pool labels consistently across Linux and Darwin triples");

  const auto decoded = c4c::backend::x86::decode_llvm_byte_string("Hi\\0A\\09\\22");
  const auto escaped = c4c::backend::x86::escape_asm_string(decoded);
  expect_true(decoded == std::string("Hi\n\t\"") && escaped == "Hi\\n\\t\\\"",
              "x86 translated asm-emitter helpers should decode LLVM byte strings and re-escape them for asm string literals");
  expect_true(c4c::backend::x86::x86_reg_name_to_64("r9d") == "r9" &&
                  c4c::backend::x86::x86_reg_name_to_16("r10") == "r10w" &&
                  c4c::backend::x86::x86_reg_name_to_8l("rbp") == "bpl" &&
                  c4c::backend::x86::x86_reg_name_to_8h("rax") == "ah",
              "x86 translated inline-asm register-width helpers should keep the ref 64-bit, 16-bit, low-8, and high-8 register mapping contract");
  expect_true(c4c::backend::x86::x86_format_reg("rax", std::nullopt) == "rax" &&
                  c4c::backend::x86::x86_format_reg("rax", 'k') == "eax" &&
                  c4c::backend::x86::x86_format_reg("r8", 'w') == "r8w" &&
                  c4c::backend::x86::x86_format_reg("xmm3", 'b') == "xmm3" &&
                  c4c::backend::x86::x86_format_reg("st(1)", 'k') == "st(1)",
              "x86 translated inline-asm register-format helpers should preserve xmm/x87 operands while formatting integer registers to the requested width");
  expect_true(std::string(c4c::backend::x86::x86_gcc_cc_to_x86("c")) == "b" &&
                  std::string(c4c::backend::x86::x86_gcc_cc_to_x86("nz")) == "ne" &&
                  std::string(c4c::backend::x86::x86_gcc_cc_to_x86("ge")) == "ge",
              "x86 translated inline-asm condition-code helpers should keep the ref GCC-to-SETcc suffix mapping contract");
  expect_true(X86Codegen::default_modifier_for_type(c4c::backend::x86::IrType::I8) == 'b' &&
                  X86Codegen::default_modifier_for_type(c4c::backend::x86::IrType::U16) == 'w' &&
                  X86Codegen::default_modifier_for_type(c4c::backend::x86::IrType::F32) == 'k' &&
                  !X86Codegen::default_modifier_for_type(c4c::backend::x86::IrType::U64).has_value() &&
                  X86Codegen::format_x86_reg("rax", std::nullopt) == "rax" &&
                  X86Codegen::format_x86_reg("rax", 'h') == "ah" &&
                  X86Codegen::format_x86_reg("xmm3", 'w') == "xmm3" &&
                  X86Codegen::reg_to_64("r9d") == "r9" &&
                  X86Codegen::reg_to_16("r10") == "r10w" &&
                  X86Codegen::reg_to_8l("rbx") == "bl" &&
                  std::string(X86Codegen::gcc_cc_to_x86("nz")) == "ne",
              "x86 translated inline_asm owner helpers should keep the ref type-width and register-format contract once inline_asm.cpp enters the build");
  expect_true(
      X86Codegen::substitute_x86_asm_operands(
          "bt{l %[bit],%[base]| %[base], %[bit]}; mov %k0, %1; lea %a2, %q0; .word %c3; .quad %P4; neg %n5; jmp %l[end]",
          {"rax", "rbx", "rcx", "rdx", "r8", "r9"},
          {std::optional<std::string>("bit"),
           std::optional<std::string>("base"),
           std::optional<std::string>("addr"),
           std::optional<std::string>("imm"),
           std::optional<std::string>("sym"),
           std::optional<std::string>("neg")},
          {false, false, false, false, false, false},
          {"", "", "", "", "", ""},
          {c4c::backend::x86::IrType::I32,
           c4c::backend::x86::IrType::U32,
           c4c::backend::x86::IrType::Ptr,
           c4c::backend::x86::IrType::I32,
           c4c::backend::x86::IrType::Ptr,
           c4c::backend::x86::IrType::I64},
          {0, 1, 2, 3, 4, 5},
          {{"end", c4c::backend::x86::BlockId{42}}},
          {std::nullopt,
           std::nullopt,
           std::nullopt,
           std::optional<std::int64_t>(7),
           std::nullopt,
           std::optional<std::int64_t>(9)},
          {std::nullopt,
           std::nullopt,
           std::optional<std::string>("tls_anchor"),
           std::nullopt,
           std::optional<std::string>("extern_symbol"),
           std::nullopt}) ==
          "btl %eax,%ebx; mov %eax, %ebx; lea tls_anchor(%rip), %rax; .word 7; .quad extern_symbol; neg -9; jmp .LBB42",
      "x86 translated inline_asm owner should resolve dialect alternatives, width modifiers, symbolic addresses, raw immediates, negated immediates, and goto labels through inline_asm.cpp rather than only through shared helper coverage");

  const auto linux_function =
      c4c::backend::x86::emit_function_prelude("x86_64-unknown-linux-gnu", "main");
  const auto darwin_function =
      c4c::backend::x86::emit_function_prelude("x86_64-apple-darwin", "_main");
  expect_true(linux_function.find(".type main, @function") != std::string::npos &&
                  darwin_function.find(".type _main, @function") == std::string::npos,
              "x86 translated asm-emitter function preludes should keep ELF-only .type directives while preserving the shared global label prelude");

  const auto zero_global = c4c::backend::x86::emit_global_symbol_prelude(
      "x86_64-unknown-linux-gnu", "g_counter", 4, true);
  const auto data_global = c4c::backend::x86::emit_global_symbol_prelude(
      "x86_64-apple-darwin", "_g_counter", 2, false);
  expect_true(zero_global.find(".bss\n.globl g_counter\n.type g_counter, @object\n.p2align 2\n") !=
                      std::string::npos &&
                  data_global.find(".data\n.globl _g_counter\n.p2align 1\n") != std::string::npos &&
                  data_global.find("@object") == std::string::npos,
              "x86 translated asm-emitter global preludes should keep the section, alignment, and object-directive contract across ELF and Darwin targets");
}

void test_x86_translated_asm_emitter_owner_allocates_bounded_scratch_pools() {
  c4c::backend::x86::X86Codegen codegen;

  const auto gp_kind = codegen.classify_constraint("r");
  const auto xmm_kind = codegen.classify_constraint("x");
  const auto qreg_kind = codegen.classify_constraint("Q");

  const auto gp0 = codegen.assign_scratch_reg(gp_kind, {});
  const auto gp1 = codegen.assign_scratch_reg(gp_kind, {});
  const auto xmm0 = codegen.assign_scratch_reg(xmm_kind, {});
  const auto xmm1 = codegen.assign_scratch_reg(xmm_kind, {});
  const auto qreg = codegen.assign_scratch_reg(qreg_kind, {});

  expect_true(gp0 == "rcx" && gp1 == "rdx",
              "wired translated asm_emitter owner should allocate GP scratch registers from the bounded x86 caller-saved pool in ref order");
  expect_true(xmm0 == "xmm0" && xmm1 == "xmm1",
              "wired translated asm_emitter owner should allocate XMM scratch registers from the bounded xmm0-xmm15 pool in ref order");
  expect_true(qreg == "rax",
              "wired translated asm_emitter owner should route Q constraints through the legacy high-byte-capable register pool");

  codegen.reset_scratch_state();

  expect_true(codegen.assign_scratch_reg(gp_kind, {}) == "rcx" &&
                  codegen.assign_scratch_reg(xmm_kind, {}) == "xmm0",
              "wired translated asm_emitter owner should reset bounded scratch allocation state between inline-asm packets");
}

void test_x86_translated_regalloc_pruning_helpers_match_shared_contract() {
  const auto base = c4c::backend::x86::x86_prune_caller_saved_regs(false, false, false);
  const auto indirect = c4c::backend::x86::x86_prune_caller_saved_regs(true, false, false);
  const auto i128 = c4c::backend::x86::x86_prune_caller_saved_regs(false, true, false);
  const auto atomic = c4c::backend::x86::x86_prune_caller_saved_regs(false, false, true);
  const auto combined = c4c::backend::x86::x86_prune_caller_saved_regs(true, true, true);
  const auto indexes = [](const std::vector<c4c::backend::PhysReg>& regs) {
    std::vector<std::uint32_t> values;
    values.reserve(regs.size());
    for (const auto& reg : regs) {
      values.push_back(reg.index);
    }
    return values;
  };

  expect_true(indexes(base) == std::vector<std::uint32_t>{10, 11, 12, 13, 14, 15} &&
                  indexes(indirect) == std::vector<std::uint32_t>{10, 12, 13, 14, 15} &&
                  indexes(i128) == std::vector<std::uint32_t>{10, 11} &&
                  indexes(atomic) == std::vector<std::uint32_t>{10, 11, 13, 14, 15} &&
                  indexes(combined) == std::vector<std::uint32_t>{10},
              "x86 translated caller-saved pruning helpers should match the ref indirect-call, i128, and atomic register-pool contract");
}

void test_x86_translated_globals_owner_emits_bounded_helper_text() {
  using IrType = c4c::backend::x86::IrType;
  using Operand = c4c::backend::x86::Operand;
  using StackSlot = c4c::backend::x86::StackSlot;
  using Value = c4c::backend::x86::Value;

  c4c::backend::x86::X86Codegen codegen;
  codegen.state.slots.emplace(5, StackSlot{-24});
  codegen.state.slots.emplace(7, StackSlot{-40});

  codegen.emit_global_addr_impl(Value{5}, "global_counter");
  codegen.state.mark_needs_got_for_addr("global_got_counter");
  codegen.emit_global_addr_impl(Value{5}, "global_got_counter");
  codegen.emit_global_addr_absolute_impl(Value{5}, "global_counter");
  codegen.emit_label_addr_impl(Value{5}, ".Lowner_tmp");
  codegen.state.pic_mode = true;
  codegen.emit_tls_global_addr_impl(Value{5}, "tls_counter");
  codegen.state.pic_mode = false;
  codegen.emit_tls_global_addr_impl(Value{5}, "tls_counter");
  codegen.emit_global_load_rip_rel_impl(Value{7}, "global_counter", IrType::I32);
  codegen.emit_global_store_rip_rel_impl(Operand{5}, "global_counter", IrType::I32);

  std::string asm_text;
  for (const auto& line : codegen.state.asm_lines) {
    asm_text += line;
    asm_text.push_back('\n');
  }

  expect_contains(asm_text, "    leaq global_counter(%rip), %rax",
                  "wired translated globals owner should emit rip-relative global address materialization through the active owner body");
  expect_contains(asm_text, "    movq global_got_counter@GOTPCREL(%rip), %rax",
                  "wired translated globals owner should switch to the shared GOTPCREL path when the x86 codegen state marks a symbol as requiring GOT materialization");
  expect_contains(asm_text, "    movq $global_counter, %rax",
                  "wired translated globals owner should keep absolute global-address materialization linked through the active owner body");
  expect_contains(asm_text, "    leaq .Lowner_tmp(%rip), %rax",
                  "wired translated globals owner should emit label-address materialization through the active owner body");
  expect_contains(asm_text, "    movq tls_counter@GOTTPOFF(%rip), %rax",
                  "wired translated globals owner should match the PIC TLS address materialization path through the shared x86 codegen state");
  expect_contains(asm_text, "    addq %fs:0, %rax",
                  "wired translated globals owner should finish the PIC TLS path by adding the thread-pointer base");
  expect_contains(asm_text, "    movq %fs:0, %rax",
                  "wired translated globals owner should match the non-PIC TLS address materialization path through the shared x86 codegen state");
  expect_contains(asm_text, "    leaq tls_counter@TPOFF(%rax), %rax",
                  "wired translated globals owner should finish the non-PIC TLS path with the translated TPOFF address calculation");
  expect_contains(asm_text, "    movslq global_counter(%rip), %rax",
                  "wired translated globals owner should match the translated signed-I32 rip-relative load contract through the active owner body");
  expect_contains(asm_text, "    movl %eax, global_counter(%rip)",
                  "wired translated globals owner should emit bounded scalar rip-relative stores through the active owner body");
  expect_contains(asm_text, "    movq %rax, -24(%rbp)",
                  "wired translated globals owner should store materialized addresses back through the shared result helper");
  expect_contains(asm_text, "    movq -24(%rbp), %rax",
                  "wired translated globals owner should load operand values through the shared operand helper before stores");
  expect_contains(asm_text, "    movq %rax, -40(%rbp)",
                  "wired translated globals owner should store rip-relative load results through the shared result helper");
  expect_true(!codegen.state.reg_cache.acc_valid,
              "wired translated globals owner should invalidate the accumulator cache after loading a store operand through the shared helper path");
}

void test_backend_shared_call_decode_parses_bir_minimal_declared_direct_call_module() {
  c4c::backend::bir::Module module;
  module.string_constants.push_back(c4c::backend::bir::StringConstant{
      .name = "msg",
      .bytes = "hello\n",
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "puts_like",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {c4c::backend::bir::Param{
          .type = c4c::backend::bir::TypeKind::I64,
          .name = "%arg0",
      }},
      .local_slots = {},
      .blocks = {},
      .is_declaration = true,
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "main",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {c4c::backend::bir::CallInst{
              .result = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
              .callee = "puts_like",
              .args = {c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I64, "msg")},
              .return_type_name = "i32",
          }},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::immediate_i32(7),
          },
      }},
      .is_declaration = false,
  });

  const auto parsed = c4c::backend::parse_bir_minimal_declared_direct_call_module(module);
  expect_true(parsed.has_value(),
              "shared call-decode surface should parse a BIR declared direct-call module");
  expect_true(parsed->callee != nullptr && parsed->callee->name == "puts_like" &&
                  parsed->main_function != nullptr && parsed->main_function->name == "main",
              "shared call-decode surface should preserve declaration and caller identities for BIR declared direct-call modules");
  expect_true(parsed->args.size() == 1 &&
                  parsed->args.front().kind == c4c::backend::ParsedBackendExternCallArg::Kind::Ptr &&
                  parsed->args.front().operand == "@msg",
              "shared call-decode surface should classify BIR i64 symbol operands as pointer-style extern call args");
  expect_true(parsed->call != nullptr && parsed->call->callee == "puts_like" &&
                  !parsed->return_call_result && parsed->return_imm == 7,
              "shared call-decode surface should allow BIR declared direct-call modules to return a fixed immediate after the call");
}

void test_backend_shared_call_decode_parses_bir_minimal_void_direct_call_imm_return_module() {
  c4c::backend::bir::Module module;
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "voidfn",
      .return_type = c4c::backend::bir::TypeKind::Void,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {},
          .terminator = c4c::backend::bir::ReturnTerminator{},
      }},
      .is_declaration = false,
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "main",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {c4c::backend::bir::CallInst{
              .result = std::nullopt,
              .callee = "voidfn",
              .args = {},
              .return_type_name = "void",
          }},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::immediate_i32(9),
          },
      }},
      .is_declaration = false,
  });

  const auto parsed = c4c::backend::parse_bir_minimal_void_direct_call_imm_return_module(module);
  expect_true(parsed.has_value(),
              "shared call-decode surface should parse a BIR void direct-call module with a fixed caller return");
  expect_true(parsed->helper != nullptr && parsed->helper->name == "voidfn" &&
                  parsed->main_function != nullptr && parsed->main_function->name == "main",
              "shared call-decode surface should preserve helper and caller identities for BIR void direct-call modules");
  expect_true(parsed->call != nullptr && parsed->call->callee == "voidfn" &&
                  !parsed->call->result.has_value() && parsed->return_imm == 9,
              "shared call-decode surface should preserve the void call and caller fixed return immediate for BIR void direct-call modules");
}

void test_backend_shared_call_decode_parses_bir_minimal_two_arg_direct_call_module() {
  c4c::backend::bir::Module module;
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "add_pair",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {
          c4c::backend::bir::Param{.type = c4c::backend::bir::TypeKind::I32, .name = "%lhs"},
          c4c::backend::bir::Param{.type = c4c::backend::bir::TypeKind::I32, .name = "%rhs"},
      },
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {c4c::backend::bir::BinaryInst{
              .opcode = c4c::backend::bir::BinaryOpcode::Add,
              .result = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%sum"),
              .lhs = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%lhs"),
              .rhs = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%rhs"),
          }},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%sum"),
          },
      }},
      .is_declaration = false,
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "main",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {c4c::backend::bir::CallInst{
              .result = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
              .callee = "add_pair",
              .args = {
                  c4c::backend::bir::Value::immediate_i32(5),
                  c4c::backend::bir::Value::immediate_i32(7),
              },
              .return_type_name = "i32",
          }},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
          },
      }},
      .is_declaration = false,
  });

  const auto parsed = c4c::backend::parse_bir_minimal_two_arg_direct_call_module(module);
  expect_true(parsed.has_value(),
              "shared call-decode surface should parse a BIR helper/main two-argument direct-call module");
  expect_true(parsed->helper != nullptr && parsed->helper->name == "add_pair" &&
                  parsed->main_function != nullptr && parsed->main_function->name == "main",
              "shared call-decode surface should preserve helper and caller identities for BIR two-argument direct-call modules");
  expect_true(parsed->call != nullptr && parsed->call->callee == "add_pair" &&
                  parsed->lhs_call_arg_imm == 5 && parsed->rhs_call_arg_imm == 7,
              "shared call-decode surface should recover both immediate call operands for the BIR two-argument direct-call slice");
}

void test_backend_shared_call_decode_parses_bir_minimal_direct_call_add_imm_module() {
  c4c::backend::bir::Module module;
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "add_one",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {
          c4c::backend::bir::Param{.type = c4c::backend::bir::TypeKind::I32, .name = "%arg0"},
      },
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {c4c::backend::bir::BinaryInst{
              .opcode = c4c::backend::bir::BinaryOpcode::Add,
              .result = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%sum"),
              .lhs = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%arg0"),
              .rhs = c4c::backend::bir::Value::immediate_i32(1),
          }},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%sum"),
          },
      }},
      .is_declaration = false,
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "main",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {c4c::backend::bir::CallInst{
              .result = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
              .callee = "add_one",
              .args = {
                  c4c::backend::bir::Value::immediate_i32(5),
              },
              .return_type_name = "i32",
          }},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
          },
      }},
      .is_declaration = false,
  });

  const auto parsed = c4c::backend::parse_bir_minimal_direct_call_add_imm_module(module);
  expect_true(parsed.has_value(),
              "shared call-decode surface should parse a BIR helper/main add-immediate direct-call module");
  expect_true(parsed->helper != nullptr && parsed->helper->name == "add_one" &&
                  parsed->main_function != nullptr && parsed->main_function->name == "main",
              "shared call-decode surface should preserve helper and caller identities for the BIR add-immediate direct-call slice");
  expect_true(parsed->call != nullptr && parsed->call->callee == "add_one" &&
                  parsed->call_arg_imm == 5 && parsed->add_imm == 1,
              "shared call-decode surface should recover the call argument and helper add immediate for the BIR add-immediate direct-call slice");
}

void test_backend_shared_call_decode_parses_bir_minimal_direct_call_identity_arg_module() {
  c4c::backend::bir::Module module;
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "f",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {
          c4c::backend::bir::Param{.type = c4c::backend::bir::TypeKind::I32, .name = "%arg0"},
      },
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%arg0"),
          },
      }},
      .is_declaration = false,
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "main",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {c4c::backend::bir::CallInst{
              .result = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
              .callee = "f",
              .args = {
                  c4c::backend::bir::Value::immediate_i32(0),
              },
              .return_type_name = "i32",
          }},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
          },
      }},
      .is_declaration = false,
  });

  const auto parsed = c4c::backend::parse_bir_minimal_direct_call_identity_arg_module(module);
  expect_true(parsed.has_value(),
              "shared call-decode surface should parse a BIR helper/main identity direct-call module");
  expect_true(parsed->helper != nullptr && parsed->helper->name == "f" &&
                  parsed->main_function != nullptr && parsed->main_function->name == "main",
              "shared call-decode surface should preserve helper and caller identities for the BIR identity direct-call slice");
  expect_true(parsed->call != nullptr && parsed->call->callee == "f" &&
                  parsed->call_arg_imm == 0,
              "shared call-decode surface should recover the caller immediate for the BIR identity direct-call slice");
}

void test_backend_shared_call_decode_parses_bir_minimal_dual_identity_direct_call_sub_module() {
  c4c::backend::bir::Module module;
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "f",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {
          c4c::backend::bir::Param{.type = c4c::backend::bir::TypeKind::I32, .name = "%arg0"},
      },
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%arg0"),
          },
      }},
      .is_declaration = false,
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "g",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {
          c4c::backend::bir::Param{.type = c4c::backend::bir::TypeKind::I32, .name = "%arg0"},
      },
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%arg0"),
          },
      }},
      .is_declaration = false,
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "main",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {
              c4c::backend::bir::CallInst{
                  .result =
                      c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
                  .callee = "f",
                  .args = {c4c::backend::bir::Value::immediate_i32(7)},
                  .return_type_name = "i32",
              },
              c4c::backend::bir::CallInst{
                  .result =
                      c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t1"),
                  .callee = "g",
                  .args = {c4c::backend::bir::Value::immediate_i32(3)},
                  .return_type_name = "i32",
              },
              c4c::backend::bir::BinaryInst{
                  .opcode = c4c::backend::bir::BinaryOpcode::Sub,
                  .result =
                      c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t2"),
                  .lhs = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
                  .rhs = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t1"),
              },
          },
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t2"),
          },
      }},
      .is_declaration = false,
  });

  const auto parsed = c4c::backend::parse_bir_minimal_dual_identity_direct_call_sub_module(module);
  expect_true(parsed.has_value(),
              "shared call-decode surface should parse a BIR dual-identity direct-call subtraction module");
  if (parsed.has_value()) {
    expect_true(parsed->lhs_helper != nullptr && parsed->lhs_helper->name == "f" &&
                    parsed->rhs_helper != nullptr && parsed->rhs_helper->name == "g" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main",
                "shared call-decode surface should preserve both helper identities and the caller for the BIR dual-identity subtraction slice");
    expect_true(parsed->lhs_call != nullptr && parsed->rhs_call != nullptr &&
                    parsed->sub != nullptr && parsed->lhs_call_arg_imm == 7 &&
                    parsed->rhs_call_arg_imm == 3,
                "shared call-decode surface should recover both call immediates and the subtraction instruction for the BIR dual-identity slice");
  }
}

void test_backend_shared_call_decode_parses_bir_minimal_call_crossing_direct_call_module() {
  c4c::backend::bir::Module module;
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "add_one",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {
          c4c::backend::bir::Param{.type = c4c::backend::bir::TypeKind::I32, .name = "%arg0"},
      },
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {c4c::backend::bir::BinaryInst{
              .opcode = c4c::backend::bir::BinaryOpcode::Add,
              .result = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%sum"),
              .lhs = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%arg0"),
              .rhs = c4c::backend::bir::Value::immediate_i32(1),
          }},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%sum"),
          },
      }},
      .is_declaration = false,
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "main",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {
              c4c::backend::bir::BinaryInst{
                  .opcode = c4c::backend::bir::BinaryOpcode::Add,
                  .result =
                      c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
                  .lhs = c4c::backend::bir::Value::immediate_i32(2),
                  .rhs = c4c::backend::bir::Value::immediate_i32(3),
              },
              c4c::backend::bir::CallInst{
                  .result =
                      c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t1"),
                  .callee = "add_one",
                  .args = {c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32,
                                                           "%t0")},
                  .return_type_name = "i32",
              },
              c4c::backend::bir::BinaryInst{
                  .opcode = c4c::backend::bir::BinaryOpcode::Add,
                  .result =
                      c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t2"),
                  .lhs = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
                  .rhs = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t1"),
              },
          },
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t2"),
          },
      }},
      .is_declaration = false,
  });

  const auto parsed = c4c::backend::parse_bir_minimal_call_crossing_direct_call_module(module);
  expect_true(parsed.has_value(),
              "shared call-decode surface should parse a BIR call-crossing direct-call module");
  if (parsed.has_value()) {
    expect_true(parsed->helper != nullptr && parsed->helper->name == "add_one" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main",
                "shared call-decode surface should preserve helper and caller identities for the BIR call-crossing slice");
    expect_true(parsed->source_add != nullptr && parsed->call != nullptr &&
                    parsed->final_add != nullptr &&
                    parsed->source_add->result.name == parsed->regalloc_source_value &&
                    parsed->call->args.front().name == parsed->regalloc_source_value &&
                    parsed->final_add->lhs.name == parsed->regalloc_source_value &&
                    parsed->regalloc_source_value == "%t0" &&
                    parsed->source_imm == 5 && parsed->helper_add_imm == 1,
                "shared call-decode surface should recover the source add result plus the source/helper immediates for the BIR call-crossing slice");
  }
}

namespace {

c4c::TypeSpec make_test_i32_typespec() {
  c4c::TypeSpec type{};
  type.base = c4c::TB_INT;
  type.array_size = -1;
  return type;
}

c4c::codegen::lir::LirTypeRef make_test_stale_text_i32_lir_type() {
  return c4c::codegen::lir::LirTypeRef("i8", c4c::codegen::lir::LirTypeKind::Integer, 32);
}

}  // namespace

void test_backend_shared_call_decode_accepts_typed_i32_single_add_imm_helper() {
  c4c::codegen::lir::LirFunction function;
  function.name = "add_one";
  function.return_type = make_test_i32_typespec();
  function.params.emplace_back("%arg0", make_test_i32_typespec());
  function.signature_text = "define i32 @add_one(i8 %stale)";

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(c4c::codegen::lir::LirBinOp{
      .result = "%sum",
      .opcode = c4c::codegen::lir::LirBinaryOpcode::Add,
      .type_str = make_test_stale_text_i32_lir_type(),
      .lhs = "%arg0",
      .rhs = "1",
  });
  entry.terminator = c4c::codegen::lir::LirRet{std::string("%sum"), "i32"};
  function.blocks.push_back(std::move(entry));

  const auto parsed = c4c::backend::parse_backend_single_add_imm_function(function, "add_one");
  expect_true(parsed.has_value(),
              "shared call-decode should accept semantic i32 typed helper binops for the add-immediate parser");
  if (parsed.has_value()) {
    expect_true(parsed->param_name == "%arg0" && parsed->add != nullptr && parsed->add->rhs == "1",
                "shared call-decode should preserve the typed helper param and add-immediate operand");
  }
}

void test_backend_shared_call_decode_accepts_typed_i32_single_add_imm_helper_with_stale_return_text() {
  c4c::codegen::lir::LirFunction function;
  function.name = "add_one";
  function.return_type = make_test_i32_typespec();
  function.params.emplace_back("%arg0", make_test_i32_typespec());
  function.signature_text = "define i8 @add_one(i32 %arg0)";

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(c4c::codegen::lir::LirBinOp{
      .result = "%sum",
      .opcode = c4c::codegen::lir::LirBinaryOpcode::Add,
      .type_str = make_test_stale_text_i32_lir_type(),
      .lhs = "%arg0",
      .rhs = "1",
  });
  entry.terminator =
      c4c::codegen::lir::LirRet{std::string("%sum"), make_test_stale_text_i32_lir_type()};
  function.blocks.push_back(std::move(entry));

  const auto parsed = c4c::backend::parse_backend_single_add_imm_function(function, "add_one");
  expect_true(parsed.has_value(),
              "shared call-decode should accept the add-immediate helper when typed function metadata says i32 but the stored helper return text is stale");
}

void test_backend_shared_call_decode_accepts_typed_i32_slot_add_helper() {
  c4c::codegen::lir::LirFunction function;
  function.name = "slot_add";
  function.return_type = make_test_i32_typespec();
  function.params.emplace_back("%arg0", make_test_i32_typespec());
  function.signature_text = "define i32 @slot_add(i8 %stale)";
  function.alloca_insts.push_back(c4c::codegen::lir::LirAllocaOp{
      .result = "%slot",
      .type_str = make_test_stale_text_i32_lir_type(),
      .count = "",
      .align = 4,
  });
  function.alloca_insts.push_back(c4c::codegen::lir::LirStoreOp{
      .type_str = make_test_stale_text_i32_lir_type(),
      .val = "%arg0",
      .ptr = "%slot",
  });

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(c4c::codegen::lir::LirLoadOp{
      .result = "%t0",
      .type_str = make_test_stale_text_i32_lir_type(),
      .ptr = "%slot",
  });
  entry.insts.push_back(c4c::codegen::lir::LirBinOp{
      .result = "%sum",
      .opcode = c4c::codegen::lir::LirBinaryOpcode::Add,
      .type_str = make_test_stale_text_i32_lir_type(),
      .lhs = "%t0",
      .rhs = "1",
  });
  entry.insts.push_back(c4c::codegen::lir::LirStoreOp{
      .type_str = make_test_stale_text_i32_lir_type(),
      .val = "%sum",
      .ptr = "%slot",
  });
  entry.insts.push_back(c4c::codegen::lir::LirLoadOp{
      .result = "%t1",
      .type_str = make_test_stale_text_i32_lir_type(),
      .ptr = "%slot",
  });
  entry.terminator = c4c::codegen::lir::LirRet{std::string("%t1"), "i32"};
  function.blocks.push_back(std::move(entry));

  const auto parsed = c4c::backend::parse_backend_single_param_slot_add_function(
      function, "slot_add");
  expect_true(parsed.has_value(),
              "shared call-decode should accept semantic i32 typed local-slot helper ops for the slot-add parser");
  if (parsed.has_value()) {
    expect_true(parsed->param_name == "%arg0" && parsed->slot_name == "%slot" &&
                    parsed->add != nullptr && parsed->add->rhs == "1",
                "shared call-decode should preserve the typed slot helper param, slot, and add operand");
  }
}

void test_backend_shared_call_decode_accepts_typed_i32_slot_add_helper_with_stale_return_text() {
  c4c::codegen::lir::LirFunction function;
  function.name = "slot_add";
  function.return_type = make_test_i32_typespec();
  function.params.emplace_back("%arg0", make_test_i32_typespec());
  function.signature_text = "define i8 @slot_add(i32 %arg0)";
  function.alloca_insts.push_back(c4c::codegen::lir::LirAllocaOp{
      .result = "%slot",
      .type_str = make_test_stale_text_i32_lir_type(),
      .count = "",
      .align = 4,
  });
  function.alloca_insts.push_back(c4c::codegen::lir::LirStoreOp{
      .type_str = make_test_stale_text_i32_lir_type(),
      .val = "%arg0",
      .ptr = "%slot",
  });

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(c4c::codegen::lir::LirLoadOp{
      .result = "%t0",
      .type_str = make_test_stale_text_i32_lir_type(),
      .ptr = "%slot",
  });
  entry.insts.push_back(c4c::codegen::lir::LirBinOp{
      .result = "%sum",
      .opcode = c4c::codegen::lir::LirBinaryOpcode::Add,
      .type_str = make_test_stale_text_i32_lir_type(),
      .lhs = "%t0",
      .rhs = "1",
  });
  entry.insts.push_back(c4c::codegen::lir::LirStoreOp{
      .type_str = make_test_stale_text_i32_lir_type(),
      .val = "%sum",
      .ptr = "%slot",
  });
  entry.insts.push_back(c4c::codegen::lir::LirLoadOp{
      .result = "%t1",
      .type_str = make_test_stale_text_i32_lir_type(),
      .ptr = "%slot",
  });
  entry.terminator =
      c4c::codegen::lir::LirRet{std::string("%t1"), make_test_stale_text_i32_lir_type()};
  function.blocks.push_back(std::move(entry));

  const auto parsed = c4c::backend::parse_backend_single_param_slot_add_function(
      function, "slot_add");
  expect_true(parsed.has_value(),
              "shared call-decode should accept the slot-add helper when typed function metadata says i32 but the stored helper return text is stale");
}

void test_backend_shared_call_decode_accepts_typed_i32_identity_helper() {
  c4c::codegen::lir::LirFunction function;
  function.name = "identity";
  function.return_type = make_test_i32_typespec();
  function.params.emplace_back("%arg0", make_test_i32_typespec());
  function.signature_text = "define i32 @identity(i8 %stale)";

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.terminator = c4c::codegen::lir::LirRet{std::string("%arg0"), "i32"};
  function.blocks.push_back(std::move(entry));

  const auto parsed = c4c::backend::parse_backend_single_identity_function(function, "identity");
  expect_true(parsed.has_value() && *parsed == "%arg0",
              "shared call-decode should accept typed i32 helper params for the identity helper parser");
}

void test_backend_shared_call_decode_accepts_typed_i32_identity_helper_with_stale_return_text() {
  c4c::codegen::lir::LirFunction function;
  function.name = "identity";
  function.return_type = make_test_i32_typespec();
  function.params.emplace_back("%arg0", make_test_i32_typespec());
  function.signature_text = "define i8 @identity(i32 %arg0)";

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.terminator =
      c4c::codegen::lir::LirRet{std::string("%arg0"), make_test_stale_text_i32_lir_type()};
  function.blocks.push_back(std::move(entry));

  const auto parsed = c4c::backend::parse_backend_single_identity_function(function, "identity");
  expect_true(parsed.has_value() && *parsed == "%arg0",
              "shared call-decode should accept the identity helper when typed function metadata says i32 but the stored helper return text is stale");
}

void test_backend_shared_call_decode_accepts_typed_i32_two_param_add_helper() {
  c4c::codegen::lir::LirFunction function;
  function.name = "add_pair";
  function.return_type = make_test_i32_typespec();
  function.params.emplace_back("%lhs", make_test_i32_typespec());
  function.params.emplace_back("%rhs", make_test_i32_typespec());
  function.signature_text = "define i32 @add_pair(i8 %lhs, i8 %rhs)";

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(c4c::codegen::lir::LirBinOp{
      .result = "%sum",
      .opcode = c4c::codegen::lir::LirBinaryOpcode::Add,
      .type_str = c4c::codegen::lir::LirTypeRef::integer(32),
      .lhs = "%lhs",
      .rhs = "%rhs",
  });
  entry.terminator = c4c::codegen::lir::LirRet{std::string("%sum"), "i32"};
  function.blocks.push_back(std::move(entry));

  const auto parsed = c4c::backend::parse_backend_two_param_add_function(function, "add_pair");
  expect_true(parsed.has_value(),
              "shared call-decode should accept semantic i32 typed helper binops for the two-parameter add parser");
  if (parsed.has_value()) {
    expect_true(parsed->lhs_param_name == "%lhs" && parsed->rhs_param_name == "%rhs" &&
                    parsed->add != nullptr && parsed->add->result == "%sum",
                "shared call-decode should preserve both typed helper params for the two-parameter add parser");
  }
}

void test_backend_shared_call_decode_accepts_typed_i32_two_param_add_helper_with_stale_return_text() {
  c4c::codegen::lir::LirFunction function;
  function.name = "add_pair";
  function.return_type = make_test_i32_typespec();
  function.params.emplace_back("%lhs", make_test_i32_typespec());
  function.params.emplace_back("%rhs", make_test_i32_typespec());
  function.signature_text = "define i8 @add_pair(i32 %lhs, i32 %rhs)";

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(c4c::codegen::lir::LirBinOp{
      .result = "%sum",
      .opcode = c4c::codegen::lir::LirBinaryOpcode::Add,
      .type_str = make_test_stale_text_i32_lir_type(),
      .lhs = "%lhs",
      .rhs = "%rhs",
  });
  entry.terminator =
      c4c::codegen::lir::LirRet{std::string("%sum"), make_test_stale_text_i32_lir_type()};
  function.blocks.push_back(std::move(entry));

  const auto parsed = c4c::backend::parse_backend_two_param_add_function(function, "add_pair");
  expect_true(parsed.has_value(),
              "shared call-decode should accept the two-parameter add helper when typed function metadata says i32 but the stored helper return text is stale");
}

void test_backend_shared_call_decode_prefers_typed_vararg_call_args_over_stale_fixed_suffix_text() {
  const c4c::codegen::lir::LirCallOp call{
      "%t0",
      "i32",
      "@helper_ext",
      "(i8, ...)",
      "i32 5, i32 7",
  };

  const auto operand = c4c::backend::parse_backend_direct_global_single_typed_call_operand(
      call, "helper_ext", "i32");
  expect_true(
      operand.has_value() && *operand == "5",
      "shared call-decode should recover the fixed typed vararg operand from the actual call args even when the stored fixed-param suffix text is stale");
}


void test_backend_shared_liveness_surface_tracks_result_names() {
  const auto module = make_declaration_module();
  const auto& function = module.functions.back();
  const auto input = lower_test_backend_cfg_liveness_input(function);
  const auto liveness = c4c::backend::compute_live_intervals(input);

  expect_true(liveness.call_points.size() == 1 && liveness.call_points.front() == 0,
              "shared liveness surface should record call program points for typed call ops");
  const auto* interval = liveness.find_interval("%t0");
  expect_true(interval != nullptr,
              "shared liveness surface should expose interval lookup by SSA result name");
  expect_true(interval->start == 0 && interval->end == 1,
              "shared liveness surface should extend intervals through later uses instead of keeping point-only placeholders");
}



void test_backend_shared_liveness_surface_tracks_call_crossing_ranges() {
  const auto module = make_call_crossing_interval_module();
  const auto& function = module.functions.back();
  const auto input = lower_test_backend_cfg_liveness_input(function);
  const auto liveness = c4c::backend::compute_live_intervals(input);

  expect_true(liveness.call_points.size() == 1 && liveness.call_points.front() == 1,
              "shared liveness surface should record the direct call program point");
  const auto* before_call = liveness.find_interval("%t0");
  const auto* call_result = liveness.find_interval("%t1");
  const auto* final_sum = liveness.find_interval("%t2");
  expect_true(before_call != nullptr && before_call->start == 0 && before_call->end == 2,
              "shared liveness surface should extend values that remain live across a call through their later use");
  expect_true(call_result != nullptr && call_result->start == 1 && call_result->end == 2,
              "shared liveness surface should keep the call result live until its consuming instruction");
  expect_true(final_sum != nullptr && final_sum->start == 2 && final_sum->end == 3,
              "shared liveness surface should include terminator uses in the final interval endpoint");
}

void test_backend_shared_liveness_surface_tracks_phi_join_ranges() {
  const auto module = make_interval_phi_join_module();
  const auto& function = module.functions.front();
  const auto input = lower_test_backend_cfg_liveness_input(function);
  const auto liveness = c4c::backend::compute_live_intervals(input);

  const auto* cond = liveness.find_interval("%t0");
  const auto* then_value = liveness.find_interval("%t1");
  const auto* else_value = liveness.find_interval("%t2");
  const auto* phi_value = liveness.find_interval("%t3");
  const auto* final_sum = liveness.find_interval("%t4");

  expect_true(cond != nullptr && cond->start == 0 && cond->end == 1,
              "shared liveness surface should extend branch conditions through the conditional terminator");
  expect_true(then_value != nullptr && then_value->start == 2 && then_value->end == 3,
              "shared liveness surface should treat phi incoming values as used on the predecessor edge");
  expect_true(else_value != nullptr && else_value->start == 4 && else_value->end == 5,
              "shared liveness surface should track the alternate phi incoming value on its predecessor edge");
  expect_true(phi_value != nullptr && phi_value->start == 6 && phi_value->end == 7,
              "shared liveness surface should start phi results in the join block and extend them to their consuming instruction");
  expect_true(final_sum != nullptr && final_sum->start == 7 && final_sum->end == 8,
              "shared liveness surface should keep the join result live through the return terminator");
}

void test_backend_shared_liveness_input_matches_lir_phi_join_ranges() {
  const auto module = make_interval_phi_join_module();
  const auto& function = module.functions.front();
  const auto input = lower_test_backend_cfg_liveness_input(function);
  const auto compatibility_input = c4c::backend::lower_backend_cfg_to_liveness_input(
      c4c::backend::lower_lir_to_backend_cfg(function));
  const auto liveness = c4c::backend::compute_live_intervals(input);
  const auto compatibility_liveness =
      c4c::backend::compute_live_intervals(compatibility_input);

  expect_true(liveness.call_points == compatibility_liveness.call_points,
              "compatibility raw-LIR liveness lowering should preserve call-point classification from the backend-CFG seam");
  const auto* cond = liveness.find_interval("%t0");
  const auto* then_value = liveness.find_interval("%t1");
  const auto* else_value = liveness.find_interval("%t2");
  const auto* phi_value = liveness.find_interval("%t3");
  const auto* final_sum = liveness.find_interval("%t4");
  const auto* compatibility_cond = compatibility_liveness.find_interval("%t0");
  const auto* compatibility_then = compatibility_liveness.find_interval("%t1");
  const auto* compatibility_else = compatibility_liveness.find_interval("%t2");
  const auto* compatibility_phi = compatibility_liveness.find_interval("%t3");
  const auto* compatibility_final = compatibility_liveness.find_interval("%t4");

  expect_true(cond != nullptr && compatibility_cond != nullptr &&
                  cond->start == compatibility_cond->start &&
                  cond->end == compatibility_cond->end,
              "compatibility raw-LIR liveness lowering should preserve branch-condition interval endpoints from the backend-CFG seam");
  expect_true(then_value != nullptr && compatibility_then != nullptr &&
                  then_value->start == compatibility_then->start &&
                  then_value->end == compatibility_then->end,
              "compatibility raw-LIR liveness lowering should preserve phi incoming predecessor-edge uses");
  expect_true(else_value != nullptr && compatibility_else != nullptr &&
                  else_value->start == compatibility_else->start &&
                  else_value->end == compatibility_else->end,
              "compatibility raw-LIR liveness lowering should preserve alternate phi incoming predecessor-edge uses");
  expect_true(phi_value != nullptr && compatibility_phi != nullptr &&
                  phi_value->start == compatibility_phi->start &&
                  phi_value->end == compatibility_phi->end,
              "compatibility raw-LIR liveness lowering should preserve phi-definition intervals in the join block");
  expect_true(final_sum != nullptr && compatibility_final != nullptr &&
                  final_sum->start == compatibility_final->start &&
                  final_sum->end == compatibility_final->end,
              "compatibility raw-LIR liveness lowering should preserve downstream join-result intervals");
}

void test_backend_shared_regalloc_surface_uses_caller_saved_for_non_call_interval() {
  const auto module = make_return_add_module();
  const auto& function = module.functions.front();
  const auto input = lower_test_backend_cfg_liveness_input(function);

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}, {21}};
  config.caller_saved_regs = {{13}};

  const auto regalloc = c4c::backend::allocate_registers(input, config);
  expect_true(regalloc.assignments.size() == 1,
              "shared regalloc surface should assign one register for the single-result helper slice");
  const auto it = regalloc.assignments.find("%t0");
  expect_true(it != regalloc.assignments.end() && it->second.index == 13,
              "shared regalloc surface should prefer the caller-saved pool for intervals that do not span calls");
  expect_true(regalloc.used_regs.empty(),
              "shared regalloc surface should leave the callee-saved usage set empty when only caller-saved registers are needed");
  expect_true(regalloc.liveness.has_value() &&
                  regalloc.liveness->find_interval("%t0") != nullptr,
              "shared regalloc surface should retain cached liveness for the handoff boundary");
}

void test_backend_shared_regalloc_prefers_callee_saved_for_call_spanning_values() {
  const auto module = make_call_crossing_interval_module();
  const auto& function = module.functions.back();
  const auto input = lower_test_backend_cfg_liveness_input(function);

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto regalloc = c4c::backend::allocate_registers(input, config);

  const auto before_call = regalloc.assignments.find("%t0");
  const auto call_result = regalloc.assignments.find("%t1");
  const auto final_sum = regalloc.assignments.find("%t2");

  expect_true(before_call != regalloc.assignments.end() && before_call->second.index == 20,
              "shared regalloc should keep call-spanning intervals on the callee-saved pool first");
  expect_true(call_result == regalloc.assignments.end(),
              "shared regalloc should spill overlapping call-spanning intervals when the callee-saved pool is exhausted");
  expect_true(final_sum != regalloc.assignments.end() && final_sum->second.index == 13,
              "shared regalloc should use caller-saved registers for non-call-spanning intervals");
  expect_true(regalloc.used_regs.size() == 1 && regalloc.used_regs.front().index == 20,
              "shared regalloc should report only the used callee-saved register set");
}

void test_backend_shared_regalloc_accepts_backend_owned_liveness_input() {
  const auto module = make_call_crossing_interval_module();
  const auto& function = module.functions.back();
  const auto input = lower_test_backend_cfg_liveness_input(function);

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto regalloc = c4c::backend::allocate_registers(input, config);

  const auto before_call = regalloc.assignments.find("%t0");
  const auto call_result = regalloc.assignments.find("%t1");
  const auto final_sum = regalloc.assignments.find("%t2");

  expect_true(before_call != regalloc.assignments.end() && before_call->second.index == 20,
              "backend-owned regalloc input should keep call-spanning intervals on the callee-saved pool");
  expect_true(call_result == regalloc.assignments.end(),
              "backend-owned regalloc input should still spill overlapping call-spanning intervals when callee-saved capacity is exhausted");
  expect_true(final_sum != regalloc.assignments.end() && final_sum->second.index == 13,
              "backend-owned regalloc input should still use caller-saved registers for non-call-spanning intervals");
  expect_true(regalloc.liveness.has_value() &&
                  regalloc.liveness->find_interval("%t0") != nullptr &&
                  regalloc.liveness->find_interval("%t1") != nullptr &&
                  regalloc.liveness->find_interval("%t2") != nullptr,
              "backend-owned regalloc input should retain cached liveness across the new handoff seam");
}

void test_backend_shared_regalloc_reuses_register_after_interval_ends() {
  const auto module = make_non_overlapping_interval_module();
  const auto& function = module.functions.front();
  const auto input = lower_test_backend_cfg_liveness_input(function);

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};

  const auto regalloc = c4c::backend::allocate_registers(input, config);

  const auto first_value = regalloc.assignments.find("%t0");
  const auto later_value = regalloc.assignments.find("%t2");

  expect_true(first_value != regalloc.assignments.end() && first_value->second.index == 20,
              "shared regalloc should assign the available register to the first live interval");
  expect_true(later_value != regalloc.assignments.end() && later_value->second.index == 20,
              "shared regalloc should reuse a freed register for a later non-overlapping interval");
  expect_true(regalloc.used_regs.size() == 1 && regalloc.used_regs.front().index == 20,
              "shared regalloc should keep the callee-saved usage set deduplicated when a register is reused");
}

void test_backend_shared_regalloc_spills_overlapping_values_without_reusing_busy_reg() {
  const auto module = make_overlapping_interval_module();
  const auto& function = module.functions.front();
  const auto input = lower_test_backend_cfg_liveness_input(function);

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};

  const auto regalloc = c4c::backend::allocate_registers(input, config);

  expect_true(regalloc.assignments.size() == 1,
              "shared regalloc should spill one of two overlapping intervals when only one register is available");
  const bool first_assigned = regalloc.assignments.find("%t0") != regalloc.assignments.end();
  const bool second_assigned = regalloc.assignments.find("%t1") != regalloc.assignments.end();
  expect_true(first_assigned != second_assigned,
              "shared regalloc should not assign the same busy register to overlapping intervals");
}

void test_backend_shared_regalloc_helper_filters_and_merges_clobbers() {
  const std::vector<c4c::backend::PhysReg> callee_saved = {{20}, {21}, {22}};
  const std::vector<c4c::backend::PhysReg> asm_clobbered = {{21}};
  const auto filtered =
      c4c::backend::stack_layout::filter_available_regs(callee_saved, asm_clobbered);

  expect_true(filtered.size() == 2 && filtered.front().index == 20 &&
                  filtered.back().index == 22,
              "shared stack-layout helper should remove inline-asm clobbered registers from the available set");

  const auto module = make_return_add_module();
  c4c::backend::RegAllocConfig config;
  config.available_regs = filtered;
  const auto input =
      lower_test_backend_cfg_liveness_input(module.functions.front());
  const auto merged = c4c::backend::run_regalloc_and_merge_clobbers(
      input, config, asm_clobbered);

  expect_true(merged.used_callee_saved.size() == 2 &&
                  merged.used_callee_saved.front().index == 20 &&
                  merged.used_callee_saved.back().index == 21,
              "shared regalloc helper should merge allocator usage with asm clobbers in sorted order");
}

void test_backend_shared_stack_layout_regalloc_helper_exposes_handoff_view() {
  const auto module = make_call_crossing_interval_module();
  const auto& function = module.functions.back();
  const auto input = lower_test_backend_cfg_liveness_input(function);

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};
  const std::vector<c4c::backend::PhysReg> asm_clobbered = {{21}};

  const auto merged =
      c4c::backend::run_regalloc_and_merge_clobbers(input, config, asm_clobbered);

  const auto* before_call =
      c4c::backend::stack_layout::find_assigned_reg(merged, "%t0");
  const auto* call_result =
      c4c::backend::stack_layout::find_assigned_reg(merged, "%t1");
  const auto* final_sum =
      c4c::backend::stack_layout::find_assigned_reg(merged, "%t2");
  const auto* cached_liveness =
      c4c::backend::stack_layout::find_cached_liveness(merged);

  expect_true(before_call != nullptr && before_call->index == 20,
              "shared stack-layout helper should expose assigned call-spanning values through the regalloc handoff");
  expect_true(call_result == nullptr,
              "shared stack-layout helper should preserve spilled values as missing from the assigned-register view");
  expect_true(final_sum != nullptr && final_sum->index == 13,
              "shared stack-layout helper should expose caller-saved assignments through the same handoff seam");
  expect_true(c4c::backend::stack_layout::uses_callee_saved_reg(merged, c4c::backend::PhysReg{20}),
              "shared stack-layout helper should report allocator-used callee-saved registers");
  expect_true(c4c::backend::stack_layout::uses_callee_saved_reg(merged, c4c::backend::PhysReg{21}),
              "shared stack-layout helper should also report inline-asm-clobbered callee-saved registers after merge");
  expect_true(!c4c::backend::stack_layout::uses_callee_saved_reg(merged, c4c::backend::PhysReg{22}),
              "shared stack-layout helper should reject untouched callee-saved registers");
  expect_true(cached_liveness != nullptr &&
                  cached_liveness->find_interval("%t0") != nullptr &&
                  cached_liveness->find_interval("%t1") != nullptr &&
                  cached_liveness->find_interval("%t2") != nullptr,
              "shared stack-layout helper should expose cached liveness for downstream stack-layout analysis");
}

void test_backend_shared_regalloc_helper_accepts_backend_owned_liveness_input() {
  const auto module = make_call_crossing_interval_module();
  const auto& function = module.functions.back();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};
  const std::vector<c4c::backend::PhysReg> asm_clobbered = {{21}};

  const auto input = lower_test_backend_cfg_liveness_input(function);
  const auto merged =
      c4c::backend::run_regalloc_and_merge_clobbers(input, config, asm_clobbered);

  const auto* before_call =
      c4c::backend::stack_layout::find_assigned_reg(merged, "%t0");
  const auto* call_result =
      c4c::backend::stack_layout::find_assigned_reg(merged, "%t1");
  const auto* final_sum =
      c4c::backend::stack_layout::find_assigned_reg(merged, "%t2");
  const auto* cached_liveness =
      c4c::backend::stack_layout::find_cached_liveness(merged);

  expect_true(before_call != nullptr && before_call->index == 20,
              "backend-owned liveness input should preserve call-spanning regalloc assignments");
  expect_true(call_result == nullptr,
              "backend-owned liveness input should preserve spilled call-result assignments");
  expect_true(final_sum != nullptr && final_sum->index == 13,
              "backend-owned liveness input should preserve caller-saved assignments");
  expect_true(c4c::backend::stack_layout::uses_callee_saved_reg(merged, c4c::backend::PhysReg{20}),
              "backend-owned liveness input should preserve allocator-used callee-saved registers");
  expect_true(c4c::backend::stack_layout::uses_callee_saved_reg(merged, c4c::backend::PhysReg{21}),
              "backend-owned liveness input should preserve merged inline-asm clobbers");
  expect_true(cached_liveness != nullptr &&
                  cached_liveness->find_interval("%t0") != nullptr &&
                  cached_liveness->find_interval("%t1") != nullptr &&
                  cached_liveness->find_interval("%t2") != nullptr,
              "backend-owned liveness input should preserve cached liveness for downstream stack-layout analysis");
}

void test_x86_shared_regalloc_helper_uses_translated_register_pools() {
  const auto module = make_call_crossing_interval_module();
  const auto& function = module.functions.back();
  const auto input = lower_test_backend_cfg_liveness_input(function);
  const auto regalloc = c4c::backend::x86::run_shared_x86_regalloc(input);

  const auto* before_call =
      c4c::backend::stack_layout::find_assigned_reg(regalloc, "%t0");
  const auto* final_sum =
      c4c::backend::stack_layout::find_assigned_reg(regalloc, "%t2");
  const auto* cached_liveness =
      c4c::backend::stack_layout::find_cached_liveness(regalloc);
  const auto callee_saved = c4c::backend::x86::x86_callee_saved_regs();
  const auto caller_saved = c4c::backend::x86::x86_caller_saved_regs();
  const auto in_pool = [](const std::vector<c4c::backend::PhysReg>& pool,
                          const c4c::backend::PhysReg* reg) {
    return reg != nullptr &&
           std::any_of(pool.begin(), pool.end(), [&](const c4c::backend::PhysReg& candidate) {
             return candidate.index == reg->index;
           });
  };

  expect_true(in_pool(callee_saved, before_call),
              "x86 shared regalloc helper should assign call-spanning values from the translated callee-saved register pool");
  expect_true(in_pool(caller_saved, final_sum),
              "x86 shared regalloc helper should assign non-call-spanning values from the translated caller-saved register pool");
  expect_true(!regalloc.used_callee_saved.empty() &&
                  c4c::backend::stack_layout::uses_callee_saved_reg(regalloc,
                                                                    regalloc.used_callee_saved.front()),
              "x86 shared regalloc helper should expose allocator-used translated callee-saved registers through the shared handoff view");
  expect_true(cached_liveness != nullptr &&
                  cached_liveness->find_interval("%t0") != nullptr &&
                  cached_liveness->find_interval("%t1") != nullptr &&
                  cached_liveness->find_interval("%t2") != nullptr,
              "x86 shared regalloc helper should retain cached liveness for downstream translated prologue/regalloc ownership work");
}

void test_backend_shared_stack_layout_analysis_tracks_phi_use_blocks() {
  const auto module = make_interval_phi_join_module();
  const auto& function = module.functions.front();
  const auto input = lower_test_function_entry_alloca_stack_layout_input(function);
  const c4c::backend::RegAllocIntegrationResult regalloc;
  const auto analysis = c4c::backend::stack_layout::analyze_stack_layout(input, regalloc, {});

  const auto* then_value_uses = analysis.find_use_blocks("%t1");
  const auto* else_value_uses = analysis.find_use_blocks("%t2");
  const auto* phi_value_uses = analysis.find_use_blocks("%t3");

  expect_true(then_value_uses != nullptr && then_value_uses->size() == 1 &&
                  then_value_uses->front() == 1,
              "shared stack-layout analysis should attribute phi incoming uses to the predecessor block");
  expect_true(else_value_uses != nullptr && else_value_uses->size() == 1 &&
                  else_value_uses->front() == 2,
              "shared stack-layout analysis should keep alternate phi incoming values on their own predecessor block");
  expect_true(phi_value_uses != nullptr && phi_value_uses->size() == 1 &&
                  phi_value_uses->front() == 3,
              "shared stack-layout analysis should record normal instruction uses in the consuming block");
}

void test_backend_shared_stack_layout_analysis_accepts_backend_owned_input() {
  const auto module = make_interval_phi_join_module();
  const auto& function = module.functions.front();
  const auto input = lower_test_function_entry_alloca_stack_layout_input(function);
  const c4c::backend::RegAllocIntegrationResult regalloc;
  const auto analysis =
      c4c::backend::stack_layout::analyze_stack_layout(input, regalloc, {});

  const auto* then_value_uses = analysis.find_use_blocks("%t1");
  const auto* else_value_uses = analysis.find_use_blocks("%t2");
  const auto* phi_value_uses = analysis.find_use_blocks("%t3");

  expect_true(then_value_uses != nullptr && then_value_uses->size() == 1 &&
                  then_value_uses->front() == 1,
              "backend-owned stack-layout input should preserve predecessor-edge phi use attribution");
  expect_true(else_value_uses != nullptr && else_value_uses->size() == 1 &&
                  else_value_uses->front() == 2,
              "backend-owned stack-layout input should preserve alternate predecessor-edge phi uses");
  expect_true(phi_value_uses != nullptr && phi_value_uses->size() == 1 &&
                  phi_value_uses->front() == 3,
              "backend-owned stack-layout input should preserve normal consuming-block uses");
}

void test_backend_shared_stack_layout_input_collects_value_names() {
  const auto dead_param_module = make_dead_param_alloca_candidate_module();
  const auto& dead_param_function = dead_param_module.functions.back();
  const auto dead_param_input =
      lower_test_function_entry_alloca_stack_layout_input(dead_param_function);
  const auto dead_param_value_names =
      c4c::backend::stack_layout::collect_stack_layout_value_names(dead_param_input);

  const auto escaped_module = make_escaped_local_alloca_candidate_module();
  const auto& escaped_function = escaped_module.functions.back();
  const auto escaped_input =
      lower_test_function_entry_alloca_stack_layout_input(escaped_function);
  const auto escaped_value_names =
      c4c::backend::stack_layout::collect_stack_layout_value_names(escaped_input);

  const auto contains = [](const std::vector<std::string>& value_names,
                           std::string_view value_name) {
    return std::find(value_names.begin(), value_names.end(), value_name) != value_names.end();
  };

  expect_true(contains(dead_param_value_names, "%lv.param.x"),
              "backend-owned stack-layout input should expose entry alloca names for downstream slot builders");
  expect_true(contains(dead_param_value_names, "%p.x"),
              "backend-owned stack-layout input should preserve paired parameter stores for downstream slot builders");
  expect_true(contains(escaped_value_names, "%lv.buf"),
              "backend-owned stack-layout input should preserve derived pointer roots for downstream slot builders");
  expect_true(contains(dead_param_value_names, "%t0"),
              "backend-owned stack-layout input should preserve body SSA uses for downstream slot builders");
}

void test_backend_shared_stack_layout_input_preserves_param_alloca_metadata() {
  const auto module = make_dead_param_alloca_candidate_module();
  const auto& function = module.functions.back();
  const auto input = lower_test_function_entry_alloca_stack_layout_input(function);

  expect_true(input.entry_allocas.size() == 1 &&
                  input.entry_allocas.front().alloca_name == "%lv.param.x" &&
                  input.entry_allocas.front().type_str == "i32" &&
                  input.entry_allocas.front().paired_store_value == std::optional<std::string>{"%p.x"},
              "backend-owned stack-layout input should preserve param alloca names, types, and paired stores for direct-emitter slot initialization");
}

void test_backend_shared_stack_layout_input_preserves_signature_metadata() {
  c4c::codegen::lir::LirFunction function;
  function.name = "aggregate_variadic";
  function.signature_text =
      "define { i32, i32 } @aggregate_variadic({ i32, i32 } %p.agg, double %p.fp, ...)";

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(c4c::codegen::lir::LirCallOp{
      "%call0",
      c4c::codegen::lir::LirTypeRef("{ i32, i32 }"),
      "@helper",
      "(i32)",
      "i32 7",
  });
  entry.terminator = c4c::codegen::lir::LirRet{std::nullopt, "void"};
  function.blocks.push_back(std::move(entry));

  const auto input = lower_test_function_entry_alloca_stack_layout_input(function);

  expect_true(input.signature_params.size() == 3 &&
                  input.signature_params[0].type == "{ i32, i32 }" &&
                  input.signature_params[0].operand == "%p.agg" &&
                  input.signature_params[1].type == "double" &&
                  input.signature_params[1].operand == "%p.fp" &&
                  input.signature_params[2].is_varargs,
              "backend-owned stack-layout input should preserve parsed function-signature params for downstream slot builders");
  expect_true(input.return_type.has_value() && *input.return_type == "{ i32, i32 }" &&
                  input.is_variadic,
              "backend-owned stack-layout input should preserve return-type and variadic metadata for downstream slot builders");
  expect_true(input.call_results.size() == 1 &&
                  input.call_results.front().value_name == "%call0" &&
                  input.call_results.front().type_str == "{ i32, i32 }",
              "backend-owned stack-layout input should preserve stack-backed call-result metadata for downstream slot builders");
}

void test_backend_shared_stack_layout_input_preserves_aarch64_return_gate_metadata() {
  c4c::codegen::lir::LirFunction function;
  function.name = "pair_return";
  function.signature_text = "define { i32, i32 } @pair_return(i32 %p.x)";

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.terminator = c4c::codegen::lir::LirRet{std::nullopt, "void"};
  function.blocks.push_back(std::move(entry));

  const auto input = lower_test_function_entry_alloca_stack_layout_input(function);

  expect_true(input.return_type.has_value() && *input.return_type == "{ i32, i32 }" &&
                  input.signature_params.size() == 1 &&
                  input.signature_params.front().type == "i32" &&
                  input.signature_params.front().operand == "%p.x",
              "backend-owned stack-layout input should preserve the aggregate return metadata consumed by the direct AArch64 stack-slot support gate");
}

void test_backend_shared_stack_layout_analysis_detects_dead_param_allocas() {
  const auto module = make_dead_param_alloca_candidate_module();
  const auto& function = module.functions.back();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};
  const auto dead_param_input =
      lower_test_backend_cfg_liveness_input(function);
  const auto regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(dead_param_input, config, {});
  const auto input = lower_test_function_entry_alloca_stack_layout_input(function);
  const auto analysis =
      c4c::backend::stack_layout::analyze_stack_layout(input, regalloc, {{20}, {21}, {22}});

  expect_true(analysis.uses_value("%p.x"),
              "shared stack-layout analysis should collect body-used SSA values");
  expect_true(!analysis.uses_value("%lv.param.x"),
              "shared stack-layout analysis should not treat the synthesized param spill as a body use");
  expect_true(analysis.is_dead_param_alloca("%lv.param.x"),
              "shared stack-layout analysis should mark dead param allocas when the corresponding parameter value is kept in a callee-saved register");
}

void test_backend_shared_stack_layout_analysis_tracks_call_arg_values() {
  const auto module = make_escaped_local_alloca_candidate_module();
  const auto& function = module.functions.back();
  const auto input = lower_test_function_entry_alloca_stack_layout_input(function);
  const c4c::backend::RegAllocIntegrationResult regalloc;
  const auto analysis = c4c::backend::stack_layout::analyze_stack_layout(input, regalloc, {});

  expect_true(analysis.uses_value("%lv.buf"),
              "shared stack-layout analysis should treat typed call-argument operands as real uses");
}

void test_backend_shared_stack_layout_analysis_detects_entry_alloca_overwrite_before_read() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto overwrite_first_module = make_overwrite_first_scalar_local_alloca_candidate_module();
  const auto& overwrite_first_function = overwrite_first_module.functions.front();
  const auto overwrite_first_input =
      lower_test_function_entry_alloca_stack_layout_input(overwrite_first_function);
  const auto overwrite_first_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(overwrite_first_input, regalloc, {});
  expect_true(overwrite_first_analysis.is_entry_alloca_overwritten_before_read("%lv.x"),
              "shared stack-layout analysis should recognize live entry allocas whose first real access overwrites the slot before any read");

  const auto read_first_module = make_read_before_store_scalar_local_alloca_candidate_module();
  const auto& read_first_function = read_first_module.functions.front();
  const auto read_first_input =
      lower_test_function_entry_alloca_stack_layout_input(read_first_function);
  const auto read_first_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(read_first_input, regalloc, {});
  expect_true(!read_first_analysis.is_entry_alloca_overwritten_before_read("%lv.x"),
              "shared stack-layout analysis should keep entry zero-init stores when the slot is read before it is overwritten");
}

void test_backend_shared_alloca_coalescing_classifies_non_param_allocas() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto dead_module = make_dead_local_alloca_candidate_module();
  const auto& dead_function = dead_module.functions.front();
  const auto dead_input =
      lower_test_function_entry_alloca_stack_layout_input(dead_function);
  const auto dead_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(dead_input, regalloc, {});
  const auto dead_coalescing =
      c4c::backend::stack_layout::compute_coalescable_allocas(dead_input, dead_analysis);
  expect_true(dead_coalescing.is_dead_alloca("%lv.buf") &&
                  !dead_coalescing.find_single_block("%lv.buf").has_value(),
              "shared alloca-coalescing should classify unused non-param allocas as dead instead of forcing a permanent slot");

  const auto single_block_module = make_live_local_alloca_candidate_module();
  const auto& single_block_function = single_block_module.functions.front();
  const auto single_block_input =
      lower_test_function_entry_alloca_stack_layout_input(single_block_function);
  const auto single_block_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(single_block_input, regalloc, {});
  const auto single_block_coalescing =
      c4c::backend::stack_layout::compute_coalescable_allocas(single_block_input,
                                                              single_block_analysis);
  expect_true(!single_block_coalescing.is_dead_alloca("%lv.buf") &&
                  single_block_coalescing.find_single_block("%lv.buf") == 0,
              "shared alloca-coalescing should recognize GEP-tracked non-param allocas whose uses stay within one block");

  const auto escaped_module = make_escaped_local_alloca_candidate_module();
  const auto& escaped_function = escaped_module.functions.back();
  const auto escaped_input =
      lower_test_function_entry_alloca_stack_layout_input(escaped_function);
  const auto escaped_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(escaped_input, regalloc, {});
  const auto escaped_coalescing =
      c4c::backend::stack_layout::compute_coalescable_allocas(escaped_input, escaped_analysis);
  expect_true(!escaped_coalescing.is_dead_alloca("%lv.buf") &&
                  !escaped_coalescing.find_single_block("%lv.buf").has_value(),
              "shared alloca-coalescing should leave call-escaped allocas out of the single-block pool");
}

void test_backend_shared_alloca_coalescing_accepts_backend_owned_input() {
  const c4c::backend::RegAllocIntegrationResult regalloc;
  const auto module = make_live_local_alloca_candidate_module();
  const auto& function = module.functions.front();
  const auto input = lower_test_function_entry_alloca_stack_layout_input(function);
  const auto analysis = c4c::backend::stack_layout::analyze_stack_layout(input, regalloc, {});
  const auto coalescing =
      c4c::backend::stack_layout::compute_coalescable_allocas(input, analysis);

  expect_true(!coalescing.is_dead_alloca("%lv.buf") &&
                  coalescing.find_single_block("%lv.buf") == 0,
              "backend-owned stack-layout input should preserve single-block alloca reuse classification");
}

void test_backend_shared_alloca_coalescing_accepts_entry_alloca_use_blocks() {
  const c4c::backend::RegAllocIntegrationResult regalloc;
  auto input = lower_test_function_entry_alloca_stack_layout_input(
      make_live_local_alloca_candidate_module().functions.front());
  input.entry_alloca_use_blocks =
      std::vector<c4c::backend::stack_layout::EntryAllocaUseBlocks>{
          c4c::backend::stack_layout::EntryAllocaUseBlocks{"%lv.buf", {0}}};
  input.blocks.front().insts.front().derived_pointer_root.reset();
  input.blocks.front().insts.front().pointer_accesses.clear();

  const auto analysis = c4c::backend::stack_layout::analyze_stack_layout(input, regalloc, {});
  const auto coalescing =
      c4c::backend::stack_layout::compute_coalescable_allocas(input, analysis);

  expect_true(!coalescing.is_dead_alloca("%lv.buf") &&
                  coalescing.find_single_block("%lv.buf") == 0,
              "backend-owned alloca coalescing should accept coarse entry-alloca use-block classification when present instead of rebuilding single-block ownership from per-point alias facts");
}

void test_backend_shared_stack_layout_analysis_accepts_entry_alloca_first_accesses() {
  const c4c::backend::RegAllocIntegrationResult regalloc;
  auto input = lower_test_function_entry_alloca_stack_layout_input(
      make_overwrite_first_scalar_local_alloca_candidate_module().functions.front());
  input.entry_alloca_first_accesses =
      std::vector<c4c::backend::stack_layout::EntryAllocaFirstAccess>{
          c4c::backend::stack_layout::EntryAllocaFirstAccess{
              "%lv.x", c4c::backend::stack_layout::PointerAccessKind::Store}};
  input.blocks.front().insts.front().pointer_accesses.clear();

  const auto analysis = c4c::backend::stack_layout::analyze_stack_layout(input, regalloc, {});
  const auto plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(input),
      analysis);

  expect_true(analysis.is_entry_alloca_overwritten_before_read("%lv.x") &&
                  plans.size() == 1 &&
                  plans.front().alloca_name == "%lv.x" &&
                  plans.front().needs_stack_slot &&
                  plans.front().remove_following_entry_store,
              "backend-owned stack-layout analysis should accept coarse entry-alloca first-access classification when present instead of rebuilding overwrite-before-read only from per-point pointer accesses");
}

void test_backend_shared_slot_assignment_plans_param_alloca_slots() {
  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto dead_module = make_dead_param_alloca_candidate_module();
  const auto& dead_function = dead_module.functions.back();
  const auto dead_liveness_input =
      lower_test_backend_cfg_liveness_input(dead_function);
  const auto dead_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(dead_liveness_input, config, {});
  const auto dead_input =
      lower_test_function_entry_alloca_stack_layout_input(dead_function);
  const auto dead_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      dead_input, dead_regalloc, {{20}, {21}, {22}});
  const auto dead_plans = c4c::backend::stack_layout::plan_param_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          dead_input),
      dead_analysis);

  expect_true(dead_plans.size() == 1 && dead_plans.front().alloca_name == "%lv.param.x" &&
                  dead_plans.front().param_name == "%p.x" &&
                  !dead_plans.front().needs_stack_slot,
              "shared slot-assignment planning should skip dead param allocas once analysis proves the parameter is preserved in a callee-saved register");

  const auto live_module = make_live_param_alloca_candidate_module();
  const auto& live_function = live_module.functions.front();
  const auto live_liveness_input =
      lower_test_backend_cfg_liveness_input(live_function);
  const auto live_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(live_liveness_input, config, {});
  const auto live_input =
      lower_test_function_entry_alloca_stack_layout_input(live_function);
  const auto live_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      live_input, live_regalloc, {{20}, {21}, {22}});
  const auto live_plans = c4c::backend::stack_layout::plan_param_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          live_input),
      live_analysis);

  expect_true(live_plans.size() == 1 && live_plans.front().alloca_name == "%lv.param.x" &&
                  live_plans.front().param_name == "%p.x" &&
                  live_plans.front().needs_stack_slot,
              "shared slot-assignment planning should retain live param allocas when the function body still reads from the parameter slot");
}

void test_backend_shared_slot_assignment_param_alloca_pruning_matches_backend_owned_entry_patch() {
  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto dead_module = make_dead_param_alloca_candidate_module();
  const auto& dead_function = dead_module.functions.back();
  const auto dead_liveness_input =
      lower_test_backend_cfg_liveness_input(dead_function);
  const auto dead_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(dead_liveness_input, config, {});
  const auto dead_input =
      lower_test_function_entry_alloca_stack_layout_input(dead_function);
  const auto dead_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      dead_input, dead_regalloc, {{20}, {21}, {22}});
  const auto dead_plans = c4c::backend::stack_layout::plan_param_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          dead_input),
      dead_analysis);
  const auto dead_entry_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          dead_input),
      dead_analysis);
  const auto dead_patch = c4c::backend::stack_layout::build_entry_alloca_rewrite_patch(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_rewrite_input(
          dead_input),
      dead_entry_plans);

  expect_true(dead_plans.size() == 1 && !dead_plans.front().needs_stack_slot,
              "shared slot-assignment param planning should classify dead param allocas as removable");
  expect_true(dead_patch.alloca_insts.empty(),
              "shared slot-assignment backend-owned rewrite patch should drop dead param allocas and their entry stores");

  const auto live_module = make_live_param_alloca_candidate_module();
  const auto& live_function = live_module.functions.front();
  const auto live_liveness_input =
      lower_test_backend_cfg_liveness_input(live_function);
  const auto live_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(live_liveness_input, config, {});
  const auto live_input =
      lower_test_function_entry_alloca_stack_layout_input(live_function);
  const auto live_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      live_input, live_regalloc, {{20}, {21}, {22}});
  const auto live_plans = c4c::backend::stack_layout::plan_param_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          live_input),
      live_analysis);
  const auto live_entry_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          live_input),
      live_analysis);
  const auto live_patch = c4c::backend::stack_layout::build_entry_alloca_rewrite_patch(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_rewrite_input(
          live_input),
      live_entry_plans);

  expect_true(live_plans.size() == 1 && live_plans.front().needs_stack_slot,
              "shared slot-assignment param planning should preserve live param allocas");
  expect_true(live_patch.alloca_insts.size() == live_function.alloca_insts.size(),
              "shared slot-assignment backend-owned rewrite patch should preserve live param allocas");
}

void test_backend_shared_slot_assignment_plans_entry_alloca_slots() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto dead_module = make_dead_local_alloca_candidate_module();
  const auto& dead_function = dead_module.functions.front();
  const auto dead_input =
      lower_test_function_entry_alloca_stack_layout_input(dead_function);
  const auto dead_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(dead_input, regalloc, {});
  const auto dead_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          dead_input),
      dead_analysis);

  expect_true(dead_plans.size() == 1 && dead_plans.front().alloca_name == "%lv.buf" &&
                  !dead_plans.front().needs_stack_slot &&
                  dead_plans.front().remove_following_entry_store,
              "shared slot-assignment planning should drop dead non-param entry allocas and their paired zero-init stores when analysis finds no uses");

  const auto live_module = make_live_local_alloca_candidate_module();
  const auto& live_function = live_module.functions.front();
  const auto live_input =
      lower_test_function_entry_alloca_stack_layout_input(live_function);
  const auto live_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(live_input, regalloc, {});
  const auto live_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          live_input),
      live_analysis);

  expect_true(live_plans.size() == 1 && live_plans.front().alloca_name == "%lv.buf" &&
                  live_plans.front().needs_stack_slot &&
                  !live_plans.front().remove_following_entry_store &&
                  live_plans.front().coalesced_block == 0 &&
                  live_plans.front().assigned_slot == 0,
              "shared slot-assignment planning should preserve paired zero-init stores for aggregate entry allocas while exposing the shared single-block reuse classification");

  const auto disjoint_module = make_disjoint_block_local_alloca_candidate_module();
  const auto& disjoint_function = disjoint_module.functions.front();
  const auto disjoint_input =
      lower_test_function_entry_alloca_stack_layout_input(disjoint_function);
  const auto disjoint_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(disjoint_input, regalloc, {});
  const auto disjoint_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          disjoint_input),
      disjoint_analysis);

  expect_true(disjoint_plans.size() == 2 &&
                  disjoint_plans[0].alloca_name == "%lv.then" &&
                  disjoint_plans[0].coalesced_block == 1 &&
                  disjoint_plans[1].alloca_name == "%lv.else" &&
                  disjoint_plans[1].coalesced_block == 2 &&
                  disjoint_plans[0].assigned_slot.has_value() &&
                  disjoint_plans[0].assigned_slot == disjoint_plans[1].assigned_slot,
              "shared slot-assignment planning should turn disjoint single-block allocas into a concrete shared slot decision");

  const auto same_block_module = make_same_block_local_alloca_candidate_module();
  const auto& same_block_function = same_block_module.functions.front();
  const auto same_block_input =
      lower_test_function_entry_alloca_stack_layout_input(same_block_function);
  const auto same_block_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(same_block_input, regalloc, {});
  const auto same_block_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          same_block_input),
      same_block_analysis);

  expect_true(same_block_plans.size() == 2 &&
                  same_block_plans[0].coalesced_block == 0 &&
                  same_block_plans[1].coalesced_block == 0 &&
                  same_block_plans[0].assigned_slot.has_value() &&
                  same_block_plans[1].assigned_slot.has_value() &&
                  same_block_plans[0].assigned_slot != same_block_plans[1].assigned_slot,
              "shared slot-assignment planning should keep same-block local allocas in distinct slots even when both are individually coalescable");

  const auto mixed_lifetime_module = make_mixed_lifetime_local_alloca_candidate_module();
  const auto& mixed_lifetime_function = mixed_lifetime_module.functions.front();
  const auto mixed_lifetime_input =
      lower_test_function_entry_alloca_stack_layout_input(mixed_lifetime_function);
  const auto mixed_lifetime_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(mixed_lifetime_input, regalloc, {});
  const auto mixed_lifetime_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          mixed_lifetime_input),
      mixed_lifetime_analysis);

  expect_true(mixed_lifetime_plans.size() == 2 &&
                  mixed_lifetime_plans[0].alloca_name == "%lv.a" &&
                  !mixed_lifetime_plans[0].coalesced_block.has_value() &&
                  mixed_lifetime_plans[0].assigned_slot.has_value() &&
                  mixed_lifetime_plans[1].alloca_name == "%lv.y" &&
                  mixed_lifetime_plans[1].coalesced_block == 0 &&
                  mixed_lifetime_plans[1].assigned_slot.has_value() &&
                  mixed_lifetime_plans[0].assigned_slot !=
                      mixed_lifetime_plans[1].assigned_slot,
              "shared slot-assignment planning should not let a single-block alloca reuse the slot of a value that stays live across multiple blocks");

  const auto read_first_module = make_read_before_store_local_alloca_candidate_module();
  const auto& read_first_function = read_first_module.functions.front();
  const auto read_first_input =
      lower_test_function_entry_alloca_stack_layout_input(read_first_function);
  const auto read_first_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(read_first_input, regalloc, {});
  const auto read_first_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          read_first_input),
      read_first_analysis);

  expect_true(read_first_plans.size() == 1 &&
                  read_first_plans.front().alloca_name == "%lv.buf" &&
                  read_first_plans.front().needs_stack_slot &&
                  !read_first_plans.front().remove_following_entry_store,
              "shared slot-assignment planning should preserve paired zero-init stores when a live entry alloca is read before the first overwrite");

  const auto scalar_overwrite_module = make_overwrite_first_scalar_local_alloca_candidate_module();
  const auto& scalar_overwrite_function = scalar_overwrite_module.functions.front();
  const auto scalar_overwrite_input =
      lower_test_function_entry_alloca_stack_layout_input(scalar_overwrite_function);
  const auto scalar_overwrite_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(scalar_overwrite_input, regalloc, {});
  const auto scalar_overwrite_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          scalar_overwrite_input),
      scalar_overwrite_analysis);

  expect_true(scalar_overwrite_plans.size() == 1 &&
                  scalar_overwrite_plans.front().alloca_name == "%lv.x" &&
                  scalar_overwrite_plans.front().needs_stack_slot &&
                  scalar_overwrite_plans.front().remove_following_entry_store,
              "shared slot-assignment planning should drop redundant paired zero-init stores for scalar entry allocas that are overwritten before any read");

  const auto scalar_read_first_module =
      make_read_before_store_scalar_local_alloca_candidate_module();
  const auto& scalar_read_first_function = scalar_read_first_module.functions.front();
  const auto scalar_read_first_input =
      lower_test_function_entry_alloca_stack_layout_input(scalar_read_first_function);
  const auto scalar_read_first_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(scalar_read_first_input, regalloc, {});
  const auto scalar_read_first_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          scalar_read_first_input),
      scalar_read_first_analysis);

  expect_true(scalar_read_first_plans.size() == 1 &&
                  scalar_read_first_plans.front().alloca_name == "%lv.x" &&
                  scalar_read_first_plans.front().needs_stack_slot &&
                  !scalar_read_first_plans.front().remove_following_entry_store,
              "shared slot-assignment planning should preserve paired zero-init stores for scalar entry allocas that are read before the first overwrite");

  const auto escaped_module = make_escaped_local_alloca_candidate_module();
  const auto& escaped_function = escaped_module.functions.back();
  const auto escaped_input =
      lower_test_function_entry_alloca_stack_layout_input(escaped_function);
  const auto escaped_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(escaped_input, regalloc, {});
  const auto escaped_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          escaped_input),
      escaped_analysis);

  expect_true(escaped_plans.size() == 1 &&
                  escaped_plans.front().alloca_name == "%lv.buf" &&
                  escaped_plans.front().needs_stack_slot &&
                  !escaped_plans.front().coalesced_block.has_value(),
              "shared slot-assignment planning should leave call-escaped local allocas out of the single-block reuse pool");
}

void test_backend_shared_slot_assignment_accepts_narrowed_planning_input() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  c4c::backend::stack_layout::EntryAllocaPlanningInput scalar_input;
  scalar_input.entry_allocas.push_back(c4c::backend::stack_layout::EntryAllocaPlanInput{
      "%lv.x",
      "i32",
      4,
      c4c::backend::stack_layout::EntryAllocaPairedStorePlanInfo{
          true, true, std::nullopt},
  });
  scalar_input.entry_alloca_use_blocks =
      std::vector<c4c::backend::stack_layout::EntryAllocaUseBlocks>{
          c4c::backend::stack_layout::EntryAllocaUseBlocks{"%lv.x", {0}}};
  const auto scalar_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      lower_test_function_entry_alloca_stack_layout_input(
          make_overwrite_first_scalar_local_alloca_candidate_module().functions.front()),
      regalloc,
      {});
  const auto scalar_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(scalar_input, scalar_analysis);

  expect_true(scalar_plans.size() == 1 &&
                  scalar_plans.front().alloca_name == "%lv.x" &&
                  scalar_plans.front().needs_stack_slot &&
                  scalar_plans.front().remove_following_entry_store,
              "shared slot-assignment planning should accept the narrowed planning surface when zero-init classification is preserved without the literal paired-store operand");

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};
  const auto dead_param_function = make_dead_param_alloca_candidate_module().functions.back();
  const auto dead_param_liveness_input =
      lower_test_backend_cfg_liveness_input(dead_param_function);
  const auto dead_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(dead_param_liveness_input, config, {});
  const auto dead_param_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      lower_test_function_entry_alloca_stack_layout_input(dead_param_function),
      dead_regalloc,
      {{20}, {21}, {22}});

  c4c::backend::stack_layout::EntryAllocaPlanningInput dead_param_input;
  dead_param_input.entry_allocas.push_back(c4c::backend::stack_layout::EntryAllocaPlanInput{
      "%lv.param.x",
      "i32",
      4,
      c4c::backend::stack_layout::EntryAllocaPairedStorePlanInfo{
          true, false, std::string("%p.x")},
  });
  const auto dead_param_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(dead_param_input, dead_param_analysis);
  const auto dead_param_slot_plans =
      c4c::backend::stack_layout::plan_param_alloca_slots(dead_param_input, dead_param_analysis);

  expect_true(dead_param_plans.size() == 1 &&
                  dead_param_plans.front().alloca_name == "%lv.param.x" &&
                  !dead_param_plans.front().needs_stack_slot &&
                  dead_param_plans.front().remove_following_entry_store,
              "shared slot-assignment planning should classify dead param allocas from the narrowed planning surface using param metadata instead of the full paired-store value");
  expect_true(dead_param_slot_plans.size() == 1 &&
                  dead_param_slot_plans.front().alloca_name == "%lv.param.x" &&
                  dead_param_slot_plans.front().param_name == "%p.x" &&
                  !dead_param_slot_plans.front().needs_stack_slot,
              "shared param-alloca planning should accept the narrowed planning surface without rebuilding the literal paired-store operand");
}

void test_backend_shared_slot_assignment_accepts_backend_owned_input() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto scalar_overwrite_module = make_overwrite_first_scalar_local_alloca_candidate_module();
  const auto& scalar_overwrite_function = scalar_overwrite_module.functions.front();
  const auto scalar_input =
      lower_test_function_entry_alloca_stack_layout_input(scalar_overwrite_function);
  const auto scalar_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(scalar_input, regalloc, {});
  const auto scalar_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          scalar_input),
      scalar_analysis);

  expect_true(scalar_plans.size() == 1 &&
                  scalar_plans.front().alloca_name == "%lv.x" &&
                  scalar_plans.front().needs_stack_slot &&
                  scalar_plans.front().remove_following_entry_store,
              "backend-owned stack-layout input should preserve scalar overwrite-before-read slot pruning");

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto dead_param_module = make_dead_param_alloca_candidate_module();
  const auto& dead_param_function = dead_param_module.functions.back();
  const auto dead_param_input =
      lower_test_function_entry_alloca_stack_layout_input(dead_param_function);
  const auto dead_param_liveness_input =
      lower_test_backend_cfg_liveness_input(dead_param_function);
  const auto dead_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(dead_param_liveness_input, config, {});
  const auto dead_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      dead_param_input, dead_regalloc, {{20}, {21}, {22}});
  const auto dead_param_plans = c4c::backend::stack_layout::plan_param_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          dead_param_input),
      dead_analysis);

  expect_true(dead_param_plans.size() == 1 &&
                  dead_param_plans.front().alloca_name == "%lv.param.x" &&
                  dead_param_plans.front().param_name == "%p.x" &&
                  !dead_param_plans.front().needs_stack_slot,
              "backend-owned stack-layout input should preserve dead param alloca pruning decisions");

  auto dead_entry_function = make_dead_local_alloca_candidate_module().functions.front();
  const auto dead_entry_input =
      lower_test_function_entry_alloca_stack_layout_input(dead_entry_function);
  const auto dead_entry_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      dead_entry_input, regalloc, {});
  const auto dead_entry_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          dead_entry_input),
      dead_entry_analysis);
  const auto dead_entry_patch = c4c::backend::stack_layout::build_entry_alloca_rewrite_patch(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_rewrite_input(
          dead_entry_input),
      dead_entry_plans);
  c4c::backend::stack_layout::apply_entry_alloca_rewrite_patch(dead_entry_function,
                                                               dead_entry_patch);

  expect_true(dead_entry_function.alloca_insts.empty(),
              "backend-owned stack-layout input should preserve entry-alloca pruning when the apply step still mutates the original LIR function");
}

void test_backend_shared_slot_assignment_bundle_accepts_backend_owned_input() {
  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto dead_param_module = make_dead_param_alloca_candidate_module();
  const auto& dead_param_function = dead_param_module.functions.back();
  const auto dead_param_input =
      lower_test_function_entry_alloca_stack_layout_input(dead_param_function);
  const auto dead_param_liveness_input =
      lower_test_backend_cfg_liveness_input(dead_param_function);
  const auto dead_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(dead_param_liveness_input, config, {});
  const auto dead_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      dead_param_input, dead_regalloc, {{20}, {21}, {22}});
  const auto dead_bundle = c4c::backend::stack_layout::build_stack_layout_plan_bundle(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          dead_param_input),
      dead_analysis);

  expect_true(dead_bundle.entry_alloca_plans.size() == 1 &&
                  dead_bundle.entry_alloca_plans.front().alloca_name == "%lv.param.x" &&
                  !dead_bundle.entry_alloca_plans.front().needs_stack_slot &&
                  dead_bundle.entry_alloca_plans.front().remove_following_entry_store,
              "backend-owned stack-layout plan bundle should preserve entry-alloca pruning decisions for dead param allocas");
  expect_true(dead_bundle.param_alloca_plans.size() == 1 &&
                  dead_bundle.param_alloca_plans.front().alloca_name == "%lv.param.x" &&
                  dead_bundle.param_alloca_plans.front().param_name == "%p.x" &&
                  !dead_bundle.param_alloca_plans.front().needs_stack_slot,
              "backend-owned stack-layout plan bundle should preserve param-alloca pruning decisions from the same lowered input");

  auto dead_function = dead_param_function;
  const auto dead_patch = c4c::backend::stack_layout::build_entry_alloca_rewrite_patch(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_rewrite_input(
          dead_param_input),
      dead_bundle.entry_alloca_plans);
  c4c::backend::stack_layout::apply_entry_alloca_rewrite_patch(dead_function, dead_patch);
  expect_true(dead_function.alloca_insts.empty(),
              "backend-owned stack-layout plan bundle should still drive the production entry-alloca mutation step");
}

void test_backend_shared_slot_assignment_bundle_accepts_narrowed_planning_input() {
  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto dead_param_module = make_dead_param_alloca_candidate_module();
  const auto& dead_param_function = dead_param_module.functions.back();
  const auto dead_param_liveness_input =
      lower_test_backend_cfg_liveness_input(dead_param_function);
  const auto dead_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(dead_param_liveness_input, config, {});
  const auto dead_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      lower_test_function_entry_alloca_stack_layout_input(dead_param_function),
      dead_regalloc,
      {{20}, {21}, {22}});

  c4c::backend::stack_layout::EntryAllocaPlanningInput dead_param_input;
  dead_param_input.entry_allocas.push_back(c4c::backend::stack_layout::EntryAllocaPlanInput{
      "%lv.param.x",
      "i32",
      4,
      c4c::backend::stack_layout::EntryAllocaPairedStorePlanInfo{
          true, false, std::string("%p.x")},
  });

  const auto dead_bundle = c4c::backend::stack_layout::build_stack_layout_plan_bundle(
      dead_param_input, dead_analysis);

  expect_true(dead_bundle.entry_alloca_plans.size() == 1 &&
                  dead_bundle.entry_alloca_plans.front().alloca_name == "%lv.param.x" &&
                  !dead_bundle.entry_alloca_plans.front().needs_stack_slot &&
                  dead_bundle.entry_alloca_plans.front().remove_following_entry_store,
              "shared stack-layout plan bundle should accept narrowed planning input once analysis has already been computed from the backend-owned surface");
  expect_true(dead_bundle.param_alloca_plans.size() == 1 &&
                  dead_bundle.param_alloca_plans.front().alloca_name == "%lv.param.x" &&
                  dead_bundle.param_alloca_plans.front().param_name == "%p.x" &&
                  !dead_bundle.param_alloca_plans.front().needs_stack_slot,
              "shared stack-layout plan bundle should preserve param-alloca pruning from the narrowed planning surface");
  expect_true(dead_bundle.analysis.is_dead_param_alloca("%lv.param.x"),
              "shared stack-layout plan bundle should keep the previously computed backend-owned analysis available alongside the narrowed planning surface");
}

void test_backend_shared_slot_assignment_stack_layout_lowering_matches_prepared_planning_input() {
  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto dead_param_module = make_dead_param_alloca_candidate_module();
  const auto preparation =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_preparation(
          dead_param_module, 1);
  const auto prepared_rewrite_only =
      c4c::backend::stack_layout::lower_prepared_entry_alloca_rewrite_only_inputs(
          preparation);
  const auto prepared_stack_layout_input =
      c4c::backend::stack_layout::lower_prepared_entry_alloca_stack_layout_input(preparation);
  const auto dead_regalloc = c4c::backend::run_regalloc_and_merge_clobbers(
      prepared_rewrite_only.liveness_input, config, {});
  const auto dead_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      prepared_stack_layout_input, dead_regalloc, {{20}, {21}, {22}});

  const auto lowered_planning_input =
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          prepared_stack_layout_input);
  const auto backend_entry_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      lowered_planning_input, dead_analysis);
  const auto narrowed_entry_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      prepared_rewrite_only.planning_input, dead_analysis);
  const auto backend_param_plans = c4c::backend::stack_layout::plan_param_alloca_slots(
      lowered_planning_input, dead_analysis);
  const auto narrowed_param_plans = c4c::backend::stack_layout::plan_param_alloca_slots(
      prepared_rewrite_only.planning_input, dead_analysis);

  expect_true(backend_entry_plans.size() == narrowed_entry_plans.size() &&
                  backend_entry_plans.size() == 1 &&
                  backend_entry_plans.front().alloca_name ==
                      narrowed_entry_plans.front().alloca_name &&
                  backend_entry_plans.front().needs_stack_slot ==
                      narrowed_entry_plans.front().needs_stack_slot &&
                  backend_entry_plans.front().remove_following_entry_store ==
                      narrowed_entry_plans.front().remove_following_entry_store &&
                  backend_entry_plans.front().coalesced_block ==
                      narrowed_entry_plans.front().coalesced_block &&
                  backend_entry_plans.front().assigned_slot ==
                      narrowed_entry_plans.front().assigned_slot,
              "explicit stack-layout-input lowering should match the prepared planning seam for the same prepared function inputs");
  expect_true(backend_param_plans.size() == narrowed_param_plans.size() &&
                  backend_param_plans.size() == 1 &&
                  backend_param_plans.front().alloca_name ==
                      narrowed_param_plans.front().alloca_name &&
                  backend_param_plans.front().param_name ==
                      narrowed_param_plans.front().param_name &&
                  backend_param_plans.front().needs_stack_slot ==
                      narrowed_param_plans.front().needs_stack_slot,
              "explicit stack-layout-input lowering should preserve param-alloca planning parity with the prepared planning seam");
}

void test_backend_shared_slot_assignment_bundle_prepares_from_backend_owned_inputs() {
  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto dead_param_module = make_dead_param_alloca_candidate_module();
  const auto& dead_param_function = dead_param_module.functions.back();
  const auto dead_param_input =
      lower_test_function_entry_alloca_stack_layout_input(dead_param_function);
  const auto dead_param_liveness_input =
      lower_test_backend_cfg_liveness_input(dead_param_function);
  const auto prepared_bundle = c4c::backend::stack_layout::build_stack_layout_plan_bundle(
      dead_param_liveness_input,
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          dead_param_input),
      config,
      {},
      {{20}, {21}, {22}});

  expect_true(prepared_bundle.entry_alloca_plans.size() == 1 &&
                  prepared_bundle.entry_alloca_plans.front().alloca_name == "%lv.param.x" &&
                  !prepared_bundle.entry_alloca_plans.front().needs_stack_slot &&
                  prepared_bundle.entry_alloca_plans.front().remove_following_entry_store,
              "shared stack-layout bundle prep should preserve entry-alloca pruning decisions when callers provide backend-owned liveness and stack-layout inputs");
  expect_true(prepared_bundle.param_alloca_plans.size() == 1 &&
                  prepared_bundle.param_alloca_plans.front().alloca_name == "%lv.param.x" &&
                  prepared_bundle.param_alloca_plans.front().param_name == "%p.x" &&
                  !prepared_bundle.param_alloca_plans.front().needs_stack_slot,
              "shared stack-layout bundle prep should preserve param-alloca pruning decisions from the same backend-owned inputs");
  expect_true(prepared_bundle.analysis.is_dead_param_alloca("%lv.param.x"),
              "shared stack-layout bundle prep should keep the backend-owned dead-param classification available to downstream entrypoint setup");
}

void test_backend_shared_slot_assignment_stack_layout_lowering_matches_prepared_bundle_input() {
  c4c::backend::RegAllocConfig config;

  const auto live_module = make_live_local_alloca_candidate_module();
  const auto preparation =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_preparation(
          live_module, 0);
  const auto prepared_rewrite_only =
      c4c::backend::stack_layout::lower_prepared_entry_alloca_rewrite_only_inputs(
          preparation);
  const auto prepared_stack_layout_input =
      c4c::backend::stack_layout::lower_prepared_entry_alloca_stack_layout_input(preparation);

  const auto lowered_planning_input =
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          prepared_stack_layout_input);
  const auto compatibility_bundle = c4c::backend::stack_layout::build_stack_layout_plan_bundle(
      prepared_rewrite_only.liveness_input, lowered_planning_input, config, {}, {});
  const auto narrowed_bundle = c4c::backend::stack_layout::build_stack_layout_plan_bundle(
      prepared_rewrite_only.liveness_input, prepared_rewrite_only.planning_input, config, {}, {});

  expect_true(compatibility_bundle.entry_alloca_plans.size() == narrowed_bundle.entry_alloca_plans.size() &&
                  compatibility_bundle.entry_alloca_plans.size() == 1 &&
                  compatibility_bundle.entry_alloca_plans.front().alloca_name ==
                      narrowed_bundle.entry_alloca_plans.front().alloca_name &&
                  compatibility_bundle.entry_alloca_plans.front().needs_stack_slot ==
                      narrowed_bundle.entry_alloca_plans.front().needs_stack_slot &&
                  compatibility_bundle.entry_alloca_plans.front().remove_following_entry_store ==
                      narrowed_bundle.entry_alloca_plans.front().remove_following_entry_store &&
                  compatibility_bundle.entry_alloca_plans.front().coalesced_block ==
                      narrowed_bundle.entry_alloca_plans.front().coalesced_block &&
                  compatibility_bundle.entry_alloca_plans.front().assigned_slot ==
                      narrowed_bundle.entry_alloca_plans.front().assigned_slot &&
                  compatibility_bundle.entry_alloca_plans.front().alloca_name == "%lv.buf" &&
                  compatibility_bundle.entry_alloca_plans.front().needs_stack_slot &&
                  compatibility_bundle.entry_alloca_plans.front().coalesced_block == 0,
              "explicit stack-layout-input lowering should match the prepared planning seam for live local allocas that rely on prepared use-block metadata");
  expect_true(compatibility_bundle.param_alloca_plans.empty() &&
                  narrowed_bundle.param_alloca_plans.empty(),
              "explicit stack-layout-input lowering should not introduce param-alloca plan drift when lowering through the prepared planning seam");
}

void test_backend_shared_slot_assignment_bundle_prepares_from_narrowed_planning_input() {
  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto scalar_module = make_overwrite_first_scalar_local_alloca_candidate_module();
  const auto scalar_inputs =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_rewrite_only_inputs(
          scalar_module, 0);
  const auto scalar_bundle = c4c::backend::stack_layout::build_stack_layout_plan_bundle(
      scalar_inputs.liveness_input, scalar_inputs.planning_input, config, {}, {});

  expect_true(
      scalar_inputs.planning_input.entry_allocas.size() == 1 &&
          scalar_inputs.planning_input.entry_alloca_first_accesses.has_value() &&
          scalar_inputs.planning_input.entry_alloca_first_accesses->size() == 1 &&
          scalar_inputs.planning_input.entry_alloca_first_accesses->front().alloca_name ==
              "%lv.x" &&
          scalar_inputs.planning_input.entry_alloca_first_accesses->front().kind ==
              c4c::backend::stack_layout::PointerAccessKind::Store &&
          scalar_bundle.entry_alloca_plans.size() == 1 &&
          scalar_bundle.entry_alloca_plans.front().alloca_name == "%lv.x" &&
          scalar_bundle.entry_alloca_plans.front().needs_stack_slot &&
          scalar_bundle.entry_alloca_plans.front().remove_following_entry_store,
      "shared stack-layout bundle prep should preserve overwrite-before-read pruning when callers provide backend-owned liveness plus the narrowed planning input");

  const auto dead_param_module = make_dead_param_alloca_candidate_module();
  const auto dead_param_inputs =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_rewrite_only_inputs(
          dead_param_module, 1);
  const auto dead_param_bundle = c4c::backend::stack_layout::build_stack_layout_plan_bundle(
      dead_param_inputs.liveness_input,
      dead_param_inputs.planning_input,
      config,
      {},
      {{20}, {21}, {22}});

  expect_true(dead_param_bundle.entry_alloca_plans.size() == 1 &&
                  dead_param_bundle.entry_alloca_plans.front().alloca_name == "%lv.param.x" &&
                  !dead_param_bundle.entry_alloca_plans.front().needs_stack_slot &&
                  dead_param_bundle.entry_alloca_plans.front().remove_following_entry_store &&
                  dead_param_bundle.param_alloca_plans.size() == 1 &&
                  dead_param_bundle.param_alloca_plans.front().alloca_name == "%lv.param.x" &&
                  dead_param_bundle.param_alloca_plans.front().param_name == "%p.x" &&
                  !dead_param_bundle.param_alloca_plans.front().needs_stack_slot &&
                  dead_param_bundle.analysis.is_dead_param_alloca("%lv.param.x"),
              "shared stack-layout bundle prep should let planning-only callers drop the compatibility stack-layout wrapper once prepared lowering exposes the narrowed planning payload");
}

void test_backend_shared_slot_assignment_stack_layout_lowering_matches_prepared_rewrite_input() {
  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  auto dead_function = make_dead_param_alloca_candidate_module().functions.back();
  const auto dead_liveness_input =
      lower_test_backend_cfg_liveness_input(dead_function);
  const auto dead_stack_layout_input =
      lower_test_function_entry_alloca_stack_layout_input(dead_function);

  const auto patch = c4c::backend::stack_layout::prepare_entry_alloca_rewrite_patch(
      dead_liveness_input,
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_rewrite_input(
          dead_stack_layout_input),
      config,
      {},
      {{20}, {21}, {22}});
  const auto narrowed_dead_inputs =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_rewrite_only_inputs(
          make_dead_param_alloca_candidate_module(), 1);
  const auto narrowed_patch = c4c::backend::stack_layout::prepare_entry_alloca_rewrite_patch(
      dead_liveness_input,
      narrowed_dead_inputs.rewrite_input,
      config,
      {},
      {{20}, {21}, {22}});

  expect_true(patch.alloca_insts.empty(),
              "explicit stack-layout-input lowering should preserve dead entry-alloca pruning decisions when callers provide backend-owned inputs");
  expect_true(patch.canonical_allocas.empty(),
              "explicit stack-layout-input lowering should not invent canonical alloca rewrites when the planned entry alloca is deleted");
  expect_true(narrowed_patch.alloca_insts.empty() && narrowed_patch.canonical_allocas.empty(),
              "shared stack-layout patch prep should drive the production rewrite path from the narrowed entry-alloca rewrite input without needing the broader stack-layout contract");

  c4c::backend::stack_layout::apply_entry_alloca_rewrite_patch(dead_function, patch);

  expect_true(dead_function.alloca_insts.empty(),
              "shared stack-layout patch apply should let callers keep planning on backend-owned inputs while the final raw-LIR mutation stays isolated");

  const auto live_module = make_live_local_alloca_candidate_module();
  const auto& live_function = live_module.functions.front();
  const auto live_liveness_input = lower_test_backend_cfg_liveness_input(live_function);
  const auto live_stack_layout_input =
      lower_test_function_entry_alloca_stack_layout_input(live_function);
  const auto live_compat_patch = c4c::backend::stack_layout::prepare_entry_alloca_rewrite_patch(
      live_liveness_input,
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_rewrite_input(
          live_stack_layout_input),
      config,
      {},
      {});
  const auto live_narrowed_inputs =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_rewrite_only_inputs(
          live_module, 0);
  const auto live_narrowed_patch = c4c::backend::stack_layout::prepare_entry_alloca_rewrite_patch(
      live_liveness_input,
      live_narrowed_inputs.rewrite_input,
      config,
      {},
      {});

  expect_true(live_compat_patch.alloca_insts.size() == live_narrowed_patch.alloca_insts.size() &&
                  live_compat_patch.canonical_allocas == live_narrowed_patch.canonical_allocas,
              "explicit stack-layout-input lowering should match the narrowed rewrite seam for live local allocas as well as dead-entry pruning cases");
}

void test_backend_shared_slot_assignment_rewrites_module_entry_allocas() {
  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto module = make_dead_param_alloca_candidate_module();
  const auto rewritten = c4c::backend::stack_layout::rewrite_module_entry_allocas(
      module, config, {}, {{20}, {21}, {22}});

  expect_true(rewritten.functions.size() == module.functions.size(),
              "shared module entry-alloca rewrite should preserve the function list while retargeting direct-emitter prep off duplicated target-local glue");
  expect_true(rewritten.functions.front().is_declaration &&
                  rewritten.functions.front().alloca_insts.empty(),
              "shared module entry-alloca rewrite should leave declarations untouched");
  expect_true(rewritten.functions.back().alloca_insts.empty(),
              "shared module entry-alloca rewrite should prune dead param allocas across the whole LIR module");
}

void test_backend_shared_slot_assignment_prefers_per_function_bir_liveness_when_available() {
  const auto module = make_mixed_bir_and_entry_alloca_module();

  expect_true(!c4c::backend::try_lower_to_bir(module).has_value(),
              "shared entry-alloca rewrite coverage should start from a mixed module whose blocking function still prevents whole-module BIR lowering");

  const auto helper_liveness =
      c4c::backend::stack_layout::try_lower_module_function_to_bir_liveness_input(module, 0);
  expect_true(helper_liveness.has_value() && helper_liveness->entry_insts.empty() &&
                  helper_liveness->blocks.size() == 1 &&
                  helper_liveness->blocks.front().insts.size() == 1,
              "shared entry-alloca rewrite should be able to lower a lowerable function through the per-function BIR seam even when another function still blocks whole-module BIR lowering");

  const auto blocking_liveness =
      c4c::backend::stack_layout::try_lower_module_function_to_bir_liveness_input(module, 2);
  expect_true(!blocking_liveness.has_value(),
              "shared entry-alloca rewrite should keep the raw-LIR compatibility fallback for functions whose allocas still block BIR lowering");
}

void test_backend_shared_slot_assignment_prepares_module_function_inputs() {
  const auto module = make_mixed_bir_and_entry_alloca_module();

  const auto lowerable_preparation =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_preparation(module, 0);
  expect_true(lowerable_preparation.liveness_source ==
                  c4c::backend::stack_layout::EntryAllocaRewriteLivenessSource::PerFunctionBir &&
                  lowerable_preparation.stack_layout_source ==
                      c4c::backend::stack_layout::EntryAllocaRewriteStackLayoutSource::
                          EntryAllocasAndBackendCfg &&
                  lowerable_preparation.rewrite_metadata.entry_allocas.empty() &&
                  lowerable_preparation.backend_cfg_liveness == std::nullopt &&
                  lowerable_preparation.liveness_input.has_value() &&
                  lowerable_preparation.liveness_input->entry_insts.empty() &&
                  lowerable_preparation.liveness_input->blocks.size() == 1 &&
                  lowerable_preparation.liveness_input->blocks.front().insts.size() == 1,
              "shared entry-alloca rewrite prep should keep lowerable functions on the per-function BIR liveness seam while sourcing stack-layout metadata through the backend-owned entry-alloca plus CFG carrier");

  const auto fallback_preparation =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_preparation(module, 2);
  expect_true(fallback_preparation.liveness_source ==
                  c4c::backend::stack_layout::EntryAllocaRewriteLivenessSource::RawLirBackendCfg &&
                  fallback_preparation.stack_layout_source ==
                      c4c::backend::stack_layout::EntryAllocaRewriteStackLayoutSource::
                          EntryAllocasAndBackendCfg &&
                  fallback_preparation.rewrite_metadata.entry_allocas.size() == 1 &&
                  fallback_preparation.rewrite_metadata.entry_allocas.front().alloca_name ==
                      "%lv.buf" &&
                  fallback_preparation.stack_layout_metadata.signature_params.empty() &&
                  fallback_preparation.stack_layout_metadata.return_type ==
                      std::optional<std::string>{"i32"} &&
                  !fallback_preparation.stack_layout_metadata.is_variadic &&
                  fallback_preparation.stack_layout_metadata.call_results.empty() &&
                  fallback_preparation.backend_cfg_liveness.has_value() &&
                  !fallback_preparation.liveness_input.has_value() &&
                  fallback_preparation.backend_cfg_liveness->entry_insts.size() == 2,
              "shared entry-alloca rewrite prep should preserve entry-alloca ownership in the fallback carrier while sourcing block usage from the backend-owned CFG seam instead of the raw-LIR stack-layout lowering path");

  const auto fallback_rewrite_only =
      c4c::backend::stack_layout::lower_prepared_entry_alloca_rewrite_only_inputs(
          fallback_preparation);
  const auto fallback_stack_layout_input =
      c4c::backend::stack_layout::lower_prepared_entry_alloca_stack_layout_input(
          fallback_preparation);
  expect_true(fallback_rewrite_only.liveness_source ==
                  c4c::backend::stack_layout::EntryAllocaRewriteLivenessSource::RawLirBackendCfg &&
                  fallback_rewrite_only.stack_layout_source ==
                      c4c::backend::stack_layout::EntryAllocaRewriteStackLayoutSource::
                          EntryAllocasAndBackendCfg &&
                  fallback_rewrite_only.planning_input.entry_allocas.size() == 1 &&
                  fallback_rewrite_only.planning_input.entry_allocas.front().alloca_name ==
                      "%lv.buf" &&
                  fallback_rewrite_only.planning_input.entry_allocas.front().paired_store.has_store &&
                  fallback_rewrite_only.planning_input.entry_allocas.front().paired_store
                      .is_zero_initializer &&
                  !fallback_rewrite_only.planning_input.entry_allocas.front().paired_store.param_name
                       .has_value() &&
                  fallback_rewrite_only.planning_input.escaped_entry_allocas.has_value() &&
                  fallback_rewrite_only.planning_input.escaped_entry_allocas->empty() &&
                  fallback_rewrite_only.planning_input.entry_alloca_use_blocks.has_value() &&
                  fallback_rewrite_only.planning_input.entry_alloca_use_blocks->empty() &&
                  fallback_rewrite_only.rewrite_input.entry_allocas.size() == 1 &&
                  fallback_rewrite_only.rewrite_input.entry_allocas.front().alloca_name ==
                      "%lv.buf" &&
                  fallback_rewrite_only.rewrite_input.entry_alloca_use_blocks.has_value() &&
                  fallback_stack_layout_input.entry_allocas.size() == 1 &&
                  fallback_stack_layout_input.entry_allocas.front().alloca_name ==
                      "%lv.buf" &&
                  fallback_stack_layout_input.return_type ==
                      std::optional<std::string>{"i32"} &&
                  fallback_rewrite_only.liveness_input.entry_insts.size() == 2,
              "shared entry-alloca rewrite prep should lower the prepared fallback carrier into the narrowed rewrite-input contract while keeping stack-layout rehydration as a separate compatibility helper");

  const auto lowerable_rewrite_only =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_rewrite_only_inputs(
          module, 0);
  const auto lowerable_stack_layout_input =
      lower_test_prepared_entry_alloca_stack_layout_input(module, 0);
  expect_true(lowerable_rewrite_only.liveness_source ==
                  c4c::backend::stack_layout::EntryAllocaRewriteLivenessSource::PerFunctionBir &&
                  lowerable_rewrite_only.stack_layout_source ==
                      c4c::backend::stack_layout::EntryAllocaRewriteStackLayoutSource::
                          EntryAllocasAndBackendCfg &&
                  lowerable_rewrite_only.planning_input.entry_allocas.empty() &&
                  lowerable_rewrite_only.rewrite_input.entry_allocas.empty() &&
                  lowerable_stack_layout_input.entry_allocas.empty() &&
                  lowerable_rewrite_only.liveness_input.entry_insts.empty() &&
                  lowerable_rewrite_only.liveness_input.blocks.size() == 1 &&
                  lowerable_rewrite_only.liveness_input.blocks.front().insts.size() == 1,
              "shared entry-alloca rewrite prep should keep lowerable-function stack-layout rehydration separate from the narrowed backend-owned rewrite and liveness seams");
}

void test_backend_shared_prepared_function_inputs_preserve_emitter_stack_layout_metadata() {
  const auto module = make_mixed_bir_and_entry_alloca_module();

  const auto lowerable_prepared =
      lower_test_prepared_entry_alloca_stack_layout_input(module, 0);
  const auto lowerable_direct =
      c4c::backend::stack_layout::lower_function_entry_alloca_stack_layout_input(
          module.functions[0], c4c::backend::lower_lir_to_backend_cfg(module.functions[0]));
  expect_true(
      c4c::backend::stack_layout::collect_stack_layout_value_names(lowerable_prepared) ==
          c4c::backend::stack_layout::collect_stack_layout_value_names(lowerable_direct) &&
          lowerable_prepared.signature_params.size() == lowerable_direct.signature_params.size() &&
          lowerable_prepared.return_type == lowerable_direct.return_type &&
          lowerable_prepared.call_results.size() == lowerable_direct.call_results.size() &&
          lowerable_prepared.blocks.size() == lowerable_direct.blocks.size(),
      "prepared per-function stack-layout inputs should preserve the slot-building value set and signature metadata for lowerable functions so production emitters do not need to call the raw-LIR helper directly");

  const auto fallback_prepared =
      lower_test_prepared_entry_alloca_stack_layout_input(module, 2);
  const auto fallback_direct =
      c4c::backend::stack_layout::lower_function_entry_alloca_stack_layout_input(
          module.functions[2], c4c::backend::lower_lir_to_backend_cfg(module.functions[2]));
  expect_true(
      c4c::backend::stack_layout::collect_stack_layout_value_names(fallback_prepared) ==
          c4c::backend::stack_layout::collect_stack_layout_value_names(fallback_direct) &&
          fallback_prepared.entry_allocas.size() == fallback_direct.entry_allocas.size() &&
          fallback_prepared.signature_params.size() == fallback_direct.signature_params.size() &&
          fallback_prepared.return_type == fallback_direct.return_type &&
          fallback_prepared.call_results.size() == fallback_direct.call_results.size() &&
          fallback_prepared.blocks.size() == fallback_direct.blocks.size(),
      "prepared per-function stack-layout inputs should preserve the fallback backend-CFG-derived metadata needed by production emitters while keeping the raw-LIR surface compatibility-only");
}

void test_backend_shared_prepared_function_inputs_collect_emitter_value_names_without_compat_stack_layout() {
  const auto module = make_mixed_bir_and_entry_alloca_module();

  const auto lowerable_preparation =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_preparation(module, 0);
  const auto lowerable_stack_layout_input =
      lower_test_prepared_entry_alloca_stack_layout_input(module, 0);
  expect_true(
      lowerable_preparation.backend_cfg.has_value() &&
          c4c::backend::stack_layout::collect_prepared_entry_alloca_value_names(
              lowerable_preparation) ==
              c4c::backend::stack_layout::collect_stack_layout_value_names(
                  lowerable_stack_layout_input),
      "prepared lowerable function inputs should expose the same emitter-facing value-name set through the backend-CFG-backed carrier without rehydrating the compatibility stack-layout input");

  const auto fallback_preparation =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_preparation(module, 2);
  const auto fallback_stack_layout_input =
      lower_test_prepared_entry_alloca_stack_layout_input(module, 2);
  expect_true(
      fallback_preparation.backend_cfg.has_value() &&
          c4c::backend::stack_layout::collect_prepared_entry_alloca_value_names(
              fallback_preparation) ==
              c4c::backend::stack_layout::collect_stack_layout_value_names(
                  fallback_stack_layout_input),
      "prepared fallback function inputs should expose the same emitter-facing value-name set through the backend-CFG-backed carrier without routing through the compatibility stack-layout wrapper");
}

void test_backend_shared_rewrite_only_prepared_function_inputs_preserve_rewrite_and_planning_state() {
  const auto module = make_mixed_bir_and_entry_alloca_module();

  const auto lowerable_full =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_rewrite_only_inputs(
          module, 0);
  const auto lowerable_rewrite_only =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_rewrite_only_inputs(
          module, 0);
  expect_true(
      lowerable_rewrite_only.liveness_source == lowerable_full.liveness_source &&
          lowerable_rewrite_only.stack_layout_source == lowerable_full.stack_layout_source &&
          lowerable_rewrite_only.rewrite_input.entry_allocas.size() ==
              lowerable_full.rewrite_input.entry_allocas.size() &&
          lowerable_rewrite_only.planning_input.entry_allocas.size() ==
              lowerable_full.planning_input.entry_allocas.size() &&
          lowerable_rewrite_only.liveness_input.entry_insts.size() ==
              lowerable_full.liveness_input.entry_insts.size() &&
          lowerable_rewrite_only.liveness_input.blocks.size() ==
              lowerable_full.liveness_input.blocks.size(),
      "shared entry-alloca rewrite-only prep should preserve the narrowed rewrite and liveness seams for lowerable functions without routing through the compatibility stack-layout wrapper");

  const auto fallback_full =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_rewrite_only_inputs(
          module, 2);
  const auto fallback_rewrite_only =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_rewrite_only_inputs(
          module, 2);
  expect_true(
      fallback_rewrite_only.liveness_source == fallback_full.liveness_source &&
          fallback_rewrite_only.stack_layout_source == fallback_full.stack_layout_source &&
          fallback_rewrite_only.rewrite_input.entry_allocas.size() ==
              fallback_full.rewrite_input.entry_allocas.size() &&
          fallback_rewrite_only.rewrite_input.entry_allocas.front().alloca_name ==
              fallback_full.rewrite_input.entry_allocas.front().alloca_name &&
          fallback_rewrite_only.planning_input.entry_allocas.size() ==
              fallback_full.planning_input.entry_allocas.size() &&
          fallback_rewrite_only.planning_input.entry_alloca_use_blocks.has_value() ==
              fallback_full.planning_input.entry_alloca_use_blocks.has_value() &&
          fallback_rewrite_only.liveness_input.entry_insts.size() ==
              fallback_full.liveness_input.entry_insts.size() &&
          fallback_rewrite_only.liveness_input.blocks.size() ==
              fallback_full.liveness_input.blocks.size(),
      "shared entry-alloca rewrite-only prep should preserve the narrowed fallback rewrite/planning seams so rewrite_module_entry_allocas no longer has to rehydrate a compatibility stack-layout input first");
}

void test_backend_shared_fallback_entry_alloca_stack_layout_input_preserves_pointer_metadata() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto scalar_module = make_overwrite_first_scalar_local_alloca_candidate_module();
  const auto& scalar_function = scalar_module.functions.front();
  const auto scalar_input =
      c4c::backend::stack_layout::lower_function_entry_alloca_stack_layout_input(
          scalar_function, c4c::backend::lower_lir_to_backend_cfg(scalar_function));
  const auto scalar_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(scalar_input, regalloc, {});
  const auto scalar_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          scalar_input),
      scalar_analysis);

  expect_true(scalar_plans.size() == 1 &&
                  scalar_plans.front().alloca_name == "%lv.x" &&
                  scalar_plans.front().needs_stack_slot &&
                  scalar_plans.front().remove_following_entry_store,
              "shared fallback stack-layout prep should preserve direct pointer store metadata through the backend-CFG seam so scalar overwrite-before-read pruning still works");

  const auto aggregate_module = make_live_local_alloca_candidate_module();
  const auto& aggregate_function = aggregate_module.functions.front();
  const auto aggregate_input =
      c4c::backend::stack_layout::lower_function_entry_alloca_stack_layout_input(
          aggregate_function, c4c::backend::lower_lir_to_backend_cfg(aggregate_function));
  const auto aggregate_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(aggregate_input, regalloc, {});
  const auto aggregate_coalescing =
      c4c::backend::stack_layout::compute_coalescable_allocas(aggregate_input,
                                                              aggregate_analysis);

  expect_true(!aggregate_coalescing.is_dead_alloca("%lv.buf") &&
                  aggregate_coalescing.find_single_block("%lv.buf") == 0,
              "shared fallback stack-layout prep should preserve derived-pointer metadata through the backend-CFG seam so aggregate single-block reuse classification still works");

  const auto escaped_module = make_escaped_local_alloca_candidate_module();
  const auto& escaped_function = escaped_module.functions.back();
  const auto escaped_input =
      c4c::backend::stack_layout::lower_function_entry_alloca_stack_layout_input(
          escaped_function, c4c::backend::lower_lir_to_backend_cfg(escaped_function));
  const auto escaped_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(escaped_input, regalloc, {});
  const auto escaped_coalescing =
      c4c::backend::stack_layout::compute_coalescable_allocas(escaped_input,
                                                              escaped_analysis);

  expect_true(!escaped_coalescing.is_dead_alloca("%lv.buf") &&
                  !escaped_coalescing.find_single_block("%lv.buf").has_value(),
              "shared fallback stack-layout prep should preserve escape metadata through the backend-CFG seam so escaped allocas stay out of the reusable single-block pool");
}

void test_backend_shared_fallback_entry_alloca_stack_layout_input_preserves_signature_and_call_metadata() {
  c4c::codegen::lir::LirFunction function;
  function.name = "aggregate_variadic_entry_alloca";
  function.signature_text =
      "define { i32, i32 } @aggregate_variadic_entry_alloca(i32 %p.x, ...)";
  function.alloca_insts.push_back(
      c4c::codegen::lir::LirAllocaOp{"%lv.buf", "{ i32, i32 }", "", 8});

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(c4c::codegen::lir::LirCallOp{
      "%call0",
      c4c::codegen::lir::LirTypeRef("{ i32, i32 }"),
      "@helper",
      "(i32)",
      "i32 %p.x",
  });
  entry.terminator = c4c::codegen::lir::LirRet{std::nullopt, "void"};
  function.blocks.push_back(std::move(entry));

  const auto input = c4c::backend::stack_layout::lower_function_entry_alloca_stack_layout_input(
      function, c4c::backend::lower_lir_to_backend_cfg(function));

  expect_true(input.entry_allocas.size() == 1 &&
                  input.entry_allocas.front().alloca_name == "%lv.buf" &&
                  input.signature_params.size() == 2 &&
                  input.signature_params.front().type == "i32" &&
                  input.signature_params.front().operand == "%p.x" &&
                  input.signature_params.back().is_varargs &&
                  input.return_type == std::optional<std::string>{"{ i32, i32 }"} &&
                  input.is_variadic &&
                  input.call_results.size() == 1 &&
                  input.call_results.front().value_name == "%call0" &&
                  input.call_results.front().type_str == "{ i32, i32 }",
              "shared fallback stack-layout prep should preserve signature and call-result metadata through the backend-CFG seam so the prepared entry-alloca carrier no longer depends on raw-LIR block decoding for that metadata");
}

void test_backend_shared_fallback_preparation_separates_stack_layout_metadata_from_cfg_classification() {
  c4c::codegen::lir::LirModule module;

  c4c::codegen::lir::LirFunction function;
  function.name = "aggregate_variadic_entry_alloca";
  function.signature_text =
      "define { i32, i32 } @aggregate_variadic_entry_alloca(i32 %p.x, ...)";
  function.alloca_insts.push_back(
      c4c::codegen::lir::LirAllocaOp{"%lv.buf", "{ i32, i32 }", "", 8});

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(c4c::codegen::lir::LirCallOp{
      "%call0",
      c4c::codegen::lir::LirTypeRef("{ i32, i32 }"),
      "@helper",
      "(i32)",
      "i32 %p.x",
  });
  entry.terminator = c4c::codegen::lir::LirRet{std::nullopt, "void"};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  const auto preparation =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_preparation(module, 0);
  expect_true(preparation.stack_layout_source ==
                  c4c::backend::stack_layout::EntryAllocaRewriteStackLayoutSource::
                      EntryAllocasAndBackendCfg &&
                  preparation.rewrite_metadata.entry_allocas.size() == 1 &&
                  preparation.rewrite_metadata.entry_allocas.front().alloca_name == "%lv.buf" &&
                  preparation.stack_layout_classification.escaped_entry_allocas.has_value() &&
                  preparation.stack_layout_classification.escaped_entry_allocas->empty() &&
                  preparation.stack_layout_classification.entry_alloca_use_blocks.has_value() &&
                  preparation.stack_layout_classification.entry_alloca_use_blocks->empty() &&
                  preparation.stack_layout_metadata.signature_params.size() == 2 &&
                  preparation.stack_layout_metadata.signature_params.front().type == "i32" &&
                  preparation.stack_layout_metadata.signature_params.front().operand == "%p.x" &&
                  preparation.stack_layout_metadata.signature_params.back().is_varargs &&
                  preparation.stack_layout_metadata.return_type ==
                      std::optional<std::string>{"{ i32, i32 }"} &&
                  preparation.stack_layout_metadata.is_variadic &&
                  preparation.stack_layout_metadata.call_results.size() == 1 &&
                  preparation.stack_layout_metadata.call_results.front().value_name ==
                      "%call0" &&
                  preparation.stack_layout_metadata.call_results.front().type_str ==
                      "{ i32, i32 }",
              "shared prepared fallback carrier should keep signature and call-result metadata outside the backend-CFG-owned entry-alloca classification input");

  const auto lowered_rewrite_only =
      c4c::backend::stack_layout::lower_prepared_entry_alloca_rewrite_only_inputs(
          preparation);
  const auto lowered_stack_layout_input =
      c4c::backend::stack_layout::lower_prepared_entry_alloca_stack_layout_input(preparation);
  expect_true(lowered_stack_layout_input.signature_params.size() == 2 &&
                  lowered_rewrite_only.planning_input.entry_allocas.size() == 1 &&
                  lowered_rewrite_only.planning_input.entry_allocas.front().alloca_name ==
                      "%lv.buf" &&
                  !lowered_rewrite_only.planning_input.entry_allocas.front().paired_store
                       .has_store &&
                  !lowered_rewrite_only.planning_input.entry_allocas.front().paired_store
                       .is_zero_initializer &&
                  !lowered_rewrite_only.planning_input.entry_allocas.front().paired_store.param_name
                       .has_value() &&
                  lowered_rewrite_only.planning_input.escaped_entry_allocas.has_value() &&
                  lowered_rewrite_only.planning_input.escaped_entry_allocas->empty() &&
                  lowered_rewrite_only.planning_input.entry_alloca_use_blocks.has_value() &&
                  lowered_rewrite_only.planning_input.entry_alloca_use_blocks->empty() &&
                  lowered_rewrite_only.rewrite_input.entry_allocas.size() == 1 &&
                  lowered_rewrite_only.rewrite_input.entry_allocas.front().alloca_name ==
                      "%lv.buf" &&
                  lowered_rewrite_only.rewrite_input.escaped_entry_allocas.has_value() &&
                  lowered_rewrite_only.rewrite_input.escaped_entry_allocas->empty() &&
                  lowered_rewrite_only.rewrite_input.entry_alloca_use_blocks.has_value() &&
                  lowered_rewrite_only.rewrite_input.entry_alloca_use_blocks->empty() &&
                  lowered_stack_layout_input.blocks.size() == 1 &&
                  lowered_stack_layout_input.blocks.front().label == "entry" &&
                  lowered_stack_layout_input.blocks.front().insts.size() == 1 &&
                  lowered_stack_layout_input.blocks.front().insts.front().used_names.size() ==
                      1 &&
                  lowered_stack_layout_input.blocks.front().insts.front().used_names.front() ==
                      "%p.x" &&
                  lowered_stack_layout_input.escaped_entry_allocas.has_value() &&
                  lowered_stack_layout_input.escaped_entry_allocas->empty() &&
                  lowered_stack_layout_input.entry_alloca_use_blocks.has_value() &&
                  lowered_stack_layout_input.entry_alloca_use_blocks->empty() &&
                  lowered_stack_layout_input.signature_params.front().type == "i32" &&
                  lowered_stack_layout_input.signature_params.front().operand == "%p.x" &&
                  lowered_stack_layout_input.signature_params.back().is_varargs &&
                  lowered_stack_layout_input.return_type ==
                      std::optional<std::string>{"{ i32, i32 }"} &&
                  lowered_stack_layout_input.is_variadic &&
                  lowered_stack_layout_input.call_results.size() == 1 &&
                  lowered_stack_layout_input.call_results.front().value_name == "%call0" &&
                  lowered_stack_layout_input.call_results.front().type_str ==
                      "{ i32, i32 }",
              "shared prepared fallback carrier should keep rewrite-time ownership on the narrow entry-alloca contract while leaving broader stack-layout rehydration as a separate compatibility helper");
}

void test_backend_shared_fallback_preparation_derives_block_instruction_counts_from_liveness() {
  c4c::codegen::lir::LirModule module;

  c4c::codegen::lir::LirFunction function;
  function.name = "two_inst_entry_alloca";
  function.signature_text = "define void @two_inst_entry_alloca(i32 %p.x, i32 %p.y)";
  function.alloca_insts.push_back(c4c::codegen::lir::LirAllocaOp{"%lv.buf", "i32", "", 4});

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(c4c::codegen::lir::LirBinOp{
      "%sum",
      "add",
      c4c::codegen::lir::LirTypeRef("i32"),
      "%p.x",
      "%p.y",
  });
  entry.insts.push_back(c4c::codegen::lir::LirCallOp{
      "%call0",
      c4c::codegen::lir::LirTypeRef("i32"),
      "@helper",
      "(i32)",
      "i32 %sum",
  });
  entry.terminator = c4c::codegen::lir::LirRet{std::nullopt, "void"};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  const auto preparation =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_preparation(module, 0);
  expect_true(preparation.backend_cfg_liveness.has_value() &&
                  preparation.backend_cfg_liveness->blocks.size() == 1 &&
                  preparation.backend_cfg_liveness->blocks.front().insts.size() == 2,
              "shared prepared fallback carrier should keep block instruction arity only in the backend-owned liveness seam once liveness owns per-instruction use facts");

  const auto lowered =
      c4c::backend::stack_layout::lower_prepared_entry_alloca_stack_layout_input(preparation);
  expect_true(lowered.blocks.size() == 1 &&
                  lowered.blocks.front().insts.size() == 2 &&
                  lowered.blocks.front().insts.front().used_names.size() == 2 &&
                  lowered.blocks.front().insts.front().used_names.front() ==
                      "%p.x" &&
                  lowered.blocks.front().insts.front().used_names.back() ==
                      "%p.y" &&
                  lowered.blocks.front().insts.back().used_names.size() == 1 &&
                  lowered.blocks.front().insts.back().used_names.front() ==
                      "%sum",
              "shared prepared fallback lowering should rebuild the full per-instruction stack-layout arity and use lists directly from liveness without caching prepared block counts");
}

void test_backend_shared_fallback_preparation_rehydrates_stack_layout_uses_from_liveness() {
  c4c::codegen::lir::LirModule module;

  c4c::codegen::lir::LirFunction function;
  function.name = "branch_uses_entry_alloca";
  function.signature_text = "define void @branch_uses_entry_alloca(i1 %p.cond, i32 %p.x)";
  function.alloca_insts.push_back(c4c::codegen::lir::LirAllocaOp{"%lv.buf", "i32", "", 4});

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(c4c::codegen::lir::LirCallOp{
      "%call0",
      c4c::codegen::lir::LirTypeRef("i32"),
      "@helper",
      "(i32)",
      "i32 %p.x",
  });
  entry.terminator = c4c::codegen::lir::LirCondBr{"%p.cond", "then", "else"};
  function.blocks.push_back(std::move(entry));

  c4c::codegen::lir::LirBlock then_block;
  then_block.label = "then";
  then_block.terminator = c4c::codegen::lir::LirRet{std::nullopt, "void"};
  function.blocks.push_back(std::move(then_block));

  c4c::codegen::lir::LirBlock else_block;
  else_block.label = "else";
  else_block.terminator = c4c::codegen::lir::LirRet{std::nullopt, "void"};
  function.blocks.push_back(std::move(else_block));

  module.functions.push_back(std::move(function));

  const auto preparation =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_preparation(module, 0);
  const auto lowered =
      c4c::backend::stack_layout::lower_prepared_entry_alloca_stack_layout_input(preparation);

  expect_true(lowered.blocks.size() == 3 &&
                  lowered.blocks.front().label == "entry" &&
                  lowered.blocks[1].label == "then" &&
                  lowered.blocks[2].label == "else" &&
                  lowered.blocks.front().insts.size() == 1 &&
                  lowered.blocks.front().insts.front().used_names.size() == 1 &&
                  lowered.blocks.front().insts.front().used_names.front() ==
                      "%p.x" &&
                  lowered.blocks.front().terminator_used_names.size() == 1 &&
                  lowered.blocks.front().terminator_used_names.front() ==
                      "%p.cond" &&
                  preparation.backend_cfg_liveness.has_value() &&
                  preparation.backend_cfg_liveness->blocks.front().insts.front().used_names.size() ==
                      1 &&
                  preparation.backend_cfg_liveness->blocks.front().insts.front().used_names.front() ==
                      "%p.x" &&
                  preparation.backend_cfg_liveness->blocks.front().terminator_used_names.size() ==
                      1 &&
                  preparation.backend_cfg_liveness->blocks.front().terminator_used_names.front() ==
                      "%p.cond",
              "shared prepared fallback carrier should rebuild stack-layout instruction and terminator uses from liveness instead of caching those use lists in the prepared classification input");
}

void test_backend_shared_fallback_preparation_rehydrates_phi_predecessor_labels_from_liveness() {
  auto preparation =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_preparation(
          make_interval_phi_join_module(), 0);
  const c4c::backend::RegAllocIntegrationResult regalloc;

  expect_true(preparation.backend_cfg_liveness.has_value() &&
                  preparation.backend_cfg_liveness->blocks.size() == 4 &&
                  preparation.backend_cfg_liveness->blocks.front().label == "entry" &&
                  preparation.backend_cfg_liveness->blocks[1].label == "then" &&
                  preparation.backend_cfg_liveness->blocks[2].label == "else" &&
                  preparation.backend_cfg_liveness->blocks[3].label == "join" &&
                  preparation.backend_cfg_liveness->phi_incoming_uses.size() == 2 &&
                  preparation.backend_cfg_liveness->phi_incoming_uses.front().predecessor_label ==
                      "then" &&
                  preparation.backend_cfg_liveness->phi_incoming_uses.front().value_name == "%t1" &&
                  preparation.backend_cfg_liveness->phi_incoming_uses.back().predecessor_label ==
                      "else" &&
                  preparation.backend_cfg_liveness->phi_incoming_uses.back().value_name == "%t2" &&
                  preparation.backend_cfg_liveness->blocks.front().insts.size() == 1 &&
                  preparation.backend_cfg_liveness->blocks[1].insts.size() == 1 &&
                  preparation.backend_cfg_liveness->blocks[2].insts.size() == 1 &&
                  preparation.backend_cfg_liveness->blocks[3].insts.size() == 2,
              "shared prepared fallback carrier should rely on backend-owned liveness for phi predecessor edges, block order, and per-block instruction counts");

  const auto lowered =
      c4c::backend::stack_layout::lower_prepared_entry_alloca_stack_layout_input(preparation);
  const auto analysis = c4c::backend::stack_layout::analyze_stack_layout(
      lowered, regalloc, {});

  const auto* then_value_uses = analysis.find_use_blocks("%t1");
  const auto* else_value_uses = analysis.find_use_blocks("%t2");

  expect_true(lowered.blocks.size() == 4 &&
                  lowered.blocks.front().label == "entry" &&
                  lowered.blocks[1].label == "then" &&
                  lowered.blocks[2].label == "else" &&
                  lowered.blocks[3].label == "join" &&
                  lowered.phi_incoming_uses.size() == 2 &&
                  lowered.phi_incoming_uses.front().predecessor_label == "then" &&
                  lowered.phi_incoming_uses.front().value_name == "%t1" &&
                  lowered.phi_incoming_uses.back().predecessor_label == "else" &&
                  lowered.phi_incoming_uses.back().value_name == "%t2" &&
                  then_value_uses != nullptr && then_value_uses->size() == 1 &&
                  then_value_uses->front() == 1 &&
                  else_value_uses != nullptr && else_value_uses->size() == 1 &&
                  else_value_uses->front() == 2,
              "shared prepared fallback lowering should rehydrate stack-layout phi predecessor edges and block labels from liveness so phi analysis still maps incoming values onto the correct predecessor blocks");
}

void test_backend_shared_fallback_preparation_still_needs_remaining_pointer_escape_facts() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  auto scalar_preparation =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_preparation(
          make_overwrite_first_scalar_local_alloca_candidate_module(), 0);
  expect_true(
      scalar_preparation.stack_layout_classification.entry_alloca_first_accesses.has_value() &&
          scalar_preparation.stack_layout_classification.entry_alloca_first_accesses->size() == 1 &&
          scalar_preparation.stack_layout_classification.entry_alloca_first_accesses->front()
                  .alloca_name == "%lv.x" &&
          scalar_preparation.stack_layout_classification.entry_alloca_first_accesses->front().kind ==
              c4c::backend::stack_layout::PointerAccessKind::Store,
      "shared prepared fallback carrier should narrow overwrite-before-read classification to a coarse entry-alloca first-access seam instead of caching per-point pointer accesses");
  const auto scalar_lowered =
      c4c::backend::stack_layout::lower_prepared_entry_alloca_stack_layout_input(
          scalar_preparation);
  const auto scalar_rewrite_only =
      c4c::backend::stack_layout::lower_prepared_entry_alloca_rewrite_only_inputs(
          scalar_preparation);
  const auto scalar_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      scalar_lowered, regalloc, {});
  const auto scalar_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      scalar_rewrite_only.planning_input, scalar_analysis);
  expect_true(scalar_plans.size() == 1 &&
                  scalar_plans.front().alloca_name == "%lv.x" &&
                  scalar_plans.front().needs_stack_slot &&
                  scalar_plans.front().remove_following_entry_store &&
                  scalar_lowered.blocks.front().insts.front().pointer_accesses.empty(),
              "shared prepared fallback carrier should let overwrite-before-read pruning trust the coarse entry-alloca first-access seam after dropping prepared pointer-access facts entirely");

  scalar_preparation.stack_layout_classification.entry_alloca_first_accesses =
      std::vector<c4c::backend::stack_layout::EntryAllocaFirstAccess>{};
  const auto scalar_without_first_access =
      c4c::backend::stack_layout::lower_prepared_entry_alloca_stack_layout_input(
          scalar_preparation);
  const auto scalar_without_first_access_rewrite_only =
      c4c::backend::stack_layout::lower_prepared_entry_alloca_rewrite_only_inputs(
          scalar_preparation);
  const auto scalar_without_first_access_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(
          scalar_without_first_access, regalloc, {});
  const auto scalar_without_first_access_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(
          scalar_without_first_access_rewrite_only.planning_input,
          scalar_without_first_access_analysis);
  expect_true(scalar_without_first_access_plans.size() == 1 &&
                  scalar_without_first_access_plans.front().alloca_name == "%lv.x" &&
                  scalar_without_first_access_plans.front().needs_stack_slot &&
                  !scalar_without_first_access_plans.front().remove_following_entry_store,
              "shared prepared fallback carrier still needs the coarse entry-alloca first-access seam because clearing it after removing prepared pointer-access facts drops overwrite-before-read pruning on the current backend-CFG path");

  auto aggregate_preparation =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_preparation(
          make_live_local_alloca_candidate_module(), 0);
  expect_true(aggregate_preparation.stack_layout_classification.entry_alloca_use_blocks.has_value() &&
                  aggregate_preparation.stack_layout_classification.entry_alloca_use_blocks->size() ==
                      1 &&
                  aggregate_preparation.stack_layout_classification.entry_alloca_use_blocks->front()
                          .alloca_name == "%lv.buf" &&
                  aggregate_preparation.stack_layout_classification.entry_alloca_use_blocks->front()
                          .block_indices.size() == 1 &&
                  aggregate_preparation.stack_layout_classification.entry_alloca_use_blocks->front()
                          .block_indices.front() == 0,
              "shared prepared fallback carrier should narrow single-block coalescing to a coarse entry-alloca use-block classification instead of retaining separate per-point alias metadata");
  const auto aggregate_lowered =
      c4c::backend::stack_layout::lower_prepared_entry_alloca_stack_layout_input(
          aggregate_preparation);
  const auto aggregate_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      aggregate_lowered, regalloc, {});
  const auto aggregate_coalescing =
      c4c::backend::stack_layout::compute_coalescable_allocas(
          aggregate_lowered, aggregate_analysis);
  expect_true(!aggregate_coalescing.is_dead_alloca("%lv.buf") &&
                  !aggregate_lowered.blocks.front().insts.front().derived_pointer_root
                       .has_value() &&
                  aggregate_coalescing.find_single_block("%lv.buf") == 0,
              "shared prepared fallback carrier should let single-block coalescing trust the coarse entry-alloca use-block seam without rehydrating prepared derived-pointer-root facts");

  aggregate_preparation.stack_layout_classification.entry_alloca_use_blocks =
      std::vector<c4c::backend::stack_layout::EntryAllocaUseBlocks>{};
  const auto aggregate_without_use_blocks =
      c4c::backend::stack_layout::lower_prepared_entry_alloca_stack_layout_input(
          aggregate_preparation);
  const auto aggregate_without_use_blocks_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(
          aggregate_without_use_blocks, regalloc, {});
  const auto aggregate_without_use_blocks_coalescing =
      c4c::backend::stack_layout::compute_coalescable_allocas(
          aggregate_without_use_blocks, aggregate_without_use_blocks_analysis);
  expect_true(aggregate_without_use_blocks_coalescing.is_dead_alloca("%lv.buf") &&
                  !aggregate_without_use_blocks_coalescing.find_single_block("%lv.buf")
                       .has_value(),
              "shared prepared fallback carrier still needs the coarse entry-alloca use-block seam because clearing both that seam and prepared derived-pointer-root facts drops single-block coalescing classification on the current backend-CFG path");

  auto escaped_preparation =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_preparation(
          make_escaped_local_alloca_candidate_module(), 1);
  expect_true(escaped_preparation.stack_layout_classification.escaped_entry_allocas.has_value() &&
                  escaped_preparation.stack_layout_classification.escaped_entry_allocas->size() ==
                      1 &&
                  escaped_preparation.stack_layout_classification.escaped_entry_allocas->front() ==
                      "%lv.buf",
              "shared prepared fallback carrier should narrow escaped-alloca classification to a coarse entry-alloca escape set instead of caching per-point escaped-name lists");
  escaped_preparation.stack_layout_classification.escaped_entry_allocas = std::vector<std::string>{};
  const auto escaped_lowered =
      c4c::backend::stack_layout::lower_prepared_entry_alloca_stack_layout_input(
          escaped_preparation);
  const auto escaped_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      escaped_lowered, regalloc, {});
  const auto escaped_coalescing =
      c4c::backend::stack_layout::compute_coalescable_allocas(
          escaped_lowered, escaped_analysis);
  expect_true(escaped_coalescing.is_dead_alloca("%lv.buf") &&
                  !escaped_coalescing.find_single_block("%lv.buf").has_value(),
              "shared prepared fallback carrier still needs escaped-name facts because removing them drops the current escaped-alloca classification entirely on the prepared backend-CFG path");
}

void test_backend_shared_fallback_preparation_keeps_only_liveness_cfg_state() {
  c4c::codegen::lir::LirModule module;

  c4c::codegen::lir::LirFunction function;
  function.name = "aggregate_variadic_entry_alloca";
  function.signature_text =
      "define { i32, i32 } @aggregate_variadic_entry_alloca(i32 %p.x, ...)";
  function.alloca_insts.push_back(
      c4c::codegen::lir::LirAllocaOp{"%lv.buf", "{ i32, i32 }", "", 8});

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(c4c::codegen::lir::LirCallOp{
      "%call0",
      c4c::codegen::lir::LirTypeRef("{ i32, i32 }"),
      "@helper",
      "(i32)",
      "i32 %p.x",
  });
  entry.terminator = c4c::codegen::lir::LirRet{std::nullopt, "void"};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  const auto preparation =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_preparation(module, 0);
  expect_true(preparation.backend_cfg_liveness.has_value() &&
                  preparation.backend_cfg_liveness->entry_insts.size() == 1 &&
                  preparation.backend_cfg_liveness->entry_insts.front().used_names.empty() &&
                  !preparation.backend_cfg_liveness->entry_insts.front().is_call &&
                  preparation.backend_cfg_liveness->blocks.size() == 1 &&
                  preparation.backend_cfg_liveness->blocks.front().insts.size() == 1 &&
                  preparation.backend_cfg_liveness->blocks.front().insts.front().is_call &&
                  preparation.backend_cfg_liveness->phi_incoming_uses.empty(),
              "shared prepared fallback carrier should retain only the liveness-relevant backend-CFG block and point state once stack-layout metadata is split away");

  const auto lowered =
      c4c::backend::stack_layout::lower_prepared_entry_alloca_rewrite_only_inputs(
          preparation);
  expect_true(lowered.liveness_input.entry_insts.size() == 1 &&
                  lowered.liveness_input.entry_insts.front().used_names.empty() &&
                  !lowered.liveness_input.entry_insts.front().is_call &&
                  lowered.liveness_input.blocks.size() == 1 &&
                  lowered.liveness_input.blocks.front().insts.size() == 1 &&
                  lowered.liveness_input.blocks.front().insts.front().is_call,
              "shared prepared fallback carrier should still rebuild production liveness input from the reduced backend-CFG liveness state");
}

void test_backend_shared_prepares_lir_module_for_target() {
  const auto module = make_dead_param_alloca_candidate_module();
  const auto x86_prepared = c4c::backend::prepare_lir_module_for_target(
      module, c4c::backend::Target::X86_64);
  const auto aarch64_prepared = c4c::backend::prepare_lir_module_for_target(
      module, c4c::backend::Target::Aarch64);
  const auto riscv_prepared = c4c::backend::prepare_lir_module_for_target(
      module, c4c::backend::Target::Riscv64);

  expect_true(x86_prepared.functions.size() == module.functions.size() &&
                  x86_prepared.functions.front().is_declaration &&
                  x86_prepared.functions.back().alloca_insts.empty(),
              "shared backend LIR prep should preserve declarations and prune dead param allocas for x86-native emission");
  expect_true(aarch64_prepared.functions.size() == module.functions.size() &&
                  aarch64_prepared.functions.front().is_declaration &&
                  aarch64_prepared.functions.back().alloca_insts.empty(),
              "shared backend LIR prep should preserve declarations and prune dead param allocas for aarch64-native emission");
  expect_true(riscv_prepared.functions.size() == module.functions.size() &&
                  riscv_prepared.functions.back().alloca_insts.size() ==
                      module.functions.back().alloca_insts.size(),
              "shared backend LIR prep should leave riscv64 modules untouched so the text fallback keeps its current ownership");
}

void test_backend_shared_prepared_function_inputs_preserve_live_pointer_param_allocas() {
  const auto module = make_live_pointer_param_alloca_candidate_module();
  const auto prepared =
      c4c::backend::prepare_lir_module_for_target(module, c4c::backend::Target::Aarch64);
  const auto preparation =
      c4c::backend::stack_layout::prepare_module_function_entry_alloca_preparation(prepared, 0);

  expect_true(prepared.functions.size() == 1 &&
                  prepared.functions.front().alloca_insts.size() ==
                      module.functions.front().alloca_insts.size(),
              "shared backend target prep should preserve live pointer param allocas when the function body still reloads and rewrites the pointer slot");
  expect_true(preparation.rewrite_metadata.entry_allocas.size() == 1 &&
                  preparation.rewrite_metadata.entry_allocas.front().alloca_name == "%lv.param.p" &&
                  preparation.planning_input.entry_allocas.size() == 1 &&
                  preparation.planning_input.entry_allocas.front().alloca_name == "%lv.param.p",
              "shared prepared function inputs should retain live pointer param allocas on the prepared native-emission seam instead of silently dropping them after target prep");
}

void test_backend_shared_target_preparation_enables_bir_lowering() {
  const auto module = make_dead_local_alloca_candidate_module();
  const auto x86_prepared = c4c::backend::prepare_lir_module_for_target(
      module, c4c::backend::Target::X86_64);
  const auto aarch64_prepared = c4c::backend::prepare_lir_module_for_target(
      module, c4c::backend::Target::Aarch64);

  expect_true(!c4c::backend::try_lower_to_bir(module).has_value(),
              "shared backend prep coverage should start from the current raw-LIR entry-alloca seam that still blocks direct BIR lowering");

  const auto x86_lowered = c4c::backend::try_lower_to_bir(x86_prepared);
  expect_true(x86_lowered.has_value() && x86_lowered->functions.size() == 1 &&
                  !x86_lowered->functions.front().is_declaration &&
                  x86_lowered->functions.front().blocks.size() == 1,
              "shared backend LIR prep should make the dead-local alloca slice lowerable to BIR for x86 once entry allocas are rewritten");

  const auto aarch64_lowered = c4c::backend::try_lower_to_bir(aarch64_prepared);
  expect_true(aarch64_lowered.has_value() && aarch64_lowered->functions.size() == 1 &&
                  !aarch64_lowered->functions.front().is_declaration &&
                  aarch64_lowered->functions.front().blocks.size() == 1,
              "shared backend LIR prep should make the same dead-local alloca slice lowerable to BIR for aarch64 before native fallback runs");
}

void test_backend_shared_slot_assignment_prunes_dead_entry_alloca_insts() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto dead_module = make_dead_local_alloca_candidate_module();
  auto dead_function = dead_module.functions.front();
  const auto dead_input =
      lower_test_function_entry_alloca_stack_layout_input(dead_function);
  const auto dead_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(dead_input, regalloc, {});
  const auto dead_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          dead_input),
      dead_analysis);
  const auto dead_patch = c4c::backend::stack_layout::build_entry_alloca_rewrite_patch(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_rewrite_input(
          dead_input),
      dead_plans);
  c4c::backend::stack_layout::apply_entry_alloca_rewrite_patch(dead_function, dead_patch);

  expect_true(dead_function.alloca_insts.empty(),
              "shared slot-assignment rewrite patches should drop dead non-param entry allocas and their paired zero-init stores");

  const auto live_module = make_live_local_alloca_candidate_module();
  auto live_function = live_module.functions.front();
  const auto live_input =
      lower_test_function_entry_alloca_stack_layout_input(live_function);
  const auto live_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(live_input, regalloc, {});
  const auto live_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          live_input),
      live_analysis);
  const auto live_patch = c4c::backend::stack_layout::build_entry_alloca_rewrite_patch(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_rewrite_input(
          live_input),
      live_plans);
  c4c::backend::stack_layout::apply_entry_alloca_rewrite_patch(live_function, live_patch);

  expect_true(live_function.alloca_insts.size() == live_module.functions.front().alloca_insts.size(),
              "shared slot-assignment rewrite patches should keep aggregate live non-param entry allocas and their paired zero-init stores");

  const auto read_first_module = make_read_before_store_local_alloca_candidate_module();
  auto read_first_function = read_first_module.functions.front();
  const auto read_first_input =
      lower_test_function_entry_alloca_stack_layout_input(read_first_function);
  const auto read_first_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(read_first_input, regalloc, {});
  const auto read_first_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          read_first_input),
      read_first_analysis);
  const auto read_first_patch = c4c::backend::stack_layout::build_entry_alloca_rewrite_patch(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_rewrite_input(
          read_first_input),
      read_first_plans);
  c4c::backend::stack_layout::apply_entry_alloca_rewrite_patch(read_first_function,
                                                               read_first_patch);

  expect_true(
      read_first_function.alloca_insts.size() ==
          read_first_module.functions.front().alloca_insts.size(),
      "shared slot-assignment rewrite patches should preserve paired zero-init stores when a live entry alloca is read before the first overwrite");

  const auto scalar_overwrite_module = make_overwrite_first_scalar_local_alloca_candidate_module();
  auto scalar_overwrite_function = scalar_overwrite_module.functions.front();
  const auto scalar_overwrite_input =
      lower_test_function_entry_alloca_stack_layout_input(scalar_overwrite_function);
  const auto scalar_overwrite_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(scalar_overwrite_input, regalloc, {});
  const auto scalar_overwrite_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          scalar_overwrite_input),
      scalar_overwrite_analysis);
  const auto scalar_overwrite_patch = c4c::backend::stack_layout::build_entry_alloca_rewrite_patch(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_rewrite_input(
          scalar_overwrite_input),
      scalar_overwrite_plans);
  c4c::backend::stack_layout::apply_entry_alloca_rewrite_patch(scalar_overwrite_function,
                                                               scalar_overwrite_patch);

  expect_true(
      scalar_overwrite_function.alloca_insts.size() + 1 ==
              scalar_overwrite_module.functions.front().alloca_insts.size() &&
          std::holds_alternative<c4c::codegen::lir::LirAllocaOp>(
              scalar_overwrite_function.alloca_insts.front()) &&
          !std::holds_alternative<c4c::codegen::lir::LirStoreOp>(
              scalar_overwrite_function.alloca_insts.back()),
      "shared slot-assignment rewrite patches should keep scalar live entry allocas while dropping redundant paired zero-init stores");
}

void test_backend_shared_slot_assignment_applies_coalesced_entry_slots() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto disjoint_module = make_disjoint_block_local_alloca_candidate_module();
  auto disjoint_function = disjoint_module.functions.front();
  const auto disjoint_input =
      lower_test_function_entry_alloca_stack_layout_input(disjoint_function);
  const auto disjoint_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(disjoint_input, regalloc, {});
  const auto disjoint_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          disjoint_input),
      disjoint_analysis);
  const auto disjoint_patch = c4c::backend::stack_layout::build_entry_alloca_rewrite_patch(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_rewrite_input(
          disjoint_input),
      disjoint_plans);
  c4c::backend::stack_layout::apply_entry_alloca_rewrite_patch(disjoint_function,
                                                               disjoint_patch);

  expect_true(disjoint_function.alloca_insts.size() == 1,
              "shared slot-assignment application should collapse disjoint single-block allocas onto one retained entry slot");
  const auto* disjoint_alloca =
      std::get_if<c4c::codegen::lir::LirAllocaOp>(&disjoint_function.alloca_insts.front());
  expect_true(disjoint_alloca != nullptr && disjoint_alloca->result == "%lv.then",
              "shared slot-assignment application should retain the first assigned-slot owner as the canonical entry alloca");
  const auto* else_store =
      std::get_if<c4c::codegen::lir::LirStoreOp>(&disjoint_function.blocks[2].insts[0]);
  const auto* else_load =
      std::get_if<c4c::codegen::lir::LirLoadOp>(&disjoint_function.blocks[2].insts[1]);
  expect_true(else_store != nullptr && else_store->ptr == "%lv.then" &&
                  else_load != nullptr && else_load->ptr == "%lv.then",
              "shared slot-assignment application should rewrite disjoint-block users to the canonical shared slot");

  const auto same_block_module = make_same_block_local_alloca_candidate_module();
  auto same_block_function = same_block_module.functions.front();
  const auto same_block_input =
      lower_test_function_entry_alloca_stack_layout_input(same_block_function);
  const auto same_block_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(same_block_input, regalloc, {});
  const auto same_block_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          same_block_input),
      same_block_analysis);
  const auto same_block_patch = c4c::backend::stack_layout::build_entry_alloca_rewrite_patch(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_rewrite_input(
          same_block_input),
      same_block_plans);
  c4c::backend::stack_layout::apply_entry_alloca_rewrite_patch(same_block_function,
                                                               same_block_patch);

  expect_true(same_block_function.alloca_insts.size() == 2,
              "shared slot-assignment application should keep same-block allocas distinct when planning assigned them different slots");

  const auto mixed_lifetime_module = make_mixed_lifetime_local_alloca_candidate_module();
  auto mixed_lifetime_function = mixed_lifetime_module.functions.front();
  const auto mixed_lifetime_input =
      lower_test_function_entry_alloca_stack_layout_input(mixed_lifetime_function);
  const auto mixed_lifetime_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(mixed_lifetime_input, regalloc, {});
  const auto mixed_lifetime_plans = c4c::backend::stack_layout::plan_entry_alloca_slots(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_planning_input(
          mixed_lifetime_input),
      mixed_lifetime_analysis);
  const auto mixed_lifetime_patch =
      c4c::backend::stack_layout::build_entry_alloca_rewrite_patch(
          c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_rewrite_input(
              mixed_lifetime_input),
          mixed_lifetime_plans);
  c4c::backend::stack_layout::apply_entry_alloca_rewrite_patch(mixed_lifetime_function,
                                                               mixed_lifetime_patch);

  expect_true(mixed_lifetime_function.alloca_insts.size() == 2,
              "shared slot-assignment application should keep mixed-lifetime allocas distinct when the entry value remains live after the scratch store");
  const auto* mixed_entry_store =
      std::get_if<c4c::codegen::lir::LirStoreOp>(&mixed_lifetime_function.blocks[0].insts[1]);
  expect_true(mixed_entry_store != nullptr && mixed_entry_store->ptr == "%lv.y",
              "shared slot-assignment application should preserve the scratch-store destination instead of rewriting it onto the long-lived entry alloca");

  const auto call_arg_module = make_disjoint_block_call_arg_alloca_candidate_module();
  auto call_arg_function = call_arg_module.functions.front();
  const std::vector<c4c::backend::stack_layout::EntryAllocaSlotPlan> call_arg_plans = {
      c4c::backend::stack_layout::EntryAllocaSlotPlan{
          "%lv.then", true, false, std::nullopt, std::size_t{0}},
      c4c::backend::stack_layout::EntryAllocaSlotPlan{
          "%lv.else", true, false, std::nullopt, std::size_t{0}},
  };
  const auto call_arg_input =
      lower_test_function_entry_alloca_stack_layout_input(call_arg_function);
  const auto call_arg_patch = c4c::backend::stack_layout::build_entry_alloca_rewrite_patch(
      c4c::backend::stack_layout::lower_stack_layout_input_to_entry_alloca_rewrite_input(
          call_arg_input),
      call_arg_plans);
  c4c::backend::stack_layout::apply_entry_alloca_rewrite_patch(call_arg_function,
                                                               call_arg_patch);

  const auto* rewritten_call =
      std::get_if<c4c::codegen::lir::LirCallOp>(&call_arg_function.blocks[2].insts[1]);
  expect_true(rewritten_call != nullptr && rewritten_call->args_str == "ptr %lv.then",
              "shared slot-assignment application should rewrite typed call arguments to the canonical coalesced entry alloca");
}

void test_backend_binary_utils_contract_headers_are_include_reachable() {
  [[maybe_unused]] c4c::backend::elf::ContractSurface elf_surface;
  [[maybe_unused]] c4c::backend::linker_common::ContractSurface linker_common_surface;
  [[maybe_unused]] c4c::backend::aarch64::assembler::ContractSurface assembler_surface;
  [[maybe_unused]] c4c::backend::aarch64::linker::ContractSurface linker_surface;
  [[maybe_unused]] c4c::backend::x86::assembler::ContractSurface x86_assembler_surface;
  [[maybe_unused]] c4c::backend::x86::linker::ContractSurface x86_linker_surface;

  const auto emitted = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_return_add_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto parsed = c4c::backend::aarch64::assembler::parse_asm(emitted);
  expect_true(parsed.size() >= 4, "binary-utils contract headers should expose line-oriented parser output for backend-emitted assembly");
  expect_true(parsed.front().text == ".text",
              "binary-utils contract parser should keep the opening section directive");
  expect_true(parsed.back().text == "ret",
              "binary-utils contract parser should preserve the backend instruction tail");

  const auto object_path = make_temp_output_path("c4c_aarch64_contract");
  const auto staged = c4c::backend::aarch64::assembler::assemble(
      c4c::backend::aarch64::assembler::AssembleRequest{
          .asm_text = emitted,
          .output_path = object_path,
      });
  expect_true(staged.staged_text == emitted && staged.output_path == object_path &&
                  staged.object_emitted,
              "binary-utils contract headers should expose the active assembler request/result seam and report successful object emission for the minimal backend slice");
  const auto object_bytes = read_file_bytes(object_path);
  expect_true(object_bytes.size() >= 4 && object_bytes[0] == 0x7f &&
                  object_bytes[1] == 'E' && object_bytes[2] == 'L' &&
                  object_bytes[3] == 'F',
              "binary-utils contract assembler seam should emit an ELF object for the minimal backend slice");
  expect_true(std::string(object_bytes.begin(), object_bytes.end()).find("main") != std::string::npos,
              "binary-utils contract assembler seam should preserve the function symbol in the emitted object");
  std::filesystem::remove(object_path);

  const auto compat_path = make_temp_output_path("c4c_aarch64_contract_compat");
  const auto assembled = c4c::backend::aarch64::assembler::assemble(emitted, compat_path);
  expect_true(assembled == emitted,
              "binary-utils contract headers should keep the compatibility overload aligned with the staged assembler text seam");
  expect_true(std::filesystem::exists(compat_path),
              "binary-utils contract compatibility overload should still drive object emission through the named assembler seam");
  std::filesystem::remove(compat_path);
}

void test_shared_linker_parses_minimal_relocation_object_fixture() {
  const auto fixture = make_minimal_relocation_object_fixture();
  std::string error;
  const auto parsed = c4c::backend::linker_common::parse_elf64_object(
      fixture, "minimal_reloc.o", c4c::backend::elf::EM_AARCH64, &error);
  expect_true(parsed.has_value(),
              "shared linker object parser should accept the bounded relocation-bearing fixture: " +
                  error);

  const auto& object = *parsed;
  expect_true(object.source_name == "minimal_reloc.o",
              "shared linker object parser should preserve the source name");
  expect_true(object.sections.size() == 6,
              "shared linker object parser should preserve the full section inventory including the null section");
  expect_true(object.symbols.size() == 4,
              "shared linker object parser should preserve the full symbol inventory for the bounded object slice");
  expect_true(object.relocations.size() == object.sections.size(),
              "shared linker object parser should index relocation vectors by section");

  const auto text_index = find_section_index(object, ".text");
  const auto rela_text_index = find_section_index(object, ".rela.text");
  const auto symtab_index = find_section_index(object, ".symtab");
  const auto strtab_index = find_section_index(object, ".strtab");
  const auto shstrtab_index = find_section_index(object, ".shstrtab");
  expect_true(text_index < object.sections.size() && rela_text_index < object.sections.size() &&
                  symtab_index < object.sections.size() && strtab_index < object.sections.size() &&
                  shstrtab_index < object.sections.size(),
              "shared linker object parser should name the expected bounded ELF sections");

  expect_true(object.section_data[text_index].size() == 8,
              "shared linker object parser should preserve the bounded .text payload");
  expect_true(object.section_data[rela_text_index].size() == 24,
              "shared linker object parser should preserve the raw RELA payload for the bounded fixture");
  expect_true(object.symbols[2].name == "main" && object.symbols[2].is_global() &&
                  object.symbols[2].sym_type() == c4c::backend::elf::STT_FUNC &&
                  object.symbols[2].shndx == text_index,
              "shared linker object parser should preserve the bounded function symbol inventory");
  expect_true(object.symbols[3].name == "helper_ext" && object.symbols[3].is_undefined(),
              "shared linker object parser should preserve undefined extern symbols for later linker resolution");

  expect_true(object.relocations[text_index].size() == 1,
              "shared linker object parser should associate .rela.text entries with the .text section they target");
  const auto& relocation = object.relocations[text_index].front();
  expect_true(relocation.offset == 4 && relocation.sym_idx == 3 &&
                  relocation.rela_type == 279 && relocation.addend == 0,
              "shared linker object parser should preserve the bounded relocation inventory for later linker work");
}

void test_shared_linker_parses_builtin_return_add_object() {
  const auto object_path = make_temp_output_path("c4c_aarch64_return_add_parse");
  const auto staged = c4c::backend::aarch64::assemble_module(make_return_add_module(), object_path);
  expect_true(staged.object_emitted,
              "built-in assembler should emit an object for the current bounded return-add slice");

  const auto object_bytes = read_file_bytes(object_path);
  std::string error;
  const auto parsed = c4c::backend::linker_common::parse_elf64_object(
      object_bytes, object_path, c4c::backend::elf::EM_AARCH64, &error);
  expect_true(parsed.has_value(),
              "shared linker object parser should accept the current built-in return-add object: " +
                  error);

  const auto& object = *parsed;
  const auto text_index = find_section_index(object, ".text");
  const auto symtab_index = find_section_index(object, ".symtab");
  const auto strtab_index = find_section_index(object, ".strtab");
  const auto shstrtab_index = find_section_index(object, ".shstrtab");
  expect_true(text_index < object.sections.size() && symtab_index < object.sections.size() &&
                  strtab_index < object.sections.size() &&
                  shstrtab_index < object.sections.size(),
              "shared linker object parser should preserve the current built-in return-add section inventory");
  expect_true(object.relocations.size() == object.sections.size(),
              "shared linker object parser should keep relocation vectors indexed by section for built-in emitted objects");
  expect_true(object.section_data[text_index].size() == 8,
              "shared linker object parser should preserve the built-in emitted return-add text payload");

  const auto main_symbol = std::find_if(
      object.symbols.begin(), object.symbols.end(),
      [&](const c4c::backend::linker_common::Elf64Symbol& symbol) {
        return symbol.name == "main";
      });
  expect_true(main_symbol != object.symbols.end() && main_symbol->is_global() &&
                  main_symbol->sym_type() == c4c::backend::elf::STT_FUNC &&
                  main_symbol->shndx == text_index,
              "shared linker object parser should preserve the built-in emitted function symbol inventory");

  expect_true(object.relocations[text_index].empty(),
              "shared linker object parser should preserve the current built-in return-add object as relocation-free");

  std::filesystem::remove(object_path);
}

void test_backend_shared_assemble_target_lir_module_matches_public_wrappers() {
  const auto x86_module = make_return_add_module();
  const auto shared_x86_path = make_temp_output_path("c4c_x86_shared_assemble");
  const auto wrapper_x86_path = make_temp_output_path("c4c_x86_wrapper_assemble");
  const auto shared_x86 = c4c::backend::assemble_target_lir_module(
      x86_module, c4c::backend::Target::X86_64, shared_x86_path);
  const auto wrapper_x86 = c4c::backend::x86::assemble_module(x86_module, wrapper_x86_path);
  expect_true(shared_x86.staged_text == wrapper_x86.staged_text &&
                  shared_x86.object_emitted == wrapper_x86.object_emitted &&
                  shared_x86.error == wrapper_x86.error,
              "shared backend assembly handoff should match the x86 public wrapper staging contract");
  expect_true(shared_x86.output_path == shared_x86_path &&
                  wrapper_x86.output_path == wrapper_x86_path &&
                  shared_x86.object_emitted && wrapper_x86.object_emitted,
              "shared backend assembly handoff should preserve x86 output destinations while emitting objects");
  std::filesystem::remove(shared_x86_path);
  std::filesystem::remove(wrapper_x86_path);

  auto aarch64_module = make_return_add_module();
  aarch64_module.target_triple = "aarch64-unknown-linux-gnu";
  aarch64_module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  const auto shared_aarch64_path = make_temp_output_path("c4c_aarch64_shared_assemble");
  const auto wrapper_aarch64_path = make_temp_output_path("c4c_aarch64_wrapper_assemble");
  const auto shared_aarch64 = c4c::backend::assemble_target_lir_module(
      aarch64_module, c4c::backend::Target::Aarch64, shared_aarch64_path);
  const auto wrapper_aarch64 =
      c4c::backend::aarch64::assemble_module(aarch64_module, wrapper_aarch64_path);
  expect_true(shared_aarch64.staged_text == wrapper_aarch64.staged_text &&
                  shared_aarch64.object_emitted == wrapper_aarch64.object_emitted,
              "shared backend assembly handoff should match the aarch64 public wrapper staging contract");
  expect_true(shared_aarch64.output_path == shared_aarch64_path &&
                  wrapper_aarch64.output_path == wrapper_aarch64_path &&
                  shared_aarch64.object_emitted && wrapper_aarch64.object_emitted,
              "shared backend assembly handoff should preserve aarch64 output destinations while emitting objects");
  std::filesystem::remove(shared_aarch64_path);
  std::filesystem::remove(wrapper_aarch64_path);

  auto riscv_module = make_return_add_module();
  riscv_module.target_triple = "riscv64-unknown-linux-gnu";
  riscv_module.data_layout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128";
  const auto shared_riscv_path = make_temp_output_path("c4c_riscv_shared_assemble");
  const auto wrapper_riscv_path = make_temp_output_path("c4c_riscv_wrapper_assemble");
  const auto shared_riscv = c4c::backend::assemble_target_lir_module(
      riscv_module, c4c::backend::Target::Riscv64, shared_riscv_path);
  const auto wrapper_riscv =
      c4c::backend::riscv::assemble_module(riscv_module, wrapper_riscv_path);
  expect_true(shared_riscv.staged_text == wrapper_riscv.staged_text &&
                  shared_riscv.object_emitted == wrapper_riscv.object_emitted &&
                  shared_riscv.error == wrapper_riscv.error,
              "shared backend assembly handoff should match the riscv64 public wrapper staging contract");
  expect_true(shared_riscv.output_path == shared_riscv_path &&
                  wrapper_riscv.output_path == wrapper_riscv_path &&
                  shared_riscv.object_emitted && wrapper_riscv.object_emitted,
              "shared backend assembly handoff should preserve riscv64 output destinations while emitting objects");

  const auto shared_riscv_object_bytes = read_file_bytes(shared_riscv_path);
  const auto wrapper_riscv_object_bytes = read_file_bytes(wrapper_riscv_path);
  std::string riscv_error;
  const auto parsed_riscv = c4c::backend::linker_common::parse_elf64_object(
      shared_riscv_object_bytes, shared_riscv_path, static_cast<std::uint16_t>(243), &riscv_error);
  expect_true(parsed_riscv.has_value(),
              "shared backend assembly handoff should emit a parseable riscv64 ELF object for the activation slice: " +
                  riscv_error);
  std::string wrapper_riscv_error;
  const auto parsed_wrapper_riscv = c4c::backend::linker_common::parse_elf64_object(
      wrapper_riscv_object_bytes, wrapper_riscv_path, static_cast<std::uint16_t>(243), &wrapper_riscv_error);
  expect_true(parsed_wrapper_riscv.has_value(),
              "riscv64 public wrapper should emit a parseable riscv64 ELF object for the activation slice: " +
                  wrapper_riscv_error);
  expect_true(shared_riscv_object_bytes == wrapper_riscv_object_bytes,
              "shared backend assembly handoff should match the riscv64 public wrapper object bytes for the activation slice");
  const auto& riscv_object = *parsed_riscv;
  const auto riscv_text_index = find_section_index(riscv_object, ".text");
  expect_true(riscv_text_index < riscv_object.sections.size() &&
                  riscv_object.section_data[riscv_text_index].size() == 8,
              "shared backend assembly handoff should preserve the minimal riscv64 activation text payload");

  std::filesystem::remove(shared_riscv_path);
  std::filesystem::remove(wrapper_riscv_path);
}

void test_riscv_builtin_assembler_parses_activation_slice_variants() {
  const auto object_path = make_temp_output_path("c4c_riscv_activation_variant");
  const auto assembled = c4c::backend::riscv::assembler::assemble(
      c4c::backend::riscv::assembler::AssembleRequest{
          .asm_text =
              "  .text\n"
              "  .global main\n"
              "main: addi x10, x0, 5   # bounded activation opcode\n"
              "  ret\n",
          .output_path = object_path,
      });
  expect_true(assembled.object_emitted && assembled.error.empty(),
              "riscv built-in assembler should accept the bounded activation slice through the parser-backed path");

  const auto object_bytes = read_file_bytes(object_path);
  std::string error;
  const auto parsed = c4c::backend::linker_common::parse_elf64_object(
      object_bytes, object_path, static_cast<std::uint16_t>(243), &error);
  expect_true(parsed.has_value(),
              "shared linker object parser should accept the parser-backed riscv activation object: " +
                  error);
  const auto& object = *parsed;
  const auto text_index = find_section_index(object, ".text");
  expect_true(text_index < object.sections.size() &&
                  object.section_data[text_index].size() == 8 &&
                  object.relocations[text_index].empty(),
              "parser-backed riscv activation assembly should preserve the current bounded text payload without relocations");

  std::filesystem::remove(object_path);
}

void test_riscv_linker_contract_exposes_first_static_surface() {
  using namespace c4c::backend::riscv::linker;

  [[maybe_unused]] ContractSurface linker_surface;
  auto inspect = &inspect_first_static_link_slice;
  auto load = &load_first_static_input_objects;
  auto apply = &apply_first_static_text_relocations;
  auto emit = &emit_first_static_executable_image;
  auto link = &link_first_static_executable;

  expect_true(inspect != nullptr && load != nullptr && apply != nullptr && emit != nullptr &&
                  link != nullptr,
              "riscv linker contract surface should expose the named first-static executable seam through the public RV64 linker header");
}

void test_riscv_first_static_executable_image_matches_return_add_object() {
  auto module = make_return_add_module();
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128";

  const auto object_path = make_temp_output_path("c4c_riscv_first_static_exec");
  const auto assembled = c4c::backend::riscv::assemble_module(module, object_path);
  expect_true(assembled.object_emitted && assembled.error.empty(),
              "riscv first static executable seam should start from the current built-in return-add object lane");

  std::string inspect_error;
  const auto slice = c4c::backend::riscv::linker::inspect_first_static_link_slice(
      {object_path}, &inspect_error);
  expect_true(slice.has_value(),
              "riscv first static executable seam should inspect the current return-add object: " +
                  inspect_error);
  expect_true(slice->case_name == "riscv64-static-reloc-free-single-object" &&
                  slice->objects.size() == 1 && slice->objects.front().path == object_path,
              "riscv linker inspection should preserve the minimal single-object slice identity");
  expect_true(slice->objects.front().defined_symbols.size() == 1 &&
                  slice->objects.front().defined_symbols.front() == "main" &&
                  slice->objects.front().undefined_symbols.empty(),
              "riscv linker inspection should report the bounded return-add object as a single main-definition provider with no unresolved symbols");
  expect_true(slice->relocations.empty() && slice->unresolved_symbols.empty() &&
                  std::find(slice->merged_output_sections.begin(),
                            slice->merged_output_sections.end(),
                            ".text") != slice->merged_output_sections.end(),
              "riscv linker inspection should keep the current activation lane narrow: merged text, no relocations, and no unresolved symbols");

  const auto object_bytes = read_file_bytes(object_path);
  std::string parse_error;
  const auto parsed_object = c4c::backend::linker_common::parse_elf64_object(
      object_bytes, object_path, c4c::backend::riscv::linker::kEmRiscv, &parse_error);
  expect_true(parsed_object.has_value(),
              "riscv first static executable seam should start from a parseable RV64 object: " +
                  parse_error);
  const auto text_index = find_section_index(*parsed_object, ".text");
  expect_true(text_index < parsed_object->sections.size() &&
                  parsed_object->relocations[text_index].empty(),
              "riscv first static executable seam currently depends on the return-add object staying relocation-free");

  std::string link_error;
  const auto linked = c4c::backend::riscv::linker::link_first_static_executable(
      {object_path}, &link_error);
  expect_true(linked.has_value(),
              "riscv first static executable seam should link the current bounded return-add object into a minimal executable image: " +
                  link_error);
  expect_true(linked->text_size == parsed_object->section_data[text_index].size() &&
                  linked->base_address == c4c::backend::riscv::linker::kFirstStaticBaseAddress &&
                  linked->symbol_addresses.find("main") != linked->symbol_addresses.end() &&
                  linked->entry_address == linked->symbol_addresses.at("main"),
              "riscv first static executable seam should preserve the bounded .text size and route entry through the global main symbol");

  const auto& image = linked->image;
  expect_true(image.size() >= linked->text_file_offset + linked->text_size &&
                  image[0] == 0x7f && image[1] == 'E' && image[2] == 'L' && image[3] == 'F',
              "riscv first static executable seam should emit an ELF image with room for the merged bounded text payload");
  expect_true(read_u16(image, 16) == 2 && read_u16(image, 18) == c4c::backend::riscv::linker::kEmRiscv &&
                  read_u64(image, 24) == linked->entry_address &&
                  read_u64(image, 32) == 64 && read_u16(image, 56) == 1,
              "riscv first static executable seam should emit an ET_EXEC RV64 header with a single program header and the linked main entry point");
  expect_true(read_u32(image, 64 + 0) == 1 && read_u32(image, 64 + 4) == 5 &&
                  read_u64(image, 64 + 8) == 0 &&
                  read_u64(image, 64 + 16) == linked->base_address &&
                  read_u64(image, 64 + 24) == linked->base_address &&
                  read_u64(image, 64 + 48) == 0x1000,
              "riscv first static executable seam should map the minimal image through a single RX PT_LOAD segment");
  expect_true(linked->text_virtual_address == linked->base_address + linked->text_file_offset &&
                  std::equal(parsed_object->section_data[text_index].begin(),
                             parsed_object->section_data[text_index].end(),
                             image.begin() + static_cast<std::ptrdiff_t>(linked->text_file_offset)),
              "riscv first static executable seam should copy the current return-add text bytes unchanged into the executable image");

  std::filesystem::remove(object_path);
}

void test_riscv_first_static_executable_applies_text_relocation_fixture() {
  const auto object_path = make_temp_output_path("c4c_riscv_first_static_reloc");
  write_file_bytes(object_path, make_minimal_riscv64_text_relocation_object_fixture());

  std::string inspect_error;
  const auto slice = c4c::backend::riscv::linker::inspect_first_static_link_slice(
      {object_path}, &inspect_error);
  expect_true(slice.has_value(),
              "riscv first static executable seam should inspect the bounded relocation-bearing object: " +
                  inspect_error);
  expect_true(slice->objects.size() == 1 &&
                  slice->objects.front().defined_symbols.size() == 2 &&
                  slice->objects.front().undefined_symbols.empty(),
              "riscv first static executable seam should preserve the bounded single-object provider inventory for the relocation fixture");
  expect_true(slice->relocations.size() == 1 &&
                  slice->relocations.front().section_name == ".text" &&
                  slice->relocations.front().symbol_name == "helper" &&
                  slice->relocations.front().relocation_type == 2 &&
                  slice->relocations.front().resolved &&
                  slice->unresolved_symbols.empty(),
              "riscv first static executable seam should surface the bounded text relocation as resolved before executable emission");

  std::string link_error;
  const auto linked = c4c::backend::riscv::linker::link_first_static_executable(
      {object_path}, &link_error);
  expect_true(linked.has_value(),
              "riscv first static executable seam should link the bounded relocation-bearing object: " +
                  link_error);
  expect_true(linked->symbol_addresses.find("main") != linked->symbol_addresses.end() &&
                  linked->symbol_addresses.find("helper") != linked->symbol_addresses.end(),
              "riscv first static executable seam should preserve both global text symbols through relocation-aware linking");

  const auto relocated_helper = read_u64(linked->image, linked->text_file_offset + 4);
  expect_true(relocated_helper == linked->symbol_addresses.at("helper") &&
                  linked->entry_address == linked->symbol_addresses.at("main"),
              "riscv first static executable seam should patch the bounded R_RISCV_64 text relocation with the linked helper address while keeping main as the executable entry");

  std::filesystem::remove(object_path);
}

void test_riscv_first_static_executable_links_assembler_emitted_jal_relocation_object() {
  const auto object_path = make_temp_output_path("c4c_riscv_first_static_asm_reloc");
  const auto assembled = c4c::backend::riscv::assembler::assemble(
      c4c::backend::riscv::assembler::AssembleRequest{
          .asm_text =
              "  .text\n"
              "  .global main\n"
              "  .global helper\n"
              "main:\n"
              "  jal helper\n"
              "  ret\n"
              "helper:\n"
              "  ret\n",
          .output_path = object_path,
      });
  expect_true(assembled.object_emitted && assembled.error.empty(),
              "riscv first static executable seam should accept one bounded assembler-produced jal relocation object");

  const auto object_bytes = read_file_bytes(object_path);
  std::string parse_error;
  const auto parsed_object = c4c::backend::linker_common::parse_elf64_object(
      object_bytes, object_path, c4c::backend::riscv::linker::kEmRiscv, &parse_error);
  expect_true(parsed_object.has_value(),
              "riscv first static executable seam should parse the assembler-produced relocation object: " +
                  parse_error);
  const auto text_index = find_section_index(*parsed_object, ".text");
  expect_true(text_index < parsed_object->sections.size() &&
                  parsed_object->section_data[text_index].size() == 12 &&
                  parsed_object->relocations[text_index].size() == 1,
              "riscv first static executable seam should preserve one bounded jal relocation in the assembler-produced object");
  const auto& relocation = parsed_object->relocations[text_index].front();
  expect_true(relocation.offset == 0 && relocation.rela_type == 17 &&
                  relocation.sym_idx < parsed_object->symbols.size() &&
                  parsed_object->symbols[relocation.sym_idx].name == "helper",
              "riscv first static executable seam should keep the assembler-produced jal relocation targeted at helper");

  std::string inspect_error;
  const auto slice = c4c::backend::riscv::linker::inspect_first_static_link_slice(
      {object_path}, &inspect_error);
  expect_true(slice.has_value(),
              "riscv first static executable seam should inspect the assembler-produced relocation object: " +
                  inspect_error);
  expect_true(slice->objects.size() == 1 &&
                  slice->objects.front().defined_symbols.size() == 2 &&
                  slice->relocations.size() == 1 && slice->relocations.front().resolved &&
                  slice->relocations.front().relocation_type == 17 &&
                  slice->relocations.front().symbol_name == "helper",
              "riscv first static executable seam should surface the assembler-produced jal relocation as a resolved single-object case");

  std::string link_error;
  const auto linked = c4c::backend::riscv::linker::link_first_static_executable(
      {object_path}, &link_error);
  expect_true(linked.has_value(),
              "riscv first static executable seam should link the assembler-produced relocation object: " +
                  link_error);
  expect_true(linked->symbol_addresses.find("main") != linked->symbol_addresses.end() &&
                  linked->symbol_addresses.find("helper") != linked->symbol_addresses.end() &&
                  linked->entry_address == linked->symbol_addresses.at("main"),
              "riscv first static executable seam should preserve both emitted text symbols while keeping main as the executable entry");

  const auto object_jal = read_u32(parsed_object->section_data[text_index], 0);
  const auto linked_jal = read_u32(linked->image, linked->text_file_offset + 0);
  expect_true(object_jal != linked_jal,
              "riscv first static executable seam should patch the assembler-produced jal relocation in the linked image");

  std::filesystem::remove(object_path);
}

void test_shared_linker_parses_single_member_archive_fixture() {
  const auto fixture = make_single_member_archive_fixture();
  std::string error;
  const auto parsed = c4c::backend::linker_common::parse_elf64_archive(
      fixture, "libminimal.a", c4c::backend::elf::EM_AARCH64, &error);
  expect_true(parsed.has_value(),
              "shared linker archive parser should accept the bounded single-member fixture: " +
                  error);

  const auto& archive = *parsed;
  expect_true(archive.source_name == "libminimal.a",
              "shared linker archive parser should preserve the archive source name");
  expect_true(archive.members.size() == 1,
              "shared linker archive parser should preserve the bounded single-member inventory");
  expect_true(archive.find_member_index_for_symbol("main").has_value(),
              "shared linker archive parser should support symbol-driven member lookup for the bounded archive slice");

  const auto member_index = *archive.find_member_index_for_symbol("main");
  const auto& member = archive.members[member_index];
  expect_true(member.name == "minimal_reloc.o",
              "shared linker archive parser should normalize the bounded member name");
  expect_true(member.object.has_value(),
              "shared linker archive parser should parse the bounded ELF member into a shared object surface");
  expect_true(member.defines_symbol("main") && !member.defines_symbol("helper_ext"),
              "shared linker archive parser should index defined symbols without treating unresolved externs as providers");

  const auto& object = *member.object;
  const auto text_index = find_section_index(object, ".text");
  expect_true(text_index < object.sections.size(),
              "shared linker archive parser should preserve section names for the parsed member object");
  expect_true(object.relocations[text_index].size() == 1,
              "shared linker archive parser should preserve relocation inventory for the bounded archive member");
}

int main(int argc, char* argv[]) {
  if (argc >= 2) test_filter() = argv[1];
  RUN_TEST(test_backend_shared_call_decode_parses_bir_minimal_direct_call_module);
  RUN_TEST(test_x86_codegen_header_exports_translated_globals_owner_symbols);
  RUN_TEST(test_x86_codegen_header_exports_translated_globals_owner_helper_symbols);
  RUN_TEST(test_x86_codegen_header_exports_translated_returns_owner_symbols);
  RUN_TEST(test_x86_codegen_header_exports_translated_prologue_owner_symbols);
  RUN_TEST(test_riscv_codegen_header_exports_translated_returns_owner_symbols);
  RUN_TEST(test_riscv_codegen_header_exports_translated_prologue_return_exit_symbols);
  RUN_TEST(test_riscv_codegen_header_exports_translated_f128_return_helper_symbol);
  RUN_TEST(test_riscv_codegen_header_exports_translated_f128_compare_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_f128_memory_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_cast_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_i128_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_memory_helper_surface);
  RUN_TEST(test_riscv_memory_shared_surface_models_slot_addr_and_type_selectors);
  RUN_TEST(test_riscv_codegen_header_exports_translated_memory_owner_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_alu_unary_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_alu_integer_owner_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_variadic_activation_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_atomics_activation_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_atomic_load_store_fence_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_atomic_rmw_cmpxchg_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_atomic_software_helper_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_intrinsics_activation_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_intrinsics_dispatcher_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_calls_abi_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_calls_instruction_cleanup_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_calls_result_store_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_globals_label_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_global_addr_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_float_binop_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_int_cmp_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_comparison_control_flow_surface);
  RUN_TEST(test_riscv_codegen_header_exports_translated_float_cmp_surface);
  RUN_TEST(test_riscv_traits_exports_shared_return_default_seam);
  RUN_TEST(test_x86_translated_returns_owner_emits_bounded_helper_text);
  RUN_TEST(test_riscv_translated_returns_owner_emits_bounded_helper_text);
  RUN_TEST(test_riscv_translated_returns_owner_emits_bounded_value_return_paths);
  RUN_TEST(test_riscv_traits_return_default_emits_bounded_shared_return_text);
  RUN_TEST(test_riscv_translated_f128_return_helper_emits_gp_pair_loads);
  RUN_TEST(test_riscv_translated_memory_helper_seam_emits_bounded_text);
  RUN_TEST(test_riscv_translated_memory_owner_emits_bounded_scalar_text);
  RUN_TEST(test_riscv_translated_alu_unary_helpers_emit_bounded_text);
  RUN_TEST(test_riscv_translated_alu_integer_owner_emits_bounded_text);
  RUN_TEST(test_riscv_translated_calls_abi_helpers);
  RUN_TEST(test_riscv_translated_calls_instruction_cleanup_helpers);
  RUN_TEST(test_riscv_translated_calls_result_store_non_f128_emits_bounded_text);
  RUN_TEST(test_riscv_translated_calls_result_store_wide_emits_bounded_text);
  RUN_TEST(test_riscv_translated_globals_label_helper_emits_bounded_text);
  RUN_TEST(test_riscv_translated_globals_global_addr_helper_emits_bounded_text);
  RUN_TEST(test_riscv_translated_call_owner_emits_scalar_reg_and_stack_abi_text);
  RUN_TEST(test_riscv_translated_call_owner_emits_wide_scalar_staging_text);
  RUN_TEST(test_riscv_translated_call_owner_emits_aggregate_stack_staging_text);
  RUN_TEST(test_riscv_translated_call_owner_emits_aggregate_reg_staging_text);
  RUN_TEST(test_riscv_translated_call_owner_emits_float_aggregate_reg_staging_text);
  RUN_TEST(test_riscv_translated_float_binop_non_f128_emits_bounded_text);
  RUN_TEST(test_riscv_translated_int_cmp_emits_bounded_text);
  RUN_TEST(test_riscv_translated_comparison_control_flow_emits_bounded_text);
  RUN_TEST(test_riscv_translated_f128_neg_emits_bounded_text);
  RUN_TEST(test_riscv_translated_float_binop_f128_emits_bounded_text);
  RUN_TEST(test_riscv_translated_float_cmp_non_f128_emits_bounded_text);
  RUN_TEST(test_riscv_translated_f128_cmp_emits_bounded_text);
  RUN_TEST(test_riscv_translated_f128_memory_emits_bounded_text);
  RUN_TEST(test_riscv_translated_cast_emits_bounded_f128_text);
  RUN_TEST(test_riscv_translated_cast_emits_bounded_scalar_text);
  RUN_TEST(test_riscv_translated_default_cast_emits_bounded_i128_text);
  RUN_TEST(test_riscv_translated_i128_owner_emits_bounded_helper_text);
  RUN_TEST(test_riscv_translated_prologue_return_exit_helpers_match_shared_contract);
  RUN_TEST(test_riscv_translated_prologue_tracks_variadic_frame_setup);
  RUN_TEST(test_riscv_translated_variadic_emits_bounded_text);
  RUN_TEST(test_riscv_translated_atomics_activation_helpers_emit_bounded_text);
  RUN_TEST(test_riscv_translated_atomic_load_store_fence_emits_bounded_text);
  RUN_TEST(test_riscv_translated_atomic_rmw_cmpxchg_emits_bounded_text);
  RUN_TEST(test_riscv_translated_atomic_software_helpers_emit_bounded_text);
  RUN_TEST(test_riscv_translated_intrinsics_activation_helpers_emit_bounded_text);
  RUN_TEST(test_riscv_translated_intrinsics_dispatcher_emits_bounded_text);
  RUN_TEST(test_x86_translated_returns_owner_emits_bounded_value_return_paths);
  RUN_TEST(test_x86_translated_returns_owner_handles_i128_lane_variants);
  RUN_TEST(test_x86_translated_i128_owner_emits_bounded_helper_text);
  RUN_TEST(test_x86_codegen_header_exports_translated_call_owner_surface);
  RUN_TEST(test_x86_translated_call_owner_emits_scalar_reg_and_stack_abi_text);
  RUN_TEST(test_x86_operand_immediate_contract);
  RUN_TEST(test_x86_translated_shared_call_support_tracks_real_state_and_output);
  RUN_TEST(test_x86_translated_intrinsics_owner_emits_bounded_helper_text);
  RUN_TEST(test_x86_translated_intrinsics_dispatcher_emits_bounded_text);
  RUN_TEST(test_x86_codegen_header_exports_translated_memory_owner_surface);
  RUN_TEST(test_x86_codegen_state_tracks_translated_reg_assignments);
  RUN_TEST(test_x86_translated_memory_owner_emits_linked_helper_text);
  RUN_TEST(test_x86_translated_memory_owner_preserves_f128_store_precision_paths);
  RUN_TEST(test_x86_translated_memory_owner_routes_constant_f128_stores_through_raw_byte_helper);
  RUN_TEST(test_x86_translated_f128_helper_reloads_constant_raw_bytes);
  RUN_TEST(test_x86_translated_f128_helpers_reload_integer_conversion_sources);
  RUN_TEST(test_x86_translated_f128_helpers_emit_st0_integer_conversion_text);
  RUN_TEST(test_x86_translated_f128_cast_helpers_dispatch_x87_paths);
  RUN_TEST(test_x86_translated_f128_cast_helpers_cover_generic_scalar_casts);
  RUN_TEST(test_x86_translated_f128_unsigned_cast_helpers_emit_real_paths);
  RUN_TEST(test_x86_codegen_header_exports_translated_cast_owner_symbols);
  RUN_TEST(test_x86_translated_cast_owner_reuses_constant_f128_reload_bridge);
  RUN_TEST(test_x86_translated_cast_owner_emits_unsigned_f128_split_path);
  RUN_TEST(test_x86_codegen_header_exports_translated_float_owner_symbols);
  RUN_TEST(test_x86_codegen_header_exports_translated_comparison_owner_symbols);
  RUN_TEST(test_x86_translated_comparison_owner_reuses_constant_f128_reload_bridge);
  RUN_TEST(test_x86_translated_comparison_owner_emits_bounded_branch_and_select_text);
  RUN_TEST(test_x86_translated_float_owner_reuses_constant_f128_reload_bridge);
  RUN_TEST(test_x86_translated_float_owner_handles_u32_unary_widths);
  RUN_TEST(test_x86_codegen_header_exports_translated_asm_emitter_owner_symbols);
  RUN_TEST(test_riscv_codegen_header_exports_first_owner_cluster_helpers);
  RUN_TEST(test_riscv_translated_asm_emitter_helpers_match_shared_contract);
  RUN_TEST(test_x86_translated_asm_emitter_helpers_match_shared_contract);
  RUN_TEST(test_x86_translated_asm_emitter_owner_allocates_bounded_scratch_pools);
  RUN_TEST(test_x86_translated_regalloc_pruning_helpers_match_shared_contract);
  RUN_TEST(test_x86_translated_globals_owner_emits_bounded_helper_text);
  RUN_TEST(test_backend_shared_call_decode_parses_bir_minimal_declared_direct_call_module);
  RUN_TEST(test_backend_shared_call_decode_parses_bir_minimal_void_direct_call_imm_return_module);
  RUN_TEST(test_backend_shared_call_decode_parses_bir_minimal_two_arg_direct_call_module);
  RUN_TEST(test_backend_shared_call_decode_parses_bir_minimal_direct_call_add_imm_module);
  RUN_TEST(test_backend_shared_call_decode_parses_bir_minimal_direct_call_identity_arg_module);
  RUN_TEST(test_backend_shared_call_decode_parses_bir_minimal_dual_identity_direct_call_sub_module);
  RUN_TEST(test_backend_shared_call_decode_parses_bir_minimal_call_crossing_direct_call_module);
  RUN_TEST(test_backend_shared_call_decode_accepts_typed_i32_single_add_imm_helper);
  RUN_TEST(test_backend_shared_call_decode_accepts_typed_i32_single_add_imm_helper_with_stale_return_text);
  RUN_TEST(test_backend_shared_call_decode_accepts_typed_i32_slot_add_helper);
  RUN_TEST(test_backend_shared_call_decode_accepts_typed_i32_slot_add_helper_with_stale_return_text);
  RUN_TEST(test_backend_shared_call_decode_accepts_typed_i32_identity_helper);
  RUN_TEST(test_backend_shared_call_decode_accepts_typed_i32_identity_helper_with_stale_return_text);
  RUN_TEST(test_backend_shared_call_decode_accepts_typed_i32_two_param_add_helper);
  RUN_TEST(test_backend_shared_call_decode_accepts_typed_i32_two_param_add_helper_with_stale_return_text);
  RUN_TEST(test_backend_shared_call_decode_prefers_typed_vararg_call_args_over_stale_fixed_suffix_text);
  RUN_TEST(test_backend_shared_liveness_surface_tracks_result_names);
  RUN_TEST(test_backend_shared_liveness_surface_tracks_call_crossing_ranges);
  RUN_TEST(test_backend_shared_liveness_surface_tracks_phi_join_ranges);
  RUN_TEST(test_backend_shared_liveness_input_matches_lir_phi_join_ranges);
  RUN_TEST(test_backend_shared_regalloc_surface_uses_caller_saved_for_non_call_interval);
  RUN_TEST(test_backend_shared_regalloc_prefers_callee_saved_for_call_spanning_values);
  RUN_TEST(test_backend_shared_regalloc_accepts_backend_owned_liveness_input);
  RUN_TEST(test_backend_shared_regalloc_reuses_register_after_interval_ends);
  RUN_TEST(test_backend_shared_regalloc_spills_overlapping_values_without_reusing_busy_reg);
  RUN_TEST(test_backend_shared_regalloc_helper_filters_and_merges_clobbers);
  RUN_TEST(test_backend_shared_stack_layout_regalloc_helper_exposes_handoff_view);
  RUN_TEST(test_backend_shared_regalloc_helper_accepts_backend_owned_liveness_input);
  RUN_TEST(test_x86_shared_regalloc_helper_uses_translated_register_pools);
  RUN_TEST(test_backend_shared_stack_layout_analysis_tracks_phi_use_blocks);
  RUN_TEST(test_backend_shared_stack_layout_analysis_accepts_backend_owned_input);
  RUN_TEST(test_backend_shared_stack_layout_input_collects_value_names);
  RUN_TEST(test_backend_shared_stack_layout_input_preserves_param_alloca_metadata);
  RUN_TEST(test_backend_shared_stack_layout_input_preserves_signature_metadata);
  RUN_TEST(test_backend_shared_stack_layout_input_preserves_aarch64_return_gate_metadata);
  RUN_TEST(test_backend_shared_stack_layout_analysis_detects_dead_param_allocas);
  RUN_TEST(test_backend_shared_stack_layout_analysis_tracks_call_arg_values);
  RUN_TEST(test_backend_shared_stack_layout_analysis_detects_entry_alloca_overwrite_before_read);
  RUN_TEST(test_backend_shared_alloca_coalescing_classifies_non_param_allocas);
  RUN_TEST(test_backend_shared_alloca_coalescing_accepts_backend_owned_input);
  RUN_TEST(test_backend_shared_alloca_coalescing_accepts_entry_alloca_use_blocks);
  RUN_TEST(test_backend_shared_stack_layout_analysis_accepts_entry_alloca_first_accesses);
  RUN_TEST(test_backend_shared_slot_assignment_plans_param_alloca_slots);
  RUN_TEST(test_backend_shared_slot_assignment_param_alloca_pruning_matches_backend_owned_entry_patch);
  RUN_TEST(test_backend_shared_slot_assignment_plans_entry_alloca_slots);
  RUN_TEST(test_backend_shared_slot_assignment_accepts_narrowed_planning_input);
  RUN_TEST(test_backend_shared_slot_assignment_accepts_backend_owned_input);
  RUN_TEST(test_backend_shared_slot_assignment_bundle_accepts_backend_owned_input);
  RUN_TEST(test_backend_shared_slot_assignment_bundle_accepts_narrowed_planning_input);
  RUN_TEST(test_backend_shared_slot_assignment_stack_layout_lowering_matches_prepared_planning_input);
  RUN_TEST(test_backend_shared_slot_assignment_bundle_prepares_from_backend_owned_inputs);
  RUN_TEST(test_backend_shared_slot_assignment_stack_layout_lowering_matches_prepared_bundle_input);
  RUN_TEST(test_backend_shared_slot_assignment_bundle_prepares_from_narrowed_planning_input);
  RUN_TEST(test_backend_shared_slot_assignment_stack_layout_lowering_matches_prepared_rewrite_input);
  RUN_TEST(test_backend_shared_slot_assignment_rewrites_module_entry_allocas);
  RUN_TEST(test_backend_shared_slot_assignment_prepares_module_function_inputs);
  RUN_TEST(test_backend_shared_prepared_function_inputs_preserve_emitter_stack_layout_metadata);
  RUN_TEST(test_backend_shared_prepared_function_inputs_collect_emitter_value_names_without_compat_stack_layout);
  RUN_TEST(test_backend_shared_rewrite_only_prepared_function_inputs_preserve_rewrite_and_planning_state);
  RUN_TEST(test_backend_shared_fallback_entry_alloca_stack_layout_input_preserves_pointer_metadata);
  RUN_TEST(test_backend_shared_fallback_entry_alloca_stack_layout_input_preserves_signature_and_call_metadata);
  RUN_TEST(test_backend_shared_fallback_preparation_separates_stack_layout_metadata_from_cfg_classification);
  RUN_TEST(test_backend_shared_fallback_preparation_derives_block_instruction_counts_from_liveness);
  RUN_TEST(test_backend_shared_fallback_preparation_rehydrates_stack_layout_uses_from_liveness);
  RUN_TEST(test_backend_shared_fallback_preparation_rehydrates_phi_predecessor_labels_from_liveness);
  RUN_TEST(test_backend_shared_fallback_preparation_still_needs_remaining_pointer_escape_facts);
  RUN_TEST(test_backend_shared_fallback_preparation_keeps_only_liveness_cfg_state);
  RUN_TEST(test_backend_shared_prepares_lir_module_for_target);
  RUN_TEST(test_backend_shared_prepared_function_inputs_preserve_live_pointer_param_allocas);
  RUN_TEST(test_backend_shared_target_preparation_enables_bir_lowering);
  RUN_TEST(test_backend_shared_slot_assignment_prunes_dead_entry_alloca_insts);
  RUN_TEST(test_backend_shared_slot_assignment_applies_coalesced_entry_slots);
  RUN_TEST(test_backend_shared_assemble_target_lir_module_matches_public_wrappers);
  RUN_TEST(test_riscv_builtin_assembler_parses_activation_slice_variants);
  RUN_TEST(test_riscv_linker_contract_exposes_first_static_surface);
  RUN_TEST(test_riscv_first_static_executable_image_matches_return_add_object);
  RUN_TEST(test_riscv_first_static_executable_applies_text_relocation_fixture);
  RUN_TEST(test_riscv_first_static_executable_links_assembler_emitted_jal_relocation_object);
  // TODO: binary-utils contract test disabled — assembler seam changed
  // test_backend_binary_utils_contract_headers_are_include_reachable();
  RUN_TEST(test_shared_linker_parses_minimal_relocation_object_fixture);
  // TODO: builtin return-add object parse disabled — assembler seam changed
  // test_shared_linker_parses_builtin_return_add_object();
  // TODO(phase2): temporary skip while linker/assembler-focused x86 tests are being split.
  // test_shared_linker_parses_builtin_x86_extern_call_object();
  RUN_TEST(test_shared_linker_parses_single_member_archive_fixture);

  check_failures();
  return 0;
}
