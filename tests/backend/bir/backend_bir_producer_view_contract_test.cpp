#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/bir_producer_view.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>

namespace {

namespace bir = c4c::backend::bir;

int fail(const char* message) {
  std::cerr << message << '\n';
  return 1;
}

bir::BinaryInst producer(const char* name) {
  return bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I64, name),
      .operand_type = bir::TypeKind::I64,
      .lhs = bir::Value::immediate_i64(1),
      .rhs = bir::Value::immediate_i64(2),
  };
}

int statuses_are_explicit_and_fail_closed() {
  bir::Block block;
  block.label = "entry";
  block.insts.emplace_back(producer("%unique"));
  block.insts.emplace_back(producer("%duplicate"));
  block.insts.emplace_back(producer("%duplicate"));
  const auto view = bir::make_bir_producer_view(block);

  const auto available = bir::find_same_block_producer(
      view, bir::Value::named(bir::TypeKind::I64, "%unique"), block.insts.size());
  if (!available || available.kind != bir::BirProducerKind::Binary ||
      available.instruction_index != 0U || available.produced_value == nullptr ||
      available.block_label != "entry" ||
      !available.scalar_materialization_available) {
    return fail("expected a narrow available producer result");
  }

  const auto unavailable = bir::find_same_block_producer(
      view, bir::Value::named(bir::TypeKind::I64, "%missing"), block.insts.size());
  const auto incomplete = bir::find_same_block_producer(
      view, bir::Value::named(bir::TypeKind::I64, ""), block.insts.size());
  const auto ambiguous = bir::find_same_block_producer(
      view, bir::Value::named(bir::TypeKind::I64, "%duplicate"), block.insts.size());
  const auto absent_view = bir::find_same_block_producer(
      bir::BirProducerView{}, bir::Value::named(bir::TypeKind::I64, "%unique"), 1U);
  if (unavailable.status != bir::BirViewStatus::Unavailable || unavailable ||
      incomplete.status != bir::BirViewStatus::Incomplete || incomplete ||
      ambiguous.status != bir::BirViewStatus::Ambiguous || ambiguous ||
      absent_view.status != bir::BirViewStatus::Unavailable || absent_view) {
    return fail("expected unavailable, incomplete, and ambiguous queries to fail closed");
  }
  return 0;
}

int named_header_has_no_route_vocabulary() {
  std::ifstream input(C4C_BIR_PRODUCER_VIEW_HEADER);
  const std::string source((std::istreambuf_iterator<char>(input)),
                           std::istreambuf_iterator<char>());
  for (const char* forbidden : {"Route1", "RouteIndex", "route_index"}) {
    if (source.find(forbidden) != std::string::npos) {
      return fail("named producer header exposed forbidden route vocabulary");
    }
  }
  return 0;
}

}  // namespace

int main() {
  static_assert(std::is_default_constructible_v<bir::BirProducerView>);
  static_assert(std::is_same_v<decltype(bir::BirProducerResult::produced_value),
                               const bir::Value*>);
  if (const int status = statuses_are_explicit_and_fail_closed(); status != 0) {
    return status;
  }
  return named_header_has_no_route_vocabulary();
}
