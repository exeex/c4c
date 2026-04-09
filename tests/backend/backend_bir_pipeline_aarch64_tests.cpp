#include "backend_bir_test_support.hpp"

#include "../../src/backend/lowering/lir_to_bir.hpp"
#include "../../src/backend/aarch64/codegen/emit.hpp"

#include <stdexcept>

namespace {

c4c::codegen::lir::LirModule make_lir_minimal_global_char_pointer_diff_module() {
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
  entry.insts.push_back(LirGepOp{"%t0", "[2 x i8]", "@g_bytes", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t2", "i8", "%t0", false, {"i64 %t1"}});
  entry.insts.push_back(LirGepOp{"%t3", "[2 x i8]", "@g_bytes", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t4", LirCastKind::SExt, "i32", "0", "i64"});
  entry.insts.push_back(LirGepOp{"%t5", "i8", "%t3", false, {"i64 %t4"}});
  entry.insts.push_back(LirCastOp{"%t6", LirCastKind::PtrToInt, "ptr", "%t2", "i64"});
  entry.insts.push_back(LirCastOp{"%t7", LirCastKind::PtrToInt, "ptr", "%t5", "i64"});
  entry.insts.push_back(LirBinOp{"%t8", "sub", "i64", "%t6", "%t7"});
  entry.insts.push_back(LirCastOp{"%t9", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirCmpOp{"%t10", false, "eq", "i64", "%t8", "%t9"});
  entry.insts.push_back(LirCastOp{"%t11", LirCastKind::ZExt, "i1", "%t10", "i32"});
  entry.terminator = LirRet{std::string("%t11"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_unsupported_double_return_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define double @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.terminator = LirRet{std::string("0.0"), "double"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_goto_only_constant_return_module() {
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
  entry.terminator = LirBr{"ulbl_start"};

  LirBlock start;
  start.id = LirBlockId{1};
  start.label = "ulbl_start";
  start.terminator = LirBr{"block_1"};

  LirBlock block_1;
  block_1.id = LirBlockId{2};
  block_1.label = "block_1";
  block_1.terminator = LirBr{"ulbl_next"};

  LirBlock next;
  next.id = LirBlockId{3};
  next.label = "ulbl_next";
  next.terminator = LirBr{"done"};

  LirBlock done;
  done.id = LirBlockId{4};
  done.label = "done";
  done.terminator = LirRet{std::string("9"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(start));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(next));
  function.blocks.push_back(std::move(done));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_conditional_return_module() {
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
  entry.insts.push_back(LirCmpOp{"%t0", false, "slt", "i32", "2", "3"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
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
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_conditional_return_with_empty_bridge_blocks_module() {
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
  entry.insts.push_back(LirCmpOp{"%t0", false, "slt", "i32", "2", "3"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "then.bridge", "else.bridge"};

  LirBlock then_bridge;
  then_bridge.id = LirBlockId{1};
  then_bridge.label = "then.bridge";
  then_bridge.terminator = LirBr{"then.ret"};

  LirBlock then_ret;
  then_ret.id = LirBlockId{2};
  then_ret.label = "then.ret";
  then_ret.terminator = LirRet{std::string("0"), "i32"};

  LirBlock else_bridge;
  else_bridge.id = LirBlockId{3};
  else_bridge.label = "else.bridge";
  else_bridge.terminator = LirBr{"else.ret"};

  LirBlock else_ret;
  else_ret.id = LirBlockId{4};
  else_ret.label = "else.ret";
  else_ret.terminator = LirRet{std::string("1"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_bridge));
  function.blocks.push_back(std::move(then_ret));
  function.blocks.push_back(std::move(else_bridge));
  function.blocks.push_back(std::move(else_ret));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_lir_minimal_conditional_return_with_asymmetric_empty_bridge_module() {
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
  entry.insts.push_back(LirCmpOp{"%t0", false, "slt", "i32", "2", "3"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "then.bridge", "else.ret"};

  LirBlock then_bridge;
  then_bridge.id = LirBlockId{1};
  then_bridge.label = "then.bridge";
  then_bridge.terminator = LirBr{"then.ret"};

  LirBlock then_ret;
  then_ret.id = LirBlockId{2};
  then_ret.label = "then.ret";
  then_ret.terminator = LirRet{std::string("0"), "i32"};

  LirBlock else_ret;
  else_ret.id = LirBlockId{3};
  else_ret.label = "else.ret";
  else_ret.terminator = LirRet{std::string("1"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_bridge));
  function.blocks.push_back(std::move(then_ret));
  function.blocks.push_back(std::move(else_ret));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_conditional_return_with_double_empty_bridge_chain_module() {
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
  entry.insts.push_back(LirCmpOp{"%t0", false, "slt", "i32", "2", "3"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "then.bridge0", "else.ret"};

  LirBlock then_bridge0;
  then_bridge0.id = LirBlockId{1};
  then_bridge0.label = "then.bridge0";
  then_bridge0.terminator = LirBr{"then.bridge1"};

  LirBlock then_bridge1;
  then_bridge1.id = LirBlockId{2};
  then_bridge1.label = "then.bridge1";
  then_bridge1.terminator = LirBr{"then.ret"};

  LirBlock then_ret;
  then_ret.id = LirBlockId{3};
  then_ret.label = "then.ret";
  then_ret.terminator = LirRet{std::string("0"), "i32"};

  LirBlock else_ret;
  else_ret.id = LirBlockId{4};
  else_ret.label = "else.ret";
  else_ret.terminator = LirRet{std::string("1"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_bridge0));
  function.blocks.push_back(std::move(then_bridge1));
  function.blocks.push_back(std::move(then_ret));
  function.blocks.push_back(std::move(else_ret));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_lir_minimal_conditional_return_with_mirrored_false_double_empty_bridge_chain_module() {
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
  entry.insts.push_back(LirCmpOp{"%t0", false, "slt", "i32", "2", "3"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "then.ret", "else.bridge0"};

  LirBlock else_bridge0;
  else_bridge0.id = LirBlockId{1};
  else_bridge0.label = "else.bridge0";
  else_bridge0.terminator = LirBr{"else.bridge1"};

  LirBlock else_bridge1;
  else_bridge1.id = LirBlockId{2};
  else_bridge1.label = "else.bridge1";
  else_bridge1.terminator = LirBr{"else.ret"};

  LirBlock else_ret;
  else_ret.id = LirBlockId{3};
  else_ret.label = "else.ret";
  else_ret.terminator = LirRet{std::string("1"), "i32"};

  LirBlock then_ret;
  then_ret.id = LirBlockId{4};
  then_ret.label = "then.ret";
  then_ret.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(else_bridge0));
  function.blocks.push_back(std::move(else_bridge1));
  function.blocks.push_back(std::move(else_ret));
  function.blocks.push_back(std::move(then_ret));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_lir_minimal_conditional_return_with_interleaved_double_empty_bridge_chains_module() {
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
  entry.insts.push_back(LirCmpOp{"%t0", false, "slt", "i32", "2", "3"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "then.bridge0", "else.bridge0"};

  LirBlock then_bridge0;
  then_bridge0.id = LirBlockId{1};
  then_bridge0.label = "then.bridge0";
  then_bridge0.terminator = LirBr{"then.bridge1"};

  LirBlock else_bridge0;
  else_bridge0.id = LirBlockId{2};
  else_bridge0.label = "else.bridge0";
  else_bridge0.terminator = LirBr{"else.bridge1"};

  LirBlock then_bridge1;
  then_bridge1.id = LirBlockId{3};
  then_bridge1.label = "then.bridge1";
  then_bridge1.terminator = LirBr{"then.ret"};

  LirBlock else_bridge1;
  else_bridge1.id = LirBlockId{4};
  else_bridge1.label = "else.bridge1";
  else_bridge1.terminator = LirBr{"else.ret"};

  LirBlock else_ret;
  else_ret.id = LirBlockId{5};
  else_ret.label = "else.ret";
  else_ret.terminator = LirRet{std::string("1"), "i32"};

  LirBlock then_ret;
  then_ret.id = LirBlockId{6};
  then_ret.label = "then.ret";
  then_ret.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_bridge0));
  function.blocks.push_back(std::move(else_bridge0));
  function.blocks.push_back(std::move(then_bridge1));
  function.blocks.push_back(std::move(else_bridge1));
  function.blocks.push_back(std::move(else_ret));
  function.blocks.push_back(std::move(then_ret));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_lir_param_conditional_return_with_interleaved_mixed_depth_bridge_chains_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "choose";
  function.signature_text = "define i32 @choose(i32 %lhs, i32 %rhs)\n";
  function.entry = LirBlockId{0};
  function.return_type.base = c4c::TB_INT;

  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%lhs", param_type});
  function.params.push_back({"%rhs", param_type});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "slt", "i32", "%lhs", "%rhs"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "then.bridge0", "else.bridge0"};

  LirBlock then_bridge0;
  then_bridge0.id = LirBlockId{1};
  then_bridge0.label = "then.bridge0";
  then_bridge0.terminator = LirBr{"then.bridge1"};

  LirBlock else_bridge0;
  else_bridge0.id = LirBlockId{2};
  else_bridge0.label = "else.bridge0";
  else_bridge0.terminator = LirBr{"else.ret"};

  LirBlock then_bridge1;
  then_bridge1.id = LirBlockId{3};
  then_bridge1.label = "then.bridge1";
  then_bridge1.terminator = LirBr{"then.bridge2"};

  LirBlock then_bridge2;
  then_bridge2.id = LirBlockId{4};
  then_bridge2.label = "then.bridge2";
  then_bridge2.terminator = LirBr{"then.ret"};

  LirBlock else_ret;
  else_ret.id = LirBlockId{5};
  else_ret.label = "else.ret";
  else_ret.terminator = LirRet{std::string("%rhs"), "i32"};

  LirBlock then_ret;
  then_ret.id = LirBlockId{6};
  then_ret.label = "then.ret";
  then_ret.terminator = LirRet{std::string("%lhs"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_bridge0));
  function.blocks.push_back(std::move(else_bridge0));
  function.blocks.push_back(std::move(then_bridge1));
  function.blocks.push_back(std::move(then_bridge2));
  function.blocks.push_back(std::move(else_ret));
  function.blocks.push_back(std::move(then_ret));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_double_indirect_local_pointer_conditional_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.p", "ptr", "", 8});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.pp", "ptr", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.x"});
  entry.insts.push_back(LirStoreOp{"ptr", "%lv.x", "%lv.p"});
  entry.insts.push_back(LirStoreOp{"ptr", "%lv.p", "%lv.pp"});
  entry.insts.push_back(LirLoadOp{"%t0", "ptr", "%lv.p"});
  entry.insts.push_back(LirLoadOp{"%t1", "i32", "%t0"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "block_1", "block_2"};

  LirBlock block_1;
  block_1.id = LirBlockId{1};
  block_1.label = "block_1";
  block_1.terminator = LirRet{std::string("1"), "i32"};

  LirBlock block_2;
  block_2.id = LirBlockId{2};
  block_2.label = "block_2";
  block_2.insts.push_back(LirLoadOp{"%t3", "ptr", "%lv.pp"});
  block_2.insts.push_back(LirLoadOp{"%t4", "ptr", "%t3"});
  block_2.insts.push_back(LirLoadOp{"%t5", "i32", "%t4"});
  block_2.insts.push_back(LirCmpOp{"%t6", false, "ne", "i32", "%t5", "0"});
  block_2.terminator = LirCondBr{"%t6", "block_3", "block_4"};

  LirBlock block_3;
  block_3.id = LirBlockId{3};
  block_3.label = "block_3";
  block_3.terminator = LirRet{std::string("1"), "i32"};

  LirBlock block_4;
  block_4.id = LirBlockId{4};
  block_4.label = "block_4";
  block_4.insts.push_back(LirLoadOp{"%t7", "ptr", "%lv.pp"});
  block_4.insts.push_back(LirLoadOp{"%t8", "ptr", "%t7"});
  block_4.insts.push_back(LirStoreOp{"i32", "1", "%t8"});
  block_4.terminator = LirBr{"block_5"};

  LirBlock block_5;
  block_5.id = LirBlockId{5};
  block_5.label = "block_5";
  block_5.insts.push_back(LirLoadOp{"%t9", "i32", "%lv.x"});
  block_5.insts.push_back(LirCmpOp{"%t10", false, "ne", "i32", "%t9", "0"});
  block_5.terminator = LirCondBr{"%t10", "block_6", "block_7"};

  LirBlock block_6;
  block_6.id = LirBlockId{6};
  block_6.label = "block_6";
  block_6.terminator = LirRet{std::string("0"), "i32"};

  LirBlock block_7;
  block_7.id = LirBlockId{7};
  block_7.label = "block_7";
  block_7.terminator = LirRet{std::string("1"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  function.blocks.push_back(std::move(block_3));
  function.blocks.push_back(std::move(block_4));
  function.blocks.push_back(std::move(block_5));
  function.blocks.push_back(std::move(block_6));
  function.blocks.push_back(std::move(block_7));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_alloca_backed_switch_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.sel", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "2", "%lv.sel"});
  entry.insts.push_back(LirLoadOp{"%sel", "i32", "%lv.sel"});
  entry.terminator = LirSwitch{"%sel", "i32", "default", {{1, "case_one"}, {2, "case_two"}}};

  LirBlock case_one;
  case_one.id = LirBlockId{1};
  case_one.label = "case_one";
  case_one.terminator = LirRet{std::string("11"), "i32"};

  LirBlock case_two;
  case_two.id = LirBlockId{2};
  case_two.label = "case_two";
  case_two.terminator = LirRet{std::string("22"), "i32"};

  LirBlock default_block;
  default_block.id = LirBlockId{3};
  default_block.label = "default";
  default_block.terminator = LirRet{std::string("33"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(case_one));
  function.blocks.push_back(std::move(case_two));
  function.blocks.push_back(std::move(default_block));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_alloca_backed_single_case_switch_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.sel", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.sel"});
  entry.insts.push_back(LirLoadOp{"%sel", "i32", "%lv.sel"});
  entry.terminator = LirSwitch{"%sel", "i32", "default", {{7, "case_hit"}}};

  LirBlock case_hit;
  case_hit.id = LirBlockId{1};
  case_hit.label = "case_hit";
  case_hit.terminator = LirRet{std::string("70"), "i32"};

  LirBlock default_block;
  default_block.id = LirBlockId{2};
  default_block.label = "default";
  default_block.terminator = LirRet{std::string("9"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(case_hit));
  function.blocks.push_back(std::move(default_block));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_constant_selector_switch_return_module() {
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
  entry.terminator = LirSwitch{"2", "i32", "default", {{1, "case_one"}, {2, "case_two"}}};

  LirBlock case_one;
  case_one.id = LirBlockId{1};
  case_one.label = "case_one";
  case_one.terminator = LirRet{std::string("11"), "i32"};

  LirBlock case_two;
  case_two.id = LirBlockId{2};
  case_two.label = "case_two";
  case_two.terminator = LirRet{std::string("22"), "i32"};

  LirBlock default_block;
  default_block.id = LirBlockId{3};
  default_block.label = "default";
  default_block.terminator = LirRet{std::string("33"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(case_one));
  function.blocks.push_back(std::move(case_two));
  function.blocks.push_back(std::move(default_block));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_constant_selector_single_case_switch_return_module() {
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
  entry.terminator = LirSwitch{"7", "i32", "default", {{7, "case_hit"}}};

  LirBlock case_hit;
  case_hit.id = LirBlockId{1};
  case_hit.label = "case_hit";
  case_hit.terminator = LirRet{std::string("70"), "i32"};

  LirBlock default_block;
  default_block.id = LirBlockId{2};
  default_block.label = "default";
  default_block.terminator = LirRet{std::string("9"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(case_hit));
  function.blocks.push_back(std::move(default_block));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_lir_u8_select_post_join_add_module() {
  auto module = make_bir_two_param_u8_select_ne_predecessor_add_phi_post_join_add_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  return module;
}

c4c::codegen::lir::LirModule make_alloca_backed_conditional_phi_constant_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%slot.flag", "i32", "", 0});
  function.alloca_insts.push_back(LirAllocaOp{"%slot.then", "i32", "", 0});
  function.alloca_insts.push_back(LirAllocaOp{"%slot.else", "i32", "", 0});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"1", "%slot.flag", "i32"});
  entry.insts.push_back(LirStoreOp{"11", "%slot.then", "i32"});
  entry.insts.push_back(LirStoreOp{"4", "%slot.else", "i32"});
  entry.insts.push_back(LirLoadOp{"%flag", "i32", "%slot.flag"});
  entry.insts.push_back(LirCmpOp{"%cond.i1", false, "eq", "i32", "%flag", "1"});
  entry.insts.push_back(LirCastOp{"%cond.i32", LirCastKind::ZExt, "i1", "%cond.i1", "i32"});
  entry.insts.push_back(LirCmpOp{"%cond.branch", false, "ne", "i32", "%cond.i32", "0"});
  entry.terminator = LirCondBr{"%cond.branch", "then", "else"};

  LirBlock then_block;
  then_block.id = LirBlockId{1};
  then_block.label = "then";
  then_block.insts.push_back(LirLoadOp{"%then.value", "i32", "%slot.then"});
  then_block.terminator = LirBr{"join"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.insts.push_back(LirLoadOp{"%else.value", "i32", "%slot.else"});
  else_block.terminator = LirBr{"join"};

  LirBlock join;
  join.id = LirBlockId{3};
  join.label = "join";
  join.insts.push_back(
      LirPhiOp{"%selected", "i32", {{"%then.value", "then"}, {"%else.value", "else"}}});
  join.terminator = LirRet{std::string("%selected"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  function.blocks.push_back(std::move(join));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_global_int_pointer_diff_module() {
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
  entry.insts.push_back(LirGepOp{"%t0", "[2 x i32]", "@g_words", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t2", "i32", "%t0", false, {"i64 %t1"}});
  entry.insts.push_back(LirGepOp{"%t3", "[2 x i32]", "@g_words", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t4", LirCastKind::SExt, "i32", "0", "i64"});
  entry.insts.push_back(LirGepOp{"%t5", "i32", "%t3", false, {"i64 %t4"}});
  entry.insts.push_back(LirCastOp{"%t6", LirCastKind::PtrToInt, "ptr", "%t2", "i64"});
  entry.insts.push_back(LirCastOp{"%t7", LirCastKind::PtrToInt, "ptr", "%t5", "i64"});
  entry.insts.push_back(LirBinOp{"%t8", "sub", "i64", "%t6", "%t7"});
  entry.insts.push_back(LirBinOp{"%t9", "sdiv", "i64", "%t8", "4"});
  entry.insts.push_back(LirCastOp{"%t10", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirCmpOp{"%t11", false, "eq", "i64", "%t9", "%t10"});
  entry.insts.push_back(LirCastOp{"%t12", LirCastKind::ZExt, "i1", "%t11", "i32"});
  entry.terminator = LirRet{std::string("%t12"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_direct_call_module() {
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
  helper_entry.terminator = LirRet{std::string("42"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_function;
  main_function.name = "main";
  main_function.signature_text = "define i32 @main()\n";
  main_function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@helper", "", ""});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(main_function));
  module.functions.push_back(std::move(helper));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_direct_call_with_dead_entry_alloca_module() {
  auto module = make_lir_minimal_direct_call_module();
  module.functions.front().alloca_insts.push_back(
      c4c::codegen::lir::LirAllocaOp{"%dead.slot", "i32", "", 4});
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_countdown_loop_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  entry.terminator = LirBr{"loop"};

  LirBlock loop;
  loop.id = LirBlockId{1};
  loop.label = "loop";
  loop.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  loop.insts.push_back(LirCmpOp{"%t1", false, "ne", "i32", "%t0", "0"});
  loop.terminator = LirCondBr{"%t1", "body", "exit"};

  LirBlock body;
  body.id = LirBlockId{2};
  body.label = "body";
  body.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  body.insts.push_back(LirBinOp{"%t3", "sub", "i32", "%t2", "1"});
  body.insts.push_back(LirStoreOp{"i32", "%t3", "%lv.x"});
  body.terminator = LirBr{"loop"};

  LirBlock exit;
  exit.id = LirBlockId{3};
  exit.label = "exit";
  exit.insts.push_back(LirLoadOp{"%t4", "i32", "%lv.x"});
  exit.terminator = LirRet{std::string("%t4"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(loop));
  function.blocks.push_back(std::move(body));
  function.blocks.push_back(std::move(exit));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::backend::bir::Module make_bir_minimal_countdown_loop_module() {
  using namespace c4c::backend::bir;

  Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  Function function;
  function.name = "main";
  function.return_type = TypeKind::I32;
  function.local_slots.push_back(
      LocalSlot{.name = "%lv.x", .type = TypeKind::I32, .size_bytes = 4});

  function.blocks.push_back(Block{
      .label = "entry",
      .insts = {StoreLocalInst{
          .slot_name = "%lv.x",
          .value = Value::immediate_i32(5),
      }},
      .terminator = BranchTerminator{.target_label = "loop"},
  });
  function.blocks.push_back(Block{
      .label = "loop",
      .insts = {
          LoadLocalInst{
              .result = Value::named(TypeKind::I32, "%t0"),
              .slot_name = "%lv.x",
          },
          BinaryInst{
              .opcode = BinaryOpcode::Ne,
              .result = Value::named(TypeKind::I32, "%t1"),
              .lhs = Value::named(TypeKind::I32, "%t0"),
              .rhs = Value::immediate_i32(0),
          },
      },
      .terminator = CondBranchTerminator{
          .condition = Value::named(TypeKind::I32, "%t1"),
          .true_label = "body",
          .false_label = "exit",
      },
  });
  function.blocks.push_back(Block{
      .label = "body",
      .insts = {
          LoadLocalInst{
              .result = Value::named(TypeKind::I32, "%t2"),
              .slot_name = "%lv.x",
          },
          BinaryInst{
              .opcode = BinaryOpcode::Sub,
              .result = Value::named(TypeKind::I32, "%t3"),
              .lhs = Value::named(TypeKind::I32, "%t2"),
              .rhs = Value::immediate_i32(1),
          },
          StoreLocalInst{
              .slot_name = "%lv.x",
              .value = Value::named(TypeKind::I32, "%t3"),
          },
      },
      .terminator = BranchTerminator{.target_label = "loop"},
  });
  function.blocks.push_back(Block{
      .label = "exit",
      .insts = {LoadLocalInst{
          .result = Value::named(TypeKind::I32, "%t4"),
          .slot_name = "%lv.x",
      }},
      .terminator = ReturnTerminator{
          .value = Value::named(TypeKind::I32, "%t4"),
      },
  });

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_scalar_global_load_module() {
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

c4c::codegen::lir::LirModule make_lir_minimal_extern_scalar_global_load_module() {
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
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "@g_counter"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_extern_global_array_load_module() {
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

c4c::codegen::lir::LirModule make_lir_minimal_scalar_global_store_reload_module() {
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
  entry.insts.push_back(LirStoreOp{"i32", "7", "@g_counter"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "@g_counter"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::backend::bir::Module make_bir_minimal_scalar_global_load_module() {
  using namespace c4c::backend::bir;

  Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.globals.push_back(Global{
      .name = "g_counter",
      .type = TypeKind::I32,
      .is_extern = false,
      .initializer = Value::immediate_i32(11),
  });

  Function function;
  function.name = "main";
  function.return_type = TypeKind::I32;
  function.blocks.push_back(Block{
      .label = "entry",
      .insts = {LoadGlobalInst{
          .result = Value::named(TypeKind::I32, "%t0"),
          .global_name = "g_counter",
      }},
      .terminator = ReturnTerminator{
          .value = Value::named(TypeKind::I32, "%t0"),
      },
  });
  module.functions.push_back(std::move(function));
  return module;
}

c4c::backend::bir::Module make_bir_minimal_extern_scalar_global_load_module() {
  using namespace c4c::backend::bir;

  Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.globals.push_back(Global{
      .name = "g_counter",
      .type = TypeKind::I32,
      .is_extern = true,
      .initializer = std::nullopt,
  });

  Function function;
  function.name = "main";
  function.return_type = TypeKind::I32;
  function.blocks.push_back(Block{
      .label = "entry",
      .insts = {LoadGlobalInst{
          .result = Value::named(TypeKind::I32, "%t0"),
          .global_name = "g_counter",
      }},
      .terminator = ReturnTerminator{
          .value = Value::named(TypeKind::I32, "%t0"),
      },
  });
  module.functions.push_back(std::move(function));
  return module;
}

c4c::backend::bir::Module make_bir_minimal_scalar_global_store_reload_module() {
  using namespace c4c::backend::bir;

  Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.globals.push_back(Global{
      .name = "g_counter",
      .type = TypeKind::I32,
      .is_extern = false,
      .initializer = Value::immediate_i32(11),
  });

  Function function;
  function.name = "main";
  function.return_type = TypeKind::I32;
  function.blocks.push_back(Block{
      .label = "entry",
      .insts = {
          StoreGlobalInst{
              .global_name = "g_counter",
              .value = Value::immediate_i32(7),
          },
          LoadGlobalInst{
              .result = Value::named(TypeKind::I32, "%t0"),
              .global_name = "g_counter",
          },
      },
      .terminator = ReturnTerminator{
          .value = Value::named(TypeKind::I32, "%t0"),
      },
  });
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_lir_declared_direct_call_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.extern_decls.push_back(LirExternDecl{"puts_like", "i32"});
  module.string_pool.push_back(LirStringConst{"@msg", "hello\\0A", 7});

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@puts_like", "", "ptr @msg"});
  entry.terminator = LirRet{std::string("7"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_void_direct_call_imm_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "voidfn";
  helper.signature_text = "define void @voidfn()\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.terminator = LirRet{std::nullopt, "void"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_function;
  main_function.name = "main";
  main_function.signature_text = "define i32 @main()\n";
  main_function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"", "void", "@voidfn", "", ""});
  entry.terminator = LirRet{std::string("9"), "i32"};
  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(main_function));
  module.functions.push_back(std::move(helper));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_two_arg_direct_call_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_pair";
  helper.signature_text = "define i32 @add_pair(i32 %lhs, i32 %rhs)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%sum", LirBinaryOpcode::Add, "i32", "%lhs", "%rhs"});
  helper_entry.terminator = LirRet{std::string("%sum"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_function;
  main_function.name = "main";
  main_function.signature_text = "define i32 @main()\n";
  main_function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@add_pair", "(i32, i32)", "i32 5, i32 7"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(main_function));
  module.functions.push_back(std::move(helper));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_direct_call_add_imm_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_one";
  helper.signature_text = "define i32 @add_one(i32 %arg0)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%sum", LirBinaryOpcode::Add, "i32", "%arg0", "1"});
  helper_entry.terminator = LirRet{std::string("%sum"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_function;
  main_function.name = "main";
  main_function.signature_text = "define i32 @main()\n";
  main_function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@add_one", "(i32)", "i32 5"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(main_function));
  module.functions.push_back(std::move(helper));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_direct_call_identity_arg_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "f";
  helper.signature_text = "define i32 @f(i32 %arg0)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.terminator = LirRet{std::string("%arg0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_function;
  main_function.name = "main";
  main_function.signature_text = "define i32 @main()\n";
  main_function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@f", "(i32)", "i32 5"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(main_function));
  module.functions.push_back(std::move(helper));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_dual_identity_direct_call_sub_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction lhs;
  lhs.name = "f";
  lhs.signature_text = "define i32 @f(i32 %arg0)\n";
  lhs.entry = LirBlockId{0};
  lhs.blocks.push_back(
      LirBlock{.id = LirBlockId{0},
               .label = "entry",
               .insts = {},
               .terminator = LirRet{std::string("%arg0"), "i32"}});

  LirFunction rhs;
  rhs.name = "g";
  rhs.signature_text = "define i32 @g(i32 %arg0)\n";
  rhs.entry = LirBlockId{0};
  rhs.blocks.push_back(
      LirBlock{.id = LirBlockId{0},
               .label = "entry",
               .insts = {},
               .terminator = LirRet{std::string("%arg0"), "i32"}});

  LirFunction main_function;
  main_function.name = "main";
  main_function.signature_text = "define i32 @main()\n";
  main_function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@f", "(i32)", "i32 7"});
  entry.insts.push_back(LirCallOp{"%t1", "i32", "@g", "(i32)", "i32 3"});
  entry.insts.push_back(LirBinOp{"%t2", LirBinaryOpcode::Sub, "i32", "%t0", "%t1"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};
  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(lhs));
  module.functions.push_back(std::move(rhs));
  module.functions.push_back(std::move(main_function));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_call_crossing_direct_call_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "add_one";
  helper.signature_text = "define i32 @add_one(i32 %p.x)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%t0", LirBinaryOpcode::Add, "i32", "%p.x", "1"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_function;
  main_function.name = "main";
  main_function.signature_text = "define i32 @main()\n";
  main_function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", LirBinaryOpcode::Add, "i32", "2", "3"});
  entry.insts.push_back(LirCallOp{"%t1", "i32", "@add_one", "(i32)", "i32 %t0"});
  entry.insts.push_back(LirBinOp{"%t2", LirBinaryOpcode::Add, "i32", "%t0", "%t1"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};
  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_function));
  return module;
}

void populate_bir_minimal_call_crossing_direct_call_module(c4c::backend::bir::Module& module) {
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "add_one",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {
          c4c::backend::bir::Param{.type = c4c::backend::bir::TypeKind::I32, .name = "%arg0"},
      },
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {c4c::backend::bir::BinaryInst{
              .opcode = c4c::backend::bir::BinaryOpcode::Add,
              .result = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%sum"),
              .lhs = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%arg0"),
              .rhs = c4c::backend::bir::Value::immediate_i32(1),
          }},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%sum"),
          },
      }},
      .is_declaration = false,
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "main",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {
              c4c::backend::bir::BinaryInst{
                  .opcode = c4c::backend::bir::BinaryOpcode::Add,
                  .result =
                      c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
                  .lhs = c4c::backend::bir::Value::immediate_i32(2),
                  .rhs = c4c::backend::bir::Value::immediate_i32(3),
              },
              c4c::backend::bir::CallInst{
                  .result =
                      c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t1"),
                  .callee = "add_one",
                  .args = {c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32,
                                                           "%t0")},
                  .return_type_name = "i32",
              },
              c4c::backend::bir::BinaryInst{
                  .opcode = c4c::backend::bir::BinaryOpcode::Add,
                  .result =
                      c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t2"),
                  .lhs = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
                  .rhs = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t1"),
              },
          },
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t2"),
          },
      }},
      .is_declaration = false,
  });
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_direct_call_end_to_end() {
  c4c::backend::bir::Module module;
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "helper",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::immediate_i32(42),
          },
      }},
      .is_declaration = false,
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "main",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {c4c::backend::bir::CallInst{
              .result = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
              .callee = "helper",
              .args = {},
              .return_type_name = "i32",
          }},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
          },
      }},
      .is_declaration = false,
  });

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".type helper, %function\nhelper:",
                  "direct BIR zero-argument direct-call input should emit the helper body on the native aarch64 path");
  expect_contains(rendered, "mov w0, #42",
                  "direct BIR zero-argument direct-call input should preserve the helper return immediate on the native aarch64 path");
  expect_contains(rendered, ".globl main",
                  "direct BIR zero-argument direct-call input should emit the caller symbol without routing through legacy backend IR");
  expect_contains(rendered, "bl helper",
                  "direct BIR zero-argument direct-call input should branch to the helper on the native aarch64 backend path");
  expect_not_contains(rendered, "target triple =",
                      "direct BIR zero-argument direct-call input should stay on native aarch64 asm emission");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_declared_direct_call_end_to_end() {
  c4c::backend::bir::Module module;
  module.string_constants.push_back(c4c::backend::bir::StringConstant{
      .name = "msg",
      .bytes = "hello\\n",
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "puts_like",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {c4c::backend::bir::Param{
          .type = c4c::backend::bir::TypeKind::I64,
          .name = "%arg0",
      }},
      .local_slots = {},
      .blocks = {},
      .is_declaration = true,
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "main",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {c4c::backend::bir::CallInst{
              .result = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
              .callee = "puts_like",
              .args = {c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I64, "msg")},
              .return_type_name = "i32",
          }},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::immediate_i32(7),
          },
      }},
      .is_declaration = false,
  });

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".section .rodata",
                  "direct BIR declared direct-call input should materialize referenced string constants on the native aarch64 path");
  expect_contains(rendered, ".asciz \"hello\\n\"",
                  "direct BIR declared direct-call input should preserve string-constant bytes for native aarch64 emission");
  expect_contains(rendered, "adrp x0, ",
                  "direct BIR declared direct-call input should materialize pointer-style extern arguments through address formation on the native aarch64 path");
  expect_contains(rendered, "bl puts_like",
                  "direct BIR declared direct-call input should branch to the declared callee without routing through legacy backend IR");
  expect_contains(rendered, "mov w0, #7",
                  "direct BIR declared direct-call input should preserve the fixed immediate return override on the native aarch64 path");
  expect_not_contains(rendered, "target triple =",
                      "direct BIR declared direct-call input should stay on native aarch64 asm emission");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_void_direct_call_imm_return_end_to_end() {
  c4c::backend::bir::Module module;
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "voidfn",
      .return_type = c4c::backend::bir::TypeKind::Void,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {},
          .terminator = c4c::backend::bir::ReturnTerminator{},
      }},
      .is_declaration = false,
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "main",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {c4c::backend::bir::CallInst{
              .result = std::nullopt,
              .callee = "voidfn",
              .args = {},
              .return_type_name = "void",
          }},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::immediate_i32(9),
          },
      }},
      .is_declaration = false,
  });

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".type voidfn, %function\nvoidfn:",
                  "direct BIR void direct-call input should emit the helper body on the native aarch64 path");
  expect_contains(rendered, "bl voidfn",
                  "direct BIR void direct-call input should branch to the helper on the native aarch64 backend path");
  expect_contains(rendered, "mov w0, #9",
                  "direct BIR void direct-call input should preserve the fixed caller return immediate on the native aarch64 path");
  expect_not_contains(rendered, "target triple =",
                      "direct BIR void direct-call input should stay on native aarch64 asm emission");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_two_arg_direct_call_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_lir_minimal_two_arg_direct_call_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".type add_pair, %function\nadd_pair:",
                  "direct BIR two-argument direct-call input should emit the helper body on the native aarch64 path");
  expect_contains(rendered, "add w0, w0, w1",
                  "direct BIR two-argument direct-call input should preserve the helper add semantics on the native aarch64 path");
  expect_contains(rendered, "mov w0, #5",
                  "direct BIR two-argument direct-call input should materialize the lhs immediate before branching to the helper");
  expect_contains(rendered, "mov w1, #7",
                  "direct BIR two-argument direct-call input should materialize the rhs immediate before branching to the helper");
  expect_contains(rendered, "bl add_pair",
                  "direct BIR two-argument direct-call input should branch to the helper without routing through legacy backend IR");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_direct_call_add_imm_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_lir_minimal_direct_call_add_imm_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".type add_one, %function\nadd_one:",
                  "direct BIR add-immediate direct-call input should emit the helper body on the native aarch64 path");
  expect_contains(rendered, "add w0, w0, #1",
                  "direct BIR add-immediate direct-call input should preserve the helper immediate adjustment on the native aarch64 path");
  expect_contains(rendered, "mov w0, #5",
                  "direct BIR add-immediate direct-call input should materialize the caller argument before branching to the helper");
  expect_contains(rendered, "bl add_one",
                  "direct BIR add-immediate direct-call input should branch to the helper without routing through legacy backend IR");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_direct_call_identity_arg_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_lir_minimal_direct_call_identity_arg_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".type f, %function\nf:",
                  "direct BIR identity direct-call input should emit the helper body on the native aarch64 path");
  expect_contains(rendered, "mov w0, #5",
                  "direct BIR identity direct-call input should materialize the caller argument before branching to the helper");
  expect_contains(rendered, "bl f",
                  "direct BIR identity direct-call input should branch to the helper without routing through legacy backend IR");
  expect_not_contains(rendered, "target triple =",
                      "direct BIR identity direct-call input should stay on native aarch64 asm emission");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_dual_identity_direct_call_sub_end_to_end() {
  c4c::backend::bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "f",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {
          c4c::backend::bir::Param{.type = c4c::backend::bir::TypeKind::I32, .name = "%arg0"},
      },
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%arg0"),
          },
      }},
      .is_declaration = false,
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "g",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {
          c4c::backend::bir::Param{.type = c4c::backend::bir::TypeKind::I32, .name = "%arg0"},
      },
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%arg0"),
          },
      }},
      .is_declaration = false,
  });
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "main",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {
              c4c::backend::bir::CallInst{
                  .result =
                      c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
                  .callee = "f",
                  .args = {c4c::backend::bir::Value::immediate_i32(7)},
                  .return_type_name = "i32",
              },
              c4c::backend::bir::CallInst{
                  .result =
                      c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t1"),
                  .callee = "g",
                  .args = {c4c::backend::bir::Value::immediate_i32(3)},
                  .return_type_name = "i32",
              },
              c4c::backend::bir::BinaryInst{
                  .opcode = c4c::backend::bir::BinaryOpcode::Sub,
                  .result =
                      c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t2"),
                  .lhs = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
                  .rhs = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t1"),
              },
          },
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t2"),
          },
      }},
      .is_declaration = false,
  });

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".type f, %function\nf:",
                  "direct BIR dual-identity subtraction input should emit the left helper definition on the native aarch64 path");
  expect_contains(rendered, ".type g, %function\ng:",
                  "direct BIR dual-identity subtraction input should emit the right helper definition on the native aarch64 path");
  expect_contains(rendered, "mov w0, #7",
                  "direct BIR dual-identity subtraction input should materialize the left caller immediate on the native aarch64 path");
  expect_contains(rendered, "mov w0, #3",
                  "direct BIR dual-identity subtraction input should materialize the right caller immediate on the native aarch64 path");
  expect_contains(rendered, "sub w19, w19, w0",
                  "direct BIR dual-identity subtraction input should lower the subtraction natively without legacy backend IR");
  expect_not_contains(rendered, "target triple =",
                      "direct BIR dual-identity subtraction input should stay on native aarch64 asm emission");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_call_crossing_direct_call_end_to_end() {
  c4c::backend::bir::Module module;
  populate_bir_minimal_call_crossing_direct_call_module(module);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".type add_one, %function\nadd_one:",
                  "direct BIR call-crossing input should emit the helper definition on the native aarch64 path");
  expect_contains(rendered, ".globl main",
                  "direct BIR call-crossing input should emit the caller definition on the native aarch64 path");
  expect_contains(rendered, "add w0, w0, #1",
                  "direct BIR call-crossing input should preserve the helper add-immediate body on the native aarch64 path");
  expect_contains(rendered, "mov w20, #5",
                  "direct BIR call-crossing input should materialize the source value in the shared callee-saved register");
  expect_contains(rendered, "bl add_one",
                  "direct BIR call-crossing input should lower the helper call without legacy backend IR");
  expect_contains(rendered, "add w0, w0, w20",
                  "direct BIR call-crossing input should add the preserved source value after the helper call");
  expect_not_contains(rendered, "target triple =",
                      "direct BIR call-crossing input should stay on native aarch64 asm emission");
}

void test_backend_bir_pipeline_drives_aarch64_lir_minimal_direct_call_through_bir_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_direct_call_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".type helper, %function\nhelper:",
                  "aarch64 LIR minimal direct-call input should still emit the helper definition after routing through the shared BIR path");
  expect_contains(rendered, "mov w0, #42",
                  "aarch64 LIR minimal direct-call input should preserve the helper immediate on the BIR-owned route");
  expect_contains(rendered, "bl helper",
                  "aarch64 LIR minimal direct-call input should lower the helper call on the native aarch64 path");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR minimal direct-call input should stay on native asm emission instead of falling back to LLVM text");
}

void test_aarch64_direct_emitter_lowers_minimal_direct_call_via_outer_bir_path() {
  const auto module = make_lir_minimal_direct_call_module();
  const auto shared_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));
  const auto rendered = c4c::backend::aarch64::emit_module(module);

  expect_contains(rendered, ".type helper, %function\nhelper:",
                  "aarch64 direct emitter should still emit the helper definition when direct-LIR staging declines a BIR-lowerable direct-call module");
  expect_contains(rendered, "mov w0, #42",
                  "aarch64 direct emitter should preserve the helper immediate after the outer BIR-lowering retry owns the module");
  expect_contains(rendered, "bl helper",
                  "aarch64 direct emitter should still lower the helper call after removing the duplicate inner BIR retry seam");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 direct emitter should stay on native asm emission when the outer BIR path handles the direct-call module");
  expect_true(rendered == shared_rendered,
              "aarch64 direct emitter should now match the shared backend entrypoint exactly once raw LIR routing is centralized");
}

void test_backend_bir_pipeline_drives_aarch64_lir_minimal_direct_call_with_dead_entry_alloca_end_to_end() {
  const auto module = make_lir_minimal_direct_call_with_dead_entry_alloca_module();
  expect_true(!c4c::backend::try_lower_to_bir(module).has_value(),
              "raw aarch64 LIR direct-call input with a dead entry alloca should still miss the bounded shared BIR matcher before backend-side pruning");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".type helper, %function\nhelper:",
                  "aarch64 LIR direct-call input with a dead entry alloca should still emit the helper definition after backend-side pruning");
  expect_contains(rendered, "mov w0, #42",
                  "aarch64 LIR direct-call input with a dead entry alloca should preserve the helper immediate after backend-side pruning");
  expect_contains(rendered, "bl helper",
                  "aarch64 LIR direct-call input with a dead entry alloca should still lower the helper call on the native aarch64 path");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR direct-call input with a dead entry alloca should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_minimal_void_direct_call_imm_return_through_bir_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_void_direct_call_imm_return_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".type voidfn, %function\nvoidfn:",
                  "aarch64 LIR void direct-call input should still emit the helper definition after routing through the shared BIR path");
  expect_contains(rendered, "bl voidfn",
                  "aarch64 LIR void direct-call input should lower the helper call on the native aarch64 path");
  expect_contains(rendered, "mov w0, #9",
                  "aarch64 LIR void direct-call input should preserve the fixed caller return immediate on the BIR-owned route");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR void direct-call input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_declared_direct_call_through_bir_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_declared_direct_call_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".section .rodata",
                  "aarch64 LIR declared direct-call input should now reach the BIR-owned extern-call emitter path");
  expect_contains(rendered, ".asciz \"hello\\n\"",
                  "aarch64 LIR declared direct-call input should preserve string bytes through the BIR lowering seam");
  expect_contains(rendered, "bl puts_like",
                  "aarch64 LIR declared direct-call input should still lower the declared call after the aarch64 legacy seam is bypassed");
  expect_contains(rendered, "mov w0, #7",
                  "aarch64 LIR declared direct-call input should preserve the fixed immediate return override on the BIR path");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR declared direct-call input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_countdown_loop_end_to_end() {
  const auto rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{make_bir_minimal_countdown_loop_module()},
                                make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, "mov w0, #5",
                  "aarch64 direct BIR countdown-loop input should materialize the initial counter on the native backend path");
  expect_contains(rendered, ".Lloop:\n  cmp w0, #0\n  b.eq .Lexit\n",
                  "aarch64 direct BIR countdown-loop input should preserve the loop test on the native backend path");
  expect_contains(rendered, ".Lbody:\n  sub w0, w0, #1\n  b .Lloop\n",
                  "aarch64 direct BIR countdown-loop input should preserve the decrement-and-backedge sequence on the native backend path");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 direct BIR countdown-loop input should stay on native asm emission");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_scalar_global_load_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_minimal_scalar_global_load_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl g_counter",
                  "aarch64 direct BIR scalar global-load input should still emit the global definition on the native backend path");
  expect_contains(rendered, "  .long 11\n",
                  "aarch64 direct BIR scalar global-load input should preserve the initialized global payload");
  expect_contains(rendered, "ldr w0, [x8, :lo12:g_counter]",
                  "aarch64 direct BIR scalar global-load input should lower bir.load_global into a native scalar memory load");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 direct BIR scalar global-load input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_extern_scalar_global_load_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_minimal_extern_scalar_global_load_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_not_contains(rendered, ".globl g_counter",
                      "aarch64 direct BIR extern scalar global-load input should keep the global as an unresolved extern instead of materializing a definition");
  expect_contains(rendered, "ldr w0, [x8, :lo12:g_counter]",
                  "aarch64 direct BIR extern scalar global-load input should lower bir.load_global into a native scalar memory load");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 direct BIR extern scalar global-load input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_scalar_global_store_reload_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_minimal_scalar_global_store_reload_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl g_counter",
                  "aarch64 direct BIR scalar global store-reload input should still emit the global definition on the native backend path");
  expect_contains(rendered, "  .long 11\n",
                  "aarch64 direct BIR scalar global store-reload input should preserve the initialized global payload");
  expect_contains(rendered, "str w9, [x8]",
                  "aarch64 direct BIR scalar global store-reload input should lower bir.store_global into a native scalar memory store");
  expect_contains(rendered, "ldr w0, [x8]",
                  "aarch64 direct BIR scalar global store-reload input should reload the just-stored scalar value on the native backend path");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 direct BIR scalar global store-reload input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_minimal_two_arg_direct_call_through_bir_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_two_arg_direct_call_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, "add w0, w0, w1",
                  "aarch64 LIR two-argument direct-call input should lower through the shared BIR path to the native helper add sequence");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 LIR two-argument direct-call input should preserve the lhs immediate on the BIR-owned route");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 LIR two-argument direct-call input should preserve the rhs immediate on the BIR-owned route");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 LIR two-argument direct-call input should branch to the helper on the native aarch64 backend path");
}

void test_backend_bir_pipeline_drives_aarch64_lir_minimal_direct_call_add_imm_through_bir_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_direct_call_add_imm_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, "add w0, w0, #1",
                  "aarch64 LIR add-immediate direct-call input should lower through the shared BIR path to the native helper add-immediate sequence");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 LIR add-immediate direct-call input should preserve the caller argument on the BIR-owned route");
  expect_contains(rendered, "bl add_one",
                  "aarch64 LIR add-immediate direct-call input should branch to the helper on the native aarch64 backend path");
}

void test_backend_bir_pipeline_drives_aarch64_lir_minimal_direct_call_identity_arg_through_bir_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_direct_call_identity_arg_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".type f, %function\nf:",
                  "aarch64 LIR identity direct-call input should still emit the helper definition after routing through the shared BIR path");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 LIR identity direct-call input should preserve the caller argument on the BIR-owned route");
  expect_contains(rendered, "bl f",
                  "aarch64 LIR identity direct-call input should branch to the helper on the native aarch64 backend path");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR identity direct-call input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_minimal_dual_identity_direct_call_sub_through_bir_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_dual_identity_direct_call_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".type f, %function\nf:",
                  "aarch64 LIR dual-identity subtraction input should still emit the left helper definition after routing through the shared BIR path");
  expect_contains(rendered, ".type g, %function\ng:",
                  "aarch64 LIR dual-identity subtraction input should still emit the right helper definition after routing through the shared BIR path");
  expect_contains(rendered, "mov w0, #7",
                  "aarch64 LIR dual-identity subtraction input should preserve the left caller immediate on the BIR-owned route");
  expect_contains(rendered, "mov w0, #3",
                  "aarch64 LIR dual-identity subtraction input should preserve the right caller immediate on the BIR-owned route");
  expect_contains(rendered, "sub w19, w19, w0",
                  "aarch64 LIR dual-identity subtraction input should lower the subtraction on the native aarch64 path");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR dual-identity subtraction input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_minimal_countdown_loop_through_bir_end_to_end() {
  const auto lowered_bir = c4c::backend::try_lower_to_bir(make_lir_minimal_countdown_loop_module());
  expect_true(lowered_bir.has_value(),
              "aarch64 LIR countdown-loop input should now lower into the bounded shared BIR local-slot loop shape");
  expect_true(lowered_bir->functions.size() == 1 &&
                  lowered_bir->functions.front().local_slots.size() == 1 &&
                  lowered_bir->functions.front().blocks.size() == 4 &&
                  lowered_bir->functions.front().blocks[0].terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Branch &&
                  lowered_bir->functions.front().blocks[1].terminator.kind ==
                      c4c::backend::bir::TerminatorKind::CondBranch,
              "aarch64 LIR countdown-loop lowering should produce the expected multi-block shared BIR control-flow skeleton");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_countdown_loop_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, "mov w0, #5",
                  "aarch64 LIR countdown-loop input should preserve the initial counter after routing through the shared BIR path");
  expect_contains(rendered, ".Lloop:\n  cmp w0, #0\n  b.eq .Lexit\n",
                  "aarch64 LIR countdown-loop input should preserve the loop test after routing through shared BIR");
  expect_contains(rendered, ".Lbody:\n  sub w0, w0, #1\n  b .Lloop\n",
                  "aarch64 LIR countdown-loop input should preserve the decrement-and-backedge sequence after routing through shared BIR");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR countdown-loop input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_minimal_scalar_global_load_through_bir_end_to_end() {
  const auto lowered_bir =
      c4c::backend::try_lower_to_bir(make_lir_minimal_scalar_global_load_module());
  expect_true(lowered_bir.has_value(),
              "aarch64 LIR scalar global-load input should now lower into the bounded shared BIR global-load shape");
  expect_true(lowered_bir->globals.size() == 1 && lowered_bir->functions.size() == 1 &&
                  lowered_bir->functions.front().blocks.size() == 1 &&
                  lowered_bir->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::LoadGlobalInst>(
                      lowered_bir->functions.front().blocks.front().insts.front()),
              "aarch64 LIR scalar global-load lowering should produce one initialized global plus one bir.load_global instruction");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_scalar_global_load_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl g_counter",
                  "aarch64 LIR scalar global-load input should still emit the global definition after routing through the shared BIR path");
  expect_contains(rendered, "ldr w0, [x8, :lo12:g_counter]",
                  "aarch64 LIR scalar global-load input should lower bir.load_global on the native aarch64 path");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR scalar global-load input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_minimal_extern_scalar_global_load_through_bir_end_to_end() {
  const auto lowered_bir =
      c4c::backend::try_lower_to_bir(make_lir_minimal_extern_scalar_global_load_module());
  expect_true(lowered_bir.has_value(),
              "aarch64 LIR extern scalar global-load input should now lower into the bounded shared BIR global-load shape");
  expect_true(lowered_bir->globals.size() == 1 && lowered_bir->functions.size() == 1 &&
                  lowered_bir->globals.front().is_extern &&
                  !lowered_bir->globals.front().initializer.has_value() &&
                  lowered_bir->functions.front().blocks.size() == 1 &&
                  lowered_bir->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::LoadGlobalInst>(
                      lowered_bir->functions.front().blocks.front().insts.front()),
              "aarch64 LIR extern scalar global-load lowering should produce one extern global declaration plus one bir.load_global instruction");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_extern_scalar_global_load_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_not_contains(rendered, ".globl g_counter",
                      "aarch64 LIR extern scalar global-load input should keep the global unresolved after routing through the shared BIR path");
  expect_contains(rendered, "ldr w0, [x8, :lo12:g_counter]",
                  "aarch64 LIR extern scalar global-load input should lower bir.load_global on the native aarch64 path");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR extern scalar global-load input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_minimal_extern_global_array_load_through_bir_end_to_end() {
  const auto lowered_bir =
      c4c::backend::try_lower_to_bir(make_lir_minimal_extern_global_array_load_module());
  expect_true(lowered_bir.has_value(),
              "aarch64 LIR extern global-array-load input should now lower into the bounded shared BIR global-load shape");
  expect_true(lowered_bir->globals.size() == 1 && lowered_bir->functions.size() == 1 &&
                  lowered_bir->globals.front().is_extern &&
                  !lowered_bir->globals.front().initializer.has_value() &&
                  lowered_bir->functions.front().blocks.size() == 1 &&
                  lowered_bir->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::LoadGlobalInst>(
                      lowered_bir->functions.front().blocks.front().insts.front()) &&
                  std::get<c4c::backend::bir::LoadGlobalInst>(
                      lowered_bir->functions.front().blocks.front().insts.front()).byte_offset == 4,
              "aarch64 LIR extern global-array-load lowering should produce one extern global declaration plus one byte-offset bir.load_global instruction");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_extern_global_array_load_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_not_contains(rendered, ".globl ext_arr",
                      "aarch64 LIR extern global-array-load input should keep the global unresolved after routing through the shared BIR path");
  expect_contains(rendered, "ldr w0, [x8, :lo12:ext_arr+4]",
                  "aarch64 LIR extern global-array-load input should lower the byte-offset bir.load_global on the native aarch64 path");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR extern global-array-load input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_minimal_global_char_pointer_diff_through_bir_end_to_end() {
  const auto lowered_bir =
      c4c::backend::try_lower_to_bir(make_lir_minimal_global_char_pointer_diff_module());
  expect_true(lowered_bir.has_value(),
              "aarch64 LIR global char pointer-diff input should now lower into the bounded shared BIR immediate-return shape");
  if (!lowered_bir.has_value()) {
    return;
  }
  expect_true(lowered_bir->globals.empty() && lowered_bir->functions.size() == 1 &&
                  lowered_bir->functions.front().blocks.size() == 1 &&
                  lowered_bir->functions.front().blocks.front().insts.empty() &&
                  lowered_bir->functions.front().blocks.front().terminator.value.has_value() &&
                  lowered_bir->functions.front().blocks.front().terminator.value->kind ==
                      c4c::backend::bir::Value::Kind::Immediate &&
                  lowered_bir->functions.front().blocks.front().terminator.value->immediate == 1,
              "aarch64 LIR global char pointer-diff lowering should produce the bounded shared immediate-return contract");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_global_char_pointer_diff_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, "mov w0, #1",
                  "aarch64 LIR global char pointer-diff input should materialize the shared immediate-return result on the native aarch64 path");
  expect_not_contains(rendered, "sub x8, x9, x8",
                      "aarch64 LIR global char pointer-diff input should no longer rely on the emitter-local pointer-diff fast path");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR global char pointer-diff input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_minimal_global_int_pointer_diff_through_bir_end_to_end() {
  const auto lowered_bir =
      c4c::backend::try_lower_to_bir(make_lir_minimal_global_int_pointer_diff_module());
  expect_true(lowered_bir.has_value(),
              "aarch64 LIR global int pointer-diff input should now lower into the bounded shared BIR immediate-return shape");
  if (!lowered_bir.has_value()) {
    return;
  }
  expect_true(lowered_bir->globals.empty() && lowered_bir->functions.size() == 1 &&
                  lowered_bir->functions.front().blocks.size() == 1 &&
                  lowered_bir->functions.front().blocks.front().insts.empty() &&
                  lowered_bir->functions.front().blocks.front().terminator.value.has_value() &&
                  lowered_bir->functions.front().blocks.front().terminator.value->kind ==
                      c4c::backend::bir::Value::Kind::Immediate &&
                  lowered_bir->functions.front().blocks.front().terminator.value->immediate == 1,
              "aarch64 LIR global int pointer-diff lowering should produce the bounded shared immediate-return contract");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_global_int_pointer_diff_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, "mov w0, #1",
                  "aarch64 LIR global int pointer-diff input should materialize the shared immediate-return result on the native aarch64 path");
  expect_not_contains(rendered, "sub x8, x9, x8",
                      "aarch64 LIR global int pointer-diff input should no longer rely on the emitter-local pointer-diff fast path");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR global int pointer-diff input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_minimal_scalar_global_store_reload_through_bir_end_to_end() {
  const auto lowered_bir =
      c4c::backend::try_lower_to_bir(make_lir_minimal_scalar_global_store_reload_module());
  expect_true(lowered_bir.has_value(),
              "aarch64 LIR scalar global store-reload input should now lower into the bounded shared BIR global store-reload shape");
  expect_true(lowered_bir->globals.size() == 1 && lowered_bir->functions.size() == 1 &&
                  lowered_bir->functions.front().blocks.size() == 1 &&
                  lowered_bir->functions.front().blocks.front().insts.size() == 2 &&
                  std::holds_alternative<c4c::backend::bir::StoreGlobalInst>(
                      lowered_bir->functions.front().blocks.front().insts.front()) &&
                  std::holds_alternative<c4c::backend::bir::LoadGlobalInst>(
                      lowered_bir->functions.front().blocks.front().insts[1]),
              "aarch64 LIR scalar global store-reload lowering should produce one initialized global plus direct bir.store_global and bir.load_global instructions");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_scalar_global_store_reload_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl g_counter",
                  "aarch64 LIR scalar global store-reload input should still emit the global definition after routing through the shared BIR path");
  expect_contains(rendered, "str w9, [x8]",
                  "aarch64 LIR scalar global store-reload input should lower bir.store_global on the native aarch64 path");
  expect_contains(rendered, "ldr w0, [x8]",
                  "aarch64 LIR scalar global store-reload input should reload the stored scalar value on the native aarch64 path");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR scalar global store-reload input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_minimal_call_crossing_direct_call_through_bir_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_call_crossing_direct_call_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".type add_one, %function\nadd_one:",
                  "aarch64 LIR call-crossing input should still emit the helper definition after routing through the shared BIR path");
  expect_contains(rendered, "mov w20, #5",
                  "aarch64 LIR call-crossing input should preserve the source value in the shared callee-saved register assignment");
  expect_contains(rendered, "bl add_one",
                  "aarch64 LIR call-crossing input should stay on the shared BIR-owned direct-call emitter path");
  expect_contains(rendered, "add w0, w0, w20",
                  "aarch64 LIR call-crossing input should preserve the final add against the source value after BIR lowering");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR call-crossing input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_single_param_chain_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_single_param_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl add_one",
                  "BIR pipeline should still reach aarch64 backend emission for the bounded one-parameter slice");
  expect_contains(rendered, "add w0, w0, #1",
                  "aarch64 BIR lowering should collapse the bounded parameter-fed chain into the expected affine adjustment");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_zero_param_staged_constant_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_bir_return_staged_constant_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl main",
                  "direct BIR zero-parameter staged constant input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov w0, #8",
                  "aarch64 direct BIR zero-parameter staged constant input should collapse the full chain to the surviving constant");
}

void test_backend_bir_pipeline_drives_aarch64_two_param_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl add_pair",
                  "BIR pipeline should still reach aarch64 backend emission for the bounded two-parameter slice");
  expect_contains(rendered, "add w0, w0, w1",
                  "aarch64 BIR lowering should combine both incoming integer argument registers for the bounded two-parameter add");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_input_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{c4c::backend::lower_to_bir(make_bir_two_param_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl add_pair",
                  "direct BIR module input should reach aarch64 backend emission without a caller-owned legacy backend module");
  expect_contains(rendered, "add w0, w0, w1",
                  "aarch64 direct BIR input should still combine both incoming integer argument registers on the backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_staged_affine_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_bir_two_param_staged_affine_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl add_pair",
                  "direct BIR staged affine input should reach aarch64 backend emission without caller-owned legacy backend IR");
  expect_contains(rendered, "add w0, w0, w1",
                  "aarch64 direct BIR staged affine input should still combine both incoming integer argument registers");
  expect_contains(rendered, "add w0, w0, #1",
                  "aarch64 direct BIR staged affine input should preserve the collapsed affine constant");
}

void test_backend_bir_pipeline_drives_aarch64_two_param_add_sub_chain_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl add_pair",
                  "BIR pipeline should still reach aarch64 backend emission for the bounded two-parameter affine chain");
  expect_contains(rendered, "add w0, w0, w1",
                  "aarch64 BIR lowering should combine both incoming integer argument registers before the immediate adjustment");
  expect_contains(rendered, "sub w0, w0, #1",
                  "aarch64 BIR lowering should preserve the trailing immediate adjustment for the bounded two-parameter affine chain");
}

void test_backend_bir_pipeline_drives_aarch64_two_param_staged_affine_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_staged_affine_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl add_pair",
                  "BIR pipeline should still reach aarch64 backend emission for the staged two-parameter affine slice");
  expect_contains(rendered, "add w0, w0, w1",
                  "aarch64 BIR lowering should combine both incoming integer argument registers for the staged affine slice");
  expect_contains(rendered, "add w0, w0, #1",
                  "aarch64 BIR lowering should collapse the staged immediate adjustments to the surviving affine constant");
}

void test_backend_bir_pipeline_drives_aarch64_sext_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_sext_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl widen_signed",
                  "BIR pipeline should reach aarch64 backend emission for the bounded sext slice");
  expect_contains(rendered, "sxtw x0, w0",
                  "aarch64 BIR lowering should sign-extend the incoming i32 argument into the i64 return register");
}

void test_backend_bir_pipeline_drives_aarch64_zext_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_zext_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl widen_unsigned",
                  "BIR pipeline should reach aarch64 backend emission for the bounded zext slice");
  expect_contains(rendered, "and w0, w0, #0xff",
                  "aarch64 BIR lowering should zero-extend the incoming i8 argument into the i32 return register");
}

void test_backend_bir_pipeline_drives_aarch64_trunc_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_trunc_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl narrow_signed",
                  "BIR pipeline should reach aarch64 backend emission for the bounded trunc slice");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 BIR lowering should keep the bounded trunc slice on the native asm path");
}

void test_backend_bir_pipeline_drives_aarch64_select_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_bir_return_select_eq_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl main",
                  "direct BIR select input should reach aarch64 backend emission without a caller-owned legacy backend module");
  expect_contains(rendered, "cmp w8, #7",
                  "aarch64 BIR lowering should compare the bounded select operands on the native backend path");
  expect_contains(rendered, "mov w0, #11",
                  "aarch64 BIR lowering should materialize the true-value arm for the bounded select slice");
  expect_contains(rendered, "mov w0, #4",
                  "aarch64 BIR lowering should materialize the false-value arm for the bounded select slice");
}

void test_backend_bir_pipeline_drives_aarch64_lir_minimal_conditional_return_through_bir_end_to_end() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_lir_minimal_conditional_return_module());
  expect_true(lowered.has_value(),
              "aarch64 LIR conditional-return input should lower into a direct BIR module before target emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::SelectInst>(
                      lowered->functions.front().blocks.front().insts.front()),
              "aarch64 LIR conditional-return input should collapse to the shared BIR select form before target emission");

  const auto rendered = c4c::backend::aarch64::emit_module(
      make_lir_minimal_conditional_return_module());

  expect_contains(rendered, ".globl main",
                  "aarch64 LIR conditional-return input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "b.ge .Lselect_false",
                  "aarch64 LIR conditional-return input should lower the fused BIR predicate to the native branch form on the shared backend path");
  expect_contains(rendered, ".Lselect_true:\n  mov w0, #0\n  ret\n",
                  "aarch64 LIR conditional-return input should preserve the then-arm immediate through the BIR-owned select emission");
  expect_contains(rendered, ".Lselect_false:\n  mov w0, #1\n  ret\n",
                  "aarch64 LIR conditional-return input should preserve the else-arm immediate through the BIR-owned select emission");
  expect_not_contains(rendered, ".main.then:",
                      "aarch64 LIR conditional-return input should no longer expose the original direct-LIR branch block labels once the shared BIR route owns the shape");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR conditional-return input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_conditional_return_with_empty_bridge_blocks_through_bir_end_to_end() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_lir_minimal_conditional_return_with_empty_bridge_blocks_module());
  expect_true(lowered.has_value(),
              "aarch64 LIR empty-bridge conditional-return input should lower into a direct BIR module before target emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::SelectInst>(
                      lowered->functions.front().blocks.front().insts.front()),
              "aarch64 LIR empty-bridge conditional-return input should collapse to the shared BIR select form before target emission");

  const auto rendered = c4c::backend::aarch64::emit_module(
      make_lir_minimal_conditional_return_with_empty_bridge_blocks_module());

  expect_contains(rendered, ".globl main",
                  "aarch64 LIR empty-bridge conditional-return input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "b.ge .Lselect_false",
                  "aarch64 LIR empty-bridge conditional-return input should lower the fused BIR predicate to the native branch form on the shared backend path");
  expect_contains(rendered, ".Lselect_true:\n  mov w0, #0\n  ret\n",
                  "aarch64 LIR empty-bridge conditional-return input should preserve the then-arm immediate through the BIR-owned select emission");
  expect_contains(rendered, ".Lselect_false:\n  mov w0, #1\n  ret\n",
                  "aarch64 LIR empty-bridge conditional-return input should preserve the else-arm immediate through the BIR-owned select emission");
  expect_not_contains(rendered, ".main.then.bridge:",
                      "aarch64 LIR empty-bridge conditional-return input should no longer expose the original bridge block labels once the shared BIR route owns the shape");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR empty-bridge conditional-return input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_conditional_return_with_asymmetric_empty_bridge_through_bir_end_to_end() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_lir_minimal_conditional_return_with_asymmetric_empty_bridge_module());
  expect_true(
      lowered.has_value(),
      "aarch64 LIR asymmetric empty-bridge conditional-return input should lower into a direct BIR module before target emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::SelectInst>(
                      lowered->functions.front().blocks.front().insts.front()),
              "aarch64 LIR asymmetric empty-bridge conditional-return input should collapse to the shared BIR select form before target emission");

  const auto rendered = c4c::backend::aarch64::emit_module(
      make_lir_minimal_conditional_return_with_asymmetric_empty_bridge_module());

  expect_contains(rendered, ".globl main",
                  "aarch64 LIR asymmetric empty-bridge conditional-return input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "b.ge .Lselect_false",
                  "aarch64 LIR asymmetric empty-bridge conditional-return input should lower the fused BIR predicate to the native branch form on the shared backend path");
  expect_contains(rendered, ".Lselect_true:\n  mov w0, #0\n  ret\n",
                  "aarch64 LIR asymmetric empty-bridge conditional-return input should preserve the then-arm immediate through the BIR-owned select emission");
  expect_contains(rendered, ".Lselect_false:\n  mov w0, #1\n  ret\n",
                  "aarch64 LIR asymmetric empty-bridge conditional-return input should preserve the else-arm immediate through the BIR-owned select emission");
  expect_not_contains(rendered, ".main.then.bridge:",
                      "aarch64 LIR asymmetric empty-bridge conditional-return input should no longer expose the original direct-LIR bridge labels once the shared BIR route owns the shape");
  expect_not_contains(rendered, ".main.else.ret:",
                      "aarch64 LIR asymmetric empty-bridge conditional-return input should no longer expose the original direct-LIR return labels once the shared BIR route owns the shape");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR asymmetric empty-bridge conditional-return input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_conditional_return_with_double_empty_bridge_chain_through_bir_end_to_end() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(
          make_lir_minimal_conditional_return_with_double_empty_bridge_chain_module());
  expect_true(
      lowered.has_value(),
      "aarch64 LIR double-empty-bridge conditional-return input should lower into a direct BIR module before target emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1,
              "aarch64 LIR double-empty-bridge conditional-return input should collapse to the shared BIR select form before target emission");

  const auto rendered = c4c::backend::aarch64::emit_module(
      make_lir_minimal_conditional_return_with_double_empty_bridge_chain_module());

  expect_contains(rendered, ".globl main",
                  "aarch64 LIR double-empty-bridge conditional-return input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "b.ge .Lselect_false",
                  "aarch64 LIR double-empty-bridge conditional-return input should lower the fused BIR predicate to the native branch form on the shared backend path");
  expect_contains(rendered, ".Lselect_true:\n  mov w0, #0\n  ret\n",
                  "aarch64 LIR double-empty-bridge conditional-return input should preserve the then-arm immediate through the BIR-owned select emission");
  expect_contains(rendered, ".Lselect_false:\n  mov w0, #1\n  ret\n",
                  "aarch64 LIR double-empty-bridge conditional-return input should preserve the else-arm immediate through the BIR-owned select emission");
  expect_not_contains(rendered, ".main.then.bridge0:",
                      "aarch64 LIR double-empty-bridge conditional-return input should no longer expose the first direct-LIR bridge label once the shared BIR route owns the shape");
  expect_not_contains(rendered, ".main.then.bridge1:",
                      "aarch64 LIR double-empty-bridge conditional-return input should no longer expose the second direct-LIR bridge label once the shared BIR route owns the shape");
  expect_not_contains(rendered, ".main.then.ret:",
                      "aarch64 LIR double-empty-bridge conditional-return input should no longer expose the original direct-LIR return label once the shared BIR route owns the shape");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR double-empty-bridge conditional-return input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_conditional_return_with_mirrored_false_double_empty_bridge_chain_through_bir_end_to_end() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_lir_minimal_conditional_return_with_mirrored_false_double_empty_bridge_chain_module());
  expect_true(
      lowered.has_value(),
      "aarch64 LIR mirrored false-arm double-empty-bridge conditional-return input should lower into a direct BIR module before target emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::SelectInst>(
                      lowered->functions.front().blocks.front().insts.front()),
              "aarch64 LIR mirrored false-arm double-empty-bridge conditional-return input should collapse to the shared BIR select form before target emission");

  const auto rendered = c4c::backend::aarch64::emit_module(
      make_lir_minimal_conditional_return_with_mirrored_false_double_empty_bridge_chain_module());

  expect_contains(rendered, ".globl main",
                  "aarch64 LIR mirrored false-arm double-empty-bridge conditional-return input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "b.ge .Lselect_false",
                  "aarch64 LIR mirrored false-arm double-empty-bridge conditional-return input should lower the fused BIR predicate to the native branch form on the shared backend path");
  expect_contains(rendered, ".Lselect_true:\n  mov w0, #0\n  ret\n",
                  "aarch64 LIR mirrored false-arm double-empty-bridge conditional-return input should preserve the then-arm immediate through the BIR-owned select emission");
  expect_contains(rendered, ".Lselect_false:\n  mov w0, #1\n  ret\n",
                  "aarch64 LIR mirrored false-arm double-empty-bridge conditional-return input should preserve the else-arm immediate through the BIR-owned select emission");
  expect_not_contains(rendered, ".main.else.bridge0:",
                      "aarch64 LIR mirrored false-arm double-empty-bridge conditional-return input should no longer expose the first direct-LIR bridge label once the shared BIR route owns the shape");
  expect_not_contains(rendered, ".main.else.bridge1:",
                      "aarch64 LIR mirrored false-arm double-empty-bridge conditional-return input should no longer expose the second direct-LIR bridge label once the shared BIR route owns the shape");
  expect_not_contains(rendered, ".main.else.ret:",
                      "aarch64 LIR mirrored false-arm double-empty-bridge conditional-return input should no longer expose the original direct-LIR return label once the shared BIR route owns the shape");
  expect_not_contains(rendered, ".main.then.ret:",
                      "aarch64 LIR mirrored false-arm double-empty-bridge conditional-return input should no longer expose the reordered direct-LIR return label once the shared BIR route owns the shape");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR mirrored false-arm double-empty-bridge conditional-return input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_conditional_return_with_interleaved_double_empty_bridge_chains_through_bir_end_to_end() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_lir_minimal_conditional_return_with_interleaved_double_empty_bridge_chains_module());
  expect_true(
      lowered.has_value(),
      "aarch64 LIR interleaved double-empty-bridge conditional-return input should lower into a direct BIR module before target emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::SelectInst>(
                      lowered->functions.front().blocks.front().insts.front()),
              "aarch64 LIR interleaved double-empty-bridge conditional-return input should collapse to the shared BIR select form before target emission");

  const auto rendered = c4c::backend::aarch64::emit_module(
      make_lir_minimal_conditional_return_with_interleaved_double_empty_bridge_chains_module());

  expect_contains(rendered, ".globl main",
                  "aarch64 LIR interleaved double-empty-bridge conditional-return input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "b.ge .Lselect_false",
                  "aarch64 LIR interleaved double-empty-bridge conditional-return input should lower the fused BIR predicate to the native branch form on the shared backend path");
  expect_contains(rendered, ".Lselect_true:\n  mov w0, #0\n  ret\n",
                  "aarch64 LIR interleaved double-empty-bridge conditional-return input should preserve the then-arm immediate through the BIR-owned select emission");
  expect_contains(rendered, ".Lselect_false:\n  mov w0, #1\n  ret\n",
                  "aarch64 LIR interleaved double-empty-bridge conditional-return input should preserve the else-arm immediate through the BIR-owned select emission");
  expect_not_contains(rendered, ".main.then.bridge0:",
                      "aarch64 LIR interleaved double-empty-bridge conditional-return input should no longer expose the first direct-LIR then bridge label once the shared BIR route owns the shape");
  expect_not_contains(rendered, ".main.then.bridge1:",
                      "aarch64 LIR interleaved double-empty-bridge conditional-return input should no longer expose the second direct-LIR then bridge label once the shared BIR route owns the shape");
  expect_not_contains(rendered, ".main.else.bridge0:",
                      "aarch64 LIR interleaved double-empty-bridge conditional-return input should no longer expose the first direct-LIR else bridge label once the shared BIR route owns the shape");
  expect_not_contains(rendered, ".main.else.bridge1:",
                      "aarch64 LIR interleaved double-empty-bridge conditional-return input should no longer expose the second direct-LIR else bridge label once the shared BIR route owns the shape");
  expect_not_contains(rendered, ".main.then.ret:",
                      "aarch64 LIR interleaved double-empty-bridge conditional-return input should no longer expose the direct-LIR then return label once the shared BIR route owns the shape");
  expect_not_contains(rendered, ".main.else.ret:",
                      "aarch64 LIR interleaved double-empty-bridge conditional-return input should no longer expose the direct-LIR else return label once the shared BIR route owns the shape");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR interleaved double-empty-bridge conditional-return input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_param_conditional_return_with_interleaved_mixed_depth_bridge_chains_through_bir_end_to_end() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_lir_param_conditional_return_with_interleaved_mixed_depth_bridge_chains_module());
  expect_true(
      lowered.has_value(),
      "aarch64 LIR parameter-fed mixed-depth conditional-return input should lower into a direct BIR module before target emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::SelectInst>(
                      lowered->functions.front().blocks.front().insts.front()),
              "aarch64 LIR parameter-fed mixed-depth conditional-return input should collapse to the shared BIR select form before target emission");

  const auto rendered = c4c::backend::aarch64::emit_module(
      make_lir_param_conditional_return_with_interleaved_mixed_depth_bridge_chains_module());

  expect_contains(rendered, ".globl choose",
                  "aarch64 LIR parameter-fed mixed-depth conditional-return input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "mov w8, w0",
                  "aarch64 LIR parameter-fed mixed-depth conditional-return input should stage the first incoming integer argument into the compare scratch register");
  expect_contains(rendered, "mov w9, w1",
                  "aarch64 LIR parameter-fed mixed-depth conditional-return input should stage the second incoming integer argument into the compare scratch register");
  expect_contains(rendered, "cmp w8, w9",
                  "aarch64 LIR parameter-fed mixed-depth conditional-return input should compare both staged integer argument registers on the shared backend path");
  expect_contains(rendered, ".Lselect_true:\n  ret\n",
                  "aarch64 LIR parameter-fed mixed-depth conditional-return input should preserve the true-arm parameter return through the BIR-owned select emission");
  expect_contains(rendered, ".Lselect_false:\n  mov w0, w1\n  ret\n",
                  "aarch64 LIR parameter-fed mixed-depth conditional-return input should preserve the false-arm parameter return through the BIR-owned select emission");
  expect_not_contains(rendered, ".choose.then.bridge0:",
                      "aarch64 LIR parameter-fed mixed-depth conditional-return input should no longer expose the first direct-LIR then bridge label once the shared BIR route owns the shape");
  expect_not_contains(rendered, ".choose.then.bridge1:",
                      "aarch64 LIR parameter-fed mixed-depth conditional-return input should no longer expose the second direct-LIR then bridge label once the shared BIR route owns the shape");
  expect_not_contains(rendered, ".choose.then.bridge2:",
                      "aarch64 LIR parameter-fed mixed-depth conditional-return input should no longer expose the third direct-LIR then bridge label once the shared BIR route owns the shape");
  expect_not_contains(rendered, ".choose.else.bridge0:",
                      "aarch64 LIR parameter-fed mixed-depth conditional-return input should no longer expose the direct-LIR else bridge label once the shared BIR route owns the shape");
  expect_not_contains(rendered, ".choose.then.ret:",
                      "aarch64 LIR parameter-fed mixed-depth conditional-return input should no longer expose the direct-LIR then return label once the shared BIR route owns the shape");
  expect_not_contains(rendered, ".choose.else.ret:",
                      "aarch64 LIR parameter-fed mixed-depth conditional-return input should no longer expose the direct-LIR else return label once the shared BIR route owns the shape");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR parameter-fed mixed-depth conditional-return input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_conditional_phi_join_through_bir_end_to_end() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_lir_single_param_select_eq_phi_module());
  expect_true(lowered.has_value(),
              "aarch64 LIR conditional-phi-join input should lower into a direct BIR module before target emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::SelectInst>(
                      lowered->functions.front().blocks.front().insts.front()),
              "aarch64 LIR conditional-phi-join input should collapse to the shared BIR select form before target emission");

  const auto rendered = c4c::backend::aarch64::emit_module(
      make_lir_single_param_select_eq_phi_module());

  expect_contains(rendered, ".globl choose",
                  "aarch64 direct emitter should still reach native asm emission after routing a lowerable conditional-phi-join through the shared BIR path");
  expect_contains(rendered, ".Lselect_true:\n  mov w0, #11\n  ret\n",
                  "aarch64 lowerable conditional-phi-join input should use the shared BIR-owned select labels instead of target-local phi-copy blocks");
  expect_contains(rendered, ".Lselect_false:\n  mov w0, #4\n  ret\n",
                  "aarch64 lowerable conditional-phi-join input should preserve the bounded false arm through the BIR-owned select emission");
  expect_not_contains(rendered, ".choose.tern.then",
                      "aarch64 lowerable conditional-phi-join input should no longer emit the original target-local phi predecessor block labels once the BIR route owns the shape");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR conditional-phi-join input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_lir_u8_select_post_join_add_through_bir_end_to_end() {
  const auto lowered = c4c::backend::try_lower_to_bir(make_lir_u8_select_post_join_add_module());
  expect_true(lowered.has_value(),
              "aarch64 LIR u8 select-plus-tail input should lower into a direct BIR module before target emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1,
              "aarch64 LIR u8 select-plus-tail input should collapse to a shared one-block BIR function before target emission");

  const auto rendered = c4c::backend::aarch64::emit_module(
      make_lir_u8_select_post_join_add_module());

  expect_contains(rendered, ".globl choose2_add_post_ne_u",
                  "aarch64 LIR u8 select-plus-tail input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, ".Lselect_true:\n  and w0, w0, #0xff\n  add w0, w0, #5\n  b .Lselect_join\n",
                  "aarch64 LIR u8 select-plus-tail input should preserve the shared BIR-owned then-arm predecessor arithmetic");
  expect_contains(rendered, ".Lselect_false:\n  and w0, w1, #0xff\n  add w0, w0, #9\n",
                  "aarch64 LIR u8 select-plus-tail input should preserve the shared BIR-owned else-arm predecessor arithmetic");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  ret\n",
                  "aarch64 LIR u8 select-plus-tail input should preserve the shared BIR-owned post-join arithmetic tail");
  expect_not_contains(rendered, ".choose2_add_post_ne_u.tern.then",
                      "aarch64 lowerable u8 select-plus-tail input should no longer emit the original target-local predecessor block labels once the BIR route owns the shape");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 LIR u8 select-plus-tail input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_single_param_select_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_bir_single_param_select_eq_phi_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose",
                  "direct BIR single-parameter select input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov w8, w0",
                  "aarch64 direct BIR single-parameter select input should stage the incoming integer argument into the compare scratch register");
  expect_contains(rendered, "cmp w8, #7",
                  "aarch64 direct BIR single-parameter select input should compare the staged integer argument against the bounded immediate");
  expect_contains(rendered, "mov w0, #11",
                  "aarch64 direct BIR single-parameter select input should materialize the true-value arm for the bounded parameter-fed select slice");
  expect_contains(rendered, "mov w0, #4",
                  "aarch64 direct BIR single-parameter select input should materialize the false-value arm for the bounded parameter-fed select slice");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_two_param_select_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_bir_two_param_select_eq_phi_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2",
                  "direct BIR two-parameter select input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov w8, w0",
                  "aarch64 direct BIR two-parameter select input should stage the first incoming integer argument into the compare scratch register");
  expect_contains(rendered, "mov w9, w1",
                  "aarch64 direct BIR two-parameter select input should stage the second incoming integer argument into the compare scratch register");
  expect_contains(rendered, "cmp w8, w9",
                  "aarch64 direct BIR two-parameter select input should compare both staged integer argument registers on the native backend path");
  expect_contains(rendered, "mov w0, w1",
                  "aarch64 direct BIR two-parameter select input should materialize the false-value arm from the second incoming integer argument register");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_post_join_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_predecessor_add_phi_post_join_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_add_post_ne_u",
                  "direct BIR u8 select-plus-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "and w8, w0, #0xff",
                  "aarch64 direct BIR u8 select-plus-tail input should mask the first incoming byte before comparison");
  expect_contains(rendered, "and w9, w1, #0xff\n  cmp w8, w9\n",
                  "aarch64 direct BIR u8 select-plus-tail input should compare the masked incoming bytes on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  and w0, w0, #0xff\n  add w0, w0, #5\n  b .Lselect_join\n",
                  "aarch64 direct BIR u8 select-plus-tail input should materialize the bounded then-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  and w0, w1, #0xff\n  add w0, w0, #9\n",
                  "aarch64 direct BIR u8 select-plus-tail input should materialize the bounded else-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  ret\n",
                  "aarch64 direct BIR u8 select-plus-tail input should preserve the bounded post-select arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_post_join_add_sub_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_split_predecessor_add_phi_post_join_add_sub_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_add_post_chain_ne_u",
                  "direct BIR u8 select-plus-add/sub-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "and w8, w0, #0xff",
                  "aarch64 direct BIR u8 select-plus-add/sub-tail input should mask the first incoming byte before comparison");
  expect_contains(rendered, "and w9, w1, #0xff\n  cmp w8, w9\n",
                  "aarch64 direct BIR u8 select-plus-add/sub-tail input should compare the masked incoming bytes on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  and w0, w0, #0xff\n  add w0, w0, #5\n  b .Lselect_join\n",
                  "aarch64 direct BIR u8 select-plus-add/sub-tail input should materialize the bounded then-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  and w0, w1, #0xff\n  add w0, w0, #9\n",
                  "aarch64 direct BIR u8 select-plus-add/sub-tail input should materialize the bounded else-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  sub w0, w0, #2\n  ret\n",
                  "aarch64 direct BIR u8 select-plus-add/sub-tail input should preserve the bounded post-select add/sub arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_post_join_add_sub_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_split_predecessor_add_phi_post_join_add_sub_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_add_post_chain_tail_ne_u",
                  "direct BIR u8 select-plus-add/sub/add-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "and w8, w0, #0xff",
                  "aarch64 direct BIR u8 select-plus-add/sub/add-tail input should mask the first incoming byte before comparison");
  expect_contains(rendered, "and w9, w1, #0xff\n  cmp w8, w9\n",
                  "aarch64 direct BIR u8 select-plus-add/sub/add-tail input should compare the masked incoming bytes on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  and w0, w0, #0xff\n  add w0, w0, #5\n  b .Lselect_join\n",
                  "aarch64 direct BIR u8 select-plus-add/sub/add-tail input should materialize the bounded then-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  and w0, w1, #0xff\n  add w0, w0, #9\n",
                  "aarch64 direct BIR u8 select-plus-add/sub/add-tail input should materialize the bounded else-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  sub w0, w0, #2\n  add w0, w0, #9\n  ret\n",
                  "aarch64 direct BIR u8 select-plus-add/sub/add-tail input should preserve the bounded post-select add/sub/add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_mixed_affine_post_join_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_split_predecessor_mixed_affine_phi_post_join_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_mixed_post_ne_u",
                  "direct BIR u8 mixed-affine select-plus-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "and w8, w0, #0xff",
                  "aarch64 direct BIR u8 mixed-affine select-plus-tail input should mask the first incoming byte before comparison");
  expect_contains(rendered, "and w9, w1, #0xff\n  cmp w8, w9\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-tail input should compare the masked incoming bytes on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  and w0, w0, #0xff\n  add w0, w0, #8\n  sub w0, w0, #3\n  b .Lselect_join\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-tail input should materialize the bounded then-arm affine arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  and w0, w1, #0xff\n  add w0, w0, #11\n  sub w0, w0, #4\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-tail input should materialize the bounded else-arm affine arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  ret\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-tail input should preserve the bounded post-select add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_mixed_affine_post_join_add_sub_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_split_predecessor_mixed_affine_phi_post_join_add_sub_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_mixed_post_chain_ne_u",
                  "direct BIR u8 mixed-affine select-plus-add/sub-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "and w8, w0, #0xff",
                  "aarch64 direct BIR u8 mixed-affine select-plus-add/sub-tail input should mask the first incoming byte before comparison");
  expect_contains(rendered, "and w9, w1, #0xff\n  cmp w8, w9\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-add/sub-tail input should compare the masked incoming bytes on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  and w0, w0, #0xff\n  add w0, w0, #8\n  sub w0, w0, #3\n  b .Lselect_join\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-add/sub-tail input should materialize the bounded then-arm affine arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  and w0, w1, #0xff\n  add w0, w0, #11\n  sub w0, w0, #4\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-add/sub-tail input should materialize the bounded else-arm affine arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  sub w0, w0, #2\n  ret\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-add/sub-tail input should preserve the bounded post-select add/sub arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_mixed_affine_post_join_add_sub_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_eq_split_predecessor_mixed_affine_phi_post_join_add_sub_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_mixed_post_chain_tail_u",
                  "direct BIR u8 mixed-affine select-plus-add/sub/add-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "and w8, w0, #0xff",
                  "aarch64 direct BIR u8 mixed-affine select-plus-add/sub/add-tail input should mask the first incoming byte before comparison");
  expect_contains(rendered, "and w9, w1, #0xff\n  cmp w8, w9\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-add/sub/add-tail input should compare the masked incoming bytes on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  and w0, w0, #0xff\n  add w0, w0, #8\n  sub w0, w0, #3\n  b .Lselect_join\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-add/sub/add-tail input should materialize the bounded then-arm affine arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  and w0, w1, #0xff\n  add w0, w0, #11\n  sub w0, w0, #4\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-add/sub/add-tail input should materialize the bounded else-arm affine arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  sub w0, w0, #2\n  add w0, w0, #9\n  ret\n",
                  "aarch64 direct BIR u8 mixed-affine select-plus-add/sub/add-tail input should preserve the bounded post-select add/sub/add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_deeper_then_mixed_affine_post_join_add_sub_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_deeper_post_chain_tail_u",
                  "direct BIR u8 deeper-then-mixed-affine select-plus-add/sub/add-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "and w8, w0, #0xff",
                  "aarch64 direct BIR u8 deeper-then-mixed-affine select-plus-add/sub/add-tail input should mask the first incoming byte before comparison");
  expect_contains(rendered, "and w9, w1, #0xff\n  cmp w8, w9\n",
                  "aarch64 direct BIR u8 deeper-then-mixed-affine select-plus-add/sub/add-tail input should compare the masked incoming bytes on the native backend path");
  expect_contains(rendered,
                  ".Lselect_true:\n  and w0, w0, #0xff\n  add w0, w0, #8\n  sub w0, w0, #3\n  add w0, w0, #5\n  b .Lselect_join\n",
                  "aarch64 direct BIR u8 deeper-then-mixed-affine select-plus-add/sub/add-tail input should materialize the bounded deeper then-arm arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  and w0, w1, #0xff\n  add w0, w0, #11\n  sub w0, w0, #4\n",
                  "aarch64 direct BIR u8 deeper-then-mixed-affine select-plus-add/sub/add-tail input should materialize the bounded else-arm mixed arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  sub w0, w0, #2\n  add w0, w0, #9\n  ret\n",
                  "aarch64 direct BIR u8 deeper-then-mixed-affine select-plus-add/sub/add-tail input should preserve the bounded post-select add/sub/add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_deeper_affine_on_both_sides_post_join_add_sub_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_eq_split_predecessor_deeper_affine_phi_post_join_add_sub_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_deeper_both_post_chain_tail_u",
                  "direct BIR u8 deeper-affine-both-sides select-plus-add/sub/add-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "and w8, w0, #0xff",
                  "aarch64 direct BIR u8 deeper-affine-both-sides select-plus-add/sub/add-tail input should mask the first incoming byte before comparison");
  expect_contains(rendered, "and w9, w1, #0xff\n  cmp w8, w9\n",
                  "aarch64 direct BIR u8 deeper-affine-both-sides select-plus-add/sub/add-tail input should compare the masked incoming bytes on the native backend path");
  expect_contains(rendered,
                  ".Lselect_true:\n  and w0, w0, #0xff\n  add w0, w0, #8\n  sub w0, w0, #3\n  add w0, w0, #5\n  b .Lselect_join\n",
                  "aarch64 direct BIR u8 deeper-affine-both-sides select-plus-add/sub/add-tail input should materialize the bounded deeper then-arm arithmetic before the synthetic join");
  expect_contains(rendered,
                  ".Lselect_false:\n  and w0, w1, #0xff\n  add w0, w0, #11\n  sub w0, w0, #4\n  add w0, w0, #7\n",
                  "aarch64 direct BIR u8 deeper-affine-both-sides select-plus-add/sub/add-tail input should materialize the bounded deeper else-arm arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  sub w0, w0, #2\n  add w0, w0, #9\n  ret\n",
                  "aarch64 direct BIR u8 deeper-affine-both-sides select-plus-add/sub/add-tail input should preserve the bounded post-select add/sub/add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_mixed_then_deeper_affine_post_join_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_mixed_then_deeper_post_ne_u",
                  "direct BIR u8 mixed-then-deeper-affine select-plus-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "and w8, w0, #0xff",
                  "aarch64 direct BIR u8 mixed-then-deeper-affine select-plus-tail input should mask the first incoming byte before comparison");
  expect_contains(rendered, "and w9, w1, #0xff\n  cmp w8, w9\n",
                  "aarch64 direct BIR u8 mixed-then-deeper-affine select-plus-tail input should compare the masked incoming bytes on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  and w0, w0, #0xff\n  add w0, w0, #8\n  sub w0, w0, #3\n  b .Lselect_join\n",
                  "aarch64 direct BIR u8 mixed-then-deeper-affine select-plus-tail input should materialize the bounded then-arm mixed arithmetic before the synthetic join");
  expect_contains(rendered,
                  ".Lselect_false:\n  and w0, w1, #0xff\n  add w0, w0, #11\n  sub w0, w0, #4\n  add w0, w0, #7\n",
                  "aarch64 direct BIR u8 mixed-then-deeper-affine select-plus-tail input should materialize the bounded else-arm deeper arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  ret\n",
                  "aarch64 direct BIR u8 mixed-then-deeper-affine select-plus-tail input should preserve the bounded post-select add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_i32_select_mixed_then_deeper_affine_post_join_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_mixed_then_deeper_post",
                  "direct BIR i32 mixed-then-deeper-affine select-plus-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov w8, w0",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-tail input should stage the first incoming integer argument into the compare scratch register");
  expect_contains(rendered, "mov w9, w1",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-tail input should stage the second incoming integer argument into the compare scratch register");
  expect_contains(rendered, "cmp w8, w9",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-tail input should compare both staged integer argument registers on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  add w0, w0, #8\n  sub w0, w0, #3\n  b .Lselect_join\n",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-tail input should materialize the bounded then-arm mixed arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  mov w0, w1\n  add w0, w0, #11\n  sub w0, w0, #4\n  add w0, w0, #7\n",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-tail input should materialize the bounded else-arm deeper arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  ret\n",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-tail input should preserve the bounded post-select add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_i32_select_mixed_then_deeper_affine_post_join_add_sub_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_mixed_then_deeper_post_chain",
                  "direct BIR i32 mixed-then-deeper-affine select-plus-add/sub-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov w8, w0",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub-tail input should stage the first incoming integer argument into the compare scratch register");
  expect_contains(rendered, "mov w9, w1",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub-tail input should stage the second incoming integer argument into the compare scratch register");
  expect_contains(rendered, "cmp w8, w9",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub-tail input should compare both staged integer argument registers on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  add w0, w0, #8\n  sub w0, w0, #3\n  b .Lselect_join\n",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub-tail input should materialize the bounded then-arm mixed arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  mov w0, w1\n  add w0, w0, #11\n  sub w0, w0, #4\n  add w0, w0, #7\n",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub-tail input should materialize the bounded else-arm deeper arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  sub w0, w0, #2\n  ret\n",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub-tail input should preserve the bounded post-select add/sub arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_aarch64_direct_bir_i32_select_mixed_then_deeper_affine_post_join_add_sub_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));

  expect_contains(rendered, ".globl choose2_mixed_then_deeper_post_chain_tail",
                  "direct BIR i32 mixed-then-deeper-affine select-plus-add/sub/add-tail input should reach aarch64 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov w8, w0",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub/add-tail input should stage the first incoming integer argument into the compare scratch register");
  expect_contains(rendered, "mov w9, w1",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub/add-tail input should stage the second incoming integer argument into the compare scratch register");
  expect_contains(rendered, "cmp w8, w9",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub/add-tail input should compare both staged integer argument registers on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  add w0, w0, #8\n  sub w0, w0, #3\n  b .Lselect_join\n",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub/add-tail input should materialize the bounded then-arm mixed arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  mov w0, w1\n  add w0, w0, #11\n  sub w0, w0, #4\n  add w0, w0, #7\n",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub/add-tail input should materialize the bounded else-arm deeper arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add w0, w0, #6\n  sub w0, w0, #2\n  add w0, w0, #9\n  ret\n",
                  "aarch64 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub/add-tail input should preserve the bounded post-select add/sub/add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_rejects_unsupported_direct_bir_input_on_aarch64() {
  try {
    (void)c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{make_unsupported_multi_function_bir_module()},
        make_bir_pipeline_options(c4c::backend::Target::Aarch64));
  } catch (const std::invalid_argument& ex) {
    expect_contains(ex.what(), "direct BIR module",
                    "aarch64 direct BIR rejection should explain that unsupported direct BIR input no longer falls back through legacy backend IR");
    expect_not_contains(ex.what(), "legacy backend IR",
                        "aarch64 direct BIR rejection should not mention the deleted legacy backend IR route at the shared backend entry");
    return;
  }

  fail("aarch64 direct BIR input should fail explicitly once the emitter-side bir_to_backend_ir fallback is removed");
}

void test_aarch64_direct_lir_emitter_rejects_unsupported_input_without_legacy_backend_ir_stub() {
  try {
    (void)c4c::backend::aarch64::emit_module(make_unsupported_double_return_lir_module());
  } catch (const std::invalid_argument& ex) {
    expect_contains(ex.what(), "direct LIR module",
                    "aarch64 direct LIR rejection should describe the direct-LIR subset instead of a deleted legacy backend IR route");
    expect_not_contains(ex.what(), "legacy backend IR",
                        "aarch64 direct LIR rejection should not mention the deleted legacy backend IR path");
    return;
  }

  fail("aarch64 direct LIR emitter should fail explicitly once unsupported input stops pretending a legacy backend IR route still exists");
}

void test_aarch64_direct_lir_emitter_rejects_alloca_backed_conditional_phi_fallback() {
  expect_true(!c4c::backend::try_lower_to_bir(
                   make_alloca_backed_conditional_phi_constant_module())
                   .has_value(),
              "alloca-backed conditional-phi input should bypass the shared BIR lowering seam so this regression exercises the remaining direct-LIR phi boundary");

  const auto rendered =
      c4c::backend::aarch64::emit_module(make_alloca_backed_conditional_phi_constant_module());
  expect_contains(rendered, ".main.join:",
                  "aarch64 direct emitter should keep bounded alloca-backed conditional-phi joins on the native path when the shared BIR lowering seam declines them");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 direct emitter should not fall back to LLVM text for the bounded direct-LIR phi-copy path");
}

void test_aarch64_direct_emitter_routes_goto_only_constant_return_through_shared_bir() {
  expect_true(c4c::backend::try_lower_to_bir(make_goto_only_constant_return_module()).has_value(),
              "goto-only constant-return input should now lower through the shared BIR boundary instead of staying stranded on the aarch64 direct-LIR CFG helper path");

  const auto rendered = c4c::backend::aarch64::emit_module(make_goto_only_constant_return_module());
  expect_contains(rendered, "mov w0, #9\n  ret\n",
                  "aarch64 direct emitter should now use the shared BIR-owned immediate-return route for the goto-only constant-return slice");
  expect_not_contains(rendered, ".main.ulbl_start:",
                      "aarch64 direct emitter should no longer expose the old target-local goto chain once shared BIR owns the branch-only constant-return shape");
}

void test_aarch64_direct_lir_emitter_rejects_double_indirect_pointer_conditional_return_fallback() {
  expect_true(
      !c4c::backend::try_lower_to_bir(
           make_double_indirect_local_pointer_conditional_return_module())
           .has_value(),
      "double-indirect local-pointer conditional-return input should continue to miss shared BIR lowering so this regression exercises the remaining aarch64 direct-LIR CFG helper boundary");

  try {
    (void)c4c::backend::aarch64::emit_module(
        make_double_indirect_local_pointer_conditional_return_module());
  } catch (const std::invalid_argument& ex) {
    expect_contains(ex.what(), "direct LIR module",
                    "aarch64 direct emitter should reject double-indirect local-pointer conditional-return folding once CFG ownership stays behind the shared BIR boundary");
    return;
  }

  fail("aarch64 direct emitter should reject double-indirect local-pointer conditional-return modules once the direct-LIR CFG helper is pruned");
}

void test_aarch64_direct_lir_emitter_rejects_alloca_backed_switch_fallback() {
  expect_true(!c4c::backend::try_lower_to_bir(make_alloca_backed_switch_return_module())
                   .has_value(),
              "alloca-backed switch input should continue to miss shared BIR lowering so this regression exercises the remaining aarch64 direct-LIR switch boundary");

  try {
    (void)c4c::backend::aarch64::emit_module(make_alloca_backed_switch_return_module());
  } catch (const std::invalid_argument& ex) {
    expect_contains(ex.what(), "direct LIR module",
                    "aarch64 direct emitter should reject alloca-backed switch CFG once ownership stays behind the shared BIR boundary");
    return;
  }

  fail("aarch64 direct emitter should reject alloca-backed switch CFG modules once the general direct-LIR switch fallback is pruned");
}

void test_aarch64_direct_lir_emitter_rejects_alloca_backed_single_case_switch_fallback() {
  expect_true(!c4c::backend::try_lower_to_bir(
                   make_alloca_backed_single_case_switch_return_module())
                   .has_value(),
              "single-case alloca-backed switch input should continue to miss shared BIR lowering so this regression exercises the remaining synthetic aarch64 direct-LIR switch boundary");

  try {
    (void)c4c::backend::aarch64::emit_module(
        make_alloca_backed_single_case_switch_return_module());
  } catch (const std::invalid_argument& ex) {
    expect_contains(ex.what(), "direct LIR module",
                    "aarch64 direct emitter should reject synthetic single-case alloca-backed switch CFG once ownership stays behind the shared BIR boundary");
    return;
  }

  fail("aarch64 direct emitter should reject synthetic single-case alloca-backed switch CFG modules once the generic direct-LIR switch fallback is fenced behind BIR");
}

void test_aarch64_direct_lir_emitter_rejects_constant_selector_switch_fallback() {
  expect_true(!c4c::backend::try_lower_to_bir(make_constant_selector_switch_return_module())
                   .has_value(),
              "constant-selector switch input should continue to miss shared BIR lowering so this regression exercises the remaining aarch64 direct-LIR switch boundary in the generic emitter");

  try {
    (void)c4c::backend::aarch64::emit_module(make_constant_selector_switch_return_module());
  } catch (const std::invalid_argument& ex) {
    expect_contains(ex.what(), "direct LIR module",
                    "aarch64 direct emitter should reject constant-selector switch CFG once ownership stays behind the shared BIR boundary");
    return;
  }

  fail("aarch64 direct emitter should reject constant-selector switch CFG modules once the generic direct-LIR switch fallback is fenced behind BIR");
}

void test_aarch64_direct_lir_emitter_rejects_constant_selector_single_case_switch_fallback() {
  expect_true(
      !c4c::backend::try_lower_to_bir(
           make_constant_selector_single_case_switch_return_module())
           .has_value(),
      "single-case constant-selector switch input should continue to miss shared BIR lowering so this regression exercises the remaining synthetic aarch64 direct-LIR switch boundary in the generic emitter");

  try {
    (void)c4c::backend::aarch64::emit_module(
        make_constant_selector_single_case_switch_return_module());
  } catch (const std::invalid_argument& ex) {
    expect_contains(
        ex.what(), "direct LIR module",
        "aarch64 direct emitter should reject synthetic single-case constant-selector switch CFG once ownership stays behind the shared BIR boundary");
    return;
  }

  fail("aarch64 direct emitter should reject synthetic single-case constant-selector switch CFG modules once the generic direct-LIR switch fallback is fenced behind BIR");
}

}  // namespace

void run_backend_bir_pipeline_aarch64_tests() {
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_direct_call_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_declared_direct_call_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_void_direct_call_imm_return_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_two_arg_direct_call_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_direct_call_add_imm_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_direct_call_identity_arg_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_dual_identity_direct_call_sub_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_call_crossing_direct_call_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_countdown_loop_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_scalar_global_load_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_extern_scalar_global_load_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_minimal_scalar_global_store_reload_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_minimal_direct_call_through_bir_end_to_end);
  RUN_TEST(test_aarch64_direct_emitter_lowers_minimal_direct_call_via_outer_bir_path);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_minimal_direct_call_with_dead_entry_alloca_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_minimal_void_direct_call_imm_return_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_declared_direct_call_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_minimal_two_arg_direct_call_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_minimal_direct_call_add_imm_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_minimal_direct_call_identity_arg_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_minimal_dual_identity_direct_call_sub_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_minimal_countdown_loop_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_minimal_scalar_global_load_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_minimal_extern_scalar_global_load_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_minimal_global_char_pointer_diff_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_minimal_global_int_pointer_diff_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_minimal_scalar_global_store_reload_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_minimal_call_crossing_direct_call_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_single_param_chain_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_zero_param_staged_constant_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_two_param_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_input_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_staged_affine_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_two_param_add_sub_chain_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_two_param_staged_affine_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_sext_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_zext_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_trunc_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_select_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_minimal_conditional_return_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_conditional_return_with_empty_bridge_blocks_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_conditional_return_with_asymmetric_empty_bridge_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_conditional_return_with_double_empty_bridge_chain_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_conditional_return_with_mirrored_false_double_empty_bridge_chain_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_conditional_return_with_interleaved_double_empty_bridge_chains_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_param_conditional_return_with_interleaved_mixed_depth_bridge_chains_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_conditional_phi_join_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_lir_u8_select_post_join_add_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_single_param_select_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_two_param_select_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_post_join_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_post_join_add_sub_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_post_join_add_sub_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_mixed_affine_post_join_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_mixed_affine_post_join_add_sub_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_mixed_affine_post_join_add_sub_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_deeper_then_mixed_affine_post_join_add_sub_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_deeper_affine_on_both_sides_post_join_add_sub_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_u8_select_mixed_then_deeper_affine_post_join_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_i32_select_mixed_then_deeper_affine_post_join_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_i32_select_mixed_then_deeper_affine_post_join_add_sub_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_aarch64_direct_bir_i32_select_mixed_then_deeper_affine_post_join_add_sub_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_rejects_unsupported_direct_bir_input_on_aarch64);
  RUN_TEST(test_aarch64_direct_lir_emitter_rejects_unsupported_input_without_legacy_backend_ir_stub);
  RUN_TEST(test_aarch64_direct_lir_emitter_rejects_alloca_backed_conditional_phi_fallback);
  RUN_TEST(test_aarch64_direct_emitter_routes_goto_only_constant_return_through_shared_bir);
  RUN_TEST(test_aarch64_direct_lir_emitter_rejects_double_indirect_pointer_conditional_return_fallback);
  RUN_TEST(test_aarch64_direct_lir_emitter_rejects_alloca_backed_switch_fallback);
  RUN_TEST(test_aarch64_direct_lir_emitter_rejects_alloca_backed_single_case_switch_fallback);
  RUN_TEST(test_aarch64_direct_lir_emitter_rejects_constant_selector_switch_fallback);
  RUN_TEST(test_aarch64_direct_lir_emitter_rejects_constant_selector_single_case_switch_fallback);
}
