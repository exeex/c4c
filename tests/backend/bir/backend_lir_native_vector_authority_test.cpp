#include "src/codegen/lir/ir.hpp"
#include "src/codegen/lir/hir_to_lir.hpp"
#include "src/codegen/lir/hir_to_lir/lowering.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace lir = c4c::codegen::lir;

namespace {

[[noreturn]] void fail(const std::string& message) {
  std::cerr << "FAIL: " << message << '\n';
  std::exit(1);
}

void expect_rejected(lir::LirModule module, const std::string& message) {
  try {
    lir::verify_module(module);
    fail(message);
  } catch (const lir::LirVerifyError&) {
  }
}

lir::LirNativeVectorShape vector_shape() {
  return {.lane_count = 4, .element_type = lir::LirTypeRef::integer(32)};
}

void attach_vector_store_ref(lir::LirModule& module, lir::LirNativeVectorAuthority& authority,
                             uint32_t lane_count = 4,
                             lir::LirTypeRef element_type = lir::LirTypeRef::integer(32)) {
  authority.vector_ref = module.register_vector({lane_count, std::move(element_type)});
}

lir::LirNativeVectorAuthority authority(c4c::LinkNameId owner, lir::LirValueId result,
                                         lir::LirValueId first,
                                         std::optional<lir::LirValueId> second = std::nullopt,
                                         std::optional<lir::LirValueId> element = std::nullopt,
                                         std::optional<lir::LirNativeVectorIndex> index = std::nullopt) {
  lir::LirNativeVectorAuthority value{
      .owner = owner,
      .result = result,
      .first_vector_use = first,
      .second_vector_use = second,
      .element_use = element,
      .result_shape = vector_shape(),
      .first_vector_shape = vector_shape(),
      .second_vector_shape = second ? std::optional(vector_shape()) : std::nullopt,
      .index = std::move(index),
  };
  if (second) {
    value.mask_lanes.assign(4, {
                                   .kind = lir::LirShuffleMaskLane::Kind::Selected,
                                   .selected_lane = 0,
                               });
  }
  return value;
}

lir::LirModule vector_authority_module() {
  lir::LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  const c4c::LinkNameId owner = module.link_names.intern("vector_authority_owner");

  lir::LirFunction function;
  function.name = "vector_authority";
  function.link_name_id = owner;
  function.signature_text = "declare void @vector_authority()";
  const auto vector_type = lir::LirTypeRef("<4 x i32>");
  for (const auto [display, id] : {std::pair{"%v1", 1u}, {"%v2", 2u},
                                   {"%element", 3u}, {"%index", 4u}}) {
    function.alloca_insts.push_back(lir::LirAllocaOp{
        .result = lir::LirOperand::ssa(display, lir::LirValueId{id}),
        .type_str = lir::LirTypeRef::integer(32),
    });
  }

  lir::LirBlock block;
  block.id = lir::LirBlockId{0};
  block.label = "entry";
  block.insts.push_back(lir::LirInsertElementOp{
      .result = lir::LirOperand::ssa("%insert", lir::LirValueId{5}),
      .vec_type = vector_type,
      .vec = lir::LirOperand::ssa("%v1", lir::LirValueId{1}),
      .elem_type = lir::LirTypeRef::integer(32),
      .elem = lir::LirOperand::ssa("%element", lir::LirValueId{3}),
      .index = lir::LirOperand::ssa("%index", lir::LirValueId{4}),
      .native_vector_authority = authority(
          owner, lir::LirValueId{5}, lir::LirValueId{1}, std::nullopt,
          lir::LirValueId{3}, lir::LirNativeVectorIndex{
              lir::LirOperand::ssa("%index", lir::LirValueId{4}), lir::LirTypeRef::integer(64)}),
  });
  block.insts.push_back(lir::LirExtractElementOp{
      .result = lir::LirOperand::ssa("%extract", lir::LirValueId{6}),
      .vec_type = vector_type,
      .vec = lir::LirOperand::ssa("%v1", lir::LirValueId{1}),
      .index_type = lir::LirTypeRef::integer(32),
      .index = lir::LirOperand::ssa("%index", lir::LirValueId{4}),
      .native_vector_authority = authority(
          owner, lir::LirValueId{6}, lir::LirValueId{1}, std::nullopt, std::nullopt,
          lir::LirNativeVectorIndex{
              lir::LirOperand::ssa("%index", lir::LirValueId{4}), lir::LirTypeRef::integer(32)}),
  });
  attach_vector_store_ref(module, *std::get<lir::LirExtractElementOp>(block.insts.back())
                                .native_vector_authority);
  block.insts.push_back(lir::LirShuffleVectorOp{
      .result = lir::LirOperand::ssa("%shuffle", lir::LirValueId{7}),
      .vec_type = vector_type,
      .vec1 = lir::LirOperand::ssa("%v1", lir::LirValueId{1}),
      .vec2 = lir::LirOperand::ssa("%v2", lir::LirValueId{2}),
      .mask_type = lir::LirTypeRef("<4 x i32>"),
      .mask = lir::LirOperand::special_token(lir::LirSpecialToken::ZeroInitializer),
      .native_vector_authority = authority(owner, lir::LirValueId{7}, lir::LirValueId{1},
                                             lir::LirValueId{2}),
  });
  block.terminator = lir::LirRet{std::nullopt, lir::LirTypeRef("void")};
  function.blocks.push_back(std::move(block));
  function.entry = lir::LirBlockId{0};
  module.functions.push_back(std::move(function));
  return module;
}

lir::LirModule selected_scalar_to_vector_splat_module() {
  auto module = vector_authority_module();
  auto& insert = std::get<lir::LirInsertElementOp>(module.functions[0].blocks[0].insts[0]);
  insert.index = lir::LirOperand::integer("selected index mirror", 0);
  insert.native_vector_authority->index = lir::LirNativeVectorIndex{
      lir::LirOperand::integer("independent zero authority", 0), lir::LirTypeRef::integer(64)};
  attach_vector_store_ref(module, *insert.native_vector_authority);
  insert.requires_native_vector_authority = true;
  auto& shuffle = std::get<lir::LirShuffleVectorOp>(module.functions[0].blocks[0].insts[2]);
  shuffle.vec1 = lir::LirOperand::ssa("%insert", lir::LirValueId{5});
  shuffle.vec2 = lir::LirOperand::special_token(lir::LirSpecialToken::Poison);
  shuffle.native_vector_authority->first_vector_use = lir::LirValueId{5};
  shuffle.native_vector_authority->second_vector_use.reset();
  shuffle.native_vector_authority->vector_ref = insert.native_vector_authority->vector_ref;
  shuffle.requires_native_vector_authority = true;
  std::swap(module.functions[0].blocks[0].insts[1], module.functions[0].blocks[0].insts[2]);
  return module;
}

lir::LirModule selected_scalar_to_vector_splat_immediate_module() {
  auto module = selected_scalar_to_vector_splat_module();
  auto& insert = std::get<lir::LirInsertElementOp>(module.functions[0].blocks[0].insts[0]);
  insert.elem = lir::LirOperand::integer("coerced immediate", 17);
  insert.native_vector_authority->element_use.reset();
  return module;
}

void test_native_vector_authority_verifier_boundary() {
  lir::verify_module(vector_authority_module());

  auto immediate_index = vector_authority_module();
  auto& immediate_extract = std::get<lir::LirExtractElementOp>(
      immediate_index.functions[0].blocks[0].insts[1]);
  immediate_extract.index = lir::LirOperand::integer("i32 2", 2);
  immediate_extract.native_vector_authority->index = lir::LirNativeVectorIndex{
      lir::LirOperand::integer("misleading immediate index display", 2),
      lir::LirTypeRef::integer(32)};
  lir::verify_module(std::move(immediate_index));

  auto missing_extract_authority = vector_authority_module();
  std::get<lir::LirExtractElementOp>(missing_extract_authority.functions[0].blocks[0].insts[1])
      .native_vector_authority.reset();
  expect_rejected(std::move(missing_extract_authority),
                  "direct vector extract must require native authority");

  lir::verify_module(selected_scalar_to_vector_splat_module());
  lir::verify_module(selected_scalar_to_vector_splat_immediate_module());

  auto missing_selected_insert_authority = selected_scalar_to_vector_splat_module();
  std::get<lir::LirInsertElementOp>(missing_selected_insert_authority.functions[0].blocks[0].insts[0])
      .native_vector_authority.reset();
  expect_rejected(std::move(missing_selected_insert_authority),
                  "selected splat insert must require native authority independently of shuffle");

  auto selected_wrong_result = selected_scalar_to_vector_splat_module();
  std::get<lir::LirInsertElementOp>(selected_wrong_result.functions[0].blocks[0].insts[0])
      .native_vector_authority->result = lir::LirValueId{6};
  expect_rejected(std::move(selected_wrong_result), "selected splat must reject a foreign result ID");

  auto selected_missing_vector = selected_scalar_to_vector_splat_module();
  std::get<lir::LirInsertElementOp>(selected_missing_vector.functions[0].blocks[0].insts[0])
      .native_vector_authority->first_vector_use.reset();
  expect_rejected(std::move(selected_missing_vector), "selected splat must reject a missing vector ID");

  auto selected_foreign_vector = selected_scalar_to_vector_splat_module();
  std::get<lir::LirInsertElementOp>(selected_foreign_vector.functions[0].blocks[0].insts[0])
      .native_vector_authority->first_vector_use = lir::LirValueId{2};
  expect_rejected(std::move(selected_foreign_vector), "selected splat must reject a foreign vector ID");

  auto selected_missing_element = selected_scalar_to_vector_splat_module();
  std::get<lir::LirInsertElementOp>(selected_missing_element.functions[0].blocks[0].insts[0])
      .native_vector_authority->element_use.reset();
  expect_rejected(std::move(selected_missing_element), "selected splat must reject a missing element ID");

  auto selected_foreign_element = selected_scalar_to_vector_splat_module();
  std::get<lir::LirInsertElementOp>(selected_foreign_element.functions[0].blocks[0].insts[0])
      .native_vector_authority->element_use = lir::LirValueId{4};
  expect_rejected(std::move(selected_foreign_element), "selected splat must reject a foreign element ID");

  auto selected_stale_shape_mirror = selected_scalar_to_vector_splat_module();
  std::get<lir::LirInsertElementOp>(selected_stale_shape_mirror.functions[0].blocks[0].insts[0])
      .native_vector_authority->first_vector_shape->lane_count = 5;
  lir::verify_module(selected_stale_shape_mirror);

  auto selected_missing_vector_ref = selected_scalar_to_vector_splat_module();
  std::get<lir::LirInsertElementOp>(selected_missing_vector_ref.functions[0].blocks[0].insts[0])
      .native_vector_authority->vector_ref.reset();
  expect_rejected(std::move(selected_missing_vector_ref),
                  "selected splat must reject a missing vector store ref");

  auto selected_foreign_vector_ref = selected_scalar_to_vector_splat_module();
  std::get<lir::LirInsertElementOp>(selected_foreign_vector_ref.functions[0].blocks[0].insts[0])
      .native_vector_authority->vector_ref = lir::LirVectorRef{999};
  expect_rejected(std::move(selected_foreign_vector_ref),
                  "selected splat must reject a non-vector-store ref");

  auto selected_zero_store_lanes = selected_scalar_to_vector_splat_module();
  auto& zero_store_insert = std::get<lir::LirInsertElementOp>(
      selected_zero_store_lanes.functions[0].blocks[0].insts[0]);
  selected_zero_store_lanes.vector_store[zero_store_insert.native_vector_authority->vector_ref->value]
      .lane_count = 0;
  expect_rejected(std::move(selected_zero_store_lanes),
                  "selected splat must reject malformed zero vector-store lanes");

  auto selected_store_lane_mismatch = selected_scalar_to_vector_splat_module();
  auto& lane_store_insert = std::get<lir::LirInsertElementOp>(
      selected_store_lane_mismatch.functions[0].blocks[0].insts[0]);
  selected_store_lane_mismatch
      .vector_store[lane_store_insert.native_vector_authority->vector_ref->value]
      .lane_count = 5;
  expect_rejected(std::move(selected_store_lane_mismatch),
                  "selected splat must reject vector-store lane mismatch");

  auto selected_store_element_mismatch = selected_scalar_to_vector_splat_module();
  auto& element_store_insert = std::get<lir::LirInsertElementOp>(
      selected_store_element_mismatch.functions[0].blocks[0].insts[0]);
  selected_store_element_mismatch
      .vector_store[element_store_insert.native_vector_authority->vector_ref->value]
      .element_type = lir::LirTypeRef::integer(64);
  expect_rejected(std::move(selected_store_element_mismatch),
                  "selected splat must reject vector-store element mismatch");

  auto selected_aggregate_without_store_fact = selected_scalar_to_vector_splat_module();
  const c4c::StructNameId aggregate_name =
      selected_aggregate_without_store_fact.struct_names.intern("%struct.VectorElement");
  auto& aggregate_insert = std::get<lir::LirInsertElementOp>(
      selected_aggregate_without_store_fact.functions[0].blocks[0].insts[0]);
  const lir::LirTypeRef aggregate_element =
      lir::LirTypeRef::struct_type("%struct.VectorElement", aggregate_name);
  aggregate_insert.elem_type = aggregate_element;
  aggregate_insert.vec_type = lir::LirTypeRef("<4 x %struct.VectorElement>");
  aggregate_insert.native_vector_authority->result_shape.element_type = aggregate_element;
  aggregate_insert.native_vector_authority->first_vector_shape->element_type =
      aggregate_element;
  selected_aggregate_without_store_fact
      .vector_store[aggregate_insert.native_vector_authority->vector_ref->value]
      .element_type =
      aggregate_element;
  expect_rejected(std::move(selected_aggregate_without_store_fact),
                  "selected splat must reject aggregate elements without aggregate store facts");

  auto selected_missing_index = selected_scalar_to_vector_splat_module();
  std::get<lir::LirInsertElementOp>(selected_missing_index.functions[0].blocks[0].insts[0])
      .native_vector_authority->index.reset();
  expect_rejected(std::move(selected_missing_index), "selected splat must reject missing index authority");

  auto selected_nonzero_index = selected_scalar_to_vector_splat_module();
  auto& nonzero_insert = std::get<lir::LirInsertElementOp>(selected_nonzero_index.functions[0].blocks[0].insts[0]);
  nonzero_insert.index = lir::LirOperand::integer("one", 1);
  nonzero_insert.native_vector_authority->index->value = lir::LirOperand::integer("one authority", 1);
  expect_rejected(std::move(selected_nonzero_index), "selected splat must reject a nonzero index");

  auto selected_non_i64_index = selected_scalar_to_vector_splat_module();
  std::get<lir::LirInsertElementOp>(selected_non_i64_index.functions[0].blocks[0].insts[0])
      .native_vector_authority->index->type = lir::LirTypeRef::integer(32);
  expect_rejected(std::move(selected_non_i64_index), "selected splat must reject a non-i64 index type");

  auto selected_wrong_element_type = selected_scalar_to_vector_splat_module();
  std::get<lir::LirInsertElementOp>(selected_wrong_element_type.functions[0].blocks[0].insts[0])
      .elem_type = lir::LirTypeRef::integer(64);
  expect_rejected(std::move(selected_wrong_element_type), "selected splat must reject mismatched element type");

  auto missing_required_authority = selected_scalar_to_vector_splat_module();
  std::get<lir::LirShuffleVectorOp>(missing_required_authority.functions[0].blocks[0].insts[1])
      .native_vector_authority.reset();
  expect_rejected(std::move(missing_required_authority),
                  "selected scalar-to-vector splat must require native authority");

  auto nonpreceding_insert = selected_scalar_to_vector_splat_module();
  auto& nonpreceding_shuffle = std::get<lir::LirShuffleVectorOp>(
      nonpreceding_insert.functions[0].blocks[0].insts[1]);
  nonpreceding_shuffle.vec1 = lir::LirOperand::ssa("%v1", lir::LirValueId{1});
  nonpreceding_shuffle.native_vector_authority->first_vector_use = lir::LirValueId{1};
  expect_rejected(std::move(nonpreceding_insert),
                  "selected scalar-to-vector splat must use the preceding native insert result");

  auto selected_shuffle_missing_vector_ref = selected_scalar_to_vector_splat_module();
  std::get<lir::LirShuffleVectorOp>(
      selected_shuffle_missing_vector_ref.functions[0].blocks[0].insts[1])
      .native_vector_authority->vector_ref.reset();
  expect_rejected(std::move(selected_shuffle_missing_vector_ref),
                  "selected splat shuffle must reject a missing vector store ref");

  auto selected_shuffle_foreign_vector_ref = selected_scalar_to_vector_splat_module();
  std::get<lir::LirShuffleVectorOp>(
      selected_shuffle_foreign_vector_ref.functions[0].blocks[0].insts[1])
      .native_vector_authority->vector_ref = lir::LirVectorRef{999};
  expect_rejected(std::move(selected_shuffle_foreign_vector_ref),
                  "selected splat shuffle must reject a non-vector-store ref");

  auto selected_shuffle_zero_store_lanes = selected_scalar_to_vector_splat_module();
  auto& zero_store_shuffle = std::get<lir::LirShuffleVectorOp>(
      selected_shuffle_zero_store_lanes.functions[0].blocks[0].insts[1]);
  selected_shuffle_zero_store_lanes
      .vector_store[zero_store_shuffle.native_vector_authority->vector_ref->value]
      .lane_count = 0;
  expect_rejected(std::move(selected_shuffle_zero_store_lanes),
                  "selected splat shuffle must reject malformed zero vector-store lanes");

  auto selected_shuffle_empty_store_element = selected_scalar_to_vector_splat_module();
  auto& empty_store_shuffle = std::get<lir::LirShuffleVectorOp>(
      selected_shuffle_empty_store_element.functions[0].blocks[0].insts[1]);
  selected_shuffle_empty_store_element
      .vector_store[empty_store_shuffle.native_vector_authority->vector_ref->value]
      .element_type = lir::LirTypeRef{};
  expect_rejected(std::move(selected_shuffle_empty_store_element),
                  "selected splat shuffle must reject malformed empty vector-store element type");

  auto selected_shuffle_store_lane_mismatch = selected_scalar_to_vector_splat_module();
  auto& lane_store_shuffle = std::get<lir::LirShuffleVectorOp>(
      selected_shuffle_store_lane_mismatch.functions[0].blocks[0].insts[1]);
  selected_shuffle_store_lane_mismatch
      .vector_store[lane_store_shuffle.native_vector_authority->vector_ref->value]
      .lane_count = 5;
  expect_rejected(std::move(selected_shuffle_store_lane_mismatch),
                  "selected splat shuffle must reject vector-store lane mismatch");

  auto selected_shuffle_store_element_mismatch = selected_scalar_to_vector_splat_module();
  auto& element_store_shuffle = std::get<lir::LirShuffleVectorOp>(
      selected_shuffle_store_element_mismatch.functions[0].blocks[0].insts[1]);
  selected_shuffle_store_element_mismatch
      .vector_store[element_store_shuffle.native_vector_authority->vector_ref->value]
      .element_type = lir::LirTypeRef::integer(64);
  expect_rejected(std::move(selected_shuffle_store_element_mismatch),
                  "selected splat shuffle must reject vector-store element mismatch");

  auto selected_shuffle_mask_lane_count_mismatch = selected_scalar_to_vector_splat_module();
  auto& mask_lane_count_shuffle = std::get<lir::LirShuffleVectorOp>(
      selected_shuffle_mask_lane_count_mismatch.functions[0].blocks[0].insts[1]);
  mask_lane_count_shuffle.native_vector_authority->vector_ref =
      selected_shuffle_mask_lane_count_mismatch.register_vector({5, lir::LirTypeRef::integer(32)});
  mask_lane_count_shuffle.vec_type = lir::LirTypeRef("<5 x i32>");
  mask_lane_count_shuffle.native_vector_authority->result_shape.lane_count = 5;
  mask_lane_count_shuffle.native_vector_authority->first_vector_shape->lane_count = 5;
  mask_lane_count_shuffle.mask_type = lir::LirTypeRef("<5 x i32>");
  expect_rejected(std::move(selected_shuffle_mask_lane_count_mismatch),
                  "selected splat shuffle mask lanes must mirror vector-store lane count");

  auto selected_shuffle_mask_type_mismatch = selected_scalar_to_vector_splat_module();
  auto& mask_type_shuffle = std::get<lir::LirShuffleVectorOp>(
      selected_shuffle_mask_type_mismatch.functions[0].blocks[0].insts[1]);
  mask_type_shuffle.native_vector_authority->vector_ref =
      selected_shuffle_mask_type_mismatch.register_vector({5, lir::LirTypeRef::integer(32)});
  mask_type_shuffle.vec_type = lir::LirTypeRef("<5 x i32>");
  mask_type_shuffle.native_vector_authority->result_shape.lane_count = 5;
  mask_type_shuffle.native_vector_authority->first_vector_shape->lane_count = 5;
  mask_type_shuffle.native_vector_authority->mask_lanes.push_back({
      .kind = lir::LirShuffleMaskLane::Kind::Selected,
      .selected_lane = 0,
  });
  expect_rejected(std::move(selected_shuffle_mask_type_mismatch),
                  "selected splat shuffle mask type must mirror vector-store lane count");

  auto selected_shuffle_non_poison_second = selected_scalar_to_vector_splat_module();
  auto& non_poison_shuffle = std::get<lir::LirShuffleVectorOp>(
      selected_shuffle_non_poison_second.functions[0].blocks[0].insts[1]);
  non_poison_shuffle.vec2 = lir::LirOperand::ssa("%v2", lir::LirValueId{2});
  non_poison_shuffle.native_vector_authority->second_vector_use = lir::LirValueId{2};
  expect_rejected(std::move(selected_shuffle_non_poison_second),
                  "selected splat shuffle must reject non-poison second vector");

  auto selected_shuffle_second_use_with_poison = selected_scalar_to_vector_splat_module();
  std::get<lir::LirShuffleVectorOp>(
      selected_shuffle_second_use_with_poison.functions[0].blocks[0].insts[1])
      .native_vector_authority->second_vector_use = lir::LirValueId{2};
  expect_rejected(std::move(selected_shuffle_second_use_with_poison),
                  "selected splat shuffle poison must reject second vector use evidence");

  auto selected_shuffle_missing_second_shape = selected_scalar_to_vector_splat_module();
  std::get<lir::LirShuffleVectorOp>(
      selected_shuffle_missing_second_shape.functions[0].blocks[0].insts[1])
      .native_vector_authority->second_vector_shape.reset();
  expect_rejected(std::move(selected_shuffle_missing_second_shape),
                  "selected splat shuffle must reject missing second vector shape");

  auto selected_shuffle_second_shape_lane_mismatch = selected_scalar_to_vector_splat_module();
  std::get<lir::LirShuffleVectorOp>(
      selected_shuffle_second_shape_lane_mismatch.functions[0].blocks[0].insts[1])
      .native_vector_authority->second_vector_shape->lane_count = 5;
  expect_rejected(std::move(selected_shuffle_second_shape_lane_mismatch),
                  "selected splat shuffle second shape lanes must mirror vector-store lane count");

  auto selected_shuffle_second_shape_element_mismatch = selected_scalar_to_vector_splat_module();
  std::get<lir::LirShuffleVectorOp>(
      selected_shuffle_second_shape_element_mismatch.functions[0].blocks[0].insts[1])
      .native_vector_authority->second_vector_shape->element_type = lir::LirTypeRef::integer(64);
  expect_rejected(std::move(selected_shuffle_second_shape_element_mismatch),
                  "selected splat shuffle second shape element must mirror vector-store element type");

  auto extract_missing_vector_ref = vector_authority_module();
  std::get<lir::LirExtractElementOp>(extract_missing_vector_ref.functions[0].blocks[0].insts[1])
      .native_vector_authority->vector_ref.reset();
  expect_rejected(std::move(extract_missing_vector_ref),
                  "direct vector extract must reject a missing vector store ref");

  auto extract_foreign_vector_ref = vector_authority_module();
  std::get<lir::LirExtractElementOp>(extract_foreign_vector_ref.functions[0].blocks[0].insts[1])
      .native_vector_authority->vector_ref = lir::LirVectorRef{999};
  expect_rejected(std::move(extract_foreign_vector_ref),
                  "direct vector extract must reject a non-vector-store ref");

  auto extract_zero_store_lanes = vector_authority_module();
  auto& zero_store_extract = std::get<lir::LirExtractElementOp>(
      extract_zero_store_lanes.functions[0].blocks[0].insts[1]);
  extract_zero_store_lanes.vector_store[zero_store_extract.native_vector_authority->vector_ref->value]
      .lane_count = 0;
  expect_rejected(std::move(extract_zero_store_lanes),
                  "direct vector extract must reject malformed zero vector-store lanes");

  auto extract_empty_store_element = vector_authority_module();
  auto& empty_store_extract = std::get<lir::LirExtractElementOp>(
      extract_empty_store_element.functions[0].blocks[0].insts[1]);
  extract_empty_store_element
      .vector_store[empty_store_extract.native_vector_authority->vector_ref->value]
      .element_type = lir::LirTypeRef{};
  expect_rejected(std::move(extract_empty_store_element),
                  "direct vector extract must reject malformed empty vector-store element type");

  auto extract_store_lane_mismatch = vector_authority_module();
  auto& lane_store_extract = std::get<lir::LirExtractElementOp>(
      extract_store_lane_mismatch.functions[0].blocks[0].insts[1]);
  extract_store_lane_mismatch
      .vector_store[lane_store_extract.native_vector_authority->vector_ref->value]
      .lane_count = 5;
  expect_rejected(std::move(extract_store_lane_mismatch),
                  "direct vector extract must reject vector-store lane mismatch");

  auto extract_store_element_mismatch = vector_authority_module();
  auto& element_store_extract = std::get<lir::LirExtractElementOp>(
      extract_store_element_mismatch.functions[0].blocks[0].insts[1]);
  extract_store_element_mismatch
      .vector_store[element_store_extract.native_vector_authority->vector_ref->value]
      .element_type = lir::LirTypeRef::integer(64);
  expect_rejected(std::move(extract_store_element_mismatch),
                  "direct vector extract must reject vector-store element mismatch");

  auto missing_owner = vector_authority_module();
  std::get<lir::LirInsertElementOp>(missing_owner.functions[0].blocks[0].insts[0])
      .native_vector_authority->owner = c4c::kInvalidLinkName;
  expect_rejected(std::move(missing_owner), "carrier must reject a missing owner");

  auto foreign_owner = vector_authority_module();
  const auto foreign = foreign_owner.link_names.intern("foreign_vector_owner");
  std::get<lir::LirInsertElementOp>(foreign_owner.functions[0].blocks[0].insts[0])
      .native_vector_authority->owner = foreign;
  expect_rejected(std::move(foreign_owner), "carrier must reject a foreign owner");

  auto missing_use = vector_authority_module();
  std::get<lir::LirInsertElementOp>(missing_use.functions[0].blocks[0].insts[0])
      .native_vector_authority->element_use.reset();
  expect_rejected(std::move(missing_use), "carrier must reject a missing element use ID");

  auto unknown_result = vector_authority_module();
  std::get<lir::LirExtractElementOp>(unknown_result.functions[0].blocks[0].insts[1])
      .native_vector_authority->result = lir::LirValueId{999};
  expect_rejected(std::move(unknown_result), "carrier must reject an undefined result ID");

  auto invalid_extract_result = vector_authority_module();
  std::get<lir::LirExtractElementOp>(invalid_extract_result.functions[0].blocks[0].insts[1])
      .native_vector_authority->result = lir::LirValueId::invalid();
  expect_rejected(std::move(invalid_extract_result),
                  "extract carrier must reject an invalid result ID");

  auto foreign_extract_result = vector_authority_module();
  std::get<lir::LirExtractElementOp>(foreign_extract_result.functions[0].blocks[0].insts[1])
      .native_vector_authority->result = lir::LirValueId{5};
  expect_rejected(std::move(foreign_extract_result),
                  "extract carrier must reject a result ID from another row");

  auto missing_extract_use = vector_authority_module();
  std::get<lir::LirExtractElementOp>(missing_extract_use.functions[0].blocks[0].insts[1])
      .native_vector_authority->first_vector_use.reset();
  expect_rejected(std::move(missing_extract_use),
                  "extract carrier must reject a missing vector use ID");

  auto foreign_extract_use = vector_authority_module();
  std::get<lir::LirExtractElementOp>(foreign_extract_use.functions[0].blocks[0].insts[1])
      .native_vector_authority->first_vector_use = lir::LirValueId{5};
  expect_rejected(std::move(foreign_extract_use),
                  "extract carrier must reject a foreign vector use ID");

  auto undefined_extract_use = vector_authority_module();
  std::get<lir::LirExtractElementOp>(undefined_extract_use.functions[0].blocks[0].insts[1])
      .native_vector_authority->first_vector_use = lir::LirValueId{999};
  expect_rejected(std::move(undefined_extract_use),
                  "extract carrier must reject an undefined vector use ID");

  auto unknown_use = vector_authority_module();
  std::get<lir::LirShuffleVectorOp>(unknown_use.functions[0].blocks[0].insts[2])
      .native_vector_authority->second_vector_use = lir::LirValueId{999};
  expect_rejected(std::move(unknown_use), "carrier must reject an unknown vector use ID");

  auto incoherent_shape = vector_authority_module();
  std::get<lir::LirInsertElementOp>(incoherent_shape.functions[0].blocks[0].insts[0])
      .native_vector_authority->result_shape.lane_count = 5;
  expect_rejected(std::move(incoherent_shape), "carrier must reject an incoherent vector display mirror");

  auto stale_extract_element_shape = vector_authority_module();
  std::get<lir::LirExtractElementOp>(stale_extract_element_shape.functions[0].blocks[0].insts[1])
      .native_vector_authority->first_vector_shape->element_type = lir::LirTypeRef::integer(64);
  lir::verify_module(stale_extract_element_shape);

  auto missing_extract_shape_mirror = vector_authority_module();
  std::get<lir::LirExtractElementOp>(missing_extract_shape_mirror.functions[0].blocks[0].insts[1])
      .native_vector_authority->first_vector_shape.reset();
  lir::verify_module(missing_extract_shape_mirror);

  auto stale_extract_zero_lanes = vector_authority_module();
  std::get<lir::LirExtractElementOp>(stale_extract_zero_lanes.functions[0].blocks[0].insts[1])
      .native_vector_authority->result_shape.lane_count = 0;
  lir::verify_module(stale_extract_zero_lanes);

  auto missing_second_shape = vector_authority_module();
  std::get<lir::LirShuffleVectorOp>(missing_second_shape.functions[0].blocks[0].insts[2])
      .native_vector_authority->second_vector_shape.reset();
  expect_rejected(std::move(missing_second_shape), "shuffle carrier must reject a missing second vector shape");

  auto poison_missing_second_shape = vector_authority_module();
  auto& poison_missing_shape_shuffle = std::get<lir::LirShuffleVectorOp>(
      poison_missing_second_shape.functions[0].blocks[0].insts[2]);
  poison_missing_shape_shuffle.vec2 = lir::LirOperand::special_token(lir::LirSpecialToken::Poison);
  poison_missing_shape_shuffle.native_vector_authority->second_vector_use.reset();
  poison_missing_shape_shuffle.native_vector_authority->second_vector_shape.reset();
  expect_rejected(std::move(poison_missing_second_shape),
                  "poison shuffle carrier must reject a missing second vector shape");

  auto poison_incoherent_second_shape = vector_authority_module();
  auto& poison_incoherent_shape_shuffle = std::get<lir::LirShuffleVectorOp>(
      poison_incoherent_second_shape.functions[0].blocks[0].insts[2]);
  poison_incoherent_shape_shuffle.vec2 = lir::LirOperand::special_token(lir::LirSpecialToken::Poison);
  poison_incoherent_shape_shuffle.native_vector_authority->second_vector_use.reset();
  poison_incoherent_shape_shuffle.native_vector_authority->second_vector_shape->lane_count = 5;
  expect_rejected(std::move(poison_incoherent_second_shape),
                  "poison shuffle carrier must reject an incoherent second vector shape");

  auto poison_with_second_use = vector_authority_module();
  auto& poison_with_use_shuffle = std::get<lir::LirShuffleVectorOp>(
      poison_with_second_use.functions[0].blocks[0].insts[2]);
  poison_with_use_shuffle.vec2 = lir::LirOperand::special_token(lir::LirSpecialToken::Poison);
  expect_rejected(std::move(poison_with_second_use),
                  "poison shuffle carrier must reject second vector-use evidence");

  auto wrong_index_value = vector_authority_module();
  std::get<lir::LirExtractElementOp>(wrong_index_value.functions[0].blocks[0].insts[1])
      .native_vector_authority->index->value = lir::LirOperand::ssa("%other", lir::LirValueId{999});
  expect_rejected(std::move(wrong_index_value), "carrier must reject an incoherent index value");

  auto missing_extract_index = vector_authority_module();
  std::get<lir::LirExtractElementOp>(missing_extract_index.functions[0].blocks[0].insts[1])
      .native_vector_authority->index.reset();
  expect_rejected(std::move(missing_extract_index),
                  "extract carrier must reject a missing native index");

  auto undefined_extract_index = vector_authority_module();
  auto& undefined_index_extract = std::get<lir::LirExtractElementOp>(
      undefined_extract_index.functions[0].blocks[0].insts[1]);
  undefined_index_extract.index = lir::LirOperand::ssa("%undefined-index", lir::LirValueId{999});
  undefined_index_extract.native_vector_authority->index->value =
      lir::LirOperand::ssa("%different-display", lir::LirValueId{999});
  expect_rejected(std::move(undefined_extract_index),
                  "extract carrier must reject an undefined structured index ID");

  auto non_i32_extract_index = vector_authority_module();
  auto& non_i32_extract = std::get<lir::LirExtractElementOp>(
      non_i32_extract_index.functions[0].blocks[0].insts[1]);
  non_i32_extract.index_type = lir::LirTypeRef::integer(64);
  non_i32_extract.native_vector_authority->index->type = lir::LirTypeRef::integer(64);
  expect_rejected(std::move(non_i32_extract_index),
                  "direct vector extract must reject a non-i32 index type");

  auto wrong_index_type = vector_authority_module();
  std::get<lir::LirInsertElementOp>(wrong_index_type.functions[0].blocks[0].insts[0])
      .native_vector_authority->index->type = lir::LirTypeRef::integer(32);
  expect_rejected(std::move(wrong_index_type), "carrier must reject an incoherent index type");

  auto missing_mask_lanes = vector_authority_module();
  std::get<lir::LirShuffleVectorOp>(missing_mask_lanes.functions[0].blocks[0].insts[2])
      .native_vector_authority->mask_lanes.clear();
  expect_rejected(std::move(missing_mask_lanes), "shuffle carrier must reject missing mask lanes");

  auto incoherent_mask_mirror = vector_authority_module();
  std::get<lir::LirShuffleVectorOp>(incoherent_mask_mirror.functions[0].blocks[0].insts[2])
      .mask_type = lir::LirTypeRef("<5 x i32>");
  expect_rejected(std::move(incoherent_mask_mirror), "shuffle carrier must reject an incoherent mask mirror");

  auto raw_mask_token = vector_authority_module();
  std::get<lir::LirShuffleVectorOp>(raw_mask_token.functions[0].blocks[0].insts[2])
      .mask = lir::LirOperand::raw("zeroinitializer");
  expect_rejected(std::move(raw_mask_token),
                  "zero-initializer shuffle mask must reject an unstructured mask token");

  auto inactive_mask_lane = vector_authority_module();
  std::get<lir::LirShuffleVectorOp>(inactive_mask_lane.functions[0].blocks[0].insts[2])
      .native_vector_authority->mask_lanes[0] = {
          .kind = lir::LirShuffleMaskLane::Kind::Inactive,
          .selected_lane = 0,
      };
  expect_rejected(std::move(inactive_mask_lane),
                  "zero-initializer shuffle mask must reject inactive native lanes");

  auto selected_mask_lane_payload = vector_authority_module();
  std::get<lir::LirShuffleVectorOp>(selected_mask_lane_payload.functions[0].blocks[0].insts[2])
      .native_vector_authority->mask_lanes[0].selected_lane = 1;
  expect_rejected(std::move(selected_mask_lane_payload),
                  "zero-initializer shuffle mask must reject nonzero native lane payloads");

  auto invalid_mask_lane_kind = vector_authority_module();
  std::get<lir::LirShuffleVectorOp>(invalid_mask_lane_kind.functions[0].blocks[0].insts[2])
      .native_vector_authority->mask_lanes[0].kind =
          static_cast<lir::LirShuffleMaskLane::Kind>(255);
  expect_rejected(std::move(invalid_mask_lane_kind),
                  "zero-initializer shuffle mask must reject invalid native lane kinds");
}

void test_module_owned_aggregate_ref_store() {
  c4c::hir::Module source_module;
  c4c::hir::Module foreign_module;
  lir::LirModule lir_module;
  lir_module.link_name_texts = std::make_shared<c4c::TextTable>();
  lir_module.struct_names.attach_text_table(lir_module.link_name_texts.get());
  const c4c::StructNameId name_id = lir_module.struct_names.intern("%struct.AggregateStore");
  const c4c::hir::HirAggregateRef source_ref = source_module.issue_aggregate_ref();

  const lir::LirAggregateRef first =
      lir_module.register_aggregate(source_module, source_ref, name_id, false);
  const lir::LirAggregateRef repeated =
      lir_module.register_aggregate(source_module, source_ref, name_id, false);
  if (!first.valid() || first.value != repeated.value || lir_module.aggregate_store.size() != 1 ||
      lir_module.find_aggregate_ref(source_module, source_ref).value != first.value ||
      lir_module.find_aggregate(first) == nullptr) {
    fail("same module-owned aggregate occurrence must intern exactly once");
  }

  try {
    lir_module.register_aggregate(
        source_module,
        c4c::hir::HirAggregateRef{source_module.aggregate_identity(),
                                   c4c::hir::HirAggregateId{}},
        name_id, false);
    fail("incomplete aggregate ref must be rejected");
  } catch (const std::runtime_error&) {
  }
  try {
    lir_module.register_aggregate(foreign_module, source_ref, name_id, false);
    fail("foreign aggregate ref must be rejected by its non-owning source module");
  } catch (const std::runtime_error&) {
  }
  if (lir_module.find_aggregate_ref(foreign_module, source_ref).valid()) {
    fail("foreign aggregate ref lookup must fail closed");
  }
}

void test_aggregate_definition_lowering_uses_module_owned_ref_store() {
  c4c::hir::Module hir_module;
  c4c::hir::HirStructDef definition;
  definition.tag = "ProducerAggregate";
  definition.size_bytes = 4;
  definition.fields.push_back({.name = "member", .elem_type = c4c::TypeSpec{.base = c4c::TB_INT},
                               .llvm_idx = 0, .size_bytes = 4});
  const auto [it, inserted] = hir_module.struct_defs.emplace(definition.tag, std::move(definition));
  if (!inserted) fail("aggregate producer test must install its HIR definition");

  const lir::LirModule lowered = lir::lower(hir_module);
  const std::optional<c4c::hir::HirAggregateRef> source_ref =
      hir_module.aggregate_ref_for_definition(it->second);
  if (!source_ref || !source_ref->complete() || lowered.aggregate_store.size() != 1 ||
      !(lowered.aggregate_store.front().hir_ref == *source_ref) ||
      lowered.find_aggregate_ref(hir_module, *source_ref).value != 0) {
    fail("aggregate definition lowering must intern its module-owned HIR ref exactly once");
  }

  lir::LirModule repeated_store;
  repeated_store.link_name_texts = hir_module.link_name_texts;
  repeated_store.struct_names.attach_text_table(repeated_store.link_name_texts.get());
  lir::build_type_decls(hir_module, &repeated_store);
  lir::build_type_decls(hir_module, &repeated_store);
  if (repeated_store.aggregate_store.size() != 1 ||
      !(repeated_store.aggregate_store.front().hir_ref == *source_ref)) {
    fail("repeated aggregate declaration lowering must preserve one canonical store entry");
  }

  c4c::hir::Module unregistered_hir_module;
  c4c::hir::HirStructDef unregistered_definition;
  unregistered_definition.tag = "UnregisteredAggregate";
  unregistered_hir_module.struct_defs.emplace(unregistered_definition.tag,
                                              std::move(unregistered_definition));
  lir::LirModule unregistered_store;
  unregistered_store.link_name_texts = unregistered_hir_module.link_name_texts;
  unregistered_store.struct_names.attach_text_table(unregistered_store.link_name_texts.get());
  try {
    lir::build_type_decls(unregistered_hir_module, &unregistered_store);
    fail("aggregate definition lowering must reject a missing HIR aggregate ref");
  } catch (const std::runtime_error&) {
  }
}

void test_aggregate_store_preserves_declaration_facts() {
  c4c::hir::Module hir_module;

  c4c::hir::HirStructDef child;
  child.tag = "NestedAggregateChild";
  child.size_bytes = 4;
  child.align_bytes = 4;
  child.fields.push_back({.name = "value", .elem_type = c4c::TypeSpec{.base = c4c::TB_INT},
                          .llvm_idx = 0, .size_bytes = 4, .align_bytes = 4});

  c4c::hir::HirStructDef parent;
  parent.tag = "AggregateFactsParent";
  parent.size_bytes = 12;
  parent.align_bytes = 4;
  parent.base_tags.push_back(child.tag);
  parent.fields.push_back({.name = "tail", .elem_type = c4c::TypeSpec{.base = c4c::TB_INT},
                           .llvm_idx = 1, .offset_bytes = 8, .size_bytes = 4,
                           .align_bytes = 4});

  c4c::hir::HirStructDef byte_union;
  byte_union.tag = "AggregateFactsUnion";
  byte_union.is_union = true;
  byte_union.size_bytes = 8;
  byte_union.fields.push_back({.name = "integer", .elem_type = c4c::TypeSpec{.base = c4c::TB_INT},
                               .llvm_idx = 0, .size_bytes = 4, .align_bytes = 4});

  c4c::hir::HirStructDef packed_storage;
  packed_storage.tag = "AggregateFactsPackedStorage";
  packed_storage.pack_align = 1;
  packed_storage.size_bytes = 4;

  hir_module.struct_defs.emplace(child.tag, std::move(child));
  hir_module.struct_defs.emplace(parent.tag, std::move(parent));
  hir_module.struct_defs.emplace(byte_union.tag, std::move(byte_union));
  hir_module.struct_defs.emplace(packed_storage.tag, std::move(packed_storage));
  hir_module.struct_def_order = {"NestedAggregateChild", "AggregateFactsParent",
                                 "AggregateFactsUnion", "AggregateFactsPackedStorage"};

  const lir::LirModule lowered = lir::lower(hir_module);
  const auto parent_ref = hir_module.aggregate_ref_for_definition(
      hir_module.struct_defs.at("AggregateFactsParent"));
  const auto union_ref = hir_module.aggregate_ref_for_definition(
      hir_module.struct_defs.at("AggregateFactsUnion"));
  const auto packed_ref = hir_module.aggregate_ref_for_definition(
      hir_module.struct_defs.at("AggregateFactsPackedStorage"));
  if (!parent_ref || !union_ref || !packed_ref) {
    fail("aggregate fact test requires registered definitions");
  }
  const lir::LirAggregateStoreEntry* parent_entry =
      lowered.find_aggregate(lowered.find_aggregate_ref(hir_module, *parent_ref));
  const lir::LirAggregateStoreEntry* union_entry =
      lowered.find_aggregate(lowered.find_aggregate_ref(hir_module, *union_ref));
  const lir::LirAggregateStoreEntry* packed_entry =
      lowered.find_aggregate(lowered.find_aggregate_ref(hir_module, *packed_ref));
  if (!parent_entry || parent_entry->layout_kind != lir::LirAggregateLayoutKind::Direct ||
      parent_entry->is_packed || parent_entry->is_opaque || parent_entry->fields.size() != 3 ||
      !parent_entry->fields[0].type.is_named_struct() ||
      parent_entry->fields[0].type.struct_name_id() == c4c::kInvalidStructName ||
      parent_entry->fields[1].type.render_llvm() != "[4 x i8]" ||
      parent_entry->fields[2].type.render_llvm() != "i32") {
    fail("aggregate store must retain ordered direct declaration fields and nested type facts");
  }
  if (!union_entry || !union_entry->is_union ||
      union_entry->layout_kind != lir::LirAggregateLayoutKind::Union ||
      union_entry->fields.size() != 1 || union_entry->fields[0].type.render_llvm() != "[8 x i8]") {
    fail("aggregate store must retain union byte-storage declaration facts");
  }
  if (!packed_entry || !packed_entry->is_packed || packed_entry->is_opaque ||
      packed_entry->layout_kind != lir::LirAggregateLayoutKind::ByteStorage ||
      packed_entry->fields.size() != 1 || packed_entry->fields[0].type.render_llvm() != "[4 x i8]") {
    fail("aggregate store must retain packed byte-storage declaration facts");
  }
}

}  // namespace

int main() {
  test_native_vector_authority_verifier_boundary();
  test_module_owned_aggregate_ref_store();
  test_aggregate_definition_lowering_uses_module_owned_ref_store();
  test_aggregate_store_preserves_declaration_facts();
  return 0;
}
