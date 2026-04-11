#include "hir/hir_ir.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool cond, const std::string& msg) {
  if (!cond) fail(msg);
}

void test_dense_id_map_tracks_assigned_dense_ids() {
  c4c::hir::DenseIdMap<c4c::hir::LocalId, std::string> names;

  names.insert(c4c::hir::LocalId{0}, "lhs");
  names.insert(c4c::hir::LocalId{3}, "rhs");

  expect_true(names.contains(c4c::hir::LocalId{0}),
              "DenseIdMap should report assigned IDs as present");
  expect_true(!names.contains(c4c::hir::LocalId{2}),
              "DenseIdMap should keep gaps unassigned until explicitly inserted");
  expect_true(names.at(c4c::hir::LocalId{3}) == "rhs",
              "DenseIdMap should preserve values at typed-ID indexes");
  expect_true(names.size() == 4,
              "DenseIdMap should grow storage to cover the highest dense ID");
}

void test_optional_dense_id_map_supports_sparse_occupancy() {
  c4c::hir::OptionalDenseIdMap<c4c::hir::ExprId, int> counts;

  expect_true(counts.find(c4c::hir::ExprId{1}) == nullptr,
              "OptionalDenseIdMap should report missing IDs through find()");

  int& first = counts.get_or_create(c4c::hir::ExprId{1}, [] { return 7; });
  int& second = counts.get_or_create(c4c::hir::ExprId{1}, [] { return 99; });
  expect_true(&first == &second && second == 7,
              "OptionalDenseIdMap should not recreate an occupied slot");
  counts.insert(c4c::hir::ExprId{4}, 11);

  expect_true(*counts.find(c4c::hir::ExprId{1}) == 7,
              "OptionalDenseIdMap should preserve earlier values after later growth");
  expect_true(counts.contains(c4c::hir::ExprId{4}),
              "OptionalDenseIdMap should track explicitly inserted sparse IDs");
  expect_true(*counts.find(c4c::hir::ExprId{4}) == 11,
              "OptionalDenseIdMap should return inserted sparse values");
  expect_true(counts.size() == 5,
              "OptionalDenseIdMap should size itself by dense typed-ID index");
}

void test_dense_id_map_supports_local_type_storage() {
  c4c::hir::DenseIdMap<c4c::hir::LocalId, c4c::TypeSpec> local_types;

  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  c4c::TypeSpec float_ptr_ts{};
  float_ptr_ts.base = c4c::TB_FLOAT;
  float_ptr_ts.ptr_level = 1;

  local_types.insert(c4c::hir::LocalId{0}, int_ts);
  local_types.insert(c4c::hir::LocalId{2}, float_ptr_ts);

  expect_true(local_types.contains(c4c::hir::LocalId{0}),
              "DenseIdMap should report stored local type entries by LocalId");
  expect_true(!local_types.contains(c4c::hir::LocalId{1}),
              "DenseIdMap should keep unassigned local type slots absent");
  expect_true(local_types.at(c4c::hir::LocalId{2}).base == c4c::TB_FLOAT &&
                  local_types.at(c4c::hir::LocalId{2}).ptr_level == 1,
              "DenseIdMap should preserve TypeSpec values at dense local slots");
}

void test_optional_dense_id_map_supports_function_id_counters() {
  c4c::hir::OptionalDenseIdMap<c4c::hir::FunctionId, int> expand_counts;

  expect_true(!expand_counts.contains(c4c::hir::FunctionId{2}),
              "OptionalDenseIdMap should leave untouched FunctionId slots absent");

  int& first = expand_counts.get_or_create(c4c::hir::FunctionId{2}, [] { return 0; });
  ++first;
  int& second = expand_counts.get_or_create(c4c::hir::FunctionId{2}, [] { return 99; });
  ++second;
  expand_counts.insert(c4c::hir::FunctionId{5}, 7);

  expect_true(first == 2 && &first == &second,
              "OptionalDenseIdMap should preserve FunctionId-backed counter slots");
  expect_true(*expand_counts.find(c4c::hir::FunctionId{5}) == 7,
              "OptionalDenseIdMap should support sparse FunctionId insertions");
  expect_true(expand_counts.size() == 6,
              "OptionalDenseIdMap should grow to the highest FunctionId index");
}

}  // namespace

int main() {
  test_dense_id_map_tracks_assigned_dense_ids();
  test_optional_dense_id_map_supports_sparse_occupancy();
  test_dense_id_map_supports_local_type_storage();
  test_optional_dense_id_map_supports_function_id_counters();

  std::cout << "PASS: frontend_hir_tests\n";
  return 0;
}
