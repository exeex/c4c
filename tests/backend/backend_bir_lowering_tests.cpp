#include "backend_bir_test_support.hpp"

#include "../../src/backend/bir_printer.hpp"
#include "../../src/backend/bir_validate.hpp"
#include "../../src/backend/lowering/lir_to_bir.hpp"

#include <string>

namespace {

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

void test_bir_printer_renders_minimal_sub_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::Sub, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(9), Value::immediate_i32(4)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.sub i32 9, 4",
                  "BIR printer should render explicit sub instructions in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let sub results flow into returns");
}

void test_bir_printer_renders_minimal_mul_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::Mul, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(6), Value::immediate_i32(7)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.mul i32 6, 7",
                  "BIR printer should render explicit mul instructions in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let mul results flow into returns");
}

void test_bir_printer_renders_minimal_and_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::And, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(14), Value::immediate_i32(11)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.and i32 14, 11",
                  "BIR printer should render explicit and instructions in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let and results flow into returns");
}

void test_bir_printer_renders_minimal_or_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::Or, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(12), Value::immediate_i32(3)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.or i32 12, 3",
                  "BIR printer should render explicit or instructions in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let or results flow into returns");
}

void test_bir_printer_renders_minimal_xor_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::Xor, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(12), Value::immediate_i32(10)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.xor i32 12, 10",
                  "BIR printer should render explicit xor instructions in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let xor results flow into returns");
}

void test_bir_printer_renders_minimal_shl_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::Shl, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(3), Value::immediate_i32(4)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.shl i32 3, 4",
                  "BIR printer should render explicit shl instructions in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let shl results flow into returns");
}

void test_bir_printer_renders_minimal_lshr_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::LShr, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(16), Value::immediate_i32(2)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.lshr i32 16, 2",
                  "BIR printer should render explicit lshr instructions in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let lshr results flow into returns");
}

void test_bir_printer_renders_minimal_ashr_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::AShr, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(-16), Value::immediate_i32(2)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.ashr i32 -16, 2",
                  "BIR printer should render explicit ashr instructions in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let ashr results flow into returns");
}

void test_bir_printer_renders_minimal_sdiv_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::SDiv, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(12), Value::immediate_i32(3)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.sdiv i32 12, 3",
                  "BIR printer should render explicit signed division instructions in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let signed-division results flow into returns");
}

void test_bir_printer_renders_minimal_udiv_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::UDiv, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(12), Value::immediate_i32(3)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.udiv i32 12, 3",
                  "BIR printer should render explicit unsigned division instructions in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let unsigned-division results flow into returns");
}

void test_bir_printer_renders_minimal_srem_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::SRem, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(14), Value::immediate_i32(5)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.srem i32 14, 5",
                  "BIR printer should render explicit signed remainder instructions in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let signed-remainder results flow into returns");
}

void test_bir_printer_renders_minimal_urem_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::URem, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(14), Value::immediate_i32(5)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.urem i32 14, 5",
                  "BIR printer should render explicit unsigned remainder instructions in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let unsigned-remainder results flow into returns");
}

void test_bir_printer_renders_minimal_eq_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::Eq, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(7), Value::immediate_i32(7)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.eq i32 7, 7",
                  "BIR printer should render explicit compare materialization in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let compare results flow into integer returns");
}

void test_bir_printer_renders_minimal_ne_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::Ne, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(7), Value::immediate_i32(3)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.ne i32 7, 3",
                  "BIR printer should render explicit inequality materialization in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let inequality results flow into integer returns");
}

void test_bir_printer_renders_minimal_slt_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::Slt, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(3), Value::immediate_i32(7)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.slt i32 3, 7",
                  "BIR printer should render signed relational compare materialization in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let signed relational compare results flow into integer returns");
}

void test_bir_printer_renders_minimal_sle_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::Sle, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(7), Value::immediate_i32(7)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.sle i32 7, 7",
                  "BIR printer should render signed less-than-or-equal compare materialization in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let signed less-than-or-equal compare results flow into integer returns");
}

void test_bir_printer_renders_minimal_sgt_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::Sgt, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(7), Value::immediate_i32(3)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.sgt i32 7, 3",
                  "BIR printer should render signed greater-than compare materialization in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let signed greater-than compare results flow into integer returns");
}

void test_bir_printer_renders_minimal_sge_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::Sge, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(7), Value::immediate_i32(7)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.sge i32 7, 7",
                  "BIR printer should render signed greater-than-or-equal compare materialization in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let signed greater-than-or-equal compare results flow into integer returns");
}

void test_bir_printer_renders_minimal_ult_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::Ult, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(3), Value::immediate_i32(7)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.ult i32 3, 7",
                  "BIR printer should render unsigned relational compare materialization in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let unsigned relational compare results flow into integer returns");
}

void test_bir_printer_renders_minimal_ule_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::Ule, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(7), Value::immediate_i32(7)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.ule i32 7, 7",
                  "BIR printer should render unsigned less-than-or-equal compare materialization in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let unsigned less-than-or-equal compare results flow into integer returns");
}

void test_bir_printer_renders_minimal_ugt_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::Ugt, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(7), Value::immediate_i32(3)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.ugt i32 7, 3",
                  "BIR printer should render unsigned greater-than compare materialization in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let unsigned greater-than compare results flow into integer returns");
}

void test_bir_printer_renders_minimal_uge_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      BinaryInst{BinaryOpcode::Uge, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(7), Value::immediate_i32(7)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.uge i32 7, 7",
                  "BIR printer should render unsigned greater-than-or-equal compare materialization in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let unsigned greater-than-or-equal compare results flow into integer returns");
}

void test_bir_printer_renders_minimal_select_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(
      SelectInst{BinaryOpcode::Eq, Value::named(TypeKind::I32, "%t0"),
                 Value::immediate_i32(7), Value::immediate_i32(7),
                 Value::immediate_i32(11), Value::immediate_i32(4)});
  block.terminator.value = Value::named(TypeKind::I32, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.select eq i32 7, 7, 11, 4",
                  "BIR printer should render bounded compare-fed select materialization in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR printer should let select results flow into integer returns");
}

void test_bir_printer_renders_minimal_return_immediate_scaffold() {
  const auto rendered = c4c::backend::bir::print(make_return_immediate_module());

  expect_contains(rendered, "bir.func @tiny_ret() -> i32 {",
                  "BIR printer should render the explicit BIR function header");
  expect_contains(rendered, "entry:\n  bir.ret i32 7",
                  "BIR printer should render the minimal return-immediate block");
}

void test_bir_printer_renders_i8_scaffold() {
  using namespace c4c::backend::bir;

  Module module;
  Function function;
  function.name = "tiny_char";
  function.return_type = TypeKind::I8;
  function.params.push_back(Param{TypeKind::I8, "%p.x"});

  Block entry;
  entry.label = "entry";
  entry.insts.push_back(
      BinaryInst{BinaryOpcode::Add, Value::named(TypeKind::I8, "%t0"),
                 Value::named(TypeKind::I8, "%p.x"), Value::immediate_i8(2)});
  entry.terminator.value = Value::named(TypeKind::I8, "%t0");
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "bir.func @tiny_char(i8 %p.x) -> i8 {",
                  "BIR printer should render i8 function signatures once the scaffold grows beyond i32-only slices");
  expect_contains(rendered, "%t0 = bir.add i8 %p.x, 2",
                  "BIR printer should preserve i8 arithmetic in BIR text form");
  expect_contains(rendered, "bir.ret i8 %t0",
                  "BIR printer should render i8 returns using the widened type surface");
}

void test_bir_printer_renders_i64_scaffold() {
  using namespace c4c::backend::bir;

  Module module;
  Function function;
  function.name = "wide_add";
  function.return_type = TypeKind::I64;
  function.params.push_back(Param{TypeKind::I64, "%p.x"});

  Block entry;
  entry.label = "entry";
  entry.insts.push_back(
      BinaryInst{BinaryOpcode::Add, Value::named(TypeKind::I64, "%t0"),
                 Value::named(TypeKind::I64, "%p.x"), Value::immediate_i64(2)});
  entry.terminator.value = Value::named(TypeKind::I64, "%t0");
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "bir.func @wide_add(i64 %p.x) -> i64 {",
                  "BIR printer should render i64 function signatures once the scaffold widens to word-sized arithmetic");
  expect_contains(rendered, "%t0 = bir.add i64 %p.x, 2",
                  "BIR printer should preserve i64 arithmetic in BIR text form");
  expect_contains(rendered, "bir.ret i64 %t0",
                  "BIR printer should render i64 returns using the widened type surface");
}

void test_bir_validator_accepts_minimal_return_immediate_scaffold() {
  std::string error;

  expect_true(c4c::backend::bir::validate(make_return_immediate_module(), &error),
              "BIR validator should accept the minimal single-block return-immediate scaffold");
  expect_true(error.empty(),
              "BIR validator should not report an error for a valid minimal scaffold");
}

void test_bir_validator_accepts_minimal_i8_scaffold() {
  std::string error;

  expect_true(c4c::backend::bir::validate(
                  c4c::backend::lower_to_bir(make_bir_i8_return_add_sub_chain_module()), &error),
              "BIR validator should accept the bounded i8 arithmetic scaffold once BIR grows a byte-wide scalar type");
  expect_true(error.empty(),
              "BIR validator should keep the widened i8 scaffold on the valid path");
}

void test_bir_validator_accepts_minimal_i64_scaffold() {
  std::string error;

  expect_true(c4c::backend::bir::validate(
                  c4c::backend::lower_to_bir(make_bir_i64_return_add_sub_chain_module()), &error),
              "BIR validator should accept the bounded i64 arithmetic scaffold once BIR grows a word-sized scalar type");
  expect_true(error.empty(),
              "BIR validator should keep the widened i64 scaffold on the valid path");
}

void test_bir_validator_accepts_minimal_select_scaffold() {
  std::string error;

  expect_true(c4c::backend::bir::validate(
                  c4c::backend::lower_to_bir(make_bir_return_select_eq_module()), &error),
              "BIR validator should accept the bounded compare-fed integer select scaffold");
  expect_true(error.empty(),
              "BIR validator should keep the bounded select scaffold on the valid path");
}

void test_bir_lowering_accepts_tiny_return_add_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_add_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "BIR lowering should preserve the function boundary under explicit bir naming");
  expect_contains(rendered, "%t0 = bir.add i32 2, 3",
                  "BIR lowering should materialize the tiny add slice in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should return the named BIR result instead of falling back to legacy text");
}

void test_bir_lowering_accepts_tiny_return_sub_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_sub_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t0 = bir.sub i32 3, 3",
                  "BIR lowering should materialize the tiny sub slice in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should return the named BIR sub result");
}

void test_bir_lowering_accepts_tiny_return_mul_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_mul_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t0 = bir.mul i32 6, 7",
                  "BIR lowering should materialize the tiny mul slice in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should return the named BIR mul result");
}

void test_bir_lowering_accepts_tiny_return_and_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_and_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t0 = bir.and i32 14, 11",
                  "BIR lowering should materialize the tiny and slice in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should return the named BIR and result");
}

void test_bir_lowering_accepts_tiny_return_or_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_or_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t0 = bir.or i32 12, 3",
                  "BIR lowering should materialize the tiny or slice in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should return the named BIR or result");
}

void test_bir_lowering_accepts_tiny_return_xor_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_xor_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t0 = bir.xor i32 12, 10",
                  "BIR lowering should materialize the tiny xor slice in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should return the named BIR xor result");
}

void test_bir_lowering_accepts_tiny_return_shl_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_shl_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t0 = bir.shl i32 3, 4",
                  "BIR lowering should materialize the tiny shl slice in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should return the named BIR shl result");
}

void test_bir_lowering_accepts_tiny_return_lshr_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_lshr_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t0 = bir.lshr i32 16, 2",
                  "BIR lowering should materialize the tiny lshr slice in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should return the named BIR lshr result");
}

void test_bir_lowering_accepts_tiny_return_ashr_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_ashr_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t0 = bir.ashr i32 -16, 2",
                  "BIR lowering should materialize the tiny ashr slice in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should return the named BIR ashr result");
}

void test_bir_lowering_accepts_tiny_return_sdiv_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_sdiv_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t0 = bir.sdiv i32 12, 3",
                  "BIR lowering should materialize the tiny signed-division slice in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should return the named BIR signed-division result");
}

void test_bir_lowering_accepts_tiny_return_udiv_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_udiv_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t0 = bir.udiv i32 12, 3",
                  "BIR lowering should materialize the tiny unsigned-division slice in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should return the named BIR unsigned-division result");
}

void test_bir_lowering_accepts_tiny_return_srem_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_srem_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t0 = bir.srem i32 14, 5",
                  "BIR lowering should materialize the tiny signed-remainder slice in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should return the named BIR signed-remainder result");
}

void test_bir_lowering_accepts_tiny_return_urem_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_urem_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t0 = bir.urem i32 14, 5",
                  "BIR lowering should materialize the tiny unsigned-remainder slice in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should return the named BIR unsigned-remainder result");
}

void test_bir_lowering_accepts_tiny_return_eq_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_eq_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t1 = bir.eq i32 7, 7",
                  "BIR lowering should fold the compare-plus-zext return pattern into a bounded BIR compare materialization");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "BIR lowering should return the widened compare result instead of leaving the legacy i1/zext pair intact");
}

void test_bir_lowering_accepts_tiny_return_ne_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_ne_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t1 = bir.ne i32 7, 3",
                  "BIR lowering should fold the inequality compare-plus-zext return pattern into a bounded BIR compare materialization");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "BIR lowering should return the widened inequality result instead of leaving the legacy i1/zext pair intact");
}

void test_bir_lowering_accepts_tiny_return_slt_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_slt_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t1 = bir.slt i32 3, 7",
                  "BIR lowering should fold a signed less-than compare-plus-zext return pattern into a bounded BIR compare materialization");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "BIR lowering should return the widened signed relational compare result instead of leaving the legacy i1/zext pair intact");
}

void test_bir_lowering_accepts_tiny_return_sle_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_sle_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t1 = bir.sle i32 7, 7",
                  "BIR lowering should fold a signed less-than-or-equal compare-plus-zext return pattern into a bounded BIR compare materialization");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "BIR lowering should return the widened signed less-than-or-equal compare result instead of leaving the legacy i1/zext pair intact");
}

void test_bir_lowering_accepts_tiny_return_sgt_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_sgt_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t1 = bir.sgt i32 7, 3",
                  "BIR lowering should fold a signed greater-than compare-plus-zext return pattern into a bounded BIR compare materialization");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "BIR lowering should return the widened signed greater-than compare result instead of leaving the legacy i1/zext pair intact");
}

void test_bir_lowering_accepts_tiny_return_sge_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_sge_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t1 = bir.sge i32 7, 7",
                  "BIR lowering should fold a signed greater-than-or-equal compare-plus-zext return pattern into a bounded BIR compare materialization");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "BIR lowering should return the widened signed greater-than-or-equal compare result instead of leaving the legacy i1/zext pair intact");
}

void test_bir_lowering_accepts_tiny_return_ult_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_ult_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t1 = bir.ult i32 3, 7",
                  "BIR lowering should fold an unsigned less-than compare-plus-zext return pattern into a bounded BIR compare materialization");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "BIR lowering should return the widened unsigned relational compare result instead of leaving the legacy i1/zext pair intact");
}

void test_bir_lowering_accepts_tiny_return_ule_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_ule_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t1 = bir.ule i32 7, 7",
                  "BIR lowering should fold an unsigned less-than-or-equal compare-plus-zext return pattern into a bounded BIR compare materialization");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "BIR lowering should return the widened unsigned less-than-or-equal compare result instead of leaving the legacy i1/zext pair intact");
}

void test_bir_lowering_accepts_tiny_return_ugt_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_ugt_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t1 = bir.ugt i32 7, 3",
                  "BIR lowering should fold an unsigned greater-than compare-plus-zext return pattern into a bounded BIR compare materialization");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "BIR lowering should return the widened unsigned greater-than compare result instead of leaving the legacy i1/zext pair intact");
}

void test_bir_lowering_accepts_tiny_return_uge_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_uge_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t1 = bir.uge i32 7, 7",
                  "BIR lowering should fold an unsigned greater-than-or-equal compare-plus-zext return pattern into a bounded BIR compare materialization");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "BIR lowering should return the widened unsigned greater-than-or-equal compare result instead of leaving the legacy i1/zext pair intact");
}

void test_bir_lowering_accepts_tiny_return_select_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_select_eq_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t1 = bir.select eq i32 7, 7, 11, 4",
                  "BIR lowering should fuse a compare-fed integer select into the bounded BIR select materialization shape");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "BIR lowering should return the select result instead of leaving the legacy compare-plus-select pair intact");
}

void test_bir_lowering_accepts_i8_return_eq() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_eq_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_eq_u() -> i8 {",
                  "BIR lowering should preserve the zero-parameter widened i8 compare-return signature");
  expect_contains(rendered, "%t1 = bir.eq i8 7, 7",
                  "BIR lowering should fold the widened i8 compare-return zext-plus-trunc shape into a direct BIR compare");
  expect_contains(rendered, "bir.ret i8 %t1",
                  "BIR lowering should return the widened i8 compare result instead of falling back to legacy LLVM IR");
}

void test_bir_lowering_accepts_i8_return_ne() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_ne_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_ne_u() -> i8 {",
                  "BIR lowering should preserve the zero-parameter widened i8 compare-return signature for not-equal");
  expect_contains(rendered, "%t1 = bir.ne i8 7, 3",
                  "BIR lowering should fold the widened i8 not-equal compare-return zext-plus-trunc shape into a direct BIR compare");
  expect_contains(rendered, "bir.ret i8 %t1",
                  "BIR lowering should return the widened i8 not-equal compare result instead of falling back to legacy LLVM IR");
}

void test_bir_lowering_accepts_i8_return_ult() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_ult_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_ult_u() -> i8 {",
                  "BIR lowering should preserve the zero-parameter widened i8 compare-return signature for unsigned less-than");
  expect_contains(rendered, "%t1 = bir.ult i8 3, 7",
                  "BIR lowering should fold the widened i8 unsigned less-than compare-return zext-plus-trunc shape into a direct BIR compare");
  expect_contains(rendered, "bir.ret i8 %t1",
                  "BIR lowering should return the widened i8 unsigned less-than compare result instead of falling back to legacy LLVM IR");
}

void test_bir_lowering_accepts_i8_return_ule() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_ule_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_ule_u() -> i8 {",
                  "BIR lowering should preserve the zero-parameter widened i8 compare-return signature for unsigned less-than-or-equal");
  expect_contains(rendered, "%t1 = bir.ule i8 7, 7",
                  "BIR lowering should fold the widened i8 unsigned less-than-or-equal compare-return zext-plus-trunc shape into a direct BIR compare");
  expect_contains(rendered, "bir.ret i8 %t1",
                  "BIR lowering should return the widened i8 unsigned less-than-or-equal compare result instead of falling back to legacy LLVM IR");
}

void test_bir_lowering_accepts_i8_return_ugt() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_ugt_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_ugt_u() -> i8 {",
                  "BIR lowering should preserve the zero-parameter widened i8 compare-return signature for unsigned greater-than");
  expect_contains(rendered, "%t1 = bir.ugt i8 7, 3",
                  "BIR lowering should fold the widened i8 unsigned greater-than compare-return zext-plus-trunc shape into a direct BIR compare");
  expect_contains(rendered, "bir.ret i8 %t1",
                  "BIR lowering should return the widened i8 unsigned greater-than compare result instead of falling back to legacy LLVM IR");
}

void test_bir_lowering_accepts_i8_return_uge() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_uge_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_uge_u() -> i8 {",
                  "BIR lowering should preserve the zero-parameter widened i8 compare-return signature for unsigned greater-than-or-equal");
  expect_contains(rendered, "%t1 = bir.uge i8 7, 7",
                  "BIR lowering should fold the widened i8 unsigned greater-than-or-equal compare-return zext-plus-trunc shape into a direct BIR compare");
  expect_contains(rendered, "bir.ret i8 %t1",
                  "BIR lowering should return the widened i8 unsigned greater-than-or-equal compare result instead of falling back to legacy LLVM IR");
}

void test_bir_lowering_accepts_i8_return_slt() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_slt_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_slt_u() -> i8 {",
                  "BIR lowering should preserve the zero-parameter widened i8 compare-return signature for signed less-than");
  expect_contains(rendered, "%t5 = bir.slt i8 3, 7",
                  "BIR lowering should fold the widened i8 signed less-than compare-return zext-plus-trunc shape into a direct BIR compare");
  expect_contains(rendered, "bir.ret i8 %t5",
                  "BIR lowering should return the widened i8 signed less-than compare result instead of falling back to legacy LLVM IR");
}

void test_bir_lowering_accepts_i8_return_immediate() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_immediate_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_const_u() -> i8 {",
                  "BIR lowering should preserve the zero-parameter widened i8 function boundary");
  expect_contains(rendered, "bir.ret i8 11",
                  "BIR lowering should accept the trunc-only widened i8 return-immediate slice");
}

void test_bir_lowering_accepts_single_param_select_branch_slice() {
  const auto lowered =
      c4c::backend::lower_to_bir(make_bir_single_param_select_eq_branch_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose(i32 %p.x) -> i32 {",
                  "BIR lowering should preserve the one-parameter signature while collapsing the bounded branch-return ternary");
  expect_contains(rendered, "%t.select = bir.select eq i32 %p.x, 7, 11, 4",
                  "BIR lowering should collapse the bounded compare-and-branch ternary into a single BIR select");
  expect_contains(rendered, "bir.ret i32 %t.select",
                  "BIR lowering should return the fused select result instead of preserving the legacy branch-return form");
}

void test_bir_lowering_accepts_single_param_select_phi_slice() {
  const auto lowered =
      c4c::backend::lower_to_bir(make_bir_single_param_select_eq_phi_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose(i32 %p.x) -> i32 {",
                  "BIR lowering should preserve the one-parameter signature while collapsing the bounded phi-join ternary");
  expect_contains(rendered, "%t8 = bir.select eq i32 %p.x, 7, 11, 4",
                  "BIR lowering should collapse the empty branch-only goto chain plus phi join into a single BIR select");
  expect_contains(rendered, "bir.ret i32 %t8",
                  "BIR lowering should return the fused select result instead of preserving the legacy phi join");
}

void test_bir_lowering_accepts_two_param_select_phi_slice() {
  const auto lowered =
      c4c::backend::lower_to_bir(make_bir_two_param_select_eq_phi_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose2(i32 %p.x, i32 %p.y) -> i32 {",
                  "BIR lowering should preserve the two-parameter ternary signature while collapsing the bounded phi-join shape");
  expect_contains(rendered, "%t8 = bir.select eq i32 %p.x, %p.y, %p.x, %p.y",
                  "BIR lowering should collapse the two-parameter phi-join ternary into a single BIR select");
  expect_contains(rendered, "bir.ret i32 %t8",
                  "BIR lowering should return the fused two-parameter select result instead of preserving the legacy phi join");
}

void test_bir_lowering_accepts_two_param_select_predecessor_add_phi_slice() {
  const auto lowered =
      c4c::backend::lower_to_bir(make_bir_two_param_select_eq_predecessor_add_phi_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose2_add(i32 %p.x, i32 %p.y) -> i32 {",
                  "BIR lowering should preserve the two-parameter signature while widening the bounded phi-join ternary to predecessor-local arithmetic");
  expect_contains(rendered, "%t3 = bir.add i32 %p.x, 5",
                  "BIR lowering should hoist the bounded then-arm predecessor arithmetic into the single BIR block");
  expect_contains(rendered, "%t4 = bir.add i32 %p.y, 9",
                  "BIR lowering should hoist the bounded else-arm predecessor arithmetic into the single BIR block");
  expect_contains(rendered, "%t5 = bir.select eq i32 %p.x, %p.y, %t3, %t4",
                  "BIR lowering should collapse the predecessor-compute phi join into a single BIR select over the computed arm values");
  expect_contains(rendered, "bir.ret i32 %t5",
                  "BIR lowering should return the fused predecessor-compute select result instead of preserving the legacy phi join");
}

void test_bir_lowering_accepts_two_param_select_split_predecessor_add_phi_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_select_eq_split_predecessor_add_phi_post_join_add_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose2_add_post(i32 %p.x, i32 %p.y) -> i32 {",
                  "BIR lowering should preserve the widened two-parameter split-predecessor ternary signature while collapsing the empty end blocks");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 5",
                  "BIR lowering should hoist the then-arm predecessor arithmetic even when the phi predecessor is a later empty end block");
  expect_contains(rendered, "%t9 = bir.add i32 %p.y, 9",
                  "BIR lowering should hoist the else-arm predecessor arithmetic even when the phi predecessor is a later empty end block");
  expect_contains(rendered, "%t10 = bir.select eq i32 %p.x, %p.y, %t8, %t9",
                  "BIR lowering should collapse the split-predecessor phi join into a single BIR select over the computed arm values");
  expect_contains(rendered, "%t11 = bir.add i32 %t10, 6",
                  "BIR lowering should preserve the bounded post-join add after the fused select for the split-predecessor form");
  expect_contains(rendered, "bir.ret i32 %t11",
                  "BIR lowering should return the split-predecessor join-local arithmetic result instead of falling back to legacy CFG form");
}

void test_bir_lowering_accepts_two_param_select_split_predecessor_add_phi_post_join_add_sub_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_select_eq_split_predecessor_add_phi_post_join_add_sub_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose2_add_post_chain(i32 %p.x, i32 %p.y) -> i32 {",
                  "BIR lowering should preserve the widened two-parameter split-predecessor ternary signature while extending the simple add-phi family to the nearby + 6 - 2 parity shape");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 5",
                  "BIR lowering should hoist the then-arm predecessor arithmetic for the simple split-predecessor family");
  expect_contains(rendered, "%t9 = bir.add i32 %p.y, 9",
                  "BIR lowering should hoist the else-arm predecessor arithmetic for the simple split-predecessor family");
  expect_contains(rendered, "%t10 = bir.select eq i32 %p.x, %p.y, %t8, %t9",
                  "BIR lowering should collapse the simple split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t11 = bir.add i32 %t10, 6",
                  "BIR lowering should preserve the first join-local arithmetic step after the simple split-predecessor select");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 2",
                  "BIR lowering should preserve the trailing join-local subtraction after the fused select for the simple split-predecessor family");
  expect_contains(rendered, "bir.ret i32 %t12",
                  "BIR lowering should return the widened simple split-predecessor join-local add/sub chain on the BIR path");
}

void test_bir_lowering_accepts_two_param_select_split_predecessor_add_phi_post_join_add_sub_add_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_select_eq_split_predecessor_add_phi_post_join_add_sub_add_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered,
                  "bir.func @choose2_add_post_chain_tail(i32 %p.x, i32 %p.y) -> i32 {",
                  "BIR lowering should preserve the simple split-predecessor ternary signature while extending the join-local arithmetic tail to the nearby + 6 - 2 + 9 parity shape");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 5",
                  "BIR lowering should hoist the then-arm predecessor arithmetic for the simple split-predecessor tail-extension slice");
  expect_contains(rendered, "%t9 = bir.add i32 %p.y, 9",
                  "BIR lowering should hoist the else-arm predecessor arithmetic for the simple split-predecessor tail-extension slice");
  expect_contains(rendered, "%t10 = bir.select eq i32 %p.x, %p.y, %t8, %t9",
                  "BIR lowering should collapse the simple split-predecessor phi join into the bounded BIR select surface before the longer join-local tail");
  expect_contains(rendered, "%t11 = bir.add i32 %t10, 6",
                  "BIR lowering should preserve the first join-local arithmetic step after the simple split-predecessor select");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 2",
                  "BIR lowering should preserve the middle join-local subtraction after the fused select for the simple split-predecessor family");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 9",
                  "BIR lowering should preserve the trailing join-local add after the fused select for the simple split-predecessor family");
  expect_contains(rendered, "bir.ret i32 %t13",
                  "BIR lowering should return the widened simple split-predecessor join-local add/sub/add chain on the BIR path");
}

void test_bir_lowering_accepts_two_param_select_split_predecessor_deeper_affine_phi_post_join_add_sub_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_select_eq_split_predecessor_deeper_affine_phi_post_join_add_sub_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered,
                  "bir.func @choose2_deeper_both_post_chain(i32 %p.x, i32 %p.y) -> i32 {",
                  "BIR lowering should preserve the symmetric deeper split-predecessor ternary signature while widening the join-local arithmetic tail beyond a single add");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "BIR lowering should keep the deeper split then-arm affine head in the fused BIR block");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "BIR lowering should keep the deeper split then-arm middle affine step in the fused BIR block");
  expect_contains(rendered, "%t10 = bir.add i32 %t9, 5",
                  "BIR lowering should keep the deeper split then-arm affine tail before the fused select");
  expect_contains(rendered, "%t11 = bir.add i32 %p.y, 11",
                  "BIR lowering should keep the deeper split else-arm affine head in the fused BIR block");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 4",
                  "BIR lowering should keep the deeper split else-arm middle affine step in the fused BIR block");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 7",
                  "BIR lowering should keep the deeper split else-arm affine tail before the fused select");
  expect_contains(rendered, "%t14 = bir.select eq i32 %p.x, %p.y, %t10, %t13",
                  "BIR lowering should collapse the symmetric deeper split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t15 = bir.add i32 %t14, 6",
                  "BIR lowering should preserve the first join-local arithmetic step after the symmetric deeper split-predecessor select");
  expect_contains(rendered, "%t16 = bir.sub i32 %t15, 2",
                  "BIR lowering should preserve the trailing join-local subtraction after the fused select");
  expect_contains(rendered, "bir.ret i32 %t16",
                  "BIR lowering should return the widened join-local add/sub chain on the BIR path");
}

void test_bir_lowering_accepts_two_param_select_split_predecessor_mixed_affine_phi_post_join_add_sub_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_select_eq_split_predecessor_mixed_affine_phi_post_join_add_sub_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose2_mixed_post_chain(i32 %p.x, i32 %p.y) -> i32 {",
                  "BIR lowering should preserve the split-predecessor mixed-affine ternary signature while widening the join-local arithmetic tail beyond a single add");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "BIR lowering should hoist the split then-arm affine head into the fused BIR block");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "BIR lowering should preserve the split then-arm affine tail before the fused select");
  expect_contains(rendered, "%t10 = bir.add i32 %p.y, 11",
                  "BIR lowering should hoist the split else-arm affine head into the fused BIR block");
  expect_contains(rendered, "%t11 = bir.sub i32 %t10, 4",
                  "BIR lowering should preserve the split else-arm affine tail before the fused select");
  expect_contains(rendered, "%t12 = bir.select eq i32 %p.x, %p.y, %t9, %t11",
                  "BIR lowering should collapse the mixed split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 6",
                  "BIR lowering should preserve the first join-local arithmetic step after the mixed split-predecessor select");
  expect_contains(rendered, "%t14 = bir.sub i32 %t13, 2",
                  "BIR lowering should preserve the trailing join-local subtraction after the fused select");
  expect_contains(rendered, "bir.ret i32 %t14",
                  "BIR lowering should return the widened mixed split-predecessor join-local add/sub chain on the BIR path");
}

void test_bir_lowering_accepts_two_param_select_split_predecessor_mixed_affine_phi_post_join_add_sub_add_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_select_eq_split_predecessor_mixed_affine_phi_post_join_add_sub_add_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered,
                  "bir.func @choose2_mixed_post_chain_tail(i32 %p.x, i32 %p.y) -> i32 {",
                  "BIR lowering should preserve the split-predecessor mixed-affine ternary signature while extending the join-local arithmetic tail to the nearby + 6 - 2 + 9 parity shape");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "BIR lowering should hoist the split then-arm affine head into the fused BIR block");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "BIR lowering should preserve the split then-arm affine tail before the fused select");
  expect_contains(rendered, "%t10 = bir.add i32 %p.y, 11",
                  "BIR lowering should hoist the split else-arm affine head into the fused BIR block");
  expect_contains(rendered, "%t11 = bir.sub i32 %t10, 4",
                  "BIR lowering should preserve the split else-arm affine tail before the fused select");
  expect_contains(rendered, "%t12 = bir.select eq i32 %p.x, %p.y, %t9, %t11",
                  "BIR lowering should collapse the mixed split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 6",
                  "BIR lowering should preserve the first join-local arithmetic step after the mixed split-predecessor select");
  expect_contains(rendered, "%t14 = bir.sub i32 %t13, 2",
                  "BIR lowering should preserve the middle join-local subtraction after the fused select");
  expect_contains(rendered, "%t15 = bir.add i32 %t14, 9",
                  "BIR lowering should preserve the trailing join-local add after the fused select");
  expect_contains(rendered, "bir.ret i32 %t15",
                  "BIR lowering should return the widened mixed split-predecessor join-local add/sub/add chain on the BIR path");
}

void test_bir_lowering_accepts_two_param_select_split_predecessor_mixed_affine_phi_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_select_eq_split_predecessor_mixed_affine_phi_post_join_add_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose2_mixed_post(i32 %p.x, i32 %p.y) -> i32 {",
                  "BIR lowering should preserve the split-predecessor ternary signature while widening each arm to a bounded add/sub chain");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "BIR lowering should hoist the split then-arm affine head into the fused BIR block");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "BIR lowering should preserve the split then-arm affine tail before the fused select");
  expect_contains(rendered, "%t10 = bir.add i32 %p.y, 11",
                  "BIR lowering should hoist the split else-arm affine head into the fused BIR block");
  expect_contains(rendered, "%t11 = bir.sub i32 %t10, 4",
                  "BIR lowering should preserve the split else-arm affine tail before the fused select");
  expect_contains(rendered, "%t12 = bir.select eq i32 %p.x, %p.y, %t9, %t11",
                  "BIR lowering should collapse the richer split-predecessor phi join into a single BIR select over the chain tails");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 6",
                  "BIR lowering should preserve the bounded post-join add after the richer split-predecessor select");
  expect_contains(rendered, "bir.ret i32 %t13",
                  "BIR lowering should return the richer split-predecessor join-local arithmetic result on the BIR path");
}

void test_bir_lowering_accepts_two_param_select_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose2_deeper_post(i32 %p.x, i32 %p.y) -> i32 {",
                  "BIR lowering should preserve the next richer split-predecessor ternary signature while widening the then arm beyond the prior mixed-affine slice");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "BIR lowering should keep the deeper split then-arm affine head in the fused BIR block");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "BIR lowering should keep the deeper split then-arm middle affine step in the fused BIR block");
  expect_contains(rendered, "%t10 = bir.add i32 %t9, 5",
                  "BIR lowering should keep the deeper split then-arm affine tail before the fused select");
  expect_contains(rendered, "%t11 = bir.add i32 %p.y, 11",
                  "BIR lowering should keep the mixed split else-arm affine head before the fused select");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 4",
                  "BIR lowering should keep the mixed split else-arm affine tail before the fused select");
  expect_contains(rendered, "%t13 = bir.select eq i32 %p.x, %p.y, %t10, %t12",
                  "BIR lowering should collapse the deeper-then split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t14 = bir.add i32 %t13, 6",
                  "BIR lowering should preserve the bounded post-join add after the deeper-then split-predecessor select");
  expect_contains(rendered, "bir.ret i32 %t14",
                  "BIR lowering should return the deeper-then split-predecessor join-local arithmetic result on the BIR path");
}

void test_bir_lowering_accepts_two_param_select_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered,
                  "bir.func @choose2_deeper_post_chain(i32 %p.x, i32 %p.y) -> i32 {",
                  "BIR lowering should preserve the asymmetric deeper-then split-predecessor ternary signature while widening the join-local arithmetic tail beyond the already-covered post-add form");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "BIR lowering should keep the deeper split then-arm affine head in the fused BIR block");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "BIR lowering should keep the deeper split then-arm middle affine step in the fused BIR block");
  expect_contains(rendered, "%t10 = bir.add i32 %t9, 5",
                  "BIR lowering should keep the deeper split then-arm affine tail before the fused select");
  expect_contains(rendered, "%t11 = bir.add i32 %p.y, 11",
                  "BIR lowering should keep the mixed split else-arm affine head before the fused select");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 4",
                  "BIR lowering should keep the mixed split else-arm affine tail before the fused select");
  expect_contains(rendered, "%t13 = bir.select eq i32 %p.x, %p.y, %t10, %t12",
                  "BIR lowering should collapse the asymmetric deeper-then split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t14 = bir.add i32 %t13, 6",
                  "BIR lowering should preserve the first join-local arithmetic step after the asymmetric deeper-then split-predecessor select");
  expect_contains(rendered, "%t15 = bir.sub i32 %t14, 2",
                  "BIR lowering should preserve the trailing join-local subtraction after the fused select");
  expect_contains(rendered, "bir.ret i32 %t15",
                  "BIR lowering should return the widened asymmetric deeper-then join-local add/sub chain on the BIR path");
}

void test_bir_lowering_accepts_two_param_select_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_add_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_add_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(
      rendered,
      "bir.func @choose2_deeper_post_chain_tail(i32 %p.x, i32 %p.y) -> i32 {",
      "BIR lowering should preserve the asymmetric deeper-then split-predecessor ternary signature while extending the join-local affine tail by one more bounded step");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "BIR lowering should keep the deeper split then-arm affine head in the fused BIR block");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "BIR lowering should keep the deeper split then-arm middle affine step in the fused BIR block");
  expect_contains(rendered, "%t10 = bir.add i32 %t9, 5",
                  "BIR lowering should keep the deeper split then-arm affine tail before the fused select");
  expect_contains(rendered, "%t11 = bir.add i32 %p.y, 11",
                  "BIR lowering should keep the mixed split else-arm affine head before the fused select");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 4",
                  "BIR lowering should keep the mixed split else-arm affine tail before the fused select");
  expect_contains(rendered, "%t13 = bir.select eq i32 %p.x, %p.y, %t10, %t12",
                  "BIR lowering should collapse the asymmetric deeper-then split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t14 = bir.add i32 %t13, 6",
                  "BIR lowering should preserve the first join-local arithmetic step after the asymmetric deeper-then split-predecessor select");
  expect_contains(rendered, "%t15 = bir.sub i32 %t14, 2",
                  "BIR lowering should preserve the second join-local arithmetic step after the fused select");
  expect_contains(rendered, "%t16 = bir.add i32 %t15, 9",
                  "BIR lowering should preserve the added third join-local arithmetic step after the fused select");
  expect_contains(rendered, "bir.ret i32 %t16",
                  "BIR lowering should return the extended asymmetric deeper-then join-local arithmetic tail on the BIR path");
}

void test_bir_lowering_accepts_two_param_select_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(
      rendered,
      "bir.func @choose2_mixed_then_deeper_post(i32 %p.x, i32 %p.y) -> i32 {",
      "BIR lowering should preserve the mirrored asymmetric deeper-else split-predecessor ternary signature on the BIR path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "BIR lowering should keep the mixed split then-arm affine head before the fused select");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "BIR lowering should keep the mixed split then-arm affine tail before the fused select");
  expect_contains(rendered, "%t10 = bir.add i32 %p.y, 11",
                  "BIR lowering should keep the deeper split else-arm affine head in the fused BIR block");
  expect_contains(rendered, "%t11 = bir.sub i32 %t10, 4",
                  "BIR lowering should keep the deeper split else-arm middle affine step in the fused BIR block");
  expect_contains(rendered, "%t12 = bir.add i32 %t11, 7",
                  "BIR lowering should keep the deeper split else-arm affine tail before the fused select");
  expect_contains(rendered, "%t13 = bir.select eq i32 %p.x, %p.y, %t9, %t12",
                  "BIR lowering should collapse the mirrored asymmetric split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t14 = bir.add i32 %t13, 6",
                  "BIR lowering should preserve the bounded post-join add after the mirrored asymmetric split-predecessor select");
  expect_contains(rendered, "bir.ret i32 %t14",
                  "BIR lowering should return the mirrored asymmetric deeper-else join-local arithmetic result on the BIR path");
}

void test_bir_lowering_accepts_two_param_select_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(
      rendered,
      "bir.func @choose2_mixed_then_deeper_post_chain(i32 %p.x, i32 %p.y) -> i32 {",
      "BIR lowering should preserve the mirrored asymmetric deeper-else split-predecessor ternary signature while widening the join-local arithmetic tail beyond the already-covered post-add form");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "BIR lowering should keep the mixed split then-arm affine head before the fused select");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "BIR lowering should keep the mixed split then-arm affine tail before the fused select");
  expect_contains(rendered, "%t10 = bir.add i32 %p.y, 11",
                  "BIR lowering should keep the deeper split else-arm affine head in the fused BIR block");
  expect_contains(rendered, "%t11 = bir.sub i32 %t10, 4",
                  "BIR lowering should keep the deeper split else-arm middle affine step in the fused BIR block");
  expect_contains(rendered, "%t12 = bir.add i32 %t11, 7",
                  "BIR lowering should keep the deeper split else-arm affine tail before the fused select");
  expect_contains(rendered, "%t13 = bir.select eq i32 %p.x, %p.y, %t9, %t12",
                  "BIR lowering should collapse the mirrored asymmetric split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t14 = bir.add i32 %t13, 6",
                  "BIR lowering should preserve the first join-local arithmetic step after the mirrored asymmetric split-predecessor select");
  expect_contains(rendered, "%t15 = bir.sub i32 %t14, 2",
                  "BIR lowering should preserve the trailing join-local subtraction after the fused select");
  expect_contains(rendered, "bir.ret i32 %t15",
                  "BIR lowering should return the widened mirrored asymmetric deeper-else join-local add/sub chain on the BIR path");
}

void test_bir_lowering_accepts_two_param_select_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_add_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_add_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(
      rendered,
      "bir.func @choose2_mixed_then_deeper_post_chain_tail(i32 %p.x, i32 %p.y) -> i32 {",
      "BIR lowering should preserve the mirrored asymmetric deeper-else split-predecessor ternary signature while extending the join-local affine tail by one more bounded step");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "BIR lowering should keep the mixed split then-arm affine head before the fused select");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "BIR lowering should keep the mixed split then-arm affine tail before the fused select");
  expect_contains(rendered, "%t10 = bir.add i32 %p.y, 11",
                  "BIR lowering should keep the deeper split else-arm affine head in the fused BIR block");
  expect_contains(rendered, "%t11 = bir.sub i32 %t10, 4",
                  "BIR lowering should keep the deeper split else-arm middle affine step in the fused BIR block");
  expect_contains(rendered, "%t12 = bir.add i32 %t11, 7",
                  "BIR lowering should keep the deeper split else-arm affine tail before the fused select");
  expect_contains(rendered, "%t13 = bir.select eq i32 %p.x, %p.y, %t9, %t12",
                  "BIR lowering should collapse the mirrored asymmetric split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t14 = bir.add i32 %t13, 6",
                  "BIR lowering should preserve the first join-local arithmetic step after the mirrored asymmetric split-predecessor select");
  expect_contains(rendered, "%t15 = bir.sub i32 %t14, 2",
                  "BIR lowering should preserve the second join-local arithmetic step after the fused select");
  expect_contains(rendered, "%t16 = bir.add i32 %t15, 9",
                  "BIR lowering should preserve the added third join-local arithmetic step after the fused select");
  expect_contains(rendered, "bir.ret i32 %t16",
                  "BIR lowering should return the extended mirrored asymmetric deeper-else join-local arithmetic tail on the BIR path");
}

void test_bir_lowering_accepts_two_param_select_split_predecessor_deeper_affine_phi_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_select_eq_split_predecessor_deeper_affine_phi_post_join_add_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose2_deeper_both_post(i32 %p.x, i32 %p.y) -> i32 {",
                  "BIR lowering should preserve the symmetric deeper split-predecessor ternary signature on the BIR path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "BIR lowering should keep the deeper split then-arm affine head in the fused BIR block");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "BIR lowering should keep the deeper split then-arm middle affine step in the fused BIR block");
  expect_contains(rendered, "%t10 = bir.add i32 %t9, 5",
                  "BIR lowering should keep the deeper split then-arm affine tail before the fused select");
  expect_contains(rendered, "%t11 = bir.add i32 %p.y, 11",
                  "BIR lowering should keep the deeper split else-arm affine head in the fused BIR block");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 4",
                  "BIR lowering should keep the deeper split else-arm middle affine step in the fused BIR block");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 7",
                  "BIR lowering should keep the deeper split else-arm affine tail before the fused select");
  expect_contains(rendered, "%t14 = bir.select eq i32 %p.x, %p.y, %t10, %t13",
                  "BIR lowering should collapse the symmetric deeper split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t15 = bir.add i32 %t14, 6",
                  "BIR lowering should preserve the bounded post-join add after the symmetric deeper split-predecessor select");
  expect_contains(rendered, "bir.ret i32 %t15",
                  "BIR lowering should return the symmetric deeper split-predecessor join-local arithmetic result on the BIR path");
}

void test_bir_lowering_accepts_mixed_predecessor_select_post_join_add_slice() {
  const auto lowered =
      c4c::backend::lower_to_bir(make_bir_mixed_predecessor_add_phi_post_join_add_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_mixed_add() -> i32 {",
                  "BIR lowering should preserve the bounded mixed predecessor/immediate conditional signature while widening the collapsed join-local arithmetic slice");
  expect_contains(rendered, "%t3 = bir.add i32 7, 5",
                  "BIR lowering should hoist the computed predecessor input into the fused BIR block before the select");
  expect_contains(rendered, "%t4 = bir.select slt i32 2, 3, %t3, 9",
                  "BIR lowering should collapse the mixed predecessor/immediate phi join into a single BIR select");
  expect_contains(rendered, "%t5 = bir.add i32 %t4, 6",
                  "BIR lowering should preserve the bounded post-join add on the fused select result inside the same BIR block");
  expect_contains(rendered, "bir.ret i32 %t5",
                  "BIR lowering should return the join-local arithmetic result instead of stopping at the fused select");
}

void test_bir_lowering_accepts_two_param_select_split_predecessor_deeper_affine_phi_post_join_add_sub_add_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_select_eq_split_predecessor_deeper_affine_phi_post_join_add_sub_add_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered,
                  "bir.func @choose2_deeper_both_post_chain_tail(i32 %p.x, i32 %p.y) -> i32 {",
                  "BIR lowering should preserve the symmetric deeper split-predecessor ternary signature while extending the join-local affine tail by one more bounded step");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "BIR lowering should keep the deeper split then-arm affine head on the BIR path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "BIR lowering should keep the deeper split then-arm middle affine step on the BIR path");
  expect_contains(rendered, "%t10 = bir.add i32 %t9, 5",
                  "BIR lowering should keep the deeper split then-arm affine tail on the BIR path");
  expect_contains(rendered, "%t11 = bir.add i32 %p.y, 11",
                  "BIR lowering should keep the deeper split else-arm affine head on the BIR path");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 4",
                  "BIR lowering should keep the deeper split else-arm middle affine step on the BIR path");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 7",
                  "BIR lowering should keep the deeper split else-arm affine tail on the BIR path");
  expect_contains(rendered, "%t14 = bir.select eq i32 %p.x, %p.y, %t10, %t13",
                  "BIR lowering should collapse the symmetric deeper split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t15 = bir.add i32 %t14, 6",
                  "BIR lowering should preserve the first join-local arithmetic step after the fused select");
  expect_contains(rendered, "%t16 = bir.sub i32 %t15, 2",
                  "BIR lowering should preserve the second join-local arithmetic step after the fused select");
  expect_contains(rendered, "%t17 = bir.add i32 %t16, 9",
                  "BIR lowering should preserve the added third join-local arithmetic step after the fused select");
  expect_contains(rendered, "bir.ret i32 %t17",
                  "BIR lowering should return the extended join-local arithmetic tail on the BIR path");
}

void test_bir_lowering_accepts_straight_line_add_sub_chain() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_add_sub_chain_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t0 = bir.add i32 2, 3",
                  "BIR lowering should keep the first chain instruction in BIR form");
  expect_contains(rendered, "%t1 = bir.sub i32 %t0, 1",
                  "BIR lowering should allow later chain instructions to reference prior BIR values");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "BIR lowering should let the return use the tail of the straight-line arithmetic chain");
}

void test_bir_lowering_accepts_i8_add_sub_chain() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_add_sub_chain_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @tiny_char(i8 %p.x) -> i8 {",
                  "BIR lowering should preserve i8 signatures while widening the scaffold beyond i32-only arithmetic");
  expect_contains(rendered, "%t0 = bir.add i8 %p.x, 2",
                  "BIR lowering should materialize the i8 add head in BIR terms");
  expect_contains(rendered, "%t1 = bir.sub i8 %t0, 1",
                  "BIR lowering should keep trailing i8 arithmetic in the widened BIR scaffold");
  expect_contains(rendered, "bir.ret i8 %t1",
                  "BIR lowering should let the widened i8 value flow into the return");
}

void test_bir_lowering_accepts_i64_add_sub_chain() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i64_return_add_sub_chain_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @wide_add(i64 %p.x) -> i64 {",
                  "BIR lowering should preserve i64 signatures while widening the scaffold beyond i8/i32 arithmetic");
  expect_contains(rendered, "%t0 = bir.add i64 %p.x, 2",
                  "BIR lowering should materialize the i64 add head in BIR terms");
  expect_contains(rendered, "%t1 = bir.sub i64 %t0, 1",
                  "BIR lowering should keep trailing i64 arithmetic in the widened BIR scaffold");
  expect_contains(rendered, "bir.ret i64 %t1",
                  "BIR lowering should let the widened i64 value flow into the return");
}

void test_bir_lowering_accepts_single_param_add_sub_chain() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_single_param_add_sub_chain_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @add_one(i32 %p.x) -> i32 {",
                  "BIR lowering should preserve the surviving one-parameter function signature");
  expect_contains(rendered, "%t0 = bir.add i32 %p.x, 2",
                  "BIR lowering should allow the bounded one-parameter slice to start from the incoming BIR value");
  expect_contains(rendered, "%t1 = bir.sub i32 %t0, 1",
                  "BIR lowering should keep the one-parameter arithmetic chain in BIR form");
}

void test_bir_lowering_accepts_two_param_add() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_two_param_add_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @add_pair(i32 %p.x, i32 %p.y) -> i32 {",
                  "BIR lowering should preserve the bounded two-parameter function signature");
  expect_contains(rendered, "%t0 = bir.add i32 %p.x, %p.y",
                  "BIR lowering should materialize the bounded two-parameter add in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should let the two-parameter add result flow directly into the return");
}

void test_bir_lowering_accepts_two_param_add_sub_chain() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_two_param_add_sub_chain_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @add_pair(i32 %p.x, i32 %p.y) -> i32 {",
                  "BIR lowering should preserve the bounded two-parameter signature for the affine chain");
  expect_contains(rendered, "%t0 = bir.add i32 %p.x, %p.y",
                  "BIR lowering should keep the two-parameter add head of the bounded affine chain in BIR form");
  expect_contains(rendered, "%t1 = bir.sub i32 %t0, 1",
                  "BIR lowering should allow a trailing immediate adjustment after the bounded two-parameter add");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "BIR lowering should let the adjusted two-parameter result flow into the return");
}

void test_bir_lowering_accepts_two_param_staged_affine_chain() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_two_param_staged_affine_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @add_pair(i32 %p.x, i32 %p.y) -> i32 {",
                  "BIR lowering should preserve the bounded two-parameter signature for the staged affine chain");
  expect_contains(rendered, "%t0 = bir.add i32 %p.x, 2",
                  "BIR lowering should keep the first staged immediate adjustment in BIR form");
  expect_contains(rendered, "%t1 = bir.add i32 %t0, %p.y",
                  "BIR lowering should allow the second parameter to enter after an earlier immediate stage");
  expect_contains(rendered, "%t2 = bir.sub i32 %t1, 1",
                  "BIR lowering should keep the trailing staged subtraction in BIR form");
  expect_contains(rendered, "bir.ret i32 %t2",
                  "BIR lowering should let the staged affine tail flow into the return");
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

}  // namespace

void run_backend_bir_lowering_tests() {
  RUN_TEST(test_bir_printer_renders_minimal_add_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_sub_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_mul_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_and_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_or_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_xor_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_shl_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_lshr_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_ashr_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_sdiv_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_udiv_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_srem_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_urem_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_eq_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_ne_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_slt_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_sle_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_sgt_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_sge_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_ult_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_ule_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_ugt_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_uge_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_select_scaffold);
  RUN_TEST(test_bir_printer_renders_minimal_return_immediate_scaffold);
  RUN_TEST(test_bir_printer_renders_i8_scaffold);
  RUN_TEST(test_bir_printer_renders_i64_scaffold);
  RUN_TEST(test_bir_validator_accepts_minimal_return_immediate_scaffold);
  RUN_TEST(test_bir_validator_accepts_minimal_i8_scaffold);
  RUN_TEST(test_bir_validator_accepts_minimal_i64_scaffold);
  RUN_TEST(test_bir_validator_accepts_minimal_select_scaffold);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_add_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_sub_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_mul_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_and_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_or_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_xor_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_shl_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_lshr_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_ashr_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_sdiv_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_udiv_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_srem_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_urem_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_eq_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_ne_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_slt_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_sle_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_sgt_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_sge_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_ult_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_ule_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_ugt_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_uge_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_select_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_i8_return_eq);
  RUN_TEST(test_bir_lowering_accepts_i8_return_ne);
  RUN_TEST(test_bir_lowering_accepts_i8_return_ult);
  RUN_TEST(test_bir_lowering_accepts_i8_return_ule);
  RUN_TEST(test_bir_lowering_accepts_i8_return_ugt);
  RUN_TEST(test_bir_lowering_accepts_i8_return_uge);
  RUN_TEST(test_bir_lowering_accepts_i8_return_slt);
  RUN_TEST(test_bir_lowering_accepts_i8_return_immediate);
  RUN_TEST(test_bir_lowering_accepts_single_param_select_branch_slice);
  RUN_TEST(test_bir_lowering_accepts_single_param_select_phi_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_select_phi_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_select_predecessor_add_phi_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_select_split_predecessor_add_phi_post_join_add_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_select_split_predecessor_add_phi_post_join_add_sub_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_select_split_predecessor_add_phi_post_join_add_sub_add_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_select_split_predecessor_deeper_affine_phi_post_join_add_sub_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_select_split_predecessor_mixed_affine_phi_post_join_add_sub_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_select_split_predecessor_mixed_affine_phi_post_join_add_sub_add_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_select_split_predecessor_mixed_affine_phi_post_join_add_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_select_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_select_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_select_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_add_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_select_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_select_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_select_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_add_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_select_split_predecessor_deeper_affine_phi_post_join_add_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_select_split_predecessor_deeper_affine_phi_post_join_add_sub_add_slice);
  RUN_TEST(test_bir_lowering_accepts_mixed_predecessor_select_post_join_add_slice);
  RUN_TEST(test_bir_lowering_accepts_straight_line_add_sub_chain);
  RUN_TEST(test_bir_lowering_accepts_i8_add_sub_chain);
  RUN_TEST(test_bir_lowering_accepts_i64_add_sub_chain);
  RUN_TEST(test_bir_lowering_accepts_single_param_add_sub_chain);
  RUN_TEST(test_bir_lowering_accepts_two_param_add);
  RUN_TEST(test_bir_lowering_accepts_two_param_add_sub_chain);
  RUN_TEST(test_bir_lowering_accepts_two_param_staged_affine_chain);
  RUN_TEST(test_bir_validator_rejects_returning_undefined_named_value);
  RUN_TEST(test_bir_validator_rejects_return_type_mismatch);
}
