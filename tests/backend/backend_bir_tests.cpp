#include "../../src/backend/bir.hpp"
#include "../../src/backend/bir_printer.hpp"
#include "../../src/backend/bir_validate.hpp"
#include "../../src/backend/linker_common/mod.hpp"
#include "backend_test_util.hpp"

#include <string>

namespace {

c4c::backend::bir::Module make_return_immediate_module() {
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

void test_bir_printer_renders_minimal_return_immediate_scaffold() {
  const auto rendered = c4c::backend::bir::print(make_return_immediate_module());

  expect_contains(rendered, "bir.func @tiny_ret() -> i32 {",
                  "BIR printer should render the explicit BIR function header");
  expect_contains(rendered, "entry:\n  bir.ret i32 7",
                  "BIR printer should render the minimal return-immediate block");
}

void test_bir_validator_accepts_minimal_return_immediate_scaffold() {
  std::string error;

  expect_true(c4c::backend::bir::validate(make_return_immediate_module(), &error),
              "BIR validator should accept the minimal single-block return-immediate scaffold");
  expect_true(error.empty(),
              "BIR validator should not report an error for a valid minimal scaffold");
}

void test_bir_validator_rejects_return_type_mismatch() {
  auto module = make_return_immediate_module();
  module.functions.front().blocks.front().terminator.value =
      c4c::backend::bir::Value::immediate_i64(7);
  std::string error;

  expect_true(!c4c::backend::bir::validate(module, &error),
              "BIR validator should reject return values whose type disagrees with the function signature");
  expect_contains(error, "must match the function return type",
                  "BIR validator should explain return type mismatches in BIR terms");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc > 1) test_filter() = argv[1];

  RUN_TEST(test_bir_printer_renders_minimal_return_immediate_scaffold);
  RUN_TEST(test_bir_validator_accepts_minimal_return_immediate_scaffold);
  RUN_TEST(test_bir_validator_rejects_return_type_mismatch);

  check_failures();
  return 0;
}
