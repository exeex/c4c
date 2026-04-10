#include "backend_bir_test_support.hpp"

#include "../../src/backend/backend.hpp"
#include "../../src/backend/lowering/call_decode.hpp"
#include "../../src/backend/lowering/lir_to_bir.hpp"
#include "../../src/backend/x86/codegen/x86_codegen.hpp"

#include <stdexcept>

namespace {

c4c::codegen::lir::LirModule make_double_countdown_guarded_zero_return_module() {
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

c4c::codegen::lir::LirModule make_lir_minimal_global_char_pointer_diff_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
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
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

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

c4c::codegen::lir::LirModule make_x86_local_temp_lir_module() {
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
  entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

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

c4c::codegen::lir::LirModule make_local_i32_array_pointer_inc_dec_compare_zero_return_module() {
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
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::SExt, "i32", "0", "i64"});
  entry.insts.push_back(LirGepOp{"%t2", "i32", "%t0", false, {"i64 %t1"}});
  entry.insts.push_back(LirStoreOp{"i32", "2", "%t2"});
  entry.insts.push_back(LirGepOp{"%t3", "[2 x i32]", "%lv.arr", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t4", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t5", "i32", "%t3", false, {"i64 %t4"}});
  entry.insts.push_back(LirStoreOp{"i32", "3", "%t5"});
  entry.insts.push_back(LirGepOp{"%t6", "[2 x i32]", "%lv.arr", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t7", LirCastKind::SExt, "i32", "0", "i64"});
  entry.insts.push_back(LirGepOp{"%t8", "i32", "%t6", false, {"i64 %t7"}});
  entry.insts.push_back(LirStoreOp{"ptr", "%t8", "%lv.p"});
  entry.insts.push_back(LirLoadOp{"%t9", "ptr", "%lv.p"});
  entry.insts.push_back(LirGepOp{"%t10", "i32", "%t9", false, {"i64 1"}});
  entry.insts.push_back(LirStoreOp{"ptr", "%t10", "%lv.p"});
  entry.insts.push_back(LirLoadOp{"%t11", "i32", "%t9"});
  entry.insts.push_back(LirCmpOp{"%t12", false, "ne", "i32", "%t11", "2"});
  entry.insts.push_back(LirCastOp{"%t13", LirCastKind::ZExt, "i1", "%t12", "i32"});
  entry.insts.push_back(LirCmpOp{"%t14", false, "ne", "i32", "%t13", "0"});
  entry.terminator = LirCondBr{"%t14", "block_1", "block_2"};

  LirBlock block_1;
  block_1.id = LirBlockId{1};
  block_1.label = "block_1";
  block_1.terminator = LirRet{std::string("1"), "i32"};

  LirBlock block_2;
  block_2.id = LirBlockId{2};
  block_2.label = "block_2";
  block_2.insts.push_back(LirLoadOp{"%t15", "ptr", "%lv.p"});
  block_2.insts.push_back(LirGepOp{"%t16", "i32", "%t15", false, {"i64 1"}});
  block_2.insts.push_back(LirStoreOp{"ptr", "%t16", "%lv.p"});
  block_2.insts.push_back(LirLoadOp{"%t17", "i32", "%t15"});
  block_2.insts.push_back(LirCmpOp{"%t18", false, "ne", "i32", "%t17", "3"});
  block_2.insts.push_back(LirCastOp{"%t19", LirCastKind::ZExt, "i1", "%t18", "i32"});
  block_2.insts.push_back(LirCmpOp{"%t20", false, "ne", "i32", "%t19", "0"});
  block_2.terminator = LirCondBr{"%t20", "block_3", "block_4"};

  LirBlock block_3;
  block_3.id = LirBlockId{3};
  block_3.label = "block_3";
  block_3.terminator = LirRet{std::string("2"), "i32"};

  LirBlock block_4;
  block_4.id = LirBlockId{4};
  block_4.label = "block_4";
  block_4.insts.push_back(LirGepOp{"%t21", "[2 x i32]", "%lv.arr", false, {"i64 0", "i64 0"}});
  block_4.insts.push_back(LirCastOp{"%t22", LirCastKind::SExt, "i32", "1", "i64"});
  block_4.insts.push_back(LirGepOp{"%t23", "i32", "%t21", false, {"i64 %t22"}});
  block_4.insts.push_back(LirStoreOp{"ptr", "%t23", "%lv.p"});
  block_4.insts.push_back(LirLoadOp{"%t24", "ptr", "%lv.p"});
  block_4.insts.push_back(LirGepOp{"%t25", "i32", "%t24", false, {"i64 -1"}});
  block_4.insts.push_back(LirStoreOp{"ptr", "%t25", "%lv.p"});
  block_4.insts.push_back(LirLoadOp{"%t26", "i32", "%t24"});
  block_4.insts.push_back(LirCmpOp{"%t27", false, "ne", "i32", "%t26", "3"});
  block_4.insts.push_back(LirCastOp{"%t28", LirCastKind::ZExt, "i1", "%t27", "i32"});
  block_4.insts.push_back(LirCmpOp{"%t29", false, "ne", "i32", "%t28", "0"});
  block_4.terminator = LirCondBr{"%t29", "block_5", "block_6"};

  LirBlock block_5;
  block_5.id = LirBlockId{5};
  block_5.label = "block_5";
  block_5.terminator = LirRet{std::string("1"), "i32"};

  LirBlock block_6;
  block_6.id = LirBlockId{6};
  block_6.label = "block_6";
  block_6.insts.push_back(LirLoadOp{"%t30", "ptr", "%lv.p"});
  block_6.insts.push_back(LirGepOp{"%t31", "i32", "%t30", false, {"i64 -1"}});
  block_6.insts.push_back(LirStoreOp{"ptr", "%t31", "%lv.p"});
  block_6.insts.push_back(LirLoadOp{"%t32", "i32", "%t30"});
  block_6.insts.push_back(LirCmpOp{"%t33", false, "ne", "i32", "%t32", "2"});
  block_6.insts.push_back(LirCastOp{"%t34", LirCastKind::ZExt, "i1", "%t33", "i32"});
  block_6.insts.push_back(LirCmpOp{"%t35", false, "ne", "i32", "%t34", "0"});
  block_6.terminator = LirCondBr{"%t35", "block_7", "block_8"};

  LirBlock block_7;
  block_7.id = LirBlockId{7};
  block_7.label = "block_7";
  block_7.terminator = LirRet{std::string("2"), "i32"};

  LirBlock block_8;
  block_8.id = LirBlockId{8};
  block_8.label = "block_8";
  block_8.insts.push_back(LirGepOp{"%t36", "[2 x i32]", "%lv.arr", false, {"i64 0", "i64 0"}});
  block_8.insts.push_back(LirCastOp{"%t37", LirCastKind::SExt, "i32", "0", "i64"});
  block_8.insts.push_back(LirGepOp{"%t38", "i32", "%t36", false, {"i64 %t37"}});
  block_8.insts.push_back(LirStoreOp{"ptr", "%t38", "%lv.p"});
  block_8.insts.push_back(LirLoadOp{"%t39", "ptr", "%lv.p"});
  block_8.insts.push_back(LirGepOp{"%t40", "i32", "%t39", false, {"i64 1"}});
  block_8.insts.push_back(LirStoreOp{"ptr", "%t40", "%lv.p"});
  block_8.insts.push_back(LirLoadOp{"%t41", "i32", "%t40"});
  block_8.insts.push_back(LirCmpOp{"%t42", false, "ne", "i32", "%t41", "3"});
  block_8.insts.push_back(LirCastOp{"%t43", LirCastKind::ZExt, "i1", "%t42", "i32"});
  block_8.insts.push_back(LirCmpOp{"%t44", false, "ne", "i32", "%t43", "0"});
  block_8.terminator = LirCondBr{"%t44", "block_9", "block_10"};

  LirBlock block_9;
  block_9.id = LirBlockId{9};
  block_9.label = "block_9";
  block_9.terminator = LirRet{std::string("1"), "i32"};

  LirBlock block_10;
  block_10.id = LirBlockId{10};
  block_10.label = "block_10";
  block_10.insts.push_back(LirGepOp{"%t45", "[2 x i32]", "%lv.arr", false, {"i64 0", "i64 0"}});
  block_10.insts.push_back(LirCastOp{"%t46", LirCastKind::SExt, "i32", "1", "i64"});
  block_10.insts.push_back(LirGepOp{"%t47", "i32", "%t45", false, {"i64 %t46"}});
  block_10.insts.push_back(LirStoreOp{"ptr", "%t47", "%lv.p"});
  block_10.insts.push_back(LirLoadOp{"%t48", "ptr", "%lv.p"});
  block_10.insts.push_back(LirGepOp{"%t49", "i32", "%t48", false, {"i64 -1"}});
  block_10.insts.push_back(LirStoreOp{"ptr", "%t49", "%lv.p"});
  block_10.insts.push_back(LirLoadOp{"%t50", "i32", "%t49"});
  block_10.insts.push_back(LirCmpOp{"%t51", false, "ne", "i32", "%t50", "2"});
  block_10.insts.push_back(LirCastOp{"%t52", LirCastKind::ZExt, "i1", "%t51", "i32"});
  block_10.insts.push_back(LirCmpOp{"%t53", false, "ne", "i32", "%t52", "0"});
  block_10.terminator = LirCondBr{"%t53", "block_11", "block_12"};

  LirBlock block_11;
  block_11.id = LirBlockId{11};
  block_11.label = "block_11";
  block_11.terminator = LirRet{std::string("1"), "i32"};

  LirBlock block_12;
  block_12.id = LirBlockId{12};
  block_12.label = "block_12";
  block_12.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  function.blocks.push_back(std::move(block_3));
  function.blocks.push_back(std::move(block_4));
  function.blocks.push_back(std::move(block_5));
  function.blocks.push_back(std::move(block_6));
  function.blocks.push_back(std::move(block_7));
  function.blocks.push_back(std::move(block_8));
  function.blocks.push_back(std::move(block_9));
  function.blocks.push_back(std::move(block_10));
  function.blocks.push_back(std::move(block_11));
  function.blocks.push_back(std::move(block_12));

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

c4c::codegen::lir::LirModule make_x86_constant_add_mul_sub_return_lir_module() {
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

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "2", "2"});
  entry.insts.push_back(LirBinOp{"%t1", "mul", "i32", "%t0", "2"});
  entry.insts.push_back(LirBinOp{"%t2", "sub", "i32", "%t1", "8"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_x86_constant_div_sub_return_lir_module() {
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

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "sdiv", "i32", "6", "2"});
  entry.insts.push_back(LirBinOp{"%t1", "sub", "i32", "%t0", "3"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_x86_constant_branch_if_return_lir_module(
    std::string predicate,
    std::string lhs,
    std::string rhs) {
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
  entry.insts.push_back(LirCmpOp{"%t0", false, std::move(predicate), "i32", std::move(lhs),
                                 std::move(rhs)});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "block_1", "block_2"};

  LirBlock block_1;
  block_1.id = LirBlockId{1};
  block_1.label = "block_1";
  block_1.terminator = LirRet{std::string("0"), "i32"};

  LirBlock block_2;
  block_2.id = LirBlockId{2};
  block_2.label = "block_2";
  block_2.terminator = LirRet{std::string("1"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_x86_param_slot_add_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

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
  helper_entry.insts.push_back(
      LirBinOp{"%t1", LirBinaryOpcode::Add, "i32", "%t0", "1"});
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

c4c::codegen::lir::LirModule make_x86_local_arg_call_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

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

c4c::codegen::lir::LirModule make_x86_declared_zero_arg_call_lir_module() {
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
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@helper_ext", "", ""});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  LirFunction decl;
  decl.name = "helper_ext";
  decl.is_declaration = true;
  decl.params.push_back({"", c4c::TypeSpec{.base = c4c::TB_VOID}});
  decl.signature_text = "declare i32 @helper_ext()\n";
  decl.return_type.base = c4c::TB_INT;

  module.functions.push_back(std::move(function));
  module.functions.push_back(std::move(decl));
  return module;
}

c4c::codegen::lir::LirModule make_goto_only_constant_return_module() {
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

c4c::codegen::lir::LirModule make_double_indirect_local_pointer_conditional_return_module() {
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

c4c::codegen::lir::LirModule make_alloca_backed_conditional_phi_constant_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

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

c4c::codegen::lir::LirModule make_lir_minimal_signed_i16_local_slot_increment_compare_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i16", "", 2});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCastOp{"%t0", LirCastKind::Trunc, "i32", "0", "i16"});
  entry.insts.push_back(LirStoreOp{"i16", "%t0", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t1", "i16", "%lv.x"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::SExt, "i16", "%t1", "i32"});
  entry.insts.push_back(LirBinOp{"%t3", "add", "i32", "%t2", "1"});
  entry.insts.push_back(LirCastOp{"%t4", LirCastKind::Trunc, "i32", "%t3", "i16"});
  entry.insts.push_back(LirStoreOp{"i16", "%t4", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t5", "i16", "%lv.x"});
  entry.insts.push_back(LirCastOp{"%t6", LirCastKind::SExt, "i16", "%t5", "i32"});
  entry.insts.push_back(LirCmpOp{"%t7", false, "ne", "i32", "%t6", "1"});
  entry.insts.push_back(LirCastOp{"%t8", LirCastKind::ZExt, "i1", "%t7", "i32"});
  entry.insts.push_back(LirCmpOp{"%t9", false, "ne", "i32", "%t8", "0"});
  entry.terminator = LirCondBr{"%t9", "block_1", "block_2"};

  LirBlock block_1;
  block_1.id = LirBlockId{1};
  block_1.label = "block_1";
  block_1.terminator = LirRet{std::string("1"), "i32"};

  LirBlock block_2;
  block_2.id = LirBlockId{2};
  block_2.label = "block_2";
  block_2.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_lir_u8_select_post_join_add_module() {
  auto module = make_bir_two_param_u8_select_ne_predecessor_add_phi_post_join_add_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_global_int_pointer_diff_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
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

c4c::codegen::lir::LirModule make_lir_minimal_repeated_zero_arg_call_compare_zero_return_module() {
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

c4c::codegen::lir::LirModule make_lir_minimal_local_i32_inc_dec_compare_return_zero_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  auto make_helper = [](std::string name, std::string value) {
    LirFunction function;
    function.name = std::move(name);
    function.signature_text = "define i32 @" + function.name + "()\n";
    function.entry = LirBlockId{0};

    LirBlock entry;
    entry.id = LirBlockId{0};
    entry.label = "entry";
    entry.terminator = LirRet{std::move(value), "i32"};
    function.blocks.push_back(std::move(entry));
    return function;
  };

  LirFunction main_function;
  main_function.name = "main";
  main_function.signature_text = "define i32 @main()\n";
  main_function.entry = LirBlockId{0};
  main_function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  main_function.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  auto append_phase = [](LirFunction& function,
                         unsigned phase_index,
                         std::string block_label,
                         std::string helper_name,
                         std::string call_result,
                         std::string load_result,
                         std::string add_result,
                         std::string x_cmp_result,
                         std::string x_cast_result,
                         std::string x_branch_result,
                         std::string y_load_result,
                         std::string y_cmp_result,
                         std::string y_cast_result,
                         std::string y_branch_result,
                         std::string x_expected,
                         std::string y_expected,
                         std::string delta,
                         bool store_updated_to_y,
                         std::string x_true_label,
                         std::string y_check_label,
                         std::string y_true_label,
                         std::string next_label) {
    const std::string x_true_label_copy = x_true_label;
    const std::string y_check_label_copy = y_check_label;
    const std::string y_true_label_copy = y_true_label;
    const std::string next_label_copy = next_label;
    const std::string y_load_result_copy = y_load_result;

    LirBlock block;
    block.id = LirBlockId{phase_index * 4};
    block.label = std::move(block_label);
    block.insts.push_back(LirCallOp{std::move(call_result), "i32", "@" + helper_name, "", ""});
    const auto& call = std::get<LirCallOp>(block.insts.back());
    block.insts.push_back(LirStoreOp{"i32", call.result.str(), "%lv.x"});
    block.insts.push_back(LirLoadOp{std::move(load_result), "i32", "%lv.x"});
    const auto& load = std::get<LirLoadOp>(block.insts.back());
    block.insts.push_back(LirBinOp{std::move(add_result), "add", "i32", load.result.str(), std::move(delta)});
    const auto& add = std::get<LirBinOp>(block.insts.back());
    block.insts.push_back(LirStoreOp{"i32", add.result.str(), "%lv.x"});
    block.insts.push_back(
        LirStoreOp{"i32", store_updated_to_y ? add.result.str() : load.result.str(), "%lv.y"});
    block.insts.push_back(LirLoadOp{std::move(y_load_result), "i32", "%lv.x"});
    const auto& x_load = std::get<LirLoadOp>(block.insts.back());
    block.insts.push_back(
        LirCmpOp{std::move(x_cmp_result), false, "ne", "i32", x_load.result.str(), std::move(x_expected)});
    const auto& x_cmp = std::get<LirCmpOp>(block.insts.back());
    block.insts.push_back(LirCastOp{std::move(x_cast_result), LirCastKind::ZExt, "i1", x_cmp.result.str(), "i32"});
    const auto& x_cast = std::get<LirCastOp>(block.insts.back());
    block.insts.push_back(LirCmpOp{std::move(x_branch_result), false, "ne", "i32", x_cast.result.str(), "0"});
    const auto& x_branch_cmp = std::get<LirCmpOp>(block.insts.back());
    block.terminator =
        LirCondBr{x_branch_cmp.result.str(), x_true_label_copy, y_check_label_copy};
    function.blocks.push_back(std::move(block));

    LirBlock x_true;
    x_true.id = LirBlockId{phase_index * 4 + 1};
    x_true.label = x_true_label_copy;
    x_true.terminator = LirRet{std::string("1"), "i32"};
    function.blocks.push_back(std::move(x_true));

    LirBlock y_check;
    y_check.id = LirBlockId{phase_index * 4 + 2};
    y_check.label = y_check_label_copy;
    y_check.insts.push_back(LirLoadOp{y_load_result_copy, "i32", "%lv.y"});
    const auto& y_load = std::get<LirLoadOp>(y_check.insts.back());
    y_check.insts.push_back(
        LirCmpOp{std::move(y_cmp_result), false, "ne", "i32", y_load.result.str(), std::move(y_expected)});
    const auto& y_cmp = std::get<LirCmpOp>(y_check.insts.back());
    y_check.insts.push_back(LirCastOp{std::move(y_cast_result), LirCastKind::ZExt, "i1", y_cmp.result.str(), "i32"});
    const auto& y_cast = std::get<LirCastOp>(y_check.insts.back());
    y_check.insts.push_back(LirCmpOp{std::move(y_branch_result), false, "ne", "i32", y_cast.result.str(), "0"});
    const auto& y_branch_cmp = std::get<LirCmpOp>(y_check.insts.back());
    y_check.terminator = LirCondBr{y_branch_cmp.result.str(), y_true_label_copy, next_label_copy};
    function.blocks.push_back(std::move(y_check));

    LirBlock y_true;
    y_true.id = LirBlockId{phase_index * 4 + 3};
    y_true.label = y_true_label_copy;
    y_true.terminator = LirRet{std::string("1"), "i32"};
    function.blocks.push_back(std::move(y_true));
  };

  append_phase(main_function, 0, "entry", "zero", "%t0", "%t1", "%t2", "%t4", "%t5", "%t6",
               "%t7", "%t8", "%t9", "%t10", "1", "1", "1", true, "block_1", "block_2",
               "block_3", "block_4");
  append_phase(main_function, 1, "block_4", "one", "%t11", "%t12", "%t13", "%t15", "%t16",
               "%t17", "%t18", "%t19", "%t20", "%t21", "0", "0", "-1", true, "block_5",
               "block_6", "block_7", "block_8");
  append_phase(main_function, 2, "block_8", "zero", "%t22", "%t23", "%t24", "%t26", "%t27",
               "%t28", "%t29", "%t30", "%t31", "%t32", "1", "0", "1", false, "block_9",
               "block_10", "block_11", "block_12");
  append_phase(main_function, 3, "block_12", "one", "%t33", "%t34", "%t35", "%t37", "%t38",
               "%t39", "%t40", "%t41", "%t42", "%t43", "0", "1", "-1", false, "block_13",
               "block_14", "block_15", "block_16");

  LirBlock final_block;
  final_block.id = LirBlockId{16};
  final_block.label = "block_16";
  final_block.terminator = LirRet{std::string("0"), "i32"};
  main_function.blocks.push_back(std::move(final_block));

  module.functions.push_back(make_helper("zero", "0"));
  module.functions.push_back(make_helper("one", "1"));
  module.functions.push_back(std::move(main_function));
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

c4c::codegen::lir::LirModule make_lir_minimal_countdown_do_while_module() {
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

c4c::backend::bir::Module make_bir_minimal_countdown_loop_module() {
  using namespace c4c::backend::bir;

  Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

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

c4c::codegen::lir::LirModule make_lir_minimal_string_literal_compare_phi_return_module() {
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

c4c::codegen::lir::LirModule make_lir_minimal_local_buffer_string_copy_printf_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  module.string_pool.push_back(LirStringConst{"@.str0", "abcdef", 7});
  module.string_pool.push_back(LirStringConst{"@.str1", "%s\\0A", 4});
  module.extern_decls.push_back(LirExternDecl{"strcpy", "ptr"});
  module.extern_decls.push_back(LirExternDecl{"printf", "i32"});

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.a", "[10 x i8]", "", 1});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "[10 x i8]", "%lv.a", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(
      LirGepOp{"%t1", "[7 x i8]", "@.str0", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCallOp{"%t2", "ptr", "@strcpy", "(ptr, ptr)", "ptr %t0, ptr %t1"});
  entry.insts.push_back(
      LirGepOp{"%t3", "[4 x i8]", "@.str1", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(
      LirGepOp{"%t4", "[10 x i8]", "%lv.a", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t5", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t6", "i8", "%t4", false, {"i64 %t5"}});
  entry.insts.push_back(LirCallOp{"%t7", "i32", "@printf", "(ptr, ...)", "ptr %t3, ptr %t6"});
  entry.terminator = LirRet{std::string("0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_lir_source_like_local_buffer_string_copy_printf_module() {
  using namespace c4c::codegen::lir;

  auto module = make_lir_minimal_local_buffer_string_copy_printf_module();
  module.globals.push_back(LirGlobal{
      LirGlobalId{0},
      "stdin",
      {},
      false,
      false,
      "external ",
      "global ",
      "ptr",
      "",
      8,
      true,
  });
  module.globals.push_back(LirGlobal{
      LirGlobalId{1},
      "stdout",
      {},
      false,
      false,
      "external ",
      "global ",
      "ptr",
      "",
      8,
      true,
  });
  module.globals.push_back(LirGlobal{
      LirGlobalId{2},
      "stderr",
      {},
      false,
      false,
      "external ",
      "global ",
      "ptr",
      "",
      8,
      true,
  });
  module.extern_decls.insert(
      module.extern_decls.begin(),
      {
          LirExternDecl{"remove", "i32", "i32"},
          LirExternDecl{"rename", "i32", "i32"},
          LirExternDecl{"fclose", "i32", "i32"},
          LirExternDecl{"puts", "i32", "i32"},
      });
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_counted_printf_ternary_loop_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  module.string_pool.push_back(LirStringConst{"@.str0", "%d\n", 4});
  module.extern_decls.push_back(LirExternDecl{"printf", "i32", "i32"});

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.Count", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.Count"});
  entry.terminator = LirBr{"for.cond.1"};

  LirBlock loop_cond;
  loop_cond.id = LirBlockId{1};
  loop_cond.label = "for.cond.1";
  loop_cond.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.Count"});
  loop_cond.insts.push_back(LirCmpOp{"%t1", false, LirCmpPredicateRef{"slt"}, "i32", "%t0", "10"});
  loop_cond.insts.push_back(LirCastOp{"%t2", LirCastKind::ZExt, "i1", "%t1", "i32"});
  loop_cond.insts.push_back(LirCmpOp{"%t3", false, LirCmpPredicateRef{"ne"}, "i32", "%t2", "0"});
  loop_cond.terminator = LirCondBr{"%t3", "block_1", "block_2"};

  LirBlock loop_latch;
  loop_latch.id = LirBlockId{2};
  loop_latch.label = "for.latch.1";
  loop_latch.insts.push_back(LirLoadOp{"%t4", "i32", "%lv.Count"});
  loop_latch.insts.push_back(LirBinOp{"%t5", "add", "i32", "%t4", "1"});
  loop_latch.insts.push_back(LirStoreOp{"i32", "%t5", "%lv.Count"});
  loop_latch.terminator = LirBr{"for.cond.1"};

  LirBlock loop_body;
  loop_body.id = LirBlockId{3};
  loop_body.label = "block_1";
  loop_body.insts.push_back(LirGepOp{"%t6", "[4 x i8]", "@.str0", false, {"i64 0", "i64 0"}});
  loop_body.insts.push_back(LirLoadOp{"%t7", "i32", "%lv.Count"});
  loop_body.insts.push_back(LirCmpOp{"%t8", false, LirCmpPredicateRef{"slt"}, "i32", "%t7", "5"});
  loop_body.insts.push_back(LirCastOp{"%t9", LirCastKind::ZExt, "i1", "%t8", "i32"});
  loop_body.insts.push_back(LirCmpOp{"%t17", false, LirCmpPredicateRef{"ne"}, "i32", "%t9", "0"});
  loop_body.terminator = LirCondBr{"%t17", "tern.then.11", "tern.else.13"};

  LirBlock then_block;
  then_block.id = LirBlockId{4};
  then_block.label = "tern.then.11";
  then_block.insts.push_back(LirLoadOp{"%t10", "i32", "%lv.Count"});
  then_block.insts.push_back(LirLoadOp{"%t11", "i32", "%lv.Count"});
  then_block.insts.push_back(LirBinOp{"%t12", "mul", "i32", "%t10", "%t11"});
  then_block.terminator = LirBr{"tern.then.end.12"};

  LirBlock then_end;
  then_end.id = LirBlockId{5};
  then_end.label = "tern.then.end.12";
  then_end.terminator = LirBr{"tern.end.15"};

  LirBlock else_block;
  else_block.id = LirBlockId{6};
  else_block.label = "tern.else.13";
  else_block.insts.push_back(LirLoadOp{"%t13", "i32", "%lv.Count"});
  else_block.insts.push_back(LirBinOp{"%t14", "mul", "i32", "%t13", "3"});
  else_block.terminator = LirBr{"tern.else.end.14"};

  LirBlock else_end;
  else_end.id = LirBlockId{7};
  else_end.label = "tern.else.end.14";
  else_end.terminator = LirBr{"tern.end.15"};

  LirBlock join_block;
  join_block.id = LirBlockId{8};
  join_block.label = "tern.end.15";
  join_block.insts.push_back(
      LirPhiOp{"%t15", "i32", {{"%t12", "tern.then.end.12"}, {"%t14", "tern.else.end.14"}}});
  join_block.insts.push_back(
      LirCallOp{"%t16", "i32", "@printf", "(ptr, ...)", "ptr %t6, i32 %t15"});
  join_block.terminator = LirBr{"for.latch.1"};

  LirBlock exit_block;
  exit_block.id = LirBlockId{9};
  exit_block.label = "block_2";
  exit_block.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(loop_cond));
  function.blocks.push_back(std::move(loop_latch));
  function.blocks.push_back(std::move(loop_body));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(then_end));
  function.blocks.push_back(std::move(else_block));
  function.blocks.push_back(std::move(else_end));
  function.blocks.push_back(std::move(join_block));
  function.blocks.push_back(std::move(exit_block));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_lir_source_like_repeated_printf_immediates_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  module.string_pool.push_back(LirStringConst{"@.str0", "%d %d\n", 7});
  module.globals.push_back(LirGlobal{
      LirGlobalId{0},
      "stdin",
      {},
      false,
      false,
      "external ",
      "global ",
      "ptr",
      "",
      8,
      true,
  });
  module.globals.push_back(LirGlobal{
      LirGlobalId{1},
      "stdout",
      {},
      false,
      false,
      "external ",
      "global ",
      "ptr",
      "",
      8,
      true,
  });
  module.extern_decls.push_back(LirExternDecl{"printf", "i32", "i32"});
  module.extern_decls.push_back(LirExternDecl{"fprintf", "i32", "i32"});
  module.extern_decls.push_back(LirExternDecl{"puts", "i32", "i32"});

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.a", "i8", "", 1});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.b", "i16", "", 2});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{"%t0", "[7 x i8]", "@.str0", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCallOp{"%t1", "i32", "@printf", "(ptr, ...)", "ptr %t0, i64 1, i64 1"});
  entry.insts.push_back(LirGepOp{"%t2", "[7 x i8]", "@.str0", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCallOp{"%t3", "i32", "@printf", "(ptr, ...)", "ptr %t2, i64 2, i64 2"});
  entry.terminator = LirRet{std::string("0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_extern_scalar_global_load_module() {
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

c4c::codegen::lir::LirModule make_lir_minimal_extern_global_array_load_module() {
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

c4c::codegen::lir::LirModule make_lir_minimal_scalar_global_store_reload_module() {
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

c4c::codegen::lir::LirModule make_lir_minimal_global_two_field_struct_store_sub_sub_module() {
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

c4c::backend::bir::Module make_bir_minimal_scalar_global_load_module() {
  using namespace c4c::backend::bir;

  Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
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
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
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
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
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

c4c::codegen::lir::LirModule make_lir_string_literal_strlen_sub_module() {
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

c4c::codegen::lir::LirModule make_lir_string_literal_char_sub_module() {
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

c4c::codegen::lir::LirModule make_lir_local_i32_store_or_sub_module() {
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

c4c::codegen::lir::LirModule make_lir_local_i32_store_and_sub_module() {
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

c4c::codegen::lir::LirModule make_lir_local_i32_store_xor_sub_module() {
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

c4c::codegen::lir::LirModule make_lir_minimal_void_direct_call_imm_return_module() {
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

c4c::codegen::lir::LirModule make_lir_minimal_two_arg_direct_call_module() {
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

c4c::codegen::lir::LirModule make_lir_minimal_two_arg_local_arg_direct_call_module() {
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
  main_function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.insts.push_back(LirCallOp{"%t1", "i32", "@add_pair", "(i32, i32)", "i32 %t0, i32 7"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(main_function));
  module.functions.push_back(std::move(helper));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_two_arg_second_local_arg_direct_call_module() {
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
  main_function.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.y"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.y"});
  entry.insts.push_back(LirCallOp{"%t1", "i32", "@add_pair", "(i32, i32)", "i32 5, i32 %t0"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(main_function));
  module.functions.push_back(std::move(helper));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_two_arg_second_local_rewrite_direct_call_module() {
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
  main_function.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.y"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.y"});
  entry.insts.push_back(LirBinOp{"%t1", LirBinaryOpcode::Add, "i32", "%t0", "0"});
  entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.y"});
  entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.y"});
  entry.insts.push_back(LirCallOp{"%t3", "i32", "@add_pair", "(i32, i32)", "i32 5, i32 %t2"});
  entry.terminator = LirRet{std::string("%t3"), "i32"};
  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(main_function));
  module.functions.push_back(std::move(helper));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_two_arg_first_local_rewrite_direct_call_module() {
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
  main_function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.insts.push_back(LirBinOp{"%t1", LirBinaryOpcode::Add, "i32", "%t0", "0"});
  entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  entry.insts.push_back(LirCallOp{"%t3", "i32", "@add_pair", "(i32, i32)", "i32 %t2, i32 7"});
  entry.terminator = LirRet{std::string("%t3"), "i32"};
  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(main_function));
  module.functions.push_back(std::move(helper));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_two_arg_both_local_arg_direct_call_module() {
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
  main_function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  main_function.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.y"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t1", "i32", "%lv.y"});
  entry.insts.push_back(LirCallOp{"%t2", "i32", "@add_pair", "(i32, i32)", "i32 %t0, i32 %t1"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};
  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(main_function));
  module.functions.push_back(std::move(helper));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_two_arg_both_local_first_rewrite_direct_call_module() {
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
  main_function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  main_function.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.y"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.insts.push_back(LirBinOp{"%t1", LirBinaryOpcode::Add, "i32", "%t0", "0"});
  entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.y"});
  entry.insts.push_back(LirCallOp{"%t4", "i32", "@add_pair", "(i32, i32)", "i32 %t2, i32 %t3"});
  entry.terminator = LirRet{std::string("%t4"), "i32"};
  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(main_function));
  module.functions.push_back(std::move(helper));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_two_arg_both_local_second_rewrite_direct_call_module() {
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
  main_function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  main_function.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.y"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.y"});
  entry.insts.push_back(LirBinOp{"%t1", LirBinaryOpcode::Add, "i32", "%t0", "0"});
  entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.y"});
  entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.y"});
  entry.insts.push_back(LirCallOp{"%t4", "i32", "@add_pair", "(i32, i32)", "i32 %t2, i32 %t3"});
  entry.terminator = LirRet{std::string("%t4"), "i32"};
  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(main_function));
  module.functions.push_back(std::move(helper));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_two_arg_both_local_double_rewrite_direct_call_module() {
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
  main_function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  main_function.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.y"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.insts.push_back(LirBinOp{"%t1", LirBinaryOpcode::Add, "i32", "%t0", "0"});
  entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.y"});
  entry.insts.push_back(LirBinOp{"%t3", LirBinaryOpcode::Add, "i32", "%t2", "0"});
  entry.insts.push_back(LirStoreOp{"i32", "%t3", "%lv.y"});
  entry.insts.push_back(LirLoadOp{"%t4", "i32", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t5", "i32", "%lv.y"});
  entry.insts.push_back(LirCallOp{"%t6", "i32", "@add_pair", "(i32, i32)", "i32 %t4, i32 %t5"});
  entry.terminator = LirRet{std::string("%t6"), "i32"};
  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(main_function));
  module.functions.push_back(std::move(helper));
  return module;
}

c4c::codegen::lir::LirModule make_lir_minimal_direct_call_add_imm_module() {
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

c4c::codegen::lir::LirModule make_lir_minimal_direct_call_identity_arg_module() {
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

c4c::codegen::lir::LirModule make_lir_minimal_folded_two_arg_direct_call_module() {
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

c4c::codegen::lir::LirModule make_lir_minimal_conditional_return_module() {
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

c4c::codegen::lir::LirModule make_lir_minimal_dual_identity_direct_call_sub_module() {
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

c4c::codegen::lir::LirModule make_lir_minimal_call_crossing_direct_call_module() {
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

void populate_bir_minimal_call_crossing_direct_call_module(c4c::backend::bir::Module& module) {
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
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

void test_backend_bir_pipeline_drives_x86_direct_bir_minimal_direct_call_end_to_end() {
  c4c::backend::bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
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

  const auto rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{module},
                                make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".type helper, %function\nhelper:\n",
                  "direct BIR minimal direct-call input should emit the helper definition on the x86 backend path");
  expect_contains(rendered, ".globl main",
                  "direct BIR minimal direct-call input should emit the main definition on the x86 backend path");
  expect_contains(rendered, "mov eax, 42",
                  "x86 direct BIR minimal direct-call input should materialize the helper immediate in native asm");
  expect_contains(rendered, "call helper",
                  "x86 direct BIR minimal direct-call input should lower the main-to-helper call without legacy backend IR");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_declared_direct_call_end_to_end() {
  c4c::backend::bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
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

  const auto rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{module},
                                make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".section .rodata",
                  "direct BIR declared direct-call input should materialize referenced string constants on the native x86 path");
  expect_contains(rendered, ".asciz \"hello\\n\"",
                  "direct BIR declared direct-call input should preserve string-constant bytes for native x86 emission");
  expect_contains(rendered, "lea rdi, ",
                  "direct BIR declared direct-call input should materialize pointer-style extern arguments through address formation on the native x86 path");
  expect_contains(rendered, "call puts_like",
                  "direct BIR declared direct-call input should branch to the declared callee without routing through legacy backend IR");
  expect_contains(rendered, "mov eax, 7",
                  "direct BIR declared direct-call input should preserve the fixed immediate return override on the native x86 path");
  expect_not_contains(rendered, "target triple =",
                      "direct BIR declared direct-call input should stay on native x86 asm emission");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_minimal_void_direct_call_imm_return_end_to_end() {
  c4c::backend::bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
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

  const auto rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{module},
                                make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".type voidfn, %function\nvoidfn:\n",
                  "direct BIR void direct-call input should emit the void helper body on the native x86 path");
  expect_contains(rendered, "call voidfn",
                  "direct BIR void direct-call input should preserve the helper call on the native x86 path");
  expect_contains(rendered, "mov eax, 9",
                  "direct BIR void direct-call input should preserve the fixed caller return immediate on the native x86 path");
  expect_not_contains(rendered, "target triple =",
                      "direct BIR void direct-call input should stay on native x86 asm emission");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_direct_call_through_bir_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_direct_call_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".type helper, %function\nhelper:\n",
                  "x86 LIR minimal direct-call input should still emit the helper definition after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, 42",
                  "x86 LIR minimal direct-call input should preserve the helper immediate on the BIR-owned route");
  expect_contains(rendered, "call helper",
                  "x86 LIR minimal direct-call input should lower the helper call on the native x86 path");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR minimal direct-call input should stay on native asm emission instead of falling back to LLVM text");
}

void test_x86_direct_emitter_lowers_minimal_direct_call_via_outer_bir_path() {
  const auto module = make_lir_minimal_direct_call_module();
  const auto shared_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));
  const auto rendered = c4c::backend::x86::emit_module(module);

  expect_contains(rendered, ".type helper, %function\nhelper:\n",
                  "x86 direct emitter should still emit the helper definition when direct-LIR staging declines a BIR-lowerable direct-call module");
  expect_contains(rendered, "mov eax, 42",
                  "x86 direct emitter should preserve the helper immediate after the outer BIR-lowering retry owns the module");
  expect_contains(rendered, "call helper",
                  "x86 direct emitter should still lower the helper call after removing the duplicate inner BIR retry seam");
  expect_not_contains(rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission when the outer BIR path handles the direct-call module");
  expect_true(rendered == shared_rendered,
              "x86 direct emitter should now match the shared backend entrypoint exactly once raw LIR routing is centralized");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_direct_call_with_dead_entry_alloca_end_to_end() {
  const auto module = make_lir_minimal_direct_call_with_dead_entry_alloca_module();
  expect_true(!c4c::backend::try_lower_to_bir(module).has_value(),
              "raw x86 LIR direct-call input with a dead entry alloca should still miss the bounded shared BIR matcher before backend-side pruning");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".type helper, %function\nhelper:",
                  "x86 LIR direct-call input with a dead entry alloca should still emit the helper definition after backend-side pruning");
  expect_contains(rendered, "mov eax, 42",
                  "x86 LIR direct-call input with a dead entry alloca should preserve the helper immediate after backend-side pruning");
  expect_contains(rendered, "call helper",
                  "x86 LIR direct-call input with a dead entry alloca should still lower the helper call on the native x86 path");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR direct-call input with a dead entry alloca should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_void_direct_call_imm_return_through_bir_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_void_direct_call_imm_return_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".type voidfn, %function\nvoidfn:\n",
                  "x86 LIR void direct-call input should still emit the helper definition after routing through the shared BIR path");
  expect_contains(rendered, "call voidfn",
                  "x86 LIR void direct-call input should lower the helper call on the native x86 path");
  expect_contains(rendered, "mov eax, 9",
                  "x86 LIR void direct-call input should preserve the fixed caller return immediate on the BIR-owned route");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR void direct-call input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_declared_direct_call_through_bir_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_declared_direct_call_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".section .rodata",
                  "x86 LIR declared direct-call input should now reach the BIR-owned extern-call emitter path");
  expect_contains(rendered, ".asciz \"hello\\n\"",
                  "x86 LIR declared direct-call input should preserve string bytes through the BIR lowering seam");
  expect_contains(rendered, "call puts_like",
                  "x86 LIR declared direct-call input should still lower the declared call after the x86 LIR-only seam is removed");
  expect_contains(rendered, "mov eax, 7",
                  "x86 LIR declared direct-call input should preserve the fixed immediate return override on the BIR path");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR declared direct-call input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_string_literal_strlen_sub_through_bir_end_to_end() {
  const auto lowered_bir =
      c4c::backend::try_lower_to_bir(make_lir_string_literal_strlen_sub_module());
  expect_true(lowered_bir.has_value(),
              "x86 LIR string-literal strlen/sub input should now lower into the bounded shared constant-return route");
  if (!lowered_bir.has_value()) {
    return;
  }

  expect_true(lowered_bir->functions.size() == 1 &&
                  lowered_bir->functions.front().blocks.size() == 1 &&
                  lowered_bir->functions.front().blocks.front().insts.empty() &&
                  lowered_bir->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return &&
                  lowered_bir->functions.front().blocks.front().terminator.value ==
                      c4c::backend::bir::Value::immediate_i32(0),
              "x86 LIR string-literal strlen/sub lowering should collapse the exact private-string call route to the shared zero return");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_string_literal_strlen_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR string-literal strlen/sub input should materialize the folded zero result on the native backend path");
  expect_not_contains(rendered, "call strlen",
                      "x86 LIR string-literal strlen/sub input should no longer depend on the unsupported direct-LIR call shape after shared lowering");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR string-literal strlen/sub input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_string_literal_char_sub_through_bir_end_to_end() {
  const auto lowered_bir =
      c4c::backend::try_lower_to_bir(make_lir_string_literal_char_sub_module());
  expect_true(lowered_bir.has_value(),
              "x86 LIR string-literal char/sub input should now lower into the bounded shared constant-return route");
  if (!lowered_bir.has_value()) {
    return;
  }

  expect_true(lowered_bir->functions.size() == 1 &&
                  lowered_bir->functions.front().blocks.size() == 1 &&
                  lowered_bir->functions.front().blocks.front().insts.empty() &&
                  lowered_bir->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return &&
                  lowered_bir->functions.front().blocks.front().terminator.value ==
                      c4c::backend::bir::Value::immediate_i32(0),
              "x86 LIR string-literal char/sub lowering should collapse the exact private-string indexed-load route to the shared zero return");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_string_literal_char_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR string-literal char/sub input should materialize the folded zero result on the native backend path");
  expect_not_contains(rendered, "movsx eax, byte ptr",
                      "x86 LIR string-literal char/sub input should no longer depend on the unsupported direct-LIR indexed-load shape after shared lowering");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR string-literal char/sub input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_local_i32_store_or_sub_through_bir_end_to_end() {
  const auto lowered_bir =
      c4c::backend::try_lower_to_bir(make_lir_local_i32_store_or_sub_module());
  expect_true(lowered_bir.has_value(),
              "x86 LIR local i32 store/or/sub input should now lower into the bounded shared constant-return route");
  if (!lowered_bir.has_value()) {
    return;
  }

  expect_true(lowered_bir->functions.size() == 1 &&
                  lowered_bir->functions.front().blocks.size() == 1 &&
                  lowered_bir->functions.front().blocks.front().insts.empty() &&
                  lowered_bir->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return &&
                  lowered_bir->functions.front().blocks.front().terminator.value ==
                      c4c::backend::bir::Value::immediate_i32(0),
              "x86 LIR local i32 store/or/sub lowering should collapse the exact slot-update route to the shared zero return");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_local_i32_store_or_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR local i32 store/or/sub input should materialize the folded zero result on the native backend path");
  expect_not_contains(rendered, "or eax, 4",
                      "x86 LIR local i32 store/or/sub input should no longer depend on the unsupported direct-LIR local slot arithmetic shape after shared lowering");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR local i32 store/or/sub input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_local_i32_store_and_sub_through_bir_end_to_end() {
  const auto lowered_bir =
      c4c::backend::try_lower_to_bir(make_lir_local_i32_store_and_sub_module());
  expect_true(lowered_bir.has_value(),
              "x86 LIR local i32 store/and/sub input should now lower into the bounded shared constant-return route");
  if (!lowered_bir.has_value()) {
    return;
  }

  expect_true(lowered_bir->functions.size() == 1 &&
                  lowered_bir->functions.front().blocks.size() == 1 &&
                  lowered_bir->functions.front().blocks.front().insts.empty() &&
                  lowered_bir->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return &&
                  lowered_bir->functions.front().blocks.front().terminator.value ==
                      c4c::backend::bir::Value::immediate_i32(0),
              "x86 LIR local i32 store/and/sub lowering should collapse the exact slot-update route to the shared zero return");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_local_i32_store_and_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR local i32 store/and/sub input should materialize the folded zero result on the native backend path");
  expect_not_contains(rendered, "and eax, 3",
                      "x86 LIR local i32 store/and/sub input should no longer depend on the unsupported direct-LIR local slot arithmetic shape after shared lowering");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR local i32 store/and/sub input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_local_i32_store_xor_sub_through_bir_end_to_end() {
  const auto lowered_bir =
      c4c::backend::try_lower_to_bir(make_lir_local_i32_store_xor_sub_module());
  expect_true(lowered_bir.has_value(),
              "x86 LIR local i32 store/xor/sub input should now lower into the bounded shared constant-return route");
  if (!lowered_bir.has_value()) {
    return;
  }

  expect_true(lowered_bir->functions.size() == 1 &&
                  lowered_bir->functions.front().blocks.size() == 1 &&
                  lowered_bir->functions.front().blocks.front().insts.empty() &&
                  lowered_bir->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return &&
                  lowered_bir->functions.front().blocks.front().terminator.value ==
                      c4c::backend::bir::Value::immediate_i32(0),
              "x86 LIR local i32 store/xor/sub lowering should collapse the exact slot-update route to the shared zero return");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_local_i32_store_xor_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR local i32 store/xor/sub input should materialize the folded zero result on the native backend path");
  expect_not_contains(rendered, "xor eax, 3",
                      "x86 LIR local i32 store/xor/sub input should no longer depend on the unsupported direct-LIR local slot arithmetic shape after shared lowering");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR local i32 store/xor/sub input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_minimal_countdown_loop_end_to_end() {
  const auto rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{make_bir_minimal_countdown_loop_module()},
                                make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, "mov eax, 5",
                  "x86 direct BIR countdown-loop input should materialize the initial counter on the native backend path");
  expect_contains(rendered, ".Lloop:\n  cmp eax, 0\n  je .Lexit\n",
                  "x86 direct BIR countdown-loop input should preserve the loop test on the native backend path");
  expect_contains(rendered, ".Lbody:\n  sub eax, 1\n  jmp .Lloop\n",
                  "x86 direct BIR countdown-loop input should preserve the decrement-and-backedge sequence on the native backend path");
  expect_not_contains(rendered, "target triple =",
                      "x86 direct BIR countdown-loop input should stay on native asm emission");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_minimal_scalar_global_load_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_minimal_scalar_global_load_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl g_counter",
                  "x86 direct BIR scalar global-load input should still emit the global definition on the native backend path");
  expect_contains(rendered, "  .long 11\n",
                  "x86 direct BIR scalar global-load input should preserve the initialized global payload");
  expect_contains(rendered, "mov eax, dword ptr [rax]",
                  "x86 direct BIR scalar global-load input should lower bir.load_global into a native scalar memory load");
  expect_not_contains(rendered, "target triple =",
                      "x86 direct BIR scalar global-load input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_minimal_extern_scalar_global_load_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_minimal_extern_scalar_global_load_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_not_contains(rendered, ".globl g_counter",
                      "x86 direct BIR extern scalar global-load input should keep the global as an unresolved extern instead of materializing a definition");
  expect_contains(rendered, "mov eax, dword ptr [rax]",
                  "x86 direct BIR extern scalar global-load input should lower bir.load_global into a native scalar memory load");
  expect_not_contains(rendered, "target triple =",
                      "x86 direct BIR extern scalar global-load input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_minimal_scalar_global_store_reload_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_minimal_scalar_global_store_reload_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl g_counter",
                  "x86 direct BIR scalar global store-reload input should still emit the global definition on the native backend path");
  expect_contains(rendered, "  .long 11\n",
                  "x86 direct BIR scalar global store-reload input should preserve the initialized global payload");
  expect_contains(rendered, "mov dword ptr [rax], 7",
                  "x86 direct BIR scalar global store-reload input should lower bir.store_global into a native scalar memory store");
  expect_contains(rendered, "mov eax, dword ptr [rax]",
                  "x86 direct BIR scalar global store-reload input should reload the just-stored scalar value on the native backend path");
  expect_not_contains(rendered, "target triple =",
                      "x86 direct BIR scalar global store-reload input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_minimal_two_arg_direct_call_end_to_end() {
  c4c::backend::bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.functions.push_back(c4c::backend::bir::Function{
      .name = "add_pair",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {
          c4c::backend::bir::Param{.type = c4c::backend::bir::TypeKind::I32, .name = "%lhs"},
          c4c::backend::bir::Param{.type = c4c::backend::bir::TypeKind::I32, .name = "%rhs"},
      },
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {c4c::backend::bir::BinaryInst{
              .opcode = c4c::backend::bir::BinaryOpcode::Add,
              .result = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%sum"),
              .lhs = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%lhs"),
              .rhs = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%rhs"),
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
          .insts = {c4c::backend::bir::CallInst{
              .result = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
              .callee = "add_pair",
              .args = {
                  c4c::backend::bir::Value::immediate_i32(5),
                  c4c::backend::bir::Value::immediate_i32(7),
              },
              .return_type_name = "i32",
          }},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
          },
      }},
      .is_declaration = false,
  });

  const auto rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{module},
                                make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".type add_pair, %function\nadd_pair:\n",
                  "direct BIR two-argument direct-call input should emit the helper definition on the x86 backend path");
  expect_contains(rendered, "mov edi, 5",
                  "direct BIR two-argument direct-call input should materialize the first call immediate on the native x86 path");
  expect_contains(rendered, "mov esi, 7",
                  "direct BIR two-argument direct-call input should materialize the second call immediate on the native x86 path");
  expect_contains(rendered, "add eax, esi",
                  "direct BIR two-argument direct-call input should lower the helper add natively without legacy backend IR");
  expect_not_contains(rendered, "target triple =",
                      "direct BIR two-argument direct-call input should stay on native asm emission");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_minimal_direct_call_add_imm_end_to_end() {
  c4c::backend::bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
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
          .insts = {c4c::backend::bir::CallInst{
              .result = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
              .callee = "add_one",
              .args = {
                  c4c::backend::bir::Value::immediate_i32(5),
              },
              .return_type_name = "i32",
          }},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
          },
      }},
      .is_declaration = false,
  });

  const auto rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{module},
                                make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".type add_one, %function\nadd_one:\n",
                  "direct BIR add-immediate direct-call input should emit the helper definition on the x86 backend path");
  expect_contains(rendered, "mov edi, 5",
                  "direct BIR add-immediate direct-call input should materialize the caller immediate on the native x86 path");
  expect_contains(rendered, "add eax, 1",
                  "direct BIR add-immediate direct-call input should lower the helper add immediate natively without legacy backend IR");
  expect_contains(rendered, "call add_one",
                  "direct BIR add-immediate direct-call input should lower the helper call on the native x86 path");
  expect_not_contains(rendered, "target triple =",
                      "direct BIR add-immediate direct-call input should stay on native asm emission");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_minimal_direct_call_identity_arg_end_to_end() {
  c4c::backend::bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
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
      .name = "main",
      .return_type = c4c::backend::bir::TypeKind::I32,
      .params = {},
      .local_slots = {},
      .blocks = {c4c::backend::bir::Block{
          .label = "entry",
          .insts = {c4c::backend::bir::CallInst{
              .result = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
              .callee = "f",
              .args = {
                  c4c::backend::bir::Value::immediate_i32(0),
              },
              .return_type_name = "i32",
          }},
          .terminator = c4c::backend::bir::ReturnTerminator{
              .value = c4c::backend::bir::Value::named(c4c::backend::bir::TypeKind::I32, "%t0"),
          },
      }},
      .is_declaration = false,
  });

  const auto rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{module},
                                make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".type f, %function\nf:\n",
                  "direct BIR identity direct-call input should emit the helper definition on the x86 backend path");
  expect_contains(rendered, "mov eax, edi",
                  "direct BIR identity direct-call input should preserve the helper identity return on the native x86 path");
  expect_contains(rendered, "mov edi, 0",
                  "direct BIR identity direct-call input should materialize the caller immediate on the native x86 path");
  expect_contains(rendered, "call f",
                  "direct BIR identity direct-call input should lower the helper call on the native x86 path");
  expect_not_contains(rendered, "target triple =",
                      "direct BIR identity direct-call input should stay on native asm emission");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_minimal_dual_identity_direct_call_sub_end_to_end() {
  c4c::backend::bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
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

  const auto rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{module},
                                make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".type f, %function\nf:\n",
                  "direct BIR dual-identity subtraction input should emit the left helper definition on the x86 backend path");
  expect_contains(rendered, ".type g, %function\ng:\n",
                  "direct BIR dual-identity subtraction input should emit the right helper definition on the x86 backend path");
  expect_contains(rendered, "mov edi, 7",
                  "direct BIR dual-identity subtraction input should materialize the left caller immediate on the native x86 path");
  expect_contains(rendered, "mov edi, 3",
                  "direct BIR dual-identity subtraction input should materialize the right caller immediate on the native x86 path");
  expect_contains(rendered, "sub ebx, eax",
                  "direct BIR dual-identity subtraction input should lower the subtraction natively without legacy backend IR");
  expect_not_contains(rendered, "target triple =",
                      "direct BIR dual-identity subtraction input should stay on native asm emission");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_minimal_call_crossing_direct_call_end_to_end() {
  c4c::backend::bir::Module module;
  populate_bir_minimal_call_crossing_direct_call_module(module);

  const auto rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{module},
                                make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".type add_one, %function\nadd_one:\n",
                  "direct BIR call-crossing input should emit the helper definition on the x86 backend path");
  expect_contains(rendered, ".globl main",
                  "direct BIR call-crossing input should emit the caller definition on the x86 backend path");
  expect_contains(rendered, "mov eax, edi\n  add eax, 1\n  ret\n",
                  "direct BIR call-crossing input should preserve the helper add-immediate body on the native x86 path");
  expect_contains(rendered, "mov ebx, 5",
                  "direct BIR call-crossing input should materialize the source value in the shared callee-saved register");
  expect_contains(rendered, "call add_one",
                  "direct BIR call-crossing input should lower the helper call without legacy backend IR");
  expect_contains(rendered, "add eax, ebx",
                  "direct BIR call-crossing input should add the preserved source value after the helper call");
  expect_not_contains(rendered, "target triple =",
                      "direct BIR call-crossing input should stay on native asm emission");
}

void test_backend_bir_pipeline_lowers_x86_direct_call_helper_families_to_shared_bir_views() {
  const auto two_arg_bir =
      c4c::backend::lower_to_bir(make_lir_minimal_two_arg_direct_call_module());
  const auto add_imm_bir =
      c4c::backend::lower_to_bir(make_lir_minimal_direct_call_add_imm_module());
  const auto identity_bir =
      c4c::backend::lower_to_bir(make_lir_minimal_direct_call_identity_arg_module());
  const auto folded_bir =
      c4c::backend::lower_to_bir(make_lir_minimal_folded_two_arg_direct_call_module());
  const auto dual_identity_bir =
      c4c::backend::lower_to_bir(make_lir_minimal_dual_identity_direct_call_sub_module());
  const auto call_crossing_bir =
      c4c::backend::lower_to_bir(make_lir_minimal_call_crossing_direct_call_module());

  const auto two_arg_slice = c4c::backend::parse_bir_minimal_two_arg_direct_call_module(two_arg_bir);
  expect_true(two_arg_slice.has_value(),
              "x86 two-argument direct-call LIR input should lower into the shared BIR two-argument parser view before target emission");

  const auto add_imm_slice =
      c4c::backend::parse_bir_minimal_direct_call_add_imm_module(add_imm_bir);
  expect_true(add_imm_slice.has_value(),
              "x86 add-immediate direct-call LIR input should lower into the shared BIR add-immediate parser view before target emission");

  const auto identity_slice =
      c4c::backend::parse_bir_minimal_direct_call_identity_arg_module(identity_bir);
  expect_true(identity_slice.has_value(),
              "x86 identity direct-call LIR input should lower into the shared BIR identity parser view before target emission");

  expect_true(folded_bir.functions.size() == 1 &&
                  folded_bir.functions.front().blocks.size() == 1 &&
                  folded_bir.functions.front().blocks.front().terminator.value.has_value() &&
                  folded_bir.functions.front().blocks.front().terminator.value->kind ==
                      c4c::backend::bir::Value::Kind::Immediate &&
                  folded_bir.functions.front().blocks.front().terminator.value->immediate == 8,
              "x86 folded two-argument direct-call LIR input should lower into the shared BIR return-immediate shape before target emission");

  const auto dual_identity_slice =
      c4c::backend::parse_bir_minimal_dual_identity_direct_call_sub_module(dual_identity_bir);
  expect_true(dual_identity_slice.has_value(),
              "x86 dual-identity direct-call subtraction LIR input should lower into the shared BIR subtraction parser view before target emission");

  const auto call_crossing_slice =
      c4c::backend::parse_bir_minimal_call_crossing_direct_call_module(call_crossing_bir);
  expect_true(call_crossing_slice.has_value(),
              "x86 call-crossing direct-call LIR input should lower into the shared BIR call-crossing parser view before target emission");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_two_arg_direct_call_through_bir_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_two_arg_direct_call_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".type add_pair, %function\nadd_pair:\n",
                  "x86 LIR two-argument direct-call input should still emit the helper definition after routing through the shared BIR path");
  expect_contains(rendered, "mov edi, 5",
                  "x86 LIR two-argument direct-call input should preserve the first immediate on the BIR-owned route");
  expect_contains(rendered, "mov esi, 7",
                  "x86 LIR two-argument direct-call input should preserve the second immediate on the BIR-owned route");
  expect_contains(rendered, "call add_pair",
                  "x86 LIR two-argument direct-call input should lower the helper call on the native x86 path");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR two-argument direct-call input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_direct_call_add_imm_through_bir_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_direct_call_add_imm_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".type add_one, %function\nadd_one:\n",
                  "x86 LIR add-immediate direct-call input should still emit the helper definition after routing through the shared BIR path");
  expect_contains(rendered, "add eax, 1",
                  "x86 LIR add-immediate direct-call input should preserve the helper add immediate on the BIR-owned route");
  expect_contains(rendered, "mov edi, 5",
                  "x86 LIR add-immediate direct-call input should preserve the caller argument immediate on the BIR-owned route");
  expect_contains(rendered, "call add_one",
                  "x86 LIR add-immediate direct-call input should lower the helper call on the native x86 path");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR add-immediate direct-call input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_direct_call_identity_arg_through_bir_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_direct_call_identity_arg_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".type f, %function\nf:\n",
                  "x86 LIR identity direct-call input should still emit the helper definition after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, edi",
                  "x86 LIR identity direct-call input should preserve the helper identity return on the BIR-owned route");
  expect_contains(rendered, "mov edi, 0",
                  "x86 LIR identity direct-call input should preserve the caller immediate on the BIR-owned route");
  expect_contains(rendered, "call f",
                  "x86 LIR identity direct-call input should lower the helper call on the native x86 path");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR identity direct-call input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_folded_two_arg_direct_call_through_bir_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_folded_two_arg_direct_call_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "x86 LIR folded two-argument direct-call input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, 8",
                  "x86 LIR folded two-argument direct-call input should preserve the folded immediate result on the BIR-owned route");
  expect_not_contains(rendered, "fold_pair",
                      "x86 LIR folded two-argument direct-call input should fully fold away the helper/main direct-call family on the BIR-owned route");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR folded two-argument direct-call input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_repeated_zero_arg_call_compare_zero_return_through_bir_end_to_end() {
  const auto lowered_bir = c4c::backend::try_lower_to_bir(
      make_lir_minimal_repeated_zero_arg_call_compare_zero_return_module());
  expect_true(lowered_bir.has_value(),
              "x86 LIR repeated zero-arg call compare input should now lower into the bounded shared helper-plus-immediate-return shape");
  if (!lowered_bir.has_value()) {
    return;
  }

  expect_true(lowered_bir->functions.size() == 2 &&
                  lowered_bir->functions[0].name == "f" &&
                  lowered_bir->functions[1].name == "main" &&
                  lowered_bir->functions[0].blocks.front().terminator.value ==
                      c4c::backend::bir::Value::immediate_i32(100) &&
                  lowered_bir->functions[1].blocks.front().terminator.value ==
                      c4c::backend::bir::Value::immediate_i32(0),
              "x86 LIR repeated zero-arg call compare lowering should preserve the helper while folding main to the shared zero return");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_lir_minimal_repeated_zero_arg_call_compare_zero_return_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".type f, @function\nf:\n",
                  "x86 LIR repeated zero-arg call compare input should still emit the helper definition after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, 100",
                  "x86 LIR repeated zero-arg call compare input should preserve the helper immediate return on the native x86 path");
  expect_contains(rendered, ".globl main",
                  "x86 LIR repeated zero-arg call compare input should still emit the caller definition on the native x86 path");
  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR repeated zero-arg call compare input should materialize the folded zero return in main on the native x86 path");
  expect_not_contains(rendered, "call f",
                      "x86 LIR repeated zero-arg call compare input should stop depending on the unsupported repeated direct-LIR call/branch chain once the shared fold owns the seam");
  expect_not_contains(rendered, "cmp eax, 1000",
                      "x86 LIR repeated zero-arg call compare input should stop emitting the repeated compare chain once the shared fold owns the seam");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR repeated zero-arg call compare input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_local_i32_inc_dec_compare_return_zero_through_bir_end_to_end() {
  const auto lowered_bir = c4c::backend::try_lower_to_bir(
      make_lir_minimal_local_i32_inc_dec_compare_return_zero_module());
  expect_true(lowered_bir.has_value(),
              "x86 LIR local-slot inc/dec compare input should now lower into the bounded shared helper-plus-immediate-return shape");
  if (!lowered_bir.has_value()) {
    return;
  }

  expect_true(lowered_bir->functions.size() == 3 &&
                  lowered_bir->functions[0].name == "zero" &&
                  lowered_bir->functions[1].name == "one" &&
                  lowered_bir->functions[2].name == "main" &&
                  lowered_bir->functions[0].blocks.front().terminator.value ==
                      c4c::backend::bir::Value::immediate_i32(0) &&
                  lowered_bir->functions[1].blocks.front().terminator.value ==
                      c4c::backend::bir::Value::immediate_i32(1) &&
                  lowered_bir->functions[2].blocks.front().terminator.value ==
                      c4c::backend::bir::Value::immediate_i32(0),
              "x86 LIR local-slot inc/dec compare lowering should preserve both helpers while folding main to the shared zero return");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_lir_minimal_local_i32_inc_dec_compare_return_zero_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".type zero, @function\nzero:\n",
                  "x86 LIR local-slot inc/dec compare input should still emit the zero helper definition after routing through the shared BIR path");
  expect_contains(rendered, ".type one, @function\none:\n",
                  "x86 LIR local-slot inc/dec compare input should still emit the one helper definition after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR local-slot inc/dec compare input should materialize the folded zero return on the native x86 path");
  expect_contains(rendered, "mov eax, 1",
                  "x86 LIR local-slot inc/dec compare input should preserve the one helper immediate return on the native x86 path");
  expect_not_contains(rendered, "cmp eax, 1",
                      "x86 LIR local-slot inc/dec compare input should stop emitting the repeated compare chain once the shared fold owns the seam");
  expect_not_contains(rendered, "call zero",
                      "x86 LIR local-slot inc/dec compare input should stop depending on the unsupported direct-LIR helper-call chain once the shared fold owns the seam");
  expect_not_contains(rendered, "call one",
                      "x86 LIR local-slot inc/dec compare input should stop depending on the unsupported direct-LIR helper-call chain once the shared fold owns the seam");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR local-slot inc/dec compare input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_countdown_loop_through_bir_end_to_end() {
  const auto lowered_bir = c4c::backend::try_lower_to_bir(make_lir_minimal_countdown_loop_module());
  expect_true(lowered_bir.has_value(),
              "x86 LIR countdown-loop input should now lower into the bounded shared BIR local-slot loop shape");
  expect_true(lowered_bir->functions.size() == 1 &&
                  lowered_bir->functions.front().local_slots.size() == 1 &&
                  lowered_bir->functions.front().blocks.size() == 4 &&
                  lowered_bir->functions.front().blocks[0].terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Branch &&
                  lowered_bir->functions.front().blocks[1].terminator.kind ==
                      c4c::backend::bir::TerminatorKind::CondBranch,
              "x86 LIR countdown-loop lowering should produce the expected multi-block shared BIR control-flow skeleton");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_countdown_loop_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, "mov eax, 5",
                  "x86 LIR countdown-loop input should preserve the initial counter after routing through the shared BIR path");
  expect_contains(rendered, ".Lloop:\n  cmp eax, 0\n  je .Lexit\n",
                  "x86 LIR countdown-loop input should preserve the loop test after the x86 LIR-only seam is removed");
  expect_contains(rendered, ".Lbody:\n  sub eax, 1\n  jmp .Lloop\n",
                  "x86 LIR countdown-loop input should preserve the decrement-and-backedge sequence after routing through shared BIR");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR countdown-loop input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_scalar_global_load_through_bir_end_to_end() {
  const auto lowered_bir =
      c4c::backend::try_lower_to_bir(make_lir_minimal_scalar_global_load_module());
  expect_true(lowered_bir.has_value(),
              "x86 LIR scalar global-load input should now lower into the bounded shared BIR global-load shape");
  expect_true(lowered_bir->globals.size() == 1 && lowered_bir->functions.size() == 1 &&
                  lowered_bir->functions.front().blocks.size() == 1 &&
                  lowered_bir->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::LoadGlobalInst>(
                      lowered_bir->functions.front().blocks.front().insts.front()),
              "x86 LIR scalar global-load lowering should produce one initialized global plus one bir.load_global instruction");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_scalar_global_load_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl g_counter",
                  "x86 LIR scalar global-load input should still emit the global definition after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, dword ptr [rax]",
                  "x86 LIR scalar global-load input should lower bir.load_global on the native x86 path");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR scalar global-load input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_string_literal_compare_phi_return_through_bir_end_to_end() {
  const auto lowered_bir =
      c4c::backend::try_lower_to_bir(make_lir_minimal_string_literal_compare_phi_return_module());
  expect_true(lowered_bir.has_value(),
              "x86 LIR string-literal compare phi-return input should now lower into the bounded shared BIR immediate-return shape");
  if (!lowered_bir.has_value()) {
    return;
  }

  expect_true(lowered_bir->functions.size() == 1 &&
                  lowered_bir->functions.front().blocks.size() == 1 &&
                  lowered_bir->functions.front().blocks.front().insts.empty() &&
                  lowered_bir->functions.front().blocks.front().terminator.value.has_value() &&
                  lowered_bir->functions.front().blocks.front().terminator.value->kind ==
                      c4c::backend::bir::Value::Kind::Immediate &&
                  lowered_bir->functions.front().blocks.front().terminator.value->immediate == 0,
              "x86 LIR string-literal compare phi-return lowering should produce the bounded shared immediate-return contract");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_string_literal_compare_phi_return_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR string-literal compare phi-return input should materialize the folded shared immediate return on the native x86 path");
  expect_not_contains(rendered, ".section .rodata",
                      "x86 LIR string-literal compare phi-return input should stop relying on the direct-LIR string-literal helper once the shared BIR fold owns the seam");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR string-literal compare phi-return input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_local_buffer_string_copy_printf_on_native_path() {
  const auto module = make_lir_minimal_local_buffer_string_copy_printf_module();
  expect_true(!c4c::backend::try_lower_to_bir(module).has_value(),
              "raw x86 LIR local-buffer copy/printf input should remain a bounded native direct-LIR seam instead of silently widening shared BIR ownership");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".section .rodata",
                  "x86 LIR local-buffer copy/printf input should still materialize its string constants on the native x86 path");
  expect_contains(rendered, ".asciz \"abcdef\"",
                  "x86 LIR local-buffer copy/printf input should preserve the strcpy source bytes on the native x86 path");
  expect_contains(rendered, ".asciz \"%s\\n\"",
                  "x86 LIR local-buffer copy/printf input should preserve the printf format bytes on the native x86 path");
  expect_contains(rendered, "call strcpy",
                  "x86 LIR local-buffer copy/printf input should lower the bounded stack-buffer copy through native x86 emission");
  expect_contains(rendered, "call printf",
                  "x86 LIR local-buffer copy/printf input should lower the bounded pointer-offset printf call through native x86 emission");
  expect_contains(rendered, "lea rsi, [rsp + 9]",
                  "x86 LIR local-buffer copy/printf input should preserve the one-byte stack-buffer offset passed to printf");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR local-buffer copy/printf input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_counted_printf_ternary_loop_on_native_path() {
  const auto module = make_lir_minimal_counted_printf_ternary_loop_module();
  expect_true(!c4c::backend::try_lower_to_bir(module).has_value(),
              "raw x86 LIR counted ternary printf loop input should remain a bounded native direct-LIR seam instead of silently widening shared BIR ownership");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".asciz \"%d\\n\"",
                  "x86 LIR counted ternary printf loop input should preserve the printf format bytes on the native x86 path");
  expect_contains(rendered, "cmp eax, 10",
                  "x86 LIR counted ternary printf loop input should keep the bounded loop exit compare in native x86 assembly");
  expect_contains(rendered, "imul esi, eax",
                  "x86 LIR counted ternary printf loop input should lower the square arm on the native x86 path");
  expect_contains(rendered, "lea esi, [rax + rax*2]",
                  "x86 LIR counted ternary printf loop input should lower the times-three arm on the native x86 path");
  expect_contains(rendered, "call printf",
                  "x86 LIR counted ternary printf loop input should lower the bounded printf call through native x86 emission");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR counted ternary printf loop input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_source_like_local_buffer_string_copy_printf_on_native_path() {
  const auto module = make_lir_source_like_local_buffer_string_copy_printf_module();
  expect_true(!c4c::backend::try_lower_to_bir(module).has_value(),
              "source-like x86 LIR local-buffer copy/printf input should stay outside shared BIR so the bounded native seam owns the noisy declaration surface");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".asciz \"abcdef\"",
                  "source-like x86 LIR local-buffer copy/printf input should preserve the strcpy source bytes on the native x86 path");
  expect_contains(rendered, "call strcpy",
                  "source-like x86 LIR local-buffer copy/printf input should still lower the bounded stack-buffer copy through native x86 emission even with unrelated libc declaration noise");
  expect_contains(rendered, "call printf",
                  "source-like x86 LIR local-buffer copy/printf input should still lower the bounded pointer-offset printf call through native x86 emission even with unrelated libc declaration noise");
  expect_not_contains(rendered, "target triple =",
                      "source-like x86 LIR local-buffer copy/printf input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_source_like_repeated_printf_immediates_on_native_path() {
  const auto module = make_lir_source_like_repeated_printf_immediates_module();
  expect_true(!c4c::backend::try_lower_to_bir(module).has_value(),
              "source-like x86 LIR repeated printf-immediates input should stay outside shared BIR so the bounded native seam owns the noisy declaration surface");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".asciz \"%d %d\\n\"",
                  "source-like x86 LIR repeated printf-immediates input should preserve the shared format bytes on the native x86 path");
  expect_contains(rendered, "mov rsi, 1",
                  "source-like x86 LIR repeated printf-immediates input should lower the first bounded sizeof call arguments through the native integer register path");
  expect_contains(rendered, "mov rsi, 2",
                  "source-like x86 LIR repeated printf-immediates input should lower the second bounded sizeof call arguments through the native integer register path");
  expect_contains(rendered, "call printf",
                  "source-like x86 LIR repeated printf-immediates input should still lower both bounded printf calls through native x86 emission even with unrelated libc declaration noise");
  expect_not_contains(rendered, "target triple =",
                      "source-like x86 LIR repeated printf-immediates input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_extern_scalar_global_load_through_bir_end_to_end() {
  const auto lowered_bir =
      c4c::backend::try_lower_to_bir(make_lir_minimal_extern_scalar_global_load_module());
  expect_true(lowered_bir.has_value(),
              "x86 LIR extern scalar global-load input should now lower into the bounded shared BIR global-load shape");
  expect_true(lowered_bir->globals.size() == 1 && lowered_bir->functions.size() == 1 &&
                  lowered_bir->globals.front().is_extern &&
                  !lowered_bir->globals.front().initializer.has_value() &&
                  lowered_bir->functions.front().blocks.size() == 1 &&
                  lowered_bir->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::LoadGlobalInst>(
                      lowered_bir->functions.front().blocks.front().insts.front()),
              "x86 LIR extern scalar global-load lowering should produce one extern global declaration plus one bir.load_global instruction");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_extern_scalar_global_load_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_not_contains(rendered, ".globl g_counter",
                      "x86 LIR extern scalar global-load input should keep the global unresolved after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, dword ptr [rax]",
                  "x86 LIR extern scalar global-load input should lower bir.load_global on the native x86 path");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR extern scalar global-load input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_extern_global_array_load_through_bir_end_to_end() {
  const auto lowered_bir =
      c4c::backend::try_lower_to_bir(make_lir_minimal_extern_global_array_load_module());
  expect_true(lowered_bir.has_value(),
              "x86 LIR extern global-array-load input should now lower into the bounded shared BIR global-load shape");
  expect_true(lowered_bir->globals.size() == 1 && lowered_bir->functions.size() == 1 &&
                  lowered_bir->globals.front().is_extern &&
                  !lowered_bir->globals.front().initializer.has_value() &&
                  lowered_bir->functions.front().blocks.size() == 1 &&
                  lowered_bir->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::LoadGlobalInst>(
                      lowered_bir->functions.front().blocks.front().insts.front()) &&
                  std::get<c4c::backend::bir::LoadGlobalInst>(
                      lowered_bir->functions.front().blocks.front().insts.front()).byte_offset == 4,
              "x86 LIR extern global-array-load lowering should produce one extern global declaration plus one byte-offset bir.load_global instruction");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_extern_global_array_load_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_not_contains(rendered, ".globl ext_arr",
                      "x86 LIR extern global-array-load input should keep the global unresolved after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, dword ptr [rax + 4]",
                  "x86 LIR extern global-array-load input should lower the byte-offset bir.load_global on the native x86 path");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR extern global-array-load input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_global_char_pointer_diff_through_bir_end_to_end() {
  const auto lowered_bir =
      c4c::backend::try_lower_to_bir(make_lir_minimal_global_char_pointer_diff_module());
  expect_true(lowered_bir.has_value(),
              "x86 LIR global char pointer-diff input should now lower into the bounded shared BIR immediate-return shape");
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
              "x86 LIR global char pointer-diff lowering should produce the bounded shared immediate-return contract");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_global_char_pointer_diff_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, "mov eax, 1",
                  "x86 LIR global char pointer-diff input should materialize the shared immediate-return result on the native x86 path");
  expect_not_contains(rendered, "sub rcx, rax",
                      "x86 LIR global char pointer-diff input should no longer rely on the emitter-local pointer-diff fast path");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR global char pointer-diff input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_global_int_pointer_diff_through_bir_end_to_end() {
  const auto lowered_bir =
      c4c::backend::try_lower_to_bir(make_lir_minimal_global_int_pointer_diff_module());
  expect_true(lowered_bir.has_value(),
              "x86 LIR global int pointer-diff input should now lower into the bounded shared BIR immediate-return shape");
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
              "x86 LIR global int pointer-diff lowering should produce the bounded shared immediate-return contract");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_global_int_pointer_diff_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, "mov eax, 1",
                  "x86 LIR global int pointer-diff input should materialize the shared immediate-return result on the native x86 path");
  expect_not_contains(rendered, "sub rcx, rax",
                      "x86 LIR global int pointer-diff input should no longer rely on the emitter-local pointer-diff fast path");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR global int pointer-diff input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_scalar_global_store_reload_through_bir_end_to_end() {
  const auto lowered_bir =
      c4c::backend::try_lower_to_bir(make_lir_minimal_scalar_global_store_reload_module());
  expect_true(lowered_bir.has_value(),
              "x86 LIR scalar global store-reload input should now lower into the bounded shared BIR global store-reload shape");
  expect_true(lowered_bir->globals.size() == 1 && lowered_bir->functions.size() == 1 &&
                  lowered_bir->functions.front().blocks.size() == 1 &&
                  lowered_bir->functions.front().blocks.front().insts.size() == 2 &&
                  std::holds_alternative<c4c::backend::bir::StoreGlobalInst>(
                      lowered_bir->functions.front().blocks.front().insts.front()) &&
                  std::holds_alternative<c4c::backend::bir::LoadGlobalInst>(
                      lowered_bir->functions.front().blocks.front().insts[1]),
              "x86 LIR scalar global store-reload lowering should produce one initialized global plus direct bir.store_global and bir.load_global instructions");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_scalar_global_store_reload_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl g_counter",
                  "x86 LIR scalar global store-reload input should still emit the global definition after routing through the shared BIR path");
  expect_contains(rendered, "mov dword ptr [rax], 7",
                  "x86 LIR scalar global store-reload input should lower bir.store_global on the native x86 path");
  expect_contains(rendered, "mov eax, dword ptr [rax]",
                  "x86 LIR scalar global store-reload input should reload the stored scalar value on the native x86 path");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR scalar global store-reload input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_zero_initialized_scalar_global_store_reload_through_bir_end_to_end() {
  auto module = make_lir_minimal_scalar_global_store_reload_module();
  module.globals.front().init_text = "zeroinitializer";
  std::get<c4c::codegen::lir::LirStoreOp>(module.functions.front().blocks.front().insts.front()).val = "0";

  const auto lowered_bir = c4c::backend::try_lower_to_bir(module);
  expect_true(lowered_bir.has_value(),
              "x86 LIR zero-initialized scalar global store-reload input should now lower into the bounded shared BIR global store-reload shape");
  if (!lowered_bir.has_value()) {
    return;
  }

  expect_true(lowered_bir->globals.size() == 1 && lowered_bir->globals.front().initializer.has_value() &&
                  *lowered_bir->globals.front().initializer == c4c::backend::bir::Value::immediate_i32(0) &&
                  lowered_bir->functions.size() == 1 &&
                  lowered_bir->functions.front().blocks.size() == 1 &&
                  lowered_bir->functions.front().blocks.front().insts.size() == 2,
              "x86 LIR zero-initialized scalar global store-reload lowering should preserve the zero-initialized global plus the bounded store/load instructions");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl g_counter",
                  "x86 LIR zero-initialized scalar global store-reload input should still emit the global definition after routing through the shared BIR path");
  expect_contains(rendered, "mov dword ptr [rax], 0",
                  "x86 LIR zero-initialized scalar global store-reload input should lower bir.store_global on the native x86 path");
  expect_contains(rendered, "mov eax, dword ptr [rax]",
                  "x86 LIR zero-initialized scalar global store-reload input should reload the stored scalar value on the native x86 path");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR zero-initialized scalar global store-reload input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_global_two_field_struct_store_sub_sub_through_bir_end_to_end() {
  const auto lowered_bir =
      c4c::backend::try_lower_to_bir(make_lir_minimal_global_two_field_struct_store_sub_sub_module());
  expect_true(lowered_bir.has_value(),
              "x86 LIR global two-field struct store/sub/sub input should now lower into the bounded shared BIR global-field route");
  if (!lowered_bir.has_value()) {
    return;
  }

  expect_true(lowered_bir->globals.size() == 1 && lowered_bir->globals.front().name == "v" &&
                  lowered_bir->globals.front().size_bytes == 8 &&
                  lowered_bir->functions.size() == 1 &&
                  lowered_bir->functions.front().blocks.size() == 1 &&
                  lowered_bir->functions.front().blocks.front().insts.size() == 6,
              "x86 LIR global two-field struct store/sub/sub lowering should preserve the bounded shared BIR field-store/load arithmetic sequence");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_global_two_field_struct_store_sub_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl v",
                  "x86 LIR global two-field struct store/sub/sub input should still emit the global definition after routing through the shared BIR path");
  expect_contains(rendered, "mov dword ptr [rax], 1",
                  "x86 LIR global two-field struct store/sub/sub input should materialize the first field store on the native x86 path");
  expect_contains(rendered, "mov dword ptr [rax + 4], 2",
                  "x86 LIR global two-field struct store/sub/sub input should materialize the second field store on the native x86 path");
  expect_contains(rendered, "sub ecx, dword ptr [rax + 4]",
                  "x86 LIR global two-field struct store/sub/sub input should reload the second field through its global byte offset on the native x86 path");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR global two-field struct store/sub/sub input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_countdown_do_while_through_bir_end_to_end() {
  const auto lowered_bir = c4c::backend::try_lower_to_bir(make_lir_minimal_countdown_do_while_module());
  expect_true(lowered_bir.has_value(),
              "x86 LIR countdown do-while input should now lower into the bounded shared BIR local-slot loop shape");
  expect_true(lowered_bir->functions.size() == 1 &&
                  lowered_bir->functions.front().local_slots.size() == 1 &&
                  lowered_bir->functions.front().blocks.size() == 4 &&
                  lowered_bir->functions.front().blocks[1].label == "cond" &&
                  lowered_bir->functions.front().blocks[1].terminator.kind ==
                      c4c::backend::bir::TerminatorKind::CondBranch,
              "x86 LIR countdown do-while lowering should normalize the empty bridge block into the shared conditional loop header");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_countdown_do_while_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, "mov eax, 50",
                  "x86 LIR countdown do-while input should preserve the initial counter after routing through the shared BIR path");
  expect_contains(rendered, ".Lcond:\n  cmp eax, 0\n  je .Lexit\n",
                  "x86 LIR countdown do-while input should now materialize the normalized shared loop header on the native backend path");
  expect_contains(rendered, ".Lbody:\n  sub eax, 1\n  jmp .Lcond\n",
                  "x86 LIR countdown do-while input should preserve the decrement-and-backedge sequence after the x86-only LIR seam is removed");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR countdown do-while input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_double_countdown_guarded_zero_return_through_bir_end_to_end() {
  const auto lowered_bir =
      c4c::backend::try_lower_to_bir(make_double_countdown_guarded_zero_return_module());
  expect_true(lowered_bir.has_value(),
              "x86 LIR double-countdown guarded-return input should now lower into the bounded shared constant-return shape");
  expect_true(lowered_bir->functions.size() == 1 &&
                  lowered_bir->functions.front().blocks.size() == 1 &&
                  lowered_bir->functions.front().blocks.front().label == "entry" &&
                  lowered_bir->functions.front().blocks.front().insts.empty() &&
                  lowered_bir->functions.front().blocks.front().terminator.kind ==
                      c4c::backend::bir::TerminatorKind::Return,
              "x86 LIR double-countdown guarded-return lowering should collapse the redundant loop-and-guard CFG to one constant-return block");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_double_countdown_guarded_zero_return_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR double-countdown guarded-return input should materialize the shared zero result on the native backend path");
  expect_not_contains(rendered, "cmp eax, 0",
                      "x86 LIR double-countdown guarded-return input should no longer carry the redundant countdown loop tests after shared lowering");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR double-countdown guarded-return input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_dual_identity_direct_call_sub_through_bir_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_dual_identity_direct_call_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".type f, %function\nf:\n",
                  "x86 LIR dual-identity subtraction input should still emit the left helper definition after routing through the shared BIR path");
  expect_contains(rendered, ".type g, %function\ng:\n",
                  "x86 LIR dual-identity subtraction input should still emit the right helper definition after routing through the shared BIR path");
  expect_contains(rendered, "mov edi, 7",
                  "x86 LIR dual-identity subtraction input should preserve the left caller immediate on the BIR-owned route");
  expect_contains(rendered, "mov edi, 3",
                  "x86 LIR dual-identity subtraction input should preserve the right caller immediate on the BIR-owned route");
  expect_contains(rendered, "sub ebx, eax",
                  "x86 LIR dual-identity subtraction input should lower the subtraction on the native x86 path");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR dual-identity subtraction input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_call_crossing_direct_call_through_bir_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_call_crossing_direct_call_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".type add_one, %function\nadd_one:\n",
                  "x86 LIR call-crossing input should still emit the helper definition after routing through the shared BIR path");
  expect_contains(rendered, "mov ebx, 5",
                  "x86 LIR call-crossing input should preserve the source value in the shared callee-saved register assignment");
  expect_contains(rendered, "call add_one",
                  "x86 LIR call-crossing input should stay on the shared BIR-owned direct-call emitter path");
  expect_contains(rendered, "add eax, ebx",
                  "x86 LIR call-crossing input should preserve the final add against the source value after BIR lowering");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR call-crossing input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_minimal_conditional_return_through_bir_end_to_end() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_lir_minimal_conditional_return_module());
  expect_true(lowered.has_value(),
              "x86 LIR conditional-return input should lower into a direct BIR module before target emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::SelectInst>(
                      lowered->functions.front().blocks.front().insts.front()),
              "x86 LIR conditional-return input should collapse to the shared BIR select form before target emission");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_minimal_conditional_return_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "x86 LIR conditional-return input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "jge .Lselect_false",
                  "x86 LIR conditional-return input should lower the fused BIR predicate to the native branch form on the x86 path");
  expect_contains(rendered, ".Lselect_true:\n  mov eax, 0\n  ret\n",
                  "x86 LIR conditional-return input should preserve the then-arm immediate through the BIR-owned select emission");
  expect_contains(rendered, ".Lselect_false:\n  mov eax, 1\n  ret\n",
                  "x86 LIR conditional-return input should preserve the else-arm immediate through the BIR-owned select emission");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR conditional-return input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_conditional_phi_join_through_bir_end_to_end() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_lir_single_param_select_eq_phi_module());
  expect_true(lowered.has_value(),
              "x86 LIR conditional-phi-join input should lower into a direct BIR module before target emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.size() == 1 &&
                  std::holds_alternative<c4c::backend::bir::SelectInst>(
                      lowered->functions.front().blocks.front().insts.front()),
              "x86 LIR conditional-phi-join input should collapse to the shared BIR select form before target emission");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_single_param_select_eq_phi_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose",
                  "x86 LIR conditional-phi-join input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "cmp eax, 7",
                  "x86 LIR conditional-phi-join input should lower the fused BIR predicate to the native branch form on the x86 path");
  expect_contains(rendered, ".Lselect_true:\n  mov eax, 11\n  ret\n",
                  "x86 lowerable conditional-phi-join input should use the shared BIR-owned select labels instead of target-local phi predecessor blocks");
  expect_contains(rendered, ".Lselect_false:\n  mov eax, 4\n  ret\n",
                  "x86 lowerable conditional-phi-join input should preserve the bounded false arm through the BIR-owned select emission");
  expect_not_contains(rendered, ".choose.tern.then",
                      "x86 lowerable conditional-phi-join input should no longer emit the original target-local phi predecessor block labels once the BIR route owns the shape");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR conditional-phi-join input should stay on native asm emission instead of falling back to LLVM text");

  const auto direct_rendered =
      c4c::backend::x86::emit_module(make_lir_single_param_select_eq_phi_module());

  expect_contains(direct_rendered, ".globl choose",
                  "x86 direct emitter should also route lowerable conditional-phi-join input through the shared BIR path before native asm emission");
  expect_contains(direct_rendered, ".Lselect_true:\n  mov eax, 11\n  ret\n",
                  "x86 direct emitter should lower the conditional-phi-join route to the shared BIR-owned select labels");
  expect_contains(direct_rendered, ".Lselect_false:\n  mov eax, 4\n  ret\n",
                  "x86 direct emitter should preserve the bounded false arm after routing the conditional-phi-join route through BIR");
  expect_not_contains(direct_rendered, ".choose.tern.then",
                      "x86 direct emitter should no longer emit the original target-local phi predecessor block labels for a lowerable conditional-phi-join route");
}

void test_backend_bir_pipeline_drives_x86_lir_signed_i16_local_slot_increment_compare_through_bir_end_to_end() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_lir_minimal_signed_i16_local_slot_increment_compare_module());
  expect_true(lowered.has_value(),
              "x86 LIR signed i16 local-slot increment-and-compare input should lower into direct BIR before native x86 emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty(),
              "x86 LIR signed i16 local-slot increment-and-compare lowering should collapse the bounded no-param local-slot slice to one constant-return BIR block");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_lir_minimal_signed_i16_local_slot_increment_compare_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "x86 LIR signed i16 local-slot increment-and-compare input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR signed i16 local-slot increment-and-compare input should preserve the constant false-arm return after bounded shared lowering");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR signed i16 local-slot increment-and-compare input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_local_i32_store_load_sub_return_through_bir_end_to_end() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_local_i32_store_load_sub_return_immediate_module());
  expect_true(lowered.has_value(),
              "x86 LIR local i32 store-load-sub-return input should lower into direct BIR before native x86 emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty(),
              "x86 LIR local i32 store-load-sub-return lowering should collapse the bounded local-slot slice to one constant-return BIR block");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_local_i32_store_load_sub_return_immediate_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "x86 LIR local i32 store-load-sub-return input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR local i32 store-load-sub-return input should preserve the folded zero return after bounded shared lowering");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR local i32 store-load-sub-return input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_local_i32_pointer_store_zero_load_return_through_bir_end_to_end() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_local_i32_pointer_store_zero_load_return_module());
  expect_true(lowered.has_value(),
              "x86 LIR local-pointer store-zero-load-return input should lower into direct BIR before native x86 emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty(),
              "x86 LIR local-pointer store-zero-load-return lowering should collapse the bounded indirect local-slot slice to one constant-return BIR block");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_local_i32_pointer_store_zero_load_return_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "x86 LIR local-pointer store-zero-load-return input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR local-pointer store-zero-load-return input should preserve the folded zero return after bounded shared lowering");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR local-pointer store-zero-load-return input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_local_i32_pointer_gep_zero_load_return_through_bir_end_to_end() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_local_i32_pointer_gep_zero_load_return_module());
  expect_true(lowered.has_value(),
              "x86 LIR local-pointer gep-zero-load-return input should lower into direct BIR before native x86 emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty(),
              "x86 LIR local-pointer gep-zero-load-return lowering should collapse the bounded zero-offset indirect local-slot slice to one constant-return BIR block");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_local_i32_pointer_gep_zero_load_return_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "x86 LIR local-pointer gep-zero-load-return input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR local-pointer gep-zero-load-return input should preserve the folded zero return after bounded shared lowering");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR local-pointer gep-zero-load-return input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_local_i32_pointer_gep_zero_store_slot_load_return_through_bir_end_to_end() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_local_i32_pointer_gep_zero_store_slot_load_return_module());
  expect_true(lowered.has_value(),
              "x86 LIR local-pointer gep-zero-store-slot-load-return input should lower into direct BIR before native x86 emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty(),
              "x86 LIR local-pointer gep-zero-store-slot-load-return lowering should collapse the bounded zero-offset alias overwrite slice to one constant-return BIR block");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_local_i32_pointer_gep_zero_store_slot_load_return_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "x86 LIR local-pointer gep-zero-store-slot-load-return input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR local-pointer gep-zero-store-slot-load-return input should preserve the folded zero return after bounded shared lowering");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR local-pointer gep-zero-store-slot-load-return input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_local_i32_array_two_slot_sum_sub_three_through_bir_end_to_end() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_local_i32_array_two_slot_sum_sub_three_module());
  expect_true(lowered.has_value(),
              "x86 LIR two-slot local-array sum-sub-three input should lower into direct BIR before native x86 emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty(),
              "x86 LIR two-slot local-array sum-sub-three lowering should collapse the bounded constant indexed array slice to one constant-return BIR block");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_local_i32_array_two_slot_sum_sub_three_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "x86 LIR two-slot local-array sum-sub-three input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR two-slot local-array sum-sub-three input should preserve the folded zero return after bounded shared lowering");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR two-slot local-array sum-sub-three input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_local_i32_array_second_slot_pointer_store_zero_load_return_through_bir_end_to_end() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(
          make_local_i32_array_second_slot_pointer_store_zero_load_return_module());
  expect_true(lowered.has_value(),
              "x86 LIR local-array second-slot pointer-store-zero-load-return input should lower into direct BIR before native x86 emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty(),
              "x86 LIR local-array second-slot pointer-store-zero-load-return lowering should collapse the bounded aliased second-slot slice to one constant-return BIR block");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_local_i32_array_second_slot_pointer_store_zero_load_return_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "x86 LIR local-array second-slot pointer-store-zero-load-return input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR local-array second-slot pointer-store-zero-load-return input should preserve the folded zero return after bounded shared lowering");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR local-array second-slot pointer-store-zero-load-return input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_local_i32_array_pointer_inc_dec_compare_zero_through_bir_end_to_end() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(
          make_local_i32_array_pointer_inc_dec_compare_zero_return_module());
  expect_true(lowered.has_value(),
              "x86 LIR local-array pointer increment-decrement compare input should lower into direct BIR before native x86 emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty(),
              "x86 LIR local-array pointer increment-decrement compare lowering should collapse the bounded pointer walk slice to one constant-return BIR block");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_local_i32_array_pointer_inc_dec_compare_zero_return_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "x86 LIR local-array pointer increment-decrement compare input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR local-array pointer increment-decrement compare input should preserve the folded zero return after bounded shared lowering");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR local-array pointer increment-decrement compare input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_local_two_field_struct_sub_sub_two_through_bir_end_to_end() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_local_two_field_struct_sub_sub_two_return_module());
  expect_true(lowered.has_value(),
              "x86 LIR two-field local-struct subtract-subtract-two input should lower into direct BIR before native x86 emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty(),
              "x86 LIR two-field local-struct subtract-subtract-two lowering should collapse the bounded field-gep slice to one constant-return BIR block");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_local_two_field_struct_sub_sub_two_return_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "x86 LIR two-field local-struct subtract-subtract-two input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR two-field local-struct subtract-subtract-two input should preserve the folded zero return after bounded shared lowering");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR two-field local-struct subtract-subtract-two input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_local_struct_pointer_alias_add_sub_three_through_bir_end_to_end() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_local_struct_pointer_alias_add_sub_three_return_module());
  expect_true(lowered.has_value(),
              "x86 LIR local struct-pointer alias add-sub-three input should lower into direct BIR before native x86 emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty(),
              "x86 LIR local struct-pointer alias add-sub-three lowering should collapse the bounded pointer-alias field slice to one constant-return BIR block");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_local_struct_pointer_alias_add_sub_three_return_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "x86 LIR local struct-pointer alias add-sub-three input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR local struct-pointer alias add-sub-three input should preserve the folded zero return after bounded shared lowering");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR local struct-pointer alias add-sub-three input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_local_self_referential_struct_pointer_chain_through_bir_end_to_end() {
  const auto lowered = c4c::backend::try_lower_to_bir(
      make_local_self_referential_struct_pointer_chain_zero_return_module());
  expect_true(lowered.has_value(),
              "x86 LIR local self-referential struct-pointer chain input should lower into direct BIR before native x86 emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty(),
              "x86 LIR local self-referential struct-pointer chain lowering should collapse the bounded repeated self-load slice to one constant-return BIR block");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_local_self_referential_struct_pointer_chain_zero_return_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "x86 LIR local self-referential struct-pointer chain input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR local self-referential struct-pointer chain input should preserve the folded zero return after bounded shared lowering");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR local self-referential struct-pointer chain input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_double_indirect_local_pointer_chain_through_bir_end_to_end() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_double_indirect_local_pointer_chain_zero_return_module());
  expect_true(lowered.has_value(),
              "x86 LIR double-indirect local pointer-chain input should lower into direct BIR before native x86 emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty(),
              "x86 LIR double-indirect local pointer-chain lowering should collapse the bounded alias-load slice to one constant-return BIR block");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_double_indirect_local_pointer_chain_zero_return_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "x86 LIR double-indirect local pointer-chain input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR double-indirect local pointer-chain input should preserve the folded zero return after bounded shared lowering");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR double-indirect local pointer-chain input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_double_indirect_local_store_one_final_branch_through_bir_end_to_end() {
  const auto lowered =
      c4c::backend::try_lower_to_bir(make_double_indirect_local_store_one_final_branch_return_module());
  expect_true(lowered.has_value(),
              "x86 LIR double-indirect local store-one input should lower into direct BIR before native x86 emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1 &&
                  lowered->functions.front().blocks.front().insts.empty(),
              "x86 LIR double-indirect local store-one lowering should collapse the bounded alias-and-branch slice to one constant-return BIR block");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_double_indirect_local_store_one_final_branch_return_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "x86 LIR double-indirect local store-one input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, "mov eax, 0",
                  "x86 LIR double-indirect local store-one input should preserve the folded zero return after bounded shared lowering");
  expect_not_contains(rendered, "cmp eax, 0",
                      "x86 LIR double-indirect local store-one input should no longer carry the dead early-return compares after shared lowering");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR double-indirect local store-one input should stay on native asm emission instead of falling back to LLVM text");
}

void test_backend_bir_pipeline_drives_x86_lir_u8_select_post_join_add_through_bir_end_to_end() {
  const auto lowered = c4c::backend::try_lower_to_bir(make_lir_u8_select_post_join_add_module());
  expect_true(lowered.has_value(),
              "x86 LIR u8 select-plus-tail input should lower into a direct BIR module before target emission");
  expect_true(lowered->functions.size() == 1 &&
                  lowered->functions.front().blocks.size() == 1,
              "x86 LIR u8 select-plus-tail input should collapse to a shared one-block BIR function before target emission");

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_lir_u8_select_post_join_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose2_add_post_ne_u",
                  "x86 LIR u8 select-plus-tail input should still reach native asm emission after routing through the shared BIR path");
  expect_contains(rendered, ".Lselect_true:\n  movzx eax, dil\n  add eax, 5\n  jmp .Lselect_join\n",
                  "x86 LIR u8 select-plus-tail input should preserve the shared BIR-owned then-arm predecessor arithmetic");
  expect_contains(rendered, ".Lselect_false:\n  movzx eax, sil\n  add eax, 9\n",
                  "x86 LIR u8 select-plus-tail input should preserve the shared BIR-owned else-arm predecessor arithmetic");
  expect_contains(rendered, ".Lselect_join:\n  add eax, 6\n  ret\n",
                  "x86 LIR u8 select-plus-tail input should preserve the shared BIR-owned post-join arithmetic tail");
  expect_not_contains(rendered, ".choose2_add_post_ne_u.tern.then",
                      "x86 lowerable u8 select-plus-tail input should no longer emit the original target-local predecessor block labels once the BIR route owns the shape");
  expect_not_contains(rendered, "target triple =",
                      "x86 LIR u8 select-plus-tail input should stay on native asm emission instead of falling back to LLVM text");

  const auto direct_rendered =
      c4c::backend::x86::emit_module(make_lir_u8_select_post_join_add_module());

  expect_contains(direct_rendered, ".globl choose2_add_post_ne_u",
                  "x86 direct emitter should also route lowerable u8 select-plus-tail input through the shared BIR path before native asm emission");
  expect_contains(direct_rendered, ".Lselect_true:\n  movzx eax, dil\n  add eax, 5\n  jmp .Lselect_join\n",
                  "x86 direct emitter should preserve the shared BIR-owned then-arm predecessor arithmetic for the lowerable u8 select-plus-tail route");
  expect_contains(direct_rendered, ".Lselect_false:\n  movzx eax, sil\n  add eax, 9\n",
                  "x86 direct emitter should preserve the shared BIR-owned else-arm predecessor arithmetic for the lowerable u8 select-plus-tail route");
  expect_contains(direct_rendered, ".Lselect_join:\n  add eax, 6\n  ret\n",
                  "x86 direct emitter should preserve the shared BIR-owned post-join arithmetic tail for the lowerable u8 select-plus-tail route");
  expect_not_contains(direct_rendered, ".choose2_add_post_ne_u.tern.then",
                      "x86 direct emitter should no longer emit the original target-local predecessor block labels for a lowerable u8 select-plus-tail route");
}

void test_backend_bir_pipeline_drives_x86_return_add_smoke_case_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "BIR pipeline should still reach x86 backend emission for the tiny scaffold case");
  expect_contains(rendered, "mov eax, 5",
                  "BIR pipeline should lower the tiny add/return case to the expected x86 return-immediate asm");
}

void test_backend_bir_pipeline_drives_x86_return_sub_smoke_case_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "BIR pipeline should still reach x86 backend emission for the tiny sub scaffold case");
  expect_contains(rendered, "mov eax, 0",
                  "BIR pipeline should lower the tiny sub/return case to the expected x86 return-immediate asm");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_zero_param_staged_constant_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_bir_return_staged_constant_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "direct BIR zero-parameter staged constant input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov eax, 8",
                  "x86 direct BIR zero-parameter staged constant input should collapse the full chain to the surviving constant");
}

void test_backend_bir_pipeline_drives_x86_single_param_chain_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_single_param_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl add_one",
                  "BIR pipeline should still reach x86 backend emission for the bounded one-parameter slice");
  expect_contains(rendered, "mov eax, edi",
                  "x86 BIR lowering should seed the result from the incoming integer argument register");
  expect_contains(rendered, "add eax, 1",
                  "x86 BIR lowering should collapse the bounded parameter-fed chain into the expected affine adjustment");
}

void test_backend_bir_pipeline_drives_x86_two_param_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl add_pair",
                  "BIR pipeline should still reach x86 backend emission for the bounded two-parameter slice");
  expect_contains(rendered, "mov eax, edi",
                  "x86 BIR lowering should seed the result from the first incoming integer argument register");
  expect_contains(rendered, "add eax, esi",
                  "x86 BIR lowering should combine the second integer argument register for the bounded two-parameter add");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_input_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{c4c::backend::lower_to_bir(make_bir_two_param_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl add_pair",
                  "direct BIR module input should reach x86 backend emission without a caller-owned legacy backend module");
  expect_contains(rendered, "mov eax, edi",
                  "x86 direct BIR input should still seed the result from the first incoming integer argument register");
  expect_contains(rendered, "add eax, esi",
                  "x86 direct BIR input should still combine the second integer argument register on the backend path");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_staged_affine_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_bir_two_param_staged_affine_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl add_pair",
                  "direct BIR staged affine input should reach x86 backend emission without caller-owned legacy backend IR");
  expect_contains(rendered, "mov eax, edi",
                  "x86 direct BIR staged affine input should still seed from the first integer argument register");
  expect_contains(rendered, "add eax, esi",
                  "x86 direct BIR staged affine input should still combine the second integer argument register");
  expect_contains(rendered, "add eax, 1",
                  "x86 direct BIR staged affine input should preserve the collapsed affine constant");
}

void test_backend_bir_pipeline_drives_x86_two_param_add_sub_chain_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl add_pair",
                  "BIR pipeline should still reach x86 backend emission for the bounded two-parameter affine chain");
  expect_contains(rendered, "mov eax, edi",
                  "x86 BIR lowering should seed the result from the first incoming integer argument register for the affine chain");
  expect_contains(rendered, "add eax, esi",
                  "x86 BIR lowering should combine the second integer argument register before the immediate adjustment");
  expect_contains(rendered, "sub eax, 1",
                  "x86 BIR lowering should preserve the trailing immediate adjustment for the bounded two-parameter affine chain");
}

void test_backend_bir_pipeline_drives_x86_two_param_staged_affine_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_staged_affine_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl add_pair",
                  "BIR pipeline should still reach x86 backend emission for the staged two-parameter affine slice");
  expect_contains(rendered, "mov eax, edi",
                  "x86 BIR lowering should seed the staged affine slice from the first incoming integer argument register");
  expect_contains(rendered, "add eax, esi",
                  "x86 BIR lowering should still combine the second incoming integer argument register for the staged affine slice");
  expect_contains(rendered, "add eax, 1",
                  "x86 BIR lowering should collapse the staged immediate adjustments to the surviving affine constant");
}

void test_backend_bir_pipeline_drives_x86_sext_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_sext_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl widen_signed",
                  "BIR pipeline should reach x86 backend emission for the bounded sext slice");
  expect_contains(rendered, "movsxd rax, edi",
                  "x86 BIR lowering should sign-extend the incoming i32 argument into the i64 return register");
}

void test_backend_bir_pipeline_drives_x86_zext_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_zext_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl widen_unsigned",
                  "BIR pipeline should reach x86 backend emission for the bounded zext slice");
  expect_contains(rendered, "movzx eax, dil",
                  "x86 BIR lowering should zero-extend the incoming i8 argument into the i32 return register");
}

void test_backend_bir_pipeline_drives_x86_trunc_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_trunc_module()},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl narrow_signed",
                  "BIR pipeline should reach x86 backend emission for the bounded trunc slice");
  expect_contains(rendered, "mov eax, edi",
                  "x86 BIR lowering should truncate the incoming i64 argument by returning its low i32 half");
}

void test_backend_bir_pipeline_drives_x86_select_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_bir_return_select_eq_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl main",
                  "direct BIR select input should reach x86 backend emission without a caller-owned legacy backend module");
  expect_contains(rendered, "cmp eax, 7",
                  "x86 BIR lowering should compare the bounded select operands on the native backend path");
  expect_contains(rendered, "mov eax, 11",
                  "x86 BIR lowering should materialize the true-value arm for the bounded select slice");
  expect_contains(rendered, "mov eax, 4",
                  "x86 BIR lowering should materialize the false-value arm for the bounded select slice");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_single_param_select_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_bir_single_param_select_eq_phi_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose",
                  "direct BIR single-parameter select input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov eax, edi",
                  "x86 direct BIR single-parameter select input should seed the compare from the incoming integer argument register");
  expect_contains(rendered, "cmp eax, 7",
                  "x86 direct BIR single-parameter select input should compare the incoming integer argument against the bounded immediate");
  expect_contains(rendered, "mov eax, 11",
                  "x86 direct BIR single-parameter select input should materialize the true-value arm for the bounded parameter-fed select slice");
  expect_contains(rendered, "mov eax, 4",
                  "x86 direct BIR single-parameter select input should materialize the false-value arm for the bounded parameter-fed select slice");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_two_param_select_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(make_bir_two_param_select_eq_phi_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose2",
                  "direct BIR two-parameter select input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov eax, edi",
                  "x86 direct BIR two-parameter select input should seed the compare from the first incoming integer argument register");
  expect_contains(rendered, "cmp eax, esi",
                  "x86 direct BIR two-parameter select input should compare both incoming integer argument registers on the native backend path");
  expect_contains(rendered, "mov eax, esi",
                  "x86 direct BIR two-parameter select input should materialize the false-value arm from the second incoming integer argument register");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_post_join_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_predecessor_add_phi_post_join_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose2_add_post_ne_u",
                  "direct BIR u8 select-plus-tail input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov al, dil",
                  "x86 direct BIR u8 select-plus-tail input should compare the low byte from the first incoming integer argument");
  expect_contains(rendered, "cmp al, sil",
                  "x86 direct BIR u8 select-plus-tail input should compare the second incoming low byte on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  movzx eax, dil\n  add eax, 5\n  jmp .Lselect_join\n",
                  "x86 direct BIR u8 select-plus-tail input should materialize the bounded then-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  movzx eax, sil\n  add eax, 9\n",
                  "x86 direct BIR u8 select-plus-tail input should materialize the bounded else-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add eax, 6\n  ret\n",
                  "x86 direct BIR u8 select-plus-tail input should preserve the bounded post-select arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_post_join_add_sub_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_split_predecessor_add_phi_post_join_add_sub_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose2_add_post_chain_ne_u",
                  "direct BIR u8 select-plus-add/sub-tail input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov al, dil",
                  "x86 direct BIR u8 select-plus-add/sub-tail input should compare the low byte from the first incoming integer argument");
  expect_contains(rendered, "cmp al, sil",
                  "x86 direct BIR u8 select-plus-add/sub-tail input should compare the second incoming low byte on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  movzx eax, dil\n  add eax, 5\n  jmp .Lselect_join\n",
                  "x86 direct BIR u8 select-plus-add/sub-tail input should materialize the bounded then-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  movzx eax, sil\n  add eax, 9\n",
                  "x86 direct BIR u8 select-plus-add/sub-tail input should materialize the bounded else-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add eax, 6\n  sub eax, 2\n  ret\n",
                  "x86 direct BIR u8 select-plus-add/sub-tail input should preserve the bounded post-select add/sub arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_post_join_add_sub_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_split_predecessor_add_phi_post_join_add_sub_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose2_add_post_chain_tail_ne_u",
                  "direct BIR u8 select-plus-add/sub/add-tail input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov al, dil",
                  "x86 direct BIR u8 select-plus-add/sub/add-tail input should compare the low byte from the first incoming integer argument");
  expect_contains(rendered, "cmp al, sil",
                  "x86 direct BIR u8 select-plus-add/sub/add-tail input should compare the second incoming low byte on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  movzx eax, dil\n  add eax, 5\n  jmp .Lselect_join\n",
                  "x86 direct BIR u8 select-plus-add/sub/add-tail input should materialize the bounded then-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  movzx eax, sil\n  add eax, 9\n",
                  "x86 direct BIR u8 select-plus-add/sub/add-tail input should materialize the bounded else-arm predecessor arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add eax, 6\n  sub eax, 2\n  add eax, 9\n  ret\n",
                  "x86 direct BIR u8 select-plus-add/sub/add-tail input should preserve the bounded post-select add/sub/add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_mixed_affine_post_join_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_split_predecessor_mixed_affine_phi_post_join_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose2_mixed_post_ne_u",
                  "direct BIR u8 mixed-affine select-plus-tail input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov al, dil",
                  "x86 direct BIR u8 mixed-affine select-plus-tail input should compare the low byte from the first incoming integer argument");
  expect_contains(rendered, "cmp al, sil",
                  "x86 direct BIR u8 mixed-affine select-plus-tail input should compare the second incoming low byte on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  movzx eax, dil\n  add eax, 8\n  sub eax, 3\n  jmp .Lselect_join\n",
                  "x86 direct BIR u8 mixed-affine select-plus-tail input should materialize the bounded then-arm affine arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  movzx eax, sil\n  add eax, 11\n  sub eax, 4\n",
                  "x86 direct BIR u8 mixed-affine select-plus-tail input should materialize the bounded else-arm affine arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add eax, 6\n  ret\n",
                  "x86 direct BIR u8 mixed-affine select-plus-tail input should preserve the bounded post-select add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_mixed_affine_post_join_add_sub_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_split_predecessor_mixed_affine_phi_post_join_add_sub_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose2_mixed_post_chain_ne_u",
                  "direct BIR u8 mixed-affine select-plus-add/sub-tail input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov al, dil",
                  "x86 direct BIR u8 mixed-affine select-plus-add/sub-tail input should compare the low byte from the first incoming integer argument");
  expect_contains(rendered, "cmp al, sil",
                  "x86 direct BIR u8 mixed-affine select-plus-add/sub-tail input should compare the second incoming low byte on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  movzx eax, dil\n  add eax, 8\n  sub eax, 3\n  jmp .Lselect_join\n",
                  "x86 direct BIR u8 mixed-affine select-plus-add/sub-tail input should materialize the bounded then-arm affine arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  movzx eax, sil\n  add eax, 11\n  sub eax, 4\n",
                  "x86 direct BIR u8 mixed-affine select-plus-add/sub-tail input should materialize the bounded else-arm affine arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add eax, 6\n  sub eax, 2\n  ret\n",
                  "x86 direct BIR u8 mixed-affine select-plus-add/sub-tail input should preserve the bounded post-select add/sub arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_mixed_affine_post_join_add_sub_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_eq_split_predecessor_mixed_affine_phi_post_join_add_sub_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose2_mixed_post_chain_tail_u",
                  "direct BIR u8 mixed-affine select-plus-add/sub/add-tail input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov al, dil",
                  "x86 direct BIR u8 mixed-affine select-plus-add/sub/add-tail input should compare the low byte from the first incoming integer argument");
  expect_contains(rendered, "cmp al, sil",
                  "x86 direct BIR u8 mixed-affine select-plus-add/sub/add-tail input should compare the second incoming low byte on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  movzx eax, dil\n  add eax, 8\n  sub eax, 3\n  jmp .Lselect_join\n",
                  "x86 direct BIR u8 mixed-affine select-plus-add/sub/add-tail input should materialize the bounded then-arm affine arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  movzx eax, sil\n  add eax, 11\n  sub eax, 4\n",
                  "x86 direct BIR u8 mixed-affine select-plus-add/sub/add-tail input should materialize the bounded else-arm affine arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add eax, 6\n  sub eax, 2\n  add eax, 9\n  ret\n",
                  "x86 direct BIR u8 mixed-affine select-plus-add/sub/add-tail input should preserve the bounded post-select add/sub/add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_deeper_then_mixed_affine_post_join_add_sub_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose2_deeper_post_chain_tail_u",
                  "direct BIR u8 deeper-then-mixed-affine select-plus-add/sub/add-tail input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov al, dil",
                  "x86 direct BIR u8 deeper-then-mixed-affine select-plus-add/sub/add-tail input should compare the low byte from the first incoming integer argument");
  expect_contains(rendered, "cmp al, sil",
                  "x86 direct BIR u8 deeper-then-mixed-affine select-plus-add/sub/add-tail input should compare the second incoming low byte on the native backend path");
  expect_contains(rendered,
                  ".Lselect_true:\n  movzx eax, dil\n  add eax, 8\n  sub eax, 3\n  add eax, 5\n  jmp .Lselect_join\n",
                  "x86 direct BIR u8 deeper-then-mixed-affine select-plus-add/sub/add-tail input should materialize the bounded deeper then-arm arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  movzx eax, sil\n  add eax, 11\n  sub eax, 4\n",
                  "x86 direct BIR u8 deeper-then-mixed-affine select-plus-add/sub/add-tail input should materialize the bounded else-arm mixed arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add eax, 6\n  sub eax, 2\n  add eax, 9\n  ret\n",
                  "x86 direct BIR u8 deeper-then-mixed-affine select-plus-add/sub/add-tail input should preserve the bounded post-select add/sub/add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_deeper_affine_on_both_sides_post_join_add_sub_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_eq_split_predecessor_deeper_affine_phi_post_join_add_sub_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose2_deeper_both_post_chain_tail_u",
                  "direct BIR u8 deeper-affine-both-sides select-plus-add/sub/add-tail input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov al, dil",
                  "x86 direct BIR u8 deeper-affine-both-sides select-plus-add/sub/add-tail input should compare the low byte from the first incoming integer argument");
  expect_contains(rendered, "cmp al, sil",
                  "x86 direct BIR u8 deeper-affine-both-sides select-plus-add/sub/add-tail input should compare the second incoming low byte on the native backend path");
  expect_contains(rendered,
                  ".Lselect_true:\n  movzx eax, dil\n  add eax, 8\n  sub eax, 3\n  add eax, 5\n  jmp .Lselect_join\n",
                  "x86 direct BIR u8 deeper-affine-both-sides select-plus-add/sub/add-tail input should materialize the bounded deeper then-arm arithmetic before the synthetic join");
  expect_contains(rendered,
                  ".Lselect_false:\n  movzx eax, sil\n  add eax, 11\n  sub eax, 4\n  add eax, 7\n",
                  "x86 direct BIR u8 deeper-affine-both-sides select-plus-add/sub/add-tail input should materialize the bounded deeper else-arm arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add eax, 6\n  sub eax, 2\n  add eax, 9\n  ret\n",
                  "x86 direct BIR u8 deeper-affine-both-sides select-plus-add/sub/add-tail input should preserve the bounded post-select add/sub/add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_mixed_then_deeper_affine_post_join_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_u8_select_ne_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose2_mixed_then_deeper_post_ne_u",
                  "direct BIR u8 mixed-then-deeper-affine select-plus-tail input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov al, dil",
                  "x86 direct BIR u8 mixed-then-deeper-affine select-plus-tail input should compare the low byte from the first incoming integer argument");
  expect_contains(rendered, "cmp al, sil",
                  "x86 direct BIR u8 mixed-then-deeper-affine select-plus-tail input should compare the second incoming low byte on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  movzx eax, dil\n  add eax, 8\n  sub eax, 3\n  jmp .Lselect_join\n",
                  "x86 direct BIR u8 mixed-then-deeper-affine select-plus-tail input should materialize the bounded then-arm mixed arithmetic before the synthetic join");
  expect_contains(rendered,
                  ".Lselect_false:\n  movzx eax, sil\n  add eax, 11\n  sub eax, 4\n  add eax, 7\n",
                  "x86 direct BIR u8 mixed-then-deeper-affine select-plus-tail input should materialize the bounded else-arm deeper arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add eax, 6\n  ret\n",
                  "x86 direct BIR u8 mixed-then-deeper-affine select-plus-tail input should preserve the bounded post-select add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_i32_select_mixed_then_deeper_affine_post_join_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose2_mixed_then_deeper_post",
                  "direct BIR i32 mixed-then-deeper-affine select-plus-tail input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov eax, edi",
                  "x86 direct BIR i32 mixed-then-deeper-affine select-plus-tail input should seed the compare from the first incoming integer argument register");
  expect_contains(rendered, "cmp eax, esi",
                  "x86 direct BIR i32 mixed-then-deeper-affine select-plus-tail input should compare both incoming integer argument registers on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  mov eax, edi\n  add eax, 8\n  sub eax, 3\n  jmp .Lselect_join\n",
                  "x86 direct BIR i32 mixed-then-deeper-affine select-plus-tail input should materialize the bounded then-arm mixed arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  mov eax, esi\n  add eax, 11\n  sub eax, 4\n  add eax, 7\n",
                  "x86 direct BIR i32 mixed-then-deeper-affine select-plus-tail input should materialize the bounded else-arm deeper arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add eax, 6\n  ret\n",
                  "x86 direct BIR i32 mixed-then-deeper-affine select-plus-tail input should preserve the bounded post-select add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_i32_select_mixed_then_deeper_affine_post_join_add_sub_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose2_mixed_then_deeper_post_chain",
                  "direct BIR i32 mixed-then-deeper-affine select-plus-add/sub-tail input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov eax, edi",
                  "x86 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub-tail input should seed the compare from the first incoming integer argument register");
  expect_contains(rendered, "cmp eax, esi",
                  "x86 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub-tail input should compare both incoming integer argument registers on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  mov eax, edi\n  add eax, 8\n  sub eax, 3\n  jmp .Lselect_join\n",
                  "x86 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub-tail input should materialize the bounded then-arm mixed arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  mov eax, esi\n  add eax, 11\n  sub eax, 4\n  add eax, 7\n",
                  "x86 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub-tail input should materialize the bounded else-arm deeper arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add eax, 6\n  sub eax, 2\n  ret\n",
                  "x86 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub-tail input should preserve the bounded post-select add/sub arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_drives_x86_direct_bir_i32_select_mixed_then_deeper_affine_post_join_add_sub_add_end_to_end() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          c4c::backend::lower_to_bir(
              make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_add_module())},
      make_bir_pipeline_options(c4c::backend::Target::X86_64));

  expect_contains(rendered, ".globl choose2_mixed_then_deeper_post_chain_tail",
                  "direct BIR i32 mixed-then-deeper-affine select-plus-add/sub/add-tail input should reach x86 backend emission without legacy backend IR lowering");
  expect_contains(rendered, "mov eax, edi",
                  "x86 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub/add-tail input should seed the compare from the first incoming integer argument register");
  expect_contains(rendered, "cmp eax, esi",
                  "x86 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub/add-tail input should compare both incoming integer argument registers on the native backend path");
  expect_contains(rendered, ".Lselect_true:\n  mov eax, edi\n  add eax, 8\n  sub eax, 3\n  jmp .Lselect_join\n",
                  "x86 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub/add-tail input should materialize the bounded then-arm mixed arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_false:\n  mov eax, esi\n  add eax, 11\n  sub eax, 4\n  add eax, 7\n",
                  "x86 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub/add-tail input should materialize the bounded else-arm deeper arithmetic before the synthetic join");
  expect_contains(rendered, ".Lselect_join:\n  add eax, 6\n  sub eax, 2\n  add eax, 9\n  ret\n",
                  "x86 direct BIR i32 mixed-then-deeper-affine select-plus-add/sub/add-tail input should preserve the bounded post-select add/sub/add arithmetic tail on the native backend path");
}

void test_backend_bir_pipeline_rejects_unsupported_direct_bir_input_on_x86() {
  try {
    (void)c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{make_unsupported_multi_function_bir_module()},
        make_bir_pipeline_options(c4c::backend::Target::X86_64));
  } catch (const std::invalid_argument& ex) {
    expect_contains(ex.what(), "direct BIR module",
                    "x86 direct BIR rejection should explain that unsupported direct BIR input no longer falls back through legacy backend IR");
    expect_not_contains(ex.what(), "legacy backend IR",
                        "x86 direct BIR rejection should not mention the deleted legacy backend IR route at the shared backend entry");
    return;
  }

  fail("x86 direct BIR input should fail explicitly once the emitter-side bir_to_backend_ir fallback is removed");
}

void test_x86_direct_lir_emitter_rejects_unsupported_input_without_legacy_backend_ir_stub() {
  try {
    (void)c4c::backend::x86::emit_module(make_unsupported_double_return_lir_module());
  } catch (const std::invalid_argument& ex) {
    expect_contains(ex.what(), "direct LIR module",
                    "x86 direct LIR rejection should describe the direct-LIR subset instead of a deleted legacy backend IR route");
    expect_not_contains(ex.what(), "legacy backend IR",
                        "x86 direct LIR rejection should not mention the deleted legacy backend IR path");
    return;
  }

  fail("x86 direct LIR emitter should fail explicitly once unsupported input stops pretending a legacy backend IR route still exists");
}

void test_x86_direct_lir_emitter_rejects_alloca_backed_conditional_phi_constant_fold_stub() {
  expect_true(!c4c::backend::try_lower_to_bir(
                   make_alloca_backed_conditional_phi_constant_module())
                   .has_value(),
              "alloca-backed conditional-phi input should bypass the shared BIR lowering seam so this regression exercises the remaining direct-LIR helper boundary");

  try {
    (void)c4c::backend::x86::emit_module(make_alloca_backed_conditional_phi_constant_module());
  } catch (const std::invalid_argument& ex) {
    expect_contains(ex.what(), "direct LIR module",
                    "x86 direct emitter should reject an alloca-backed conditional-phi join instead of reviving phi-aware constant folding past the BIR boundary");
    return;
  }

  fail("x86 direct emitter should reject alloca-backed conditional-phi joins once phi-aware constant folding is pruned from the direct-LIR fallback");
}

void test_x86_direct_emitter_routes_goto_only_constant_return_through_shared_bir() {
  expect_true(c4c::backend::try_lower_to_bir(make_goto_only_constant_return_module()).has_value(),
              "goto-only constant-return input should now lower through the shared BIR boundary instead of depending on the deleted x86 direct-LIR CFG helper");

  const auto rendered = c4c::backend::x86::emit_module(make_goto_only_constant_return_module());
  expect_contains(rendered, "main:\n  mov eax, 9\n  ret\n",
                  "x86 direct emitter should now use the shared BIR-owned immediate-return route for the goto-only constant-return slice");
  expect_not_contains(rendered, "ulbl_start",
                      "x86 direct emitter should no longer expose the old direct-LIR goto chain once shared BIR owns the branch-only constant-return shape");
}

void test_x86_direct_emitter_lowers_minimal_local_temp_slice() {
  auto module = make_x86_local_temp_lir_module();

  expect_true(!c4c::backend::try_lower_to_bir(module).has_value(),
              "minimal x86 local-temp input should continue to miss shared BIR lowering so this regression exercises the x86-owned direct-LIR seam");

  const auto rendered = c4c::backend::x86::emit_module(module);
  expect_contains(rendered, ".globl main",
                  "x86 direct emitter should still emit the function definition for the bounded local-temp direct-LIR slice");
  expect_contains(rendered, "main:\n  mov eax, 5\n  ret\n",
                  "x86 direct emitter should fold the bounded store-load local-temp slice into a direct return-immediate sequence");
  expect_not_contains(rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded local-temp direct-LIR slice");
}

void test_x86_direct_emitter_lowers_constant_add_mul_sub_return_slice() {
  auto module = make_x86_constant_add_mul_sub_return_lir_module();

  expect_true(c4c::backend::try_lower_to_bir(module).has_value(),
              "constant add/mul/sub return input should continue to route through shared BIR so this regression pins the x86 BIR-owned emission seam");

  const auto rendered = c4c::backend::x86::emit_module(module);
  expect_contains(rendered, "main:\n  mov eax, 0\n  ret\n",
                  "x86 direct emitter should constant-fold the bounded add/mul/sub return chain once the shared-BIR route reaches native x86 emission");
  expect_not_contains(rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded add/mul/sub return slice");
}

void test_x86_direct_emitter_lowers_constant_div_sub_return_slice() {
  auto module = make_x86_constant_div_sub_return_lir_module();

  expect_true(c4c::backend::try_lower_to_bir(module).has_value(),
              "constant sdiv/sub return input should continue to route through shared BIR so this regression pins the x86 BIR-owned emission seam");

  const auto rendered = c4c::backend::x86::emit_module(module);
  expect_contains(rendered, "main:\n  mov eax, 0\n  ret\n",
                  "x86 direct emitter should constant-fold the bounded sdiv/sub return chain once the shared-BIR route reaches native x86 emission");
  expect_not_contains(rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded sdiv/sub return slice");
}

void test_x86_direct_emitter_lowers_local_i32_arithmetic_chain_return_slice() {
  auto module = make_local_i32_arithmetic_chain_return_immediate_module();

  expect_true(c4c::backend::try_lower_to_bir(module).has_value(),
              "local i32 arithmetic-chain return input should continue to route through shared BIR so this regression pins the x86 BIR-owned source-backed seam");

  const auto rendered = c4c::backend::x86::emit_module(module);
  expect_contains(rendered, "main:\n  mov eax, 0\n  ret\n",
                  "x86 direct emitter should constant-fold the bounded local-slot arithmetic chain once the shared-BIR route reaches native x86 emission");
  expect_not_contains(rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded local-slot arithmetic-chain slice");
}

void test_x86_direct_emitter_lowers_two_local_i32_zero_init_return_first_slice() {
  auto module = make_two_local_i32_zero_init_return_first_module();

  expect_true(c4c::backend::try_lower_to_bir(module).has_value(),
              "two-local zero-init return input should continue to route through shared BIR so this regression pins the x86 BIR-owned source-backed seam");

  const auto rendered = c4c::backend::x86::emit_module(module);
  expect_contains(rendered, "main:\n  mov eax, 0\n  ret\n",
                  "x86 direct emitter should constant-fold the bounded two-local zero-init return slice once the shared-BIR route reaches native x86 emission");
  expect_not_contains(rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded two-local zero-init return slice");
}

void test_x86_direct_emitter_lowers_local_i32_array_two_slot_sum_sub_three_slice() {
  auto module = make_local_i32_array_two_slot_sum_sub_three_module();

  expect_true(c4c::backend::try_lower_to_bir(module).has_value(),
              "two-slot local-array sum-sub-three input should continue to route through shared BIR so this regression pins the x86 BIR-owned source-backed seam");

  const auto rendered = c4c::backend::x86::emit_module(module);
  expect_contains(rendered, "main:\n  mov eax, 0\n  ret\n",
                  "x86 direct emitter should constant-fold the bounded two-slot local-array sum-sub-three slice once the shared-BIR route reaches native x86 emission");
  expect_not_contains(rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded two-slot local-array sum-sub-three slice");
}

void test_x86_direct_emitter_lowers_local_i32_array_second_slot_pointer_store_zero_load_return_slice() {
  auto module = make_local_i32_array_second_slot_pointer_store_zero_load_return_module();

  expect_true(c4c::backend::try_lower_to_bir(module).has_value(),
              "local-array second-slot pointer-store-zero-load-return input should continue to route through shared BIR so this regression pins the x86 BIR-owned source-backed seam");

  const auto rendered = c4c::backend::x86::emit_module(module);
  expect_contains(rendered, "main:\n  mov eax, 0\n  ret\n",
                  "x86 direct emitter should constant-fold the bounded local-array second-slot pointer-store-zero-load-return slice once the shared-BIR route reaches native x86 emission");
  expect_not_contains(rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded local-array second-slot pointer-store-zero-load-return slice");
}

void test_x86_direct_emitter_lowers_local_i32_array_pointer_inc_dec_compare_zero_slice() {
  auto module = make_local_i32_array_pointer_inc_dec_compare_zero_return_module();

  expect_true(c4c::backend::try_lower_to_bir(module).has_value(),
              "local-array pointer increment-decrement compare input should continue to route through shared BIR so this regression pins the x86 BIR-owned source-backed seam");

  const auto rendered = c4c::backend::x86::emit_module(module);
  expect_contains(rendered, "main:\n  mov eax, 0\n  ret\n",
                  "x86 direct emitter should constant-fold the bounded local-array pointer increment-decrement compare slice once the shared-BIR route reaches native x86 emission");
  expect_not_contains(rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded local-array pointer increment-decrement compare slice");
}

void test_x86_direct_emitter_lowers_constant_branch_if_eq_return_slice() {
  auto module = make_x86_constant_branch_if_return_lir_module("eq", "2", "2");

  const auto rendered = c4c::backend::x86::try_emit_prepared_lir_module(module);
  expect_true(rendered.has_value(),
              "x86 direct emitter should accept the bounded constant eq branch-return slice through the native prepared-LIR seam");
  if (!rendered.has_value()) {
    return;
  }
  expect_contains(*rendered, "main:\n  mov eax, 0\n  ret\n",
                  "x86 direct emitter should constant-fold the bounded eq branch-return slice down to the selected return immediate");
  expect_not_contains(*rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded eq branch-return slice");
}

void test_x86_direct_emitter_lowers_constant_branch_if_uge_return_slice() {
  auto module = make_x86_constant_branch_if_return_lir_module("uge", "3", "2");

  const auto rendered = c4c::backend::x86::try_emit_prepared_lir_module(module);
  expect_true(rendered.has_value(),
              "x86 direct emitter should accept the bounded constant unsigned-ge branch-return slice through the native prepared-LIR seam");
  if (!rendered.has_value()) {
    return;
  }
  expect_contains(*rendered, "main:\n  mov eax, 0\n  ret\n",
                  "x86 direct emitter should constant-fold the bounded unsigned-ge branch-return slice down to the selected return immediate");
  expect_not_contains(*rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded unsigned-ge branch-return slice");
}

void test_x86_direct_emitter_lowers_minimal_param_slot_add_slice() {
  auto module = make_x86_param_slot_add_lir_module();

  const auto rendered = c4c::backend::x86::try_emit_prepared_lir_module(module);
  expect_true(rendered.has_value(),
              "x86 direct emitter should accept the bounded single-param slot-add helper through the native prepared-LIR seam");
  if (!rendered.has_value()) {
    return;
  }
  expect_contains(*rendered, ".globl add_one",
                  "x86 direct emitter should still emit the helper definition for the bounded param-slot direct-LIR slice");
  expect_contains(*rendered, "add_one:\n  mov eax, edi\n  add eax, 1\n  ret\n",
                  "x86 direct emitter should fold the bounded single-param slot-add helper into a register add sequence");
  expect_contains(*rendered, "main:\n  mov edi, 5\n  call add_one\n  ret\n",
                  "x86 direct emitter should keep the bounded entry call on the native direct-LIR path");
  expect_not_contains(*rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded param-slot direct-LIR slice");
}

void test_x86_direct_emitter_lowers_minimal_extern_zero_arg_call_slice() {
  auto module = make_x86_declared_zero_arg_call_lir_module();

  const auto rendered = c4c::backend::x86::try_emit_prepared_lir_module(module);
  expect_true(rendered.has_value(),
              "x86 direct emitter should accept the bounded extern zero-arg helper call through the native prepared-LIR seam");
  if (!rendered.has_value()) {
    return;
  }
  expect_contains(*rendered, ".extern helper_ext\n",
                  "x86 direct emitter should preserve the extern helper declaration for the bounded zero-arg call slice");
  expect_contains(*rendered, "main:\n  call helper_ext\n  ret\n",
                  "x86 direct emitter should lower the bounded extern zero-arg helper call without reviving legacy backend IR");
  expect_not_contains(*rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded extern zero-arg call slice");
}

void test_x86_direct_emitter_lowers_minimal_local_arg_call_slice() {
  auto module = make_x86_local_arg_call_lir_module();

  const auto rendered = c4c::backend::x86::try_emit_prepared_lir_module(module);
  expect_true(rendered.has_value(),
              "x86 direct emitter should accept the bounded single-local-arg helper call through the native prepared-LIR seam");
  if (!rendered.has_value()) {
    return;
  }
  expect_contains(*rendered, ".globl add_one",
                  "x86 direct emitter should still emit the helper definition for the bounded local-arg direct-LIR slice");
  expect_contains(*rendered, "add_one:\n  mov eax, edi\n  add eax, 1\n  ret\n",
                  "x86 direct emitter should keep the helper add-immediate body on the native direct-LIR path");
  expect_contains(*rendered, "main:\n  mov edi, 5\n  call add_one\n  ret\n",
                  "x86 direct emitter should materialize the single local-slot argument before calling the helper on the native x86 path");
  expect_not_contains(*rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded local-arg direct-LIR slice");
}

void test_x86_direct_emitter_lowers_minimal_two_arg_helper_call_slice() {
  auto module = make_lir_minimal_two_arg_direct_call_module();

  const auto rendered = c4c::backend::x86::try_emit_prepared_lir_module(module);
  expect_true(rendered.has_value(),
              "x86 direct emitter should accept the bounded two-arg helper call through the native prepared-LIR seam");
  if (!rendered.has_value()) {
    return;
  }
  expect_contains(*rendered, ".globl add_pair",
                  "x86 direct emitter should still emit the helper definition for the bounded two-arg direct-LIR slice");
  expect_contains(*rendered, "add_pair:\n  mov eax, edi\n  add eax, esi\n  ret\n",
                  "x86 direct emitter should keep the bounded two-parameter helper body on the native direct-LIR path");
  expect_contains(*rendered, "main:\n  mov edi, 5\n  mov esi, 7\n  call add_pair\n  ret\n",
                  "x86 direct emitter should materialize both immediate call operands before invoking the two-arg helper on the native x86 path");
  expect_not_contains(*rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded two-arg helper slice");
}

void test_x86_direct_emitter_lowers_minimal_two_arg_local_arg_call_slice() {
  auto module = make_lir_minimal_two_arg_local_arg_direct_call_module();

  const auto rendered = c4c::backend::x86::try_emit_prepared_lir_module(module);
  expect_true(rendered.has_value(),
              "x86 direct emitter should accept the bounded first-local two-arg helper call through the native prepared-LIR seam");
  if (!rendered.has_value()) {
    return;
  }
  expect_contains(*rendered, ".globl add_pair",
                  "x86 direct emitter should still emit the helper definition for the bounded first-local two-arg direct-LIR slice");
  expect_contains(*rendered, "add_pair:\n  mov eax, edi\n  add eax, esi\n  ret\n",
                  "x86 direct emitter should keep the bounded two-parameter helper body on the native direct-LIR path");
  expect_contains(*rendered, "main:\n  mov edi, 5\n  mov esi, 7\n  call add_pair\n  ret\n",
                  "x86 direct emitter should materialize the reloaded local first operand and immediate second operand before invoking the helper on the native x86 path");
  expect_not_contains(*rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded first-local two-arg helper slice");
}

void test_x86_direct_emitter_lowers_minimal_two_arg_second_local_arg_call_slice() {
  auto module = make_lir_minimal_two_arg_second_local_arg_direct_call_module();

  const auto rendered = c4c::backend::x86::try_emit_prepared_lir_module(module);
  expect_true(rendered.has_value(),
              "x86 direct emitter should accept the bounded second-local two-arg helper call through the native prepared-LIR seam");
  if (!rendered.has_value()) {
    return;
  }
  expect_contains(*rendered, ".globl add_pair",
                  "x86 direct emitter should still emit the helper definition for the bounded second-local two-arg direct-LIR slice");
  expect_contains(*rendered, "add_pair:\n  mov eax, edi\n  add eax, esi\n  ret\n",
                  "x86 direct emitter should keep the bounded two-parameter helper body on the native direct-LIR path");
  expect_contains(*rendered, "main:\n  mov edi, 5\n  mov esi, 7\n  call add_pair\n  ret\n",
                  "x86 direct emitter should materialize the immediate first operand and reloaded local second operand before invoking the helper on the native x86 path");
  expect_not_contains(*rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded second-local two-arg helper slice");
}

void test_x86_direct_emitter_lowers_minimal_two_arg_second_local_rewrite_call_slice() {
  auto module = make_lir_minimal_two_arg_second_local_rewrite_direct_call_module();

  const auto rendered = c4c::backend::x86::try_emit_prepared_lir_module(module);
  expect_true(rendered.has_value(),
              "x86 direct emitter should accept the bounded second-local rewrite two-arg helper call through the native prepared-LIR seam");
  if (!rendered.has_value()) {
    return;
  }
  expect_contains(*rendered, ".globl add_pair",
                  "x86 direct emitter should still emit the helper definition for the bounded second-local rewrite two-arg direct-LIR slice");
  expect_contains(*rendered, "add_pair:\n  mov eax, edi\n  add eax, esi\n  ret\n",
                  "x86 direct emitter should keep the bounded two-parameter helper body on the native direct-LIR path");
  expect_contains(*rendered, "main:\n  mov edi, 5\n  mov esi, 7\n  call add_pair\n  ret\n",
                  "x86 direct emitter should fold the second-local rewrite back to the stored immediate before invoking the helper on the native x86 path");
  expect_not_contains(*rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded second-local rewrite two-arg helper slice");
}

void test_x86_direct_emitter_lowers_minimal_two_arg_first_local_rewrite_call_slice() {
  auto module = make_lir_minimal_two_arg_first_local_rewrite_direct_call_module();

  const auto rendered = c4c::backend::x86::try_emit_prepared_lir_module(module);
  expect_true(rendered.has_value(),
              "x86 direct emitter should accept the bounded first-local rewrite two-arg helper call through the native prepared-LIR seam");
  if (!rendered.has_value()) {
    return;
  }
  expect_contains(*rendered, ".globl add_pair",
                  "x86 direct emitter should still emit the helper definition for the bounded first-local rewrite two-arg direct-LIR slice");
  expect_contains(*rendered, "add_pair:\n  mov eax, edi\n  add eax, esi\n  ret\n",
                  "x86 direct emitter should keep the bounded two-parameter helper body on the native direct-LIR path");
  expect_contains(*rendered, "main:\n  mov edi, 5\n  mov esi, 7\n  call add_pair\n  ret\n",
                  "x86 direct emitter should fold the first-local rewrite back to the stored immediate before invoking the helper on the native x86 path");
  expect_not_contains(*rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded first-local rewrite two-arg helper slice");
}

void test_x86_direct_emitter_lowers_minimal_two_arg_both_local_arg_call_slice() {
  auto module = make_lir_minimal_two_arg_both_local_arg_direct_call_module();

  const auto rendered = c4c::backend::x86::try_emit_prepared_lir_module(module);
  expect_true(rendered.has_value(),
              "x86 direct emitter should accept the bounded both-local two-arg helper call through the native prepared-LIR seam");
  if (!rendered.has_value()) {
    return;
  }
  expect_contains(*rendered, ".globl add_pair",
                  "x86 direct emitter should still emit the helper definition for the bounded both-local two-arg direct-LIR slice");
  expect_contains(*rendered, "add_pair:\n  mov eax, edi\n  add eax, esi\n  ret\n",
                  "x86 direct emitter should keep the bounded two-parameter helper body on the native direct-LIR path");
  expect_contains(*rendered, "main:\n  mov edi, 5\n  mov esi, 7\n  call add_pair\n  ret\n",
                  "x86 direct emitter should materialize both reloaded local operands before invoking the helper on the native x86 path");
  expect_not_contains(*rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded both-local two-arg helper slice");
}

void test_x86_direct_emitter_lowers_minimal_two_arg_both_local_first_rewrite_call_slice() {
  auto module = make_lir_minimal_two_arg_both_local_first_rewrite_direct_call_module();
  const auto prepared =
      c4c::backend::prepare_lir_module_for_target(module, c4c::backend::Target::X86_64);

  const auto rendered = c4c::backend::x86::try_emit_prepared_lir_module(prepared);
  expect_true(rendered.has_value(),
              "x86 direct emitter should accept the bounded both-local first-rewrite two-arg helper call through the native prepared-LIR seam");
  if (!rendered.has_value()) {
    return;
  }
  expect_contains(*rendered, ".globl add_pair",
                  "x86 direct emitter should still emit the helper definition for the bounded both-local first-rewrite two-arg direct-LIR slice");
  expect_contains(*rendered, "add_pair:\n  mov eax, edi\n  add eax, esi\n  ret\n",
                  "x86 direct emitter should keep the bounded two-parameter helper body on the native direct-LIR path");
  expect_contains(*rendered, "main:\n  mov edi, 5\n  mov esi, 7\n  call add_pair\n  ret\n",
                  "x86 direct emitter should fold the first local rewrite while still reloading the second local operand before invoking the helper on the native x86 path");
  expect_not_contains(*rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded both-local first-rewrite helper slice");
}

void test_x86_direct_emitter_lowers_minimal_two_arg_both_local_second_rewrite_call_slice() {
  auto module = make_lir_minimal_two_arg_both_local_second_rewrite_direct_call_module();
  const auto prepared =
      c4c::backend::prepare_lir_module_for_target(module, c4c::backend::Target::X86_64);

  const auto rendered = c4c::backend::x86::try_emit_prepared_lir_module(prepared);
  expect_true(rendered.has_value(),
              "x86 direct emitter should accept the bounded both-local second-rewrite two-arg helper call through the native prepared-LIR seam");
  if (!rendered.has_value()) {
    return;
  }
  expect_contains(*rendered, ".globl add_pair",
                  "x86 direct emitter should still emit the helper definition for the bounded both-local second-rewrite two-arg direct-LIR slice");
  expect_contains(*rendered, "add_pair:\n  mov eax, edi\n  add eax, esi\n  ret\n",
                  "x86 direct emitter should keep the bounded two-parameter helper body on the native direct-LIR path");
  expect_contains(*rendered, "main:\n  mov edi, 5\n  mov esi, 7\n  call add_pair\n  ret\n",
                  "x86 direct emitter should fold the second local rewrite while still reloading the first local operand before invoking the helper on the native x86 path");
  expect_not_contains(*rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded both-local second-rewrite helper slice");
}

void test_x86_direct_emitter_lowers_minimal_two_arg_both_local_double_rewrite_call_slice() {
  auto module = make_lir_minimal_two_arg_both_local_double_rewrite_direct_call_module();
  const auto prepared =
      c4c::backend::prepare_lir_module_for_target(module, c4c::backend::Target::X86_64);

  const auto rendered = c4c::backend::x86::try_emit_prepared_lir_module(prepared);
  expect_true(rendered.has_value(),
              "x86 direct emitter should accept the bounded both-local double-rewrite two-arg helper call through the native prepared-LIR seam");
  if (!rendered.has_value()) {
    return;
  }
  expect_contains(*rendered, ".globl add_pair",
                  "x86 direct emitter should still emit the helper definition for the bounded both-local double-rewrite two-arg direct-LIR slice");
  expect_contains(*rendered, "add_pair:\n  mov eax, edi\n  add eax, esi\n  ret\n",
                  "x86 direct emitter should keep the bounded two-parameter helper body on the native direct-LIR path");
  expect_contains(*rendered, "main:\n  mov edi, 5\n  mov esi, 7\n  call add_pair\n  ret\n",
                  "x86 direct emitter should fold both local rewrites while still reloading both operands before invoking the helper on the native x86 path");
  expect_not_contains(*rendered, "target triple =",
                      "x86 direct emitter should stay on native asm emission for the bounded both-local double-rewrite helper slice");
}

void test_x86_direct_lir_emitter_rejects_double_indirect_pointer_conditional_return_fallback() {
  expect_true(
      !c4c::backend::try_lower_to_bir(
           make_double_indirect_local_pointer_conditional_return_module())
           .has_value(),
      "double-indirect local-pointer conditional-return input should continue to miss shared BIR lowering so this regression exercises the remaining direct-LIR CFG helper boundary");

  try {
    (void)c4c::backend::x86::emit_module(
        make_double_indirect_local_pointer_conditional_return_module());
  } catch (const std::invalid_argument& ex) {
    expect_contains(ex.what(), "direct LIR module",
                    "x86 direct emitter should reject double-indirect local-pointer conditional-return folding once CFG ownership stays behind the shared BIR boundary");
    return;
  }

  fail("x86 direct emitter should reject double-indirect local-pointer conditional-return modules once the direct-LIR CFG helper is pruned");
}

}  // namespace

void run_backend_bir_pipeline_x86_64_tests() {
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_minimal_direct_call_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_declared_direct_call_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_minimal_void_direct_call_imm_return_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_minimal_two_arg_direct_call_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_minimal_direct_call_add_imm_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_minimal_direct_call_identity_arg_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_minimal_dual_identity_direct_call_sub_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_minimal_call_crossing_direct_call_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_minimal_countdown_loop_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_minimal_scalar_global_load_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_minimal_extern_scalar_global_load_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_minimal_scalar_global_store_reload_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_lowers_x86_direct_call_helper_families_to_shared_bir_views);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_direct_call_through_bir_end_to_end);
  RUN_TEST(test_x86_direct_emitter_lowers_minimal_direct_call_via_outer_bir_path);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_direct_call_with_dead_entry_alloca_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_void_direct_call_imm_return_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_declared_direct_call_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_string_literal_strlen_sub_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_string_literal_char_sub_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_local_i32_store_or_sub_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_local_i32_store_and_sub_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_local_i32_store_xor_sub_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_local_i32_inc_dec_compare_return_zero_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_two_arg_direct_call_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_direct_call_add_imm_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_direct_call_identity_arg_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_folded_two_arg_direct_call_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_repeated_zero_arg_call_compare_zero_return_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_countdown_loop_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_scalar_global_load_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_string_literal_compare_phi_return_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_local_buffer_string_copy_printf_on_native_path);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_counted_printf_ternary_loop_on_native_path);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_source_like_local_buffer_string_copy_printf_on_native_path);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_source_like_repeated_printf_immediates_on_native_path);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_extern_scalar_global_load_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_global_char_pointer_diff_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_global_int_pointer_diff_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_scalar_global_store_reload_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_zero_initialized_scalar_global_store_reload_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_global_two_field_struct_store_sub_sub_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_countdown_do_while_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_double_countdown_guarded_zero_return_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_dual_identity_direct_call_sub_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_call_crossing_direct_call_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_conditional_return_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_conditional_phi_join_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_signed_i16_local_slot_increment_compare_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_local_i32_store_load_sub_return_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_local_i32_pointer_store_zero_load_return_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_local_i32_pointer_gep_zero_load_return_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_local_i32_pointer_gep_zero_store_slot_load_return_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_local_i32_array_two_slot_sum_sub_three_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_local_i32_array_second_slot_pointer_store_zero_load_return_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_local_i32_array_pointer_inc_dec_compare_zero_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_local_two_field_struct_sub_sub_two_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_local_struct_pointer_alias_add_sub_three_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_local_self_referential_struct_pointer_chain_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_double_indirect_local_pointer_chain_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_double_indirect_local_store_one_final_branch_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_u8_select_post_join_add_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_return_add_smoke_case_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_return_sub_smoke_case_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_zero_param_staged_constant_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_single_param_chain_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_two_param_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_input_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_staged_affine_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_two_param_add_sub_chain_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_two_param_staged_affine_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_sext_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_zext_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_trunc_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_select_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_single_param_select_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_two_param_select_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_post_join_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_post_join_add_sub_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_post_join_add_sub_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_mixed_affine_post_join_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_mixed_affine_post_join_add_sub_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_mixed_affine_post_join_add_sub_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_deeper_then_mixed_affine_post_join_add_sub_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_deeper_affine_on_both_sides_post_join_add_sub_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_u8_select_mixed_then_deeper_affine_post_join_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_i32_select_mixed_then_deeper_affine_post_join_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_i32_select_mixed_then_deeper_affine_post_join_add_sub_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_i32_select_mixed_then_deeper_affine_post_join_add_sub_add_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_rejects_unsupported_direct_bir_input_on_x86);
  RUN_TEST(test_x86_direct_lir_emitter_rejects_unsupported_input_without_legacy_backend_ir_stub);
  RUN_TEST(test_x86_direct_lir_emitter_rejects_alloca_backed_conditional_phi_constant_fold_stub);
  RUN_TEST(test_x86_direct_emitter_routes_goto_only_constant_return_through_shared_bir);
  RUN_TEST(test_x86_direct_emitter_lowers_minimal_local_temp_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_constant_add_mul_sub_return_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_constant_div_sub_return_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_local_i32_arithmetic_chain_return_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_two_local_i32_zero_init_return_first_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_local_i32_array_two_slot_sum_sub_three_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_local_i32_array_second_slot_pointer_store_zero_load_return_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_local_i32_array_pointer_inc_dec_compare_zero_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_constant_branch_if_eq_return_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_constant_branch_if_uge_return_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_minimal_param_slot_add_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_minimal_extern_zero_arg_call_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_minimal_local_arg_call_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_minimal_two_arg_helper_call_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_minimal_two_arg_local_arg_call_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_minimal_two_arg_second_local_arg_call_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_minimal_two_arg_second_local_rewrite_call_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_minimal_two_arg_first_local_rewrite_call_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_minimal_two_arg_both_local_arg_call_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_minimal_two_arg_both_local_first_rewrite_call_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_minimal_two_arg_both_local_second_rewrite_call_slice);
  RUN_TEST(test_x86_direct_emitter_lowers_minimal_two_arg_both_local_double_rewrite_call_slice);
  RUN_TEST(test_x86_direct_lir_emitter_rejects_double_indirect_pointer_conditional_return_fallback);
}
