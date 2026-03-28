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

c4c::codegen::lir::LirModule make_void_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "helper";
  function.signature_text = "define void @helper()\n";
  function.entry = LirBlockId{0};

  LirBlock block;
  block.id = LirBlockId{0};
  block.label = "entry";
  block.terminator = LirRet{};
  function.blocks.push_back(std::move(block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_declaration_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.Pair = type { i32, i32 }");

  LirFunction decl;
  decl.name = "helper";
  decl.signature_text = "declare i32 @helper(i32)\n";
  decl.is_declaration = true;

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@helper", "", "i32 5"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  main_fn.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(decl));
  module.functions.push_back(std::move(main_fn));
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

c4c::codegen::lir::LirModule make_param_slot_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main(i32 %p.x)\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.param.x", "i32", "", 4});
  function.alloca_insts.push_back(LirStoreOp{"i32", "%p.x", "%lv.param.x"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.param.x"});
  entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%t0", "1"});
  entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.param.x"});
  entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.param.x"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_one";
  helper.signature_text = "define i32 @add_one(i32 %p.x)\n";
  helper.entry = LirBlockId{0};
  helper.alloca_insts.push_back(LirAllocaOp{"%lv.param.x", "i32", "", 4});
  helper.alloca_insts.push_back(LirStoreOp{"i32", "%p.x", "%lv.param.x"});

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.param.x"});
  helper_entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%t0", "1"});
  helper_entry.terminator = LirRet{std::string("%t1"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirCallOp{"%t0", "i32", "@add_one", "(i32)", "i32 5"});
  main_entry.terminator = LirRet{std::string("%t0"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_local_array_gep_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.arr", "[2 x i32]", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "[2 x i32]", "%lv.arr", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(
      LirCastOp{"%t1", LirCastKind::SExt, "i32", "0", "i64"});
  entry.insts.push_back(LirGepOp{"%t2", "i32", "%t0", false, {"i64 %t1"}});
  entry.insts.push_back(LirStoreOp{"i32", "4", "%t2"});
  entry.insts.push_back(
      LirGepOp{"%t3", "[2 x i32]", "%lv.arr", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(
      LirCastOp{"%t4", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t5", "i32", "%t3", false, {"i64 %t4"}});
  entry.insts.push_back(LirStoreOp{"i32", "3", "%t5"});
  entry.insts.push_back(LirLoadOp{"%t6", "i32", "%t2"});
  entry.insts.push_back(LirLoadOp{"%t7", "i32", "%t5"});
  entry.insts.push_back(LirBinOp{"%t8", "add", "i32", "%t6", "%t7"});
  entry.terminator = LirRet{std::string("%t8"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_param_member_array_gep_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.Pair = type { [2 x i32] }");

  LirFunction function;
  function.name = "get_second";
  function.signature_text = "define i32 @get_second(%struct.Pair %p.p)\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.param.p", "%struct.Pair", "", 4});
  function.alloca_insts.push_back(
      LirStoreOp{"%struct.Pair", "%p.p", "%lv.param.p"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "%struct.Pair", "%lv.param.p", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(
      LirGepOp{"%t1", "[2 x i32]", "%t0", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(
      LirCastOp{"%t2", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t3", "i32", "%t1", false, {"i64 %t2"}});
  entry.insts.push_back(LirLoadOp{"%t4", "i32", "%t3"});
  entry.terminator = LirRet{std::string("%t4"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_nested_member_pointer_array_gep_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.Inner = type { [2 x i32] }");
  module.type_decls.push_back("%struct.Outer = type { ptr }");

  LirFunction function;
  function.name = "get_second";
  function.signature_text = "define i32 @get_second(ptr %p.o)\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "%struct.Outer", "%p.o", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(LirLoadOp{"%t1", "ptr", "%t0"});
  entry.insts.push_back(
      LirGepOp{"%t2", "%struct.Inner", "%t1", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(
      LirGepOp{"%t3", "[2 x i32]", "%t2", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(
      LirCastOp{"%t4", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t5", "i32", "%t3", false, {"i64 %t4"}});
  entry.insts.push_back(LirLoadOp{"%t6", "i32", "%t5"});
  entry.terminator = LirRet{std::string("%t6"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_nested_param_member_array_gep_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.Inner = type { [2 x i32] }");
  module.type_decls.push_back("%struct.Outer = type { %struct.Inner }");

  LirFunction function;
  function.name = "get_second";
  function.signature_text = "define i32 @get_second(%struct.Outer %p.o)\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.param.o", "%struct.Outer", "", 4});
  function.alloca_insts.push_back(
      LirStoreOp{"%struct.Outer", "%p.o", "%lv.param.o"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "%struct.Outer", "%lv.param.o", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(
      LirGepOp{"%t1", "%struct.Inner", "%t0", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(
      LirCastOp{"%t2", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t3", "i32", "%t1", false, {"i64 %t2"}});
  entry.insts.push_back(LirLoadOp{"%t4", "i32", "%t3"});
  entry.terminator = LirRet{std::string("%t4"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_global_load_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.globals.push_back(LirGlobal{
      LirGlobalId{0},
      "g_counter",
      {},
      false,
      false,
      "",
      "global ",
      "i32",
      "11",
      4,
      false,
  });

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "@g_counter"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_string_literal_char_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.string_pool.push_back(LirStringConst{"@.str0", "hi", 3});

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "[3 x i8]", "@.str0", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirGepOp{"%t1", "i8", "%t0", false, {"i64 1"}});
  entry.insts.push_back(LirLoadOp{"%t2", "i8", "%t1"});
  entry.insts.push_back(
      LirCastOp{"%t3", LirCastKind::SExt, "i8", "%t2", "i32"});
  entry.terminator = LirRet{std::string("%t3"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_extern_decl_call_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.extern_decls.push_back(LirExternDecl{"helper_ext", "i32"});

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@helper_ext", "", "i32 5"});
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

void test_aarch64_backend_renders_void_return_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_void_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "define void @helper()",
                  "aarch64 backend should preserve void signatures");
  expect_contains(rendered, "ret void",
                  "aarch64 backend should render void returns");
}

void test_aarch64_backend_preserves_module_headers_and_declarations() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_declaration_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "target datalayout = \"e-m:e-i64:64-i128:128-n32:64-S128\"",
                  "aarch64 backend should preserve the module datalayout");
  expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                  "aarch64 backend should preserve the module target triple");
  expect_contains(rendered, "%struct.Pair = type { i32, i32 }",
                  "aarch64 backend should preserve module type declarations");
  expect_contains(rendered, "declare i32 @helper(i32)\n",
                  "aarch64 backend should preserve declarations without bodies");
  expect_contains(rendered, "define i32 @main()\n{\nentry:\n",
                  "aarch64 backend should still open function bodies for definitions");
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

void test_aarch64_backend_renders_param_slot_memory_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_param_slot_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "define i32 @main(i32 %p.x)",
                  "aarch64 backend should preserve parameterized signatures");
  expect_contains(rendered, "%lv.param.x = alloca i32, align 4",
                  "aarch64 backend should render modified parameter slots");
  expect_contains(rendered, "store i32 %p.x, ptr %lv.param.x",
                  "aarch64 backend should spill modified parameters into entry slots");
  expect_contains(rendered, "%t0 = load i32, ptr %lv.param.x",
                  "aarch64 backend should reload parameter slots");
  expect_contains(rendered, "store i32 %t1, ptr %lv.param.x",
                  "aarch64 backend should write updated parameter values back to slots");
  expect_contains(rendered, "ret i32 %t2",
                  "aarch64 backend should preserve the final reloaded parameter value");
}

void test_aarch64_backend_renders_typed_direct_call_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_typed_direct_call_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "define i32 @add_one(i32 %p.x)",
                  "aarch64 backend should preserve helper signatures with parameters");
  expect_contains(rendered, "call i32 (i32) @add_one(i32 5)",
                  "aarch64 backend should render typed direct calls");
  expect_contains(rendered, "ret i32 %t0",
                  "aarch64 backend should preserve typed call results through return");
}

void test_aarch64_backend_renders_local_array_gep_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_local_array_gep_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "%lv.arr = alloca [2 x i32], align 4",
                  "aarch64 backend should render local array allocas");
  expect_contains(rendered, "%t0 = getelementptr [2 x i32], ptr %lv.arr, i64 0, i64 0",
                  "aarch64 backend should render array-decay GEP");
  expect_contains(rendered, "%t1 = sext i32 0 to i64",
                  "aarch64 backend should render integer index widening for local arrays");
  expect_contains(rendered, "%t2 = getelementptr i32, ptr %t0, i64 %t1",
                  "aarch64 backend should render indexed local array GEP");
  expect_contains(rendered, "store i32 4, ptr %t2",
                  "aarch64 backend should store through local array element pointers");
  expect_contains(rendered, "store i32 3, ptr %t5",
                  "aarch64 backend should store through later local array element pointers");
  expect_contains(rendered, "ret i32 %t8",
                  "aarch64 backend should preserve the loaded array sum");
}

void test_aarch64_backend_renders_param_member_array_gep_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_param_member_array_gep_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "%struct.Pair = type { [2 x i32] }",
                  "aarch64 backend should preserve struct member array type declarations");
  expect_contains(rendered, "%lv.param.p = alloca %struct.Pair, align 4",
                  "aarch64 backend should spill by-value struct parameters into stack slots");
  expect_contains(rendered, "store %struct.Pair %p.p, ptr %lv.param.p",
                  "aarch64 backend should store by-value struct parameters into their slots");
  expect_contains(rendered, "%t0 = getelementptr %struct.Pair, ptr %lv.param.p, i32 0, i32 0",
                  "aarch64 backend should render the member-addressing GEP");
  expect_contains(rendered, "%t1 = getelementptr [2 x i32], ptr %t0, i64 0, i64 0",
                  "aarch64 backend should render array decay from struct members");
  expect_contains(rendered, "%t3 = getelementptr i32, ptr %t1, i64 %t2",
                  "aarch64 backend should render indexed member-array addressing");
  expect_contains(rendered, "ret i32 %t4",
                  "aarch64 backend should preserve the loaded member-array result");
}

void test_aarch64_backend_renders_nested_member_pointer_array_gep_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_nested_member_pointer_array_gep_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "%struct.Inner = type { [2 x i32] }",
                  "aarch64 backend should preserve nested pointee type declarations");
  expect_contains(rendered, "%struct.Outer = type { ptr }",
                  "aarch64 backend should preserve nested pointer-holder type declarations");
  expect_contains(rendered, "%t0 = getelementptr %struct.Outer, ptr %p.o, i32 0, i32 0",
                  "aarch64 backend should render outer member-addressing GEP");
  expect_contains(rendered, "%t1 = load ptr, ptr %t0",
                  "aarch64 backend should reload nested struct pointers from outer members");
  expect_contains(rendered, "%t2 = getelementptr %struct.Inner, ptr %t1, i32 0, i32 0",
                  "aarch64 backend should render nested pointee member-addressing GEP");
  expect_contains(rendered, "%t3 = getelementptr [2 x i32], ptr %t2, i64 0, i64 0",
                  "aarch64 backend should render array decay from nested pointee members");
  expect_contains(rendered, "%t5 = getelementptr i32, ptr %t3, i64 %t4",
                  "aarch64 backend should render indexed nested member-pointer addressing");
  expect_contains(rendered, "ret i32 %t6",
                  "aarch64 backend should preserve the loaded nested member-pointer result");
}

void test_aarch64_backend_renders_nested_param_member_array_gep_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_nested_param_member_array_gep_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "%struct.Inner = type { [2 x i32] }",
                  "aarch64 backend should preserve nested by-value member type declarations");
  expect_contains(rendered, "%struct.Outer = type { %struct.Inner }",
                  "aarch64 backend should preserve nested by-value outer type declarations");
  expect_contains(rendered, "%lv.param.o = alloca %struct.Outer, align 4",
                  "aarch64 backend should spill nested by-value parameters into stack slots");
  expect_contains(rendered, "store %struct.Outer %p.o, ptr %lv.param.o",
                  "aarch64 backend should store nested by-value parameters into their slots");
  expect_contains(rendered, "%t0 = getelementptr %struct.Outer, ptr %lv.param.o, i32 0, i32 0",
                  "aarch64 backend should render the outer nested by-value member-addressing GEP");
  expect_contains(rendered, "%t1 = getelementptr %struct.Inner, ptr %t0, i32 0, i32 0",
                  "aarch64 backend should render the inner nested by-value member-addressing GEP");
  expect_contains(rendered, "%t3 = getelementptr i32, ptr %t1, i64 %t2",
                  "aarch64 backend should render indexed nested by-value member-array addressing");
  expect_contains(rendered, "ret i32 %t4",
                  "aarch64 backend should preserve the loaded nested by-value member-array result");
}

void test_aarch64_backend_renders_global_definition_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_global_load_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "@g_counter = global i32 11, align 4",
                  "aarch64 backend should render module global definitions");
  expect_contains(rendered, "define i32 @main()\n{\nentry:\n  %t0 = load i32, ptr @g_counter\n",
                  "aarch64 backend should preserve global loads through the target-local memory path");
  expect_contains(rendered, "ret i32 %t0",
                  "aarch64 backend should preserve the global load result");
}

void test_aarch64_backend_renders_string_pool_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_string_literal_char_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "@.str0 = private unnamed_addr constant [3 x i8] c\"hi\\00\"",
                  "aarch64 backend should render module string pool constants");
  expect_contains(rendered, "%t0 = getelementptr [3 x i8], ptr @.str0, i64 0, i64 0",
                  "aarch64 backend should preserve string-literal decay through GEP");
  expect_contains(rendered, "%t1 = getelementptr i8, ptr %t0, i64 1",
                  "aarch64 backend should preserve indexed string-literal addressing");
  expect_contains(rendered, "%t2 = load i8, ptr %t1",
                  "aarch64 backend should preserve string-literal byte loads");
  expect_contains(rendered, "%t3 = sext i8 %t2 to i32",
                  "aarch64 backend should preserve byte-to-int promotion");
  expect_contains(rendered, "ret i32 %t3",
                  "aarch64 backend should preserve the promoted string-literal result");
}

void test_aarch64_backend_renders_extern_decl_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_extern_decl_call_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "declare i32 @helper_ext(...)\n",
                  "aarch64 backend should render module extern declarations");
  expect_contains(rendered, "define i32 @main()\n{\nentry:\n  %t0 = call i32 @helper_ext(i32 5)\n",
                  "aarch64 backend should preserve direct calls that target extern declarations");
  expect_contains(rendered, "ret i32 %t0",
                  "aarch64 backend should preserve the extern call result");
}

}  // namespace

int main() {
  test_adapts_direct_return();
  test_renders_return_add();
  test_rejects_unsupported_instruction();
  test_aarch64_backend_scaffold_renders_supported_slice();
  test_aarch64_backend_renders_void_return_slice();
  test_aarch64_backend_preserves_module_headers_and_declarations();
  test_aarch64_backend_renders_compare_and_branch_slice();
  test_aarch64_backend_scaffold_rejects_out_of_slice_ir();
  test_aarch64_backend_renders_direct_call_slice();
  test_aarch64_backend_renders_local_temp_memory_slice();
  test_aarch64_backend_renders_param_slot_memory_slice();
  test_aarch64_backend_renders_typed_direct_call_slice();
  test_aarch64_backend_renders_local_array_gep_slice();
  test_aarch64_backend_renders_param_member_array_gep_slice();
  test_aarch64_backend_renders_nested_member_pointer_array_gep_slice();
  test_aarch64_backend_renders_nested_param_member_array_gep_slice();
  test_aarch64_backend_renders_global_definition_slice();
  test_aarch64_backend_renders_string_pool_slice();
  test_aarch64_backend_renders_extern_decl_slice();
  return 0;
}
