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

void test_bir_lowering_accepts_tiny_return_sdiv_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_sdiv_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t0 = bir.sdiv i32 12, 3",
                  "BIR lowering should materialize the tiny signed-division slice in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should return the named BIR signed-division result");
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
  RUN_TEST(test_bir_printer_renders_minimal_sdiv_scaffold);
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
  RUN_TEST(test_bir_lowering_accepts_tiny_return_sdiv_lir_slice);
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
  RUN_TEST(test_bir_lowering_accepts_single_param_select_branch_slice);
  RUN_TEST(test_bir_lowering_accepts_single_param_select_phi_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_select_phi_slice);
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
