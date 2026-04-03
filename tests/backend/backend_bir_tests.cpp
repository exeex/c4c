#include "backend.hpp"
#include "../../src/backend/bir.hpp"
#include "../../src/backend/bir_printer.hpp"
#include "../../src/backend/bir_validate.hpp"
#include "../../src/backend/elf/mod.hpp"
#include "../../src/backend/lowering/bir_to_backend_ir.hpp"
#include "../../src/backend/lowering/lir_to_bir.hpp"
#include "../../src/backend/linker_common/mod.hpp"
#include "backend_test_fixtures.hpp"
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

void test_bir_printer_renders_minimal_add_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::Add, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(2), Value::immediate_i32(3)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.add i32 2, 3",
                  "BIR printer should render explicit add instructions in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let returns reference named BIR values");
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

void test_bir_lowering_accepts_tiny_return_add_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_return_add_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "BIR lowering should preserve the function boundary under explicit bir naming");
  expect_contains(rendered, "%t0 = bir.add i32 2, 3",
                  "BIR lowering should materialize the tiny add slice in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should return the named BIR result instead of falling back to legacy text");
}

void test_bir_validator_rejects_returning_undefined_named_value() {
  auto module = make_return_immediate_module();
  module.functions.front().blocks.front().terminator.value =
      c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%missing");
  std::string error;

  expect_true(!c4c::backend::bir::validate(module, &error),
              "BIR validator should reject returns that reference values the block never defines");
  expect_contains(error, "defined earlier in the block",
                  "BIR validator should explain named-value dominance failures in BIR terms");
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

void test_backend_default_path_remains_legacy_when_bir_pipeline_is_not_selected() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_return_add_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Riscv64});

  expect_contains(rendered, "define i32 @main()",
                  "default backend flow should keep returning legacy LLVM text when the BIR path is not selected");
  expect_not_contains(rendered, "bir.func",
                      "default backend flow should not silently switch into the BIR scaffold");
}

void test_backend_bir_pipeline_is_opt_in_through_backend_options() {
  c4c::backend::BackendOptions options;
  options.target = c4c::backend::Target::Riscv64;
  options.pipeline = c4c::backend::BackendPipeline::Bir;

  const auto rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{make_return_add_module()},
                                options);

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit backend options should select the BIR pipeline");
  expect_contains(rendered, "%t0 = bir.add i32 2, 3",
                  "explicit BIR selection should route the tiny add slice through the BIR lowering seam");
}

void test_backend_bir_pipeline_drives_x86_return_add_smoke_case_end_to_end() {
  c4c::backend::BackendOptions options;
  options.target = c4c::backend::Target::X86_64;
  options.pipeline = c4c::backend::BackendPipeline::Bir;

  const auto rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{make_return_add_module()},
                                options);

  expect_contains(rendered, ".globl main",
                  "BIR pipeline should still reach x86 backend emission for the tiny scaffold case");
  expect_contains(rendered, "mov eax, 5",
                  "BIR pipeline should lower the tiny add/return case to the expected x86 return-immediate asm");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc > 1) test_filter() = argv[1];

  RUN_TEST(test_bir_printer_renders_minimal_add_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_return_immediate_scaffold);
  RUN_TEST(test_bir_validator_accepts_minimal_return_immediate_scaffold);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_add_lir_slice);
  RUN_TEST(test_bir_validator_rejects_returning_undefined_named_value);
  RUN_TEST(test_bir_validator_rejects_return_type_mismatch);
  RUN_TEST(test_backend_default_path_remains_legacy_when_bir_pipeline_is_not_selected);
  RUN_TEST(test_backend_bir_pipeline_is_opt_in_through_backend_options);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_return_add_smoke_case_end_to_end);

  check_failures();
  return 0;
}
