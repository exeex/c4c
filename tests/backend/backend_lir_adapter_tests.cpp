#include "backend.hpp"
#include "lir_adapter.hpp"
#include "target.hpp"
#include "../../src/backend/aarch64/assembler/parser.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace c4c::backend::aarch64::assembler {

bool is_branch_reloc_type(std::uint32_t elf_type);

}  // namespace c4c::backend::aarch64::assembler

namespace c4c::backend::aarch64::assembler::encoder {

bool is_64bit_reg(const std::string& name);
bool is_32bit_reg(const std::string& name);
bool is_fp_reg(const std::string& name);
std::uint32_t sf_bit(bool is_64);

}  // namespace c4c::backend::aarch64::assembler::encoder

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

c4c::codegen::lir::LirModule make_conditional_return_le_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  auto& function = module.functions.front();
  function.blocks.clear();
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "sle", "i32", "2", "3"});
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

c4c::codegen::lir::LirModule make_conditional_return_gt_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  auto& function = module.functions.front();
  function.blocks.clear();
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "sgt", "i32", "3", "2"});
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

c4c::codegen::lir::LirModule make_conditional_return_ge_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  auto& function = module.functions.front();
  function.blocks.clear();
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "sge", "i32", "3", "2"});
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
  module.type_decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");

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
  helper_entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.param.x"});
  helper_entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.param.x"});
  helper_entry.terminator = LirRet{std::string("%t2"), "i32"};
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

c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_pair";
  helper.signature_text = "define i32 @add_pair(i32 %p.x, i32 %p.y)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "%p.y"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(
      LirCallOp{"%t0", "i32", "@add_pair", "(i32, i32)", "i32 5, i32 7"});
  main_entry.terminator = LirRet{std::string("%t0"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_local_arg_module() {
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
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  main_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  main_entry.insts.push_back(LirCallOp{"%t1", "i32", "@add_one", "(i32)", "i32 %t0"});
  main_entry.terminator = LirRet{std::string("%t1"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_local_arg_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_pair";
  helper.signature_text = "define i32 @add_pair(i32 %p.x, i32 %p.y)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "%p.y"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  main_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  main_entry.insts.push_back(
      LirCallOp{"%t1", "i32", "@add_pair", "(i32, i32)", "i32 %t0, i32 7"});
  main_entry.terminator = LirRet{std::string("%t1"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_second_local_arg_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_pair";
  helper.signature_text = "define i32 @add_pair(i32 %p.x, i32 %p.y)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "%p.y"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.y"});
  main_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.y"});
  main_entry.insts.push_back(
      LirCallOp{"%t1", "i32", "@add_pair", "(i32, i32)", "i32 5, i32 %t0"});
  main_entry.terminator = LirRet{std::string("%t1"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_second_local_rewrite_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_pair";
  helper.signature_text = "define i32 @add_pair(i32 %p.x, i32 %p.y)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "%p.y"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.y"});
  main_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.y"});
  main_entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%t0", "0"});
  main_entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.y"});
  main_entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.y"});
  main_entry.insts.push_back(
      LirCallOp{"%t3", "i32", "@add_pair", "(i32, i32)", "i32 5, i32 %t2"});
  main_entry.terminator = LirRet{std::string("%t3"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_first_local_rewrite_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_pair";
  helper.signature_text = "define i32 @add_pair(i32 %p.x, i32 %p.y)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "%p.y"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  main_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  main_entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%t0", "0"});
  main_entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.x"});
  main_entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  main_entry.insts.push_back(
      LirCallOp{"%t3", "i32", "@add_pair", "(i32, i32)", "i32 %t2, i32 7"});
  main_entry.terminator = LirRet{std::string("%t3"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_both_local_arg_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_pair";
  helper.signature_text = "define i32 @add_pair(i32 %p.x, i32 %p.y)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "%p.y"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  main_entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.y"});
  main_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  main_entry.insts.push_back(LirLoadOp{"%t1", "i32", "%lv.y"});
  main_entry.insts.push_back(
      LirCallOp{"%t2", "i32", "@add_pair", "(i32, i32)", "i32 %t0, i32 %t1"});
  main_entry.terminator = LirRet{std::string("%t2"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_both_local_first_rewrite_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_pair";
  helper.signature_text = "define i32 @add_pair(i32 %p.x, i32 %p.y)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "%p.y"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  main_entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.y"});
  main_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  main_entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%t0", "0"});
  main_entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.x"});
  main_entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  main_entry.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.y"});
  main_entry.insts.push_back(
      LirCallOp{"%t4", "i32", "@add_pair", "(i32, i32)", "i32 %t2, i32 %t3"});
  main_entry.terminator = LirRet{std::string("%t4"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_both_local_second_rewrite_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_pair";
  helper.signature_text = "define i32 @add_pair(i32 %p.x, i32 %p.y)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "%p.y"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  main_entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.y"});
  main_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.y"});
  main_entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%t0", "0"});
  main_entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.y"});
  main_entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  main_entry.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.y"});
  main_entry.insts.push_back(
      LirCallOp{"%t4", "i32", "@add_pair", "(i32, i32)", "i32 %t2, i32 %t3"});
  main_entry.terminator = LirRet{std::string("%t4"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_both_local_double_rewrite_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_pair";
  helper.signature_text = "define i32 @add_pair(i32 %p.x, i32 %p.y)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "%p.y"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  main_entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.y"});
  main_entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  main_entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%t0", "0"});
  main_entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.x"});
  main_entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.y"});
  main_entry.insts.push_back(LirBinOp{"%t3", "add", "i32", "%t2", "0"});
  main_entry.insts.push_back(LirStoreOp{"i32", "%t3", "%lv.y"});
  main_entry.insts.push_back(LirLoadOp{"%t4", "i32", "%lv.x"});
  main_entry.insts.push_back(LirLoadOp{"%t5", "i32", "%lv.y"});
  main_entry.insts.push_back(
      LirCallOp{"%t6", "i32", "@add_pair", "(i32, i32)", "i32 %t4, i32 %t5"});
  main_entry.terminator = LirRet{std::string("%t6"), "i32"};
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

c4c::codegen::lir::LirModule make_extern_global_load_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.globals.push_back(LirGlobal{
      LirGlobalId{0},
      "ext_counter",
      {},
      false,
      false,
      "external ",
      "global ",
      "i32",
      "",
      4,
      true,
  });

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "@ext_counter"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_extern_global_array_load_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.globals.push_back(LirGlobal{
      LirGlobalId{0},
      "ext_arr",
      {},
      false,
      false,
      "external ",
      "global ",
      "[2 x i32]",
      "",
      4,
      true,
  });

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "[2 x i32]", "@ext_arr", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t2", "i32", "%t0", false, {"i64 %t1"}});
  entry.insts.push_back(LirLoadOp{"%t3", "i32", "%t2"});
  entry.terminator = LirRet{std::string("%t3"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_global_char_pointer_diff_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.globals.push_back(LirGlobal{
      LirGlobalId{0},
      "g_bytes",
      {},
      false,
      false,
      "",
      "global ",
      "[2 x i8]",
      "zeroinitializer",
      1,
      false,
  });

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "[2 x i8]", "@g_bytes", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirGepOp{"%t1", "i8", "%t0", false, {"i64 1"}});
  entry.insts.push_back(
      LirCastOp{"%t2", LirCastKind::PtrToInt, "ptr", "%t1", "i64"});
  entry.insts.push_back(
      LirCastOp{"%t3", LirCastKind::PtrToInt, "ptr", "%t0", "i64"});
  entry.insts.push_back(LirBinOp{"%t4", "sub", "i64", "%t2", "%t3"});
  entry.insts.push_back(LirCmpOp{"%t5", false, "eq", "i64", "%t4", "1"});
  entry.insts.push_back(
      LirCastOp{"%t6", LirCastKind::ZExt, "i1", "%t5", "i32"});
  entry.terminator = LirRet{std::string("%t6"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_global_int_pointer_diff_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.globals.push_back(LirGlobal{
      LirGlobalId{0},
      "g_words",
      {},
      false,
      false,
      "",
      "global ",
      "[2 x i32]",
      "zeroinitializer",
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
  entry.insts.push_back(
      LirGepOp{"%t0", "[2 x i32]", "@g_words", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirGepOp{"%t1", "i32", "%t0", false, {"i64 1"}});
  entry.insts.push_back(
      LirCastOp{"%t2", LirCastKind::PtrToInt, "ptr", "%t1", "i64"});
  entry.insts.push_back(
      LirCastOp{"%t3", LirCastKind::PtrToInt, "ptr", "%t0", "i64"});
  entry.insts.push_back(LirBinOp{"%t4", "sub", "i64", "%t2", "%t3"});
  entry.insts.push_back(LirBinOp{"%t5", "sdiv", "i64", "%t4", "4"});
  entry.insts.push_back(LirCmpOp{"%t6", false, "eq", "i64", "%t5", "1"});
  entry.insts.push_back(
      LirCastOp{"%t7", LirCastKind::ZExt, "i1", "%t6", "i32"});
  entry.terminator = LirRet{std::string("%t7"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_global_int_pointer_roundtrip_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.globals.push_back(LirGlobal{
      LirGlobalId{0},
      "g_value",
      {},
      false,
      false,
      "",
      "global ",
      "i32",
      "9",
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
  entry.insts.push_back(
      LirCastOp{"%t0", LirCastKind::PtrToInt, "ptr", "@g_value", "i64"});
  entry.insts.push_back(
      LirCastOp{"%t1", LirCastKind::IntToPtr, "i64", "%t0", "ptr"});
  entry.insts.push_back(LirLoadOp{"%t2", "i32", "%t1"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bitcast_scalar_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i64 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirCastOp{"%t0", LirCastKind::Bitcast, "double", "0.000000e+00", "i64"});
  entry.terminator = LirRet{std::string("%t0"), "i64"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_trunc_scalar_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i16 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirCastOp{"%t0", LirCastKind::Trunc, "i32", "13124", "i16"});
  entry.terminator = LirRet{std::string("%t0"), "i16"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_va_intrinsic_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");
  module.need_va_start = true;
  module.need_va_end = true;
  module.need_va_copy = true;

  LirFunction function;
  function.name = "variadic_probe";
  function.signature_text = "define void @variadic_probe(i32 %p.count, ...)\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(
      LirAllocaOp{"%lv.ap", "%struct.__va_list_tag_", "", 8});
  function.alloca_insts.push_back(
      LirAllocaOp{"%lv.ap_copy", "%struct.__va_list_tag_", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirVaStartOp{"%lv.ap"});
  entry.insts.push_back(LirVaCopyOp{"%lv.ap_copy", "%lv.ap"});
  entry.insts.push_back(LirVaEndOp{"%lv.ap_copy"});
  entry.insts.push_back(LirVaEndOp{"%lv.ap"});
  entry.terminator = LirRet{};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_va_arg_scalar_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");
  module.need_va_start = true;
  module.need_va_end = true;

  LirFunction function;
  function.name = "sum2";
  function.signature_text = "define i32 @sum2(i32 %p.first, ...)\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(
      LirAllocaOp{"%lv.ap", "%struct.__va_list_tag_", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirVaStartOp{"%lv.ap"});
  entry.insts.push_back(LirVaArgOp{"%t0", "%lv.ap", "i32"});
  entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%p.first", "%t0"});
  entry.insts.push_back(LirVaEndOp{"%lv.ap"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_va_arg_pair_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");
  module.type_decls.push_back("%struct.Pair = type { i32, i32 }");
  module.need_va_start = true;
  module.need_va_end = true;
  module.need_memcpy = true;

  LirFunction function;
  function.name = "pair_sum";
  function.signature_text = "define i32 @pair_sum(i32 %p.seed, ...)\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(
      LirAllocaOp{"%lv.ap", "%struct.__va_list_tag_", "", 8});
  function.alloca_insts.push_back(
      LirAllocaOp{"%lv.pair.tmp", "%struct.Pair", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirVaStartOp{"%lv.ap"});
  entry.insts.push_back(LirCmpOp{"%t0", false, "slt", "i32", "-8", "0"});
  entry.terminator = LirCondBr{"%t0", "reg", "stack"};

  LirBlock reg_block;
  reg_block.id = LirBlockId{1};
  reg_block.label = "reg";
  reg_block.insts.push_back(
      LirGepOp{"%t1", "%struct.__va_list_tag_", "%lv.ap", false, {"i32 0", "i32 1"}});
  reg_block.insts.push_back(LirLoadOp{"%t2", "ptr", "%t1"});
  reg_block.insts.push_back(LirGepOp{"%t3", "i8", "%t2", false, {"i32 -8"}});
  reg_block.terminator = LirBr{"join"};

  LirBlock stack_block;
  stack_block.id = LirBlockId{2};
  stack_block.label = "stack";
  stack_block.insts.push_back(
      LirGepOp{"%t4", "%struct.__va_list_tag_", "%lv.ap", false, {"i32 0", "i32 0"}});
  stack_block.insts.push_back(LirLoadOp{"%t5", "ptr", "%t4"});
  stack_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(
      LirPhiOp{"%t6", "ptr", {{"%t3", "reg"}, {"%t5", "stack"}}});
  join_block.insts.push_back(LirMemcpyOp{"%lv.pair.tmp", "%t6", "8", false});
  join_block.insts.push_back(LirVaEndOp{"%lv.ap"});
  join_block.terminator = LirRet{std::string("%p.seed"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(reg_block));
  function.blocks.push_back(std::move(stack_block));
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_va_arg_bigints_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");
  module.type_decls.push_back("%struct.BigInts = type { i32, i32, i32, i32, i32 }");
  module.need_va_start = true;
  module.need_va_end = true;
  module.need_memcpy = true;

  LirFunction function;
  function.name = "bigints_sum";
  function.signature_text = "define i32 @bigints_sum(i32 %p.seed, ...)\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(
      LirAllocaOp{"%lv.ap", "%struct.__va_list_tag_", "", 8});
  function.alloca_insts.push_back(
      LirAllocaOp{"%lv.bigints.tmp", "%struct.BigInts", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirVaStartOp{"%lv.ap"});
  entry.insts.push_back(
      LirGepOp{"%t0", "%struct.__va_list_tag_", "%lv.ap", false, {"i32 0", "i32 3"}});
  entry.insts.push_back(LirLoadOp{"%t1", "i32", "%t0"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "sge", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "stack", "regtry"};

  LirBlock reg_try_block;
  reg_try_block.id = LirBlockId{1};
  reg_try_block.label = "regtry";
  reg_try_block.insts.push_back(LirBinOp{"%t3", "add", "i32", "%t1", "8"});
  reg_try_block.insts.push_back(LirStoreOp{"i32", "%t3", "%t0"});
  reg_try_block.insts.push_back(LirCmpOp{"%t4", false, "sle", "i32", "%t3", "0"});
  reg_try_block.terminator = LirCondBr{"%t4", "reg", "stack"};

  LirBlock reg_block;
  reg_block.id = LirBlockId{2};
  reg_block.label = "reg";
  reg_block.insts.push_back(
      LirGepOp{"%t5", "%struct.__va_list_tag_", "%lv.ap", false, {"i32 0", "i32 1"}});
  reg_block.insts.push_back(LirLoadOp{"%t6", "ptr", "%t5"});
  reg_block.insts.push_back(LirGepOp{"%t7", "i8", "%t6", false, {"i32 %t1"}});
  reg_block.terminator = LirBr{"join"};

  LirBlock stack_block;
  stack_block.id = LirBlockId{3};
  stack_block.label = "stack";
  stack_block.insts.push_back(
      LirGepOp{"%t8", "%struct.__va_list_tag_", "%lv.ap", false, {"i32 0", "i32 0"}});
  stack_block.insts.push_back(LirLoadOp{"%t9", "ptr", "%t8"});
  stack_block.insts.push_back(LirGepOp{"%t10", "i8", "%t9", false, {"i64 8"}});
  stack_block.insts.push_back(LirStoreOp{"ptr", "%t10", "%t8"});
  stack_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{4};
  join_block.label = "join";
  join_block.insts.push_back(
      LirPhiOp{"%t11", "ptr", {{"%t7", "reg"}, {"%t9", "stack"}}});
  join_block.insts.push_back(LirLoadOp{"%t12", "ptr", "%t11"});
  join_block.insts.push_back(LirMemcpyOp{"%lv.bigints.tmp", "%t12", "20", false});
  join_block.insts.push_back(LirVaEndOp{"%lv.ap"});
  join_block.terminator = LirRet{std::string("%p.seed"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(reg_try_block));
  function.blocks.push_back(std::move(reg_block));
  function.blocks.push_back(std::move(stack_block));
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_phi_join_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "slt", "i32", "1", "2"});
  entry.terminator = LirCondBr{"%t0", "reg", "stack"};

  LirBlock reg_block;
  reg_block.id = LirBlockId{1};
  reg_block.label = "reg";
  reg_block.terminator = LirBr{"join"};

  LirBlock stack_block;
  stack_block.id = LirBlockId{2};
  stack_block.label = "stack";
  stack_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(LirPhiOp{"%t1", "ptr", {{"%reg.addr", "reg"}, {"%stack.addr", "stack"}}});
  join_block.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(reg_block));
  function.blocks.push_back(std::move(stack_block));
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

void test_adapts_direct_return() {
  const auto adapted = c4c::backend::adapt_minimal_module(make_return_zero_module());
  expect_true(adapted.functions.size() == 1, "adapter should preserve one function");
  expect_true(adapted.functions.front().blocks.size() == 1,
              "adapter should preserve one block");
  expect_true(!adapted.functions.front().blocks.front().terminator.value->compare("0"),
              "adapter should preserve the return literal");
}

void test_renders_return_add() {
  const auto adapted = c4c::backend::adapt_minimal_module(make_return_add_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "%t0 = add i32 2, 3",
                  "adapter renderer should emit the add instruction");
  expect_contains(rendered, "ret i32 %t0",
                  "adapter renderer should emit the adapted return");
}

void test_adapter_normalizes_typed_direct_call_helper_slice() {
  const auto adapted = c4c::backend::adapt_minimal_module(make_typed_direct_call_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_one(i32 %p.x)",
                  "adapter should preserve the single-argument helper signature");
  expect_contains(rendered, "%t1 = add i32 %p.x, 1",
                  "adapter should normalize the helper slot pattern into a backend-owned add");
  expect_contains(rendered, "ret i32 %t1",
                  "adapter should return the normalized helper add result directly");
  expect_contains(rendered, "call i32 (i32) @add_one(i32 5)",
                  "adapter should preserve the typed direct-call site");
}

void test_adapter_preserves_typed_two_arg_direct_call_helper_slice() {
  const auto adapted =
      c4c::backend::adapt_minimal_module(make_typed_direct_call_two_arg_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_pair(i32 %p.x, i32 %p.y)",
                  "adapter should preserve the two-argument helper signature");
  expect_contains(rendered, "%t0 = add i32 %p.x, %p.y",
                  "adapter should preserve the register-only two-argument helper add");
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should preserve the typed two-argument direct-call site");
}

void test_adapter_normalizes_typed_direct_call_local_arg_slice() {
  const auto adapted =
      c4c::backend::adapt_minimal_module(make_typed_direct_call_local_arg_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_one(i32 %p.x)",
                  "adapter should preserve the local-argument helper signature");
  expect_contains(rendered, "%t1 = add i32 %p.x, 1",
                  "adapter should still normalize the helper slot pattern");
  expect_contains(rendered, "call i32 (i32) @add_one(i32 5)",
                  "adapter should normalize the local slot direct-call argument into the backend slice");
}

void test_adapter_normalizes_typed_two_arg_direct_call_local_arg_slice() {
  const auto adapted =
      c4c::backend::adapt_minimal_module(make_typed_direct_call_two_arg_local_arg_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_pair(i32 %p.x, i32 %p.y)",
                  "adapter should preserve the two-argument local-argument helper signature");
  expect_contains(rendered, "%t0 = add i32 %p.x, %p.y",
                  "adapter should still preserve the two-argument helper add");
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize the local slot first argument into the backend-owned two-argument slice");
}

void test_adapter_normalizes_typed_two_arg_direct_call_second_local_arg_slice() {
  const auto adapted = c4c::backend::adapt_minimal_module(
      make_typed_direct_call_two_arg_second_local_arg_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_pair(i32 %p.x, i32 %p.y)",
                  "adapter should preserve the two-argument second-local helper signature");
  expect_contains(rendered, "%t0 = add i32 %p.x, %p.y",
                  "adapter should still preserve the two-argument helper add");
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize the local slot second argument into the backend-owned two-argument slice");
}

void test_adapter_normalizes_typed_two_arg_direct_call_second_local_rewrite_slice() {
  const auto adapted = c4c::backend::adapt_minimal_module(
      make_typed_direct_call_two_arg_second_local_rewrite_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_pair(i32 %p.x, i32 %p.y)",
                  "adapter should preserve the rewritten second-local helper signature");
  expect_contains(rendered, "%t0 = add i32 %p.x, %p.y",
                  "adapter should still preserve the two-argument helper add");
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize the rewritten second-local slot into the backend-owned two-argument slice");
}

void test_adapter_normalizes_typed_two_arg_direct_call_first_local_rewrite_slice() {
  const auto adapted = c4c::backend::adapt_minimal_module(
      make_typed_direct_call_two_arg_first_local_rewrite_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_pair(i32 %p.x, i32 %p.y)",
                  "adapter should preserve the rewritten first-local helper signature");
  expect_contains(rendered, "%t0 = add i32 %p.x, %p.y",
                  "adapter should still preserve the two-argument helper add");
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize the rewritten first-local slot into the backend-owned two-argument slice");
}

void test_adapter_normalizes_typed_two_arg_direct_call_both_local_arg_slice() {
  const auto adapted =
      c4c::backend::adapt_minimal_module(make_typed_direct_call_two_arg_both_local_arg_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_pair(i32 %p.x, i32 %p.y)",
                  "adapter should preserve the two-argument both-local helper signature");
  expect_contains(rendered, "%t0 = add i32 %p.x, %p.y",
                  "adapter should still preserve the two-argument helper add");
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize both local slot arguments into the backend-owned two-argument slice");
}

void test_adapter_normalizes_typed_two_arg_direct_call_both_local_first_rewrite_slice() {
  const auto adapted = c4c::backend::adapt_minimal_module(
      make_typed_direct_call_two_arg_both_local_first_rewrite_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_pair(i32 %p.x, i32 %p.y)",
                  "adapter should preserve the rewritten both-local helper signature");
  expect_contains(rendered, "%t0 = add i32 %p.x, %p.y",
                  "adapter should still preserve the rewritten both-local helper add");
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize the rewritten first local plus second local caller into the backend-owned two-argument slice");
}

void test_adapter_normalizes_typed_two_arg_direct_call_both_local_second_rewrite_slice() {
  const auto adapted = c4c::backend::adapt_minimal_module(
      make_typed_direct_call_two_arg_both_local_second_rewrite_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_pair(i32 %p.x, i32 %p.y)",
                  "adapter should preserve the rewritten both-local helper signature");
  expect_contains(rendered, "%t0 = add i32 %p.x, %p.y",
                  "adapter should still preserve the rewritten both-local helper add");
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize the preserved first local plus rewritten second local caller into the backend-owned two-argument slice");
}

void test_adapter_normalizes_typed_two_arg_direct_call_both_local_double_rewrite_slice() {
  const auto adapted = c4c::backend::adapt_minimal_module(
      make_typed_direct_call_two_arg_both_local_double_rewrite_module());
  const auto rendered = c4c::backend::render_module(adapted);
  expect_contains(rendered, "define i32 @add_pair(i32 %p.x, i32 %p.y)",
                  "adapter should preserve the double-rewritten both-local helper signature");
  expect_contains(rendered, "%t0 = add i32 %p.x, %p.y",
                  "adapter should still preserve the double-rewritten both-local helper add");
  expect_contains(rendered, "call i32 (i32, i32) @add_pair(i32 5, i32 7)",
                  "adapter should normalize the double-rewritten both-local caller into the backend-owned two-argument slice");
}

void test_adapter_tracks_structured_signature_contract() {
  const auto adapted = c4c::backend::adapt_minimal_module(make_return_zero_module());
  const auto& signature = adapted.functions.front().signature;
  expect_true(signature.linkage == "define",
              "adapter should preserve whether a function is defined or declared");
  expect_true(signature.return_type == "i32",
              "adapter should preserve the function return type separately from the name");
  expect_true(signature.name == "main",
              "adapter should preserve the function name separately from the signature text");
  expect_true(signature.params.empty(),
              "adapter should preserve the empty parameter list for the minimal return-only slice");
  expect_true(!signature.is_vararg,
              "adapter should keep the minimal return-only slice non-variadic");
}

void test_adapter_tracks_structured_entry_block_and_return_contract() {
  const auto adapted = c4c::backend::adapt_minimal_module(make_return_add_module());
  const auto& function = adapted.functions.front();
  expect_true(function.blocks.size() == 1,
              "adapter should preserve the single-block bring-up slice");
  const auto& block = function.blocks.front();
  expect_true(block.label == "entry",
              "adapter should preserve the entry block label separately from rendering");
  expect_true(block.insts.size() == 1,
              "adapter should preserve the current single add instruction");
  const auto* add = std::get_if<c4c::backend::BackendBinaryInst>(&block.insts.front());
  expect_true(add != nullptr,
              "adapter should materialize the return-add instruction as a backend-owned op");
  expect_true(add->opcode == c4c::backend::BackendBinaryOpcode::Add,
              "adapter should preserve the add opcode in backend-owned form");
  expect_true(add->result == "%t0",
              "adapter should preserve the add result name");
  expect_true(add->type_str == "i32",
              "adapter should preserve the add type");
  expect_true(add->lhs == "2" && add->rhs == "3",
              "adapter should preserve the add operands");
  expect_true(block.terminator.value.has_value() && *block.terminator.value == "%t0",
              "adapter should preserve the return value separately from the block text");
  expect_true(block.terminator.type_str == "i32",
              "adapter should preserve the return type separately from the block text");
}

void test_rejects_unsupported_instruction() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(LirLoadOp{"%t0", "i32", "%ptr"});
  block.terminator = LirRet{std::string("%t0"), "i32"};

  try {
    (void)c4c::backend::adapt_minimal_module(module);
    fail("adapter should reject unsupported instructions");
  } catch (const c4c::backend::LirAdapterError& ex) {
    expect_contains(ex.what(), "non-binary/non-call instructions",
                    "adapter should explain unsupported instructions");
    expect_true(ex.kind() == c4c::backend::LirAdapterErrorKind::Unsupported,
                "adapter should classify unsupported instructions distinctly from malformed input");
  }
}

void test_aarch64_backend_scaffold_renders_supported_slice() {
  auto module = make_return_add_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 scaffold should emit a global entry symbol for the minimal asm slice");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 scaffold should materialize the folded return-add result in w0");
  expect_contains(rendered, "ret",
                  "aarch64 scaffold should terminate the minimal asm slice with ret");
}

void test_aarch64_backend_scaffold_renders_direct_return_immediate_slice() {
  auto module = make_return_zero_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 scaffold should emit a global entry symbol for direct return immediates");
  expect_contains(rendered, "mov w0, #0",
                  "aarch64 scaffold should materialize direct return immediates in w0");
  expect_contains(rendered, "ret",
                  "aarch64 scaffold should terminate direct return immediates with ret");
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

void test_aarch64_backend_propagates_malformed_signature_in_supported_slice() {
  auto module = make_return_add_module();
  module.functions.front().signature_text = "define @main()\n";

  try {
    (void)c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{module},
        c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
    fail("aarch64 backend should not hide malformed supported-slice signatures behind fallback");
  } catch (const c4c::backend::LirAdapterError& ex) {
    expect_true(ex.kind() == c4c::backend::LirAdapterErrorKind::Malformed,
                "aarch64 backend should preserve malformed-signature classification");
    expect_contains(ex.what(), "could not parse signature",
                    "aarch64 backend should surface the malformed signature detail");
  }
}

void test_aarch64_backend_renders_compare_and_branch_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #2\n",
                  "aarch64 backend should materialize the first compare immediate");
  expect_contains(rendered, "  cmp w8, #3\n",
                  "aarch64 backend should compare the materialized value against the second immediate");
  expect_contains(rendered, "  b.ge .Lelse\n",
                  "aarch64 backend should branch to the else label when the signed less-than test fails");
  expect_contains(rendered, ".Lthen:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the then return block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should lower the else return block directly in assembly");
}

void test_aarch64_backend_renders_compare_and_branch_le_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_le_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the signed less-or-equal conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #2\n",
                  "aarch64 backend should materialize the first signed less-or-equal compare immediate");
  expect_contains(rendered, "  cmp w8, #3\n",
                  "aarch64 backend should compare the materialized less-or-equal lhs against the rhs immediate");
  expect_contains(rendered, "  b.gt .Lelse\n",
                  "aarch64 backend should branch to the else label when the signed less-or-equal test fails");
  expect_contains(rendered, ".Lthen:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the signed less-or-equal then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should lower the signed less-or-equal else block directly in assembly");
}

void test_aarch64_backend_renders_compare_and_branch_gt_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_gt_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the signed greater-than conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #3\n",
                  "aarch64 backend should materialize the first signed greater-than compare immediate");
  expect_contains(rendered, "  cmp w8, #2\n",
                  "aarch64 backend should compare the materialized greater-than lhs against the rhs immediate");
  expect_contains(rendered, "  b.le .Lelse\n",
                  "aarch64 backend should branch to the else label when the signed greater-than test fails");
  expect_contains(rendered, ".Lthen:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the signed greater-than then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should lower the signed greater-than else block directly in assembly");
}

void test_aarch64_backend_renders_compare_and_branch_ge_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_ge_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the signed greater-or-equal conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #3\n",
                  "aarch64 backend should materialize the first signed greater-or-equal compare immediate");
  expect_contains(rendered, "  cmp w8, #2\n",
                  "aarch64 backend should compare the materialized greater-or-equal lhs against the rhs immediate");
  expect_contains(rendered, "  b.lt .Lelse\n",
                  "aarch64 backend should branch to the else label when the signed greater-or-equal test fails");
  expect_contains(rendered, ".Lthen:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the signed greater-or-equal then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should lower the signed greater-or-equal else block directly in assembly");
}

void test_aarch64_backend_scaffold_rejects_out_of_slice_ir() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  auto& block = module.functions.front().blocks.front();
  block.insts.push_back(LirSelectOp{"%t0", "i32", "true", "1", "0"});

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
  expect_contains(rendered, ".type helper, %function",
                  "aarch64 backend should lower the helper definition into a real function symbol");
  expect_contains(rendered, "helper:\n  mov w0, #7\n  ret\n",
                  "aarch64 backend should emit the minimal helper body as assembly");
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should still publish main as the entry symbol");
  expect_contains(rendered, "sub sp, sp, #16",
                  "aarch64 backend should preserve the link register before a helper call");
  expect_contains(rendered, "str x30, [sp, #8]",
                  "aarch64 backend should spill x30 in the minimal helper-call frame");
  expect_contains(rendered, "bl helper",
                  "aarch64 backend should lower the supported direct call slice with bl");
  expect_contains(rendered, "ldr x30, [sp, #8]",
                  "aarch64 backend should restore x30 after the helper call");
  expect_contains(rendered, "add sp, sp, #16",
                  "aarch64 backend should tear down the minimal helper-call frame");
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
  expect_contains(rendered, ".type add_one, %function",
                  "aarch64 backend should lower the single-argument helper into a real function symbol");
  expect_contains(rendered, "add_one:\n  add w0, w0, #1\n  ret\n",
                  "aarch64 backend should lower the normalized helper add into register-based assembly");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should materialize the direct-call argument in w0 before bl");
  expect_contains(rendered, "bl add_one",
                  "aarch64 backend should lower the typed direct call with bl");
}

void test_aarch64_backend_renders_typed_two_arg_direct_call_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_typed_direct_call_two_arg_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend should lower the two-argument helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  add w0, w0, w1\n  ret\n",
                  "aarch64 backend should lower the register-only two-argument helper add");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should materialize the first call argument in w0 before bl");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend should materialize the second call argument in w1 before bl");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend should lower the typed two-argument direct call with bl");
}

void test_aarch64_backend_renders_typed_direct_call_local_arg_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_typed_direct_call_local_arg_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_one, %function",
                  "aarch64 backend should lower the local-argument helper into a real function symbol");
  expect_contains(rendered, "add_one:\n  add w0, w0, #1\n  ret\n",
                  "aarch64 backend should keep the helper on the register-based add path");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should materialize the local direct-call argument in w0 before bl");
  expect_contains(rendered, "bl add_one",
                  "aarch64 backend should lower the local-argument direct call with bl");
}

void test_aarch64_backend_renders_typed_two_arg_direct_call_local_arg_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_typed_direct_call_two_arg_local_arg_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend should lower the two-argument local-argument helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  add w0, w0, w1\n  ret\n",
                  "aarch64 backend should keep the local-argument helper on the register-only add path");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should materialize the normalized local first argument in w0 before bl");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend should preserve the immediate second argument in w1 before bl");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend should lower the two-argument local-argument direct call with bl");
}

void test_aarch64_backend_renders_typed_two_arg_direct_call_second_local_arg_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_typed_direct_call_two_arg_second_local_arg_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend should lower the two-argument second-local helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  add w0, w0, w1\n  ret\n",
                  "aarch64 backend should keep the second-local helper on the register-only add path");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should preserve the immediate first argument in w0 before bl");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend should materialize the normalized local second argument in w1 before bl");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend should lower the two-argument second-local direct call with bl");
}

void test_aarch64_backend_renders_typed_two_arg_direct_call_second_local_rewrite_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_typed_direct_call_two_arg_second_local_rewrite_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend should lower the rewritten second-local helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  add w0, w0, w1\n  ret\n",
                  "aarch64 backend should keep the rewritten second-local helper on the register-only add path");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should preserve the immediate first argument in w0 before bl for the rewritten second-local slice");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend should materialize the rewritten second-local argument in w1 before bl");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend should lower the rewritten second-local direct call with bl");
}

void test_aarch64_backend_renders_typed_two_arg_direct_call_first_local_rewrite_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_typed_direct_call_two_arg_first_local_rewrite_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend should lower the rewritten first-local helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  add w0, w0, w1\n  ret\n",
                  "aarch64 backend should keep the rewritten first-local helper on the register-only add path");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should materialize the rewritten first-local argument in w0 before bl");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend should preserve the immediate second argument in w1 before bl for the rewritten first-local slice");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend should lower the rewritten first-local direct call with bl");
}

void test_aarch64_backend_renders_typed_two_arg_direct_call_both_local_arg_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_typed_direct_call_two_arg_both_local_arg_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend should lower the two-argument both-local helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  add w0, w0, w1\n  ret\n",
                  "aarch64 backend should keep the both-local helper on the register-only add path");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should materialize the normalized first local argument in w0 before bl");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend should materialize the normalized second local argument in w1 before bl");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend should lower the two-argument both-local direct call with bl");
}

void test_aarch64_backend_renders_typed_two_arg_direct_call_both_local_first_rewrite_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_typed_direct_call_two_arg_both_local_first_rewrite_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend should lower the rewritten both-local helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  add w0, w0, w1\n  ret\n",
                  "aarch64 backend should keep the rewritten both-local helper on the register-only add path");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should materialize the rewritten first local argument in w0 before bl");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend should materialize the preserved second local argument in w1 before bl");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend should lower the rewritten both-local direct call with bl");
}

void test_aarch64_backend_renders_typed_two_arg_direct_call_both_local_second_rewrite_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_typed_direct_call_two_arg_both_local_second_rewrite_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend should lower the rewritten both-local helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  add w0, w0, w1\n  ret\n",
                  "aarch64 backend should keep the rewritten both-local helper on the register-only add path");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should materialize the preserved first local argument in w0 before bl");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend should materialize the rewritten second local argument in w1 before bl");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend should lower the rewritten both-local second-slot direct call with bl");
}

void test_aarch64_backend_renders_typed_two_arg_direct_call_both_local_double_rewrite_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_typed_direct_call_two_arg_both_local_double_rewrite_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend should lower the double-rewritten both-local helper into a real function symbol");
  expect_contains(rendered, "add_pair:\n  add w0, w0, w1\n  ret\n",
                  "aarch64 backend should keep the double-rewritten both-local helper on the register-only add path");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should materialize the double-rewritten first local argument in w0 before bl");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend should materialize the double-rewritten second local argument in w1 before bl");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend should lower the double-rewritten both-local direct call with bl");
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

void test_aarch64_backend_renders_extern_global_load_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_extern_global_load_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "@ext_counter = external global i32, align 4",
                  "aarch64 backend should render extern global declarations");
  expect_contains(rendered, "define i32 @main()\n{\nentry:\n  %t0 = load i32, ptr @ext_counter\n",
                  "aarch64 backend should preserve scalar loads from extern globals");
  expect_contains(rendered, "ret i32 %t0",
                  "aarch64 backend should preserve the extern global load result");
}

void test_aarch64_backend_renders_extern_global_array_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_extern_global_array_load_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "@ext_arr = external global [2 x i32], align 4",
                  "aarch64 backend should render extern global array declarations");
  expect_contains(rendered, "%t0 = getelementptr [2 x i32], ptr @ext_arr, i64 0, i64 0",
                  "aarch64 backend should preserve extern global array decay through GEP");
  expect_contains(rendered, "%t1 = sext i32 1 to i64",
                  "aarch64 backend should preserve extern global array index widening");
  expect_contains(rendered, "%t2 = getelementptr i32, ptr %t0, i64 %t1",
                  "aarch64 backend should preserve indexed addressing into extern global arrays");
  expect_contains(rendered, "%t3 = load i32, ptr %t2",
                  "aarch64 backend should preserve loads through extern global array element pointers");
  expect_contains(rendered, "ret i32 %t3",
                  "aarch64 backend should preserve the extern global array load result");
}

void test_aarch64_backend_renders_global_char_pointer_diff_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_global_char_pointer_diff_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "@g_bytes = global [2 x i8] zeroinitializer",
                  "aarch64 backend should render global byte-array definitions");
  expect_contains(rendered, "%t0 = getelementptr [2 x i8], ptr @g_bytes, i64 0, i64 0",
                  "aarch64 backend should preserve byte-array decay from globals");
  expect_contains(rendered, "%t1 = getelementptr i8, ptr %t0, i64 1",
                  "aarch64 backend should preserve indexed byte-addressing from globals");
  expect_contains(rendered, "%t2 = ptrtoint ptr %t1 to i64",
                  "aarch64 backend should render ptrtoint for pointer-difference lowering");
  expect_contains(rendered, "%t3 = ptrtoint ptr %t0 to i64",
                  "aarch64 backend should render ptrtoint for the base global address");
  expect_contains(rendered, "%t4 = sub i64 %t2, %t3",
                  "aarch64 backend should preserve byte-granular pointer subtraction");
  expect_contains(rendered, "ret i32 %t6",
                  "aarch64 backend should preserve the pointer-difference comparison result");
}

void test_aarch64_backend_renders_global_int_pointer_diff_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_global_int_pointer_diff_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "@g_words = global [2 x i32] zeroinitializer, align 4",
                  "aarch64 backend should render global int-array definitions");
  expect_contains(rendered, "%t0 = getelementptr [2 x i32], ptr @g_words, i64 0, i64 0",
                  "aarch64 backend should preserve int-array decay from globals");
  expect_contains(rendered, "%t1 = getelementptr i32, ptr %t0, i64 1",
                  "aarch64 backend should preserve indexed int-pointer addressing from globals");
  expect_contains(rendered, "%t4 = sub i64 %t2, %t3",
                  "aarch64 backend should preserve byte-granular pointer subtraction before scaling");
  expect_contains(rendered, "%t5 = sdiv i64 %t4, 4",
                  "aarch64 backend should render the scaled pointer-difference divide");
  expect_contains(rendered, "ret i32 %t7",
                  "aarch64 backend should preserve the scaled pointer-difference comparison result");
}

void test_aarch64_backend_renders_global_int_pointer_roundtrip_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_global_int_pointer_roundtrip_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "@g_value = global i32 9, align 4",
                  "aarch64 backend should render the round-trip global definition");
  expect_contains(rendered, "%t0 = ptrtoint ptr @g_value to i64",
                  "aarch64 backend should render ptrtoint for address round-tripping");
  expect_contains(rendered, "%t1 = inttoptr i64 %t0 to ptr",
                  "aarch64 backend should render inttoptr for address round-tripping");
  expect_contains(rendered, "%t2 = load i32, ptr %t1",
                  "aarch64 backend should preserve loads through round-tripped pointers");
  expect_contains(rendered, "ret i32 %t2",
                  "aarch64 backend should preserve the round-tripped load result");
}

void test_aarch64_backend_renders_bitcast_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bitcast_scalar_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "%t0 = bitcast double 0.000000e+00 to i64",
                  "aarch64 backend should render bitcast within the target-local cast path");
  expect_contains(rendered, "ret i64 %t0",
                  "aarch64 backend should preserve the bitcast result");
}

void test_aarch64_backend_renders_trunc_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_trunc_scalar_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "%t0 = trunc i32 13124 to i16",
                  "aarch64 backend should render trunc within the target-local cast path");
  expect_contains(rendered, "ret i16 %t0",
                  "aarch64 backend should preserve the trunc result");
}

void test_aarch64_backend_renders_va_intrinsic_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_va_intrinsic_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "declare void @llvm.va_start.p0(ptr)",
                  "aarch64 backend should render llvm.va_start declarations");
  expect_contains(rendered, "declare void @llvm.va_end.p0(ptr)",
                  "aarch64 backend should render llvm.va_end declarations");
  expect_contains(rendered, "declare void @llvm.va_copy.p0.p0(ptr, ptr)",
                  "aarch64 backend should render llvm.va_copy declarations");
  expect_contains(rendered, "call void @llvm.va_start.p0(ptr %lv.ap)",
                  "aarch64 backend should render llvm.va_start calls");
  expect_contains(rendered, "call void @llvm.va_copy.p0.p0(ptr %lv.ap_copy, ptr %lv.ap)",
                  "aarch64 backend should render llvm.va_copy calls");
  expect_contains(rendered, "call void @llvm.va_end.p0(ptr %lv.ap_copy)",
                  "aarch64 backend should render llvm.va_end calls for copied lists");
  expect_contains(rendered, "call void @llvm.va_end.p0(ptr %lv.ap)",
                  "aarch64 backend should render llvm.va_end calls for the original list");
}

void test_aarch64_backend_renders_va_arg_scalar_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_va_arg_scalar_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "call void @llvm.va_start.p0(ptr %lv.ap)",
                  "aarch64 backend should preserve va_start before va_arg");
  expect_contains(rendered, "%t0 = va_arg ptr %lv.ap, i32",
                  "aarch64 backend should render scalar va_arg in the target-local memory path");
  expect_contains(rendered, "%t1 = add i32 %p.first, %t0",
                  "aarch64 backend should preserve arithmetic that consumes va_arg results");
  expect_contains(rendered, "call void @llvm.va_end.p0(ptr %lv.ap)",
                  "aarch64 backend should preserve va_end after va_arg");
  expect_contains(rendered, "ret i32 %t1",
                  "aarch64 backend should preserve the scalar va_arg result");
}

void test_aarch64_backend_renders_va_arg_pair_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_va_arg_pair_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)",
                  "aarch64 backend should render llvm.memcpy declarations for aggregate va_arg");
  expect_contains(rendered, "%t6 = phi ptr [ %t3, %reg ], [ %t5, %stack ]",
                  "aarch64 backend should preserve the aggregate va_arg helper phi join");
  expect_contains(rendered,
                  "call void @llvm.memcpy.p0.p0.i64(ptr %lv.pair.tmp, ptr %t6, i64 8, i1 false)",
                  "aarch64 backend should render aggregate va_arg copies through llvm.memcpy");
  expect_contains(rendered, "call void @llvm.va_end.p0(ptr %lv.ap)",
                  "aarch64 backend should preserve va_end after aggregate va_arg handling");
  expect_contains(rendered, "ret i32 %p.seed",
                  "aarch64 backend should preserve the enclosing function return");
}

void test_aarch64_backend_renders_va_arg_bigints_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_va_arg_bigints_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "%struct.BigInts = type { i32, i32, i32, i32, i32 }",
                  "aarch64 backend should preserve larger aggregate type declarations");
  expect_contains(rendered, "%t11 = phi ptr [ %t7, %reg ], [ %t9, %stack ]",
                  "aarch64 backend should preserve the indirect aggregate helper phi join");
  expect_contains(rendered, "%t12 = load ptr, ptr %t11",
                  "aarch64 backend should reload the indirect aggregate source pointer");
  expect_contains(rendered,
                  "call void @llvm.memcpy.p0.p0.i64(ptr %lv.bigints.tmp, ptr %t12, i64 20, i1 false)",
                  "aarch64 backend should render indirect aggregate va_arg copies through llvm.memcpy");
  expect_contains(rendered, "call void @llvm.va_end.p0(ptr %lv.ap)",
                  "aarch64 backend should preserve va_end after indirect aggregate va_arg handling");
}

void test_aarch64_backend_renders_phi_join_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_phi_join_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "br i1 %t0, label %reg, label %stack",
                  "aarch64 backend should preserve the helper control-flow split before a phi join");
  expect_contains(rendered, "%t1 = phi ptr [ %reg.addr, %reg ], [ %stack.addr, %stack ]",
                  "aarch64 backend should render pointer phi joins used by target-local va_arg helpers");
}

void test_aarch64_assembler_parser_stub_preserves_text() {
  const std::string asm_text = ".text\n.globl main\nmain:\n  ret\n";
  const auto statements = c4c::backend::aarch64::assembler::parse_asm(asm_text);
  expect_true(statements.size() == 1,
              "aarch64 assembler parser stub should keep returning a single placeholder statement");
  expect_true(statements.front().text == asm_text,
              "aarch64 assembler parser stub should preserve the raw assembly text");
}

void test_aarch64_assembler_elf_writer_branch_reloc_helper() {
  using c4c::backend::aarch64::assembler::is_branch_reloc_type;

  expect_true(is_branch_reloc_type(279),
              "aarch64 elf writer helper should treat R_AARCH64_CALL26 as a branch reloc");
  expect_true(is_branch_reloc_type(280),
              "aarch64 elf writer helper should treat R_AARCH64_JUMP26 as a branch reloc");
  expect_true(is_branch_reloc_type(282),
              "aarch64 elf writer helper should treat R_AARCH64_CONDBR19 as a branch reloc");
  expect_true(is_branch_reloc_type(283),
              "aarch64 elf writer helper should treat R_AARCH64_TSTBR14 as a branch reloc");
  expect_true(!is_branch_reloc_type(257),
              "aarch64 elf writer helper should keep non-branch relocations out of the branch-only set");
}

void test_aarch64_assembler_encoder_helper_smoke() {
  namespace encoder = c4c::backend::aarch64::assembler::encoder;

  expect_true(encoder::is_64bit_reg("x9"),
              "aarch64 encoder helper should recognize x-register names as 64-bit GPRs");
  expect_true(!encoder::is_64bit_reg("w9"),
              "aarch64 encoder helper should reject w-register names as 64-bit GPRs");
  expect_true(encoder::is_32bit_reg("w3"),
              "aarch64 encoder helper should recognize w-register names as 32-bit GPRs");
  expect_true(!encoder::is_32bit_reg("x3"),
              "aarch64 encoder helper should reject x-register names as 32-bit GPRs");
  expect_true(encoder::is_fp_reg("d0") && encoder::is_fp_reg("s1") &&
                  encoder::is_fp_reg("q2") && encoder::is_fp_reg("v3"),
              "aarch64 encoder helper should recognize the current FP/SIMD register prefixes");
  expect_true(!encoder::is_fp_reg("x0"),
              "aarch64 encoder helper should keep integer register names out of the FP/SIMD set");
  expect_true(encoder::sf_bit(true) == 1u && encoder::sf_bit(false) == 0u,
              "aarch64 encoder helper should map the sf bit directly from operand width");
}

}  // namespace

int main() {
  test_adapts_direct_return();
  test_renders_return_add();
  test_adapter_normalizes_typed_direct_call_helper_slice();
  test_adapter_preserves_typed_two_arg_direct_call_helper_slice();
  test_adapter_normalizes_typed_direct_call_local_arg_slice();
  test_adapter_normalizes_typed_two_arg_direct_call_local_arg_slice();
  test_adapter_normalizes_typed_two_arg_direct_call_second_local_arg_slice();
  test_adapter_normalizes_typed_two_arg_direct_call_second_local_rewrite_slice();
  test_adapter_normalizes_typed_two_arg_direct_call_first_local_rewrite_slice();
  test_adapter_normalizes_typed_two_arg_direct_call_both_local_arg_slice();
  test_adapter_normalizes_typed_two_arg_direct_call_both_local_first_rewrite_slice();
  test_adapter_normalizes_typed_two_arg_direct_call_both_local_second_rewrite_slice();
  test_adapter_normalizes_typed_two_arg_direct_call_both_local_double_rewrite_slice();
  test_adapter_tracks_structured_signature_contract();
  test_adapter_tracks_structured_entry_block_and_return_contract();
  test_rejects_unsupported_instruction();
  test_aarch64_backend_scaffold_renders_supported_slice();
  test_aarch64_backend_scaffold_renders_direct_return_immediate_slice();
  test_aarch64_backend_renders_void_return_slice();
  test_aarch64_backend_preserves_module_headers_and_declarations();
  test_aarch64_backend_propagates_malformed_signature_in_supported_slice();
  test_aarch64_backend_renders_compare_and_branch_slice();
  test_aarch64_backend_renders_compare_and_branch_le_slice();
  test_aarch64_backend_renders_compare_and_branch_gt_slice();
  test_aarch64_backend_renders_compare_and_branch_ge_slice();
  test_aarch64_backend_scaffold_rejects_out_of_slice_ir();
  test_aarch64_backend_renders_direct_call_slice();
  test_aarch64_backend_renders_local_temp_memory_slice();
  test_aarch64_backend_renders_param_slot_memory_slice();
  test_aarch64_backend_renders_typed_direct_call_slice();
  test_aarch64_backend_renders_typed_two_arg_direct_call_slice();
  test_aarch64_backend_renders_typed_direct_call_local_arg_slice();
  test_aarch64_backend_renders_typed_two_arg_direct_call_local_arg_slice();
  test_aarch64_backend_renders_typed_two_arg_direct_call_second_local_arg_slice();
  test_aarch64_backend_renders_typed_two_arg_direct_call_second_local_rewrite_slice();
  test_aarch64_backend_renders_typed_two_arg_direct_call_first_local_rewrite_slice();
  test_aarch64_backend_renders_typed_two_arg_direct_call_both_local_arg_slice();
  test_aarch64_backend_renders_typed_two_arg_direct_call_both_local_first_rewrite_slice();
  test_aarch64_backend_renders_typed_two_arg_direct_call_both_local_second_rewrite_slice();
  test_aarch64_backend_renders_typed_two_arg_direct_call_both_local_double_rewrite_slice();
  test_aarch64_backend_renders_local_array_gep_slice();
  test_aarch64_backend_renders_param_member_array_gep_slice();
  test_aarch64_backend_renders_nested_member_pointer_array_gep_slice();
  test_aarch64_backend_renders_nested_param_member_array_gep_slice();
  test_aarch64_backend_renders_global_definition_slice();
  test_aarch64_backend_renders_string_pool_slice();
  test_aarch64_backend_renders_extern_decl_slice();
  test_aarch64_backend_renders_extern_global_load_slice();
  test_aarch64_backend_renders_extern_global_array_slice();
  test_aarch64_backend_renders_global_char_pointer_diff_slice();
  test_aarch64_backend_renders_global_int_pointer_diff_slice();
  test_aarch64_backend_renders_global_int_pointer_roundtrip_slice();
  test_aarch64_backend_renders_bitcast_slice();
  test_aarch64_backend_renders_trunc_slice();
  test_aarch64_backend_renders_va_intrinsic_slice();
  test_aarch64_backend_renders_va_arg_scalar_slice();
  test_aarch64_backend_renders_va_arg_pair_slice();
  test_aarch64_backend_renders_va_arg_bigints_slice();
  test_aarch64_backend_renders_phi_join_slice();
  test_aarch64_assembler_parser_stub_preserves_text();
  test_aarch64_assembler_elf_writer_branch_reloc_helper();
  test_aarch64_assembler_encoder_helper_smoke();
  return 0;
}
