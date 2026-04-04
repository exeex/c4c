#pragma once

#include "backend.hpp"
#include "../../src/backend/bir.hpp"
#include "../../src/codegen/lir/ir.hpp"
#include "../../src/backend/elf/mod.hpp"
#include "../../src/backend/linker_common/mod.hpp"
#include "backend_test_util.hpp"

namespace {

inline c4c::backend::bir::Module make_return_immediate_module() {
  using namespace c4c::backend::bir;

  Module module;
  Function function;
  function.name = "tiny_ret";
  function.return_type = TypeKind::I32;

  Block entry;
  entry.label = "entry";
  entry.terminator.value = Value::immediate_i32(7);

  function.blocks.push_back(entry);
  module.functions.push_back(function);
  return module;
}

inline c4c::backend::BackendOptions make_bir_pipeline_options(c4c::backend::Target target) {
  c4c::backend::BackendOptions options;
  options.target = target;
  options.pipeline = c4c::backend::BackendPipeline::Bir;
  return options;
}

inline c4c::backend::bir::Module make_unsupported_multi_function_bir_module() {
  using namespace c4c::backend::bir;

  Module module;

  Function first;
  first.name = "first";
  first.return_type = TypeKind::I32;
  Block first_entry;
  first_entry.label = "entry";
  first_entry.terminator.value = Value::immediate_i32(7);
  first.blocks.push_back(first_entry);

  Function second;
  second.name = "second";
  second.return_type = TypeKind::I32;
  Block second_entry;
  second_entry.label = "entry";
  second_entry.terminator.value = Value::immediate_i32(9);
  second.blocks.push_back(second_entry);

  module.functions.push_back(first);
  module.functions.push_back(second);
  return module;
}

}  // namespace

c4c::codegen::lir::LirModule make_bir_return_add_module();
c4c::codegen::lir::LirModule make_bir_return_sub_module();
c4c::codegen::lir::LirModule make_bir_return_add_sub_chain_module();
c4c::codegen::lir::LirModule make_bir_return_staged_constant_module();
c4c::codegen::lir::LirModule make_bir_return_mul_module();
c4c::codegen::lir::LirModule make_bir_return_and_module();
c4c::codegen::lir::LirModule make_bir_return_or_module();
c4c::codegen::lir::LirModule make_bir_return_xor_module();
c4c::codegen::lir::LirModule make_bir_return_shl_module();
c4c::codegen::lir::LirModule make_bir_return_lshr_module();
c4c::codegen::lir::LirModule make_bir_return_ashr_module();
c4c::codegen::lir::LirModule make_bir_return_sdiv_module();
c4c::codegen::lir::LirModule make_bir_return_udiv_module();
c4c::codegen::lir::LirModule make_bir_return_srem_module();
c4c::codegen::lir::LirModule make_bir_return_urem_module();
c4c::codegen::lir::LirModule make_bir_return_eq_module();
c4c::codegen::lir::LirModule make_bir_return_ne_module();
c4c::codegen::lir::LirModule make_bir_return_slt_module();
c4c::codegen::lir::LirModule make_bir_return_sle_module();
c4c::codegen::lir::LirModule make_bir_return_sgt_module();
c4c::codegen::lir::LirModule make_bir_return_sge_module();
c4c::codegen::lir::LirModule make_bir_return_ult_module();
c4c::codegen::lir::LirModule make_bir_return_ule_module();
c4c::codegen::lir::LirModule make_bir_return_ugt_module();
c4c::codegen::lir::LirModule make_bir_return_uge_module();
c4c::codegen::lir::LirModule make_bir_return_select_eq_module();
c4c::codegen::lir::LirModule make_bir_i8_return_add_module();
c4c::codegen::lir::LirModule make_bir_i8_return_sub_module();
c4c::codegen::lir::LirModule make_bir_i8_return_mul_module();
c4c::codegen::lir::LirModule make_bir_i8_return_and_module();
c4c::codegen::lir::LirModule make_bir_i8_return_or_module();
c4c::codegen::lir::LirModule make_bir_i8_return_xor_module();
c4c::codegen::lir::LirModule make_bir_i8_return_shl_module();
c4c::codegen::lir::LirModule make_bir_i8_return_lshr_module();
c4c::codegen::lir::LirModule make_bir_i8_return_ashr_module();
c4c::codegen::lir::LirModule make_bir_i8_return_sdiv_module();
c4c::codegen::lir::LirModule make_bir_i8_return_udiv_module();
c4c::codegen::lir::LirModule make_bir_i8_return_eq_module();
c4c::codegen::lir::LirModule make_bir_i8_return_ne_module();
c4c::codegen::lir::LirModule make_bir_i8_return_ult_module();
c4c::codegen::lir::LirModule make_bir_i8_return_ule_module();
c4c::codegen::lir::LirModule make_bir_i8_return_ugt_module();
c4c::codegen::lir::LirModule make_bir_i8_return_uge_module();
c4c::codegen::lir::LirModule make_bir_i8_return_slt_module();
c4c::codegen::lir::LirModule make_bir_i8_return_sle_module();
c4c::codegen::lir::LirModule make_bir_i8_return_sgt_module();
c4c::codegen::lir::LirModule make_bir_i8_return_sge_module();
c4c::codegen::lir::LirModule make_bir_i8_return_immediate_module();
c4c::codegen::lir::LirModule make_bir_single_param_select_eq_branch_module();
c4c::codegen::lir::LirModule make_bir_single_param_select_eq_phi_module();
c4c::codegen::lir::LirModule make_bir_two_param_select_eq_phi_module();
c4c::codegen::lir::LirModule make_bir_two_param_select_eq_predecessor_add_phi_module();
c4c::codegen::lir::LirModule make_bir_two_param_select_eq_split_predecessor_add_phi_post_join_add_module();
c4c::codegen::lir::LirModule
make_bir_two_param_select_eq_split_predecessor_add_phi_post_join_add_sub_module();
c4c::codegen::lir::LirModule
make_bir_two_param_select_eq_split_predecessor_add_phi_post_join_add_sub_add_module();
c4c::codegen::lir::LirModule make_bir_two_param_select_eq_split_predecessor_deeper_affine_phi_post_join_add_sub_module();
c4c::codegen::lir::LirModule make_bir_two_param_select_eq_split_predecessor_mixed_affine_phi_post_join_add_sub_module();
c4c::codegen::lir::LirModule make_bir_two_param_select_eq_split_predecessor_mixed_affine_phi_post_join_add_sub_add_module();
c4c::codegen::lir::LirModule make_bir_two_param_select_eq_split_predecessor_mixed_affine_phi_post_join_add_module();
c4c::codegen::lir::LirModule make_bir_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_module();
c4c::codegen::lir::LirModule make_bir_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_module();
c4c::codegen::lir::LirModule make_bir_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_add_module();
c4c::codegen::lir::LirModule make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_module();
c4c::codegen::lir::LirModule make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_module();
c4c::codegen::lir::LirModule make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_add_module();
c4c::codegen::lir::LirModule make_bir_two_param_select_eq_split_predecessor_deeper_affine_phi_post_join_add_module();
c4c::codegen::lir::LirModule make_bir_two_param_select_eq_split_predecessor_deeper_affine_phi_post_join_add_sub_add_module();
c4c::codegen::lir::LirModule make_bir_mixed_predecessor_add_phi_post_join_add_module();
c4c::codegen::lir::LirModule make_bir_i8_return_add_sub_chain_module();
c4c::codegen::lir::LirModule make_bir_i64_return_add_sub_chain_module();
c4c::codegen::lir::LirModule make_bir_single_param_add_sub_chain_module();
c4c::codegen::lir::LirModule make_bir_i8_two_param_add_module();
c4c::codegen::lir::LirModule make_bir_two_param_add_module();
c4c::codegen::lir::LirModule make_bir_two_param_add_sub_chain_module();
c4c::codegen::lir::LirModule make_bir_two_param_staged_affine_module();

void run_backend_bir_lowering_tests();
void run_backend_bir_pipeline_tests();
