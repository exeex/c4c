#include "src/codegen/lir/ir.hpp"

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
      .index_type = lir::LirTypeRef::integer(64),
      .index = lir::LirOperand::ssa("%index", lir::LirValueId{4}),
      .native_vector_authority = authority(
          owner, lir::LirValueId{6}, lir::LirValueId{1}, std::nullopt, std::nullopt,
          lir::LirNativeVectorIndex{
              lir::LirOperand::ssa("%index", lir::LirValueId{4}), lir::LirTypeRef::integer(64)}),
  });
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

void test_native_vector_authority_verifier_boundary() {
  lir::verify_module(vector_authority_module());

  auto structured_poison_second = vector_authority_module();
  auto& poison_shuffle = std::get<lir::LirShuffleVectorOp>(
      structured_poison_second.functions[0].blocks[0].insts[2]);
  poison_shuffle.vec2 = lir::LirOperand::special_token(lir::LirSpecialToken::Poison);
  poison_shuffle.native_vector_authority->second_vector_use.reset();
  lir::verify_module(structured_poison_second);

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

  auto unknown_use = vector_authority_module();
  std::get<lir::LirShuffleVectorOp>(unknown_use.functions[0].blocks[0].insts[2])
      .native_vector_authority->second_vector_use = lir::LirValueId{999};
  expect_rejected(std::move(unknown_use), "carrier must reject an unknown vector use ID");

  auto incoherent_shape = vector_authority_module();
  std::get<lir::LirInsertElementOp>(incoherent_shape.functions[0].blocks[0].insts[0])
      .native_vector_authority->result_shape.lane_count = 5;
  expect_rejected(std::move(incoherent_shape), "carrier must reject an incoherent vector display mirror");

  auto incoherent_element_shape = vector_authority_module();
  std::get<lir::LirExtractElementOp>(incoherent_element_shape.functions[0].blocks[0].insts[1])
      .native_vector_authority->first_vector_shape->element_type = lir::LirTypeRef::integer(64);
  expect_rejected(std::move(incoherent_element_shape),
                  "carrier must reject an incoherent vector element shape");

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

}  // namespace

int main() {
  test_native_vector_authority_verifier_boundary();
  return 0;
}
