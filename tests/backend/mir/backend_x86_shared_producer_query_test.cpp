#include "src/backend/mir/query.hpp"
#include "src/backend/mir/x86/codegen/x86_codegen.hpp"

#include <iostream>
#include <optional>
#include <string>

namespace {

namespace bir = c4c::backend::bir;
namespace mir = c4c::backend::mir;
namespace x86 = c4c::backend::x86;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

bir::Value named(bir::TypeKind type, const char* name) {
  return bir::Value::named(type, name);
}

bool matches_load_local_dependency(const mir::DependencyTraversalRecord& record) {
  return record.kind == mir::SameBlockProducerKind::LoadLocal &&
         record.depth == 1U &&
         record.source_value != nullptr &&
         record.source_value->name == "%from_slot";
}

bir::Block make_query_block() {
  bir::Block block;
  block.label = "entry";
  block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = named(bir::TypeKind::I64, "%sum"),
      .operand_type = bir::TypeKind::I64,
      .lhs = bir::Value::immediate_i64(4),
      .rhs = bir::Value::immediate_i64(5),
  });
  block.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = named(bir::TypeKind::I64, "%wide"),
      .operand = named(bir::TypeKind::I64, "%sum"),
  });
  block.insts.push_back(bir::LoadLocalInst{
      .result = named(bir::TypeKind::I64, "%from_slot"),
      .slot_name = "local0",
      .byte_offset = 8,
      .align_bytes = 8,
  });
  block.insts.push_back(bir::LoadGlobalInst{
      .result = named(bir::TypeKind::I64, "%from_global"),
      .global_name = "global0",
      .byte_offset = 16,
      .align_bytes = 8,
  });
  block.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Eq,
      .result = named(bir::TypeKind::I64, "%choice"),
      .compare_type = bir::TypeKind::I64,
      .lhs = named(bir::TypeKind::I64, "%sum"),
      .rhs = bir::Value::immediate_i64(9),
      .true_value = named(bir::TypeKind::I64, "%wide"),
      .false_value = named(bir::TypeKind::I64, "%from_slot"),
  });
  block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = named(bir::TypeKind::I64, "%after"),
      .operand_type = bir::TypeKind::I64,
      .lhs = named(bir::TypeKind::I64, "%choice"),
      .rhs = bir::Value::immediate_i64(1),
  });
  return block;
}

int x86_facing_code_can_consume_shared_query_records() {
  const std::string x86_stack_operand =
      x86::render_prepared_stack_memory_operand(16, "QWORD");
  if (x86_stack_operand != "QWORD PTR [rsp + 16]") {
    return fail("expected x86-facing context to remain available");
  }

  const auto block = make_query_block();
  const auto sum = named(bir::TypeKind::I64, "%sum");
  const auto choice = named(bir::TypeKind::I64, "%choice");

  const auto sum_producer = mir::find_same_block_binary_producer(&block, sum);
  if (!sum_producer || sum_producer.instruction_index != 0U ||
      sum_producer.binary->opcode != bir::BinaryOpcode::Add) {
    return fail("expected shared query to find same-block binary producer");
  }

  const auto sum_constant = mir::evaluate_same_block_integer_constant(&block, sum);
  if (!sum_constant.has_value() || sum_constant->value != 9 ||
      sum_constant->depth != 0U) {
    return fail("expected shared query to evaluate same-block integer constant");
  }
  const auto immediate = bir::Value::immediate_i64(42);
  const auto immediate_identity = mir::same_block_value_identity(immediate);
  if (!immediate_identity.immediate_constant.has_value() ||
      *immediate_identity.immediate_constant != 42 ||
      immediate_identity.type != bir::TypeKind::I64) {
    return fail("expected shared query value identity to expose immediate constants only as value facts");
  }

  const auto select_producer =
      mir::find_same_block_select_producer(&block, choice, block.insts.size());
  if (!select_producer || select_producer.instruction_index != 4U ||
      select_producer.select->false_value.name != "%from_slot") {
    return fail("expected shared query to find same-block select producer");
  }

  const auto cast_record =
      mir::find_same_block_named_producer_record(&block, "%wide", block.insts.size());
  if (!cast_record || cast_record.instruction_index != 1U ||
      cast_record.kind != mir::SameBlockProducerKind::Cast) {
    return fail("expected shared query to classify a named cast producer");
  }
  if (cast_record.block_label != "entry" ||
      cast_record.before_instruction_index != block.insts.size() ||
      cast_record.produced_value.name != "%wide" ||
      cast_record.produced_value.type != bir::TypeKind::I64 ||
      cast_record.produced_value.value == nullptr ||
      cast_record.produced_value.immediate_constant.has_value() ||
      !cast_record.materialization_available) {
    return fail("expected shared query producer record to expose BIR identity facts");
  }

  const auto cast_index = mir::producer_instruction_index_record(&block, cast_record.inst);
  if (!cast_index.has_value() || cast_index->instruction_index != 1U ||
      cast_index->producer != cast_record.inst) {
    return fail("expected shared query to report producer instruction index");
  }

  const auto load_local_identity = mir::find_same_block_producer_identity(
      mir::SameBlockProducerIdentityRequest{
          .block = &block,
          .block_label = "entry",
          .value_name = "%from_slot",
          .value_type = bir::TypeKind::I64,
          .before_instruction_index = block.insts.size(),
      });
  if (!load_local_identity ||
      load_local_identity.kind != mir::SameBlockProducerKind::LoadLocal ||
      load_local_identity.instruction_index != 2U ||
      load_local_identity.produced_value.name != "%from_slot" ||
      !load_local_identity.materialization_available) {
    return fail("expected shared query identity to classify load-local producer facts");
  }

  const auto load_global_identity = mir::find_same_block_producer_identity(
      mir::SameBlockProducerIdentityRequest{
          .block = &block,
          .block_label = "entry",
          .value_name = "%from_global",
          .value_type = bir::TypeKind::I64,
          .before_instruction_index = block.insts.size(),
      });
  if (!load_global_identity ||
      load_global_identity.kind != mir::SameBlockProducerKind::LoadGlobal ||
      load_global_identity.instruction_index != 3U ||
      load_global_identity.produced_value.name != "%from_global" ||
      !load_global_identity.materialization_available) {
    return fail("expected shared query identity to classify load-global producer facts");
  }

  const auto future_product = mir::find_same_block_producer_identity(
      mir::SameBlockProducerIdentityRequest{
          .block = &block,
          .block_label = "entry",
          .value_name = "%after",
          .value_type = bir::TypeKind::I64,
          .before_instruction_index = block.insts.size() - 1U,
      });
  if (future_product) {
    return fail("expected shared query identity to fail closed for future producers");
  }
  const auto mismatched_type = mir::find_same_block_producer_identity(
      mir::SameBlockProducerIdentityRequest{
          .block = &block,
          .block_label = "entry",
          .value_name = "%sum",
          .value_type = bir::TypeKind::I32,
          .before_instruction_index = block.insts.size(),
      });
  if (mismatched_type) {
    return fail("expected shared query identity to fail closed for mismatched value type");
  }
  const auto missing_name = mir::find_same_block_producer_identity(
      mir::SameBlockProducerIdentityRequest{
          .block = &block,
          .block_label = "entry",
          .value_name = "%missing",
          .value_type = bir::TypeKind::I64,
          .before_instruction_index = block.insts.size(),
      });
  if (missing_name) {
    return fail("expected shared query identity to fail closed for missing names");
  }

  if (!mir::select_chain_contains_dependency(
          &block, choice, block.insts.size(), matches_load_local_dependency)) {
    return fail("expected shared query to traverse select-chain dependencies");
  }

  return 0;
}

}  // namespace

int main() {
  return x86_facing_code_can_consume_shared_query_records();
}
