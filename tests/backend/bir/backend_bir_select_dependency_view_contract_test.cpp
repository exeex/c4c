#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/bir_select_dependency_view.hpp"

#include <iostream>
#include <type_traits>

namespace {
namespace bir = c4c::backend::bir;

int fail(const char* message) {
  std::cerr << message << '\n';
  return 1;
}

bir::BinaryInst immediate_binary(const char* name) {
  return bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I64, name),
      .operand_type = bir::TypeKind::I64,
      .lhs = bir::Value::immediate_i64(1),
      .rhs = bir::Value::immediate_i64(2),
  };
}

bir::BirSelectDependencyResult query(bir::Block& block,
                                     const bir::Value& root,
                                     std::size_t before) {
  return bir::find_bir_select_dependency({
      .block = &block,
      .root_value = &root,
      .block_label = block.label,
      .before_instruction_index = before,
  });
}

int direct_global_dependency_has_stable_identity() {
  bir::Block block;
  block.label = "entry";
  block.insts.emplace_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I64, "%global"),
      .global_name = "source",
  });
  block.insts.emplace_back(bir::CastInst{
      .opcode = bir::CastOpcode::Bitcast,
      .result = bir::Value::named(bir::TypeKind::I64, "%cast"),
      .operand = bir::Value::named(bir::TypeKind::I64, "%global"),
  });
  block.insts.emplace_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I64, "%root"),
      .compare_type = bir::TypeKind::I64,
      .lhs = bir::Value::immediate_i64(0),
      .rhs = bir::Value::immediate_i64(0),
      .true_value = bir::Value::named(bir::TypeKind::I64, "%cast"),
      .false_value = bir::Value::immediate_i64(0),
  });
  const auto* root = &std::get<bir::SelectInst>(block.insts[2]).result;
  const auto* load = &std::get<bir::LoadGlobalInst>(block.insts[0]);
  const auto result = query(block, *root, block.insts.size());
  if (result.status != bir::BirSelectDependencyStatus::CompleteDirectGlobal ||
      !result.complete() || result.block != &block || result.block_label != "entry" ||
      result.root_value != root || result.root_instruction_index != 2U ||
      result.dependency_load != load ||
      result.dependency_value != &load->result ||
      result.dependency_instruction_index != 0U ||
      result.before_instruction_index != block.insts.size()) {
    return fail("direct-global dependency lost stable root/dependency identity");
  }
  return 0;
}

int complete_no_dependency_is_explicit() {
  bir::Block block;
  block.label = "entry";
  block.insts.emplace_back(immediate_binary("%root"));
  const auto& root = std::get<bir::BinaryInst>(block.insts[0]).result;
  const auto result = query(block, root, 1U);
  if (result.status != bir::BirSelectDependencyStatus::CompleteNoDependency ||
      !result.complete() || result.root_value != &root ||
      result.dependency_load != nullptr) {
    return fail("complete no-dependency result was not explicit");
  }
  return 0;
}

int negative_statuses_and_before_index_fail_closed() {
  bir::Block block;
  block.label = "entry";
  block.insts.emplace_back(immediate_binary("%duplicate"));
  block.insts.emplace_back(immediate_binary("%duplicate"));
  block.insts.emplace_back(bir::CastInst{
      .opcode = bir::CastOpcode::Bitcast,
      .result = bir::Value::named(bir::TypeKind::I64, "%incomplete"),
      .operand = bir::Value::named(bir::TypeKind::I64, ""),
  });
  const auto& duplicate = std::get<bir::BinaryInst>(block.insts[1]).result;
  const auto& incomplete = std::get<bir::CastInst>(block.insts[2]).result;
  const auto ambiguous = query(block, duplicate, block.insts.size());
  const auto incomplete_result = query(block, incomplete, block.insts.size());
  const auto before_root = query(block, incomplete, 2U);
  auto mismatched_request = bir::BirSelectDependencyRequest{
      .block = &block,
      .root_value = &incomplete,
      .block_label = "other",
      .before_instruction_index = block.insts.size(),
  };
  const auto mismatched = bir::find_bir_select_dependency(mismatched_request);
  const auto unavailable = bir::find_bir_select_dependency({});
  if (ambiguous.status != bir::BirSelectDependencyStatus::Ambiguous ||
      incomplete_result.status != bir::BirSelectDependencyStatus::Incomplete ||
      before_root.status != bir::BirSelectDependencyStatus::Unavailable ||
      mismatched.status != bir::BirSelectDependencyStatus::Mismatched ||
      unavailable.status != bir::BirSelectDependencyStatus::Unavailable) {
    return fail("negative statuses or before-index behavior did not fail closed");
  }
  return 0;
}
}  // namespace

int main() {
  static_assert(std::is_same_v<decltype(bir::BirSelectDependencyResult::root_value),
                               const bir::Value*>);
  if (const int status = direct_global_dependency_has_stable_identity(); status) {
    return status;
  }
  if (const int status = complete_no_dependency_is_explicit(); status) {
    return status;
  }
  return negative_statuses_and_before_index_fail_closed();
}
