#include "backend.hpp"
#include "lir_adapter.hpp"
#include "target.hpp"

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

c4c::codegen::lir::LirModule make_conditional_return_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  auto& function = module.functions.front();
  function.blocks.clear();
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "slt", "i32", "2", "3"});
  entry.insts.push_back(
      LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "then", "else"};

  LirBlock then_block;
  then_block.id = LirBlockId{1};
  then_block.label = "then";
  then_block.terminator = LirRet{std::string("0"), "i32"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.terminator = LirRet{std::string("1"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  return module;
}

c4c::codegen::lir::LirModule make_direct_call_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "helper";
  helper.signature_text = "define i32 @helper()\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.terminator = LirRet{std::string("7"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirCallOp{"%t0", "i32", "@helper", "", ""});
  main_entry.terminator = LirRet{std::string("%t0"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_local_temp_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  function.alloca_insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
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

void test_aarch64_backend_scaffold_renders_supported_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_return_add_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "%t0 = add i32 2, 3",
                  "aarch64 scaffold should render the current adapter slice");
  expect_contains(rendered, "ret i32 %t0",
                  "aarch64 scaffold should preserve the current return path");
}

void test_aarch64_backend_renders_compare_and_branch_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "%t0 = icmp slt i32 2, 3",
                  "aarch64 backend should render integer compare instructions");
  expect_contains(rendered, "%t1 = zext i1 %t0 to i32",
                  "aarch64 backend should render compare normalization casts");
  expect_contains(rendered, "%t2 = icmp ne i32 %t1, 0",
                  "aarch64 backend should render the final branch predicate compare");
  expect_contains(rendered, "br i1 %t2, label %then, label %else",
                  "aarch64 backend should render conditional branches");
  expect_contains(rendered, "then:\n  ret i32 0",
                  "aarch64 backend should preserve the then return block");
  expect_contains(rendered, "else:\n  ret i32 1",
                  "aarch64 backend should preserve the else return block");
}

void test_aarch64_backend_scaffold_rejects_out_of_slice_ir() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(LirPhiOp{"%t0", "i32", {{"0", "entry"}}});

  try {
    (void)c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{module},
        c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
    fail("aarch64 scaffold should reject IR outside the adapter slice");
  } catch (const std::invalid_argument& ex) {
    expect_contains(ex.what(), "aarch64 backend emitter does not support",
                    "aarch64 emitter should identify target-local support limits");
    expect_contains(ex.what(), "non-ALU/non-branch/non-call/non-memory instructions",
                    "aarch64 emitter should preserve the unsupported detail");
  }
}

void test_aarch64_backend_renders_direct_call_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_direct_call_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "define i32 @helper()",
                  "aarch64 backend should preserve helper definitions");
  expect_contains(rendered, "%t0 = call i32 @helper()",
                  "aarch64 backend should render direct calls");
  expect_contains(rendered, "ret i32 %t0",
                  "aarch64 backend should preserve call results through return");
}

void test_aarch64_backend_renders_local_temp_memory_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_local_temp_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }",
                  "aarch64 backend should preserve required type declarations");
  expect_contains(rendered, "%lv.x = alloca i32, align 4",
                  "aarch64 backend should render entry allocas");
  expect_contains(rendered, "store i32 5, ptr %lv.x",
                  "aarch64 backend should render entry stores");
  expect_contains(rendered, "%t0 = load i32, ptr %lv.x",
                  "aarch64 backend should render local temporary loads");
  expect_contains(rendered, "ret i32 %t0",
                  "aarch64 backend should preserve the loaded return value");
}

}  // namespace

int main() {
  test_adapts_direct_return();
  test_renders_return_add();
  test_rejects_unsupported_instruction();
  test_aarch64_backend_scaffold_renders_supported_slice();
  test_aarch64_backend_renders_compare_and_branch_slice();
  test_aarch64_backend_scaffold_rejects_out_of_slice_ir();
  test_aarch64_backend_renders_direct_call_slice();
  test_aarch64_backend_renders_local_temp_memory_slice();
  return 0;
}
