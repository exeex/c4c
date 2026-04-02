

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
          c4c::backend::BackendReturn::TypeKind::Unknown) {
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
  module.globals.push_back(c4c::backend::BackendGlobal{
      "g_const",
      "",
      c4c::backend::BackendGlobalLinkage::Default,
      c4c::backend::BackendGlobalStorageKind::Constant,
      "i32",
      c4c::backend::BackendGlobalInitializer::integer_literal(7),
      4,
  });

  const auto rendered = c4c::backend::print_backend_ir(module);
  expect_contains(rendered, "@g_const = constant i32 7, align 4\n",
                  "backend IR printer should render structured constant globals without falling back to raw qualifier text");
}

void test_backend_ir_printer_renders_structured_extern_global_declaration() {
  c4c::backend::BackendModule module;
  module.globals.push_back(c4c::backend::BackendGlobal{
      "ext_counter",
      "external ",
      c4c::backend::BackendGlobalLinkage::External,
      c4c::backend::BackendGlobalStorageKind::Mutable,
      "i32",
      c4c::backend::BackendGlobalInitializer::declaration(),
      4,
  });

  const auto rendered = c4c::backend::print_backend_ir(module);
  expect_contains(rendered, "@ext_counter = external global i32, align 4\n",
                  "backend IR printer should render structured extern globals without a raw initializer payload");
}

void test_backend_ir_validator_rejects_structured_defined_global_without_initializer() {
  c4c::backend::BackendModule module;
  module.globals.push_back(c4c::backend::BackendGlobal{
      "g_missing_init",
      "",
      c4c::backend::BackendGlobalLinkage::Default,
      c4c::backend::BackendGlobalStorageKind::Mutable,
      "i32",
      c4c::backend::BackendGlobalInitializer::declaration(),
      0,
  });

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
  test_backend_ir_printer_renders_lowered_extern_decl_slice();
  test_backend_ir_validator_accepts_lowered_extern_decl_slice();
  test_backend_ir_validator_accepts_structured_signature_and_call_types_without_raw_text();
  test_backend_ir_printer_renders_lowered_global_load_slice();
  test_backend_ir_validator_accepts_lowered_global_load_slice();
  test_backend_ir_printer_renders_lowered_global_store_reload_slice();
  test_backend_ir_validator_accepts_lowered_global_store_reload_slice();
  test_backend_ir_printer_renders_structured_global_store_reload_slice_without_type_text();
  test_backend_ir_validator_accepts_structured_global_store_reload_slice_without_type_text();
  test_backend_ir_printer_renders_lowered_string_literal_char_slice();
  test_backend_ir_validator_accepts_lowered_string_literal_char_slice();
  test_backend_ir_printer_renders_structured_string_literal_char_slice_without_type_text();
  test_backend_ir_validator_accepts_structured_string_literal_char_slice_without_type_text();
  test_backend_ir_printer_renders_lowered_global_int_pointer_roundtrip_slice();
  test_backend_ir_validator_accepts_lowered_global_int_pointer_roundtrip_slice();
  test_backend_ir_printer_renders_lowered_global_char_pointer_diff_slice();
  test_backend_ir_validator_accepts_lowered_global_char_pointer_diff_slice();
  test_backend_ir_printer_renders_lowered_global_int_pointer_diff_slice();
  test_backend_ir_validator_accepts_lowered_global_int_pointer_diff_slice();
  test_backend_ir_printer_renders_structured_global_int_pointer_diff_slice_without_type_text();
  test_backend_ir_validator_accepts_structured_global_int_pointer_diff_slice_without_type_text();
  test_backend_ir_printer_renders_lowered_extern_global_array_load_slice();
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
