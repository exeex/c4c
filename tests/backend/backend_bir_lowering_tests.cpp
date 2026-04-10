#include "backend_bir_test_support.hpp"

#include "../../src/backend/bir_printer.hpp"
#include "../../src/backend/bir_validate.hpp"
#include "../../src/backend/lowering/call_decode.hpp"
#include "../../src/backend/lowering/lir_to_bir.hpp"

#include <string>
#include <utility>

namespace {

c4c::codegen::lir::LirModule make_double_countdown_guarded_zero_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { i32, i32, ptr, ptr }");

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "1", "%lv.x"});
  entry.insts.push_back(LirStoreOp{"i32", "10", "%lv.x"});
  entry.terminator = LirBr{"for.cond.1"};

  LirBlock first_cond;
  first_cond.id = LirBlockId{1};
  first_cond.label = "for.cond.1";
  first_cond.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  first_cond.insts.push_back(LirCmpOp{"%t1", false, "ne", "i32", "%t0", "0"});
  first_cond.terminator = LirCondBr{"%t1", "block_1", "block_2"};

  LirBlock first_latch;
  first_latch.id = LirBlockId{2};
  first_latch.label = "for.latch.1";
  first_latch.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  first_latch.insts.push_back(LirBinOp{"%t3", "sub", "i32", "%t2", "1"});
  first_latch.insts.push_back(LirStoreOp{"i32", "%t3", "%lv.x"});
  first_latch.terminator = LirBr{"for.cond.1"};

  LirBlock first_body;
  first_body.id = LirBlockId{3};
  first_body.label = "block_1";
  first_body.terminator = LirBr{"for.latch.1"};

  LirBlock guard;
  guard.id = LirBlockId{4};
  guard.label = "block_2";
  guard.insts.push_back(LirLoadOp{"%t4", "i32", "%lv.x"});
  guard.insts.push_back(LirCmpOp{"%t5", false, "ne", "i32", "%t4", "0"});
  guard.terminator = LirCondBr{"%t5", "block_3", "block_4"};

  LirBlock guarded_return;
  guarded_return.id = LirBlockId{5};
  guarded_return.label = "block_3";
  guarded_return.terminator = LirRet{std::string("1"), "i32"};

  LirBlock second_init;
  second_init.id = LirBlockId{6};
  second_init.label = "block_4";
  second_init.insts.push_back(LirStoreOp{"i32", "10", "%lv.x"});
  second_init.terminator = LirBr{"for.cond.5"};

  LirBlock second_cond;
  second_cond.id = LirBlockId{7};
  second_cond.label = "for.cond.5";
  second_cond.insts.push_back(LirLoadOp{"%t6", "i32", "%lv.x"});
  second_cond.insts.push_back(LirCmpOp{"%t7", false, "ne", "i32", "%t6", "0"});
  second_cond.terminator = LirCondBr{"%t7", "block_5", "block_6"};

  LirBlock second_latch;
  second_latch.id = LirBlockId{8};
  second_latch.label = "for.latch.5";
  second_latch.terminator = LirBr{"for.cond.5"};

  LirBlock second_body;
  second_body.id = LirBlockId{9};
  second_body.label = "block_5";
  second_body.insts.push_back(LirLoadOp{"%t8", "i32", "%lv.x"});
  second_body.insts.push_back(LirBinOp{"%t9", "sub", "i32", "%t8", "1"});
  second_body.insts.push_back(LirStoreOp{"i32", "%t9", "%lv.x"});
  second_body.terminator = LirBr{"for.latch.5"};

  LirBlock exit;
  exit.id = LirBlockId{10};
  exit.label = "block_6";
  exit.insts.push_back(LirLoadOp{"%t10", "i32", "%lv.x"});
  exit.terminator = LirRet{std::string("%t10"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(first_cond));
  function.blocks.push_back(std::move(first_latch));
  function.blocks.push_back(std::move(first_body));
  function.blocks.push_back(std::move(guard));
  function.blocks.push_back(std::move(guarded_return));
  function.blocks.push_back(std::move(second_init));
  function.blocks.push_back(std::move(second_cond));
  function.blocks.push_back(std::move(second_latch));
  function.blocks.push_back(std::move(second_body));
  function.blocks.push_back(std::move(exit));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_minimal_global_char_pointer_diff_lir_module() {
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
  entry.insts.push_back(LirBinOp{"%t8", LirBinaryOpcode::Sub, "i64", "%t6", "%t7"});
  entry.insts.push_back(LirCastOp{"%t9", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirCmpOp{"%t10", false, LirCmpPredicate::Eq, "i64", "%t8", "%t9"});
  entry.insts.push_back(LirCastOp{"%t11", LirCastKind::ZExt, "i1", "%t10", "i32"});
  entry.terminator = LirRet{std::string("%t11"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_minimal_conditional_return_with_empty_bridge_blocks_lir_module() {
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

c4c::codegen::lir::LirModule make_local_i32_store_load_sub_return_immediate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "4", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.insts.push_back(LirBinOp{"%t1", "sub", "i32", "%t0", "4"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_local_i32_arithmetic_chain_return_immediate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "1", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.insts.push_back(LirBinOp{"%t1", "mul", "i32", "%t0", "10"});
  entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  entry.insts.push_back(LirBinOp{"%t3", "sdiv", "i32", "%t2", "2"});
  entry.insts.push_back(LirStoreOp{"i32", "%t3", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t4", "i32", "%lv.x"});
  entry.insts.push_back(LirBinOp{"%t5", "srem", "i32", "%t4", "3"});
  entry.insts.push_back(LirStoreOp{"i32", "%t5", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t6", "i32", "%lv.x"});
  entry.insts.push_back(LirBinOp{"%t7", "sub", "i32", "%t6", "2"});
  entry.terminator = LirRet{std::string("%t7"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_two_local_i32_zero_init_return_first_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.y"});
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_local_i32_pointer_store_zero_load_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.p", "ptr", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "4", "%lv.x"});
  entry.insts.push_back(LirStoreOp{"ptr", "%lv.x", "%lv.p"});
  entry.insts.push_back(LirLoadOp{"%t0", "ptr", "%lv.p"});
  entry.insts.push_back(LirStoreOp{"i32", "0", "%t0"});
  entry.insts.push_back(LirLoadOp{"%t1", "ptr", "%lv.p"});
  entry.insts.push_back(LirLoadOp{"%t2", "i32", "%t1"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_local_i32_pointer_gep_zero_load_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.p", "ptr", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.x"});
  entry.insts.push_back(LirStoreOp{"ptr", "%lv.x", "%lv.p"});
  entry.insts.push_back(LirLoadOp{"%t0", "ptr", "%lv.p"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::SExt, "i32", "0", "i64"});
  entry.insts.push_back(LirGepOp{"%t2", "i32", "%t0", false, {"i64 %t1"}});
  entry.insts.push_back(LirLoadOp{"%t3", "i32", "%t2"});
  entry.terminator = LirRet{std::string("%t3"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_local_i32_pointer_gep_zero_store_slot_load_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.p", "ptr", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "1", "%lv.x"});
  entry.insts.push_back(LirStoreOp{"ptr", "%lv.x", "%lv.p"});
  entry.insts.push_back(LirLoadOp{"%t0", "ptr", "%lv.p"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::SExt, "i32", "0", "i64"});
  entry.insts.push_back(LirGepOp{"%t2", "i32", "%t0", false, {"i64 %t1"}});
  entry.insts.push_back(LirStoreOp{"i32", "0", "%t2"});
  entry.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.x"});
  entry.terminator = LirRet{std::string("%t3"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_local_i32_array_two_slot_sum_sub_three_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.arr", "[2 x i32]", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{"%t0", "[2 x i32]", "%lv.arr", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::SExt, "i32", "0", "i64"});
  entry.insts.push_back(LirGepOp{"%t2", "i32", "%t0", false, {"i64 %t1"}});
  entry.insts.push_back(LirStoreOp{"i32", "1", "%t2"});
  entry.insts.push_back(LirGepOp{"%t3", "[2 x i32]", "%lv.arr", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t4", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t5", "i32", "%t3", false, {"i64 %t4"}});
  entry.insts.push_back(LirStoreOp{"i32", "2", "%t5"});
  entry.insts.push_back(LirGepOp{"%t6", "[2 x i32]", "%lv.arr", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t7", LirCastKind::SExt, "i32", "0", "i64"});
  entry.insts.push_back(LirGepOp{"%t8", "i32", "%t6", false, {"i64 %t7"}});
  entry.insts.push_back(LirLoadOp{"%t9", "i32", "%t8"});
  entry.insts.push_back(LirGepOp{"%t10", "[2 x i32]", "%lv.arr", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t11", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t12", "i32", "%t10", false, {"i64 %t11"}});
  entry.insts.push_back(LirLoadOp{"%t13", "i32", "%t12"});
  entry.insts.push_back(LirBinOp{"%t14", "add", "i32", "%t9", "%t13"});
  entry.insts.push_back(LirBinOp{"%t15", "sub", "i32", "%t14", "3"});
  entry.terminator = LirRet{std::string("%t15"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_local_i32_array_second_slot_pointer_store_zero_load_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.arr", "[2 x i32]", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.p", "ptr", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{"%t0", "[2 x i32]", "%lv.arr", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t2", "i32", "%t0", false, {"i64 %t1"}});
  entry.insts.push_back(LirStoreOp{"ptr", "%t2", "%lv.p"});
  entry.insts.push_back(LirLoadOp{"%t3", "ptr", "%lv.p"});
  entry.insts.push_back(LirStoreOp{"i32", "0", "%t3"});
  entry.insts.push_back(LirGepOp{"%t4", "[2 x i32]", "%lv.arr", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t5", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t6", "i32", "%t4", false, {"i64 %t5"}});
  entry.insts.push_back(LirLoadOp{"%t7", "i32", "%t6"});
  entry.terminator = LirRet{std::string("%t7"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_local_two_field_struct_sub_sub_two_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  module.type_decls.push_back("%struct.S = type { i32, i32 }");

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.s", "%struct.S", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{"%t0", "%struct.S", "%lv.s", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(LirStoreOp{"i32", "3", "%t0"});
  entry.insts.push_back(LirGepOp{"%t1", "%struct._anon_0", "%lv.s", false, {"i32 0", "i32 1"}});
  entry.insts.push_back(LirStoreOp{"i32", "5", "%t1"});
  entry.insts.push_back(LirGepOp{"%t2", "%struct._anon_0", "%lv.s", false, {"i32 0", "i32 1"}});
  entry.insts.push_back(LirLoadOp{"%t3", "i32", "%t2"});
  entry.insts.push_back(LirGepOp{"%t4", "%struct._anon_0", "%lv.s", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(LirLoadOp{"%t5", "i32", "%t4"});
  entry.insts.push_back(LirBinOp{"%t6", "sub", "i32", "%t3", "%t5"});
  entry.insts.push_back(LirBinOp{"%t7", "sub", "i32", "%t6", "2"});
  entry.terminator = LirRet{std::string("%t7"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_local_struct_pointer_alias_add_sub_three_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  module.type_decls.push_back("%struct.S = type { i32, i32 }");

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.s", "%struct.S", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.p", "ptr", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"ptr", "%lv.s", "%lv.p"});
  entry.insts.push_back(LirGepOp{"%t0", "%struct.S", "%lv.s", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(LirStoreOp{"i32", "1", "%t0"});
  entry.insts.push_back(LirLoadOp{"%t1", "ptr", "%lv.p"});
  entry.insts.push_back(LirGepOp{"%t2", "%struct.S", "%t1", false, {"i32 0", "i32 1"}});
  entry.insts.push_back(LirStoreOp{"i32", "2", "%t2"});
  entry.insts.push_back(LirLoadOp{"%t3", "ptr", "%lv.p"});
  entry.insts.push_back(LirGepOp{"%t4", "%struct.S", "%t3", false, {"i32 0", "i32 1"}});
  entry.insts.push_back(LirLoadOp{"%t5", "i32", "%t4"});
  entry.insts.push_back(LirLoadOp{"%t6", "ptr", "%lv.p"});
  entry.insts.push_back(LirGepOp{"%t7", "%struct.S", "%t6", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(LirLoadOp{"%t8", "i32", "%t7"});
  entry.insts.push_back(LirBinOp{"%t9", "add", "i32", "%t5", "%t8"});
  entry.insts.push_back(LirBinOp{"%t10", "sub", "i32", "%t9", "3"});
  entry.terminator = LirRet{std::string("%t10"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_local_self_referential_struct_pointer_chain_zero_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  module.type_decls.push_back("%struct.S = type { ptr, i32, [4 x i8] }");

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.s", "%struct.S", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{"%t0", "%struct.S", "%lv.s", false, {"i32 0", "i32 1"}});
  entry.insts.push_back(LirStoreOp{"i32", "0", "%t0"});
  entry.insts.push_back(LirGepOp{"%t1", "%struct.S", "%lv.s", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(LirStoreOp{"ptr", "%lv.s", "%t1"});
  entry.insts.push_back(LirGepOp{"%t2", "%struct.S", "%lv.s", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(LirLoadOp{"%t3", "ptr", "%t2"});
  entry.insts.push_back(LirGepOp{"%t4", "%struct.S", "%t3", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(LirLoadOp{"%t5", "ptr", "%t4"});
  entry.insts.push_back(LirGepOp{"%t6", "%struct.S", "%t5", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(LirLoadOp{"%t7", "ptr", "%t6"});
  entry.insts.push_back(LirGepOp{"%t8", "%struct.S", "%t7", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(LirLoadOp{"%t9", "ptr", "%t8"});
  entry.insts.push_back(LirGepOp{"%t10", "%struct.S", "%t9", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(LirLoadOp{"%t11", "ptr", "%t10"});
  entry.insts.push_back(LirGepOp{"%t12", "%struct.S", "%t11", false, {"i32 0", "i32 1"}});
  entry.insts.push_back(LirLoadOp{"%t13", "i32", "%t12"});
  entry.terminator = LirRet{std::string("%t13"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_double_indirect_local_pointer_chain_zero_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

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
  entry.insts.push_back(LirLoadOp{"%t0", "ptr", "%lv.pp"});
  entry.insts.push_back(LirLoadOp{"%t1", "ptr", "%t0"});
  entry.insts.push_back(LirLoadOp{"%t2", "i32", "%t1"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_double_indirect_local_store_one_final_branch_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

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

  LirBlock first_ret;
  first_ret.id = LirBlockId{1};
  first_ret.label = "block_1";
  first_ret.terminator = LirRet{std::string("1"), "i32"};

  LirBlock second_check;
  second_check.id = LirBlockId{2};
  second_check.label = "block_2";
  second_check.insts.push_back(LirLoadOp{"%t3", "ptr", "%lv.pp"});
  second_check.insts.push_back(LirLoadOp{"%t4", "ptr", "%t3"});
  second_check.insts.push_back(LirLoadOp{"%t5", "i32", "%t4"});
  second_check.insts.push_back(LirCmpOp{"%t6", false, "ne", "i32", "%t5", "0"});
  second_check.terminator = LirCondBr{"%t6", "block_3", "block_4"};

  LirBlock second_ret;
  second_ret.id = LirBlockId{3};
  second_ret.label = "block_3";
  second_ret.terminator = LirRet{std::string("1"), "i32"};

  LirBlock store_one;
  store_one.id = LirBlockId{4};
  store_one.label = "block_4";
  store_one.insts.push_back(LirLoadOp{"%t7", "ptr", "%lv.pp"});
  store_one.insts.push_back(LirLoadOp{"%t8", "ptr", "%t7"});
  store_one.insts.push_back(LirStoreOp{"i32", "1", "%t8"});
  store_one.terminator = LirBr{"block_5"};

  LirBlock final_check;
  final_check.id = LirBlockId{5};
  final_check.label = "block_5";
  final_check.insts.push_back(LirLoadOp{"%t9", "i32", "%lv.x"});
  final_check.insts.push_back(LirCmpOp{"%t10", false, "ne", "i32", "%t9", "0"});
  final_check.terminator = LirCondBr{"%t10", "block_6", "block_7"};

  LirBlock final_true_ret;
  final_true_ret.id = LirBlockId{6};
  final_true_ret.label = "block_6";
  final_true_ret.terminator = LirRet{std::string("0"), "i32"};

  LirBlock final_false_ret;
  final_false_ret.id = LirBlockId{7};
  final_false_ret.label = "block_7";
  final_false_ret.terminator = LirRet{std::string("1"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(first_ret));
  function.blocks.push_back(std::move(second_check));
  function.blocks.push_back(std::move(second_ret));
  function.blocks.push_back(std::move(store_one));
  function.blocks.push_back(std::move(final_check));
  function.blocks.push_back(std::move(final_true_ret));
  function.blocks.push_back(std::move(final_false_ret));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_minimal_conditional_return_with_asymmetric_empty_bridge_lir_module() {
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

c4c::codegen::lir::LirModule
make_bir_minimal_conditional_return_with_double_empty_bridge_chain_lir_module() {
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
make_bir_minimal_conditional_return_with_mirrored_false_double_empty_bridge_chain_lir_module() {
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
make_bir_minimal_conditional_return_with_interleaved_double_empty_bridge_chains_lir_module() {
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
make_bir_param_conditional_return_with_interleaved_mixed_depth_bridge_chains_lir_module() {
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

c4c::codegen::lir::LirModule make_bir_minimal_global_int_pointer_diff_lir_module() {
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
  entry.insts.push_back(LirBinOp{"%t8", LirBinaryOpcode::Sub, "i64", "%t6", "%t7"});
  entry.insts.push_back(LirBinOp{"%t9", LirBinaryOpcode::SDiv, "i64", "%t8", "4"});
  entry.insts.push_back(LirCastOp{"%t10", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirCmpOp{"%t11", false, LirCmpPredicate::Eq, "i64", "%t9", "%t10"});
  entry.insts.push_back(LirCastOp{"%t12", LirCastKind::ZExt, "i1", "%t11", "i32"});
  entry.terminator = LirRet{std::string("%t12"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_minimal_countdown_do_while_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "50", "%lv.x"});
  entry.terminator = LirBr{"body"};

  LirBlock body;
  body.id = LirBlockId{1};
  body.label = "body";
  body.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  body.insts.push_back(LirBinOp{"%t1", "sub", "i32", "%t0", "1"});
  body.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.x"});
  body.terminator = LirBr{"bridge"};

  LirBlock bridge;
  bridge.id = LirBlockId{2};
  bridge.label = "bridge";
  bridge.terminator = LirBr{"cond"};

  LirBlock cond;
  cond.id = LirBlockId{3};
  cond.label = "cond";
  cond.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  cond.insts.push_back(LirCmpOp{"%t3", false, "ne", "i32", "%t2", "0"});
  cond.terminator = LirCondBr{"%t3", "body", "exit"};

  LirBlock exit;
  exit.id = LirBlockId{4};
  exit.label = "exit";
  exit.insts.push_back(LirLoadOp{"%t4", "i32", "%lv.x"});
  exit.terminator = LirRet{std::string("%t4"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(body));
  function.blocks.push_back(std::move(bridge));
  function.blocks.push_back(std::move(cond));
  function.blocks.push_back(std::move(exit));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_minimal_goto_only_constant_return_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

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

  LirBlock middle;
  middle.id = LirBlockId{2};
  middle.label = "block_1";
  middle.terminator = LirBr{"ulbl_next"};

  LirBlock next;
  next.id = LirBlockId{3};
  next.label = "ulbl_next";
  next.terminator = LirRet{std::string("9"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(start));
  function.blocks.push_back(std::move(middle));
  function.blocks.push_back(std::move(next));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_minimal_scalar_global_load_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
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

c4c::codegen::lir::LirModule make_bir_minimal_repeated_zero_arg_call_compare_zero_return_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction helper;
  helper.name = "f";
  helper.signature_text = "define i32 @f()\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.terminator = LirRet{std::string("100"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_function;
  main_function.name = "main";
  main_function.signature_text = "define i32 @main()\n";
  main_function.entry = LirBlockId{0};

  auto append_compare_block = [](LirFunction& function,
                                 int block_id,
                                 std::string label,
                                 std::string call_result,
                                 std::string cmp_result,
                                 std::string zext_result,
                                 std::string branch_result,
                                 std::string predicate,
                                 std::string lhs,
                                 std::string rhs,
                                 std::string true_label,
                                 std::string false_label) {
    LirBlock block;
    block.id = LirBlockId{static_cast<unsigned>(block_id)};
    block.label = std::move(label);
    block.insts.push_back(LirCallOp{std::move(call_result), "i32", "@f", "", ""});
    const auto& call = std::get<LirCallOp>(block.insts.back());
    block.insts.push_back(
        LirCmpOp{std::move(cmp_result), false, std::move(predicate), "i32", std::move(lhs), std::move(rhs)});
    const auto& cmp = std::get<LirCmpOp>(block.insts.back());
    block.insts.push_back(LirCastOp{std::move(zext_result), LirCastKind::ZExt, "i1", cmp.result.str(), "i32"});
    const auto& zext = std::get<LirCastOp>(block.insts.back());
    block.insts.push_back(LirCmpOp{std::move(branch_result), false, "ne", "i32", zext.result.str(), "0"});
    const auto& branch_cmp = std::get<LirCmpOp>(block.insts.back());
    block.terminator = LirCondBr{branch_cmp.result.str(), std::move(true_label), std::move(false_label)};
    function.blocks.push_back(std::move(block));
  };

  append_compare_block(main_function, 0, "entry", "%t0", "%t1", "%t2", "%t3",
                       "sgt", "%t0", "1000", "block_2", "block_3");

  LirBlock block_2;
  block_2.id = LirBlockId{1};
  block_2.label = "block_2";
  block_2.terminator = LirRet{std::string("1"), "i32"};
  main_function.blocks.push_back(std::move(block_2));

  append_compare_block(main_function, 2, "block_3", "%t4", "%t5", "%t6", "%t7",
                       "sge", "%t4", "1000", "block_4", "block_5");

  LirBlock block_4;
  block_4.id = LirBlockId{3};
  block_4.label = "block_4";
  block_4.terminator = LirRet{std::string("1"), "i32"};
  main_function.blocks.push_back(std::move(block_4));

  append_compare_block(main_function, 4, "block_5", "%t8", "%t9", "%t10", "%t11",
                       "slt", "1000", "%t8", "block_6", "block_7");

  LirBlock block_6;
  block_6.id = LirBlockId{5};
  block_6.label = "block_6";
  block_6.terminator = LirRet{std::string("1"), "i32"};
  main_function.blocks.push_back(std::move(block_6));

  append_compare_block(main_function, 6, "block_7", "%t12", "%t13", "%t14", "%t15",
                       "sle", "1000", "%t12", "block_8", "block_9");

  LirBlock block_8;
  block_8.id = LirBlockId{7};
  block_8.label = "block_8";
  block_8.terminator = LirRet{std::string("1"), "i32"};
  main_function.blocks.push_back(std::move(block_8));

  append_compare_block(main_function, 8, "block_9", "%t16", "%t17", "%t18", "%t19",
                       "eq", "1000", "%t16", "block_10", "block_11");

  LirBlock block_10;
  block_10.id = LirBlockId{9};
  block_10.label = "block_10";
  block_10.terminator = LirRet{std::string("1"), "i32"};
  main_function.blocks.push_back(std::move(block_10));

  append_compare_block(main_function, 10, "block_11", "%t20", "%t21", "%t22", "%t23",
                       "ne", "100", "%t20", "block_12", "block_13");

  LirBlock block_12;
  block_12.id = LirBlockId{11};
  block_12.label = "block_12";
  block_12.terminator = LirRet{std::string("1"), "i32"};
  main_function.blocks.push_back(std::move(block_12));

  LirBlock block_13;
  block_13.id = LirBlockId{12};
  block_13.label = "block_13";
  block_13.terminator = LirRet{std::string("0"), "i32"};
  main_function.blocks.push_back(std::move(block_13));

  module.functions.push_back(std::move(main_function));
  module.functions.push_back(std::move(helper));
  return module;
}

c4c::codegen::lir::LirModule make_bir_minimal_string_literal_compare_phi_return_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  module.string_pool.push_back(LirStringConst{"@.str0", "hi", 3});

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.a", "ptr", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "[3 x i8]", "@.str0", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirStoreOp{"ptr", "%t0", "%lv.a"});
  entry.insts.push_back(LirLoadOp{"%t1", "ptr", "%lv.a"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t3", "i8", "%t1", false, {"i64 %t2"}});
  entry.insts.push_back(LirLoadOp{"%t4", "i8", "%t3"});
  entry.insts.push_back(LirCastOp{"%t5", LirCastKind::SExt, "i8", "%t4", "i32"});
  entry.insts.push_back(LirCmpOp{"%t6", false, "eq", "i32", "%t5", "105"});
  entry.insts.push_back(LirCastOp{"%t7", LirCastKind::ZExt, "i1", "%t6", "i32"});
  entry.insts.push_back(LirCmpOp{"%t8", false, "ne", "i32", "%t7", "0"});
  entry.terminator = LirCondBr{"%t8", "tern.then.9", "tern.else.11"};

  LirBlock then_block;
  then_block.id = LirBlockId{1};
  then_block.label = "tern.then.9";
  then_block.terminator = LirBr{"tern.then.end.10"};

  LirBlock then_end;
  then_end.id = LirBlockId{2};
  then_end.label = "tern.then.end.10";
  then_end.terminator = LirBr{"tern.end.13"};

  LirBlock else_block;
  else_block.id = LirBlockId{3};
  else_block.label = "tern.else.11";
  else_block.terminator = LirBr{"tern.else.end.12"};

  LirBlock else_end;
  else_end.id = LirBlockId{4};
  else_end.label = "tern.else.end.12";
  else_end.terminator = LirBr{"tern.end.13"};

  LirBlock join;
  join.id = LirBlockId{5};
  join.label = "tern.end.13";
  join.insts.push_back(
      LirPhiOp{"%t14", "i32", {{"0", "tern.then.end.10"}, {"1", "tern.else.end.12"}}});
  join.terminator = LirRet{std::string("%t14"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(then_end));
  function.blocks.push_back(std::move(else_block));
  function.blocks.push_back(std::move(else_end));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_minimal_extern_scalar_global_load_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
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

c4c::codegen::lir::LirModule make_bir_minimal_extern_global_array_load_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
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

c4c::codegen::lir::LirModule make_bir_minimal_scalar_global_store_reload_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
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

c4c::codegen::lir::LirModule make_bir_minimal_global_two_field_struct_store_sub_sub_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  module.type_decls.push_back("%struct._anon_0 = type { i32, i32 }");
  module.globals.push_back(LirGlobal{
      LirGlobalId{0},
      "v",
      {},
      false,
      false,
      "",
      "global ",
      "%struct._anon_0",
      "{ i32 zeroinitializer, i32 zeroinitializer }",
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
  entry.insts.push_back(LirGepOp{"%t0", "%struct._anon_0", "@v", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(LirStoreOp{"i32", "1", "%t0"});
  entry.insts.push_back(LirGepOp{"%t1", "%struct._anon_0", "@v", false, {"i32 0", "i32 1"}});
  entry.insts.push_back(LirStoreOp{"i32", "2", "%t1"});
  entry.insts.push_back(LirGepOp{"%t2", "%struct._anon_0", "@v", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(LirLoadOp{"%t3", "i32", "%t2"});
  entry.insts.push_back(LirBinOp{"%t4", "sub", "i32", "3", "%t3"});
  entry.insts.push_back(LirGepOp{"%t5", "%struct._anon_0", "@v", false, {"i32 0", "i32 1"}});
  entry.insts.push_back(LirLoadOp{"%t6", "i32", "%t5"});
  entry.insts.push_back(LirBinOp{"%t7", "sub", "i32", "%t4", "%t6"});
  entry.terminator = LirRet{std::string("%t7"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_minimal_direct_call_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

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

c4c::codegen::lir::LirModule make_bir_declared_direct_call_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
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

c4c::codegen::lir::LirModule make_bir_string_literal_strlen_sub_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  module.string_pool.push_back(LirStringConst{"@.str0", "hello", 6});

  LirFunction callee;
  callee.name = "strlen";
  callee.is_declaration = true;
  callee.signature_text = "declare i32 @strlen(ptr)\n";
  callee.return_type.base = c4c::TB_INT;
  callee.params.push_back({"%arg0", c4c::TypeSpec{.base = c4c::TB_CHAR, .ptr_level = 1}});

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.p", "ptr", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{"%t0", "[6 x i8]", "@.str0", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirStoreOp{"ptr", "%t0", "%lv.p"});
  entry.insts.push_back(LirLoadOp{"%t1", "ptr", "%lv.p"});
  entry.insts.push_back(LirCallOp{"%t2", "i32", "@strlen", "(ptr)", "ptr %t1"});
  entry.insts.push_back(LirBinOp{"%t3", "sub", "i32", "%t2", "5"});
  entry.terminator = LirRet{std::string("%t3"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(callee));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_string_literal_char_sub_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  module.string_pool.push_back(LirStringConst{"@.str0", "hello", 6});

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.p", "ptr", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{"%t0", "[6 x i8]", "@.str0", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirStoreOp{"ptr", "%t0", "%lv.p"});
  entry.insts.push_back(LirLoadOp{"%t1", "ptr", "%lv.p"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::SExt, "i32", "0", "i64"});
  entry.insts.push_back(LirGepOp{"%t3", "i8", "%t1", false, {"i64 %t2"}});
  entry.insts.push_back(LirLoadOp{"%t4", "i8", "%t3"});
  entry.insts.push_back(LirCastOp{"%t5", LirCastKind::SExt, "i8", "%t4", "i32"});
  entry.insts.push_back(LirBinOp{"%t6", "sub", "i32", "%t5", "104"});
  entry.terminator = LirRet{std::string("%t6"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_local_i32_store_or_sub_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "1", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.insts.push_back(LirBinOp{"%t1", "or", "i32", "%t0", "4"});
  entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  entry.insts.push_back(LirBinOp{"%t3", "sub", "i32", "%t2", "5"});
  entry.terminator = LirRet{std::string("%t3"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_local_i32_store_and_sub_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "1", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.insts.push_back(LirBinOp{"%t1", "and", "i32", "%t0", "3"});
  entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  entry.insts.push_back(LirBinOp{"%t3", "sub", "i32", "%t2", "1"});
  entry.terminator = LirRet{std::string("%t3"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_local_i32_store_xor_sub_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "1", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.insts.push_back(LirBinOp{"%t1", "xor", "i32", "%t0", "3"});
  entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  entry.insts.push_back(LirBinOp{"%t3", "sub", "i32", "%t2", "2"});
  entry.terminator = LirRet{std::string("%t3"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_declared_direct_call_inferred_params_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  module.extern_decls.push_back(LirExternDecl{"helper_ext", "i32"});

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@helper_ext", "", "i32 5, i32 7"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_declared_direct_call_typed_decl_metadata_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction callee;
  callee.name = "helper_decl";
  callee.is_declaration = true;
  callee.signature_text = "declare i64 @helper_decl()\n";
  callee.return_type.base = c4c::TB_INT;

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i64 @main()\n";
  function.return_type.base = c4c::TB_INT;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      "%t0",
      LirTypeRef{"i64", LirTypeKind::Integer, 32},
      "@helper_decl",
      "",
      "",
  });
  entry.terminator = LirRet{std::string("%t0"), "i64"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  module.functions.push_back(std::move(callee));
  return module;
}

c4c::codegen::lir::LirModule make_bir_declared_direct_call_typed_extern_metadata_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirExternDecl callee;
  callee.name = "helper_ext";
  callee.return_type_str = "i64";
  callee.return_type = LirTypeRef{"i64", LirTypeKind::Integer, 32};
  module.extern_decls.push_back(std::move(callee));

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i64 @main()\n";
  function.return_type.base = c4c::TB_INT;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      "%t0",
      LirTypeRef{"i64", LirTypeKind::Integer, 32},
      "@helper_ext",
      "",
      "i32 5, i32 7",
  });
  entry.terminator = LirRet{std::string("%t0"), "i64"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_declared_direct_call_vararg_fixed_param_typed_match_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction callee;
  callee.name = "helper_decl";
  callee.is_declaration = true;
  callee.signature_text = "declare i32 @helper_decl(i32 %arg0, ...)\n";
  callee.return_type.base = c4c::TB_INT;

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.return_type.base = c4c::TB_INT;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      "%t0",
      LirTypeRef{"i32", LirTypeKind::Integer, 32},
      "@helper_decl",
      "(i8, ...)",
      "i32 5",
  });
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  module.functions.push_back(std::move(callee));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_declared_direct_call_zero_arg_vararg_typed_decl_metadata_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction callee;
  callee.name = "helper_decl";
  callee.is_declaration = true;
  callee.signature_text = "declare i64 @helper_decl(...)\n";
  callee.return_type.base = c4c::TB_INT;

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.return_type.base = c4c::TB_INT;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      "%t0",
      LirTypeRef{"i32", LirTypeKind::Integer, 32},
      "@helper_decl",
      "(...)",
      "",
  });
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  module.functions.push_back(std::move(callee));
  return module;
}

c4c::codegen::lir::LirModule make_bir_minimal_void_direct_call_imm_return_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

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

c4c::codegen::lir::LirModule make_bir_minimal_two_arg_direct_call_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

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

c4c::codegen::lir::LirModule make_bir_minimal_two_arg_direct_call_local_slot_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

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
  main_function.alloca_insts.push_back(LirAllocaOp{"%slot", "i32", "", 0});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "5", "%slot"});
  entry.insts.push_back(LirLoadOp{"%loaded_lhs", "i32", "%slot"});
  entry.insts.push_back(
      LirCallOp{"%t0", "i32", "@add_pair", "(i32, i32)", "i32 %loaded_lhs, i32 7"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(main_function));
  module.functions.push_back(std::move(helper));
  return module;
}

c4c::codegen::lir::LirTypeRef make_test_stale_text_i32_lir_type() {
  return c4c::codegen::lir::LirTypeRef("i8", c4c::codegen::lir::LirTypeKind::Integer, 32);
}

c4c::codegen::lir::LirTypeRef make_test_stale_text_i8_lir_type() {
  return c4c::codegen::lir::LirTypeRef("i32", c4c::codegen::lir::LirTypeKind::Integer, 8);
}

c4c::codegen::lir::LirTypeRef make_test_stale_text_i1_lir_type() {
  return c4c::codegen::lir::LirTypeRef("i8", c4c::codegen::lir::LirTypeKind::Integer, 1);
}

c4c::codegen::lir::LirTypeRef make_test_stale_text_i64_lir_type() {
  return c4c::codegen::lir::LirTypeRef("i32", c4c::codegen::lir::LirTypeKind::Integer, 64);
}

c4c::codegen::lir::LirTypeRef make_test_stale_text_ptr_lir_type() {
  return c4c::codegen::lir::LirTypeRef("i32", c4c::codegen::lir::LirTypeKind::Pointer);
}

void make_test_stale_typed_i32_main_return(c4c::codegen::lir::LirFunction& function) {
  using namespace c4c::codegen::lir;

  function.return_type.base = c4c::TB_INT;
  function.signature_text = "define i8 @" + function.name + "()\n";

  auto& ret = std::get<LirRet>(function.blocks.front().terminator);
  ret.type_str = make_test_stale_text_i32_lir_type();
}

void make_test_stale_typed_i32_helper_return(c4c::codegen::lir::LirFunction& function) {
  using namespace c4c::codegen::lir;

  function.return_type.base = c4c::TB_INT;
  function.signature_text = "define i8 @" + function.name + "()\n";

  auto& ret = std::get<LirRet>(function.blocks.front().terminator);
  ret.type_str = make_test_stale_text_i32_lir_type();
}

c4c::codegen::lir::LirModule make_bir_minimal_direct_call_lir_module_with_typed_helper_return() {
  auto module = make_bir_minimal_direct_call_lir_module();
  make_test_stale_typed_i32_helper_return(module.functions[1]);
  return module;
}

c4c::codegen::lir::LirModule make_bir_minimal_direct_call_lir_module_with_typed_main_return() {
  auto module = make_bir_minimal_direct_call_lir_module();
  make_test_stale_typed_i32_main_return(module.functions[0]);
  return module;
}

c4c::codegen::lir::LirModule
make_bir_minimal_two_arg_direct_call_lir_module_with_typed_helper_params() {
  using namespace c4c::codegen::lir;

  auto module = make_bir_minimal_two_arg_direct_call_lir_module();
  auto& helper = module.functions[1];
  helper.return_type.base = c4c::TB_INT;
  helper.params.clear();

  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  helper.params.push_back({"%lhs", param_type});
  helper.params.push_back({"%rhs", param_type});
  helper.signature_text = "define i32 @add_pair(i8 %lhs, i8 %rhs)\n";

  auto& helper_entry = helper.blocks.front();
  auto& add = std::get<LirBinOp>(helper_entry.insts.front());
  add.type_str = make_test_stale_text_i32_lir_type();

  return module;
}

c4c::codegen::lir::LirModule
make_bir_minimal_two_arg_direct_call_lir_module_with_typed_main_return() {
  auto module = make_bir_minimal_two_arg_direct_call_lir_module();
  make_test_stale_typed_i32_main_return(module.functions[0]);
  return module;
}

c4c::codegen::lir::LirModule
make_bir_minimal_two_arg_direct_call_local_slot_lir_module_with_stale_typed_slot_text() {
  auto module = make_bir_minimal_two_arg_direct_call_local_slot_lir_module();
  auto& main_function = module.functions[0];
  std::get<c4c::codegen::lir::LirAllocaOp>(main_function.alloca_insts.front()).type_str =
      make_test_stale_text_i32_lir_type();

  auto& entry = main_function.blocks.front();
  std::get<c4c::codegen::lir::LirStoreOp>(entry.insts[0]).type_str =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirLoadOp>(entry.insts[1]).type_str =
      make_test_stale_text_i32_lir_type();
  return module;
}

c4c::codegen::lir::LirModule make_bir_minimal_direct_call_add_imm_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

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

c4c::codegen::lir::LirModule
make_bir_minimal_direct_call_add_imm_lir_module_with_typed_helper_param() {
  using namespace c4c::codegen::lir;

  auto module = make_bir_minimal_direct_call_add_imm_lir_module();
  auto& helper = module.functions[1];
  helper.return_type.base = c4c::TB_INT;
  helper.params.clear();

  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  helper.params.push_back({"%arg0", param_type});
  helper.signature_text = "define i32 @add_one(i8 %arg0)\n";

  auto& helper_entry = helper.blocks.front();
  auto& add = std::get<LirBinOp>(helper_entry.insts.front());
  add.type_str = make_test_stale_text_i32_lir_type();

  return module;
}

c4c::codegen::lir::LirModule
make_bir_minimal_direct_call_add_imm_lir_module_with_typed_main_return() {
  auto module = make_bir_minimal_direct_call_add_imm_lir_module();
  make_test_stale_typed_i32_main_return(module.functions[0]);
  return module;
}

c4c::codegen::lir::LirModule
make_bir_minimal_direct_call_add_imm_lir_module_with_stale_typed_local_slot_text() {
  using namespace c4c::codegen::lir;

  auto module = make_bir_minimal_direct_call_add_imm_lir_module();
  auto& main_function = module.functions[0];
  main_function.alloca_insts.push_back(
      LirAllocaOp{"%slot", make_test_stale_text_i32_lir_type(), "", 4});

  auto& entry = main_function.blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirStoreOp{make_test_stale_text_i32_lir_type(), "5", "%slot"});
  entry.insts.push_back(LirLoadOp{"%t_load", make_test_stale_text_i32_lir_type(), "%slot"});
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@add_one", "(i32)", "i32 %t_load"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule
make_bir_minimal_direct_call_add_imm_slot_lir_module_with_typed_helper_param() {
  using namespace c4c::codegen::lir;

  auto module = make_bir_minimal_direct_call_add_imm_lir_module();
  auto& helper = module.functions[1];
  helper.return_type.base = c4c::TB_INT;
  helper.params.clear();

  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  helper.params.push_back({"%arg0", param_type});
  helper.signature_text = "define i32 @add_one(i8 %arg0)\n";
  helper.alloca_insts.push_back(LirAllocaOp{"%slot", make_test_stale_text_i32_lir_type(), "", 4});
  helper.alloca_insts.push_back(LirStoreOp{make_test_stale_text_i32_lir_type(), "%arg0", "%slot"});

  auto& helper_entry = helper.blocks.front();
  helper_entry.insts.clear();
  helper_entry.insts.push_back(LirLoadOp{"%t0", make_test_stale_text_i32_lir_type(), "%slot"});
  helper_entry.insts.push_back(LirBinOp{"%sum", LirBinaryOpcode::Add, make_test_stale_text_i32_lir_type(),
                                        "%t0", "1"});
  helper_entry.insts.push_back(LirStoreOp{make_test_stale_text_i32_lir_type(), "%sum", "%slot"});
  helper_entry.insts.push_back(LirLoadOp{"%t1", make_test_stale_text_i32_lir_type(), "%slot"});
  helper_entry.terminator = LirRet{std::string("%t1"), "i32"};

  return module;
}

c4c::codegen::lir::LirModule make_bir_minimal_direct_call_identity_arg_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

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
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@f", "(i32)", "i32 0"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(main_function));
  module.functions.push_back(std::move(helper));
  return module;
}

c4c::codegen::lir::LirModule make_bir_minimal_direct_call_identity_arg_lir_module_helper_first() {
  auto module = make_bir_minimal_direct_call_identity_arg_lir_module();
  std::swap(module.functions[0], module.functions[1]);
  return module;
}

c4c::codegen::lir::LirModule
make_bir_minimal_direct_call_identity_arg_lir_module_with_typed_helper_param() {
  using namespace c4c::codegen::lir;

  auto module = make_bir_minimal_direct_call_identity_arg_lir_module();
  auto& helper = module.functions[1];
  helper.return_type.base = c4c::TB_INT;
  helper.params.clear();

  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  helper.params.push_back({"%arg0", param_type});
  helper.signature_text = "define i32 @f(i8 %arg0)\n";

  return module;
}

c4c::codegen::lir::LirModule
make_bir_minimal_direct_call_identity_arg_lir_module_with_typed_main_return() {
  auto module = make_bir_minimal_direct_call_identity_arg_lir_module();
  make_test_stale_typed_i32_main_return(module.functions[0]);
  return module;
}

c4c::codegen::lir::LirModule make_bir_minimal_folded_two_arg_direct_call_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction helper;
  helper.name = "fold_pair";
  helper.signature_text = "define i32 @fold_pair(i32 %lhs, i32 %rhs)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(LirBinOp{"%t0", LirBinaryOpcode::Add, "i32", "10", "%lhs"});
  helper_entry.insts.push_back(LirBinOp{"%t1", LirBinaryOpcode::Sub, "i32", "%t0", "%rhs"});
  helper_entry.terminator = LirRet{std::string("%t1"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_function;
  main_function.name = "main";
  main_function.signature_text = "define i32 @main()\n";
  main_function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@fold_pair", "(i32, i32)", "i32 5, i32 7"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(main_function));
  module.functions.push_back(std::move(helper));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_minimal_folded_two_arg_direct_call_lir_module_with_typed_helper_params() {
  using namespace c4c::codegen::lir;

  auto module = make_bir_minimal_folded_two_arg_direct_call_lir_module();
  auto& helper = module.functions[1];
  helper.return_type.base = c4c::TB_INT;
  helper.params.clear();

  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  helper.params.push_back({"%lhs", param_type});
  helper.params.push_back({"%rhs", param_type});

  helper.signature_text = "define i32 @fold_pair(i8 %lhs, i8 %rhs)\n";

  auto& helper_entry = helper.blocks.front();
  auto& add = std::get<LirBinOp>(helper_entry.insts[0]);
  auto& sub = std::get<LirBinOp>(helper_entry.insts[1]);
  add.type_str = make_test_stale_text_i32_lir_type();
  sub.type_str = make_test_stale_text_i32_lir_type();

  return module;
}

c4c::codegen::lir::LirModule make_bir_minimal_folded_two_arg_direct_call_lir_module_helper_first() {
  auto module = make_bir_minimal_folded_two_arg_direct_call_lir_module();
  std::swap(module.functions[0], module.functions[1]);
  return module;
}

c4c::codegen::lir::LirModule make_bir_minimal_dual_identity_direct_call_sub_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

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

c4c::codegen::lir::LirModule
make_bir_minimal_dual_identity_direct_call_sub_lir_module_helper_first() {
  auto module = make_bir_minimal_dual_identity_direct_call_sub_lir_module();
  std::swap(module.functions[0], module.functions[2]);
  return module;
}

c4c::codegen::lir::LirModule
make_bir_minimal_dual_identity_direct_call_sub_lir_module_with_typed_helper_params() {
  using namespace c4c::codegen::lir;

  auto module = make_bir_minimal_dual_identity_direct_call_sub_lir_module();

  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;

  auto set_typed_identity_helper = [&](LirFunction& helper) {
    helper.return_type.base = c4c::TB_INT;
    helper.params.clear();
    helper.params.push_back({"%arg0", param_type});
    helper.signature_text = "define i32 @" + helper.name + "(i8 %arg0)\n";
  };

  set_typed_identity_helper(module.functions[0]);
  set_typed_identity_helper(module.functions[1]);
  return module;
}

c4c::codegen::lir::LirModule
make_bir_minimal_dual_identity_direct_call_sub_lir_module_with_typed_main_return() {
  auto module = make_bir_minimal_dual_identity_direct_call_sub_lir_module();
  make_test_stale_typed_i32_main_return(module.functions[2]);
  return module;
}

c4c::codegen::lir::LirModule make_bir_minimal_call_crossing_direct_call_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

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

c4c::codegen::lir::LirModule
make_bir_minimal_call_crossing_direct_call_lir_module_helper_first() {
  auto module = make_bir_minimal_call_crossing_direct_call_lir_module();
  std::swap(module.functions[0], module.functions[1]);
  return module;
}

c4c::codegen::lir::LirModule
make_bir_minimal_call_crossing_direct_call_lir_module_with_typed_helper_param() {
  using namespace c4c::codegen::lir;

  auto module = make_bir_minimal_call_crossing_direct_call_lir_module();
  auto& helper = module.functions[0];
  helper.return_type.base = c4c::TB_INT;
  helper.params.clear();

  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  helper.params.push_back({"%p.x", param_type});
  helper.signature_text = "define i32 @add_one(i8 %p.x)\n";

  auto& helper_entry = helper.blocks.front();
  auto& add = std::get<LirBinOp>(helper_entry.insts.front());
  add.type_str = make_test_stale_text_i32_lir_type();

  auto& main_entry = module.functions[1].blocks.front();
  auto& source_add = std::get<LirBinOp>(main_entry.insts[0]);
  auto& final_add = std::get<LirBinOp>(main_entry.insts[2]);
  source_add.type_str = make_test_stale_text_i32_lir_type();
  final_add.type_str = make_test_stale_text_i32_lir_type();

  return module;
}

c4c::codegen::lir::LirModule
make_bir_minimal_call_crossing_direct_call_lir_module_with_typed_main_return() {
  auto module = make_bir_minimal_call_crossing_direct_call_lir_module();
  make_test_stale_typed_i32_main_return(module.functions[1]);
  return module;
}

void test_bir_lowering_accepts_minimal_direct_call_lir_module() {
  const auto lowered = c4c::backend::try_lower_to_bir(make_bir_minimal_direct_call_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the zero-arg minimal direct-call LIR module slice");

  const auto parsed = c4c::backend::parse_bir_minimal_direct_call_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should match the shared minimal direct-call BIR parser");
  if (parsed.has_value()) {
    expect_true(parsed->helper != nullptr && parsed->helper->name == "helper" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main" &&
                    parsed->return_imm == 42,
                "the lowered BIR module should preserve the helper/main structure and immediate return");
  }
}

void test_bir_lowering_accepts_minimal_direct_call_lir_module_with_typed_helper_return() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_direct_call_lir_module_with_typed_helper_return());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the zero-arg minimal direct-call helper when typed helper return metadata stays i32 but the helper signature and ret text are stale");

  const auto parsed = c4c::backend::parse_bir_minimal_direct_call_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should still match the shared minimal direct-call parser when helper typed return metadata is authoritative");
  if (parsed.has_value()) {
    expect_true(parsed->return_imm == 42,
                "the lowered BIR module should preserve the helper immediate return when helper return typing comes from semantic metadata");
  }
}

void test_bir_lowering_accepts_minimal_direct_call_lir_module_with_typed_main_return() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_bir_minimal_direct_call_lir_module_with_typed_main_return());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the zero-arg minimal direct-call caller when typed caller return metadata stays i32 but the caller signature and ret text are stale");

  const auto parsed = c4c::backend::parse_bir_minimal_direct_call_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should still match the shared minimal direct-call parser when caller typed return metadata is authoritative");
  if (parsed.has_value()) {
    expect_true(parsed->main_function != nullptr && parsed->main_function->name == "main",
                "the lowered BIR module should preserve the caller identity when caller return typing comes from semantic metadata");
  }
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

void test_bir_lowering_accepts_minimal_declared_direct_call_lir_module() {
  const auto lowered = c4c::backend::try_lower_to_bir(make_bir_declared_direct_call_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the remaining minimal declared direct-call LIR module slice");

  expect_true(lowered->string_constants.size() == 1 &&
                  lowered->string_constants.front().name == "msg" &&
                  lowered->string_constants.front().bytes == "hello\n",
              "BIR lowering should decode LIR string-pool entries into BIR string constants for minimal declared direct-call modules");
  expect_true(lowered->functions.size() == 2 &&
                  lowered->functions.front().is_declaration &&
                  lowered->functions.front().name == "puts_like",
              "BIR lowering should synthesize the callee declaration in BIR for minimal declared direct-call modules");

  const auto parsed = c4c::backend::parse_bir_minimal_declared_direct_call_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should match the shared minimal declared direct-call BIR parser");
  expect_true(parsed->args.size() == 1 &&
                  parsed->args.front().kind == c4c::backend::ParsedBackendExternCallArg::Kind::Ptr &&
                  parsed->args.front().operand == "@msg" &&
                  !parsed->return_call_result && parsed->return_imm == 7,
              "the lowered BIR module should preserve the pointer argument and fixed return immediate");
}

void test_bir_lowering_accepts_string_literal_strlen_sub_lir_module() {
  const auto lowered = c4c::backend::try_lower_to_bir(make_bir_string_literal_strlen_sub_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded string-literal strlen/sub source-like LIR slice");
  if (!lowered.has_value()) {
    return;
  }

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().name == "main" &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return &&
                  lowered->functions.front().blocks.front().terminator.value ==
                      c4c::backend::bir::Value::immediate_i32(0),
              "the lowered string-literal strlen/sub BIR module should collapse to the exact constant return implied by the private string bytes");
}

void test_bir_lowering_accepts_string_literal_char_sub_lir_module() {
  const auto lowered = c4c::backend::try_lower_to_bir(make_bir_string_literal_char_sub_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded string-literal char/sub source-like LIR slice");
  if (!lowered.has_value()) {
    return;
  }

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().name == "main" &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return &&
                  lowered->functions.front().blocks.front().terminator.value ==
                      c4c::backend::bir::Value::immediate_i32(0),
              "the lowered string-literal char/sub BIR module should collapse to the exact constant return implied by the private string bytes");
}

void test_bir_lowering_accepts_local_i32_store_or_sub_lir_module() {
  const auto lowered = c4c::backend::try_lower_to_bir(make_bir_local_i32_store_or_sub_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded local i32 store/or/sub source-like LIR slice");
  if (!lowered.has_value()) {
    return;
  }

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().name == "main" &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return &&
                  lowered->functions.front().blocks.front().terminator.value ==
                      c4c::backend::bir::Value::immediate_i32(0),
              "the lowered local i32 store/or/sub BIR module should collapse to the exact constant return implied by the slot update");
}

void test_bir_lowering_accepts_local_i32_store_and_sub_lir_module() {
  const auto lowered = c4c::backend::try_lower_to_bir(make_bir_local_i32_store_and_sub_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded local i32 store/and/sub source-like LIR slice");
  if (!lowered.has_value()) {
    return;
  }

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return &&
                  lowered->functions.front().blocks.front().terminator.value ==
                      c4c::backend::bir::Value::immediate_i32(0),
              "the lowered local i32 store/and/sub BIR module should collapse to the exact constant return implied by the slot update");
}

void test_bir_lowering_accepts_local_i32_store_xor_sub_lir_module() {
  const auto lowered = c4c::backend::try_lower_to_bir(make_bir_local_i32_store_xor_sub_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded local i32 store/xor/sub source-like LIR slice");
  if (!lowered.has_value()) {
    return;
  }

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return &&
                  lowered->functions.front().blocks.front().terminator.value ==
                      c4c::backend::bir::Value::immediate_i32(0),
              "the lowered local i32 store/xor/sub BIR module should collapse to the exact constant return implied by the slot update");
}

void test_bir_lowering_infers_extern_decl_params_from_typed_call_lir_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_bir_declared_direct_call_inferred_params_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept extern declarations whose parameter surface is inferred from one typed call");

  expect_true(lowered->functions.size() == 2 &&
                  lowered->functions.front().is_declaration &&
                  lowered->functions.front().name == "helper_ext" &&
                  lowered->functions.front().params.size() == 2 &&
                  lowered->functions.front().params[0].type == c4c::backend::bir::TypeKind::I32 &&
                  lowered->functions.front().params[1].type == c4c::backend::bir::TypeKind::I32,
              "BIR lowering should synthesize an extern declaration with the inferred typed parameter list");

  const auto parsed = c4c::backend::parse_bir_minimal_declared_direct_call_module(*lowered);
  expect_true(parsed.has_value() &&
                  parsed->args.size() == 2 &&
                  parsed->args[0].kind == c4c::backend::ParsedBackendExternCallArg::Kind::I32Imm &&
                  parsed->args[0].imm == 5 &&
                  parsed->args[1].kind == c4c::backend::ParsedBackendExternCallArg::Kind::I32Imm &&
                  parsed->args[1].imm == 7,
              "the lowered BIR module should preserve the inferred immediate call arguments for extern declarations");
}

void test_bir_lowering_uses_typed_declared_direct_call_metadata_when_text_is_stale() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_declared_direct_call_typed_decl_metadata_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept declared direct-call modules when typed caller/callee/call metadata says i32 but the stored signature and return text is stale");

  const auto parsed = c4c::backend::parse_bir_minimal_declared_direct_call_module(*lowered);
  expect_true(parsed.has_value() &&
                  parsed->callee != nullptr &&
                  parsed->callee->name == "helper_decl" &&
                  parsed->main_function != nullptr &&
                  parsed->main_function->name == "main" &&
                  parsed->args.empty() &&
                  parsed->return_call_result,
              "the lowered BIR module should preserve the zero-arg declared direct-call structure from typed metadata");
}

void test_bir_lowering_uses_typed_extern_declared_direct_call_metadata_when_text_is_stale() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_declared_direct_call_typed_extern_metadata_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept extern declared direct-call modules when typed caller/callee/call metadata says i32 but the stored extern signature and return text is stale");

  const auto parsed = c4c::backend::parse_bir_minimal_declared_direct_call_module(*lowered);
  expect_true(parsed.has_value() &&
                  parsed->callee != nullptr &&
                  parsed->callee->name == "helper_ext" &&
                  parsed->args.size() == 2 &&
                  parsed->args[0].kind == c4c::backend::ParsedBackendExternCallArg::Kind::I32Imm &&
                  parsed->args[0].imm == 5 &&
                  parsed->args[1].kind == c4c::backend::ParsedBackendExternCallArg::Kind::I32Imm &&
                  parsed->args[1].imm == 7 &&
                  parsed->return_call_result,
              "the lowered BIR module should preserve the extern declared direct-call structure from typed metadata");
}

void test_bir_lowering_uses_actual_fixed_vararg_call_types_for_declared_direct_calls() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_declared_direct_call_vararg_fixed_param_typed_match_lir_module());
  expect_true(
      lowered.has_value(),
      "BIR lowering should accept declared direct-call vararg modules when the fixed call-surface type is only recoverable from the actual typed call args");

  const auto parsed = c4c::backend::parse_bir_minimal_declared_direct_call_module(*lowered);
  expect_true(parsed.has_value() &&
                  parsed->callee != nullptr &&
                  parsed->callee->name == "helper_decl" &&
                  parsed->callee->params.size() == 1 &&
                  parsed->callee->params[0].type == c4c::backend::bir::TypeKind::I32 &&
                  parsed->args.size() == 1 &&
                  parsed->args[0].kind == c4c::backend::ParsedBackendExternCallArg::Kind::I32Imm &&
                  parsed->args[0].imm == 5 &&
                  parsed->return_call_result,
              "the lowered BIR module should preserve the fixed declared vararg operand recovered from the typed call args");
}

void test_bir_lowering_uses_typed_zero_arg_vararg_declared_direct_call_metadata_when_text_is_stale() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_declared_direct_call_zero_arg_vararg_typed_decl_metadata_lir_module());
  expect_true(
      lowered.has_value(),
      "BIR lowering should accept zero-fixed-param declared direct-call vararg modules when typed callee metadata says i32 but the stored declaration text is stale");

  const auto parsed = c4c::backend::parse_bir_minimal_declared_direct_call_module(*lowered);
  expect_true(parsed.has_value() &&
                  parsed->callee != nullptr &&
                  parsed->callee->name == "helper_decl" &&
                  parsed->callee->params.empty() &&
                  parsed->args.empty() &&
                  parsed->return_call_result,
              "the lowered BIR module should preserve the zero-arg declared vararg direct-call structure from typed metadata");
}

void test_bir_lowering_accepts_minimal_void_direct_call_imm_return_lir_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_bir_minimal_void_direct_call_imm_return_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal void direct-call plus fixed return LIR module slice");

  const auto parsed = c4c::backend::parse_bir_minimal_void_direct_call_imm_return_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should match the shared void direct-call BIR parser");
  if (parsed.has_value()) {
    expect_true(parsed->helper != nullptr && parsed->helper->name == "voidfn" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main" &&
                    parsed->return_imm == 9,
                "the lowered BIR module should preserve the helper/main structure and caller fixed return immediate");
  }
}

void test_bir_lowering_accepts_minimal_two_arg_direct_call_lir_module() {
  const auto lowered = c4c::backend::try_lower_to_bir(make_bir_minimal_two_arg_direct_call_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal two-argument direct-call LIR module slice");

  const auto parsed = c4c::backend::parse_bir_minimal_two_arg_direct_call_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should match the shared minimal two-argument direct-call BIR parser");
  if (parsed.has_value()) {
    expect_true(parsed->helper != nullptr && parsed->helper->name == "add_pair" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main" &&
                    parsed->lhs_call_arg_imm == 5 && parsed->rhs_call_arg_imm == 7,
                "the lowered BIR module should preserve the helper/main structure and both immediate call operands");
  }
}

void test_bir_lowering_accepts_minimal_two_arg_direct_call_local_slot_lir_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_bir_minimal_two_arg_direct_call_local_slot_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal two-argument direct-call LIR local-slot slice");

  const auto parsed = c4c::backend::parse_bir_minimal_two_arg_direct_call_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered local-slot BIR module should match the shared minimal two-argument direct-call BIR parser");
  if (parsed.has_value()) {
    expect_true(parsed->helper != nullptr && parsed->helper->name == "add_pair" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main" &&
                    parsed->lhs_call_arg_imm == 5 && parsed->rhs_call_arg_imm == 7,
                "the lowered local-slot BIR module should preserve the helper/main structure and normalized immediate call operands");
  }
}

void test_bir_lowering_accepts_minimal_two_arg_direct_call_local_slot_lir_module_with_stale_typed_slot_text() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_two_arg_direct_call_local_slot_lir_module_with_stale_typed_slot_text());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal two-argument direct-call local-slot slice when the caller alloca/store/load text is stale but typed metadata still says i32");

  const auto parsed = c4c::backend::parse_bir_minimal_two_arg_direct_call_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered local-slot BIR module should still match the shared minimal two-argument direct-call parser when the caller slot text is stale");
  if (parsed.has_value()) {
    expect_true(parsed->lhs_call_arg_imm == 5 && parsed->rhs_call_arg_imm == 7,
                "the lowered local-slot BIR module should preserve the recovered immediate operands when the caller slot text is stale");
  }
}

void test_bir_lowering_accepts_minimal_two_arg_direct_call_lir_module_with_typed_helper_params() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_two_arg_direct_call_lir_module_with_typed_helper_params());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal two-argument direct-call LIR module slice when typed helper params disagree with stale signature param text");

  const auto parsed = c4c::backend::parse_bir_minimal_two_arg_direct_call_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should still match the shared minimal two-argument direct-call BIR parser when typed helper params disagree with stale signature text");
  if (parsed.has_value()) {
    expect_true(parsed->helper != nullptr && parsed->helper->name == "add_pair" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main" &&
                    parsed->lhs_call_arg_imm == 5 && parsed->rhs_call_arg_imm == 7,
                "the lowered BIR module should preserve the helper/main structure and both immediate call operands for the typed helper-param two-argument direct-call slice");
  }
}

void test_bir_lowering_accepts_minimal_two_arg_direct_call_lir_module_with_typed_main_return() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_two_arg_direct_call_lir_module_with_typed_main_return());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal two-argument direct-call LIR module slice when the caller return text is stale but typed function metadata still says i32");

  const auto parsed = c4c::backend::parse_bir_minimal_two_arg_direct_call_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should still match the shared minimal two-argument direct-call BIR parser when the caller return text is stale");
}

void test_bir_lowering_accepts_minimal_direct_call_add_imm_lir_module() {
  const auto lowered = c4c::backend::try_lower_to_bir(make_bir_minimal_direct_call_add_imm_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal add-immediate direct-call LIR module slice");

  const auto parsed = c4c::backend::parse_bir_minimal_direct_call_add_imm_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should match the shared add-immediate direct-call BIR parser");
  if (parsed.has_value()) {
    expect_true(parsed->helper != nullptr && parsed->helper->name == "add_one" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main" &&
                    parsed->call_arg_imm == 5 && parsed->add_imm == 1,
                "the lowered BIR module should preserve the helper/main structure plus the call and add immediates");
  }
}

void test_bir_lowering_accepts_minimal_direct_call_add_imm_lir_module_with_typed_helper_param() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_direct_call_add_imm_lir_module_with_typed_helper_param());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the add-immediate direct-call LIR module slice when the typed helper param disagrees with stale signature param text");

  const auto parsed = c4c::backend::parse_bir_minimal_direct_call_add_imm_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should still match the shared add-immediate direct-call BIR parser when typed helper params disagree with stale signature text");
  if (parsed.has_value()) {
    expect_true(parsed->helper != nullptr && parsed->helper->name == "add_one" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main" &&
                    parsed->call_arg_imm == 5 && parsed->add_imm == 1,
                "the lowered BIR module should preserve the helper/main structure plus the call and add immediates for the typed helper-param add-immediate slice");
  }
}

void test_bir_lowering_accepts_minimal_direct_call_add_imm_lir_module_with_typed_main_return() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_direct_call_add_imm_lir_module_with_typed_main_return());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the add-immediate direct-call LIR module slice when the caller return text is stale but typed function metadata still says i32");

  const auto parsed = c4c::backend::parse_bir_minimal_direct_call_add_imm_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should still match the shared add-immediate direct-call BIR parser when the caller return text is stale");
}

void test_bir_lowering_accepts_minimal_direct_call_add_imm_lir_module_with_stale_typed_local_slot_text() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_direct_call_add_imm_lir_module_with_stale_typed_local_slot_text());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the add-immediate direct-call local-slot slice when the caller alloca/store/load text is stale but typed metadata still says i32");

  const auto parsed = c4c::backend::parse_bir_minimal_direct_call_add_imm_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should still match the shared add-immediate direct-call parser when the caller slot text is stale");
  if (parsed.has_value()) {
    expect_true(parsed->call_arg_imm == 5 && parsed->add_imm == 1,
                "the lowered BIR module should preserve the call and helper immediates when the caller slot text is stale");
  }
}

void test_bir_lowering_accepts_minimal_direct_call_add_imm_slot_lir_module_with_typed_helper_param() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_direct_call_add_imm_slot_lir_module_with_typed_helper_param());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the slot-based add-immediate direct-call LIR module slice when the typed helper param disagrees with stale signature param text");

  const auto parsed = c4c::backend::parse_bir_minimal_direct_call_add_imm_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should still match the shared add-immediate direct-call BIR parser for the slot-based typed helper-param slice");
  if (parsed.has_value()) {
    expect_true(parsed->helper != nullptr && parsed->helper->name == "add_one" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main" &&
                    parsed->call_arg_imm == 5 && parsed->add_imm == 1,
                "the lowered BIR module should preserve the helper/main structure plus the call and add immediates for the slot-based typed helper-param add-immediate slice");
  }
}

void test_bir_lowering_accepts_minimal_direct_call_identity_arg_lir_module() {
  const auto lowered = c4c::backend::try_lower_to_bir(make_bir_minimal_direct_call_identity_arg_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal identity direct-call LIR module slice");

  const auto parsed = c4c::backend::parse_bir_minimal_direct_call_identity_arg_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should match the shared identity direct-call BIR parser");
  if (parsed.has_value()) {
    expect_true(parsed->helper != nullptr && parsed->helper->name == "f" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main" &&
                    parsed->call_arg_imm == 0,
                "the lowered BIR module should preserve the helper/main structure and the caller immediate for the identity direct-call slice");
  }
}

void test_bir_lowering_accepts_minimal_direct_call_identity_arg_lir_module_with_helper_first() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_bir_minimal_direct_call_identity_arg_lir_module_helper_first());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal identity direct-call LIR module slice when the helper is listed first");

  const auto parsed = c4c::backend::parse_bir_minimal_direct_call_identity_arg_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should still match the shared identity direct-call BIR parser when the helper is listed first");
  if (parsed.has_value()) {
    expect_true(parsed->helper != nullptr && parsed->helper->name == "f" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main" &&
                    parsed->call_arg_imm == 0,
                "the lowered BIR module should preserve the helper/main structure and the caller immediate for the helper-first identity direct-call slice");
  }
}

void test_bir_lowering_accepts_minimal_direct_call_identity_arg_lir_module_with_typed_helper_param() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_direct_call_identity_arg_lir_module_with_typed_helper_param());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the identity direct-call LIR module slice when the typed helper param disagrees with stale signature param text");

  const auto parsed = c4c::backend::parse_bir_minimal_direct_call_identity_arg_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should still match the shared identity direct-call BIR parser when typed helper params disagree with stale signature text");
  if (parsed.has_value()) {
    expect_true(parsed->helper != nullptr && parsed->helper->name == "f" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main" &&
                    parsed->call_arg_imm == 0,
                "the lowered BIR module should preserve the helper/main structure and the caller immediate for the typed helper-param identity direct-call slice");
  }
}

void test_bir_lowering_accepts_minimal_direct_call_identity_arg_lir_module_with_typed_main_return() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_direct_call_identity_arg_lir_module_with_typed_main_return());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the identity direct-call LIR module slice when the caller return text is stale but typed function metadata still says i32");

  const auto parsed = c4c::backend::parse_bir_minimal_direct_call_identity_arg_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should still match the shared identity direct-call BIR parser when the caller return text is stale");
}

void test_bir_lowering_accepts_minimal_folded_two_arg_direct_call_lir_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_bir_minimal_folded_two_arg_direct_call_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal folded two-argument direct-call LIR module slice");

  expect_true(lowered->functions.size() == 1 && lowered->functions.front().name == "main" &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.value.has_value() &&
                  lowered->functions.front().blocks.front().terminator.value->kind ==
                      c4c::backend::bir::Value::Kind::Immediate &&
                  lowered->functions.front().blocks.front().terminator.value->immediate == 8,
              "the lowered BIR module should collapse the folded two-argument direct-call slice to a single caller return-immediate function");
  if (!lowered->functions.empty()) {
    expect_true(lowered->functions.front().return_type == c4c::backend::bir::TypeKind::I32,
                "the lowered folded two-argument direct-call slice should preserve the caller return type");
  }
}

void test_bir_lowering_accepts_minimal_folded_two_arg_direct_call_lir_module_with_typed_helper_params() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_folded_two_arg_direct_call_lir_module_with_typed_helper_params());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the folded two-argument direct-call slice when typed helper params disagree with stale signature param text");

  expect_true(lowered->functions.size() == 1 && lowered->functions.front().name == "main" &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.value.has_value() &&
                  lowered->functions.front().blocks.front().terminator.value->kind ==
                      c4c::backend::bir::Value::Kind::Immediate &&
                  lowered->functions.front().blocks.front().terminator.value->immediate == 8,
              "The typed helper-param folded direct-call slice should still collapse to the same caller return immediate");
}

void test_bir_lowering_accepts_minimal_folded_two_arg_direct_call_lir_module_with_helper_first() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_folded_two_arg_direct_call_lir_module_helper_first());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal folded two-argument direct-call LIR module slice when the helper is listed first");

  expect_true(lowered->functions.size() == 1 && lowered->functions.front().name == "main" &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.value.has_value() &&
                  lowered->functions.front().blocks.front().terminator.value->kind ==
                      c4c::backend::bir::Value::Kind::Immediate &&
                  lowered->functions.front().blocks.front().terminator.value->immediate == 8,
              "the lowered BIR module should still collapse the helper-first folded two-argument direct-call slice to a single caller return-immediate function");
}

void test_bir_lowering_accepts_minimal_dual_identity_direct_call_sub_lir_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_bir_minimal_dual_identity_direct_call_sub_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal dual-identity subtraction direct-call LIR module slice");

  const auto parsed = c4c::backend::parse_bir_minimal_dual_identity_direct_call_sub_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should match the shared dual-identity subtraction direct-call BIR parser");
  if (parsed.has_value()) {
    expect_true(parsed->lhs_helper != nullptr && parsed->lhs_helper->name == "f" &&
                    parsed->rhs_helper != nullptr && parsed->rhs_helper->name == "g" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main" &&
                    parsed->lhs_call_arg_imm == 7 && parsed->rhs_call_arg_imm == 3,
                "the lowered BIR module should preserve both helper identities and both caller immediates for the dual-identity subtraction slice");
  }
}

void test_bir_lowering_accepts_minimal_dual_identity_direct_call_sub_lir_module_with_helper_first() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_dual_identity_direct_call_sub_lir_module_helper_first());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal dual-identity subtraction direct-call LIR module slice when a helper is listed first");

  const auto parsed = c4c::backend::parse_bir_minimal_dual_identity_direct_call_sub_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should still match the shared dual-identity subtraction direct-call BIR parser when a helper is listed first");
  if (parsed.has_value()) {
    expect_true(parsed->lhs_helper != nullptr && parsed->lhs_helper->name == "f" &&
                    parsed->rhs_helper != nullptr && parsed->rhs_helper->name == "g" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main" &&
                    parsed->lhs_call_arg_imm == 7 && parsed->rhs_call_arg_imm == 3,
                "the lowered BIR module should preserve both helper identities and both caller immediates for the helper-first dual-identity subtraction slice");
  }
}

void test_bir_lowering_accepts_minimal_dual_identity_direct_call_sub_lir_module_with_typed_helper_params() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_dual_identity_direct_call_sub_lir_module_with_typed_helper_params());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the dual-identity subtraction direct-call LIR module slice when typed helper params disagree with stale signature param text");

  const auto parsed = c4c::backend::parse_bir_minimal_dual_identity_direct_call_sub_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should still match the shared dual-identity subtraction direct-call BIR parser when typed helper params disagree with stale signature text");
  if (parsed.has_value()) {
    expect_true(parsed->lhs_helper != nullptr && parsed->lhs_helper->name == "f" &&
                    parsed->rhs_helper != nullptr && parsed->rhs_helper->name == "g" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main" &&
                    parsed->lhs_call_arg_imm == 7 && parsed->rhs_call_arg_imm == 3,
                "the lowered BIR module should preserve both helper identities and both caller immediates for the typed helper-param dual-identity subtraction slice");
  }
}

void test_bir_lowering_accepts_minimal_dual_identity_direct_call_sub_lir_module_with_typed_main_return() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_dual_identity_direct_call_sub_lir_module_with_typed_main_return());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the dual-identity subtraction direct-call LIR module slice when the caller return text is stale but typed function metadata still says i32");

  const auto parsed = c4c::backend::parse_bir_minimal_dual_identity_direct_call_sub_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should still match the shared dual-identity subtraction direct-call BIR parser when the caller return text is stale");
}

void test_bir_lowering_accepts_minimal_call_crossing_direct_call_lir_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_bir_minimal_call_crossing_direct_call_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal call-crossing direct-call LIR module slice");

  const auto parsed = c4c::backend::parse_bir_minimal_call_crossing_direct_call_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should match the shared call-crossing direct-call BIR parser");
  if (parsed.has_value()) {
    expect_true(parsed->helper != nullptr && parsed->helper->name == "add_one" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main" &&
                    parsed->regalloc_source_value == "%t0" &&
                    parsed->source_imm == 5 && parsed->helper_add_imm == 1,
                "the lowered BIR module should preserve the helper/main structure plus the recovered source and helper immediates for the call-crossing slice");
  }
}

void test_bir_lowering_accepts_minimal_call_crossing_direct_call_lir_module_with_helper_first() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_call_crossing_direct_call_lir_module_helper_first());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal call-crossing direct-call LIR module slice when a helper is listed first");

  const auto parsed = c4c::backend::parse_bir_minimal_call_crossing_direct_call_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should still match the shared call-crossing direct-call BIR parser when a helper is listed first");
  if (parsed.has_value()) {
    expect_true(parsed->helper != nullptr && parsed->helper->name == "add_one" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main" &&
                    parsed->regalloc_source_value == "%t0" &&
                    parsed->source_imm == 5 && parsed->helper_add_imm == 1,
                "the lowered BIR module should preserve the recovered source and helper immediates for the helper-first call-crossing slice");
  }
}

void test_bir_lowering_accepts_minimal_call_crossing_direct_call_lir_module_with_typed_helper_param() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_call_crossing_direct_call_lir_module_with_typed_helper_param());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal call-crossing direct-call LIR module slice when the typed helper param disagrees with stale signature param text");

  const auto parsed = c4c::backend::parse_bir_minimal_call_crossing_direct_call_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should still match the shared call-crossing direct-call BIR parser when typed helper params disagree with stale signature text");
  if (parsed.has_value()) {
    expect_true(parsed->helper != nullptr && parsed->helper->name == "add_one" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main" &&
                    parsed->regalloc_source_value == "%t0" &&
                    parsed->source_imm == 5 && parsed->helper_add_imm == 1,
                "the lowered BIR module should preserve the helper/main structure plus the recovered source and helper immediates for the typed helper-param call-crossing slice");
  }
}

void test_bir_lowering_accepts_minimal_call_crossing_direct_call_lir_module_with_typed_main_return() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_call_crossing_direct_call_lir_module_with_typed_main_return());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal call-crossing direct-call LIR module slice when the caller return text is stale but typed function metadata still says i32");

  const auto parsed = c4c::backend::parse_bir_minimal_call_crossing_direct_call_module(*lowered);
  expect_true(parsed.has_value(),
              "the lowered BIR module should still match the shared call-crossing direct-call BIR parser when the caller return text is stale");
}

void test_bir_lowering_accepts_minimal_countdown_do_while_lir_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_bir_minimal_countdown_do_while_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded countdown do-while LIR slice through the shared local-slot loop contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().local_slots.size() == 1 &&
                  lowered->functions.front().blocks.size() == 4 &&
                  lowered->functions.front().blocks[1].label == "cond" &&
                  lowered->functions.front().blocks[1].terminator.kind ==
                      c4c::backend::bir::TerminatorKind::CondBranch,
              "the lowered countdown do-while BIR module should normalize the empty bridge block into the shared four-block loop shape");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "entry:\n  bir.store_local %lv.x, i32 50\n  bir.br cond\n",
                  "the lowered countdown do-while BIR module should preserve the initial counter while normalizing the loop header");
  expect_contains(rendered, "cond:\n  %t2 = bir.load_local i32 %lv.x\n  %t3 = bir.ne i32 %t2, 0\n  bir.cond_br i32 %t3, body, exit\n",
                  "the lowered countdown do-while BIR module should route through the shared conditional loop header");
}

void test_bir_lowering_accepts_minimal_goto_only_constant_return_lir_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_bir_minimal_goto_only_constant_return_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the goto-only constant-return LIR slice through the shared branch-only constant-return contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().label == "entry" &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered goto-only constant-return BIR module should collapse the empty branch chain to one canonical return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "bir.func @main() -> i32 {\nentry:\n  bir.ret i32 9\n}\n",
                  "the lowered goto-only constant-return BIR module should normalize the empty branch chain to a single immediate return");
}

void test_bir_lowering_accepts_local_i32_store_load_sub_return_immediate_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_local_i32_store_load_sub_return_immediate_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded local i32 store-load-sub-return slice through the shared constant-return contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered local i32 store-load-sub-return module should collapse to one canonical constant-return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "bir.func @main() -> i32 {\nentry:\n  bir.ret i32 0\n}\n",
                  "the lowered local i32 store-load-sub-return module should normalize the local-slot arithmetic to a single immediate return");
}

void test_bir_lowering_accepts_local_i32_arithmetic_chain_return_immediate_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_local_i32_arithmetic_chain_return_immediate_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded local i32 arithmetic-chain slice through the shared constant-return contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered local i32 arithmetic-chain module should collapse to one canonical constant-return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "bir.func @main() -> i32 {\nentry:\n  bir.ret i32 0\n}\n",
                  "the lowered local i32 arithmetic-chain module should normalize the repeated local-slot arithmetic to a single immediate zero return");
}

void test_bir_lowering_accepts_two_local_i32_zero_init_return_first_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_two_local_i32_zero_init_return_first_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded two-local zero-init return slice through the shared constant-return contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered two-local zero-init module should collapse to one canonical constant-return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "bir.func @main() -> i32 {\nentry:\n  bir.ret i32 0\n}\n",
                  "the lowered two-local zero-init module should normalize the chained zero stores to a single immediate zero return");
}

void test_bir_lowering_accepts_local_i32_pointer_store_zero_load_return_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_local_i32_pointer_store_zero_load_return_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded local-pointer store-zero-load-return slice through the shared constant-return contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered local-pointer store-zero-load-return module should collapse to one canonical constant-return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "bir.func @main() -> i32 {\nentry:\n  bir.ret i32 0\n}\n",
                  "the lowered local-pointer store-zero-load-return module should normalize the indirect local-slot zeroing to a single immediate return");
}

void test_bir_lowering_accepts_local_i32_pointer_gep_zero_load_return_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_local_i32_pointer_gep_zero_load_return_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded local-pointer gep-zero-load-return slice through the shared constant-return contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered local-pointer gep-zero-load-return module should collapse to one canonical constant-return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "bir.func @main() -> i32 {\nentry:\n  bir.ret i32 0\n}\n",
                  "the lowered local-pointer gep-zero-load-return module should normalize the zero-offset indirect local-slot load to a single immediate return");
}

void test_bir_lowering_accepts_local_i32_pointer_gep_zero_store_slot_load_return_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_local_i32_pointer_gep_zero_store_slot_load_return_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded local-pointer gep-zero-store-slot-load-return slice through the shared constant-return contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered local-pointer gep-zero-store-slot-load-return module should collapse to one canonical constant-return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "bir.func @main() -> i32 {\nentry:\n  bir.ret i32 0\n}\n",
                  "the lowered local-pointer gep-zero-store-slot-load-return module should normalize the zero-offset alias overwrite of the original local slot to a single immediate return");
}

void test_bir_lowering_accepts_local_i32_array_two_slot_sum_sub_three_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_local_i32_array_two_slot_sum_sub_three_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded two-slot local-array sum-sub-three slice through the shared constant-return contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered two-slot local-array sum-sub-three module should collapse to one canonical constant-return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "bir.func @main() -> i32 {\nentry:\n  bir.ret i32 0\n}\n",
                  "the lowered two-slot local-array sum-sub-three module should normalize the constant indexed array stores and arithmetic tail to a single immediate zero return");
}

void test_bir_lowering_accepts_local_i32_array_second_slot_pointer_store_zero_load_return_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(
          make_local_i32_array_second_slot_pointer_store_zero_load_return_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded local-array second-slot pointer-store-zero-load-return slice through the shared constant-return contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered local-array second-slot pointer-store-zero-load-return module should collapse to one canonical constant-return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "bir.func @main() -> i32 {\nentry:\n  bir.ret i32 0\n}\n",
                  "the lowered local-array second-slot pointer-store-zero-load-return module should normalize the aliased second-slot zero-store to a single immediate zero return");
}

void test_bir_lowering_accepts_local_two_field_struct_sub_sub_two_return_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_local_two_field_struct_sub_sub_two_return_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded two-field local-struct subtract-subtract-two slice through the shared constant-return contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered two-field local-struct subtract-subtract-two module should collapse to one canonical constant-return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "bir.func @main() -> i32 {\nentry:\n  bir.ret i32 0\n}\n",
                  "the lowered two-field local-struct subtract-subtract-two module should normalize the constant field stores and subtraction tail to a single immediate zero return");
}

void test_bir_lowering_accepts_local_struct_pointer_alias_add_sub_three_return_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_local_struct_pointer_alias_add_sub_three_return_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded local struct-pointer alias add-sub-three slice through the shared constant-return contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered local struct-pointer alias add-sub-three module should collapse to one canonical constant-return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "bir.func @main() -> i32 {\nentry:\n  bir.ret i32 0\n}\n",
                  "the lowered local struct-pointer alias add-sub-three module should normalize the aliased field stores and arithmetic tail to a single immediate zero return");
}

void test_bir_lowering_accepts_local_self_referential_struct_pointer_chain_zero_return_module() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_local_self_referential_struct_pointer_chain_zero_return_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded local self-referential struct-pointer chain slice through the shared constant-return contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered local self-referential struct-pointer chain module should collapse to one canonical constant-return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "bir.func @main() -> i32 {\nentry:\n  bir.ret i32 0\n}\n",
                  "the lowered local self-referential struct-pointer chain module should normalize the repeated self-load field chain to a single immediate zero return");
}

void test_bir_lowering_accepts_double_indirect_local_pointer_chain_zero_return_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_double_indirect_local_pointer_chain_zero_return_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded double-indirect local pointer-chain slice through the shared constant-return contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered double-indirect local pointer-chain module should collapse to one canonical constant-return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "bir.func @main() -> i32 {\nentry:\n  bir.ret i32 0\n}\n",
                  "the lowered double-indirect local pointer-chain module should normalize the bounded alias loads to a single immediate zero return");
}

void test_bir_lowering_accepts_double_indirect_local_store_one_final_branch_return_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_double_indirect_local_store_one_final_branch_return_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded double-indirect local store-one slice through the shared constant-return contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered double-indirect local store-one module should collapse to one canonical constant-return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "bir.func @main() -> i32 {\nentry:\n  bir.ret i32 0\n}\n",
                  "the lowered double-indirect local store-one module should normalize the dead early returns and final branch to a single immediate zero return");
}

void test_bir_lowering_accepts_minimal_conditional_return_lir_module_with_empty_bridge_blocks() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_conditional_return_with_empty_bridge_blocks_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the empty-bridge conditional-return LIR slice through the shared branch-only select contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().label == "entry" &&
                  lowered->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::SelectInst>(
                      lowered->functions.front().blocks.front().insts.front()) &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered empty-bridge conditional-return BIR module should collapse the branch-only bridge blocks to one canonical select-and-return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered,
                  "bir.func @main() -> i32 {\nentry:\n  %t.select = bir.select slt i32 2, 3, 0, 1\n  bir.ret i32 %t.select\n}\n",
                  "the lowered empty-bridge conditional-return BIR module should normalize the bridge-only CFG to one shared select");
}

void test_bir_lowering_accepts_minimal_conditional_return_lir_module_with_asymmetric_empty_bridge() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_conditional_return_with_asymmetric_empty_bridge_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the asymmetric empty-bridge conditional-return LIR slice through the shared branch-only select contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().label == "entry" &&
                  lowered->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::SelectInst>(
                      lowered->functions.front().blocks.front().insts.front()) &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered asymmetric empty-bridge conditional-return BIR module should collapse the branch-only CFG to one canonical select-and-return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered,
                  "bir.func @main() -> i32 {\nentry:\n  %t.select = bir.select slt i32 2, 3, 0, 1\n  bir.ret i32 %t.select\n}\n",
                  "the lowered asymmetric empty-bridge conditional-return BIR module should normalize the mixed direct/bridge CFG to one shared select");
}

void test_bir_lowering_accepts_minimal_conditional_return_lir_module_with_double_empty_bridge_chain() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_conditional_return_with_double_empty_bridge_chain_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the double-empty-bridge conditional-return LIR slice through the shared branch-only select contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().label == "entry" &&
                  lowered->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::SelectInst>(
                      lowered->functions.front().blocks.front().insts.front()) &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered double-empty-bridge conditional-return BIR module should collapse the longer branch-only goto chain to one canonical select-and-return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered,
                  "bir.func @main() -> i32 {\nentry:\n  %t.select = bir.select slt i32 2, 3, 0, 1\n  bir.ret i32 %t.select\n}\n",
                  "the lowered double-empty-bridge conditional-return BIR module should normalize the longer mixed direct/bridge CFG to one shared select");
}

void test_bir_lowering_accepts_minimal_conditional_return_lir_module_with_mirrored_false_double_empty_bridge_chain() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_conditional_return_with_mirrored_false_double_empty_bridge_chain_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the mirrored false-arm double-empty-bridge conditional-return LIR slice through the shared branch-only select contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().label == "entry" &&
                  lowered->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::SelectInst>(
                      lowered->functions.front().blocks.front().insts.front()) &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered mirrored false-arm double-empty-bridge conditional-return BIR module should collapse the reordered branch-only CFG to one canonical select-and-return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered,
                  "bir.func @main() -> i32 {\nentry:\n  %t.select = bir.select slt i32 2, 3, 0, 1\n  bir.ret i32 %t.select\n}\n",
                  "the lowered mirrored false-arm double-empty-bridge conditional-return BIR module should normalize the reordered longer goto chain to one shared select");
}

void test_bir_lowering_accepts_minimal_conditional_return_lir_module_with_interleaved_double_empty_bridge_chains() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_conditional_return_with_interleaved_double_empty_bridge_chains_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the interleaved double-empty-bridge conditional-return LIR slice through the shared branch-only select contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().label == "entry" &&
                  lowered->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::SelectInst>(
                      lowered->functions.front().blocks.front().insts.front()) &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered interleaved double-empty-bridge conditional-return BIR module should collapse the reordered branch-only CFG to one canonical select-and-return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered,
                  "bir.func @main() -> i32 {\nentry:\n  %t.select = bir.select slt i32 2, 3, 0, 1\n  bir.ret i32 %t.select\n}\n",
                  "the lowered interleaved double-empty-bridge conditional-return BIR module should normalize the interleaved goto chains to one shared select");
}

void test_bir_lowering_accepts_param_conditional_return_lir_module_with_interleaved_mixed_depth_bridge_chains() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_param_conditional_return_with_interleaved_mixed_depth_bridge_chains_lir_module());
  expect_true(
      lowered.has_value(),
      "BIR lowering should accept the parameter-fed mixed-depth conditional-return LIR slice through the shared branch-only select contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().label == "entry" &&
                  lowered->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::SelectInst>(
                      lowered->functions.front().blocks.front().insts.front()) &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered parameter-fed mixed-depth conditional-return BIR module should collapse the interleaved branch-only CFG to one canonical select-and-return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered,
                  "bir.func @choose(i32 %lhs, i32 %rhs) -> i32 {\nentry:\n  %t.select = bir.select slt i32 %lhs, %rhs, %lhs, %rhs\n  bir.ret i32 %t.select\n}\n",
                  "the lowered parameter-fed mixed-depth conditional-return BIR module should normalize the deeper mixed-arm goto chains to one shared select over the incoming compare operands");
}

void test_bir_lowering_accepts_minimal_countdown_do_while_lir_module_with_stale_typed_i32_text() {
  auto module = make_bir_minimal_countdown_do_while_lir_module();
  auto& function = module.functions.front();
  std::get<c4c::codegen::lir::LirAllocaOp>(function.alloca_insts.front()).type_str =
      make_test_stale_text_i32_lir_type();

  auto& entry = function.blocks[0];
  std::get<c4c::codegen::lir::LirStoreOp>(entry.insts.front()).type_str =
      make_test_stale_text_i32_lir_type();

  auto& body = function.blocks[1];
  std::get<c4c::codegen::lir::LirLoadOp>(body.insts[0]).type_str =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirBinOp>(body.insts[1]).type_str =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirStoreOp>(body.insts[2]).type_str =
      make_test_stale_text_i32_lir_type();

  auto& cond = function.blocks[3];
  std::get<c4c::codegen::lir::LirLoadOp>(cond.insts[0]).type_str =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirCmpOp>(cond.insts[1]).type_str =
      make_test_stale_text_i32_lir_type();

  auto& exit = function.blocks[4];
  std::get<c4c::codegen::lir::LirLoadOp>(exit.insts[0]).type_str =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirRet>(exit.terminator).type_str = "i8";

  const auto lowered = c4c::backend::lower_to_bir(module);
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "BIR lowering should preserve the countdown do-while i32 signature when loop-local render text is stale");
  expect_contains(rendered, "entry:\n  bir.store_local %lv.x, i32 50\n  bir.br cond\n",
                  "BIR lowering should still recover the countdown entry store from semantic i32 metadata instead of stale loop-local text");
  expect_contains(rendered, "cond:\n  %t2 = bir.load_local i32 %lv.x\n  %t3 = bir.ne i32 %t2, 0\n  bir.cond_br i32 %t3, body, exit\n",
                  "BIR lowering should still normalize the countdown loop header when load, compare, and return render text only agree semantically");
}

void test_bir_lowering_accepts_double_countdown_guarded_zero_return_lir_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_double_countdown_guarded_zero_return_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded double-countdown guarded-return LIR slice through a shared constant-return contract");

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().label == "entry" &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "the lowered double-countdown guarded-return BIR module should collapse to one canonical constant-return block");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "bir.func @main() -> i32 {\nentry:\n  bir.ret i32 0\n}\n",
                  "the lowered double-countdown guarded-return BIR module should normalize the two countdown loops and dead guard to the constant zero result");
}

void test_bir_lowering_accepts_minimal_scalar_global_load_lir_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_bir_minimal_scalar_global_load_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal scalar global-load LIR slice");
  expect_true(lowered->globals.size() == 1 && lowered->functions.size() == 1 &&
                  lowered->globals.front().name == "g_counter" &&
                  lowered->globals.front().initializer.has_value() &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::LoadGlobalInst>(
                      lowered->functions.front().blocks.front().insts.front()),
              "the lowered scalar global-load BIR module should carry one initialized global plus one direct bir.load_global instruction");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "entry:\n  %t0 = bir.load_global i32 @g_counter\n  bir.ret i32 %t0\n",
                  "the lowered scalar global-load BIR module should print the bounded bir.load_global contract");
}

void test_bir_lowering_accepts_minimal_repeated_zero_arg_call_compare_zero_return_lir_module() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_repeated_zero_arg_call_compare_zero_return_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded repeated zero-arg call compare/return LIR slice");
  if (!lowered.has_value()) {
    return;
  }

  expect_true(lowered->functions.size() == 2 &&
                  lowered->functions[0].name == "f" &&
                  lowered->functions[1].name == "main" &&
                  lowered->functions[0].blocks.size() == 1 &&
                  lowered->functions[1].blocks.size() == 1 &&
                  lowered->functions[0].blocks.front().insts.empty() &&
                  lowered->functions[1].blocks.front().insts.empty() &&
                  lowered->functions[0].blocks.front().terminator.value ==
                      c4c::backend::bir::Value::immediate_i32(100) &&
                  lowered->functions[1].blocks.front().terminator.value ==
                      c4c::backend::bir::Value::immediate_i32(0),
              "the lowered repeated zero-arg call compare slice should preserve the helper return and collapse main to the exact zero return");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "bir.func @f() -> i32 {\nentry:\n  bir.ret i32 100\n}\n",
                  "the lowered repeated zero-arg call compare slice should keep the helper definition in the shared BIR module");
  expect_contains(rendered, "bir.func @main() -> i32 {\nentry:\n  bir.ret i32 0\n}\n",
                  "the lowered repeated zero-arg call compare slice should print the folded main immediate return");
}

void test_bir_lowering_accepts_minimal_string_literal_compare_phi_return_lir_module() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_bir_minimal_string_literal_compare_phi_return_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal string-literal compare phi-return LIR slice");
  if (!lowered.has_value()) {
    return;
  }

  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.value.has_value() &&
                  lowered->functions.front().blocks.front().terminator.value->kind ==
                      c4c::backend::bir::Value::Kind::Immediate &&
                  lowered->functions.front().blocks.front().terminator.value->immediate == 0,
              "the lowered string-literal compare phi-return BIR module should collapse to the bounded immediate-return contract");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "entry:\n  bir.ret i32 0\n",
                  "the lowered string-literal compare phi-return BIR module should print the shared immediate-return contract");
}

void test_bir_lowering_accepts_typed_minimal_scalar_global_load_lir_slice_with_stale_text() {
  auto module = make_bir_minimal_scalar_global_load_lir_module();
  module.globals.front().type.base = c4c::TB_INT;
  module.globals.front().llvm_type = "i8";

  auto& function = module.functions.front();
  function.return_type.base = c4c::TB_INT;

  auto& entry = function.blocks.front();
  std::get<c4c::codegen::lir::LirLoadOp>(entry.insts.front()).type_str =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirRet>(entry.terminator).type_str = "i8";

  const auto lowered = c4c::backend::try_lower_to_bir(module);
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal scalar global-load slice when the global, load, and ret text are stale but typed metadata still says i32");
  if (!lowered.has_value()) {
    return;
  }

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "entry:\n  %t0 = bir.load_global i32 @g_counter\n  bir.ret i32 %t0\n",
                  "the lowered scalar global-load BIR module should still recover the canonical i32 contract from semantic global and load metadata");
}

void test_bir_lowering_accepts_minimal_extern_scalar_global_load_lir_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_bir_minimal_extern_scalar_global_load_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal extern scalar global-load LIR slice");
  expect_true(lowered->globals.size() == 1 && lowered->functions.size() == 1 &&
                  lowered->globals.front().name == "g_counter" &&
                  lowered->globals.front().is_extern &&
                  !lowered->globals.front().initializer.has_value() &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::LoadGlobalInst>(
                      lowered->functions.front().blocks.front().insts.front()),
              "the lowered extern scalar global-load BIR module should carry one extern global plus one direct bir.load_global instruction");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "entry:\n  %t0 = bir.load_global i32 @g_counter\n  bir.ret i32 %t0\n",
                  "the lowered extern scalar global-load BIR module should print the bounded bir.load_global contract");
}

void test_bir_lowering_accepts_typed_minimal_extern_scalar_global_load_lir_slice_with_stale_text() {
  auto module = make_bir_minimal_extern_scalar_global_load_lir_module();
  module.globals.front().type.base = c4c::TB_INT;
  module.globals.front().llvm_type = "i8";

  auto& function = module.functions.front();
  function.return_type.base = c4c::TB_INT;

  auto& entry = function.blocks.front();
  std::get<c4c::codegen::lir::LirLoadOp>(entry.insts.front()).type_str =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirRet>(entry.terminator).type_str = "i8";

  const auto lowered = c4c::backend::try_lower_to_bir(module);
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal extern scalar global-load slice when the global, load, and ret text are stale but typed metadata still says i32");
  if (!lowered.has_value()) {
    return;
  }

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "entry:\n  %t0 = bir.load_global i32 @g_counter\n  bir.ret i32 %t0\n",
                  "the lowered extern scalar global-load BIR module should still recover the canonical i32 contract from semantic global and load metadata");
}

void test_bir_lowering_accepts_minimal_extern_global_array_load_lir_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_bir_minimal_extern_global_array_load_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal extern global-array-load LIR slice");
  expect_true(lowered->globals.size() == 1 && lowered->functions.size() == 1 &&
                  lowered->globals.front().name == "ext_arr" &&
                  lowered->globals.front().is_extern &&
                  !lowered->globals.front().initializer.has_value() &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::LoadGlobalInst>(
                      lowered->functions.front().blocks.front().insts.front()) &&
                  std::get<c4c::backend::bir::LoadGlobalInst>(
                      lowered->functions.front().blocks.front().insts.front()).byte_offset == 4,
              "the lowered extern global-array-load BIR module should carry one extern global plus one byte-offset bir.load_global instruction");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered,
                  "entry:\n  %t3 = bir.load_global i32 @ext_arr, offset 4\n  bir.ret i32 %t3\n",
                  "the lowered extern global-array-load BIR module should print the bounded offset bir.load_global contract");
}

void test_bir_lowering_accepts_typed_minimal_extern_global_array_load_lir_slice_with_stale_text() {
  auto module = make_bir_minimal_extern_global_array_load_lir_module();
  auto& function = module.functions.front();
  function.return_type.base = c4c::TB_INT;

  auto& entry = function.blocks.front();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[1]).from_type =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[1]).to_type =
      make_test_stale_text_i64_lir_type();
  std::get<c4c::codegen::lir::LirGepOp>(entry.insts[2]).element_type =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirLoadOp>(entry.insts[3]).type_str =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirRet>(entry.terminator).type_str = "i8";

  const auto lowered = c4c::backend::try_lower_to_bir(module);
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal extern global-array-load slice when the cast, element, load, and ret text are stale but typed metadata still carries the i32/i64 contract");
  if (!lowered.has_value()) {
    return;
  }

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered,
                  "entry:\n  %t3 = bir.load_global i32 @ext_arr, offset 4\n  bir.ret i32 %t3\n",
                  "the lowered extern global-array-load BIR module should still recover the canonical i32 contract from semantic instruction and function metadata");
}

void test_bir_lowering_accepts_minimal_global_char_pointer_diff_lir_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_bir_minimal_global_char_pointer_diff_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal global char pointer-diff LIR slice");
  if (!lowered.has_value()) {
    return;
  }
  expect_true(lowered->globals.empty() && lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.value.has_value() &&
                  lowered->functions.front().blocks.front().terminator.value->kind ==
                      c4c::backend::bir::Value::Kind::Immediate &&
                  lowered->functions.front().blocks.front().terminator.value->immediate == 1,
              "the lowered global char pointer-diff BIR module should collapse to the bounded immediate-return contract");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "entry:\n  bir.ret i32 1\n",
                  "the lowered global char pointer-diff BIR module should print the shared immediate-return contract");
}

void test_bir_lowering_accepts_typed_minimal_global_char_pointer_diff_lir_slice_with_stale_text() {
  auto module = make_bir_minimal_global_char_pointer_diff_lir_module();
  auto& function = module.functions.front();
  function.return_type.base = c4c::TB_INT;

  auto& entry = function.blocks.front();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[1]).from_type =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[1]).to_type =
      make_test_stale_text_i64_lir_type();
  std::get<c4c::codegen::lir::LirGepOp>(entry.insts[2]).element_type =
      make_test_stale_text_i8_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[4]).from_type =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[4]).to_type =
      make_test_stale_text_i64_lir_type();
  std::get<c4c::codegen::lir::LirGepOp>(entry.insts[5]).element_type =
      make_test_stale_text_i8_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[6]).from_type =
      make_test_stale_text_ptr_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[6]).to_type =
      make_test_stale_text_i64_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[7]).from_type =
      make_test_stale_text_ptr_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[7]).to_type =
      make_test_stale_text_i64_lir_type();
  std::get<c4c::codegen::lir::LirBinOp>(entry.insts[8]).type_str =
      make_test_stale_text_i64_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[9]).from_type =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[9]).to_type =
      make_test_stale_text_i64_lir_type();
  std::get<c4c::codegen::lir::LirCmpOp>(entry.insts[10]).type_str =
      make_test_stale_text_i64_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[11]).from_type =
      make_test_stale_text_i1_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[11]).to_type =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirRet>(entry.terminator).type_str = "i8";

  const auto lowered = c4c::backend::try_lower_to_bir(module);
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal char pointer-diff slice when cast, gep, binop, cmp, and ret text are stale but typed metadata still carries the i8/i32/i64 contract");
  if (!lowered.has_value()) {
    return;
  }

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "entry:\n  bir.ret i32 1\n",
                  "the lowered global char pointer-diff BIR module should still recover the shared immediate-return contract from semantic instruction and function metadata");
}

void test_bir_lowering_accepts_minimal_global_int_pointer_diff_lir_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_bir_minimal_global_int_pointer_diff_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal global int pointer-diff LIR slice");
  if (!lowered.has_value()) {
    return;
  }
  expect_true(lowered->globals.empty() && lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty() &&
                  lowered->functions.front().blocks.front().terminator.value.has_value() &&
                  lowered->functions.front().blocks.front().terminator.value->kind ==
                      c4c::backend::bir::Value::Kind::Immediate &&
                  lowered->functions.front().blocks.front().terminator.value->immediate == 1,
              "the lowered global int pointer-diff BIR module should collapse to the bounded immediate-return contract");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "entry:\n  bir.ret i32 1\n",
                  "the lowered global int pointer-diff BIR module should print the shared immediate-return contract");
}

void test_bir_lowering_accepts_typed_minimal_global_int_pointer_diff_lir_slice_with_stale_text() {
  auto module = make_bir_minimal_global_int_pointer_diff_lir_module();
  auto& function = module.functions.front();
  function.return_type.base = c4c::TB_INT;

  auto& entry = function.blocks.front();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[1]).from_type =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[1]).to_type =
      make_test_stale_text_i64_lir_type();
  std::get<c4c::codegen::lir::LirGepOp>(entry.insts[2]).element_type =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[4]).from_type =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[4]).to_type =
      make_test_stale_text_i64_lir_type();
  std::get<c4c::codegen::lir::LirGepOp>(entry.insts[5]).element_type =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[6]).from_type =
      make_test_stale_text_ptr_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[6]).to_type =
      make_test_stale_text_i64_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[7]).from_type =
      make_test_stale_text_ptr_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[7]).to_type =
      make_test_stale_text_i64_lir_type();
  std::get<c4c::codegen::lir::LirBinOp>(entry.insts[8]).type_str =
      make_test_stale_text_i64_lir_type();
  std::get<c4c::codegen::lir::LirBinOp>(entry.insts[9]).type_str =
      make_test_stale_text_i64_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[10]).from_type =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[10]).to_type =
      make_test_stale_text_i64_lir_type();
  std::get<c4c::codegen::lir::LirCmpOp>(entry.insts[11]).type_str =
      make_test_stale_text_i64_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[12]).from_type =
      make_test_stale_text_i1_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[12]).to_type =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirRet>(entry.terminator).type_str = "i8";

  const auto lowered = c4c::backend::try_lower_to_bir(module);
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal int pointer-diff slice when cast, gep, binop, cmp, and ret text are stale but typed metadata still carries the i32/i64 contract");
  if (!lowered.has_value()) {
    return;
  }

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered, "entry:\n  bir.ret i32 1\n",
                  "the lowered global int pointer-diff BIR module should still recover the shared immediate-return contract from semantic instruction and function metadata");
}

void test_bir_lowering_accepts_minimal_scalar_global_store_reload_lir_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_bir_minimal_scalar_global_store_reload_lir_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal scalar global store-reload LIR slice");
  expect_true(lowered->globals.size() == 1 && lowered->functions.size() == 1 &&
                  lowered->globals.front().name == "g_counter" &&
                  lowered->globals.front().initializer.has_value() &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.size() == 2 &&
                  std::holds_alternative<c4c::backend::bir::StoreGlobalInst>(
                      lowered->functions.front().blocks.front().insts.front()) &&
                  std::holds_alternative<c4c::backend::bir::LoadGlobalInst>(
                      lowered->functions.front().blocks.front().insts[1]),
              "the lowered scalar global store-reload BIR module should carry one initialized global plus direct bir.store_global and bir.load_global instructions");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered,
                  "entry:\n  bir.store_global @g_counter, i32 7\n  %t0 = bir.load_global i32 @g_counter\n  bir.ret i32 %t0\n",
                  "the lowered scalar global store-reload BIR module should print the bounded bir.store_global plus bir.load_global contract");
}

void test_bir_lowering_accepts_typed_minimal_scalar_global_store_reload_lir_slice_with_stale_text() {
  auto module = make_bir_minimal_scalar_global_store_reload_lir_module();
  module.globals.front().type.base = c4c::TB_INT;
  module.globals.front().llvm_type = "i8";

  auto& function = module.functions.front();
  function.return_type.base = c4c::TB_INT;

  auto& entry = function.blocks.front();
  std::get<c4c::codegen::lir::LirStoreOp>(entry.insts[0]).type_str =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirLoadOp>(entry.insts[1]).type_str =
      make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirRet>(entry.terminator).type_str = "i8";

  const auto lowered = c4c::backend::try_lower_to_bir(module);
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal scalar global store-reload slice when the global, store, load, and ret text are stale but typed metadata still says i32");
  if (!lowered.has_value()) {
    return;
  }

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered,
                  "entry:\n  bir.store_global @g_counter, i32 7\n  %t0 = bir.load_global i32 @g_counter\n  bir.ret i32 %t0\n",
                  "the lowered scalar global store-reload BIR module should still recover the canonical i32 store/load contract from semantic global and instruction metadata");
}

void test_bir_lowering_accepts_zero_initialized_scalar_global_store_reload_lir_module() {
  auto module = make_bir_minimal_scalar_global_store_reload_lir_module();
  module.globals.front().init_text = "zeroinitializer";
  std::get<c4c::codegen::lir::LirStoreOp>(module.functions.front().blocks.front().insts.front()).val = "0";

  const auto lowered = c4c::backend::try_lower_to_bir(module);
  expect_true(lowered.has_value(),
              "BIR lowering should accept the minimal scalar global store-reload LIR slice when the global uses zeroinitializer and the stored value is zero");
  if (!lowered.has_value()) {
    return;
  }

  expect_true(lowered->globals.size() == 1 && lowered->globals.front().initializer.has_value() &&
                  *lowered->globals.front().initializer == c4c::backend::bir::Value::immediate_i32(0),
              "the lowered zero-initialized scalar global store-reload BIR module should preserve the canonical i32 zero initializer");

  const auto rendered = c4c::backend::bir::print(*lowered);
  expect_contains(rendered,
                  "entry:\n  bir.store_global @g_counter, i32 0\n  %t0 = bir.load_global i32 @g_counter\n  bir.ret i32 %t0\n",
                  "the lowered zero-initialized scalar global store-reload BIR module should keep the bounded store/load contract");
}

void test_bir_lowering_accepts_global_two_field_struct_store_sub_sub_lir_module() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_bir_minimal_global_two_field_struct_store_sub_sub_module());
  expect_true(lowered.has_value(),
              "BIR lowering should accept the bounded global two-field struct store/sub/sub LIR slice");
  if (!lowered.has_value()) {
    return;
  }

  expect_true(lowered->globals.size() == 1 && lowered->functions.size() == 1 &&
                  lowered->globals.front().name == "v" &&
                  lowered->globals.front().size_bytes == 8 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.size() == 6,
              "the lowered global two-field struct store/sub/sub BIR module should keep one 8-byte global plus the bounded field-store/load arithmetic sequence");
  if (lowered->functions.front().blocks.front().insts.size() != 6) {
    return;
  }

  const auto& insts = lowered->functions.front().blocks.front().insts;
  const auto* store_field0 = std::get_if<c4c::backend::bir::StoreGlobalInst>(&insts[0]);
  const auto* store_field1 = std::get_if<c4c::backend::bir::StoreGlobalInst>(&insts[1]);
  const auto* load_field0 = std::get_if<c4c::backend::bir::LoadGlobalInst>(&insts[2]);
  const auto* first_sub = std::get_if<c4c::backend::bir::BinaryInst>(&insts[3]);
  const auto* load_field1 = std::get_if<c4c::backend::bir::LoadGlobalInst>(&insts[4]);
  const auto* second_sub = std::get_if<c4c::backend::bir::BinaryInst>(&insts[5]);
  expect_true(store_field0 != nullptr && store_field1 != nullptr && load_field0 != nullptr &&
                  first_sub != nullptr && load_field1 != nullptr && second_sub != nullptr &&
                  store_field0->global_name == "v" && store_field0->byte_offset == 0 &&
                  store_field1->global_name == "v" && store_field1->byte_offset == 4 &&
                  load_field0->global_name == "v" && load_field0->byte_offset == 0 &&
                  load_field1->global_name == "v" && load_field1->byte_offset == 4 &&
                  first_sub->opcode == c4c::backend::bir::BinaryOpcode::Sub &&
                  second_sub->opcode == c4c::backend::bir::BinaryOpcode::Sub,
              "the lowered global two-field struct store/sub/sub BIR module should route both fields through explicit global byte offsets plus two subtraction steps");
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

void test_bir_printer_renders_minimal_cast_scaffold() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& function = module.functions.front();
  function.return_type = TypeKind::I64;
  auto& block = function.blocks.front();
  block.insts.push_back(CastInst{CastOpcode::SExt, Value::named(TypeKind::I64, "%t0"),
                                 Value::immediate_i32(-7)});
  block.terminator.value = Value::named(TypeKind::I64, "%t0");

  const auto rendered = c4c::backend::bir::print(module);
  expect_contains(rendered, "%t0 = bir.sext i32 -7 to i64",
                  "BIR printer should render explicit cast instructions in BIR terms");
  expect_contains(rendered, "bir.ret i64 %t0",
                  "BIR printer should let cast results flow into returns");
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

void test_bir_validator_accepts_minimal_cast_scaffold() {
  std::string error;

  expect_true(c4c::backend::bir::validate(
                  c4c::backend::lower_to_bir(make_bir_return_sext_module()), &error),
              "BIR validator should accept the bounded cast scaffold once BIR grows explicit cast instructions");
  expect_true(error.empty(),
              "BIR validator should keep the bounded cast scaffold on the valid path");
}

void test_bir_validator_accepts_minimal_select_scaffold() {
  std::string error;

  expect_true(c4c::backend::bir::validate(
                  c4c::backend::lower_to_bir(make_bir_return_select_eq_module()), &error),
              "BIR validator should accept the bounded compare-fed integer select scaffold");
  expect_true(error.empty(),
              "BIR validator should keep the bounded select scaffold on the valid path");
}

void test_lir_verify_rejects_typed_integer_text_mismatch() {
  auto module = make_bir_i8_return_mul_module();
  auto& bin = std::get<c4c::codegen::lir::LirBinOp>(module.functions.front().blocks.front().insts.front());
  bin.type_str = c4c::codegen::lir::LirTypeRef("i32", c4c::codegen::lir::LirTypeKind::Integer, 8);

  expect_throws_lir_verify(
      [&]() { c4c::codegen::lir::verify_module(module); },
      "typed integer width 8 disagrees with text 'i32'",
      "LIR verifier should reject integer type refs whose typed width disagrees with their renderable text");
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

void test_bir_lowering_accepts_typed_tiny_return_add_lir_slice_with_stale_text() {
  auto module = make_bir_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  auto& add = std::get<c4c::codegen::lir::LirBinOp>(entry.insts[0]);
  auto& ret = std::get<c4c::codegen::lir::LirRet>(entry.terminator);
  add.type_str = make_test_stale_text_i32_lir_type();
  ret.type_str = "i8";

  const auto lowered = c4c::backend::lower_to_bir(module);
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "BIR lowering should preserve the straight-line add signature when the instruction and return types only agree semantically");
  expect_contains(rendered, "%t0 = bir.add i32 2, 3",
                  "BIR lowering should materialize the tiny add slice from typed integer widths instead of stale render text");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should return the typed straight-line add result on the direct BIR path when only semantic widths agree");
}

void test_bir_lowering_accepts_typed_i8_return_add_lir_slice_with_stale_text() {
  auto module = make_bir_i8_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  auto& add = std::get<c4c::codegen::lir::LirBinOp>(entry.insts[0]);
  auto& trunc = std::get<c4c::codegen::lir::LirCastOp>(entry.insts[1]);
  auto& ret = std::get<c4c::codegen::lir::LirRet>(entry.terminator);
  add.type_str = make_test_stale_text_i32_lir_type();
  trunc.from_type = make_test_stale_text_i32_lir_type();
  trunc.to_type = make_test_stale_text_i8_lir_type();
  ret.type_str = "i32";

  const auto lowered = c4c::backend::lower_to_bir(module);
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_add_u() -> i8 {",
                  "BIR lowering should preserve the widened i8 add signature when the widened instruction type refs only agree semantically");
  expect_contains(rendered, "%t0 = bir.add i8 2, 3",
                  "BIR lowering should lower widened i8 add/trunc slices from typed widths instead of stale render text");
  expect_contains(rendered, "bir.ret i8 %t0",
                  "BIR lowering should keep the widened add result on the direct BIR path when typed widths stay authoritative");
}

void test_bir_lowering_accepts_tiny_return_sub_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_sub_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "%t0 = bir.sub i32 3, 3",
                  "BIR lowering should materialize the tiny sub slice in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should return the named BIR sub result");
}

void test_bir_lowering_accepts_tiny_return_sext_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_sext_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @widen_signed(i32 %p.x) -> i64 {",
                  "BIR lowering should preserve widened return signatures for straight-line sext slices");
  expect_contains(rendered, "%t0 = bir.sext i32 %p.x to i64",
                  "BIR lowering should materialize straight-line sext slices in BIR terms");
  expect_contains(rendered, "bir.ret i64 %t0",
                  "BIR lowering should return the named BIR sext result");
}

void test_bir_lowering_accepts_tiny_return_zext_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_zext_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @widen_unsigned(i8 %p.x) -> i32 {",
                  "BIR lowering should preserve widened parameter signatures for straight-line zext slices");
  expect_contains(rendered, "%t0 = bir.zext i8 %p.x to i32",
                  "BIR lowering should materialize straight-line zext slices in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should return the named BIR zext result");
}

void test_bir_lowering_accepts_typed_tiny_return_zext_lir_slice() {
  auto module = make_bir_return_zext_module();
  c4c::codegen::lir::verify_module(module);

  const auto lowered = c4c::backend::lower_to_bir(module);
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @widen_unsigned(i8 %p.x) -> i32 {",
                  "BIR lowering should preserve the typed widened parameter signature for straight-line zext slices");
  expect_contains(rendered, "%t0 = bir.zext i8 %p.x to i32",
                  "BIR lowering should materialize typed straight-line zext slices in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should return the typed zext result on the direct BIR path");
}

void test_bir_lowering_accepts_typed_i8_return_ne_lir_slice_with_stale_text() {
  auto module = make_bir_i8_return_ne_module();
  auto& entry = module.functions.front().blocks.front();
  auto& cmp = std::get<c4c::codegen::lir::LirCmpOp>(entry.insts[0]);
  auto& cond_cast = std::get<c4c::codegen::lir::LirCastOp>(entry.insts[1]);
  auto& trunc = std::get<c4c::codegen::lir::LirCastOp>(entry.insts[2]);
  auto& ret = std::get<c4c::codegen::lir::LirRet>(entry.terminator);
  cmp.type_str = make_test_stale_text_i32_lir_type();
  cond_cast.from_type = make_test_stale_text_i1_lir_type();
  cond_cast.to_type = make_test_stale_text_i32_lir_type();
  trunc.from_type = make_test_stale_text_i32_lir_type();
  trunc.to_type = make_test_stale_text_i8_lir_type();
  ret.type_str = "i32";

  const auto lowered = c4c::backend::lower_to_bir(module);
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_ne_u() -> i8 {",
                  "BIR lowering should preserve the widened i8 compare-return signature when compare and cast type refs only agree semantically");
  expect_contains(rendered, "%t1 = bir.ne i8 7, 3",
                  "BIR lowering should lower widened compare materialization from typed widths instead of stale i32/i8 text");
  expect_contains(rendered, "bir.ret i8 %t1",
                  "BIR lowering should keep the widened compare result on the direct BIR path when only the typed widths agree");
}

void test_bir_lowering_accepts_typed_tiny_return_ne_lir_slice_with_stale_text() {
  auto module = make_bir_return_ne_module();
  auto& entry = module.functions.front().blocks.front();
  auto& cmp = std::get<c4c::codegen::lir::LirCmpOp>(entry.insts[0]);
  auto& cond_cast = std::get<c4c::codegen::lir::LirCastOp>(entry.insts[1]);
  auto& ret = std::get<c4c::codegen::lir::LirRet>(entry.terminator);
  cmp.type_str = make_test_stale_text_i32_lir_type();
  cond_cast.from_type = make_test_stale_text_i1_lir_type();
  cond_cast.to_type = make_test_stale_text_i32_lir_type();
  ret.type_str = "i8";

  const auto lowered = c4c::backend::lower_to_bir(module);
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "BIR lowering should preserve the straight-line compare-return signature when compare and cast types only agree semantically");
  expect_contains(rendered, "%t1 = bir.ne i32 7, 3",
                  "BIR lowering should materialize the tiny compare-return slice from typed widths instead of stale render text");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "BIR lowering should return the typed compare result on the direct BIR path when only semantic widths agree");
}

void test_bir_lowering_accepts_tiny_return_trunc_lir_slice() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_return_trunc_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @narrow_signed(i64 %p.x) -> i32 {",
                  "BIR lowering should preserve narrowed parameter signatures for straight-line trunc slices");
  expect_contains(rendered, "%t0 = bir.trunc i64 %p.x to i32",
                  "BIR lowering should materialize straight-line trunc slices in BIR terms");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "BIR lowering should return the named BIR trunc result");
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

void test_bir_lowering_accepts_i8_return_lshr() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_lshr_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_lshr_u() -> i8 {",
                  "BIR lowering should preserve the widened i8 lshr wrapper signature");
  expect_contains(rendered, "%t0 = bir.lshr i8 16, 2",
                  "BIR lowering should narrow the widened lshr slice back to direct i8 BIR");
  expect_contains(rendered, "bir.ret i8 %t0",
                  "BIR lowering should return the narrowed i8 lshr result directly");
}

void test_bir_lowering_accepts_i8_return_ashr() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_ashr_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_ashr_u() -> i8 {",
                  "BIR lowering should keep widened i8 ashr slices on the direct BIR route");
  expect_contains(rendered, "%t0 = bir.ashr i8 -16, 2",
                  "BIR lowering should narrow the widened ashr slice back to direct i8 BIR");
  expect_contains(rendered, "bir.ret i8 %t0",
                  "BIR lowering should return the narrowed i8 ashr result directly");
}

void test_bir_lowering_accepts_i8_return_sdiv() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_sdiv_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_sdiv_u() -> i8 {",
                  "BIR lowering should keep widened i8 sdiv slices on the direct BIR route");
  expect_contains(rendered, "%t0 = bir.sdiv i8 12, 3",
                  "BIR lowering should narrow the widened sdiv slice back to direct i8 BIR");
  expect_contains(rendered, "bir.ret i8 %t0",
                  "BIR lowering should return the narrowed i8 sdiv result directly");
}

void test_bir_lowering_accepts_i8_return_udiv() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_udiv_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_udiv_u() -> i8 {",
                  "BIR lowering should keep widened i8 udiv slices on the direct BIR route");
  expect_contains(rendered, "%t0 = bir.udiv i8 12, 3",
                  "BIR lowering should narrow the widened udiv slice back to direct i8 BIR");
  expect_contains(rendered, "bir.ret i8 %t0",
                  "BIR lowering should return the narrowed i8 udiv result directly");
}

void test_bir_lowering_accepts_i8_return_srem() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_srem_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_srem_u() -> i8 {",
                  "BIR lowering should keep widened i8 srem slices on the direct BIR route");
  expect_contains(rendered, "%t0 = bir.srem i8 14, 5",
                  "BIR lowering should narrow the widened srem slice back to direct i8 BIR");
  expect_contains(rendered, "bir.ret i8 %t0",
                  "BIR lowering should return the narrowed i8 srem result directly");
}

void test_bir_lowering_accepts_i8_return_urem() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_urem_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_urem_u() -> i8 {",
                  "BIR lowering should keep widened i8 urem slices on the direct BIR route");
  expect_contains(rendered, "%t0 = bir.urem i8 14, 5",
                  "BIR lowering should narrow the widened urem slice back to direct i8 BIR");
  expect_contains(rendered, "bir.ret i8 %t0",
                  "BIR lowering should return the narrowed i8 urem result directly");
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

void test_bir_lowering_accepts_i8_return_sle() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_sle_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_sle_u() -> i8 {",
                  "BIR lowering should preserve the zero-parameter widened i8 compare-return signature for signed less-than-or-equal");
  expect_contains(rendered, "%t5 = bir.sle i8 7, 7",
                  "BIR lowering should fold the widened i8 signed less-than-or-equal compare-return zext-plus-trunc shape into a direct BIR compare");
  expect_contains(rendered, "bir.ret i8 %t5",
                  "BIR lowering should return the widened i8 signed less-than-or-equal compare result instead of falling back to legacy LLVM IR");
}

void test_bir_lowering_accepts_i8_return_sgt() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_sgt_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_sgt_u() -> i8 {",
                  "BIR lowering should preserve the zero-parameter widened i8 compare-return signature for signed greater-than");
  expect_contains(rendered, "%t5 = bir.sgt i8 7, 3",
                  "BIR lowering should fold the widened i8 signed greater-than compare-return zext-plus-trunc shape into a direct BIR compare");
  expect_contains(rendered, "bir.ret i8 %t5",
                  "BIR lowering should return the widened i8 signed greater-than compare result instead of falling back to legacy LLVM IR");
}

void test_bir_lowering_accepts_i8_return_sge() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_sge_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_sge_u() -> i8 {",
                  "BIR lowering should preserve the zero-parameter widened i8 compare-return signature for signed greater-than-or-equal");
  expect_contains(rendered, "%t5 = bir.sge i8 7, 7",
                  "BIR lowering should fold the widened i8 signed greater-than-or-equal compare-return zext-plus-trunc shape into a direct BIR compare");
  expect_contains(rendered, "bir.ret i8 %t5",
                  "BIR lowering should return the widened i8 signed greater-than-or-equal compare result instead of falling back to legacy LLVM IR");
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

void test_bir_lowering_accepts_typed_single_param_select_branch_slice_with_stale_text() {
  auto module = make_bir_single_param_select_eq_branch_module();
  auto& entry = module.functions.front().blocks.front();
  auto& cmp0 = std::get<c4c::codegen::lir::LirCmpOp>(entry.insts[0]);
  auto& cast = std::get<c4c::codegen::lir::LirCastOp>(entry.insts[1]);
  auto& cmp1 = std::get<c4c::codegen::lir::LirCmpOp>(entry.insts[2]);
  cmp0.type_str = make_test_stale_text_i32_lir_type();
  cast.from_type = make_test_stale_text_i1_lir_type();
  cast.to_type = make_test_stale_text_i32_lir_type();
  cmp1.type_str = make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirRet>(module.functions.front().blocks[1].terminator).type_str = "i8";
  std::get<c4c::codegen::lir::LirRet>(module.functions.front().blocks[2].terminator).type_str = "i8";

  const auto lowered = c4c::backend::lower_to_bir(module);
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose(i32 %p.x) -> i32 {",
                  "BIR lowering should preserve the one-parameter branch-return ternary when compare, cast, and branch return types only agree semantically");
  expect_contains(rendered, "%t.select = bir.select eq i32 %p.x, 7, 11, 4",
                  "BIR lowering should collapse the branch-return ternary from typed integer widths instead of stale render text");
  expect_contains(rendered, "bir.ret i32 %t.select",
                  "BIR lowering should return the typed branch-select result on the direct BIR path when only semantic widths agree");
}

void test_bir_lowering_accepts_typed_single_param_select_branch_slice_with_stale_return_text() {
  auto module = make_bir_single_param_select_eq_branch_module();
  std::get<c4c::codegen::lir::LirRet>(module.functions.front().blocks[1].terminator).type_str = "i8";
  std::get<c4c::codegen::lir::LirRet>(module.functions.front().blocks[2].terminator).type_str = "i8";

  const auto lowered = c4c::backend::lower_to_bir(module);
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose(i32 %p.x) -> i32 {",
                  "BIR lowering should preserve the one-parameter branch-return ternary when the branch return text is stale but the function metadata still carries the right scalar type");
  expect_contains(rendered, "%t.select = bir.select eq i32 %p.x, 7, 11, 4",
                  "BIR lowering should keep the bounded branch-return select on the typed path when only the branch return text is stale");
  expect_contains(rendered, "bir.ret i32 %t.select",
                  "BIR lowering should return the typed branch-select result instead of trusting stale branch return text");
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

void test_bir_lowering_accepts_typed_single_param_select_phi_slice_with_stale_text() {
  auto module = make_bir_single_param_select_eq_phi_module();
  auto& entry = module.functions.front().blocks.front();
  auto& cmp0 = std::get<c4c::codegen::lir::LirCmpOp>(entry.insts[0]);
  auto& cast = std::get<c4c::codegen::lir::LirCastOp>(entry.insts[1]);
  auto& cmp1 = std::get<c4c::codegen::lir::LirCmpOp>(entry.insts[2]);
  cmp0.type_str = make_test_stale_text_i32_lir_type();
  cast.from_type = make_test_stale_text_i1_lir_type();
  cast.to_type = make_test_stale_text_i32_lir_type();
  cmp1.type_str = make_test_stale_text_i32_lir_type();
  auto& join_block = module.functions.front().blocks.back();
  std::get<c4c::codegen::lir::LirPhiOp>(join_block.insts.front()).type_str = make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirRet>(join_block.terminator).type_str = "i8";

  const auto lowered = c4c::backend::lower_to_bir(module);
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose(i32 %p.x) -> i32 {",
                  "BIR lowering should preserve the one-parameter phi ternary when compare, cast, phi, and return types only agree semantically");
  expect_contains(rendered, "%t8 = bir.select eq i32 %p.x, 7, 11, 4",
                  "BIR lowering should collapse the phi-join ternary from typed integer widths instead of stale render text");
  expect_contains(rendered, "bir.ret i32 %t8",
                  "BIR lowering should return the typed phi-select result on the direct BIR path when only semantic widths agree");
}

void test_bir_lowering_accepts_typed_single_param_select_phi_slice_with_stale_phi_text() {
  auto module = make_bir_single_param_select_eq_phi_module();
  auto& join_block = module.functions.front().blocks.back();
  std::get<c4c::codegen::lir::LirPhiOp>(join_block.insts.front()).type_str = make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirRet>(join_block.terminator).type_str = "i8";

  const auto lowered = c4c::backend::lower_to_bir(module);
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose(i32 %p.x) -> i32 {",
                  "BIR lowering should preserve the one-parameter phi ternary when the phi and join return text are stale but their typed semantics still match the enclosing function");
  expect_contains(rendered, "%t8 = bir.select eq i32 %p.x, 7, 11, 4",
                  "BIR lowering should collapse the phi-join ternary from typed phi semantics instead of raw phi text equality");
  expect_contains(rendered, "bir.ret i32 %t8",
                  "BIR lowering should return the typed phi-select result instead of trusting stale phi or join return text");
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

void test_bir_lowering_accepts_two_param_u8_select_ne_phi_slice() {
  const auto lowered =
      c4c::backend::lower_to_bir(make_bir_two_param_select_ne_phi_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose2_ne_u(i8 %p.x, i8 %p.y) -> i8 {",
                  "BIR lowering should preserve the widened two-parameter unsigned-char ternary signature for not-equal");
  expect_contains(rendered, "%t5 = bir.select ne i8 %p.x, %p.y, %p.x, %p.y",
                  "BIR lowering should collapse the widened two-parameter not-equal phi join into a direct i8 BIR select");
  expect_contains(rendered, "bir.ret i8 %t5",
                  "BIR lowering should return the widened select result instead of falling back to legacy LLVM IR");
}

void test_bir_lowering_accepts_typed_two_param_u8_select_ne_phi_slice() {
  auto module = make_bir_two_param_select_ne_phi_module();
  c4c::codegen::lir::verify_module(module);

  const auto lowered = c4c::backend::lower_to_bir(module);
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose2_ne_u(i8 %p.x, i8 %p.y) -> i8 {",
                  "BIR lowering should preserve the typed widened two-parameter unsigned-char ternary signature for not-equal");
  expect_contains(rendered, "%t5 = bir.select ne i8 %p.x, %p.y, %p.x, %p.y",
                  "BIR lowering should collapse the typed widened two-parameter not-equal phi join into a direct i8 BIR select");
  expect_contains(rendered, "bir.ret i8 %t5",
                  "BIR lowering should return the typed widened select result on the direct BIR path");
}

void test_bir_lowering_accepts_typed_two_param_u8_select_ne_branch_slice_with_stale_text() {
  auto module = make_bir_two_param_select_ne_branch_module();
  auto& entry = module.functions.front().blocks.front();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[0]).from_type = make_test_stale_text_i8_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[0]).to_type = make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[1]).from_type = make_test_stale_text_i8_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[1]).to_type = make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirCmpOp>(entry.insts[2]).type_str = make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[3]).from_type = make_test_stale_text_i1_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[3]).to_type = make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirCmpOp>(entry.insts[4]).type_str = make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirRet>(module.functions.front().blocks[1].terminator).type_str = "i32";
  std::get<c4c::codegen::lir::LirRet>(module.functions.front().blocks[2].terminator).type_str = "i32";

  const auto lowered = c4c::backend::lower_to_bir(module);
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose2_ne_branch_u(i8 %p.x, i8 %p.y) -> i8 {",
                  "BIR lowering should preserve the widened branch-return unsigned-char ternary signature when branch return text is stale");
  expect_contains(rendered, "%t.select = bir.select ne i8 %p.x, %p.y, %p.x, %p.y",
                  "BIR lowering should collapse the widened branch-return not-equal ternary from semantic widths instead of stale i8/i32 render text");
  expect_contains(rendered, "bir.ret i8 %t.select",
                  "BIR lowering should return the widened branch-return select result on the direct BIR path when typed widths stay authoritative");
}

void test_bir_lowering_accepts_typed_two_param_u8_select_ne_phi_slice_with_stale_text() {
  auto module = make_bir_two_param_select_ne_phi_module();
  auto& entry = module.functions.front().blocks.front();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[0]).from_type = make_test_stale_text_i8_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[0]).to_type = make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[1]).from_type = make_test_stale_text_i8_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[1]).to_type = make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirCmpOp>(entry.insts[2]).type_str = make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[3]).from_type = make_test_stale_text_i1_lir_type();
  std::get<c4c::codegen::lir::LirCastOp>(entry.insts[3]).to_type = make_test_stale_text_i32_lir_type();
  std::get<c4c::codegen::lir::LirCmpOp>(entry.insts[4]).type_str = make_test_stale_text_i32_lir_type();
  auto& join_block = module.functions.front().blocks.back();
  std::get<c4c::codegen::lir::LirPhiOp>(join_block.insts.front()).type_str = make_test_stale_text_i8_lir_type();
  std::get<c4c::codegen::lir::LirRet>(join_block.terminator).type_str = "i32";

  const auto lowered = c4c::backend::lower_to_bir(module);
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose2_ne_u(i8 %p.x, i8 %p.y) -> i8 {",
                  "BIR lowering should preserve the typed widened two-parameter unsigned-char ternary signature when entry and phi type refs only agree semantically");
  expect_contains(rendered, "%t5 = bir.select ne i8 %p.x, %p.y, %p.x, %p.y",
                  "BIR lowering should collapse the widened not-equal phi join from semantic widths instead of stale i8/i32 render text");
  expect_contains(rendered, "bir.ret i8 %t5",
                  "BIR lowering should return the widened select result on the direct BIR path when typed widths stay authoritative");
}

void test_bir_lowering_accepts_two_param_u8_select_ne_predecessor_add_phi_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_u8_select_ne_predecessor_add_phi_post_join_add_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered,
                  "bir.func @choose2_add_post_ne_u(i8 %p.x, i8 %p.y) -> i8 {",
                  "BIR lowering should preserve the widened unsigned-char predecessor-add ternary signature for not-equal");
  expect_contains(rendered, "%t6 = bir.add i8 %p.x, 5",
                  "BIR lowering should hoist the widened then-arm predecessor arithmetic for the not-equal predecessor-add slice");
  expect_contains(rendered, "%t9 = bir.add i8 %p.y, 9",
                  "BIR lowering should hoist the widened else-arm predecessor arithmetic for the not-equal predecessor-add slice");
  expect_contains(rendered, "%t11 = bir.select ne i8 %p.x, %p.y, %t6, %t9",
                  "BIR lowering should collapse the widened predecessor-add phi join into a direct i8 not-equal select");
  expect_contains(rendered, "%t12 = bir.add i8 %t11, 6",
                  "BIR lowering should preserve the widened join-local add after the fused not-equal select");
  expect_contains(rendered, "bir.ret i8 %t12",
                  "BIR lowering should return the widened not-equal predecessor-add result on the direct BIR path");
}

void test_bir_lowering_accepts_two_param_u8_select_ne_split_predecessor_add_phi_post_join_add_sub_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_u8_select_ne_split_predecessor_add_phi_post_join_add_sub_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered,
                  "bir.func @choose2_add_post_chain_ne_u(i8 %p.x, i8 %p.y) -> i8 {",
                  "BIR lowering should preserve the widened unsigned-char split-predecessor ternary signature for the adjacent not-equal add/sub tail slice");
  expect_contains(rendered, "%t6 = bir.add i8 %p.x, 5",
                  "BIR lowering should hoist the widened then-arm predecessor arithmetic for the split-predecessor not-equal slice");
  expect_contains(rendered, "%t9 = bir.add i8 %p.y, 9",
                  "BIR lowering should hoist the widened else-arm predecessor arithmetic for the split-predecessor not-equal slice");
  expect_contains(rendered, "%t11 = bir.select ne i8 %p.x, %p.y, %t6, %t9",
                  "BIR lowering should collapse the widened split-predecessor phi join into a direct i8 not-equal select");
  expect_contains(rendered, "%t12 = bir.add i8 %t11, 6",
                  "BIR lowering should preserve the widened join-local add after the fused split-predecessor not-equal select");
  expect_contains(rendered, "%t13 = bir.sub i8 %t12, 2",
                  "BIR lowering should preserve the widened join-local subtraction after the fused split-predecessor not-equal select");
  expect_contains(rendered, "bir.ret i8 %t13",
                  "BIR lowering should return the widened split-predecessor not-equal add/sub chain on the direct BIR path");
}

void test_bir_lowering_accepts_two_param_u8_select_ne_split_predecessor_add_phi_post_join_add_sub_add_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_u8_select_ne_split_predecessor_add_phi_post_join_add_sub_add_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered,
                  "bir.func @choose2_add_post_chain_tail_ne_u(i8 %p.x, i8 %p.y) -> i8 {",
                  "BIR lowering should preserve the widened unsigned-char split-predecessor ternary signature for the adjacent not-equal add/sub/add tail slice");
  expect_contains(rendered, "%t6 = bir.add i8 %p.x, 5",
                  "BIR lowering should hoist the widened then-arm predecessor arithmetic for the split-predecessor not-equal add/sub/add slice");
  expect_contains(rendered, "%t9 = bir.add i8 %p.y, 9",
                  "BIR lowering should hoist the widened else-arm predecessor arithmetic for the split-predecessor not-equal add/sub/add slice");
  expect_contains(rendered, "%t11 = bir.select ne i8 %p.x, %p.y, %t6, %t9",
                  "BIR lowering should collapse the widened split-predecessor phi join into a direct i8 not-equal select for the add/sub/add tail slice");
  expect_contains(rendered, "%t12 = bir.add i8 %t11, 6",
                  "BIR lowering should preserve the widened join-local add after the fused split-predecessor not-equal select");
  expect_contains(rendered, "%t13 = bir.sub i8 %t12, 2",
                  "BIR lowering should preserve the widened join-local subtraction after the fused split-predecessor not-equal select");
  expect_contains(rendered, "%t14 = bir.add i8 %t13, 9",
                  "BIR lowering should preserve the widened trailing join-local add after the fused split-predecessor not-equal select");
  expect_contains(rendered, "bir.ret i8 %t14",
                  "BIR lowering should return the widened split-predecessor not-equal add/sub/add chain on the direct BIR path");
}

void test_bir_lowering_accepts_two_param_u8_select_ne_split_predecessor_mixed_affine_phi_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_u8_select_ne_split_predecessor_mixed_affine_phi_post_join_add_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered,
                  "bir.func @choose2_mixed_post_ne_u(i8 %p.x, i8 %p.y) -> i8 {",
                  "BIR lowering should preserve the widened unsigned-char split-predecessor mixed-affine ternary signature for the adjacent not-equal post-add slice");
  expect_contains(rendered, "%t6 = bir.add i8 %p.x, 8",
                  "BIR lowering should hoist the widened then-arm mixed-affine head for the split-predecessor not-equal slice");
  expect_contains(rendered, "%t7 = bir.sub i8 %t6, 3",
                  "BIR lowering should preserve the widened then-arm mixed-affine tail before the fused not-equal select");
  expect_contains(rendered, "%t10 = bir.add i8 %p.y, 11",
                  "BIR lowering should hoist the widened else-arm mixed-affine head for the split-predecessor not-equal slice");
  expect_contains(rendered, "%t11 = bir.sub i8 %t10, 4",
                  "BIR lowering should preserve the widened else-arm mixed-affine tail before the fused not-equal select");
  expect_contains(rendered, "%t13 = bir.select ne i8 %p.x, %p.y, %t7, %t11",
                  "BIR lowering should collapse the widened split-predecessor mixed-affine phi join into a direct i8 not-equal select");
  expect_contains(rendered, "%t14 = bir.add i8 %t13, 6",
                  "BIR lowering should preserve the widened join-local add after the fused split-predecessor mixed-affine not-equal select");
  expect_contains(rendered, "bir.ret i8 %t14",
                  "BIR lowering should return the widened split-predecessor mixed-affine not-equal post-add result on the direct BIR path");
}

void test_bir_lowering_accepts_two_param_u8_select_ne_split_predecessor_mixed_affine_phi_post_join_add_sub_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_u8_select_ne_split_predecessor_mixed_affine_phi_post_join_add_sub_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered,
                  "bir.func @choose2_mixed_post_chain_ne_u(i8 %p.x, i8 %p.y) -> i8 {",
                  "BIR lowering should preserve the widened unsigned-char split-predecessor mixed-affine ternary signature for the adjacent not-equal post-add/sub slice");
  expect_contains(rendered, "%t6 = bir.add i8 %p.x, 8",
                  "BIR lowering should hoist the widened then-arm mixed-affine head for the split-predecessor not-equal post-add/sub slice");
  expect_contains(rendered, "%t7 = bir.sub i8 %t6, 3",
                  "BIR lowering should preserve the widened then-arm mixed-affine tail before the fused not-equal select in the post-add/sub slice");
  expect_contains(rendered, "%t10 = bir.add i8 %p.y, 11",
                  "BIR lowering should hoist the widened else-arm mixed-affine head for the split-predecessor not-equal post-add/sub slice");
  expect_contains(rendered, "%t11 = bir.sub i8 %t10, 4",
                  "BIR lowering should preserve the widened else-arm mixed-affine tail before the fused not-equal select in the post-add/sub slice");
  expect_contains(rendered, "%t13 = bir.select ne i8 %p.x, %p.y, %t7, %t11",
                  "BIR lowering should collapse the widened split-predecessor mixed-affine phi join into a direct i8 not-equal select for the post-add/sub slice");
  expect_contains(rendered, "%t14 = bir.add i8 %t13, 6",
                  "BIR lowering should preserve the widened join-local add after the fused split-predecessor mixed-affine not-equal select");
  expect_contains(rendered, "%t15 = bir.sub i8 %t14, 2",
                  "BIR lowering should preserve the widened join-local subtraction after the fused split-predecessor mixed-affine not-equal select");
  expect_contains(rendered, "bir.ret i8 %t15",
                  "BIR lowering should return the widened split-predecessor mixed-affine not-equal post-add/sub result on the direct BIR path");
}

void test_bir_lowering_accepts_two_param_u8_select_ne_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_slice() {
  const auto lowered = c4c::backend::lower_to_bir(
      make_bir_two_param_u8_select_ne_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered,
                  "bir.func @choose2_mixed_then_deeper_post_ne_u(i8 %p.x, i8 %p.y) -> i8 {",
                  "BIR lowering should preserve the widened unsigned-char split-predecessor mixed-then-deeper-affine ternary signature for the adjacent not-equal post-add slice");
  expect_contains(rendered, "%t6 = bir.add i8 %p.x, 8",
                  "BIR lowering should hoist the widened then-arm mixed-affine head for the split-predecessor not-equal mixed-then-deeper slice");
  expect_contains(rendered, "%t7 = bir.sub i8 %t6, 3",
                  "BIR lowering should preserve the widened then-arm mixed-affine tail before the fused not-equal select in the mixed-then-deeper slice");
  expect_contains(rendered, "%t10 = bir.add i8 %p.y, 11",
                  "BIR lowering should hoist the widened else-arm deeper-affine head for the split-predecessor not-equal mixed-then-deeper slice");
  expect_contains(rendered, "%t11 = bir.sub i8 %t10, 4",
                  "BIR lowering should preserve the widened else-arm deeper-affine middle step before the fused not-equal select in the mixed-then-deeper slice");
  expect_contains(rendered, "%t12 = bir.add i8 %t11, 7",
                  "BIR lowering should preserve the widened else-arm deeper-affine tail before the fused not-equal select in the mixed-then-deeper slice");
  expect_contains(rendered, "%t14 = bir.select ne i8 %p.x, %p.y, %t7, %t12",
                  "BIR lowering should collapse the widened split-predecessor mixed-then-deeper-affine phi join into a direct i8 not-equal select");
  expect_contains(rendered, "%t15 = bir.add i8 %t14, 6",
                  "BIR lowering should preserve the widened join-local add after the fused split-predecessor mixed-then-deeper-affine not-equal select");
  expect_contains(rendered, "bir.ret i8 %t15",
                  "BIR lowering should return the widened split-predecessor mixed-then-deeper-affine not-equal post-add result on the direct BIR path");
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

void test_bir_lowering_accepts_i8_return_add() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_add_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_add_u() -> i8 {",
                  "BIR lowering should preserve the widened zero-parameter i8 return signature");
  expect_contains(rendered, "%t0 = bir.add i8 2, 3",
                  "BIR lowering should materialize the widened zero-parameter i8 add directly in BIR terms");
  expect_contains(rendered, "bir.ret i8 %t0",
                  "BIR lowering should let the widened zero-parameter i8 add flow directly into the return");
}

void test_bir_lowering_accepts_i8_return_sub() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_sub_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_sub_u() -> i8 {",
                  "BIR lowering should preserve the widened zero-parameter i8 subtract return signature");
  expect_contains(rendered, "%t0 = bir.sub i8 9, 4",
                  "BIR lowering should materialize the widened zero-parameter i8 subtract directly in BIR terms");
  expect_contains(rendered, "bir.ret i8 %t0",
                  "BIR lowering should let the widened zero-parameter i8 subtract flow directly into the return");
}

void test_bir_lowering_accepts_i8_return_mul() {
  auto module = make_bir_i8_return_mul_module();
  auto& block = module.functions.front().blocks.front();
  auto& bin = std::get<c4c::codegen::lir::LirBinOp>(block.insts.front());
  bin.type_str = c4c::codegen::lir::LirTypeRef::integer(8);
  block.terminator = c4c::codegen::lir::LirRet{std::string("%t0"),
                                               c4c::codegen::lir::LirTypeRef::integer(8)};

  c4c::codegen::lir::verify_module(module);

  const auto lowered = c4c::backend::lower_to_bir(module);
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_mul_u() -> i8 {",
                  "BIR lowering should preserve the widened zero-parameter i8 multiply return signature");
  expect_contains(rendered, "%t0 = bir.mul i8 6, 7",
                  "BIR lowering should materialize the widened zero-parameter i8 multiply directly in BIR terms");
  expect_contains(rendered, "bir.ret i8 %t0",
                  "BIR lowering should let the widened zero-parameter i8 multiply flow directly into the return");
}

void test_bir_lowering_accepts_i8_return_and() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_and_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_and_u() -> i8 {",
                  "BIR lowering should preserve the widened zero-parameter i8 bitwise-and return signature");
  expect_contains(rendered, "%t0 = bir.and i8 14, 11",
                  "BIR lowering should materialize the widened zero-parameter i8 bitwise-and directly in BIR terms");
  expect_contains(rendered, "bir.ret i8 %t0",
                  "BIR lowering should let the widened zero-parameter i8 bitwise-and flow directly into the return");
}

void test_bir_lowering_accepts_i8_return_or() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_or_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_or_u() -> i8 {",
                  "BIR lowering should preserve the widened zero-parameter i8 bitwise-or return signature");
  expect_contains(rendered, "%t0 = bir.or i8 12, 3",
                  "BIR lowering should materialize the widened zero-parameter i8 bitwise-or directly in BIR terms");
  expect_contains(rendered, "bir.ret i8 %t0",
                  "BIR lowering should let the widened zero-parameter i8 bitwise-or flow directly into the return");
}

void test_bir_lowering_accepts_i8_return_xor() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_xor_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_xor_u() -> i8 {",
                  "BIR lowering should preserve the widened zero-parameter i8 bitwise-xor return signature");
  expect_contains(rendered, "%t0 = bir.xor i8 12, 10",
                  "BIR lowering should materialize the widened zero-parameter i8 bitwise-xor directly in BIR terms");
  expect_contains(rendered, "bir.ret i8 %t0",
                  "BIR lowering should let the widened zero-parameter i8 bitwise-xor flow directly into the return");
}

void test_bir_lowering_accepts_i8_return_shl() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_return_shl_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @choose_shl_u() -> i8 {",
                  "BIR lowering should preserve the widened zero-parameter i8 left-shift return signature");
  expect_contains(rendered, "%t0 = bir.shl i8 3, 4",
                  "BIR lowering should materialize the widened zero-parameter i8 left-shift directly in BIR terms");
  expect_contains(rendered, "bir.ret i8 %t0",
                  "BIR lowering should let the widened zero-parameter i8 left-shift flow directly into the return");
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

void test_bir_lowering_accepts_i8_two_param_add() {
  const auto lowered = c4c::backend::lower_to_bir(make_bir_i8_two_param_add_module());
  const auto rendered = c4c::backend::bir::print(lowered);

  expect_contains(rendered, "bir.func @add_pair_u(i8 %p.x, i8 %p.y) -> i8 {",
                  "BIR lowering should preserve the widened i8 two-parameter function signature");
  expect_contains(rendered, "%t0 = bir.add i8 %p.x, %p.y",
                  "BIR lowering should materialize the widened i8 two-parameter add in BIR terms");
  expect_contains(rendered, "bir.ret i8 %t0",
                  "BIR lowering should let the widened i8 two-parameter add result flow directly into the return");
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

void test_bir_validator_rejects_non_widening_sext() {
  using namespace c4c::backend::bir;

  auto module = make_return_immediate_module();
  auto& function = module.functions.front();
  function.return_type = TypeKind::I64;
  auto& block = function.blocks.front();
  block.insts.push_back(CastInst{CastOpcode::SExt, Value::named(TypeKind::I32, "%t0"),
                                 Value::immediate_i64(7)});
  block.terminator.value = Value::named(TypeKind::I64, "%t0");
  std::string error;

  expect_true(!c4c::backend::bir::validate(module, &error),
              "BIR validator should reject sext instructions that do not widen the source type");
  expect_contains(error, "must widen the type",
                  "BIR validator should explain why invalid sext instructions are rejected");
}

}  // namespace

void run_backend_bir_lowering_tests() {
  RUN_TEST(test_bir_lowering_accepts_minimal_direct_call_lir_module);
  RUN_TEST(test_bir_lowering_accepts_minimal_direct_call_lir_module_with_typed_helper_return);
  RUN_TEST(test_bir_lowering_accepts_minimal_direct_call_lir_module_with_typed_main_return);
  RUN_TEST(test_bir_lowering_accepts_minimal_void_direct_call_imm_return_lir_module);
  RUN_TEST(test_bir_lowering_accepts_minimal_two_arg_direct_call_lir_module);
  RUN_TEST(test_bir_lowering_accepts_minimal_two_arg_direct_call_local_slot_lir_module);
  RUN_TEST(test_bir_lowering_accepts_minimal_two_arg_direct_call_local_slot_lir_module_with_stale_typed_slot_text);
  RUN_TEST(test_bir_lowering_accepts_minimal_two_arg_direct_call_lir_module_with_typed_helper_params);
  RUN_TEST(test_bir_lowering_accepts_minimal_two_arg_direct_call_lir_module_with_typed_main_return);
  RUN_TEST(test_bir_lowering_accepts_minimal_direct_call_add_imm_lir_module);
  RUN_TEST(test_bir_lowering_accepts_minimal_direct_call_add_imm_lir_module_with_typed_helper_param);
  RUN_TEST(test_bir_lowering_accepts_minimal_direct_call_add_imm_lir_module_with_typed_main_return);
  RUN_TEST(test_bir_lowering_accepts_minimal_direct_call_add_imm_lir_module_with_stale_typed_local_slot_text);
  RUN_TEST(test_bir_lowering_accepts_minimal_direct_call_add_imm_slot_lir_module_with_typed_helper_param);
  RUN_TEST(test_bir_lowering_accepts_minimal_direct_call_identity_arg_lir_module);
  RUN_TEST(test_bir_lowering_accepts_minimal_direct_call_identity_arg_lir_module_with_helper_first);
  RUN_TEST(test_bir_lowering_accepts_minimal_direct_call_identity_arg_lir_module_with_typed_helper_param);
  RUN_TEST(test_bir_lowering_accepts_minimal_direct_call_identity_arg_lir_module_with_typed_main_return);
  RUN_TEST(test_bir_lowering_accepts_minimal_folded_two_arg_direct_call_lir_module);
  RUN_TEST(test_bir_lowering_accepts_minimal_folded_two_arg_direct_call_lir_module_with_typed_helper_params);
  RUN_TEST(test_bir_lowering_accepts_minimal_folded_two_arg_direct_call_lir_module_with_helper_first);
  RUN_TEST(test_bir_lowering_accepts_minimal_dual_identity_direct_call_sub_lir_module);
  RUN_TEST(test_bir_lowering_accepts_minimal_dual_identity_direct_call_sub_lir_module_with_helper_first);
  RUN_TEST(test_bir_lowering_accepts_minimal_dual_identity_direct_call_sub_lir_module_with_typed_helper_params);
  RUN_TEST(test_bir_lowering_accepts_minimal_dual_identity_direct_call_sub_lir_module_with_typed_main_return);
  RUN_TEST(test_bir_lowering_accepts_minimal_call_crossing_direct_call_lir_module);
  RUN_TEST(test_bir_lowering_accepts_minimal_call_crossing_direct_call_lir_module_with_helper_first);
  RUN_TEST(test_bir_lowering_accepts_minimal_call_crossing_direct_call_lir_module_with_typed_helper_param);
  RUN_TEST(test_bir_lowering_accepts_minimal_call_crossing_direct_call_lir_module_with_typed_main_return);
  RUN_TEST(test_bir_lowering_accepts_minimal_goto_only_constant_return_lir_module);
  RUN_TEST(test_bir_lowering_accepts_local_i32_store_load_sub_return_immediate_module);
  RUN_TEST(test_bir_lowering_accepts_local_i32_arithmetic_chain_return_immediate_module);
  RUN_TEST(test_bir_lowering_accepts_two_local_i32_zero_init_return_first_module);
  RUN_TEST(test_bir_lowering_accepts_local_i32_pointer_store_zero_load_return_module);
  RUN_TEST(test_bir_lowering_accepts_local_i32_pointer_gep_zero_load_return_module);
  RUN_TEST(test_bir_lowering_accepts_local_i32_pointer_gep_zero_store_slot_load_return_module);
  RUN_TEST(test_bir_lowering_accepts_local_i32_array_two_slot_sum_sub_three_module);
  RUN_TEST(test_bir_lowering_accepts_local_i32_array_second_slot_pointer_store_zero_load_return_module);
  RUN_TEST(test_bir_lowering_accepts_local_two_field_struct_sub_sub_two_return_module);
  RUN_TEST(test_bir_lowering_accepts_local_struct_pointer_alias_add_sub_three_return_module);
  RUN_TEST(test_bir_lowering_accepts_local_self_referential_struct_pointer_chain_zero_return_module);
  RUN_TEST(test_bir_lowering_accepts_double_indirect_local_pointer_chain_zero_return_module);
  RUN_TEST(test_bir_lowering_accepts_double_indirect_local_store_one_final_branch_return_module);
  RUN_TEST(test_bir_lowering_accepts_minimal_countdown_do_while_lir_module);
  RUN_TEST(test_bir_lowering_accepts_minimal_countdown_do_while_lir_module_with_stale_typed_i32_text);
  RUN_TEST(test_bir_lowering_accepts_double_countdown_guarded_zero_return_lir_module);
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
  RUN_TEST(test_bir_printer_renders_minimal_cast_scaffold);
  RUN_TEST(test_bir_validator_accepts_minimal_return_immediate_scaffold);
  RUN_TEST(test_bir_validator_accepts_minimal_i8_scaffold);
  RUN_TEST(test_bir_validator_accepts_minimal_i64_scaffold);
  RUN_TEST(test_bir_validator_accepts_minimal_cast_scaffold);
  RUN_TEST(test_bir_validator_accepts_minimal_select_scaffold);
  RUN_TEST(test_lir_verify_rejects_typed_integer_text_mismatch);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_add_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_typed_tiny_return_add_lir_slice_with_stale_text);
  RUN_TEST(test_bir_lowering_accepts_typed_i8_return_add_lir_slice_with_stale_text);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_sub_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_sext_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_zext_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_typed_tiny_return_zext_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_typed_i8_return_ne_lir_slice_with_stale_text);
  RUN_TEST(test_bir_lowering_accepts_typed_tiny_return_ne_lir_slice_with_stale_text);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_trunc_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_mul_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_and_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_or_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_xor_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_tiny_return_shl_lir_slice);
  RUN_TEST(test_bir_lowering_accepts_i8_return_lshr);
  RUN_TEST(test_bir_lowering_accepts_i8_return_ashr);
  RUN_TEST(test_bir_lowering_accepts_i8_return_sdiv);
  RUN_TEST(test_bir_lowering_accepts_i8_return_udiv);
  RUN_TEST(test_bir_lowering_accepts_i8_return_srem);
  RUN_TEST(test_bir_lowering_accepts_i8_return_urem);
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
  RUN_TEST(test_bir_lowering_accepts_i8_return_sle);
  RUN_TEST(test_bir_lowering_accepts_i8_return_sgt);
  RUN_TEST(test_bir_lowering_accepts_i8_return_sge);
  RUN_TEST(test_bir_lowering_accepts_i8_return_immediate);
  RUN_TEST(test_bir_lowering_accepts_single_param_select_branch_slice);
  RUN_TEST(test_bir_lowering_accepts_minimal_conditional_return_lir_module_with_empty_bridge_blocks);
  RUN_TEST(test_bir_lowering_accepts_minimal_conditional_return_lir_module_with_asymmetric_empty_bridge);
  RUN_TEST(test_bir_lowering_accepts_minimal_conditional_return_lir_module_with_double_empty_bridge_chain);
  RUN_TEST(test_bir_lowering_accepts_minimal_conditional_return_lir_module_with_mirrored_false_double_empty_bridge_chain);
  RUN_TEST(test_bir_lowering_accepts_minimal_conditional_return_lir_module_with_interleaved_double_empty_bridge_chains);
  RUN_TEST(test_bir_lowering_accepts_param_conditional_return_lir_module_with_interleaved_mixed_depth_bridge_chains);
  RUN_TEST(test_bir_lowering_accepts_typed_single_param_select_branch_slice_with_stale_text);
  RUN_TEST(test_bir_lowering_accepts_typed_single_param_select_branch_slice_with_stale_return_text);
  RUN_TEST(test_bir_lowering_accepts_single_param_select_phi_slice);
  RUN_TEST(test_bir_lowering_accepts_typed_single_param_select_phi_slice_with_stale_text);
  RUN_TEST(test_bir_lowering_accepts_typed_single_param_select_phi_slice_with_stale_phi_text);
  RUN_TEST(test_bir_lowering_accepts_two_param_select_phi_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_u8_select_ne_phi_slice);
  RUN_TEST(test_bir_lowering_accepts_typed_two_param_u8_select_ne_phi_slice);
  RUN_TEST(test_bir_lowering_accepts_typed_two_param_u8_select_ne_branch_slice_with_stale_text);
  RUN_TEST(test_bir_lowering_accepts_typed_two_param_u8_select_ne_phi_slice_with_stale_text);
  RUN_TEST(test_bir_lowering_accepts_two_param_u8_select_ne_predecessor_add_phi_post_join_add_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_u8_select_ne_split_predecessor_add_phi_post_join_add_sub_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_u8_select_ne_split_predecessor_add_phi_post_join_add_sub_add_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_u8_select_ne_split_predecessor_mixed_affine_phi_post_join_add_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_u8_select_ne_split_predecessor_mixed_affine_phi_post_join_add_sub_slice);
  RUN_TEST(test_bir_lowering_accepts_two_param_u8_select_ne_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_slice);
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
  RUN_TEST(test_bir_lowering_accepts_i8_return_add);
  RUN_TEST(test_bir_lowering_accepts_i8_return_sub);
  RUN_TEST(test_bir_lowering_accepts_i8_return_mul);
  RUN_TEST(test_bir_lowering_accepts_i8_return_and);
  RUN_TEST(test_bir_lowering_accepts_i8_return_or);
  RUN_TEST(test_bir_lowering_accepts_i8_return_xor);
  RUN_TEST(test_bir_lowering_accepts_i8_return_shl);
  RUN_TEST(test_bir_lowering_accepts_i8_add_sub_chain);
  RUN_TEST(test_bir_lowering_accepts_i64_add_sub_chain);
  RUN_TEST(test_bir_lowering_accepts_i8_two_param_add);
  RUN_TEST(test_bir_lowering_accepts_single_param_add_sub_chain);
  RUN_TEST(test_bir_lowering_accepts_two_param_add);
  RUN_TEST(test_bir_lowering_accepts_two_param_add_sub_chain);
  RUN_TEST(test_bir_lowering_accepts_two_param_staged_affine_chain);
  RUN_TEST(test_bir_lowering_accepts_minimal_declared_direct_call_lir_module);
  RUN_TEST(test_bir_lowering_accepts_string_literal_strlen_sub_lir_module);
  RUN_TEST(test_bir_lowering_accepts_string_literal_char_sub_lir_module);
  RUN_TEST(test_bir_lowering_accepts_local_i32_store_or_sub_lir_module);
  RUN_TEST(test_bir_lowering_accepts_local_i32_store_and_sub_lir_module);
  RUN_TEST(test_bir_lowering_accepts_local_i32_store_xor_sub_lir_module);
  RUN_TEST(test_bir_lowering_infers_extern_decl_params_from_typed_call_lir_module);
  RUN_TEST(test_bir_lowering_uses_typed_declared_direct_call_metadata_when_text_is_stale);
  RUN_TEST(test_bir_lowering_uses_typed_extern_declared_direct_call_metadata_when_text_is_stale);
  RUN_TEST(test_bir_lowering_uses_actual_fixed_vararg_call_types_for_declared_direct_calls);
  RUN_TEST(test_bir_lowering_uses_typed_zero_arg_vararg_declared_direct_call_metadata_when_text_is_stale);
  RUN_TEST(test_bir_lowering_accepts_minimal_scalar_global_load_lir_module);
  RUN_TEST(test_bir_lowering_accepts_minimal_repeated_zero_arg_call_compare_zero_return_lir_module);
  RUN_TEST(test_bir_lowering_accepts_minimal_string_literal_compare_phi_return_lir_module);
  RUN_TEST(test_bir_lowering_accepts_typed_minimal_scalar_global_load_lir_slice_with_stale_text);
  RUN_TEST(test_bir_lowering_accepts_minimal_extern_scalar_global_load_lir_module);
  RUN_TEST(test_bir_lowering_accepts_typed_minimal_extern_scalar_global_load_lir_slice_with_stale_text);
  RUN_TEST(test_bir_lowering_accepts_minimal_extern_global_array_load_lir_module);
  RUN_TEST(test_bir_lowering_accepts_typed_minimal_extern_global_array_load_lir_slice_with_stale_text);
  RUN_TEST(test_bir_lowering_accepts_minimal_global_char_pointer_diff_lir_module);
  RUN_TEST(test_bir_lowering_accepts_typed_minimal_global_char_pointer_diff_lir_slice_with_stale_text);
  RUN_TEST(test_bir_lowering_accepts_minimal_global_int_pointer_diff_lir_module);
  RUN_TEST(test_bir_lowering_accepts_typed_minimal_global_int_pointer_diff_lir_slice_with_stale_text);
  RUN_TEST(test_bir_lowering_accepts_minimal_scalar_global_store_reload_lir_module);
  RUN_TEST(test_bir_lowering_accepts_typed_minimal_scalar_global_store_reload_lir_slice_with_stale_text);
  RUN_TEST(test_bir_lowering_accepts_zero_initialized_scalar_global_store_reload_lir_module);
  RUN_TEST(test_bir_lowering_accepts_global_two_field_struct_store_sub_sub_lir_module);
  RUN_TEST(test_bir_validator_rejects_returning_undefined_named_value);
  RUN_TEST(test_bir_validator_rejects_return_type_mismatch);
  RUN_TEST(test_bir_validator_rejects_non_widening_sext);
}
