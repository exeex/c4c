

#include "backend.hpp"
#include "generation.hpp"
#include "ir_printer.hpp"
#include "ir_validate.hpp"
#include "liveness.hpp"
#include "lir_adapter.hpp"
#include "regalloc.hpp"
#include "stack_layout/analysis.hpp"
#include "stack_layout/alloca_coalescing.hpp"
#include "stack_layout/regalloc_helpers.hpp"
#include "stack_layout/slot_assignment.hpp"
#include "target.hpp"
#include "../../src/codegen/lir/call_args.hpp"
#include "../../src/codegen/lir/lir_printer.hpp"
#include "../../src/codegen/lir/verify.hpp"
#include "../../src/backend/elf/mod.hpp"
#include "../../src/backend/linker_common/mod.hpp"
#include "../../src/backend/aarch64/assembler/mod.hpp"
#include "../../src/backend/aarch64/codegen/emit.hpp"
#include "../../src/backend/aarch64/linker/mod.hpp"
#include "../../src/backend/aarch64/assembler/parser.hpp"
#include "../../src/backend/x86/assembler/mod.hpp"
#include "../../src/backend/x86/assembler/encoder/mod.hpp"
#include "../../src/backend/x86/assembler/parser.hpp"
#include "../../src/backend/x86/codegen/emit.hpp"
#include "../../src/backend/x86/linker/mod.hpp"
#include "backend_test_helper.hpp"

void clear_backend_memory_type_compatibility_shims(c4c::backend::BackendModule& module) {
  for (auto& function : module.functions) {
    for (auto& block : function.blocks) {
      if (c4c::backend::backend_return_type_kind(block.terminator) !=
          c4c::backend::BackendValueTypeKind::Unknown) {
        block.terminator.type_str.clear();
      }
      for (auto& inst : block.insts) {
        if (auto* phi = std::get_if<c4c::backend::BackendPhiInst>(&inst)) {
          if (phi->value_type != c4c::backend::BackendScalarType::Unknown) {
            phi->type_str.clear();
          }
          continue;
        }
        if (auto* bin = std::get_if<c4c::backend::BackendBinaryInst>(&inst)) {
          if (bin->value_type != c4c::backend::BackendScalarType::Unknown) {
            bin->type_str.clear();
          }
          continue;
        }
        if (auto* cmp = std::get_if<c4c::backend::BackendCompareInst>(&inst)) {
          if (cmp->operand_type != c4c::backend::BackendScalarType::Unknown) {
            cmp->type_str.clear();
          }
          continue;
        }
        if (auto* load = std::get_if<c4c::backend::BackendLoadInst>(&inst)) {
          if (load->value_type != c4c::backend::BackendScalarType::Unknown) {
            load->type_str.clear();
          }
          if (load->memory_value_type != c4c::backend::BackendScalarType::Unknown) {
            load->memory_type.clear();
          }
          continue;
        }
        if (auto* store = std::get_if<c4c::backend::BackendStoreInst>(&inst)) {
          if (store->value_type != c4c::backend::BackendScalarType::Unknown) {
            store->type_str.clear();
          }
          continue;
        }
        if (auto* ptrdiff = std::get_if<c4c::backend::BackendPtrDiffEqInst>(&inst)) {
          if (ptrdiff->result_type != c4c::backend::BackendScalarType::Unknown) {
            ptrdiff->type_str.clear();
          }
        }
      }
    }
  }
}

void clear_backend_signature_and_call_type_compatibility_shims(
    c4c::backend::BackendModule& module) {
  for (auto& function : module.functions) {
    if (function.signature.return_type_kind != c4c::backend::BackendValueTypeKind::Unknown) {
      function.signature.return_type.clear();
    }
    for (auto& param : function.signature.params) {
      if (param.type_kind != c4c::backend::BackendValueTypeKind::Unknown) {
        param.type_str.clear();
      }
    }
    for (auto& block : function.blocks) {
      for (auto& inst : block.insts) {
        auto* call = std::get_if<c4c::backend::BackendCallInst>(&inst);
        if (call == nullptr) {
          continue;
        }
        if (call->return_type_kind != c4c::backend::BackendValueTypeKind::Unknown) {
          call->return_type.clear();
        }
        for (std::size_t index = 0; index < call->param_type_kinds.size() &&
                                    index < call->param_types.size();
             ++index) {
          if (call->param_type_kinds[index] != c4c::backend::BackendValueTypeKind::Unknown) {
            call->param_types[index].clear();
          }
        }
        for (std::size_t index = 0; index < call->param_type_kinds.size() &&
                                    index < call->args.size();
             ++index) {
          if (call->param_type_kinds[index] != c4c::backend::BackendValueTypeKind::Unknown) {
            call->args[index].type.clear();
          }
        }
      }
    }
  }
}

void clear_backend_global_linkage_compatibility_shims(c4c::backend::BackendModule& module) {
  for (auto& global : module.globals) {
    if (global.linkage_kind != c4c::backend::BackendGlobalLinkage::Default) {
      global.linkage.clear();
    }
  }
}

void clear_backend_global_type_compatibility_shims(c4c::backend::BackendModule& module) {
  for (auto& global : module.globals) {
    if (global.array_type.has_value() ||
        global.type_kind != c4c::backend::BackendValueTypeKind::Unknown) {
      global.llvm_type.clear();
    }
  }
}

c4c::backend::BackendModule make_structured_local_slot_ptrdiff_module() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_local_array_gep_module());
  auto& block = lowered.functions.front().blocks.front();
  block.insts.resize(2);
  block.insts.push_back(c4c::backend::BackendPtrDiffEqInst{
      "%t.ptrdiff",
      "i32",
      c4c::backend::BackendAddress{"%lv.arr", 4},
      c4c::backend::BackendAddress{"%lv.arr", 0},
      4,
      1,
      c4c::backend::BackendScalarType::I32,
      c4c::backend::BackendScalarType::I32,
  });
  block.terminator = c4c::backend::make_backend_return("%t.ptrdiff", "i32");
  return lowered;
}

c4c::backend::BackendModule make_structured_cross_local_slot_ptrdiff_module() {
  auto lowered = make_structured_local_slot_ptrdiff_module();
  lowered.functions.front().local_slots.push_back(
      c4c::backend::BackendLocalSlot{"%lv.other", 8, c4c::backend::BackendScalarType::I32, 4});
  auto& ptrdiff = std::get<c4c::backend::BackendPtrDiffEqInst>(
      lowered.functions.front().blocks.front().insts.back());
  ptrdiff.rhs_address.base_symbol = "%lv.other";
  return lowered;
}

void test_backend_ir_printer_renders_lowered_conditional_return_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_conditional_return_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "%t0 = icmp slt i32 2, 3",
                  "backend IR printer should render the lowered compare for the conditional-return slice");
  expect_contains(rendered, "br i1 %t0, label %then, label %else",
                  "backend IR printer should render the lowered conditional branch");
  expect_contains(rendered, "then:\n  ret i32 0",
                  "backend IR printer should preserve the lowered then return block");
  expect_contains(rendered, "else:\n  ret i32 1",
                  "backend IR printer should preserve the lowered else return block");
}

void test_backend_ir_validator_accepts_lowered_conditional_return_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_conditional_return_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept lowered conditional-return control-flow slices");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid lowered conditional-return slices");
}

void test_backend_ir_printer_renders_structured_conditional_return_slice_without_type_text() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_conditional_return_module());
  clear_backend_memory_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "%t0 = icmp slt i32 2, 3",
                  "backend IR printer should render lowered conditional compares from structured compare metadata without legacy type text");
  expect_contains(rendered, "br i1 %t0, label %then, label %else",
                  "backend IR printer should preserve structured lowered conditional branches without compare type shims");
}

void test_backend_ir_validator_accepts_structured_conditional_return_slice_without_type_text() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_conditional_return_module());
  clear_backend_memory_type_compatibility_shims(lowered);
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept lowered conditional-return slices when compare type metadata is structured and legacy text is cleared");
  expect_true(error.empty(),
              "backend IR validator should not report an error for structured conditional-return slices without compare type text");
}

void test_backend_ir_printer_renders_lowered_conditional_phi_join_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_conditional_phi_join_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "%t0 = icmp slt i32 2, 3",
                  "backend IR printer should render the lowered compare for the conditional-join slice");
  expect_contains(rendered, "br i1 %t0, label %then, label %else",
                  "backend IR printer should render the lowered branch into the join predecessors");
  expect_contains(rendered, "then:\n  br label %join",
                  "backend IR printer should preserve the explicit then-to-join edge");
  expect_contains(rendered, "else:\n  br label %join",
                  "backend IR printer should preserve the explicit else-to-join edge");
  expect_contains(rendered, "join:\n  %t.join = phi i32 [ 7, %then ], [ 9, %else ]",
                  "backend IR printer should render the lowered join phi");
  expect_contains(rendered, "ret i32 %t.join",
                  "backend IR printer should preserve the lowered join return");
}

void test_backend_ir_validator_accepts_lowered_conditional_phi_join_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_conditional_phi_join_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept lowered conditional-join slices");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid lowered conditional joins");
}

void test_backend_ir_printer_renders_lowered_conditional_phi_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_conditional_phi_join_add_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "%t0 = icmp slt i32 2, 3",
                  "backend IR printer should render the lowered compare for the computed conditional-join slice");
  expect_contains(rendered, "join:\n  %t.join = phi i32 [ 7, %then ], [ 9, %else ]\n  %t4 = add i32 %t.join, 5",
                  "backend IR printer should preserve the phi-fed add in the lowered join block");
  expect_contains(rendered, "ret i32 %t4",
                  "backend IR printer should preserve the computed conditional-join return");
}

void test_backend_ir_validator_accepts_lowered_conditional_phi_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_conditional_phi_join_add_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept lowered conditional joins whose phi feeds a computed scalar");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid computed conditional joins");
}

void test_backend_ir_printer_renders_structured_conditional_phi_join_add_slice_without_type_text() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_conditional_phi_join_add_module());
  clear_backend_memory_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "join:\n  %t.join = phi i32 [ 7, %then ], [ 9, %else ]\n  %t4 = add i32 %t.join, 5",
                  "backend IR printer should render structured phi and binary metadata without legacy type text");
  expect_contains(rendered, "ret i32 %t4",
                  "backend IR printer should preserve the computed conditional-join return after clearing phi and binary type shims");
}

void test_backend_ir_validator_accepts_structured_conditional_phi_join_add_slice_without_type_text() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_conditional_phi_join_add_module());
  clear_backend_memory_type_compatibility_shims(lowered);
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept lowered computed conditional joins when phi and binary type metadata is structured and legacy text is cleared");
  expect_true(error.empty(),
              "backend IR validator should not report an error for structured conditional-join slices without phi/binary type text");
}

void test_backend_ir_printer_renders_lowered_conditional_phi_join_predecessor_add_slice() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_conditional_phi_join_predecessor_add_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "then:\n  %t3 = add i32 7, 5\n  br label %join",
                  "backend IR printer should preserve predecessor-local computation before the join");
  expect_contains(rendered, "else:\n  %t4 = add i32 9, 4\n  br label %join",
                  "backend IR printer should preserve the alternate predecessor-local computation before the join");
  expect_contains(rendered, "join:\n  %t.join = phi i32 [ %t3, %then ], [ %t4, %else ]",
                  "backend IR printer should render the merge of predecessor-computed phi inputs");
  expect_contains(rendered, "ret i32 %t.join",
                  "backend IR printer should preserve the join return for predecessor-computed phi inputs");
}

void test_backend_ir_validator_accepts_lowered_conditional_phi_join_predecessor_add_slice() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_conditional_phi_join_predecessor_add_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept lowered conditional joins whose predecessors compute phi inputs");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid predecessor-computed conditional joins");
}

void test_backend_ir_printer_renders_lowered_conditional_phi_join_predecessor_sub_slice() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_conditional_phi_join_predecessor_sub_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "then:\n  %t3 = sub i32 12, 5\n  br label %join",
                  "backend IR printer should preserve predecessor-local sub computation before the join");
  expect_contains(rendered, "else:\n  %t4 = sub i32 15, 6\n  br label %join",
                  "backend IR printer should preserve the alternate predecessor-local sub computation before the join");
  expect_contains(rendered, "join:\n  %t.join = phi i32 [ %t3, %then ], [ %t4, %else ]",
                  "backend IR printer should render the merge of predecessor-computed sub inputs");
  expect_contains(rendered, "ret i32 %t.join",
                  "backend IR printer should preserve the join return for predecessor-computed sub inputs");
}

void test_backend_ir_validator_accepts_lowered_conditional_phi_join_predecessor_sub_slice() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_conditional_phi_join_predecessor_sub_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept lowered conditional joins whose predecessors compute sub-based phi inputs");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid predecessor-computed sub joins");
}

void test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_add_slice() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_conditional_phi_join_mixed_predecessor_add_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "then:\n  %t3 = add i32 7, 5\n  br label %join",
                  "backend IR printer should preserve the predecessor-local add on the computed join input");
  expect_contains(rendered, "else:\n  br label %join",
                  "backend IR printer should preserve the direct-immediate predecessor edge");
  expect_contains(rendered, "join:\n  %t.join = phi i32 [ %t3, %then ], [ 9, %else ]",
                  "backend IR printer should render the mixed predecessor/immediate phi merge");
  expect_contains(rendered, "ret i32 %t.join",
                  "backend IR printer should preserve the join return for mixed predecessor-computed phi inputs");
}

void test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_add_slice() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_conditional_phi_join_mixed_predecessor_add_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept lowered conditional joins with mixed predecessor-computed and immediate phi inputs");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid mixed predecessor/immediate joins");
}

void test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_add_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_add_post_join_add_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "then:\n  %t3 = add i32 7, 5\n  br label %join",
                  "backend IR printer should preserve the computed predecessor input before the asymmetric join");
  expect_contains(rendered,
                  "join:\n  %t.join = phi i32 [ %t3, %then ], [ 9, %else ]\n  %t5 = add i32 %t.join, 6",
                  "backend IR printer should preserve the join-local add fed by the mixed predecessor/immediate phi");
  expect_contains(rendered, "ret i32 %t5",
                  "backend IR printer should preserve the computed return for mixed predecessor/immediate post-phi joins");
}

void test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_add_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_add_post_join_add_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept mixed predecessor/immediate joins whose phi feeds a join-local add");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid mixed predecessor/immediate post-phi joins");
}

void test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_sub_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_sub_post_join_add_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "then:\n  %t3 = sub i32 12, 5\n  br label %join",
                  "backend IR printer should preserve the predecessor-local sub on the asymmetric join input");
  expect_contains(rendered,
                  "join:\n  %t.join = phi i32 [ %t3, %then ], [ 9, %else ]\n  %t5 = add i32 %t.join, 6",
                  "backend IR printer should preserve the join-local add fed by the mixed sub/immediate phi");
  expect_contains(rendered, "ret i32 %t5",
                  "backend IR printer should preserve the computed return for mixed predecessor-sub post-phi joins");
}

void test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_sub_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_sub_post_join_add_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept mixed predecessor-sub/immediate joins whose phi feeds a join-local add");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid mixed predecessor-sub/immediate post-phi joins");
}

void test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_add_sub_chain_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_add_sub_chain_post_join_add_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered,
                  "then:\n  %t3 = add i32 20, 5\n  %t4 = sub i32 %t3, 3\n  br label %join",
                  "backend IR printer should preserve the bounded predecessor-local add/sub chain on the asymmetric join input");
  expect_contains(rendered,
                  "join:\n  %t.join = phi i32 [ %t4, %then ], [ 9, %else ]\n  %t6 = add i32 %t.join, 6",
                  "backend IR printer should preserve the mixed chained predecessor input and join-local add");
  expect_contains(rendered, "ret i32 %t6",
                  "backend IR printer should preserve the computed return for mixed chained predecessor/immediate post-phi joins");
}

void test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_add_sub_chain_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_add_sub_chain_post_join_add_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept mixed predecessor/immediate joins whose computed edge uses a bounded add/sub chain before the phi");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid mixed chained predecessor/immediate post-phi joins");
}

void test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_chain_and_add_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_chain_and_add_post_join_add_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered,
                  "then:\n  %t3 = add i32 20, 5\n  %t4 = sub i32 %t3, 3\n  br label %join",
                  "backend IR printer should preserve the bounded predecessor-local add/sub chain on the primary asymmetric join input");
  expect_contains(rendered, "else:\n  %t5 = add i32 9, 4\n  br label %join",
                  "backend IR printer should preserve the widened alternate predecessor-local add on the asymmetric join input");
  expect_contains(rendered,
                  "join:\n  %t.join = phi i32 [ %t4, %then ], [ %t5, %else ]\n  %t7 = add i32 %t.join, 6",
                  "backend IR printer should preserve the asymmetric chain/add predecessor merge plus join-local add");
  expect_contains(rendered, "ret i32 %t7",
                  "backend IR printer should preserve the computed return for mixed chained predecessor/computed-edge post-phi joins");
}

void test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_chain_and_add_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_chain_and_add_post_join_add_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept mixed predecessor joins whose primary edge uses a bounded add/sub chain and alternate edge uses a computed add");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid mixed chained predecessor/computed-edge post-phi joins");
}

void test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_chain_and_chain_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_chain_and_chain_post_join_add_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered,
                  "then:\n  %t3 = add i32 20, 5\n  %t4 = sub i32 %t3, 3\n  br label %join",
                  "backend IR printer should preserve the bounded predecessor-local add/sub chain on the primary asymmetric join input");
  expect_contains(rendered,
                  "else:\n  %t5 = add i32 9, 4\n  %t6 = sub i32 %t5, 2\n  br label %join",
                  "backend IR printer should preserve the widened alternate predecessor-local add/sub chain on the asymmetric join input");
  expect_contains(rendered,
                  "join:\n  %t.join = phi i32 [ %t4, %then ], [ %t6, %else ]\n  %t8 = add i32 %t.join, 6",
                  "backend IR printer should preserve the asymmetric chain/chain predecessor merge plus join-local add");
  expect_contains(rendered, "ret i32 %t8",
                  "backend IR printer should preserve the computed return for mixed chained predecessor/two-edge post-phi joins");
}

void test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_chain_and_chain_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_chain_and_chain_post_join_add_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept mixed predecessor joins whose primary and alternate edges both use bounded add/sub chains");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid mixed chained predecessor/two-edge post-phi joins");
}

void test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_deeper_chain_and_chain_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_deeper_chain_and_chain_post_join_add_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered,
                  "then:\n  %t3 = add i32 20, 5\n  %t4 = sub i32 %t3, 3\n  %t5 = add i32 %t4, 8\n  br label %join",
                  "backend IR printer should preserve the widened three-op predecessor-local add/sub/add chain on the primary asymmetric join input");
  expect_contains(rendered,
                  "else:\n  %t6 = add i32 9, 4\n  %t7 = sub i32 %t6, 2\n  br label %join",
                  "backend IR printer should preserve the bounded alternate predecessor-local add/sub chain on the asymmetric join input");
  expect_contains(rendered,
                  "join:\n  %t.join = phi i32 [ %t5, %then ], [ %t7, %else ]\n  %t9 = add i32 %t.join, 6",
                  "backend IR printer should preserve the asymmetric deeper-chain/chain predecessor merge plus join-local add");
  expect_contains(rendered, "ret i32 %t9",
                  "backend IR printer should preserve the computed return for mixed deeper chained predecessor/two-edge post-phi joins");
}

void test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_deeper_chain_and_chain_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_deeper_chain_and_chain_post_join_add_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept mixed predecessor joins whose primary edge widens beyond the current two-op chain while the alternate edge remains a bounded add/sub chain");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid mixed deeper chained predecessor/two-edge post-phi joins");
}

void test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_deeper_chain_and_deeper_chain_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_deeper_chain_and_deeper_chain_post_join_add_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered,
                  "then:\n  %t3 = add i32 20, 5\n  %t4 = sub i32 %t3, 3\n  %t5 = add i32 %t4, 8\n  br label %join",
                  "backend IR printer should preserve the widened three-op predecessor-local chain on the primary asymmetric join input");
  expect_contains(rendered,
                  "else:\n  %t6 = add i32 9, 4\n  %t7 = sub i32 %t6, 2\n  %t8 = add i32 %t7, 11\n  br label %join",
                  "backend IR printer should preserve the widened three-op alternate predecessor-local chain on the asymmetric join input");
  expect_contains(rendered,
                  "join:\n  %t.join = phi i32 [ %t5, %then ], [ %t8, %else ]\n  %t10 = add i32 %t.join, 6",
                  "backend IR printer should preserve the asymmetric deeper-chain/deeper-chain predecessor merge plus join-local add");
  expect_contains(rendered, "ret i32 %t10",
                  "backend IR printer should preserve the computed return for mixed deeper chained predecessor/deeper-edge post-phi joins");
}

void test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_deeper_chain_and_deeper_chain_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_deeper_chain_and_deeper_chain_post_join_add_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept mixed predecessor joins whose primary and alternate edges both widen beyond the current two-op chain");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid mixed deeper chained predecessor/deeper-edge post-phi joins");
}

void test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_four_op_chain_and_deeper_chain_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_four_op_chain_and_deeper_chain_post_join_add_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered,
                  "then:\n  %t3 = add i32 20, 5\n  %t4 = sub i32 %t3, 3\n  %t5 = add i32 %t4, 8\n  %t6 = sub i32 %t5, 4\n  br label %join",
                  "backend IR printer should preserve the widened four-op predecessor-local chain on the primary asymmetric join input");
  expect_contains(rendered,
                  "else:\n  %t7 = add i32 9, 4\n  %t8 = sub i32 %t7, 2\n  %t9 = add i32 %t8, 11\n  br label %join",
                  "backend IR printer should preserve the existing deeper-chain alternate predecessor-local input");
  expect_contains(rendered,
                  "join:\n  %t.join = phi i32 [ %t6, %then ], [ %t9, %else ]\n  %t11 = add i32 %t.join, 6",
                  "backend IR printer should preserve the asymmetric four-op-chain/deeper-chain predecessor merge plus join-local add");
  expect_contains(rendered, "ret i32 %t11",
                  "backend IR printer should preserve the computed return for mixed four-op predecessor/deeper-edge post-phi joins");
}

void test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_four_op_chain_and_deeper_chain_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_four_op_chain_and_deeper_chain_post_join_add_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept mixed predecessor joins whose primary edge widens beyond the current three-op chain while the alternate edge remains a deeper three-op chain");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid mixed four-op predecessor/deeper-edge post-phi joins");
}

void test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_four_op_chain_and_four_op_chain_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_four_op_chain_and_four_op_chain_post_join_add_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered,
                  "then:\n  %t3 = add i32 20, 5\n  %t4 = sub i32 %t3, 3\n  %t5 = add i32 %t4, 8\n  %t6 = sub i32 %t5, 4\n  br label %join",
                  "backend IR printer should preserve the existing widened four-op predecessor-local chain on the primary asymmetric join input");
  expect_contains(rendered,
                  "else:\n  %t7 = add i32 9, 4\n  %t8 = sub i32 %t7, 2\n  %t9 = add i32 %t8, 11\n  %t10 = sub i32 %t9, 6\n  br label %join",
                  "backend IR printer should preserve the widened four-op alternate predecessor-local chain on the asymmetric join input");
  expect_contains(rendered,
                  "join:\n  %t.join = phi i32 [ %t6, %then ], [ %t10, %else ]\n  %t12 = add i32 %t.join, 6",
                  "backend IR printer should preserve the asymmetric four-op-chain/four-op-chain predecessor merge plus join-local add");
  expect_contains(rendered, "ret i32 %t12",
                  "backend IR printer should preserve the computed return for mixed four-op predecessor/four-op alternate post-phi joins");
}

void test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_four_op_chain_and_four_op_chain_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_four_op_chain_and_four_op_chain_post_join_add_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept mixed predecessor joins whose primary and alternate edges both widen to four-op add/sub chains");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid mixed four-op predecessor/four-op alternate post-phi joins");
}

void test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_five_op_chain_and_four_op_chain_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_five_op_chain_and_four_op_chain_post_join_add_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered,
                  "then:\n  %t3 = add i32 20, 5\n  %t4 = sub i32 %t3, 3\n  %t5 = add i32 %t4, 8\n  %t6 = sub i32 %t5, 4\n  %t7 = add i32 %t6, 10\n  br label %join",
                  "backend IR printer should preserve the widened five-op predecessor-local chain on the primary asymmetric join input");
  expect_contains(rendered,
                  "else:\n  %t8 = add i32 9, 4\n  %t9 = sub i32 %t8, 2\n  %t10 = add i32 %t9, 11\n  %t11 = sub i32 %t10, 6\n  br label %join",
                  "backend IR printer should preserve the bounded four-op alternate predecessor-local chain on the asymmetric join input");
  expect_contains(rendered,
                  "join:\n  %t.join = phi i32 [ %t7, %then ], [ %t11, %else ]\n  %t13 = add i32 %t.join, 6",
                  "backend IR printer should preserve the asymmetric five-op-chain/four-op-chain predecessor merge plus join-local add");
  expect_contains(rendered, "ret i32 %t13",
                  "backend IR printer should preserve the computed return for mixed five-op predecessor/four-op alternate post-phi joins");
}

void test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_five_op_chain_and_four_op_chain_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_five_op_chain_and_four_op_chain_post_join_add_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept mixed predecessor joins whose primary edge widens beyond the current four-op chain while the alternate edge remains a bounded four-op chain");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid mixed five-op predecessor/four-op alternate post-phi joins");
}

void test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_five_op_chain_and_five_op_chain_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_five_op_chain_and_five_op_chain_post_join_add_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered,
                  "then:\n  %t3 = add i32 20, 5\n  %t4 = sub i32 %t3, 3\n  %t5 = add i32 %t4, 8\n  %t6 = sub i32 %t5, 4\n  %t7 = add i32 %t6, 10\n  br label %join",
                  "backend IR printer should preserve the bounded five-op predecessor-local chain on the primary asymmetric join input");
  expect_contains(rendered,
                  "else:\n  %t8 = add i32 9, 4\n  %t9 = sub i32 %t8, 2\n  %t10 = add i32 %t9, 11\n  %t11 = sub i32 %t10, 6\n  %t12 = add i32 %t11, 13\n  br label %join",
                  "backend IR printer should widen the alternate predecessor-local chain beyond the current four-op depth on the asymmetric join input");
  expect_contains(rendered,
                  "join:\n  %t.join = phi i32 [ %t7, %then ], [ %t12, %else ]\n  %t14 = add i32 %t.join, 6",
                  "backend IR printer should preserve the asymmetric five-op-chain/five-op-chain predecessor merge plus join-local add");
  expect_contains(rendered, "ret i32 %t14",
                  "backend IR printer should preserve the computed return for mixed five-op predecessor/five-op alternate post-phi joins");
}

void test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_five_op_chain_and_five_op_chain_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_five_op_chain_and_five_op_chain_post_join_add_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept mixed predecessor joins whose alternate edge widens beyond the current four-op chain while the primary edge remains a bounded five-op chain");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid mixed five-op predecessor/five-op alternate post-phi joins");
}

void test_backend_ir_printer_renders_lowered_countdown_while_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_countdown_while_return_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "entry:\n  br label %block_1",
                  "backend IR printer should preserve the explicit countdown-loop entry branch");
  expect_contains(rendered, "%t.iter = phi i32 [ 50, %entry ], [ %t.dec, %block_2 ]",
                  "backend IR printer should render the lowered loop-carried phi state");
  expect_contains(rendered, "%t.keep_going = icmp ne i32 %t.iter, 0",
                  "backend IR printer should render the lowered countdown compare");
  expect_contains(rendered, "br i1 %t.keep_going, label %block_2, label %block_3",
                  "backend IR printer should render the lowered loop backedge split");
  expect_contains(rendered, "block_2:\n  %t.dec = sub i32 %t.iter, 1\n  br label %block_1",
                  "backend IR printer should render the lowered loop body decrement and backedge");
  expect_contains(rendered, "block_3:\n  ret i32 %t.iter",
                  "backend IR printer should preserve the lowered loop exit return");
}

void test_backend_ir_validator_accepts_lowered_countdown_while_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_countdown_while_return_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept lowered countdown-loop backedge slices");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid lowered countdown loops");
}

void test_backend_ir_printer_renders_lowered_string_literal_char_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_string_literal_char_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered,
                  "@.str0 = private unnamed_addr constant [3 x i8] c\"hi\\00\"\n",
                  "backend IR printer should render lowered string-pool entities");
  expect_contains(rendered, "%t4 = load i32 from i8 sext, ptr @.str0 + 1\n",
                  "backend IR printer should render the lowered widened string-literal byte load");
  expect_contains(rendered, "ret i32 %t4",
                  "backend IR printer should preserve the lowered string-literal return");
}

void test_backend_ir_validator_accepts_lowered_string_literal_char_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_string_literal_char_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept lowered string-literal slices");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid lowered string constants");
}

void test_backend_ir_printer_renders_structured_string_literal_char_slice_without_type_text() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_string_literal_char_module());
  clear_backend_memory_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "%t4 = load i32 from i8 sext, ptr @.str0 + 1\n",
                  "backend IR printer should render widened string-literal loads from structured scalar metadata without legacy type text");
}

void test_backend_ir_validator_accepts_structured_string_literal_char_slice_without_type_text() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_string_literal_char_module());
  clear_backend_memory_type_compatibility_shims(lowered);
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept widened string-literal loads described only by structured scalar metadata");
  expect_true(error.empty(),
              "backend IR validator should not report an error when widened string-literal load type shims are cleared");
}

void test_backend_ir_validator_rejects_string_literal_load_past_structured_bounds() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_string_literal_char_module());
  auto& load = std::get<c4c::backend::BackendLoadInst>(
      lowered.functions.front().blocks.front().insts.front());
  load.address.byte_offset = 3;
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured string-literal loads that exceed the referenced byte bounds");
  expect_contains(error, "load address: address exceeds referenced global bounds",
                  "backend IR validator should explain when a structured string-literal load escapes the referenced bytes");
}

void test_backend_ir_validator_rejects_load_from_unknown_global_style_symbol() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_load_module());
  auto& load = std::get<c4c::backend::BackendLoadInst>(
      lowered.functions.front().blocks.front().insts.front());
  load.address.base_symbol = "g_missing";
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured loads that target unknown global-style symbols");
  expect_contains(
      error,
      "load address: base symbol must reference a known global, string constant, or local slot",
      "backend IR validator should explain when a structured load references an unknown global-style symbol");
}

void test_backend_ir_validator_rejects_store_to_string_literal_constant() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_string_literal_char_module());
  auto& insts = lowered.functions.front().blocks.front().insts;
  insts.insert(insts.begin(),
               c4c::backend::BackendStoreInst{
                   "i8",
                   "65",
                   c4c::backend::BackendAddress{".str0", 0},
                   c4c::backend::BackendScalarType::I8,
               });
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured stores that target immutable string constants");
  expect_contains(error, "stores must not target referenced string constants",
                  "backend IR validator should explain when a structured store targets a string constant");
}

void test_backend_ir_validator_rejects_store_to_unknown_global_style_symbol() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_store_reload_module());
  auto& store = std::get<c4c::backend::BackendStoreInst>(
      lowered.functions.front().blocks.front().insts.front());
  store.address.base_symbol = "g_missing";
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured stores that target unknown global-style symbols");
  expect_contains(
      error,
      "store address: base symbol must reference a known global, string constant, or local slot",
      "backend IR validator should explain when a structured store references an unknown global-style symbol");
}

void test_backend_ir_validator_rejects_local_slot_load_with_negative_offset() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_local_array_gep_module());
  auto& load = std::get<c4c::backend::BackendLoadInst>(
      lowered.functions.front().blocks.front().insts[2]);
  load.address.byte_offset = -4;
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured local-slot loads that use negative byte offsets");
  expect_contains(error, "load address: address byte offset must not be negative",
                  "backend IR validator should explain when a structured local-slot load uses a negative offset");
}

void test_backend_ir_validator_rejects_local_slot_store_with_negative_offset() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_local_array_gep_module());
  auto& store = std::get<c4c::backend::BackendStoreInst>(
      lowered.functions.front().blocks.front().insts.front());
  store.address.byte_offset = -4;
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured local-slot stores that use negative byte offsets");
  expect_contains(error, "store address: address byte offset must not be negative",
                  "backend IR validator should explain when a structured local-slot store uses a negative offset");
}

void test_backend_ir_validator_rejects_local_slot_load_with_misaligned_offset() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_local_array_gep_module());
  auto& load = std::get<c4c::backend::BackendLoadInst>(
      lowered.functions.front().blocks.front().insts[2]);
  load.address.byte_offset = 2;
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured local-slot loads whose byte offset is not aligned to the load memory size");
  expect_contains(error, "load address: address byte offset must align to local access size",
                  "backend IR validator should explain when a structured local-slot load uses a misaligned byte offset");
}

void test_backend_ir_validator_rejects_local_slot_store_with_misaligned_offset() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_local_array_gep_module());
  auto& store = std::get<c4c::backend::BackendStoreInst>(
      lowered.functions.front().blocks.front().insts.front());
  store.address.byte_offset = 2;
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured local-slot stores whose byte offset is not aligned to the store value size");
  expect_contains(error, "store address: address byte offset must align to local access size",
                  "backend IR validator should explain when a structured local-slot store uses a misaligned byte offset");
}

void test_backend_ir_validator_rejects_local_slot_load_past_structured_bounds() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_local_array_gep_module());
  auto& load = std::get<c4c::backend::BackendLoadInst>(
      lowered.functions.front().blocks.front().insts[2]);
  load.address.byte_offset = 8;
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured local-slot loads that exceed the bounded lowered local-array slot");
  expect_contains(error, "load address: address exceeds referenced global bounds",
                  "backend IR validator should explain when a structured local-slot load escapes the bounded local slot");
}

void test_backend_ir_validator_rejects_local_slot_store_past_structured_bounds() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_local_array_gep_module());
  auto& store = std::get<c4c::backend::BackendStoreInst>(
      lowered.functions.front().blocks.front().insts.front());
  store.address.byte_offset = 8;
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured local-slot stores that exceed the bounded lowered local-array slot");
  expect_contains(error, "store address: address exceeds referenced global bounds",
                  "backend IR validator should explain when a structured local-slot store escapes the bounded local slot");
}

void test_backend_ir_printer_renders_lowered_global_int_pointer_roundtrip_slice() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_global_int_pointer_roundtrip_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "@g_value = global i32 9, align 4\n",
                  "backend IR printer should keep the bounded round-trip global definition");
  expect_contains(rendered, "%t4 = load i32, ptr @g_value\n",
                  "backend IR printer should collapse the bounded round-trip back into a direct global load");
  expect_not_contains(rendered, "ptrtoint",
                      "backend IR printer should not preserve ptrtoint once the bounded round-trip is lowered");
  expect_not_contains(rendered, "inttoptr",
                      "backend IR printer should not preserve inttoptr once the bounded round-trip is lowered");
  expect_contains(rendered, "ret i32 %t4",
                  "backend IR printer should preserve the bounded round-trip return");
}

void test_backend_ir_validator_accepts_lowered_global_int_pointer_roundtrip_slice() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_global_int_pointer_roundtrip_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept lowered global int pointer round-trip slices");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid lowered round-trip globals");
}

void test_backend_ir_printer_renders_lowered_global_char_pointer_diff_slice() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_global_char_pointer_diff_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "@g_bytes = global [2 x i8] zeroinitializer\n",
                  "backend IR printer should keep the bounded char pointer-difference global definition");
  expect_contains(rendered,
                  "%t11 = ptrdiff_eq i32, ptr @g_bytes + 1, ptr @g_bytes, elem_size 1, expected 1\n",
                  "backend IR printer should collapse the bounded char pointer-difference slice into a backend-owned compare");
  expect_not_contains(rendered, "ptrtoint",
                      "backend IR printer should not preserve ptrtoint once the bounded char pointer-difference slice is lowered");
  expect_contains(rendered, "ret i32 %t11",
                  "backend IR printer should preserve the bounded char pointer-difference return");
}

void test_backend_ir_validator_accepts_lowered_global_char_pointer_diff_slice() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_global_char_pointer_diff_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept lowered global char pointer-difference slices");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid lowered char pointer-difference globals");
}

void test_backend_ir_printer_renders_lowered_global_int_pointer_diff_slice() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_global_int_pointer_diff_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "@g_words = global [2 x i32] zeroinitializer, align 4\n",
                  "backend IR printer should keep the bounded int pointer-difference global definition");
  expect_contains(rendered,
                  "%t12 = ptrdiff_eq i32, ptr @g_words + 4, ptr @g_words, elem_size 4, expected 1\n",
                  "backend IR printer should collapse the bounded int pointer-difference slice into a backend-owned compare");
  expect_not_contains(rendered, "ptrtoint",
                      "backend IR printer should not preserve ptrtoint once the bounded int pointer-difference slice is lowered");
  expect_not_contains(rendered, "sdiv",
                      "backend IR printer should not preserve integer scaling once the bounded int pointer-difference slice is lowered");
  expect_contains(rendered, "ret i32 %t12",
                  "backend IR printer should preserve the bounded int pointer-difference return");
}

void test_backend_ir_validator_accepts_lowered_global_int_pointer_diff_slice() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_global_int_pointer_diff_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept lowered global int pointer-difference slices");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid lowered int pointer-difference globals");
}

void test_backend_ir_printer_renders_structured_global_int_pointer_diff_slice_without_type_text() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_int_pointer_diff_module());
  clear_backend_memory_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered,
                  "%t12 = ptrdiff_eq i32, ptr @g_words + 4, ptr @g_words, elem_size 4, expected 1\n",
                  "backend IR printer should render ptrdiff slices from structured scalar metadata without legacy result type text");
}

void test_backend_ir_validator_accepts_structured_global_int_pointer_diff_slice_without_type_text() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_int_pointer_diff_module());
  clear_backend_memory_type_compatibility_shims(lowered);
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept ptrdiff slices described only by structured scalar metadata");
  expect_true(error.empty(),
              "backend IR validator should not report an error when ptrdiff result type shims are cleared");
}

void test_backend_ir_printer_renders_lowered_extern_global_array_load_slice() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_extern_global_array_load_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "@ext_arr = external global [2 x i32], align 4\n",
                  "backend IR printer should render lowered extern global declarations");
  expect_contains(rendered, "%t3 = load i32, ptr @ext_arr + 4\n",
                  "backend IR printer should render lowered extern global array loads");
  expect_contains(rendered, "ret i32 %t3",
                  "backend IR printer should preserve the lowered extern global array load return");
}

void test_backend_ir_printer_renders_structured_extern_global_array_load_slice_without_raw_type_text() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_extern_global_array_load_module());
  clear_backend_global_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "@ext_arr = external global [2 x i32], align 4\n",
                  "backend IR printer should render extern global arrays from structured array metadata when raw global type text is cleared");
}

void test_backend_ir_validator_accepts_structured_extern_global_array_load_slice_without_raw_type_text() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_extern_global_array_load_module());
  clear_backend_global_type_compatibility_shims(lowered);
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept extern global arrays described only by structured array metadata");
  expect_true(error.empty(),
              "backend IR validator should not report an error when raw global array type text is cleared");
}

void test_backend_ir_validator_rejects_structured_global_array_with_nonscalar_element_type() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_extern_global_array_load_module());
  clear_backend_global_type_compatibility_shims(lowered);
  auto& global = lowered.globals.front();
  global.array_type = c4c::backend::BackendGlobalArrayType{
      2,
      "ptr",
      c4c::backend::BackendValueTypeKind::Ptr,
      c4c::backend::BackendScalarType::Unknown,
  };
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured global arrays whose element metadata is not a sized scalar");
  expect_contains(error, "structured global arrays must use a sized scalar element type",
                  "backend IR validator should explain invalid structured global-array element metadata");
}

void test_backend_ir_validator_rejects_extern_global_array_load_past_structured_bounds() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_extern_global_array_load_module());
  auto& load = std::get<c4c::backend::BackendLoadInst>(
      lowered.functions.front().blocks.front().insts.front());
  load.address.byte_offset = 8;
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured extern global array loads that exceed the referenced array bounds");
  expect_contains(error, "address exceeds referenced global bounds",
                  "backend IR validator should explain when a structured global-array load offset exceeds the referenced array");
}

void test_backend_ir_validator_rejects_global_int_ptrdiff_past_structured_bounds() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_int_pointer_diff_module());
  auto& ptrdiff = std::get<c4c::backend::BackendPtrDiffEqInst>(
      lowered.functions.front().blocks.front().insts.back());
  ptrdiff.lhs_address.byte_offset = 8;
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured ptrdiff slices whose referenced address escapes the global array bounds");
  expect_contains(error, "ptrdiff lhs address: address exceeds referenced global bounds",
                  "backend IR validator should explain when a structured ptrdiff address escapes the referenced global array");
}

void test_backend_ir_validator_rejects_ptrdiff_across_different_structured_globals() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_int_pointer_diff_module());
  auto other_global = lowered.globals.front();
  other_global.name = "g_words_other";
  lowered.globals.push_back(other_global);
  auto& ptrdiff = std::get<c4c::backend::BackendPtrDiffEqInst>(
      lowered.functions.front().blocks.front().insts.back());
  ptrdiff.rhs_address.base_symbol = "g_words_other";
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured ptrdiff slices that compare addresses from different globals");
  expect_contains(error, "ptrdiff addresses must reference the same global",
                  "backend IR validator should explain cross-global structured ptrdiff mismatches");
}

void test_backend_ir_validator_rejects_ptrdiff_with_only_one_global_backed_address() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_int_pointer_diff_module());
  auto& ptrdiff = std::get<c4c::backend::BackendPtrDiffEqInst>(
      lowered.functions.front().blocks.front().insts.back());
  ptrdiff.rhs_address.base_symbol = "%stack_slot";
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured ptrdiff slices when only one address is global-backed");
  expect_contains(
      error,
      "ptrdiff addresses must both reference the same global when either side is global-backed",
      "backend IR validator should explain mixed global-backed ptrdiff address mismatches");
}

void test_backend_ir_validator_rejects_ptrdiff_with_unknown_global_style_symbol() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_int_pointer_diff_module());
  auto& ptrdiff = std::get<c4c::backend::BackendPtrDiffEqInst>(
      lowered.functions.front().blocks.front().insts.back());
  ptrdiff.rhs_address.base_symbol = "g_missing";
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured ptrdiff slices that reference unknown global-style symbols");
  expect_contains(
      error,
      "ptrdiff rhs address: base symbol must reference a known global, string constant, or local slot",
      "backend IR validator should explain when structured ptrdiff addresses reference unknown global-style symbols");
}

void test_backend_ir_validator_rejects_local_slot_ptrdiff_with_negative_offset() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_int_pointer_diff_module());
  auto& ptrdiff = std::get<c4c::backend::BackendPtrDiffEqInst>(
      lowered.functions.front().blocks.front().insts.back());
  ptrdiff.lhs_address.base_symbol = "%stack_slot";
  ptrdiff.rhs_address.base_symbol = "%stack_slot";
  ptrdiff.lhs_address.byte_offset = -4;
  ptrdiff.rhs_address.byte_offset = 0;
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured local-slot ptrdiff slices that use negative byte offsets");
  expect_contains(error, "ptrdiff lhs address: address byte offset must not be negative",
                  "backend IR validator should explain when a structured local-slot ptrdiff address uses a negative offset");
}

void test_backend_ir_validator_rejects_local_slot_ptrdiff_with_misaligned_offset() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_int_pointer_diff_module());
  auto& ptrdiff = std::get<c4c::backend::BackendPtrDiffEqInst>(
      lowered.functions.front().blocks.front().insts.back());
  ptrdiff.lhs_address.base_symbol = "%stack_slot";
  ptrdiff.rhs_address.base_symbol = "%stack_slot";
  ptrdiff.lhs_address.byte_offset = 2;
  ptrdiff.rhs_address.byte_offset = 0;
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured local-slot ptrdiff slices whose byte offsets are not aligned to the ptrdiff element size");
  expect_contains(error, "ptrdiff lhs address: address byte offset must align to local access size",
                  "backend IR validator should explain when a structured local-slot ptrdiff address uses a misaligned byte offset");
}

void test_backend_ir_validator_accepts_structured_local_slot_ptrdiff_slice_without_raw_text() {
  auto lowered = make_structured_local_slot_ptrdiff_module();
  clear_backend_memory_type_compatibility_shims(lowered);
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept structured local-slot ptrdiff slices when local-slot bounds metadata and ptrdiff scalar metadata remain after clearing legacy type text");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid structured local-slot ptrdiff slices without raw type text");
}

void test_backend_ir_validator_rejects_local_slot_ptrdiff_past_structured_bounds() {
  auto lowered = make_structured_local_slot_ptrdiff_module();
  auto& ptrdiff = std::get<c4c::backend::BackendPtrDiffEqInst>(
      lowered.functions.front().blocks.front().insts.back());
  ptrdiff.lhs_address.byte_offset = 8;
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured local-slot ptrdiff slices whose referenced address escapes the bounded local slot");
  expect_contains(error, "ptrdiff lhs address: address exceeds referenced global bounds",
                  "backend IR validator should explain when a structured local-slot ptrdiff address escapes the bounded local slot");
}

void test_backend_ir_validator_rejects_ptrdiff_across_different_structured_local_slots() {
  auto lowered = make_structured_cross_local_slot_ptrdiff_module();
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured ptrdiff slices that compare addresses from different local slots");
  expect_contains(error, "ptrdiff addresses must reference the same local slot",
                  "backend IR validator should explain cross-local structured ptrdiff mismatches");
}

void test_backend_ir_printer_renders_return_add() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_return_add_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);
  expect_contains(rendered, "%t0 = add i32 2, 3",
                  "backend IR printer should render the lowered add instruction");
  expect_contains(rendered, "ret i32 %t0",
                  "backend IR printer should render the lowered return");
}

void test_backend_ir_validator_accepts_lowered_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_return_add_module());
  std::string error;
  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept a lowered minimal slice");
  expect_true(error.empty(),
              "backend IR validator should not report an error for a valid module");
}

void test_backend_ir_validator_rejects_definition_without_blocks() {
  c4c::backend::BackendModule module;
  c4c::backend::BackendFunction function;
  function.signature.linkage = "define";
  function.signature.linkage_kind = c4c::backend::BackendFunctionLinkage::Define;
  function.signature.return_type = "i32";
  function.signature.name = "main";
  module.functions.push_back(function);

  std::string error;
  expect_true(!c4c::backend::validate_backend_ir(module, &error),
              "backend IR validator should reject definitions without blocks");
  expect_contains(error, "definitions must have at least one block",
                  "backend IR validator should explain the malformed definition");
}

void test_backend_ir_printer_renders_structured_global_constant_initializer() {
  c4c::backend::BackendModule module;
  c4c::backend::BackendGlobal global;
  global.name = "g_const";
  global.linkage_kind = c4c::backend::BackendGlobalLinkage::Default;
  global.storage = c4c::backend::BackendGlobalStorageKind::Constant;
  global.llvm_type = "i32";
  global.initializer = c4c::backend::BackendGlobalInitializer::integer_literal(7);
  global.align_bytes = 4;
  module.globals.push_back(std::move(global));

  const auto rendered = c4c::backend::print_backend_ir(module);
  expect_contains(rendered, "@g_const = constant i32 7, align 4\n",
                  "backend IR printer should render structured constant globals without falling back to raw qualifier text");
}

void test_backend_ir_printer_renders_structured_extern_global_declaration() {
  c4c::backend::BackendModule module;
  c4c::backend::BackendGlobal global;
  global.name = "ext_counter";
  global.linkage = "external ";
  global.linkage_kind = c4c::backend::BackendGlobalLinkage::External;
  global.storage = c4c::backend::BackendGlobalStorageKind::Mutable;
  global.llvm_type = "i32";
  global.initializer = c4c::backend::BackendGlobalInitializer::declaration();
  global.align_bytes = 4;
  module.globals.push_back(std::move(global));

  const auto rendered = c4c::backend::print_backend_ir(module);
  expect_contains(rendered, "@ext_counter = external global i32, align 4\n",
                  "backend IR printer should render structured extern globals without a raw initializer payload");
}

void test_backend_ir_validator_rejects_structured_defined_global_without_initializer() {
  c4c::backend::BackendModule module;
  c4c::backend::BackendGlobal global;
  global.name = "g_missing_init";
  global.linkage_kind = c4c::backend::BackendGlobalLinkage::Default;
  global.storage = c4c::backend::BackendGlobalStorageKind::Mutable;
  global.llvm_type = "i32";
  global.initializer = c4c::backend::BackendGlobalInitializer::declaration();
  module.globals.push_back(std::move(global));

  std::string error;
  expect_true(!c4c::backend::validate_backend_ir(module, &error),
              "backend IR validator should reject structured global definitions that omit an initializer");
  expect_contains(error, "defined globals must have an initializer",
                  "backend IR validator should explain missing structured initializers");
}

void test_backend_ir_printer_renders_structured_function_linkage_without_raw_text() {
  c4c::backend::BackendModule module;
  c4c::backend::BackendFunction function;
  function.signature.linkage_kind = c4c::backend::BackendFunctionLinkage::Define;
  function.signature.return_type = "i32";
  function.signature.name = "main";
  function.blocks.push_back(c4c::backend::BackendBlock{
      "entry",
      {},
      c4c::backend::make_backend_return(std::string("0"), "i32"),
  });
  module.functions.push_back(function);

  const auto rendered = c4c::backend::print_backend_ir(module);
  expect_contains(rendered, "define i32 @main()",
                  "backend IR printer should render structured function linkage without raw signature text");
}

void test_backend_ir_printer_renders_structured_signature_and_call_types_without_raw_text() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_x86_extern_decl_object_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  const auto rendered = c4c::backend::print_backend_ir(lowered);
  expect_contains(rendered, "declare i32 @helper_ext()\n",
                  "backend IR printer should render structured declaration return types without raw signature text");
  expect_contains(rendered, "define i32 @main()",
                  "backend IR printer should render structured definition return types without raw signature text");
  expect_contains(rendered, "%t0 = call i32 () @helper_ext()",
                  "backend IR printer should render structured call return and param types without raw call text");
}

void test_backend_ir_printer_renders_structured_call_arg_types_without_raw_text() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  const auto rendered = c4c::backend::print_backend_ir(lowered);
  expect_contains(rendered, "%t0 = call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "backend IR printer should render typed call operands from structured call metadata when raw arg type text is cleared");
}

void test_backend_ir_validator_accepts_structured_signature_and_call_types_without_raw_text() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_x86_extern_decl_object_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept structured signature and call types when the legacy text shims are cleared");
  expect_true(error.empty(),
              "backend IR validator should not report an error for structured signature and call type metadata without compatibility text");
}

void test_backend_ir_tracks_structured_two_arg_direct_call_signature_and_call_contract() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_module());
  const auto find_function = [&lowered](std::string_view name)
      -> const c4c::backend::BackendFunction* {
    for (const auto& function : lowered.functions) {
      if (function.signature.name == name) {
        return &function;
      }
    }
    return nullptr;
  };
  const auto* helper = find_function("add_pair");
  const auto* main_fn = find_function("main");

  expect_true(helper != nullptr && main_fn != nullptr,
              "backend IR should preserve both functions in the two-argument direct-call slice");
  expect_true(helper != nullptr && c4c::backend::backend_function_is_definition(helper->signature) &&
                  c4c::backend::backend_signature_return_scalar_type(helper->signature) ==
                      c4c::backend::BackendScalarType::I32,
              "backend IR should preserve the helper as a structured i32 definition");
  expect_true(helper != nullptr && helper->signature.params.size() == 2 &&
                  c4c::backend::backend_param_scalar_type(helper->signature.params[0]) ==
                      c4c::backend::BackendScalarType::I32 &&
                  c4c::backend::backend_param_scalar_type(helper->signature.params[1]) ==
                      c4c::backend::BackendScalarType::I32,
              "backend IR should preserve both helper parameters as structured i32 values");
  expect_true(main_fn != nullptr && c4c::backend::backend_function_is_definition(main_fn->signature) &&
                  main_fn->signature.params.empty() && !main_fn->signature.is_vararg,
              "backend IR should preserve the main entry signature separately from legacy signature text");

  const auto* call =
      main_fn == nullptr || main_fn->blocks.empty() || main_fn->blocks.front().insts.empty()
          ? nullptr
          : std::get_if<c4c::backend::BackendCallInst>(&main_fn->blocks.front().insts.front());
  expect_true(call != nullptr && call->callee.kind ==
                                  c4c::backend::BackendCallCalleeKind::DirectGlobal &&
                  call->callee.symbol_name == "add_pair",
              "backend IR should preserve the helper call as a direct-global backend call");
  expect_true(call != nullptr &&
                  c4c::backend::backend_call_return_scalar_type(*call) ==
                      c4c::backend::BackendScalarType::I32 &&
                  call->param_types.size() == 2 &&
                  c4c::backend::backend_call_param_scalar_type(*call, 0) ==
                      c4c::backend::BackendScalarType::I32 &&
                  c4c::backend::backend_call_param_scalar_type(*call, 1) ==
                      c4c::backend::BackendScalarType::I32,
              "backend IR should preserve the structured i32 call signature for the two-argument helper call");
  expect_true(call != nullptr && call->args.size() == 2 && call->args[0].operand == "5" &&
                  call->args[1].operand == "7",
              "backend IR should preserve the normalized two-argument direct-call operands");
}

void test_backend_ir_tracks_structured_single_arg_direct_call_return_contract() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_module());

  const auto find_function = [&](std::string_view name) -> const c4c::backend::BackendFunction* {
    for (const auto& function : lowered.functions) {
      if (function.signature.name == name) {
        return &function;
      }
    }
    return nullptr;
  };

  const auto* main_fn = find_function("main");
  expect_true(main_fn != nullptr && c4c::backend::backend_function_is_definition(main_fn->signature) &&
                  main_fn->blocks.size() == 1,
              "backend IR should preserve the single-argument direct-call main body as a structured definition");

  const auto* call =
      main_fn == nullptr || main_fn->blocks.empty() || main_fn->blocks.front().insts.empty()
          ? nullptr
          : std::get_if<c4c::backend::BackendCallInst>(&main_fn->blocks.front().insts.front());
  expect_true(call != nullptr && call->callee.kind ==
                                  c4c::backend::BackendCallCalleeKind::DirectGlobal &&
                  call->callee.symbol_name == "add_one",
              "backend IR should preserve the single-argument helper call as a direct-global backend call");
  expect_true(call != nullptr &&
                  c4c::backend::backend_call_return_type_kind(*call) ==
                      c4c::backend::BackendValueTypeKind::Scalar &&
                  c4c::backend::backend_call_return_scalar_type(*call) ==
                      c4c::backend::BackendScalarType::I32 &&
                  call->param_types.size() == 1 &&
                  c4c::backend::backend_call_param_scalar_type(*call, 0) ==
                      c4c::backend::BackendScalarType::I32,
              "backend IR should preserve structured single-argument call return and parameter metadata");
  expect_true(call != nullptr && call->args.size() == 1 && call->args[0].operand == "5",
              "backend IR should preserve the normalized single-argument direct-call operand");
}

void test_backend_ir_validator_accepts_structured_direct_call_add_imm_slice_without_raw_text() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_local_arg_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);
  clear_backend_memory_type_compatibility_shims(lowered);
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept lowered direct-call add-immediate slices when structured signature, call, and binary type metadata remain after clearing legacy text");
  expect_true(error.empty(),
              "backend IR validator should not report an error for structured direct-call add-immediate slices without compatibility text");
}

void test_backend_ir_preserves_structured_local_array_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_local_array_gep_module());

  const auto* main_fn = lowered.functions.empty() ? nullptr : &lowered.functions.front();
  expect_true(main_fn != nullptr && c4c::backend::backend_function_is_definition(main_fn->signature) &&
                  c4c::backend::backend_signature_return_scalar_type(main_fn->signature) ==
                      c4c::backend::BackendScalarType::I32 &&
                  main_fn->signature.name == "main" && main_fn->signature.params.empty() &&
                  main_fn->blocks.size() == 1,
              "backend IR should preserve the bounded local-array slice as a structured i32 main definition");

  const auto* store0 =
      main_fn != nullptr && !main_fn->blocks.empty() && main_fn->blocks.front().insts.size() > 0
          ? std::get_if<c4c::backend::BackendStoreInst>(&main_fn->blocks.front().insts[0])
          : nullptr;
  const auto* store1 =
      main_fn != nullptr && !main_fn->blocks.empty() && main_fn->blocks.front().insts.size() > 1
          ? std::get_if<c4c::backend::BackendStoreInst>(&main_fn->blocks.front().insts[1])
          : nullptr;
  const auto* load0 =
      main_fn != nullptr && !main_fn->blocks.empty() && main_fn->blocks.front().insts.size() > 2
          ? std::get_if<c4c::backend::BackendLoadInst>(&main_fn->blocks.front().insts[2])
          : nullptr;
  const auto* load1 =
      main_fn != nullptr && !main_fn->blocks.empty() && main_fn->blocks.front().insts.size() > 3
          ? std::get_if<c4c::backend::BackendLoadInst>(&main_fn->blocks.front().insts[3])
          : nullptr;
  const auto* add =
      main_fn != nullptr && !main_fn->blocks.empty() && main_fn->blocks.front().insts.size() > 4
          ? std::get_if<c4c::backend::BackendBinaryInst>(&main_fn->blocks.front().insts[4])
          : nullptr;

  expect_true(store0 != nullptr && store1 != nullptr && load0 != nullptr && load1 != nullptr &&
                  add != nullptr && c4c::backend::backend_store_value_type(*store0) ==
                                       c4c::backend::BackendScalarType::I32 &&
                  c4c::backend::backend_store_value_type(*store1) ==
                      c4c::backend::BackendScalarType::I32 &&
                  c4c::backend::backend_load_value_type(*load0) ==
                      c4c::backend::BackendScalarType::I32 &&
                  c4c::backend::backend_load_memory_type(*load0) ==
                      c4c::backend::BackendScalarType::I32 &&
                  c4c::backend::backend_load_value_type(*load1) ==
                      c4c::backend::BackendScalarType::I32 &&
                  c4c::backend::backend_load_memory_type(*load1) ==
                      c4c::backend::BackendScalarType::I32 &&
                  c4c::backend::backend_binary_value_type(*add) ==
                      c4c::backend::BackendScalarType::I32,
              "backend IR should preserve structured local-array store, load, and add metadata");
  expect_true(store0 != nullptr && store1 != nullptr && load0 != nullptr && load1 != nullptr &&
                  store0->address.base_symbol == "%lv.arr" &&
                  store1->address.base_symbol == "%lv.arr" &&
                  load0->address.base_symbol == "%lv.arr" &&
                  load1->address.base_symbol == "%lv.arr" &&
                  store0->address.byte_offset == 0 && store1->address.byte_offset == 4 &&
                  load0->address.byte_offset == 0 && load1->address.byte_offset == 4,
              "backend IR should preserve the bounded local-array stack-slot base and folded element offsets");
  expect_true(store0 != nullptr && store1 != nullptr && load0 != nullptr && load1 != nullptr &&
                  add != nullptr && store0->value == "4" && store1->value == "3" &&
                  ((add->lhs == load0->result && add->rhs == load1->result) ||
                   (add->lhs == load1->result && add->rhs == load0->result)),
              "backend IR should preserve the bounded local-array immediate stores and add inputs");
  expect_true(main_fn != nullptr && main_fn->local_slots.size() == 1 &&
                  main_fn->local_slots.front().name == "%lv.arr" &&
                  main_fn->local_slots.front().size_bytes == 8 &&
                  main_fn->local_slots.front().element_type ==
                      c4c::backend::BackendScalarType::I32 &&
                  main_fn->local_slots.front().element_size_bytes == 4,
              "backend IR should preserve structured local-slot bounds metadata for the lowered local-array slice");
}

void test_backend_ir_validator_accepts_structured_local_array_slice_without_raw_text() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_local_array_gep_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);
  clear_backend_memory_type_compatibility_shims(lowered);
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept the structured local-array slice when signature and memory compatibility text are cleared");
  expect_true(error.empty(),
              "backend IR validator should not report an error for structured local-array slices without raw type text");
}

void test_backend_ir_printer_renders_structured_pointer_return_without_raw_text() {
  c4c::backend::BackendModule module;
  c4c::backend::BackendFunction function;
  function.signature.linkage_kind = c4c::backend::BackendFunctionLinkage::Define;
  function.signature.return_type_kind = c4c::backend::BackendValueTypeKind::Ptr;
  function.signature.name = "identity_ptr";

  c4c::backend::BackendTerminator terminator;
  terminator.value = "%p.ret";
  terminator.type_kind = c4c::backend::BackendValueTypeKind::Ptr;
  function.blocks.push_back(c4c::backend::BackendBlock{"entry", {}, std::move(terminator)});
  module.functions.push_back(std::move(function));

  const auto rendered = c4c::backend::print_backend_ir(module);
  expect_contains(rendered, "define ptr @identity_ptr()",
                  "backend IR printer should render structured pointer signature returns without raw text");
  expect_contains(rendered, "ret ptr %p.ret",
                  "backend IR printer should render structured pointer terminators without raw text");
}

void test_backend_ir_validator_accepts_structured_pointer_return_without_raw_text() {
  c4c::backend::BackendModule module;
  c4c::backend::BackendFunction function;
  function.signature.linkage_kind = c4c::backend::BackendFunctionLinkage::Define;
  function.signature.return_type_kind = c4c::backend::BackendValueTypeKind::Ptr;
  function.signature.name = "identity_ptr";

  c4c::backend::BackendTerminator terminator;
  terminator.value = "%p.ret";
  terminator.type_kind = c4c::backend::BackendValueTypeKind::Ptr;
  function.blocks.push_back(c4c::backend::BackendBlock{"entry", {}, std::move(terminator)});
  module.functions.push_back(std::move(function));

  std::string error;
  expect_true(c4c::backend::validate_backend_ir(module, &error),
              "backend IR validator should accept structured pointer terminators without raw text");
  expect_true(error.empty(),
              "backend IR validator should not report an error for structured pointer return metadata without compatibility text");
}

void test_backend_ir_printer_renders_structured_global_linkage_without_raw_text() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_extern_global_array_load_module());
  clear_backend_global_linkage_compatibility_shims(lowered);

  const auto rendered = c4c::backend::print_backend_ir(lowered);
  expect_contains(rendered, "@ext_arr = external global [2 x i32], align 4\n",
                  "backend IR printer should render structured global linkage without raw linkage text");
}

void test_backend_ir_printer_renders_lowered_extern_decl_slice() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_x86_extern_decl_object_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "declare i32 @helper_ext()\n",
                  "backend IR printer should render lowered extern declarations");
  expect_contains(rendered, "%t0 = call i32 () @helper_ext()",
                  "backend IR printer should render the lowered extern call site");
  expect_contains(rendered, "ret i32 %t0",
                  "backend IR printer should preserve the lowered extern call return");
}

void test_backend_ir_validator_accepts_lowered_extern_decl_slice() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_x86_extern_decl_object_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept lowered extern declaration slices");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid lowered extern declarations");
}

void test_backend_ir_printer_renders_lowered_global_load_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_global_load_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "@g_counter = global i32 11, align 4\n",
                  "backend IR printer should render lowered global definitions");
  expect_contains(rendered, "%t0 = load i32, ptr @g_counter\n",
                  "backend IR printer should render lowered global loads");
  expect_contains(rendered, "ret i32 %t0",
                  "backend IR printer should preserve the lowered global load return");
}

void test_backend_ir_validator_accepts_lowered_global_load_slice() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_global_load_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept lowered global-load slices");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid lowered globals");
}

void test_backend_ir_printer_renders_lowered_global_store_reload_slice() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_global_store_reload_module());
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "@g_counter = global i32 11, align 4\n",
                  "backend IR printer should keep the bounded store-reload global definition");
  expect_contains(rendered, "store i32 7, ptr @g_counter\n",
                  "backend IR printer should render lowered scalar global stores");
  expect_contains(rendered, "%t0 = load i32, ptr @g_counter\n",
                  "backend IR printer should render the reload after the lowered scalar global store");
  expect_contains(rendered, "ret i32 %t0",
                  "backend IR printer should preserve the lowered scalar global store-reload return");
}

void test_backend_ir_validator_accepts_lowered_global_store_reload_slice() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_global_store_reload_module());
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept lowered global store-reload slices");
  expect_true(error.empty(),
              "backend IR validator should not report an error for valid lowered scalar global stores");
}

void test_backend_ir_printer_renders_structured_global_store_reload_slice_without_type_text() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_store_reload_module());
  clear_backend_memory_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "store i32 7, ptr @g_counter\n",
                  "backend IR printer should render structured scalar global stores without legacy store type text");
  expect_contains(rendered, "%t0 = load i32, ptr @g_counter\n",
                  "backend IR printer should render structured scalar global reloads without legacy load type text");
}

void test_backend_ir_validator_accepts_structured_global_store_reload_slice_without_type_text() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_store_reload_module());
  clear_backend_memory_type_compatibility_shims(lowered);
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept scalar global store-reload slices described only by structured scalar metadata");
  expect_true(error.empty(),
              "backend IR validator should not report an error when scalar load/store type shims are cleared");
}

void test_backend_ir_printer_renders_structured_scalar_global_without_raw_global_type_text() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_load_module());
  clear_backend_global_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::print_backend_ir(lowered);

  expect_contains(rendered, "@g_counter = global i32 11, align 4\n",
                  "backend IR printer should render structured scalar globals after raw global type text is cleared");
  expect_contains(rendered, "%t0 = load i32, ptr @g_counter\n",
                  "backend IR printer should preserve lowered scalar global loads after raw global type text is cleared");
}

void test_backend_ir_validator_accepts_structured_scalar_global_without_raw_global_type_text() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_load_module());
  clear_backend_global_type_compatibility_shims(lowered);
  std::string error;

  expect_true(c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should accept scalar globals described only by structured global type metadata");
  expect_true(error.empty(),
              "backend IR validator should not report an error when raw scalar global type text is cleared");
}

void test_backend_ir_validator_rejects_scalar_global_load_with_mismatched_structured_memory_type() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_load_module());
  clear_backend_global_type_compatibility_shims(lowered);
  auto& load = std::get<c4c::backend::BackendLoadInst>(
      lowered.functions.front().blocks.front().insts.front());
  load.memory_type = "i8";
  load.memory_value_type = c4c::backend::BackendScalarType::I8;
  load.extension = c4c::backend::BackendLoadExtension::ZeroExtend;
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured scalar-global loads whose memory type disagrees with the referenced global scalar type");
  expect_contains(error, "load memory type: type must match referenced global element type",
                  "backend IR validator should explain scalar-global load memory type mismatches");
}

void test_backend_ir_validator_rejects_scalar_global_store_with_mismatched_structured_type() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_store_reload_module());
  clear_backend_global_type_compatibility_shims(lowered);
  auto& store = std::get<c4c::backend::BackendStoreInst>(
      lowered.functions.front().blocks.front().insts.front());
  store.type_str = "i8";
  store.value_type = c4c::backend::BackendScalarType::I8;
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured scalar-global stores whose store type disagrees with the referenced global scalar type");
  expect_contains(error, "store type: type must match referenced global element type",
                  "backend IR validator should explain scalar-global store type mismatches");
}

void test_backend_ir_validator_rejects_extern_global_array_load_with_mismatched_structured_memory_type() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_extern_global_array_load_module());
  clear_backend_global_type_compatibility_shims(lowered);
  auto& load = std::get<c4c::backend::BackendLoadInst>(
      lowered.functions.front().blocks.front().insts.front());
  load.memory_type = "i8";
  load.memory_value_type = c4c::backend::BackendScalarType::I8;
  load.extension = c4c::backend::BackendLoadExtension::ZeroExtend;
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured global-array loads whose memory type disagrees with the referenced element type");
  expect_contains(error, "load memory type: type must match referenced global element type",
                  "backend IR validator should explain global-array load memory type mismatches");
}

void test_backend_ir_validator_rejects_structured_load_extension_that_does_not_widen() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_load_module());
  clear_backend_global_type_compatibility_shims(lowered);
  auto& load = std::get<c4c::backend::BackendLoadInst>(
      lowered.functions.front().blocks.front().insts.front());
  load.type_str = "i8";
  load.value_type = c4c::backend::BackendScalarType::I8;
  load.memory_type = "i32";
  load.memory_value_type = c4c::backend::BackendScalarType::I32;
  load.extension = c4c::backend::BackendLoadExtension::ZeroExtend;
  std::string error;

  expect_true(!c4c::backend::validate_backend_ir(lowered, &error),
              "backend IR validator should reject structured loads that claim an extension without widening");
  expect_contains(error, "extended loads must widen from memory type to value type",
                  "backend IR validator should explain non-widening structured load extensions");
}
int main(int argc, char* argv[]) {
  if (argc >= 2) test_filter() = argv[1];

  test_backend_ir_printer_renders_return_add();
  test_backend_ir_validator_accepts_lowered_slice();
  test_backend_ir_validator_rejects_definition_without_blocks();
  test_backend_ir_printer_renders_structured_global_constant_initializer();
  test_backend_ir_printer_renders_structured_extern_global_declaration();
  test_backend_ir_validator_rejects_structured_defined_global_without_initializer();
  test_backend_ir_printer_renders_structured_global_linkage_without_raw_text();
  test_backend_ir_printer_renders_structured_function_linkage_without_raw_text();
  test_backend_ir_printer_renders_structured_signature_and_call_types_without_raw_text();
  test_backend_ir_printer_renders_structured_call_arg_types_without_raw_text();
  test_backend_ir_tracks_structured_two_arg_direct_call_signature_and_call_contract();
  test_backend_ir_tracks_structured_single_arg_direct_call_return_contract();
  test_backend_ir_validator_accepts_structured_direct_call_add_imm_slice_without_raw_text();
  test_backend_ir_preserves_structured_local_array_slice();
  test_backend_ir_validator_accepts_structured_local_array_slice_without_raw_text();
  test_backend_ir_printer_renders_lowered_extern_decl_slice();
  test_backend_ir_validator_accepts_lowered_extern_decl_slice();
  test_backend_ir_validator_accepts_structured_signature_and_call_types_without_raw_text();
  test_backend_ir_printer_renders_lowered_global_load_slice();
  test_backend_ir_validator_accepts_lowered_global_load_slice();
  test_backend_ir_printer_renders_structured_scalar_global_without_raw_global_type_text();
  test_backend_ir_validator_accepts_structured_scalar_global_without_raw_global_type_text();
  test_backend_ir_validator_rejects_scalar_global_load_with_mismatched_structured_memory_type();
  test_backend_ir_validator_rejects_scalar_global_store_with_mismatched_structured_type();
  test_backend_ir_printer_renders_lowered_global_store_reload_slice();
  test_backend_ir_validator_accepts_lowered_global_store_reload_slice();
  test_backend_ir_printer_renders_structured_global_store_reload_slice_without_type_text();
  test_backend_ir_validator_accepts_structured_global_store_reload_slice_without_type_text();
  test_backend_ir_printer_renders_lowered_string_literal_char_slice();
  test_backend_ir_validator_accepts_lowered_string_literal_char_slice();
  test_backend_ir_printer_renders_structured_string_literal_char_slice_without_type_text();
  test_backend_ir_validator_accepts_structured_string_literal_char_slice_without_type_text();
  test_backend_ir_validator_rejects_string_literal_load_past_structured_bounds();
  test_backend_ir_validator_rejects_load_from_unknown_global_style_symbol();
  test_backend_ir_validator_rejects_store_to_string_literal_constant();
  test_backend_ir_validator_rejects_store_to_unknown_global_style_symbol();
  test_backend_ir_validator_rejects_local_slot_load_with_negative_offset();
  test_backend_ir_validator_rejects_local_slot_store_with_negative_offset();
  test_backend_ir_validator_rejects_local_slot_load_with_misaligned_offset();
  test_backend_ir_validator_rejects_local_slot_store_with_misaligned_offset();
  test_backend_ir_validator_rejects_local_slot_load_past_structured_bounds();
  test_backend_ir_validator_rejects_local_slot_store_past_structured_bounds();
  test_backend_ir_printer_renders_lowered_global_int_pointer_roundtrip_slice();
  test_backend_ir_validator_accepts_lowered_global_int_pointer_roundtrip_slice();
  test_backend_ir_printer_renders_lowered_global_char_pointer_diff_slice();
  test_backend_ir_validator_accepts_lowered_global_char_pointer_diff_slice();
  test_backend_ir_printer_renders_lowered_global_int_pointer_diff_slice();
  test_backend_ir_validator_accepts_lowered_global_int_pointer_diff_slice();
  test_backend_ir_printer_renders_structured_global_int_pointer_diff_slice_without_type_text();
  test_backend_ir_validator_accepts_structured_global_int_pointer_diff_slice_without_type_text();
  test_backend_ir_printer_renders_lowered_extern_global_array_load_slice();
  test_backend_ir_printer_renders_structured_extern_global_array_load_slice_without_raw_type_text();
  test_backend_ir_validator_accepts_structured_extern_global_array_load_slice_without_raw_type_text();
  test_backend_ir_validator_rejects_structured_global_array_with_nonscalar_element_type();
  test_backend_ir_validator_rejects_extern_global_array_load_past_structured_bounds();
  test_backend_ir_validator_rejects_extern_global_array_load_with_mismatched_structured_memory_type();
  test_backend_ir_validator_rejects_structured_load_extension_that_does_not_widen();
  test_backend_ir_validator_rejects_global_int_ptrdiff_past_structured_bounds();
  test_backend_ir_validator_rejects_ptrdiff_across_different_structured_globals();
  test_backend_ir_validator_rejects_ptrdiff_with_only_one_global_backed_address();
  test_backend_ir_validator_rejects_ptrdiff_with_unknown_global_style_symbol();
  test_backend_ir_validator_rejects_local_slot_ptrdiff_with_negative_offset();
  test_backend_ir_validator_rejects_local_slot_ptrdiff_with_misaligned_offset();
  test_backend_ir_validator_accepts_structured_local_slot_ptrdiff_slice_without_raw_text();
  test_backend_ir_validator_rejects_local_slot_ptrdiff_past_structured_bounds();
  test_backend_ir_validator_rejects_ptrdiff_across_different_structured_local_slots();
  test_backend_ir_printer_renders_lowered_conditional_return_slice();
  test_backend_ir_validator_accepts_lowered_conditional_return_slice();
  test_backend_ir_printer_renders_structured_conditional_return_slice_without_type_text();
  test_backend_ir_validator_accepts_structured_conditional_return_slice_without_type_text();
  test_backend_ir_printer_renders_lowered_conditional_phi_join_slice();
  test_backend_ir_validator_accepts_lowered_conditional_phi_join_slice();
  test_backend_ir_printer_renders_lowered_conditional_phi_join_add_slice();
  test_backend_ir_validator_accepts_lowered_conditional_phi_join_add_slice();
  test_backend_ir_printer_renders_structured_conditional_phi_join_add_slice_without_type_text();
  test_backend_ir_validator_accepts_structured_conditional_phi_join_add_slice_without_type_text();
  test_backend_ir_printer_renders_lowered_conditional_phi_join_predecessor_add_slice();
  test_backend_ir_validator_accepts_lowered_conditional_phi_join_predecessor_add_slice();
  test_backend_ir_printer_renders_lowered_conditional_phi_join_predecessor_sub_slice();
  test_backend_ir_validator_accepts_lowered_conditional_phi_join_predecessor_sub_slice();
  test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_add_slice();
  test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_add_slice();
  test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_add_post_join_add_slice();
  test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_add_post_join_add_slice();
  test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_sub_post_join_add_slice();
  test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_sub_post_join_add_slice();
  test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_add_sub_chain_post_join_add_slice();
  test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_add_sub_chain_post_join_add_slice();
  test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_chain_and_add_post_join_add_slice();
  test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_chain_and_add_post_join_add_slice();
  test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_chain_and_chain_post_join_add_slice();
  test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_chain_and_chain_post_join_add_slice();
  test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_deeper_chain_and_chain_post_join_add_slice();
  test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_deeper_chain_and_chain_post_join_add_slice();
  test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_deeper_chain_and_deeper_chain_post_join_add_slice();
  test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_deeper_chain_and_deeper_chain_post_join_add_slice();
  test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_four_op_chain_and_deeper_chain_post_join_add_slice();
  test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_four_op_chain_and_deeper_chain_post_join_add_slice();
  test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_four_op_chain_and_four_op_chain_post_join_add_slice();
  test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_four_op_chain_and_four_op_chain_post_join_add_slice();
  test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_five_op_chain_and_four_op_chain_post_join_add_slice();
  test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_five_op_chain_and_four_op_chain_post_join_add_slice();
  test_backend_ir_printer_renders_lowered_conditional_phi_join_mixed_predecessor_five_op_chain_and_five_op_chain_post_join_add_slice();
  test_backend_ir_validator_accepts_lowered_conditional_phi_join_mixed_predecessor_five_op_chain_and_five_op_chain_post_join_add_slice();
  test_backend_ir_printer_renders_lowered_countdown_while_slice();
  test_backend_ir_validator_accepts_lowered_countdown_while_slice();
  check_failures();
  return 0;
}
