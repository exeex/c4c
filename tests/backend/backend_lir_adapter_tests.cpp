#include "lir_adapter.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

[[noreturn]] void fail(const std::string& message) {
  std::cerr << "FAIL: " << message << "\n";
  std::exit(1);
}

void expect_true(bool condition, const std::string& message) {
  if (!condition) fail(message);
}

void expect_contains(const std::string& text,
                     const std::string& needle,
                     const std::string& message) {
  if (text.find(needle) == std::string::npos) {
    fail(message + "\nExpected: " + needle + "\nActual:\n" + text);
  }
}

c4c::codegen::lir::LirModule make_return_zero_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-n8:16:32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock block;
  block.id = LirBlockId{0};
  block.label = "entry";
  block.terminator = LirRet{std::string("0"), "i32"};
  function.blocks.push_back(std::move(block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_return_add_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  auto& function = module.functions.front();
  auto& block = function.blocks.front();
  block.insts.push_back(LirBinOp{"%t0", "add", "i32", "2", "3"});
  block.terminator = LirRet{std::string("%t0"), "i32"};
  return module;
}

void test_adapts_direct_return() {
  const auto adapted = c4c::backend::adapt_return_only_module(make_return_zero_module());
  expect_true(adapted.functions.size() == 1, "adapter should preserve one function");
  expect_true(adapted.functions.front().blocks.size() == 1,
              "adapter should preserve one block");
  expect_true(!adapted.functions.front().blocks.front().terminator.value->compare("0"),
              "adapter should preserve the return literal");
}

void test_renders_return_add() {
  const auto adapted = c4c::backend::adapt_return_only_module(make_return_add_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "%t0 = add i32 2, 3",
                  "adapter renderer should emit the add instruction");
  expect_contains(rendered, "ret i32 %t0",
                  "adapter renderer should emit the adapted return");
}

void test_rejects_unsupported_instruction() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(LirLoadOp{"%t0", "i32", "%ptr"});
  block.terminator = LirRet{std::string("%t0"), "i32"};

  try {
    (void)c4c::backend::adapt_return_only_module(module);
    fail("adapter should reject unsupported instructions");
  } catch (const std::invalid_argument& ex) {
    expect_contains(ex.what(), "non-binary instructions",
                    "adapter should explain unsupported instructions");
  }
}

}  // namespace

int main() {
  test_adapts_direct_return();
  test_renders_return_add();
  test_rejects_unsupported_instruction();
  return 0;
}
