#pragma once
#include "backend_test_util.hpp"
// --- Shared make_* fixture functions (from aarch64 tests) ---

inline c4c::codegen::lir::LirModule make_param_slot_runtime_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");

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
  main_entry.insts.push_back(LirCallOp{
      "%t0",
      "i32",
      LirOperand(std::string("@add_one"), LirOperandKind::Global),
      "(i32)",
      "i32 5",
  });
  main_entry.terminator = LirRet{std::string("%t0"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

inline c4c::codegen::lir::LirModule make_param_slot_with_following_alloca_module() {
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
  function.alloca_insts.push_back(LirAllocaOp{"%lv.buf", "i8", "%p.x", 1});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.param.x"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_typed_direct_call_module() {
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

inline c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_module() {
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
  helper_entry.insts.push_back(
      LirBinOp{"%t0", LirBinaryOpcode::Add, "i32", "%p.x", "%p.y"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirCallOp{
      "%t0",
      "i32",
      LirOperand(std::string("@add_pair"), LirOperandKind::Global),
      "(i32, i32)",
      "i32 5, i32 7",
  });
  main_entry.terminator = LirRet{std::string("%t0"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

inline c4c::codegen::lir::LirModule make_typed_direct_call_local_arg_module() {
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

inline c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_local_arg_module() {
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

inline c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_second_local_arg_module() {
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

inline c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_second_local_rewrite_module() {
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

inline c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_first_local_rewrite_module() {
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

inline c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_both_local_arg_module() {
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

inline c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_both_local_first_rewrite_module() {
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

inline c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_both_local_second_rewrite_module() {
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

inline c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_both_local_double_rewrite_module() {
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

inline c4c::codegen::lir::LirModule make_double_printf_runtime_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.string_pool.push_back(LirStringConst{"@.str0", "%f, %f\\0A", 8});

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "double", "", 8});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.y", "double", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"double", "0x4028AE147AE147AE", "%lv.x"});
  entry.insts.push_back(LirStoreOp{"double", "0x404C63D70A3D70A4", "%lv.y"});
  entry.insts.push_back(LirLoadOp{"%t0", "double", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t1", "double", "%lv.y"});
  entry.insts.push_back(
      LirGepOp{"%t2", "[8 x i8]", "@.str0", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCallOp{
      "%t3",
      "i32",
      LirOperand(std::string("@printf"), LirOperandKind::Global),
      "(ptr, ...)",
      "ptr %t2, double %t0, double %t1",
  });
  entry.terminator = LirRet{std::string("0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_printf_vararg_decl_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  module.extern_decls.push_back(LirExternDecl{"printf", "i32"});
  module.string_pool.push_back(LirStringConst{"@.str0", "hello\\0A", 7});

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{"%fmt", "[7 x i8]", "@.str0", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCallOp{"%call0", "i32", "@printf", "(ptr, ...)", "ptr %fmt"});
  entry.terminator = LirRet{std::string("0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_local_array_gep_module() {
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

inline c4c::codegen::lir::LirModule make_param_member_array_runtime_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.Pair = type { [2 x i32] }");

  LirFunction get_second;
  get_second.name = "get_second";
  get_second.signature_text = "define i32 @get_second(%struct.Pair %p.p)\n";
  get_second.entry = LirBlockId{0};
  get_second.alloca_insts.push_back(LirAllocaOp{"%lv.param.p", "%struct.Pair", "", 4});
  get_second.alloca_insts.push_back(LirStoreOp{"%struct.Pair", "%p.p", "%lv.param.p"});

  LirBlock get_second_entry;
  get_second_entry.id = LirBlockId{0};
  get_second_entry.label = "entry";
  get_second_entry.insts.push_back(
      LirGepOp{"%t0", "%struct.Pair", "%lv.param.p", false, {"i32 0", "i32 0"}});
  get_second_entry.insts.push_back(
      LirGepOp{"%t1", "[2 x i32]", "%t0", false, {"i64 0", "i64 0"}});
  get_second_entry.insts.push_back(
      LirCastOp{"%t2", LirCastKind::SExt, "i32", "1", "i64"});
  get_second_entry.insts.push_back(LirGepOp{"%t3", "i32", "%t1", false, {"i64 %t2"}});
  get_second_entry.insts.push_back(LirLoadOp{"%t4", "i32", "%t3"});
  get_second_entry.terminator = LirRet{std::string("%t4"), "i32"};
  get_second.blocks.push_back(std::move(get_second_entry));
  module.functions.push_back(std::move(get_second));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.p", "%struct.Pair", "", 4});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(
      LirGepOp{"%t0", "%struct.Pair", "%lv.p", false, {"i32 0", "i32 0"}});
  main_entry.insts.push_back(
      LirGepOp{"%t1", "[2 x i32]", "%t0", false, {"i64 0", "i64 0"}});
  main_entry.insts.push_back(
      LirCastOp{"%t2", LirCastKind::SExt, "i32", "0", "i64"});
  main_entry.insts.push_back(LirGepOp{"%t3", "i32", "%t1", false, {"i64 %t2"}});
  main_entry.insts.push_back(LirStoreOp{"i32", "4", "%t3"});
  main_entry.insts.push_back(
      LirGepOp{"%t4", "%struct.Pair", "%lv.p", false, {"i32 0", "i32 0"}});
  main_entry.insts.push_back(
      LirGepOp{"%t5", "[2 x i32]", "%t4", false, {"i64 0", "i64 0"}});
  main_entry.insts.push_back(
      LirCastOp{"%t6", LirCastKind::SExt, "i32", "1", "i64"});
  main_entry.insts.push_back(LirGepOp{"%t7", "i32", "%t5", false, {"i64 %t6"}});
  main_entry.insts.push_back(LirStoreOp{"i32", "6", "%t7"});
  main_entry.insts.push_back(LirLoadOp{"%t8", "%struct.Pair", "%lv.p"});
  main_entry.insts.push_back(
      LirCallOp{"%t9", "i32", "@get_second", "(%struct.Pair)", "%struct.Pair %t8"});
  main_entry.terminator = LirRet{std::string("%t9"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));
  module.functions.push_back(std::move(main_fn));

  return module;
}

inline c4c::codegen::lir::LirModule make_nested_param_member_array_runtime_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.Inner = type { [2 x i32] }");
  module.type_decls.push_back("%struct.Outer = type { %struct.Inner }");

  LirFunction get_second;
  get_second.name = "get_second";
  get_second.signature_text = "define i32 @get_second(%struct.Outer %p.o)\n";
  get_second.entry = LirBlockId{0};
  get_second.alloca_insts.push_back(LirAllocaOp{"%lv.param.o", "%struct.Outer", "", 4});
  get_second.alloca_insts.push_back(LirStoreOp{"%struct.Outer", "%p.o", "%lv.param.o"});

  LirBlock get_second_entry;
  get_second_entry.id = LirBlockId{0};
  get_second_entry.label = "entry";
  get_second_entry.insts.push_back(
      LirGepOp{"%t0", "%struct.Outer", "%lv.param.o", false, {"i32 0", "i32 0"}});
  get_second_entry.insts.push_back(
      LirGepOp{"%t1", "%struct.Inner", "%t0", false, {"i32 0", "i32 0"}});
  get_second_entry.insts.push_back(
      LirGepOp{"%t2", "[2 x i32]", "%t1", false, {"i64 0", "i64 0"}});
  get_second_entry.insts.push_back(
      LirCastOp{"%t3", LirCastKind::SExt, "i32", "1", "i64"});
  get_second_entry.insts.push_back(
      LirGepOp{"%t4", "i32", "%t2", false, {"i64 %t3"}});
  get_second_entry.insts.push_back(LirLoadOp{"%t5", "i32", "%t4"});
  get_second_entry.terminator = LirRet{std::string("%t5"), "i32"};
  get_second.blocks.push_back(std::move(get_second_entry));
  module.functions.push_back(std::move(get_second));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.outer", "%struct.Outer", "", 4});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirMemsetOp{"%lv.outer", "0", "8", false});
  main_entry.insts.push_back(
      LirGepOp{"%t0", "%struct.Outer", "%lv.outer", false, {"i32 0", "i32 0"}});
  main_entry.insts.push_back(
      LirGepOp{"%t1", "%struct.Inner", "%t0", false, {"i32 0", "i32 0"}});
  main_entry.insts.push_back(
      LirGepOp{"%t2", "[2 x i32]", "%t1", false, {"i64 0", "i64 0"}});
  main_entry.insts.push_back(
      LirCastOp{"%t3", LirCastKind::SExt, "i32", "0", "i64"});
  main_entry.insts.push_back(
      LirGepOp{"%t4", "i32", "%t2", false, {"i64 %t3"}});
  main_entry.insts.push_back(LirStoreOp{"i32", "4", "%t4"});
  main_entry.insts.push_back(
      LirGepOp{"%t5", "%struct.Outer", "%lv.outer", false, {"i32 0", "i32 0"}});
  main_entry.insts.push_back(
      LirGepOp{"%t6", "%struct.Inner", "%t5", false, {"i32 0", "i32 0"}});
  main_entry.insts.push_back(
      LirGepOp{"%t7", "[2 x i32]", "%t6", false, {"i64 0", "i64 0"}});
  main_entry.insts.push_back(
      LirCastOp{"%t8", LirCastKind::SExt, "i32", "1", "i64"});
  main_entry.insts.push_back(
      LirGepOp{"%t9", "i32", "%t7", false, {"i64 %t8"}});
  main_entry.insts.push_back(LirStoreOp{"i32", "9", "%t9"});
  main_entry.insts.push_back(LirLoadOp{"%t10", "%struct.Outer", "%lv.outer"});
  main_entry.insts.push_back(
      LirCallOp{"%t11", "i32", "@get_second", "(%struct.Outer)", "%struct.Outer %t10"});
  main_entry.terminator = LirRet{std::string("%t11"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));
  module.functions.push_back(std::move(main_fn));

  return module;
}

inline c4c::codegen::lir::LirModule make_nested_member_pointer_array_runtime_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.Inner = type { [2 x i32] }");
  module.type_decls.push_back("%struct.Outer = type { ptr }");

  LirFunction get_second;
  get_second.name = "get_second";
  get_second.signature_text = "define i32 @get_second(ptr %p.o)\n";
  get_second.entry = LirBlockId{0};

  LirBlock get_second_entry;
  get_second_entry.id = LirBlockId{0};
  get_second_entry.label = "entry";
  get_second_entry.insts.push_back(
      LirGepOp{"%t0", "%struct.Outer", "%p.o", false, {"i32 0", "i32 0"}});
  get_second_entry.insts.push_back(LirLoadOp{"%t1", "ptr", "%t0"});
  get_second_entry.insts.push_back(
      LirGepOp{"%t2", "%struct.Inner", "%t1", false, {"i32 0", "i32 0"}});
  get_second_entry.insts.push_back(
      LirGepOp{"%t3", "[2 x i32]", "%t2", false, {"i64 0", "i64 0"}});
  get_second_entry.insts.push_back(
      LirCastOp{"%t4", LirCastKind::SExt, "i32", "1", "i64"});
  get_second_entry.insts.push_back(
      LirGepOp{"%t5", "i32", "%t3", false, {"i64 %t4"}});
  get_second_entry.insts.push_back(LirLoadOp{"%t6", "i32", "%t5"});
  get_second_entry.terminator = LirRet{std::string("%t6"), "i32"};
  get_second.blocks.push_back(std::move(get_second_entry));
  module.functions.push_back(std::move(get_second));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.inner", "%struct.Inner", "", 4});
  main_fn.alloca_insts.push_back(LirAllocaOp{"%lv.outer", "%struct.Outer", "", 8});

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirMemsetOp{"%lv.inner", "0", "8", false});
  main_entry.insts.push_back(
      LirGepOp{"%t0", "%struct.Inner", "%lv.inner", false, {"i32 0", "i32 0"}});
  main_entry.insts.push_back(
      LirGepOp{"%t1", "[2 x i32]", "%t0", false, {"i64 0", "i64 0"}});
  main_entry.insts.push_back(
      LirCastOp{"%t2", LirCastKind::SExt, "i32", "0", "i64"});
  main_entry.insts.push_back(
      LirGepOp{"%t3", "i32", "%t1", false, {"i64 %t2"}});
  main_entry.insts.push_back(LirStoreOp{"i32", "4", "%t3"});
  main_entry.insts.push_back(
      LirGepOp{"%t4", "%struct.Inner", "%lv.inner", false, {"i32 0", "i32 0"}});
  main_entry.insts.push_back(
      LirGepOp{"%t5", "[2 x i32]", "%t4", false, {"i64 0", "i64 0"}});
  main_entry.insts.push_back(
      LirCastOp{"%t6", LirCastKind::SExt, "i32", "1", "i64"});
  main_entry.insts.push_back(
      LirGepOp{"%t7", "i32", "%t5", false, {"i64 %t6"}});
  main_entry.insts.push_back(LirStoreOp{"i32", "8", "%t7"});
  main_entry.insts.push_back(LirMemsetOp{"%lv.outer", "0", "8", false});
  main_entry.insts.push_back(
      LirGepOp{"%t8", "%struct.Outer", "%lv.outer", false, {"i32 0", "i32 0"}});
  main_entry.insts.push_back(LirStoreOp{"ptr", "%lv.inner", "%t8"});
  main_entry.insts.push_back(LirCallOp{"%t9", "i32", "@get_second", "(ptr)", "ptr %lv.outer"});
  main_entry.terminator = LirRet{std::string("%t9"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));
  module.functions.push_back(std::move(main_fn));

  return module;
}

inline c4c::codegen::lir::LirModule make_global_load_module() {
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

inline c4c::codegen::lir::LirModule make_global_store_reload_module() {
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

inline c4c::codegen::lir::LirModule make_string_literal_char_module() {
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
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t2", "i8", "%t0", false, {"i64 %t1"}});
  entry.insts.push_back(LirLoadOp{"%t3", "i8", "%t2"});
  entry.insts.push_back(
      LirCastOp{"%t4", LirCastKind::SExt, "i8", "%t3", "i32"});
  entry.terminator = LirRet{std::string("%t4"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_extern_global_array_load_module() {
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

inline c4c::codegen::lir::LirModule make_global_char_pointer_diff_module() {
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
  entry.insts.push_back(
      LirCastOp{"%t1", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t2", "i8", "%t0", false, {"i64 %t1"}});
  entry.insts.push_back(
      LirGepOp{"%t3", "[2 x i8]", "@g_bytes", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(
      LirCastOp{"%t4", LirCastKind::SExt, "i32", "0", "i64"});
  entry.insts.push_back(LirGepOp{"%t5", "i8", "%t3", false, {"i64 %t4"}});
  entry.insts.push_back(
      LirCastOp{"%t6", LirCastKind::PtrToInt, "ptr", "%t2", "i64"});
  entry.insts.push_back(
      LirCastOp{"%t7", LirCastKind::PtrToInt, "ptr", "%t5", "i64"});
  entry.insts.push_back(LirBinOp{"%t8", "sub", "i64", "%t6", "%t7"});
  entry.insts.push_back(
      LirCastOp{"%t9", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirCmpOp{"%t10", false, "eq", "i64", "%t8", "%t9"});
  entry.insts.push_back(
      LirCastOp{"%t11", LirCastKind::ZExt, "i1", "%t10", "i32"});
  entry.terminator = LirRet{std::string("%t11"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_global_int_pointer_diff_module() {
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
  entry.insts.push_back(
      LirCastOp{"%t1", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t2", "i32", "%t0", false, {"i64 %t1"}});
  entry.insts.push_back(
      LirGepOp{"%t3", "[2 x i32]", "@g_words", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(
      LirCastOp{"%t4", LirCastKind::SExt, "i32", "0", "i64"});
  entry.insts.push_back(LirGepOp{"%t5", "i32", "%t3", false, {"i64 %t4"}});
  entry.insts.push_back(
      LirCastOp{"%t6", LirCastKind::PtrToInt, "ptr", "%t2", "i64"});
  entry.insts.push_back(
      LirCastOp{"%t7", LirCastKind::PtrToInt, "ptr", "%t5", "i64"});
  entry.insts.push_back(LirBinOp{"%t8", "sub", "i64", "%t6", "%t7"});
  entry.insts.push_back(LirBinOp{"%t9", "sdiv", "i64", "%t8", "4"});
  entry.insts.push_back(
      LirCastOp{"%t10", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirCmpOp{"%t11", false, "eq", "i64", "%t9", "%t10"});
  entry.insts.push_back(
      LirCastOp{"%t12", LirCastKind::ZExt, "i1", "%t11", "i32"});
  entry.terminator = LirRet{std::string("%t12"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_global_int_pointer_roundtrip_module() {
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
  function.alloca_insts.push_back(LirAllocaOp{"%lv.addr", "i64", "", 8});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.p", "ptr", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirCastOp{"%t0", LirCastKind::PtrToInt, "ptr", "@g_value", "i64"});
  entry.insts.push_back(LirStoreOp{"i64", "%t0", "%lv.addr"});
  entry.insts.push_back(LirLoadOp{"%t1", "i64", "%lv.addr"});
  entry.insts.push_back(
      LirCastOp{"%t2", LirCastKind::IntToPtr, "i64", "%t1", "ptr"});
  entry.insts.push_back(LirStoreOp{"ptr", "%t2", "%lv.p"});
  entry.insts.push_back(LirLoadOp{"%t3", "ptr", "%lv.p"});
  entry.insts.push_back(LirLoadOp{"%t4", "i32", "%t3"});
  entry.terminator = LirRet{std::string("%t4"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_dead_param_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction decl;
  decl.name = "helper";
  decl.signature_text = "declare i32 @helper(i32)\n";
  decl.is_declaration = true;

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main(i32 %p.x)\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.param.x", "i32", "", 4});
  function.alloca_insts.push_back(LirStoreOp{"i32", "%p.x", "%lv.param.x"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@helper", "", "i32 %p.x"});
  entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%p.x", "%t0"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(decl));
  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_live_param_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

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
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_dead_local_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.buf", "[2 x i32]", "", 4});
  function.alloca_insts.push_back(
      LirStoreOp{"[2 x i32]", "zeroinitializer", "%lv.buf"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.terminator = LirRet{std::string("0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_live_local_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.buf", "[2 x i32]", "", 4});
  function.alloca_insts.push_back(
      LirStoreOp{"[2 x i32]", "zeroinitializer", "%lv.buf"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "[2 x i32]", "%lv.buf", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirStoreOp{"i32", "7", "%t0"});
  entry.insts.push_back(LirLoadOp{"%t1", "i32", "%t0"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_disjoint_block_local_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main(i1 %p.cond)\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.then", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.else", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.terminator = LirCondBr{"%p.cond", "then", "else"};
  function.blocks.push_back(std::move(entry));

  LirBlock then_block;
  then_block.id = LirBlockId{1};
  then_block.label = "then";
  then_block.insts.push_back(LirStoreOp{"i32", "7", "%lv.then"});
  then_block.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.then"});
  then_block.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(then_block));

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.insts.push_back(LirStoreOp{"i32", "9", "%lv.else"});
  else_block.insts.push_back(LirLoadOp{"%t1", "i32", "%lv.else"});
  else_block.terminator = LirRet{std::string("%t1"), "i32"};
  function.blocks.push_back(std::move(else_block));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_same_block_local_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "5", "%lv.x"});
  entry.insts.push_back(LirStoreOp{"i32", "6", "%lv.y"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t1", "i32", "%lv.y"});
  entry.insts.push_back(LirBinOp{"%t2", "add", "i32", "%t0", "%t1"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_mixed_lifetime_local_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.a", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.y", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "12", "%lv.a"});
  entry.insts.push_back(LirStoreOp{"i32", "34", "%lv.y"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.a"});
  entry.insts.push_back(LirCmpOp{"%t1", false, "ne", "i32", "%t0", "0"});
  entry.terminator = LirCondBr{"%t1", "then", "else"};

  LirBlock then_block;
  then_block.id = LirBlockId{1};
  then_block.label = "then";
  then_block.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.a"});
  then_block.terminator = LirRet{std::string("%t2"), "i32"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_read_before_store_local_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.buf", "[2 x i32]", "", 4});
  function.alloca_insts.push_back(
      LirStoreOp{"[2 x i32]", "zeroinitializer", "%lv.buf"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "[2 x i32]", "%lv.buf", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirLoadOp{"%t1", "i32", "%t0"});
  entry.insts.push_back(LirStoreOp{"i32", "7", "%t0"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_overwrite_first_scalar_local_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  function.alloca_insts.push_back(LirStoreOp{"i32", "zeroinitializer", "%lv.x"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.x"});
  entry.terminator = LirBr{"exit"};
  function.blocks.push_back(std::move(entry));

  LirBlock exit;
  exit.id = LirBlockId{1};
  exit.label = "exit";
  exit.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  exit.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(exit));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_read_before_store_scalar_local_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  function.alloca_insts.push_back(LirStoreOp{"i32", "zeroinitializer", "%lv.x"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.insts.push_back(LirStoreOp{"i32", "7", "%lv.x"});
  entry.terminator = LirBr{"exit"};
  function.blocks.push_back(std::move(entry));

  LirBlock exit;
  exit.id = LirBlockId{1};
  exit.label = "exit";
  exit.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(exit));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_escaped_local_alloca_candidate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction decl;
  decl.name = "sink";
  decl.signature_text = "declare void @sink(ptr)\n";
  decl.is_declaration = true;

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.buf", "[2 x i32]", "", 4});
  function.alloca_insts.push_back(
      LirStoreOp{"[2 x i32]", "zeroinitializer", "%lv.buf"});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"", "void", "@sink", "", "ptr %lv.buf"});
  entry.terminator = LirRet{std::string("0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(decl));
  module.functions.push_back(std::move(function));
  return module;
}


// --- Shared make_* fixture functions (from x86 tests) ---

inline c4c::codegen::lir::LirModule make_double_indirect_local_pointer_conditional_return_module() {
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

  LirBlock block_8;
  block_8.id = LirBlockId{8};
  block_8.label = "block_8";
  block_8.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  function.blocks.push_back(std::move(block_3));
  function.blocks.push_back(std::move(block_4));
  function.blocks.push_back(std::move(block_5));
  function.blocks.push_back(std::move(block_6));
  function.blocks.push_back(std::move(block_7));
  function.blocks.push_back(std::move(block_8));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_goto_only_constant_return_module() {
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
  entry.terminator = LirBr{"ulbl_start"};

  LirBlock start;
  start.id = LirBlockId{1};
  start.label = "ulbl_start";
  start.terminator = LirBr{"block_1"};

  LirBlock block_1;
  block_1.id = LirBlockId{2};
  block_1.label = "block_1";
  block_1.terminator = LirBr{"ulbl_next"};

  LirBlock success;
  success.id = LirBlockId{3};
  success.label = "ulbl_success";
  success.terminator = LirBr{"block_2"};

  LirBlock block_2;
  block_2.id = LirBlockId{4};
  block_2.label = "block_2";
  block_2.terminator = LirRet{std::string("0"), "i32"};

  LirBlock next;
  next.id = LirBlockId{5};
  next.label = "ulbl_next";
  next.terminator = LirBr{"block_3"};

  LirBlock block_3;
  block_3.id = LirBlockId{6};
  block_3.label = "block_3";
  block_3.terminator = LirBr{"ulbl_foo"};

  LirBlock foo;
  foo.id = LirBlockId{7};
  foo.label = "ulbl_foo";
  foo.terminator = LirBr{"block_4"};

  LirBlock block_4;
  block_4.id = LirBlockId{8};
  block_4.label = "block_4";
  block_4.terminator = LirBr{"block_2"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(start));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(success));
  function.blocks.push_back(std::move(block_2));
  function.blocks.push_back(std::move(next));
  function.blocks.push_back(std::move(block_3));
  function.blocks.push_back(std::move(foo));
  function.blocks.push_back(std::move(block_4));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_constant_conditional_goto_return_module() {
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
  entry.terminator = LirCondBr{"%t0", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{1};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{2};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{3};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{4};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_i64_constant_conditional_goto_return_module() {
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
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i64", "9", "9"});
  entry.terminator = LirCondBr{"%t0", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{1};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{2};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{3};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{4};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_mixed_cast_constant_conditional_goto_return_module() {
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
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "7", "7"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::SExt, "i32", "%t1", "i64"});
  entry.insts.push_back(LirCmpOp{"%t3", false, "ne", "i64", "%t2", "0"});
  entry.terminator = LirCondBr{"%t3", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{1};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{2};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{3};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{4};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_small_integer_cast_constant_conditional_goto_return_module() {
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
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "7", "7"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i8"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::SExt, "i8", "%t1", "i32"});
  entry.insts.push_back(LirCastOp{"%t3", LirCastKind::SExt, "i32", "%t2", "i64"});
  entry.insts.push_back(LirCmpOp{"%t4", false, "ne", "i64", "%t3", "0"});
  entry.terminator = LirCondBr{"%t4", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{1};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{2};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{3};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{4};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_truncating_binop_constant_conditional_goto_return_module() {
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
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "7", "7"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirBinOp{"%t2", LirBinaryOpcode::Shl, "i32", "%t1", "8"});
  entry.insts.push_back(LirCastOp{"%t3", LirCastKind::Trunc, "i32", "%t2", "i8"});
  entry.insts.push_back(LirBinOp{"%t4", LirBinaryOpcode::Xor, "i8", "%t3", "1"});
  entry.insts.push_back(LirCmpOp{"%t5", false, "ne", "i8", "%t4", "0"});
  entry.terminator = LirCondBr{"%t5", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{1};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{2};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{3};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{4};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_select_constant_conditional_goto_return_module() {
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
  entry.insts.push_back(LirSelectOp{"%t1", "i32", "%t0", "7", "0"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{1};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{2};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{3};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{4};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_local_slot_constant_conditional_goto_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.flag", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "9", "9"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.flag"});
  entry.terminator = LirBr{"check"};

  LirBlock check;
  check.id = LirBlockId{1};
  check.label = "check";
  check.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.flag"});
  check.insts.push_back(LirCmpOp{"%t3", false, "ne", "i32", "%t2", "0"});
  check.terminator = LirCondBr{"%t3", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{2};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{3};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{4};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{5};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(check));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_i8_local_slot_constant_conditional_goto_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.flag", "i8", "", 1});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "9", "9"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i8"});
  entry.insts.push_back(LirStoreOp{"i8", "%t1", "%lv.flag"});
  entry.terminator = LirBr{"check"};

  LirBlock check;
  check.id = LirBlockId{1};
  check.label = "check";
  check.insts.push_back(LirLoadOp{"%t2", "i8", "%lv.flag"});
  check.insts.push_back(LirCmpOp{"%t3", false, "ne", "i8", "%t2", "0"});
  check.terminator = LirCondBr{"%t3", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{2};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{3};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{4};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{5};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(check));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_two_local_slot_constant_conditional_goto_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.state", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.flag", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.state"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.state"});
  entry.insts.push_back(LirCmpOp{"%t1", false, "eq", "i32", "%t0", "0"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::ZExt, "i1", "%t1", "i32"});
  entry.insts.push_back(LirStoreOp{"i32", "%t2", "%lv.flag"});
  entry.terminator = LirBr{"check"};

  LirBlock check;
  check.id = LirBlockId{1};
  check.label = "check";
  check.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.flag"});
  check.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  check.terminator = LirCondBr{"%t4", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{2};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{3};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{4};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{5};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(check));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_three_local_slot_constant_conditional_goto_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.state", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.flag", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.state"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.state"});
  entry.insts.push_back(LirCmpOp{"%t1", false, "eq", "i32", "%t0", "0"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::ZExt, "i1", "%t1", "i32"});
  entry.insts.push_back(LirStoreOp{"i32", "%t2", "%lv.tmp"});
  entry.terminator = LirBr{"check"};

  LirBlock check;
  check.id = LirBlockId{1};
  check.label = "check";
  check.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.tmp"});
  check.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  check.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i1", "%t4", "i32"});
  check.insts.push_back(LirStoreOp{"i32", "%t5", "%lv.flag"});
  check.terminator = LirBr{"branch"};

  LirBlock branch;
  branch.id = LirBlockId{2};
  branch.label = "branch";
  branch.insts.push_back(LirLoadOp{"%t6", "i32", "%lv.flag"});
  branch.insts.push_back(LirCmpOp{"%t7", false, "ne", "i32", "%t6", "0"});
  branch.terminator = LirCondBr{"%t7", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{3};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{4};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{5};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{6};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(check));
  function.blocks.push_back(std::move(branch));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_four_local_slot_constant_conditional_goto_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.state", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp0", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp1", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.flag", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.state"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.state"});
  entry.insts.push_back(LirCmpOp{"%t1", false, "eq", "i32", "%t0", "0"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::ZExt, "i1", "%t1", "i32"});
  entry.insts.push_back(LirStoreOp{"i32", "%t2", "%lv.tmp0"});
  entry.terminator = LirBr{"check"};

  LirBlock check;
  check.id = LirBlockId{1};
  check.label = "check";
  check.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.tmp0"});
  check.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  check.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i1", "%t4", "i32"});
  check.insts.push_back(LirStoreOp{"i32", "%t5", "%lv.tmp1"});
  check.terminator = LirBr{"branch"};

  LirBlock branch;
  branch.id = LirBlockId{2};
  branch.label = "branch";
  branch.insts.push_back(LirLoadOp{"%t6", "i32", "%lv.tmp1"});
  branch.insts.push_back(LirCmpOp{"%t7", false, "ne", "i32", "%t6", "0"});
  branch.insts.push_back(LirCastOp{"%t8", LirCastKind::ZExt, "i1", "%t7", "i32"});
  branch.insts.push_back(LirStoreOp{"i32", "%t8", "%lv.flag"});
  branch.terminator = LirBr{"finish_check"};

  LirBlock finish_check;
  finish_check.id = LirBlockId{3};
  finish_check.label = "finish_check";
  finish_check.insts.push_back(LirLoadOp{"%t9", "i32", "%lv.flag"});
  finish_check.insts.push_back(LirCmpOp{"%t10", false, "ne", "i32", "%t9", "0"});
  finish_check.terminator = LirCondBr{"%t10", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{4};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{5};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{6};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{7};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(check));
  function.blocks.push_back(std::move(branch));
  function.blocks.push_back(std::move(finish_check));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_five_local_slot_constant_conditional_goto_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.state", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp0", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp1", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp2", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.flag", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.state"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.state"});
  entry.insts.push_back(LirCmpOp{"%t1", false, "eq", "i32", "%t0", "0"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::ZExt, "i1", "%t1", "i32"});
  entry.insts.push_back(LirStoreOp{"i32", "%t2", "%lv.tmp0"});
  entry.terminator = LirBr{"check"};

  LirBlock check;
  check.id = LirBlockId{1};
  check.label = "check";
  check.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.tmp0"});
  check.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  check.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i1", "%t4", "i32"});
  check.insts.push_back(LirStoreOp{"i32", "%t5", "%lv.tmp1"});
  check.terminator = LirBr{"branch"};

  LirBlock branch;
  branch.id = LirBlockId{2};
  branch.label = "branch";
  branch.insts.push_back(LirLoadOp{"%t6", "i32", "%lv.tmp1"});
  branch.insts.push_back(LirCmpOp{"%t7", false, "ne", "i32", "%t6", "0"});
  branch.insts.push_back(LirCastOp{"%t8", LirCastKind::ZExt, "i1", "%t7", "i32"});
  branch.insts.push_back(LirStoreOp{"i32", "%t8", "%lv.tmp2"});
  branch.terminator = LirBr{"finish_check"};

  LirBlock finish_check;
  finish_check.id = LirBlockId{3};
  finish_check.label = "finish_check";
  finish_check.insts.push_back(LirLoadOp{"%t9", "i32", "%lv.tmp2"});
  finish_check.insts.push_back(LirCmpOp{"%t10", false, "ne", "i32", "%t9", "0"});
  finish_check.insts.push_back(LirCastOp{"%t11", LirCastKind::ZExt, "i1", "%t10", "i32"});
  finish_check.insts.push_back(LirStoreOp{"i32", "%t11", "%lv.flag"});
  finish_check.terminator = LirBr{"flag_check"};

  LirBlock flag_check;
  flag_check.id = LirBlockId{4};
  flag_check.label = "flag_check";
  flag_check.insts.push_back(LirLoadOp{"%t12", "i32", "%lv.flag"});
  flag_check.insts.push_back(LirCmpOp{"%t13", false, "ne", "i32", "%t12", "0"});
  flag_check.terminator = LirCondBr{"%t13", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{5};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{6};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{7};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{8};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(check));
  function.blocks.push_back(std::move(branch));
  function.blocks.push_back(std::move(finish_check));
  function.blocks.push_back(std::move(flag_check));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_six_local_slot_constant_conditional_goto_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.state", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp0", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp1", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp2", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp3", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.flag", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.state"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.state"});
  entry.insts.push_back(LirCmpOp{"%t1", false, "eq", "i32", "%t0", "0"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::ZExt, "i1", "%t1", "i32"});
  entry.insts.push_back(LirStoreOp{"i32", "%t2", "%lv.tmp0"});
  entry.terminator = LirBr{"check"};

  LirBlock check;
  check.id = LirBlockId{1};
  check.label = "check";
  check.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.tmp0"});
  check.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  check.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i1", "%t4", "i32"});
  check.insts.push_back(LirStoreOp{"i32", "%t5", "%lv.tmp1"});
  check.terminator = LirBr{"branch"};

  LirBlock branch;
  branch.id = LirBlockId{2};
  branch.label = "branch";
  branch.insts.push_back(LirLoadOp{"%t6", "i32", "%lv.tmp1"});
  branch.insts.push_back(LirCmpOp{"%t7", false, "ne", "i32", "%t6", "0"});
  branch.insts.push_back(LirCastOp{"%t8", LirCastKind::ZExt, "i1", "%t7", "i32"});
  branch.insts.push_back(LirStoreOp{"i32", "%t8", "%lv.tmp2"});
  branch.terminator = LirBr{"finish_check"};

  LirBlock finish_check;
  finish_check.id = LirBlockId{3};
  finish_check.label = "finish_check";
  finish_check.insts.push_back(LirLoadOp{"%t9", "i32", "%lv.tmp2"});
  finish_check.insts.push_back(LirCmpOp{"%t10", false, "ne", "i32", "%t9", "0"});
  finish_check.insts.push_back(LirCastOp{"%t11", LirCastKind::ZExt, "i1", "%t10", "i32"});
  finish_check.insts.push_back(LirStoreOp{"i32", "%t11", "%lv.tmp3"});
  finish_check.terminator = LirBr{"flag_check"};

  LirBlock flag_check;
  flag_check.id = LirBlockId{4};
  flag_check.label = "flag_check";
  flag_check.insts.push_back(LirLoadOp{"%t12", "i32", "%lv.tmp3"});
  flag_check.insts.push_back(LirCmpOp{"%t13", false, "ne", "i32", "%t12", "0"});
  flag_check.insts.push_back(LirCastOp{"%t14", LirCastKind::ZExt, "i1", "%t13", "i32"});
  flag_check.insts.push_back(LirStoreOp{"i32", "%t14", "%lv.flag"});
  flag_check.terminator = LirBr{"return_check"};

  LirBlock return_check;
  return_check.id = LirBlockId{5};
  return_check.label = "return_check";
  return_check.insts.push_back(LirLoadOp{"%t15", "i32", "%lv.flag"});
  return_check.insts.push_back(LirCmpOp{"%t16", false, "ne", "i32", "%t15", "0"});
  return_check.terminator = LirCondBr{"%t16", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{6};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{7};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{8};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{9};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(check));
  function.blocks.push_back(std::move(branch));
  function.blocks.push_back(std::move(finish_check));
  function.blocks.push_back(std::move(flag_check));
  function.blocks.push_back(std::move(return_check));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_seven_local_slot_constant_conditional_goto_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.state", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp0", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp1", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp2", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp3", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp4", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.flag", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.state"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.state"});
  entry.insts.push_back(LirCmpOp{"%t1", false, "eq", "i32", "%t0", "0"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::ZExt, "i1", "%t1", "i32"});
  entry.insts.push_back(LirStoreOp{"i32", "%t2", "%lv.tmp0"});
  entry.terminator = LirBr{"check"};

  LirBlock check;
  check.id = LirBlockId{1};
  check.label = "check";
  check.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.tmp0"});
  check.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  check.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i1", "%t4", "i32"});
  check.insts.push_back(LirStoreOp{"i32", "%t5", "%lv.tmp1"});
  check.terminator = LirBr{"branch"};

  LirBlock branch;
  branch.id = LirBlockId{2};
  branch.label = "branch";
  branch.insts.push_back(LirLoadOp{"%t6", "i32", "%lv.tmp1"});
  branch.insts.push_back(LirCmpOp{"%t7", false, "ne", "i32", "%t6", "0"});
  branch.insts.push_back(LirCastOp{"%t8", LirCastKind::ZExt, "i1", "%t7", "i32"});
  branch.insts.push_back(LirStoreOp{"i32", "%t8", "%lv.tmp2"});
  branch.terminator = LirBr{"finish_check"};

  LirBlock finish_check;
  finish_check.id = LirBlockId{3};
  finish_check.label = "finish_check";
  finish_check.insts.push_back(LirLoadOp{"%t9", "i32", "%lv.tmp2"});
  finish_check.insts.push_back(LirCmpOp{"%t10", false, "ne", "i32", "%t9", "0"});
  finish_check.insts.push_back(LirCastOp{"%t11", LirCastKind::ZExt, "i1", "%t10", "i32"});
  finish_check.insts.push_back(LirStoreOp{"i32", "%t11", "%lv.tmp3"});
  finish_check.terminator = LirBr{"flag_prep"};

  LirBlock flag_prep;
  flag_prep.id = LirBlockId{4};
  flag_prep.label = "flag_prep";
  flag_prep.insts.push_back(LirLoadOp{"%t12", "i32", "%lv.tmp3"});
  flag_prep.insts.push_back(LirCmpOp{"%t13", false, "ne", "i32", "%t12", "0"});
  flag_prep.insts.push_back(LirCastOp{"%t14", LirCastKind::ZExt, "i1", "%t13", "i32"});
  flag_prep.insts.push_back(LirStoreOp{"i32", "%t14", "%lv.tmp4"});
  flag_prep.terminator = LirBr{"flag_check"};

  LirBlock flag_check;
  flag_check.id = LirBlockId{5};
  flag_check.label = "flag_check";
  flag_check.insts.push_back(LirLoadOp{"%t15", "i32", "%lv.tmp4"});
  flag_check.insts.push_back(LirCmpOp{"%t16", false, "ne", "i32", "%t15", "0"});
  flag_check.insts.push_back(LirCastOp{"%t17", LirCastKind::ZExt, "i1", "%t16", "i32"});
  flag_check.insts.push_back(LirStoreOp{"i32", "%t17", "%lv.flag"});
  flag_check.terminator = LirBr{"branch_check"};

  LirBlock branch_check;
  branch_check.id = LirBlockId{6};
  branch_check.label = "branch_check";
  branch_check.insts.push_back(LirLoadOp{"%t18", "i32", "%lv.flag"});
  branch_check.insts.push_back(LirCmpOp{"%t19", false, "ne", "i32", "%t18", "0"});
  branch_check.terminator = LirCondBr{"%t19", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{7};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{8};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{9};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{10};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(check));
  function.blocks.push_back(std::move(branch));
  function.blocks.push_back(std::move(finish_check));
  function.blocks.push_back(std::move(flag_prep));
  function.blocks.push_back(std::move(flag_check));
  function.blocks.push_back(std::move(branch_check));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_eight_local_slot_constant_conditional_goto_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.state", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp0", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp1", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp2", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp3", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp4", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp5", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.flag", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.state"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.state"});
  entry.insts.push_back(LirCmpOp{"%t1", false, "eq", "i32", "%t0", "0"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::ZExt, "i1", "%t1", "i32"});
  entry.insts.push_back(LirStoreOp{"i32", "%t2", "%lv.tmp0"});
  entry.terminator = LirBr{"check"};

  LirBlock check;
  check.id = LirBlockId{1};
  check.label = "check";
  check.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.tmp0"});
  check.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  check.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i1", "%t4", "i32"});
  check.insts.push_back(LirStoreOp{"i32", "%t5", "%lv.tmp1"});
  check.terminator = LirBr{"branch"};

  LirBlock branch;
  branch.id = LirBlockId{2};
  branch.label = "branch";
  branch.insts.push_back(LirLoadOp{"%t6", "i32", "%lv.tmp1"});
  branch.insts.push_back(LirCmpOp{"%t7", false, "ne", "i32", "%t6", "0"});
  branch.insts.push_back(LirCastOp{"%t8", LirCastKind::ZExt, "i1", "%t7", "i32"});
  branch.insts.push_back(LirStoreOp{"i32", "%t8", "%lv.tmp2"});
  branch.terminator = LirBr{"finish_check"};

  LirBlock finish_check;
  finish_check.id = LirBlockId{3};
  finish_check.label = "finish_check";
  finish_check.insts.push_back(LirLoadOp{"%t9", "i32", "%lv.tmp2"});
  finish_check.insts.push_back(LirCmpOp{"%t10", false, "ne", "i32", "%t9", "0"});
  finish_check.insts.push_back(LirCastOp{"%t11", LirCastKind::ZExt, "i1", "%t10", "i32"});
  finish_check.insts.push_back(LirStoreOp{"i32", "%t11", "%lv.tmp3"});
  finish_check.terminator = LirBr{"flag_prep"};

  LirBlock flag_prep;
  flag_prep.id = LirBlockId{4};
  flag_prep.label = "flag_prep";
  flag_prep.insts.push_back(LirLoadOp{"%t12", "i32", "%lv.tmp3"});
  flag_prep.insts.push_back(LirCmpOp{"%t13", false, "ne", "i32", "%t12", "0"});
  flag_prep.insts.push_back(LirCastOp{"%t14", LirCastKind::ZExt, "i1", "%t13", "i32"});
  flag_prep.insts.push_back(LirStoreOp{"i32", "%t14", "%lv.tmp4"});
  flag_prep.terminator = LirBr{"flag_check"};

  LirBlock flag_check;
  flag_check.id = LirBlockId{5};
  flag_check.label = "flag_check";
  flag_check.insts.push_back(LirLoadOp{"%t15", "i32", "%lv.tmp4"});
  flag_check.insts.push_back(LirCmpOp{"%t16", false, "ne", "i32", "%t15", "0"});
  flag_check.insts.push_back(LirCastOp{"%t17", LirCastKind::ZExt, "i1", "%t16", "i32"});
  flag_check.insts.push_back(LirStoreOp{"i32", "%t17", "%lv.tmp5"});
  flag_check.terminator = LirBr{"branch_check"};

  LirBlock branch_check;
  branch_check.id = LirBlockId{6};
  branch_check.label = "branch_check";
  branch_check.insts.push_back(LirLoadOp{"%t18", "i32", "%lv.tmp5"});
  branch_check.insts.push_back(LirCmpOp{"%t19", false, "ne", "i32", "%t18", "0"});
  branch_check.insts.push_back(LirCastOp{"%t20", LirCastKind::ZExt, "i1", "%t19", "i32"});
  branch_check.insts.push_back(LirStoreOp{"i32", "%t20", "%lv.flag"});
  branch_check.terminator = LirBr{"final_check"};

  LirBlock final_check;
  final_check.id = LirBlockId{7};
  final_check.label = "final_check";
  final_check.insts.push_back(LirLoadOp{"%t21", "i32", "%lv.flag"});
  final_check.insts.push_back(LirCmpOp{"%t22", false, "ne", "i32", "%t21", "0"});
  final_check.terminator = LirCondBr{"%t22", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{8};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{9};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{10};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{11};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(check));
  function.blocks.push_back(std::move(branch));
  function.blocks.push_back(std::move(finish_check));
  function.blocks.push_back(std::move(flag_prep));
  function.blocks.push_back(std::move(flag_check));
  function.blocks.push_back(std::move(branch_check));
  function.blocks.push_back(std::move(final_check));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_nine_local_slot_constant_conditional_goto_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.state", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp0", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp1", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp2", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp3", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp4", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp5", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp6", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.flag", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.state"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.state"});
  entry.insts.push_back(LirCmpOp{"%t1", false, "eq", "i32", "%t0", "0"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::ZExt, "i1", "%t1", "i32"});
  entry.insts.push_back(LirStoreOp{"i32", "%t2", "%lv.tmp0"});
  entry.terminator = LirBr{"check"};

  LirBlock check;
  check.id = LirBlockId{1};
  check.label = "check";
  check.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.tmp0"});
  check.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  check.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i1", "%t4", "i32"});
  check.insts.push_back(LirStoreOp{"i32", "%t5", "%lv.tmp1"});
  check.terminator = LirBr{"branch"};

  LirBlock branch;
  branch.id = LirBlockId{2};
  branch.label = "branch";
  branch.insts.push_back(LirLoadOp{"%t6", "i32", "%lv.tmp1"});
  branch.insts.push_back(LirCmpOp{"%t7", false, "ne", "i32", "%t6", "0"});
  branch.insts.push_back(LirCastOp{"%t8", LirCastKind::ZExt, "i1", "%t7", "i32"});
  branch.insts.push_back(LirStoreOp{"i32", "%t8", "%lv.tmp2"});
  branch.terminator = LirBr{"finish_check"};

  LirBlock finish_check;
  finish_check.id = LirBlockId{3};
  finish_check.label = "finish_check";
  finish_check.insts.push_back(LirLoadOp{"%t9", "i32", "%lv.tmp2"});
  finish_check.insts.push_back(LirCmpOp{"%t10", false, "ne", "i32", "%t9", "0"});
  finish_check.insts.push_back(LirCastOp{"%t11", LirCastKind::ZExt, "i1", "%t10", "i32"});
  finish_check.insts.push_back(LirStoreOp{"i32", "%t11", "%lv.tmp3"});
  finish_check.terminator = LirBr{"flag_prep"};

  LirBlock flag_prep;
  flag_prep.id = LirBlockId{4};
  flag_prep.label = "flag_prep";
  flag_prep.insts.push_back(LirLoadOp{"%t12", "i32", "%lv.tmp3"});
  flag_prep.insts.push_back(LirCmpOp{"%t13", false, "ne", "i32", "%t12", "0"});
  flag_prep.insts.push_back(LirCastOp{"%t14", LirCastKind::ZExt, "i1", "%t13", "i32"});
  flag_prep.insts.push_back(LirStoreOp{"i32", "%t14", "%lv.tmp4"});
  flag_prep.terminator = LirBr{"flag_check"};

  LirBlock flag_check;
  flag_check.id = LirBlockId{5};
  flag_check.label = "flag_check";
  flag_check.insts.push_back(LirLoadOp{"%t15", "i32", "%lv.tmp4"});
  flag_check.insts.push_back(LirCmpOp{"%t16", false, "ne", "i32", "%t15", "0"});
  flag_check.insts.push_back(LirCastOp{"%t17", LirCastKind::ZExt, "i1", "%t16", "i32"});
  flag_check.insts.push_back(LirStoreOp{"i32", "%t17", "%lv.tmp5"});
  flag_check.terminator = LirBr{"branch_check"};

  LirBlock branch_check;
  branch_check.id = LirBlockId{6};
  branch_check.label = "branch_check";
  branch_check.insts.push_back(LirLoadOp{"%t18", "i32", "%lv.tmp5"});
  branch_check.insts.push_back(LirCmpOp{"%t19", false, "ne", "i32", "%t18", "0"});
  branch_check.insts.push_back(LirCastOp{"%t20", LirCastKind::ZExt, "i1", "%t19", "i32"});
  branch_check.insts.push_back(LirStoreOp{"i32", "%t20", "%lv.tmp6"});
  branch_check.terminator = LirBr{"final_check"};

  LirBlock final_check;
  final_check.id = LirBlockId{7};
  final_check.label = "final_check";
  final_check.insts.push_back(LirLoadOp{"%t21", "i32", "%lv.tmp6"});
  final_check.insts.push_back(LirCmpOp{"%t22", false, "ne", "i32", "%t21", "0"});
  final_check.insts.push_back(LirCastOp{"%t23", LirCastKind::ZExt, "i1", "%t22", "i32"});
  final_check.insts.push_back(LirStoreOp{"i32", "%t23", "%lv.flag"});
  final_check.terminator = LirBr{"decision"};

  LirBlock decision;
  decision.id = LirBlockId{8};
  decision.label = "decision";
  decision.insts.push_back(LirLoadOp{"%t24", "i32", "%lv.flag"});
  decision.insts.push_back(LirCmpOp{"%t25", false, "ne", "i32", "%t24", "0"});
  decision.terminator = LirCondBr{"%t25", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{9};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{10};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{11};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{12};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(check));
  function.blocks.push_back(std::move(branch));
  function.blocks.push_back(std::move(finish_check));
  function.blocks.push_back(std::move(flag_prep));
  function.blocks.push_back(std::move(flag_check));
  function.blocks.push_back(std::move(branch_check));
  function.blocks.push_back(std::move(final_check));
  function.blocks.push_back(std::move(decision));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_ten_local_slot_constant_conditional_goto_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.state", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp0", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp1", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp2", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp3", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp4", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp5", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp6", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp7", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.flag", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.state"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.state"});
  entry.insts.push_back(LirCmpOp{"%t1", false, "eq", "i32", "%t0", "0"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::ZExt, "i1", "%t1", "i32"});
  entry.insts.push_back(LirStoreOp{"i32", "%t2", "%lv.tmp0"});
  entry.terminator = LirBr{"check"};

  LirBlock check;
  check.id = LirBlockId{1};
  check.label = "check";
  check.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.tmp0"});
  check.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  check.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i1", "%t4", "i32"});
  check.insts.push_back(LirStoreOp{"i32", "%t5", "%lv.tmp1"});
  check.terminator = LirBr{"branch"};

  LirBlock branch;
  branch.id = LirBlockId{2};
  branch.label = "branch";
  branch.insts.push_back(LirLoadOp{"%t6", "i32", "%lv.tmp1"});
  branch.insts.push_back(LirCmpOp{"%t7", false, "ne", "i32", "%t6", "0"});
  branch.insts.push_back(LirCastOp{"%t8", LirCastKind::ZExt, "i1", "%t7", "i32"});
  branch.insts.push_back(LirStoreOp{"i32", "%t8", "%lv.tmp2"});
  branch.terminator = LirBr{"finish_check"};

  LirBlock finish_check;
  finish_check.id = LirBlockId{3};
  finish_check.label = "finish_check";
  finish_check.insts.push_back(LirLoadOp{"%t9", "i32", "%lv.tmp2"});
  finish_check.insts.push_back(LirCmpOp{"%t10", false, "ne", "i32", "%t9", "0"});
  finish_check.insts.push_back(LirCastOp{"%t11", LirCastKind::ZExt, "i1", "%t10", "i32"});
  finish_check.insts.push_back(LirStoreOp{"i32", "%t11", "%lv.tmp3"});
  finish_check.terminator = LirBr{"flag_prep"};

  LirBlock flag_prep;
  flag_prep.id = LirBlockId{4};
  flag_prep.label = "flag_prep";
  flag_prep.insts.push_back(LirLoadOp{"%t12", "i32", "%lv.tmp3"});
  flag_prep.insts.push_back(LirCmpOp{"%t13", false, "ne", "i32", "%t12", "0"});
  flag_prep.insts.push_back(LirCastOp{"%t14", LirCastKind::ZExt, "i1", "%t13", "i32"});
  flag_prep.insts.push_back(LirStoreOp{"i32", "%t14", "%lv.tmp4"});
  flag_prep.terminator = LirBr{"flag_check"};

  LirBlock flag_check;
  flag_check.id = LirBlockId{5};
  flag_check.label = "flag_check";
  flag_check.insts.push_back(LirLoadOp{"%t15", "i32", "%lv.tmp4"});
  flag_check.insts.push_back(LirCmpOp{"%t16", false, "ne", "i32", "%t15", "0"});
  flag_check.insts.push_back(LirCastOp{"%t17", LirCastKind::ZExt, "i1", "%t16", "i32"});
  flag_check.insts.push_back(LirStoreOp{"i32", "%t17", "%lv.tmp5"});
  flag_check.terminator = LirBr{"branch_check"};

  LirBlock branch_check;
  branch_check.id = LirBlockId{6};
  branch_check.label = "branch_check";
  branch_check.insts.push_back(LirLoadOp{"%t18", "i32", "%lv.tmp5"});
  branch_check.insts.push_back(LirCmpOp{"%t19", false, "ne", "i32", "%t18", "0"});
  branch_check.insts.push_back(LirCastOp{"%t20", LirCastKind::ZExt, "i1", "%t19", "i32"});
  branch_check.insts.push_back(LirStoreOp{"i32", "%t20", "%lv.tmp6"});
  branch_check.terminator = LirBr{"final_check"};

  LirBlock final_check;
  final_check.id = LirBlockId{7};
  final_check.label = "final_check";
  final_check.insts.push_back(LirLoadOp{"%t21", "i32", "%lv.tmp6"});
  final_check.insts.push_back(LirCmpOp{"%t22", false, "ne", "i32", "%t21", "0"});
  final_check.insts.push_back(LirCastOp{"%t23", LirCastKind::ZExt, "i1", "%t22", "i32"});
  final_check.insts.push_back(LirStoreOp{"i32", "%t23", "%lv.tmp7"});
  final_check.terminator = LirBr{"decision_prep"};

  LirBlock decision_prep;
  decision_prep.id = LirBlockId{8};
  decision_prep.label = "decision_prep";
  decision_prep.insts.push_back(LirLoadOp{"%t24", "i32", "%lv.tmp7"});
  decision_prep.insts.push_back(LirCmpOp{"%t25", false, "ne", "i32", "%t24", "0"});
  decision_prep.insts.push_back(LirCastOp{"%t26", LirCastKind::ZExt, "i1", "%t25", "i32"});
  decision_prep.insts.push_back(LirStoreOp{"i32", "%t26", "%lv.flag"});
  decision_prep.terminator = LirBr{"decision"};

  LirBlock decision;
  decision.id = LirBlockId{9};
  decision.label = "decision";
  decision.insts.push_back(LirLoadOp{"%t27", "i32", "%lv.flag"});
  decision.insts.push_back(LirCmpOp{"%t28", false, "ne", "i32", "%t27", "0"});
  decision.terminator = LirCondBr{"%t28", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{10};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{11};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{12};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{13};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(check));
  function.blocks.push_back(std::move(branch));
  function.blocks.push_back(std::move(finish_check));
  function.blocks.push_back(std::move(flag_prep));
  function.blocks.push_back(std::move(flag_check));
  function.blocks.push_back(std::move(branch_check));
  function.blocks.push_back(std::move(final_check));
  function.blocks.push_back(std::move(decision_prep));
  function.blocks.push_back(std::move(decision));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_eleven_local_slot_constant_conditional_goto_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.state", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp0", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp1", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp2", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp3", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp4", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp5", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp6", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp7", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp8", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.flag", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.state"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.state"});
  entry.insts.push_back(LirCmpOp{"%t1", false, "eq", "i32", "%t0", "0"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::ZExt, "i1", "%t1", "i32"});
  entry.insts.push_back(LirStoreOp{"i32", "%t2", "%lv.tmp0"});
  entry.terminator = LirBr{"check"};

  LirBlock check;
  check.id = LirBlockId{1};
  check.label = "check";
  check.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.tmp0"});
  check.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  check.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i1", "%t4", "i32"});
  check.insts.push_back(LirStoreOp{"i32", "%t5", "%lv.tmp1"});
  check.terminator = LirBr{"branch"};

  LirBlock branch;
  branch.id = LirBlockId{2};
  branch.label = "branch";
  branch.insts.push_back(LirLoadOp{"%t6", "i32", "%lv.tmp1"});
  branch.insts.push_back(LirCmpOp{"%t7", false, "ne", "i32", "%t6", "0"});
  branch.insts.push_back(LirCastOp{"%t8", LirCastKind::ZExt, "i1", "%t7", "i32"});
  branch.insts.push_back(LirStoreOp{"i32", "%t8", "%lv.tmp2"});
  branch.terminator = LirBr{"finish_check"};

  LirBlock finish_check;
  finish_check.id = LirBlockId{3};
  finish_check.label = "finish_check";
  finish_check.insts.push_back(LirLoadOp{"%t9", "i32", "%lv.tmp2"});
  finish_check.insts.push_back(LirCmpOp{"%t10", false, "ne", "i32", "%t9", "0"});
  finish_check.insts.push_back(LirCastOp{"%t11", LirCastKind::ZExt, "i1", "%t10", "i32"});
  finish_check.insts.push_back(LirStoreOp{"i32", "%t11", "%lv.tmp3"});
  finish_check.terminator = LirBr{"flag_prep"};

  LirBlock flag_prep;
  flag_prep.id = LirBlockId{4};
  flag_prep.label = "flag_prep";
  flag_prep.insts.push_back(LirLoadOp{"%t12", "i32", "%lv.tmp3"});
  flag_prep.insts.push_back(LirCmpOp{"%t13", false, "ne", "i32", "%t12", "0"});
  flag_prep.insts.push_back(LirCastOp{"%t14", LirCastKind::ZExt, "i1", "%t13", "i32"});
  flag_prep.insts.push_back(LirStoreOp{"i32", "%t14", "%lv.tmp4"});
  flag_prep.terminator = LirBr{"flag_check"};

  LirBlock flag_check;
  flag_check.id = LirBlockId{5};
  flag_check.label = "flag_check";
  flag_check.insts.push_back(LirLoadOp{"%t15", "i32", "%lv.tmp4"});
  flag_check.insts.push_back(LirCmpOp{"%t16", false, "ne", "i32", "%t15", "0"});
  flag_check.insts.push_back(LirCastOp{"%t17", LirCastKind::ZExt, "i1", "%t16", "i32"});
  flag_check.insts.push_back(LirStoreOp{"i32", "%t17", "%lv.tmp5"});
  flag_check.terminator = LirBr{"branch_check"};

  LirBlock branch_check;
  branch_check.id = LirBlockId{6};
  branch_check.label = "branch_check";
  branch_check.insts.push_back(LirLoadOp{"%t18", "i32", "%lv.tmp5"});
  branch_check.insts.push_back(LirCmpOp{"%t19", false, "ne", "i32", "%t18", "0"});
  branch_check.insts.push_back(LirCastOp{"%t20", LirCastKind::ZExt, "i1", "%t19", "i32"});
  branch_check.insts.push_back(LirStoreOp{"i32", "%t20", "%lv.tmp6"});
  branch_check.terminator = LirBr{"final_check"};

  LirBlock final_check;
  final_check.id = LirBlockId{7};
  final_check.label = "final_check";
  final_check.insts.push_back(LirLoadOp{"%t21", "i32", "%lv.tmp6"});
  final_check.insts.push_back(LirCmpOp{"%t22", false, "ne", "i32", "%t21", "0"});
  final_check.insts.push_back(LirCastOp{"%t23", LirCastKind::ZExt, "i1", "%t22", "i32"});
  final_check.insts.push_back(LirStoreOp{"i32", "%t23", "%lv.tmp7"});
  final_check.terminator = LirBr{"decision_prep"};

  LirBlock decision_prep;
  decision_prep.id = LirBlockId{8};
  decision_prep.label = "decision_prep";
  decision_prep.insts.push_back(LirLoadOp{"%t24", "i32", "%lv.tmp7"});
  decision_prep.insts.push_back(LirCmpOp{"%t25", false, "ne", "i32", "%t24", "0"});
  decision_prep.insts.push_back(LirCastOp{"%t26", LirCastKind::ZExt, "i1", "%t25", "i32"});
  decision_prep.insts.push_back(LirStoreOp{"i32", "%t26", "%lv.tmp8"});
  decision_prep.terminator = LirBr{"decision"};

  LirBlock decision;
  decision.id = LirBlockId{9};
  decision.label = "decision";
  decision.insts.push_back(LirLoadOp{"%t27", "i32", "%lv.tmp8"});
  decision.insts.push_back(LirCmpOp{"%t28", false, "ne", "i32", "%t27", "0"});
  decision.insts.push_back(LirCastOp{"%t29", LirCastKind::ZExt, "i1", "%t28", "i32"});
  decision.insts.push_back(LirStoreOp{"i32", "%t29", "%lv.flag"});
  decision.terminator = LirBr{"branch_decision"};

  LirBlock branch_decision;
  branch_decision.id = LirBlockId{10};
  branch_decision.label = "branch_decision";
  branch_decision.insts.push_back(LirLoadOp{"%t30", "i32", "%lv.flag"});
  branch_decision.insts.push_back(LirCmpOp{"%t31", false, "ne", "i32", "%t30", "0"});
  branch_decision.terminator = LirCondBr{"%t31", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{11};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{12};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{13};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{14};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(check));
  function.blocks.push_back(std::move(branch));
  function.blocks.push_back(std::move(finish_check));
  function.blocks.push_back(std::move(flag_prep));
  function.blocks.push_back(std::move(flag_check));
  function.blocks.push_back(std::move(branch_check));
  function.blocks.push_back(std::move(final_check));
  function.blocks.push_back(std::move(decision_prep));
  function.blocks.push_back(std::move(decision));
  function.blocks.push_back(std::move(branch_decision));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_twelve_local_slot_constant_conditional_goto_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.state", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp0", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp1", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp2", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp3", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp4", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp5", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp6", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp7", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp8", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp9", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.flag", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.state"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.state"});
  entry.insts.push_back(LirCmpOp{"%t1", false, "eq", "i32", "%t0", "0"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::ZExt, "i1", "%t1", "i32"});
  entry.insts.push_back(LirStoreOp{"i32", "%t2", "%lv.tmp0"});
  entry.terminator = LirBr{"check"};

  LirBlock check;
  check.id = LirBlockId{1};
  check.label = "check";
  check.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.tmp0"});
  check.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  check.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i1", "%t4", "i32"});
  check.insts.push_back(LirStoreOp{"i32", "%t5", "%lv.tmp1"});
  check.terminator = LirBr{"branch"};

  LirBlock branch;
  branch.id = LirBlockId{2};
  branch.label = "branch";
  branch.insts.push_back(LirLoadOp{"%t6", "i32", "%lv.tmp1"});
  branch.insts.push_back(LirCmpOp{"%t7", false, "ne", "i32", "%t6", "0"});
  branch.insts.push_back(LirCastOp{"%t8", LirCastKind::ZExt, "i1", "%t7", "i32"});
  branch.insts.push_back(LirStoreOp{"i32", "%t8", "%lv.tmp2"});
  branch.terminator = LirBr{"finish_check"};

  LirBlock finish_check;
  finish_check.id = LirBlockId{3};
  finish_check.label = "finish_check";
  finish_check.insts.push_back(LirLoadOp{"%t9", "i32", "%lv.tmp2"});
  finish_check.insts.push_back(LirCmpOp{"%t10", false, "ne", "i32", "%t9", "0"});
  finish_check.insts.push_back(LirCastOp{"%t11", LirCastKind::ZExt, "i1", "%t10", "i32"});
  finish_check.insts.push_back(LirStoreOp{"i32", "%t11", "%lv.tmp3"});
  finish_check.terminator = LirBr{"flag_prep"};

  LirBlock flag_prep;
  flag_prep.id = LirBlockId{4};
  flag_prep.label = "flag_prep";
  flag_prep.insts.push_back(LirLoadOp{"%t12", "i32", "%lv.tmp3"});
  flag_prep.insts.push_back(LirCmpOp{"%t13", false, "ne", "i32", "%t12", "0"});
  flag_prep.insts.push_back(LirCastOp{"%t14", LirCastKind::ZExt, "i1", "%t13", "i32"});
  flag_prep.insts.push_back(LirStoreOp{"i32", "%t14", "%lv.tmp4"});
  flag_prep.terminator = LirBr{"flag_check"};

  LirBlock flag_check;
  flag_check.id = LirBlockId{5};
  flag_check.label = "flag_check";
  flag_check.insts.push_back(LirLoadOp{"%t15", "i32", "%lv.tmp4"});
  flag_check.insts.push_back(LirCmpOp{"%t16", false, "ne", "i32", "%t15", "0"});
  flag_check.insts.push_back(LirCastOp{"%t17", LirCastKind::ZExt, "i1", "%t16", "i32"});
  flag_check.insts.push_back(LirStoreOp{"i32", "%t17", "%lv.tmp5"});
  flag_check.terminator = LirBr{"branch_check"};

  LirBlock branch_check;
  branch_check.id = LirBlockId{6};
  branch_check.label = "branch_check";
  branch_check.insts.push_back(LirLoadOp{"%t18", "i32", "%lv.tmp5"});
  branch_check.insts.push_back(LirCmpOp{"%t19", false, "ne", "i32", "%t18", "0"});
  branch_check.insts.push_back(LirCastOp{"%t20", LirCastKind::ZExt, "i1", "%t19", "i32"});
  branch_check.insts.push_back(LirStoreOp{"i32", "%t20", "%lv.tmp6"});
  branch_check.terminator = LirBr{"final_check"};

  LirBlock final_check;
  final_check.id = LirBlockId{7};
  final_check.label = "final_check";
  final_check.insts.push_back(LirLoadOp{"%t21", "i32", "%lv.tmp6"});
  final_check.insts.push_back(LirCmpOp{"%t22", false, "ne", "i32", "%t21", "0"});
  final_check.insts.push_back(LirCastOp{"%t23", LirCastKind::ZExt, "i1", "%t22", "i32"});
  final_check.insts.push_back(LirStoreOp{"i32", "%t23", "%lv.tmp7"});
  final_check.terminator = LirBr{"decision_prep"};

  LirBlock decision_prep;
  decision_prep.id = LirBlockId{8};
  decision_prep.label = "decision_prep";
  decision_prep.insts.push_back(LirLoadOp{"%t24", "i32", "%lv.tmp7"});
  decision_prep.insts.push_back(LirCmpOp{"%t25", false, "ne", "i32", "%t24", "0"});
  decision_prep.insts.push_back(LirCastOp{"%t26", LirCastKind::ZExt, "i1", "%t25", "i32"});
  decision_prep.insts.push_back(LirStoreOp{"i32", "%t26", "%lv.tmp8"});
  decision_prep.terminator = LirBr{"decision"};

  LirBlock decision;
  decision.id = LirBlockId{9};
  decision.label = "decision";
  decision.insts.push_back(LirLoadOp{"%t27", "i32", "%lv.tmp8"});
  decision.insts.push_back(LirCmpOp{"%t28", false, "ne", "i32", "%t27", "0"});
  decision.insts.push_back(LirCastOp{"%t29", LirCastKind::ZExt, "i1", "%t28", "i32"});
  decision.insts.push_back(LirStoreOp{"i32", "%t29", "%lv.tmp9"});
  decision.terminator = LirBr{"flag_store"};

  LirBlock flag_store;
  flag_store.id = LirBlockId{10};
  flag_store.label = "flag_store";
  flag_store.insts.push_back(LirLoadOp{"%t30", "i32", "%lv.tmp9"});
  flag_store.insts.push_back(LirCmpOp{"%t31", false, "ne", "i32", "%t30", "0"});
  flag_store.insts.push_back(LirCastOp{"%t32", LirCastKind::ZExt, "i1", "%t31", "i32"});
  flag_store.insts.push_back(LirStoreOp{"i32", "%t32", "%lv.flag"});
  flag_store.terminator = LirBr{"branch_decision"};

  LirBlock branch_decision;
  branch_decision.id = LirBlockId{11};
  branch_decision.label = "branch_decision";
  branch_decision.insts.push_back(LirLoadOp{"%t33", "i32", "%lv.flag"});
  branch_decision.insts.push_back(LirCmpOp{"%t34", false, "ne", "i32", "%t33", "0"});
  branch_decision.terminator = LirCondBr{"%t34", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{12};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{13};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{14};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{15};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(check));
  function.blocks.push_back(std::move(branch));
  function.blocks.push_back(std::move(finish_check));
  function.blocks.push_back(std::move(flag_prep));
  function.blocks.push_back(std::move(flag_check));
  function.blocks.push_back(std::move(branch_check));
  function.blocks.push_back(std::move(final_check));
  function.blocks.push_back(std::move(decision_prep));
  function.blocks.push_back(std::move(decision));
  function.blocks.push_back(std::move(flag_store));
  function.blocks.push_back(std::move(branch_decision));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_thirteen_local_slot_constant_conditional_goto_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.state", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp0", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp1", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp2", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp3", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp4", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp5", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp6", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp7", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp8", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp9", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp10", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.flag", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.state"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.state"});
  entry.insts.push_back(LirCmpOp{"%t1", false, "eq", "i32", "%t0", "0"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::ZExt, "i1", "%t1", "i32"});
  entry.insts.push_back(LirStoreOp{"i32", "%t2", "%lv.tmp0"});
  entry.terminator = LirBr{"check"};

  LirBlock check;
  check.id = LirBlockId{1};
  check.label = "check";
  check.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.tmp0"});
  check.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  check.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i1", "%t4", "i32"});
  check.insts.push_back(LirStoreOp{"i32", "%t5", "%lv.tmp1"});
  check.terminator = LirBr{"branch"};

  LirBlock branch;
  branch.id = LirBlockId{2};
  branch.label = "branch";
  branch.insts.push_back(LirLoadOp{"%t6", "i32", "%lv.tmp1"});
  branch.insts.push_back(LirCmpOp{"%t7", false, "ne", "i32", "%t6", "0"});
  branch.insts.push_back(LirCastOp{"%t8", LirCastKind::ZExt, "i1", "%t7", "i32"});
  branch.insts.push_back(LirStoreOp{"i32", "%t8", "%lv.tmp2"});
  branch.terminator = LirBr{"finish_check"};

  LirBlock finish_check;
  finish_check.id = LirBlockId{3};
  finish_check.label = "finish_check";
  finish_check.insts.push_back(LirLoadOp{"%t9", "i32", "%lv.tmp2"});
  finish_check.insts.push_back(LirCmpOp{"%t10", false, "ne", "i32", "%t9", "0"});
  finish_check.insts.push_back(LirCastOp{"%t11", LirCastKind::ZExt, "i1", "%t10", "i32"});
  finish_check.insts.push_back(LirStoreOp{"i32", "%t11", "%lv.tmp3"});
  finish_check.terminator = LirBr{"flag_prep"};

  LirBlock flag_prep;
  flag_prep.id = LirBlockId{4};
  flag_prep.label = "flag_prep";
  flag_prep.insts.push_back(LirLoadOp{"%t12", "i32", "%lv.tmp3"});
  flag_prep.insts.push_back(LirCmpOp{"%t13", false, "ne", "i32", "%t12", "0"});
  flag_prep.insts.push_back(LirCastOp{"%t14", LirCastKind::ZExt, "i1", "%t13", "i32"});
  flag_prep.insts.push_back(LirStoreOp{"i32", "%t14", "%lv.tmp4"});
  flag_prep.terminator = LirBr{"flag_check"};

  LirBlock flag_check;
  flag_check.id = LirBlockId{5};
  flag_check.label = "flag_check";
  flag_check.insts.push_back(LirLoadOp{"%t15", "i32", "%lv.tmp4"});
  flag_check.insts.push_back(LirCmpOp{"%t16", false, "ne", "i32", "%t15", "0"});
  flag_check.insts.push_back(LirCastOp{"%t17", LirCastKind::ZExt, "i1", "%t16", "i32"});
  flag_check.insts.push_back(LirStoreOp{"i32", "%t17", "%lv.tmp5"});
  flag_check.terminator = LirBr{"branch_check"};

  LirBlock branch_check;
  branch_check.id = LirBlockId{6};
  branch_check.label = "branch_check";
  branch_check.insts.push_back(LirLoadOp{"%t18", "i32", "%lv.tmp5"});
  branch_check.insts.push_back(LirCmpOp{"%t19", false, "ne", "i32", "%t18", "0"});
  branch_check.insts.push_back(LirCastOp{"%t20", LirCastKind::ZExt, "i1", "%t19", "i32"});
  branch_check.insts.push_back(LirStoreOp{"i32", "%t20", "%lv.tmp6"});
  branch_check.terminator = LirBr{"final_check"};

  LirBlock final_check;
  final_check.id = LirBlockId{7};
  final_check.label = "final_check";
  final_check.insts.push_back(LirLoadOp{"%t21", "i32", "%lv.tmp6"});
  final_check.insts.push_back(LirCmpOp{"%t22", false, "ne", "i32", "%t21", "0"});
  final_check.insts.push_back(LirCastOp{"%t23", LirCastKind::ZExt, "i1", "%t22", "i32"});
  final_check.insts.push_back(LirStoreOp{"i32", "%t23", "%lv.tmp7"});
  final_check.terminator = LirBr{"decision_prep"};

  LirBlock decision_prep;
  decision_prep.id = LirBlockId{8};
  decision_prep.label = "decision_prep";
  decision_prep.insts.push_back(LirLoadOp{"%t24", "i32", "%lv.tmp7"});
  decision_prep.insts.push_back(LirCmpOp{"%t25", false, "ne", "i32", "%t24", "0"});
  decision_prep.insts.push_back(LirCastOp{"%t26", LirCastKind::ZExt, "i1", "%t25", "i32"});
  decision_prep.insts.push_back(LirStoreOp{"i32", "%t26", "%lv.tmp8"});
  decision_prep.terminator = LirBr{"decision"};

  LirBlock decision;
  decision.id = LirBlockId{9};
  decision.label = "decision";
  decision.insts.push_back(LirLoadOp{"%t27", "i32", "%lv.tmp8"});
  decision.insts.push_back(LirCmpOp{"%t28", false, "ne", "i32", "%t27", "0"});
  decision.insts.push_back(LirCastOp{"%t29", LirCastKind::ZExt, "i1", "%t28", "i32"});
  decision.insts.push_back(LirStoreOp{"i32", "%t29", "%lv.tmp9"});
  decision.terminator = LirBr{"flag_store"};

  LirBlock flag_store;
  flag_store.id = LirBlockId{10};
  flag_store.label = "flag_store";
  flag_store.insts.push_back(LirLoadOp{"%t30", "i32", "%lv.tmp9"});
  flag_store.insts.push_back(LirCmpOp{"%t31", false, "ne", "i32", "%t30", "0"});
  flag_store.insts.push_back(LirCastOp{"%t32", LirCastKind::ZExt, "i1", "%t31", "i32"});
  flag_store.insts.push_back(LirStoreOp{"i32", "%t32", "%lv.tmp10"});
  flag_store.terminator = LirBr{"flag_materialize"};

  LirBlock flag_materialize;
  flag_materialize.id = LirBlockId{11};
  flag_materialize.label = "flag_materialize";
  flag_materialize.insts.push_back(LirLoadOp{"%t33", "i32", "%lv.tmp10"});
  flag_materialize.insts.push_back(LirCmpOp{"%t34", false, "ne", "i32", "%t33", "0"});
  flag_materialize.insts.push_back(LirCastOp{"%t35", LirCastKind::ZExt, "i1", "%t34", "i32"});
  flag_materialize.insts.push_back(LirStoreOp{"i32", "%t35", "%lv.flag"});
  flag_materialize.terminator = LirBr{"branch_decision"};

  LirBlock branch_decision;
  branch_decision.id = LirBlockId{12};
  branch_decision.label = "branch_decision";
  branch_decision.insts.push_back(LirLoadOp{"%t36", "i32", "%lv.flag"});
  branch_decision.insts.push_back(LirCmpOp{"%t37", false, "ne", "i32", "%t36", "0"});
  branch_decision.terminator = LirCondBr{"%t37", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{13};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{14};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{15};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{16};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(check));
  function.blocks.push_back(std::move(branch));
  function.blocks.push_back(std::move(finish_check));
  function.blocks.push_back(std::move(flag_prep));
  function.blocks.push_back(std::move(flag_check));
  function.blocks.push_back(std::move(branch_check));
  function.blocks.push_back(std::move(final_check));
  function.blocks.push_back(std::move(decision_prep));
  function.blocks.push_back(std::move(decision));
  function.blocks.push_back(std::move(flag_store));
  function.blocks.push_back(std::move(flag_materialize));
  function.blocks.push_back(std::move(branch_decision));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_fourteen_local_slot_constant_conditional_goto_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.state", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp0", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp1", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp2", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp3", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp4", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp5", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp6", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp7", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp8", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp9", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp10", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp11", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.flag", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.state"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.state"});
  entry.insts.push_back(LirCmpOp{"%t1", false, "eq", "i32", "%t0", "0"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::ZExt, "i1", "%t1", "i32"});
  entry.insts.push_back(LirStoreOp{"i32", "%t2", "%lv.tmp0"});
  entry.terminator = LirBr{"check"};

  LirBlock check;
  check.id = LirBlockId{1};
  check.label = "check";
  check.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.tmp0"});
  check.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  check.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i1", "%t4", "i32"});
  check.insts.push_back(LirStoreOp{"i32", "%t5", "%lv.tmp1"});
  check.terminator = LirBr{"branch"};

  LirBlock branch;
  branch.id = LirBlockId{2};
  branch.label = "branch";
  branch.insts.push_back(LirLoadOp{"%t6", "i32", "%lv.tmp1"});
  branch.insts.push_back(LirCmpOp{"%t7", false, "ne", "i32", "%t6", "0"});
  branch.insts.push_back(LirCastOp{"%t8", LirCastKind::ZExt, "i1", "%t7", "i32"});
  branch.insts.push_back(LirStoreOp{"i32", "%t8", "%lv.tmp2"});
  branch.terminator = LirBr{"finish_check"};

  LirBlock finish_check;
  finish_check.id = LirBlockId{3};
  finish_check.label = "finish_check";
  finish_check.insts.push_back(LirLoadOp{"%t9", "i32", "%lv.tmp2"});
  finish_check.insts.push_back(LirCmpOp{"%t10", false, "ne", "i32", "%t9", "0"});
  finish_check.insts.push_back(LirCastOp{"%t11", LirCastKind::ZExt, "i1", "%t10", "i32"});
  finish_check.insts.push_back(LirStoreOp{"i32", "%t11", "%lv.tmp3"});
  finish_check.terminator = LirBr{"flag_prep"};

  LirBlock flag_prep;
  flag_prep.id = LirBlockId{4};
  flag_prep.label = "flag_prep";
  flag_prep.insts.push_back(LirLoadOp{"%t12", "i32", "%lv.tmp3"});
  flag_prep.insts.push_back(LirCmpOp{"%t13", false, "ne", "i32", "%t12", "0"});
  flag_prep.insts.push_back(LirCastOp{"%t14", LirCastKind::ZExt, "i1", "%t13", "i32"});
  flag_prep.insts.push_back(LirStoreOp{"i32", "%t14", "%lv.tmp4"});
  flag_prep.terminator = LirBr{"flag_check"};

  LirBlock flag_check;
  flag_check.id = LirBlockId{5};
  flag_check.label = "flag_check";
  flag_check.insts.push_back(LirLoadOp{"%t15", "i32", "%lv.tmp4"});
  flag_check.insts.push_back(LirCmpOp{"%t16", false, "ne", "i32", "%t15", "0"});
  flag_check.insts.push_back(LirCastOp{"%t17", LirCastKind::ZExt, "i1", "%t16", "i32"});
  flag_check.insts.push_back(LirStoreOp{"i32", "%t17", "%lv.tmp5"});
  flag_check.terminator = LirBr{"branch_check"};

  LirBlock branch_check;
  branch_check.id = LirBlockId{6};
  branch_check.label = "branch_check";
  branch_check.insts.push_back(LirLoadOp{"%t18", "i32", "%lv.tmp5"});
  branch_check.insts.push_back(LirCmpOp{"%t19", false, "ne", "i32", "%t18", "0"});
  branch_check.insts.push_back(LirCastOp{"%t20", LirCastKind::ZExt, "i1", "%t19", "i32"});
  branch_check.insts.push_back(LirStoreOp{"i32", "%t20", "%lv.tmp6"});
  branch_check.terminator = LirBr{"final_check"};

  LirBlock final_check;
  final_check.id = LirBlockId{7};
  final_check.label = "final_check";
  final_check.insts.push_back(LirLoadOp{"%t21", "i32", "%lv.tmp6"});
  final_check.insts.push_back(LirCmpOp{"%t22", false, "ne", "i32", "%t21", "0"});
  final_check.insts.push_back(LirCastOp{"%t23", LirCastKind::ZExt, "i1", "%t22", "i32"});
  final_check.insts.push_back(LirStoreOp{"i32", "%t23", "%lv.tmp7"});
  final_check.terminator = LirBr{"decision_prep"};

  LirBlock decision_prep;
  decision_prep.id = LirBlockId{8};
  decision_prep.label = "decision_prep";
  decision_prep.insts.push_back(LirLoadOp{"%t24", "i32", "%lv.tmp7"});
  decision_prep.insts.push_back(LirCmpOp{"%t25", false, "ne", "i32", "%t24", "0"});
  decision_prep.insts.push_back(LirCastOp{"%t26", LirCastKind::ZExt, "i1", "%t25", "i32"});
  decision_prep.insts.push_back(LirStoreOp{"i32", "%t26", "%lv.tmp8"});
  decision_prep.terminator = LirBr{"decision"};

  LirBlock decision;
  decision.id = LirBlockId{9};
  decision.label = "decision";
  decision.insts.push_back(LirLoadOp{"%t27", "i32", "%lv.tmp8"});
  decision.insts.push_back(LirCmpOp{"%t28", false, "ne", "i32", "%t27", "0"});
  decision.insts.push_back(LirCastOp{"%t29", LirCastKind::ZExt, "i1", "%t28", "i32"});
  decision.insts.push_back(LirStoreOp{"i32", "%t29", "%lv.tmp9"});
  decision.terminator = LirBr{"flag_store"};

  LirBlock flag_store;
  flag_store.id = LirBlockId{10};
  flag_store.label = "flag_store";
  flag_store.insts.push_back(LirLoadOp{"%t30", "i32", "%lv.tmp9"});
  flag_store.insts.push_back(LirCmpOp{"%t31", false, "ne", "i32", "%t30", "0"});
  flag_store.insts.push_back(LirCastOp{"%t32", LirCastKind::ZExt, "i1", "%t31", "i32"});
  flag_store.insts.push_back(LirStoreOp{"i32", "%t32", "%lv.tmp10"});
  flag_store.terminator = LirBr{"branch_decision"};

  LirBlock branch_decision;
  branch_decision.id = LirBlockId{11};
  branch_decision.label = "branch_decision";
  branch_decision.insts.push_back(LirLoadOp{"%t33", "i32", "%lv.tmp10"});
  branch_decision.insts.push_back(LirCmpOp{"%t34", false, "ne", "i32", "%t33", "0"});
  branch_decision.insts.push_back(LirCastOp{"%t35", LirCastKind::ZExt, "i1", "%t34", "i32"});
  branch_decision.insts.push_back(LirStoreOp{"i32", "%t35", "%lv.tmp11"});
  branch_decision.terminator = LirBr{"flag_bridge"};

  LirBlock flag_bridge;
  flag_bridge.id = LirBlockId{12};
  flag_bridge.label = "flag_bridge";
  flag_bridge.insts.push_back(LirLoadOp{"%t36", "i32", "%lv.tmp11"});
  flag_bridge.insts.push_back(LirCmpOp{"%t37", false, "ne", "i32", "%t36", "0"});
  flag_bridge.insts.push_back(LirCastOp{"%t38", LirCastKind::ZExt, "i1", "%t37", "i32"});
  flag_bridge.insts.push_back(LirStoreOp{"i32", "%t38", "%lv.flag"});
  flag_bridge.terminator = LirBr{"done"};

  LirBlock done;
  done.id = LirBlockId{13};
  done.label = "done";
  done.insts.push_back(LirLoadOp{"%t39", "i32", "%lv.flag"});
  done.insts.push_back(LirCmpOp{"%t40", false, "ne", "i32", "%t39", "0"});
  done.terminator = LirCondBr{"%t40", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{14};
  then_path.label = "then_path";
  then_path.terminator = LirRet{std::string("0"), "i32"};

  LirBlock else_path;
  else_path.id = LirBlockId{15};
  else_path.label = "else_path";
  else_path.terminator = LirRet{std::string("1"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(check));
  function.blocks.push_back(std::move(branch));
  function.blocks.push_back(std::move(finish_check));
  function.blocks.push_back(std::move(flag_prep));
  function.blocks.push_back(std::move(flag_check));
  function.blocks.push_back(std::move(branch_check));
  function.blocks.push_back(std::move(final_check));
  function.blocks.push_back(std::move(decision_prep));
  function.blocks.push_back(std::move(decision));
  function.blocks.push_back(std::move(flag_store));
  function.blocks.push_back(std::move(branch_decision));
  function.blocks.push_back(std::move(flag_bridge));
  function.blocks.push_back(std::move(done));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_fifteen_local_slot_constant_conditional_goto_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.state", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp0", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp1", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp2", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp3", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp4", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp5", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp6", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp7", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp8", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp9", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp10", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp11", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp12", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.flag", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.state"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.state"});
  entry.insts.push_back(LirCmpOp{"%t1", false, "eq", "i32", "%t0", "0"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::ZExt, "i1", "%t1", "i32"});
  entry.insts.push_back(LirStoreOp{"i32", "%t2", "%lv.tmp0"});
  entry.terminator = LirBr{"check"};

  LirBlock check;
  check.id = LirBlockId{1};
  check.label = "check";
  check.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.tmp0"});
  check.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  check.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i1", "%t4", "i32"});
  check.insts.push_back(LirStoreOp{"i32", "%t5", "%lv.tmp1"});
  check.terminator = LirBr{"branch"};

  LirBlock branch;
  branch.id = LirBlockId{2};
  branch.label = "branch";
  branch.insts.push_back(LirLoadOp{"%t6", "i32", "%lv.tmp1"});
  branch.insts.push_back(LirCmpOp{"%t7", false, "ne", "i32", "%t6", "0"});
  branch.insts.push_back(LirCastOp{"%t8", LirCastKind::ZExt, "i1", "%t7", "i32"});
  branch.insts.push_back(LirStoreOp{"i32", "%t8", "%lv.tmp2"});
  branch.terminator = LirBr{"finish_check"};

  LirBlock finish_check;
  finish_check.id = LirBlockId{3};
  finish_check.label = "finish_check";
  finish_check.insts.push_back(LirLoadOp{"%t9", "i32", "%lv.tmp2"});
  finish_check.insts.push_back(LirCmpOp{"%t10", false, "ne", "i32", "%t9", "0"});
  finish_check.insts.push_back(LirCastOp{"%t11", LirCastKind::ZExt, "i1", "%t10", "i32"});
  finish_check.insts.push_back(LirStoreOp{"i32", "%t11", "%lv.tmp3"});
  finish_check.terminator = LirBr{"flag_prep"};

  LirBlock flag_prep;
  flag_prep.id = LirBlockId{4};
  flag_prep.label = "flag_prep";
  flag_prep.insts.push_back(LirLoadOp{"%t12", "i32", "%lv.tmp3"});
  flag_prep.insts.push_back(LirCmpOp{"%t13", false, "ne", "i32", "%t12", "0"});
  flag_prep.insts.push_back(LirCastOp{"%t14", LirCastKind::ZExt, "i1", "%t13", "i32"});
  flag_prep.insts.push_back(LirStoreOp{"i32", "%t14", "%lv.tmp4"});
  flag_prep.terminator = LirBr{"flag_check"};

  LirBlock flag_check;
  flag_check.id = LirBlockId{5};
  flag_check.label = "flag_check";
  flag_check.insts.push_back(LirLoadOp{"%t15", "i32", "%lv.tmp4"});
  flag_check.insts.push_back(LirCmpOp{"%t16", false, "ne", "i32", "%t15", "0"});
  flag_check.insts.push_back(LirCastOp{"%t17", LirCastKind::ZExt, "i1", "%t16", "i32"});
  flag_check.insts.push_back(LirStoreOp{"i32", "%t17", "%lv.tmp5"});
  flag_check.terminator = LirBr{"branch_check"};

  LirBlock branch_check;
  branch_check.id = LirBlockId{6};
  branch_check.label = "branch_check";
  branch_check.insts.push_back(LirLoadOp{"%t18", "i32", "%lv.tmp5"});
  branch_check.insts.push_back(LirCmpOp{"%t19", false, "ne", "i32", "%t18", "0"});
  branch_check.insts.push_back(LirCastOp{"%t20", LirCastKind::ZExt, "i1", "%t19", "i32"});
  branch_check.insts.push_back(LirStoreOp{"i32", "%t20", "%lv.tmp6"});
  branch_check.terminator = LirBr{"final_check"};

  LirBlock final_check;
  final_check.id = LirBlockId{7};
  final_check.label = "final_check";
  final_check.insts.push_back(LirLoadOp{"%t21", "i32", "%lv.tmp6"});
  final_check.insts.push_back(LirCmpOp{"%t22", false, "ne", "i32", "%t21", "0"});
  final_check.insts.push_back(LirCastOp{"%t23", LirCastKind::ZExt, "i1", "%t22", "i32"});
  final_check.insts.push_back(LirStoreOp{"i32", "%t23", "%lv.tmp7"});
  final_check.terminator = LirBr{"decision_prep"};

  LirBlock decision_prep;
  decision_prep.id = LirBlockId{8};
  decision_prep.label = "decision_prep";
  decision_prep.insts.push_back(LirLoadOp{"%t24", "i32", "%lv.tmp7"});
  decision_prep.insts.push_back(LirCmpOp{"%t25", false, "ne", "i32", "%t24", "0"});
  decision_prep.insts.push_back(LirCastOp{"%t26", LirCastKind::ZExt, "i1", "%t25", "i32"});
  decision_prep.insts.push_back(LirStoreOp{"i32", "%t26", "%lv.tmp8"});
  decision_prep.terminator = LirBr{"decision"};

  LirBlock decision;
  decision.id = LirBlockId{9};
  decision.label = "decision";
  decision.insts.push_back(LirLoadOp{"%t27", "i32", "%lv.tmp8"});
  decision.insts.push_back(LirCmpOp{"%t28", false, "ne", "i32", "%t27", "0"});
  decision.insts.push_back(LirCastOp{"%t29", LirCastKind::ZExt, "i1", "%t28", "i32"});
  decision.insts.push_back(LirStoreOp{"i32", "%t29", "%lv.tmp9"});
  decision.terminator = LirBr{"flag_store"};

  LirBlock flag_store;
  flag_store.id = LirBlockId{10};
  flag_store.label = "flag_store";
  flag_store.insts.push_back(LirLoadOp{"%t30", "i32", "%lv.tmp9"});
  flag_store.insts.push_back(LirCmpOp{"%t31", false, "ne", "i32", "%t30", "0"});
  flag_store.insts.push_back(LirCastOp{"%t32", LirCastKind::ZExt, "i1", "%t31", "i32"});
  flag_store.insts.push_back(LirStoreOp{"i32", "%t32", "%lv.tmp10"});
  flag_store.terminator = LirBr{"flag_materialize"};

  LirBlock flag_materialize;
  flag_materialize.id = LirBlockId{11};
  flag_materialize.label = "flag_materialize";
  flag_materialize.insts.push_back(LirLoadOp{"%t33", "i32", "%lv.tmp10"});
  flag_materialize.insts.push_back(LirCmpOp{"%t34", false, "ne", "i32", "%t33", "0"});
  flag_materialize.insts.push_back(LirCastOp{"%t35", LirCastKind::ZExt, "i1", "%t34", "i32"});
  flag_materialize.insts.push_back(LirStoreOp{"i32", "%t35", "%lv.tmp11"});
  flag_materialize.terminator = LirBr{"branch_decision"};

  LirBlock branch_decision;
  branch_decision.id = LirBlockId{12};
  branch_decision.label = "branch_decision";
  branch_decision.insts.push_back(LirLoadOp{"%t36", "i32", "%lv.tmp11"});
  branch_decision.insts.push_back(LirCmpOp{"%t37", false, "ne", "i32", "%t36", "0"});
  branch_decision.insts.push_back(LirCastOp{"%t38", LirCastKind::ZExt, "i1", "%t37", "i32"});
  branch_decision.insts.push_back(LirStoreOp{"i32", "%t38", "%lv.tmp12"});
  branch_decision.terminator = LirBr{"flag_check_tail"};

  LirBlock flag_check_tail;
  flag_check_tail.id = LirBlockId{13};
  flag_check_tail.label = "flag_check_tail";
  flag_check_tail.insts.push_back(LirLoadOp{"%t39", "i32", "%lv.tmp12"});
  flag_check_tail.insts.push_back(LirCmpOp{"%t40", false, "ne", "i32", "%t39", "0"});
  flag_check_tail.insts.push_back(LirCastOp{"%t41", LirCastKind::ZExt, "i1", "%t40", "i32"});
  flag_check_tail.insts.push_back(LirStoreOp{"i32", "%t41", "%lv.flag"});
  flag_check_tail.terminator = LirBr{"flag_decision"};

  LirBlock flag_decision;
  flag_decision.id = LirBlockId{14};
  flag_decision.label = "flag_decision";
  flag_decision.insts.push_back(LirLoadOp{"%t42", "i32", "%lv.flag"});
  flag_decision.insts.push_back(LirCmpOp{"%t43", false, "ne", "i32", "%t42", "0"});
  flag_decision.terminator = LirCondBr{"%t43", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{15};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{16};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{17};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{18};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(check));
  function.blocks.push_back(std::move(branch));
  function.blocks.push_back(std::move(finish_check));
  function.blocks.push_back(std::move(flag_prep));
  function.blocks.push_back(std::move(flag_check));
  function.blocks.push_back(std::move(branch_check));
  function.blocks.push_back(std::move(final_check));
  function.blocks.push_back(std::move(decision_prep));
  function.blocks.push_back(std::move(decision));
  function.blocks.push_back(std::move(flag_store));
  function.blocks.push_back(std::move(flag_materialize));
  function.blocks.push_back(std::move(branch_decision));
  function.blocks.push_back(std::move(flag_check_tail));
  function.blocks.push_back(std::move(flag_decision));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_sixteen_local_slot_constant_conditional_goto_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.state", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp0", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp1", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp2", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp3", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp4", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp5", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp6", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp7", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp8", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp9", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp10", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp11", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp12", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp13", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.flag", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.state"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.state"});
  entry.insts.push_back(LirCmpOp{"%t1", false, "eq", "i32", "%t0", "0"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::ZExt, "i1", "%t1", "i32"});
  entry.insts.push_back(LirStoreOp{"i32", "%t2", "%lv.tmp0"});
  entry.terminator = LirBr{"check"};

  LirBlock check;
  check.id = LirBlockId{1};
  check.label = "check";
  check.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.tmp0"});
  check.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  check.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i1", "%t4", "i32"});
  check.insts.push_back(LirStoreOp{"i32", "%t5", "%lv.tmp1"});
  check.terminator = LirBr{"branch"};

  LirBlock branch;
  branch.id = LirBlockId{2};
  branch.label = "branch";
  branch.insts.push_back(LirLoadOp{"%t6", "i32", "%lv.tmp1"});
  branch.insts.push_back(LirCmpOp{"%t7", false, "ne", "i32", "%t6", "0"});
  branch.insts.push_back(LirCastOp{"%t8", LirCastKind::ZExt, "i1", "%t7", "i32"});
  branch.insts.push_back(LirStoreOp{"i32", "%t8", "%lv.tmp2"});
  branch.terminator = LirBr{"finish_check"};

  LirBlock finish_check;
  finish_check.id = LirBlockId{3};
  finish_check.label = "finish_check";
  finish_check.insts.push_back(LirLoadOp{"%t9", "i32", "%lv.tmp2"});
  finish_check.insts.push_back(LirCmpOp{"%t10", false, "ne", "i32", "%t9", "0"});
  finish_check.insts.push_back(LirCastOp{"%t11", LirCastKind::ZExt, "i1", "%t10", "i32"});
  finish_check.insts.push_back(LirStoreOp{"i32", "%t11", "%lv.tmp3"});
  finish_check.terminator = LirBr{"flag_prep"};

  LirBlock flag_prep;
  flag_prep.id = LirBlockId{4};
  flag_prep.label = "flag_prep";
  flag_prep.insts.push_back(LirLoadOp{"%t12", "i32", "%lv.tmp3"});
  flag_prep.insts.push_back(LirCmpOp{"%t13", false, "ne", "i32", "%t12", "0"});
  flag_prep.insts.push_back(LirCastOp{"%t14", LirCastKind::ZExt, "i1", "%t13", "i32"});
  flag_prep.insts.push_back(LirStoreOp{"i32", "%t14", "%lv.tmp4"});
  flag_prep.terminator = LirBr{"flag_check"};

  LirBlock flag_check;
  flag_check.id = LirBlockId{5};
  flag_check.label = "flag_check";
  flag_check.insts.push_back(LirLoadOp{"%t15", "i32", "%lv.tmp4"});
  flag_check.insts.push_back(LirCmpOp{"%t16", false, "ne", "i32", "%t15", "0"});
  flag_check.insts.push_back(LirCastOp{"%t17", LirCastKind::ZExt, "i1", "%t16", "i32"});
  flag_check.insts.push_back(LirStoreOp{"i32", "%t17", "%lv.tmp5"});
  flag_check.terminator = LirBr{"branch_check"};

  LirBlock branch_check;
  branch_check.id = LirBlockId{6};
  branch_check.label = "branch_check";
  branch_check.insts.push_back(LirLoadOp{"%t18", "i32", "%lv.tmp5"});
  branch_check.insts.push_back(LirCmpOp{"%t19", false, "ne", "i32", "%t18", "0"});
  branch_check.insts.push_back(LirCastOp{"%t20", LirCastKind::ZExt, "i1", "%t19", "i32"});
  branch_check.insts.push_back(LirStoreOp{"i32", "%t20", "%lv.tmp6"});
  branch_check.terminator = LirBr{"final_check"};

  LirBlock final_check;
  final_check.id = LirBlockId{7};
  final_check.label = "final_check";
  final_check.insts.push_back(LirLoadOp{"%t21", "i32", "%lv.tmp6"});
  final_check.insts.push_back(LirCmpOp{"%t22", false, "ne", "i32", "%t21", "0"});
  final_check.insts.push_back(LirCastOp{"%t23", LirCastKind::ZExt, "i1", "%t22", "i32"});
  final_check.insts.push_back(LirStoreOp{"i32", "%t23", "%lv.tmp7"});
  final_check.terminator = LirBr{"decision_prep"};

  LirBlock decision_prep;
  decision_prep.id = LirBlockId{8};
  decision_prep.label = "decision_prep";
  decision_prep.insts.push_back(LirLoadOp{"%t24", "i32", "%lv.tmp7"});
  decision_prep.insts.push_back(LirCmpOp{"%t25", false, "ne", "i32", "%t24", "0"});
  decision_prep.insts.push_back(LirCastOp{"%t26", LirCastKind::ZExt, "i1", "%t25", "i32"});
  decision_prep.insts.push_back(LirStoreOp{"i32", "%t26", "%lv.tmp8"});
  decision_prep.terminator = LirBr{"decision"};

  LirBlock decision;
  decision.id = LirBlockId{9};
  decision.label = "decision";
  decision.insts.push_back(LirLoadOp{"%t27", "i32", "%lv.tmp8"});
  decision.insts.push_back(LirCmpOp{"%t28", false, "ne", "i32", "%t27", "0"});
  decision.insts.push_back(LirCastOp{"%t29", LirCastKind::ZExt, "i1", "%t28", "i32"});
  decision.insts.push_back(LirStoreOp{"i32", "%t29", "%lv.tmp9"});
  decision.terminator = LirBr{"flag_store"};

  LirBlock flag_store;
  flag_store.id = LirBlockId{10};
  flag_store.label = "flag_store";
  flag_store.insts.push_back(LirLoadOp{"%t30", "i32", "%lv.tmp9"});
  flag_store.insts.push_back(LirCmpOp{"%t31", false, "ne", "i32", "%t30", "0"});
  flag_store.insts.push_back(LirCastOp{"%t32", LirCastKind::ZExt, "i1", "%t31", "i32"});
  flag_store.insts.push_back(LirStoreOp{"i32", "%t32", "%lv.tmp10"});
  flag_store.terminator = LirBr{"flag_materialize"};

  LirBlock flag_materialize;
  flag_materialize.id = LirBlockId{11};
  flag_materialize.label = "flag_materialize";
  flag_materialize.insts.push_back(LirLoadOp{"%t33", "i32", "%lv.tmp10"});
  flag_materialize.insts.push_back(LirCmpOp{"%t34", false, "ne", "i32", "%t33", "0"});
  flag_materialize.insts.push_back(LirCastOp{"%t35", LirCastKind::ZExt, "i1", "%t34", "i32"});
  flag_materialize.insts.push_back(LirStoreOp{"i32", "%t35", "%lv.tmp11"});
  flag_materialize.terminator = LirBr{"branch_decision"};

  LirBlock branch_decision;
  branch_decision.id = LirBlockId{12};
  branch_decision.label = "branch_decision";
  branch_decision.insts.push_back(LirLoadOp{"%t36", "i32", "%lv.tmp11"});
  branch_decision.insts.push_back(LirCmpOp{"%t37", false, "ne", "i32", "%t36", "0"});
  branch_decision.insts.push_back(LirCastOp{"%t38", LirCastKind::ZExt, "i1", "%t37", "i32"});
  branch_decision.insts.push_back(LirStoreOp{"i32", "%t38", "%lv.tmp12"});
  branch_decision.terminator = LirBr{"flag_check_tail"};

  LirBlock flag_check_tail;
  flag_check_tail.id = LirBlockId{13};
  flag_check_tail.label = "flag_check_tail";
  flag_check_tail.insts.push_back(LirLoadOp{"%t39", "i32", "%lv.tmp12"});
  flag_check_tail.insts.push_back(LirCmpOp{"%t40", false, "ne", "i32", "%t39", "0"});
  flag_check_tail.insts.push_back(LirCastOp{"%t41", LirCastKind::ZExt, "i1", "%t40", "i32"});
  flag_check_tail.insts.push_back(LirStoreOp{"i32", "%t41", "%lv.tmp13"});
  flag_check_tail.terminator = LirBr{"flag_finalize"};

  LirBlock flag_finalize;
  flag_finalize.id = LirBlockId{14};
  flag_finalize.label = "flag_finalize";
  flag_finalize.insts.push_back(LirLoadOp{"%t42", "i32", "%lv.tmp13"});
  flag_finalize.insts.push_back(LirCmpOp{"%t43", false, "ne", "i32", "%t42", "0"});
  flag_finalize.insts.push_back(LirCastOp{"%t44", LirCastKind::ZExt, "i1", "%t43", "i32"});
  flag_finalize.insts.push_back(LirStoreOp{"i32", "%t44", "%lv.flag"});
  flag_finalize.terminator = LirBr{"flag_decision"};

  LirBlock flag_decision;
  flag_decision.id = LirBlockId{15};
  flag_decision.label = "flag_decision";
  flag_decision.insts.push_back(LirLoadOp{"%t45", "i32", "%lv.flag"});
  flag_decision.insts.push_back(LirCmpOp{"%t46", false, "ne", "i32", "%t45", "0"});
  flag_decision.terminator = LirCondBr{"%t46", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{16};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{17};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{18};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{19};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(check));
  function.blocks.push_back(std::move(branch));
  function.blocks.push_back(std::move(finish_check));
  function.blocks.push_back(std::move(flag_prep));
  function.blocks.push_back(std::move(flag_check));
  function.blocks.push_back(std::move(branch_check));
  function.blocks.push_back(std::move(final_check));
  function.blocks.push_back(std::move(decision_prep));
  function.blocks.push_back(std::move(decision));
  function.blocks.push_back(std::move(flag_store));
  function.blocks.push_back(std::move(flag_materialize));
  function.blocks.push_back(std::move(branch_decision));
  function.blocks.push_back(std::move(flag_check_tail));
  function.blocks.push_back(std::move(flag_finalize));
  function.blocks.push_back(std::move(flag_decision));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_seventeen_local_slot_constant_conditional_goto_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.state", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp0", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp1", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp2", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp3", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp4", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp5", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp6", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp7", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp8", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp9", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp10", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp11", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp12", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp13", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp14", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.flag", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.state"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.state"});
  entry.insts.push_back(LirCmpOp{"%t1", false, "eq", "i32", "%t0", "0"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::ZExt, "i1", "%t1", "i32"});
  entry.insts.push_back(LirStoreOp{"i32", "%t2", "%lv.tmp0"});
  entry.terminator = LirBr{"check"};

  LirBlock check;
  check.id = LirBlockId{1};
  check.label = "check";
  check.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.tmp0"});
  check.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  check.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i1", "%t4", "i32"});
  check.insts.push_back(LirStoreOp{"i32", "%t5", "%lv.tmp1"});
  check.terminator = LirBr{"branch"};

  LirBlock branch;
  branch.id = LirBlockId{2};
  branch.label = "branch";
  branch.insts.push_back(LirLoadOp{"%t6", "i32", "%lv.tmp1"});
  branch.insts.push_back(LirCmpOp{"%t7", false, "ne", "i32", "%t6", "0"});
  branch.insts.push_back(LirCastOp{"%t8", LirCastKind::ZExt, "i1", "%t7", "i32"});
  branch.insts.push_back(LirStoreOp{"i32", "%t8", "%lv.tmp2"});
  branch.terminator = LirBr{"finish_check"};

  LirBlock finish_check;
  finish_check.id = LirBlockId{3};
  finish_check.label = "finish_check";
  finish_check.insts.push_back(LirLoadOp{"%t9", "i32", "%lv.tmp2"});
  finish_check.insts.push_back(LirCmpOp{"%t10", false, "ne", "i32", "%t9", "0"});
  finish_check.insts.push_back(LirCastOp{"%t11", LirCastKind::ZExt, "i1", "%t10", "i32"});
  finish_check.insts.push_back(LirStoreOp{"i32", "%t11", "%lv.tmp3"});
  finish_check.terminator = LirBr{"flag_prep"};

  LirBlock flag_prep;
  flag_prep.id = LirBlockId{4};
  flag_prep.label = "flag_prep";
  flag_prep.insts.push_back(LirLoadOp{"%t12", "i32", "%lv.tmp3"});
  flag_prep.insts.push_back(LirCmpOp{"%t13", false, "ne", "i32", "%t12", "0"});
  flag_prep.insts.push_back(LirCastOp{"%t14", LirCastKind::ZExt, "i1", "%t13", "i32"});
  flag_prep.insts.push_back(LirStoreOp{"i32", "%t14", "%lv.tmp4"});
  flag_prep.terminator = LirBr{"flag_check"};

  LirBlock flag_check;
  flag_check.id = LirBlockId{5};
  flag_check.label = "flag_check";
  flag_check.insts.push_back(LirLoadOp{"%t15", "i32", "%lv.tmp4"});
  flag_check.insts.push_back(LirCmpOp{"%t16", false, "ne", "i32", "%t15", "0"});
  flag_check.insts.push_back(LirCastOp{"%t17", LirCastKind::ZExt, "i1", "%t16", "i32"});
  flag_check.insts.push_back(LirStoreOp{"i32", "%t17", "%lv.tmp5"});
  flag_check.terminator = LirBr{"branch_check"};

  LirBlock branch_check;
  branch_check.id = LirBlockId{6};
  branch_check.label = "branch_check";
  branch_check.insts.push_back(LirLoadOp{"%t18", "i32", "%lv.tmp5"});
  branch_check.insts.push_back(LirCmpOp{"%t19", false, "ne", "i32", "%t18", "0"});
  branch_check.insts.push_back(LirCastOp{"%t20", LirCastKind::ZExt, "i1", "%t19", "i32"});
  branch_check.insts.push_back(LirStoreOp{"i32", "%t20", "%lv.tmp6"});
  branch_check.terminator = LirBr{"final_check"};

  LirBlock final_check;
  final_check.id = LirBlockId{7};
  final_check.label = "final_check";
  final_check.insts.push_back(LirLoadOp{"%t21", "i32", "%lv.tmp6"});
  final_check.insts.push_back(LirCmpOp{"%t22", false, "ne", "i32", "%t21", "0"});
  final_check.insts.push_back(LirCastOp{"%t23", LirCastKind::ZExt, "i1", "%t22", "i32"});
  final_check.insts.push_back(LirStoreOp{"i32", "%t23", "%lv.tmp7"});
  final_check.terminator = LirBr{"decision_prep"};

  LirBlock decision_prep;
  decision_prep.id = LirBlockId{8};
  decision_prep.label = "decision_prep";
  decision_prep.insts.push_back(LirLoadOp{"%t24", "i32", "%lv.tmp7"});
  decision_prep.insts.push_back(LirCmpOp{"%t25", false, "ne", "i32", "%t24", "0"});
  decision_prep.insts.push_back(LirCastOp{"%t26", LirCastKind::ZExt, "i1", "%t25", "i32"});
  decision_prep.insts.push_back(LirStoreOp{"i32", "%t26", "%lv.tmp8"});
  decision_prep.terminator = LirBr{"decision"};

  LirBlock decision;
  decision.id = LirBlockId{9};
  decision.label = "decision";
  decision.insts.push_back(LirLoadOp{"%t27", "i32", "%lv.tmp8"});
  decision.insts.push_back(LirCmpOp{"%t28", false, "ne", "i32", "%t27", "0"});
  decision.insts.push_back(LirCastOp{"%t29", LirCastKind::ZExt, "i1", "%t28", "i32"});
  decision.insts.push_back(LirStoreOp{"i32", "%t29", "%lv.tmp9"});
  decision.terminator = LirBr{"flag_store"};

  LirBlock flag_store;
  flag_store.id = LirBlockId{10};
  flag_store.label = "flag_store";
  flag_store.insts.push_back(LirLoadOp{"%t30", "i32", "%lv.tmp9"});
  flag_store.insts.push_back(LirCmpOp{"%t31", false, "ne", "i32", "%t30", "0"});
  flag_store.insts.push_back(LirCastOp{"%t32", LirCastKind::ZExt, "i1", "%t31", "i32"});
  flag_store.insts.push_back(LirStoreOp{"i32", "%t32", "%lv.tmp10"});
  flag_store.terminator = LirBr{"flag_materialize"};

  LirBlock flag_materialize;
  flag_materialize.id = LirBlockId{11};
  flag_materialize.label = "flag_materialize";
  flag_materialize.insts.push_back(LirLoadOp{"%t33", "i32", "%lv.tmp10"});
  flag_materialize.insts.push_back(LirCmpOp{"%t34", false, "ne", "i32", "%t33", "0"});
  flag_materialize.insts.push_back(LirCastOp{"%t35", LirCastKind::ZExt, "i1", "%t34", "i32"});
  flag_materialize.insts.push_back(LirStoreOp{"i32", "%t35", "%lv.tmp11"});
  flag_materialize.terminator = LirBr{"branch_decision"};

  LirBlock branch_decision;
  branch_decision.id = LirBlockId{12};
  branch_decision.label = "branch_decision";
  branch_decision.insts.push_back(LirLoadOp{"%t36", "i32", "%lv.tmp11"});
  branch_decision.insts.push_back(LirCmpOp{"%t37", false, "ne", "i32", "%t36", "0"});
  branch_decision.insts.push_back(LirCastOp{"%t38", LirCastKind::ZExt, "i1", "%t37", "i32"});
  branch_decision.insts.push_back(LirStoreOp{"i32", "%t38", "%lv.tmp12"});
  branch_decision.terminator = LirBr{"flag_check_tail"};

  LirBlock flag_check_tail;
  flag_check_tail.id = LirBlockId{13};
  flag_check_tail.label = "flag_check_tail";
  flag_check_tail.insts.push_back(LirLoadOp{"%t39", "i32", "%lv.tmp12"});
  flag_check_tail.insts.push_back(LirCmpOp{"%t40", false, "ne", "i32", "%t39", "0"});
  flag_check_tail.insts.push_back(LirCastOp{"%t41", LirCastKind::ZExt, "i1", "%t40", "i32"});
  flag_check_tail.insts.push_back(LirStoreOp{"i32", "%t41", "%lv.tmp13"});
  flag_check_tail.terminator = LirBr{"flag_finalize"};

  LirBlock flag_finalize;
  flag_finalize.id = LirBlockId{14};
  flag_finalize.label = "flag_finalize";
  flag_finalize.insts.push_back(LirLoadOp{"%t42", "i32", "%lv.tmp13"});
  flag_finalize.insts.push_back(LirCmpOp{"%t43", false, "ne", "i32", "%t42", "0"});
  flag_finalize.insts.push_back(LirCastOp{"%t44", LirCastKind::ZExt, "i1", "%t43", "i32"});
  flag_finalize.insts.push_back(LirStoreOp{"i32", "%t44", "%lv.tmp14"});
  flag_finalize.terminator = LirBr{"flag_decision_tail"};

  LirBlock flag_decision_tail;
  flag_decision_tail.id = LirBlockId{15};
  flag_decision_tail.label = "flag_decision_tail";
  flag_decision_tail.insts.push_back(LirLoadOp{"%t45", "i32", "%lv.tmp14"});
  flag_decision_tail.insts.push_back(LirCmpOp{"%t46", false, "ne", "i32", "%t45", "0"});
  flag_decision_tail.insts.push_back(LirCastOp{"%t47", LirCastKind::ZExt, "i1", "%t46", "i32"});
  flag_decision_tail.insts.push_back(LirStoreOp{"i32", "%t47", "%lv.flag"});
  flag_decision_tail.terminator = LirBr{"flag_decision"};

  LirBlock flag_decision;
  flag_decision.id = LirBlockId{16};
  flag_decision.label = "flag_decision";
  flag_decision.insts.push_back(LirLoadOp{"%t48", "i32", "%lv.flag"});
  flag_decision.insts.push_back(LirCmpOp{"%t49", false, "ne", "i32", "%t48", "0"});
  flag_decision.terminator = LirCondBr{"%t49", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{17};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{18};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{19};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{20};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(check));
  function.blocks.push_back(std::move(branch));
  function.blocks.push_back(std::move(finish_check));
  function.blocks.push_back(std::move(flag_prep));
  function.blocks.push_back(std::move(flag_check));
  function.blocks.push_back(std::move(branch_check));
  function.blocks.push_back(std::move(final_check));
  function.blocks.push_back(std::move(decision_prep));
  function.blocks.push_back(std::move(decision));
  function.blocks.push_back(std::move(flag_store));
  function.blocks.push_back(std::move(flag_materialize));
  function.blocks.push_back(std::move(branch_decision));
  function.blocks.push_back(std::move(flag_check_tail));
  function.blocks.push_back(std::move(flag_finalize));
  function.blocks.push_back(std::move(flag_decision_tail));
  function.blocks.push_back(std::move(flag_decision));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule
make_eighteen_local_slot_constant_conditional_goto_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.state", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp0", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp1", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp2", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp3", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp4", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp5", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp6", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp7", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp8", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp9", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp10", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp11", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp12", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp13", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp14", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.tmp15", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.flag", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.state"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.state"});
  entry.insts.push_back(LirCmpOp{"%t1", false, "eq", "i32", "%t0", "0"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::ZExt, "i1", "%t1", "i32"});
  entry.insts.push_back(LirStoreOp{"i32", "%t2", "%lv.tmp0"});
  entry.terminator = LirBr{"check"};

  LirBlock check;
  check.id = LirBlockId{1};
  check.label = "check";
  check.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.tmp0"});
  check.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  check.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i1", "%t4", "i32"});
  check.insts.push_back(LirStoreOp{"i32", "%t5", "%lv.tmp1"});
  check.terminator = LirBr{"branch"};

  LirBlock branch;
  branch.id = LirBlockId{2};
  branch.label = "branch";
  branch.insts.push_back(LirLoadOp{"%t6", "i32", "%lv.tmp1"});
  branch.insts.push_back(LirCmpOp{"%t7", false, "ne", "i32", "%t6", "0"});
  branch.insts.push_back(LirCastOp{"%t8", LirCastKind::ZExt, "i1", "%t7", "i32"});
  branch.insts.push_back(LirStoreOp{"i32", "%t8", "%lv.tmp2"});
  branch.terminator = LirBr{"finish_check"};

  LirBlock finish_check;
  finish_check.id = LirBlockId{3};
  finish_check.label = "finish_check";
  finish_check.insts.push_back(LirLoadOp{"%t9", "i32", "%lv.tmp2"});
  finish_check.insts.push_back(LirCmpOp{"%t10", false, "ne", "i32", "%t9", "0"});
  finish_check.insts.push_back(LirCastOp{"%t11", LirCastKind::ZExt, "i1", "%t10", "i32"});
  finish_check.insts.push_back(LirStoreOp{"i32", "%t11", "%lv.tmp3"});
  finish_check.terminator = LirBr{"flag_prep"};

  LirBlock flag_prep;
  flag_prep.id = LirBlockId{4};
  flag_prep.label = "flag_prep";
  flag_prep.insts.push_back(LirLoadOp{"%t12", "i32", "%lv.tmp3"});
  flag_prep.insts.push_back(LirCmpOp{"%t13", false, "ne", "i32", "%t12", "0"});
  flag_prep.insts.push_back(LirCastOp{"%t14", LirCastKind::ZExt, "i1", "%t13", "i32"});
  flag_prep.insts.push_back(LirStoreOp{"i32", "%t14", "%lv.tmp4"});
  flag_prep.terminator = LirBr{"flag_check"};

  LirBlock flag_check;
  flag_check.id = LirBlockId{5};
  flag_check.label = "flag_check";
  flag_check.insts.push_back(LirLoadOp{"%t15", "i32", "%lv.tmp4"});
  flag_check.insts.push_back(LirCmpOp{"%t16", false, "ne", "i32", "%t15", "0"});
  flag_check.insts.push_back(LirCastOp{"%t17", LirCastKind::ZExt, "i1", "%t16", "i32"});
  flag_check.insts.push_back(LirStoreOp{"i32", "%t17", "%lv.tmp5"});
  flag_check.terminator = LirBr{"branch_check"};

  LirBlock branch_check;
  branch_check.id = LirBlockId{6};
  branch_check.label = "branch_check";
  branch_check.insts.push_back(LirLoadOp{"%t18", "i32", "%lv.tmp5"});
  branch_check.insts.push_back(LirCmpOp{"%t19", false, "ne", "i32", "%t18", "0"});
  branch_check.insts.push_back(LirCastOp{"%t20", LirCastKind::ZExt, "i1", "%t19", "i32"});
  branch_check.insts.push_back(LirStoreOp{"i32", "%t20", "%lv.tmp6"});
  branch_check.terminator = LirBr{"final_check"};

  LirBlock final_check;
  final_check.id = LirBlockId{7};
  final_check.label = "final_check";
  final_check.insts.push_back(LirLoadOp{"%t21", "i32", "%lv.tmp6"});
  final_check.insts.push_back(LirCmpOp{"%t22", false, "ne", "i32", "%t21", "0"});
  final_check.insts.push_back(LirCastOp{"%t23", LirCastKind::ZExt, "i1", "%t22", "i32"});
  final_check.insts.push_back(LirStoreOp{"i32", "%t23", "%lv.tmp7"});
  final_check.terminator = LirBr{"decision_prep"};

  LirBlock decision_prep;
  decision_prep.id = LirBlockId{8};
  decision_prep.label = "decision_prep";
  decision_prep.insts.push_back(LirLoadOp{"%t24", "i32", "%lv.tmp7"});
  decision_prep.insts.push_back(LirCmpOp{"%t25", false, "ne", "i32", "%t24", "0"});
  decision_prep.insts.push_back(LirCastOp{"%t26", LirCastKind::ZExt, "i1", "%t25", "i32"});
  decision_prep.insts.push_back(LirStoreOp{"i32", "%t26", "%lv.tmp8"});
  decision_prep.terminator = LirBr{"decision"};

  LirBlock decision;
  decision.id = LirBlockId{9};
  decision.label = "decision";
  decision.insts.push_back(LirLoadOp{"%t27", "i32", "%lv.tmp8"});
  decision.insts.push_back(LirCmpOp{"%t28", false, "ne", "i32", "%t27", "0"});
  decision.insts.push_back(LirCastOp{"%t29", LirCastKind::ZExt, "i1", "%t28", "i32"});
  decision.insts.push_back(LirStoreOp{"i32", "%t29", "%lv.tmp9"});
  decision.terminator = LirBr{"flag_store"};

  LirBlock flag_store;
  flag_store.id = LirBlockId{10};
  flag_store.label = "flag_store";
  flag_store.insts.push_back(LirLoadOp{"%t30", "i32", "%lv.tmp9"});
  flag_store.insts.push_back(LirCmpOp{"%t31", false, "ne", "i32", "%t30", "0"});
  flag_store.insts.push_back(LirCastOp{"%t32", LirCastKind::ZExt, "i1", "%t31", "i32"});
  flag_store.insts.push_back(LirStoreOp{"i32", "%t32", "%lv.tmp10"});
  flag_store.terminator = LirBr{"flag_materialize"};

  LirBlock flag_materialize;
  flag_materialize.id = LirBlockId{11};
  flag_materialize.label = "flag_materialize";
  flag_materialize.insts.push_back(LirLoadOp{"%t33", "i32", "%lv.tmp10"});
  flag_materialize.insts.push_back(LirCmpOp{"%t34", false, "ne", "i32", "%t33", "0"});
  flag_materialize.insts.push_back(LirCastOp{"%t35", LirCastKind::ZExt, "i1", "%t34", "i32"});
  flag_materialize.insts.push_back(LirStoreOp{"i32", "%t35", "%lv.tmp11"});
  flag_materialize.terminator = LirBr{"branch_decision"};

  LirBlock branch_decision;
  branch_decision.id = LirBlockId{12};
  branch_decision.label = "branch_decision";
  branch_decision.insts.push_back(LirLoadOp{"%t36", "i32", "%lv.tmp11"});
  branch_decision.insts.push_back(LirCmpOp{"%t37", false, "ne", "i32", "%t36", "0"});
  branch_decision.insts.push_back(LirCastOp{"%t38", LirCastKind::ZExt, "i1", "%t37", "i32"});
  branch_decision.insts.push_back(LirStoreOp{"i32", "%t38", "%lv.tmp12"});
  branch_decision.terminator = LirBr{"flag_check_tail"};

  LirBlock flag_check_tail;
  flag_check_tail.id = LirBlockId{13};
  flag_check_tail.label = "flag_check_tail";
  flag_check_tail.insts.push_back(LirLoadOp{"%t39", "i32", "%lv.tmp12"});
  flag_check_tail.insts.push_back(LirCmpOp{"%t40", false, "ne", "i32", "%t39", "0"});
  flag_check_tail.insts.push_back(LirCastOp{"%t41", LirCastKind::ZExt, "i1", "%t40", "i32"});
  flag_check_tail.insts.push_back(LirStoreOp{"i32", "%t41", "%lv.tmp13"});
  flag_check_tail.terminator = LirBr{"flag_finalize"};

  LirBlock flag_finalize;
  flag_finalize.id = LirBlockId{14};
  flag_finalize.label = "flag_finalize";
  flag_finalize.insts.push_back(LirLoadOp{"%t42", "i32", "%lv.tmp13"});
  flag_finalize.insts.push_back(LirCmpOp{"%t43", false, "ne", "i32", "%t42", "0"});
  flag_finalize.insts.push_back(LirCastOp{"%t44", LirCastKind::ZExt, "i1", "%t43", "i32"});
  flag_finalize.insts.push_back(LirStoreOp{"i32", "%t44", "%lv.tmp14"});
  flag_finalize.terminator = LirBr{"flag_decision_tail"};

  LirBlock flag_decision_tail;
  flag_decision_tail.id = LirBlockId{15};
  flag_decision_tail.label = "flag_decision_tail";
  flag_decision_tail.insts.push_back(LirLoadOp{"%t45", "i32", "%lv.tmp14"});
  flag_decision_tail.insts.push_back(LirCmpOp{"%t46", false, "ne", "i32", "%t45", "0"});
  flag_decision_tail.insts.push_back(LirCastOp{"%t47", LirCastKind::ZExt, "i1", "%t46", "i32"});
  flag_decision_tail.insts.push_back(LirStoreOp{"i32", "%t47", "%lv.tmp15"});
  flag_decision_tail.terminator = LirBr{"flag_materialize_tail"};

  LirBlock flag_materialize_tail;
  flag_materialize_tail.id = LirBlockId{16};
  flag_materialize_tail.label = "flag_materialize_tail";
  flag_materialize_tail.insts.push_back(LirLoadOp{"%t48", "i32", "%lv.tmp15"});
  flag_materialize_tail.insts.push_back(LirCmpOp{"%t49", false, "ne", "i32", "%t48", "0"});
  flag_materialize_tail.insts.push_back(LirCastOp{"%t50", LirCastKind::ZExt, "i1", "%t49", "i32"});
  flag_materialize_tail.insts.push_back(LirStoreOp{"i32", "%t50", "%lv.flag"});
  flag_materialize_tail.terminator = LirBr{"flag_decision"};

  LirBlock flag_decision;
  flag_decision.id = LirBlockId{17};
  flag_decision.label = "flag_decision";
  flag_decision.insts.push_back(LirLoadOp{"%t51", "i32", "%lv.flag"});
  flag_decision.insts.push_back(LirCmpOp{"%t52", false, "ne", "i32", "%t51", "0"});
  flag_decision.terminator = LirCondBr{"%t52", "then_path", "else_path"};

  LirBlock then_path;
  then_path.id = LirBlockId{18};
  then_path.label = "then_path";
  then_path.terminator = LirBr{"finish"};

  LirBlock else_path;
  else_path.id = LirBlockId{19};
  else_path.label = "else_path";
  else_path.terminator = LirBr{"dead_end"};

  LirBlock dead_end;
  dead_end.id = LirBlockId{20};
  dead_end.label = "dead_end";
  dead_end.terminator = LirBr{"finish"};

  LirBlock finish;
  finish.id = LirBlockId{21};
  finish.label = "finish";
  finish.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(check));
  function.blocks.push_back(std::move(branch));
  function.blocks.push_back(std::move(finish_check));
  function.blocks.push_back(std::move(flag_prep));
  function.blocks.push_back(std::move(flag_check));
  function.blocks.push_back(std::move(branch_check));
  function.blocks.push_back(std::move(final_check));
  function.blocks.push_back(std::move(decision_prep));
  function.blocks.push_back(std::move(decision));
  function.blocks.push_back(std::move(flag_store));
  function.blocks.push_back(std::move(flag_materialize));
  function.blocks.push_back(std::move(branch_decision));
  function.blocks.push_back(std::move(flag_check_tail));
  function.blocks.push_back(std::move(flag_finalize));
  function.blocks.push_back(std::move(flag_decision_tail));
  function.blocks.push_back(std::move(flag_materialize_tail));
  function.blocks.push_back(std::move(flag_decision));
  function.blocks.push_back(std::move(then_path));
  function.blocks.push_back(std::move(else_path));
  function.blocks.push_back(std::move(dead_end));
  function.blocks.push_back(std::move(finish));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_countdown_while_return_module() {
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
  entry.insts.push_back(LirStoreOp{"i32", "50", "%lv.x"});
  entry.terminator = LirBr{"block_1"};

  LirBlock block_1;
  block_1.id = LirBlockId{1};
  block_1.label = "block_1";
  block_1.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  block_1.insts.push_back(LirCmpOp{"%t1", false, "ne", "i32", "%t0", "0"});
  block_1.terminator = LirCondBr{"%t1", "block_2", "block_3"};

  LirBlock block_2;
  block_2.id = LirBlockId{2};
  block_2.label = "block_2";
  block_2.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  block_2.insts.push_back(LirBinOp{"%t3", "sub", "i32", "%t2", "1"});
  block_2.insts.push_back(LirStoreOp{"i32", "%t3", "%lv.x"});
  block_2.terminator = LirBr{"block_1"};

  LirBlock block_3;
  block_3.id = LirBlockId{3};
  block_3.label = "block_3";
  block_3.insts.push_back(LirLoadOp{"%t4", "i32", "%lv.x"});
  block_3.terminator = LirRet{std::string("%t4"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  function.blocks.push_back(std::move(block_3));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_countdown_do_while_return_module() {
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
  entry.insts.push_back(LirStoreOp{"i32", "50", "%lv.x"});
  entry.terminator = LirBr{"block_1"};

  LirBlock block_1;
  block_1.id = LirBlockId{1};
  block_1.label = "block_1";
  block_1.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  block_1.insts.push_back(LirBinOp{"%t1", "sub", "i32", "%t0", "1"});
  block_1.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.x"});
  block_1.terminator = LirBr{"block_2"};

  LirBlock block_2;
  block_2.id = LirBlockId{2};
  block_2.label = "block_2";
  block_2.terminator = LirBr{"dowhile.cond.1"};

  LirBlock cond;
  cond.id = LirBlockId{3};
  cond.label = "dowhile.cond.1";
  cond.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  cond.insts.push_back(LirCmpOp{"%t3", false, "ne", "i32", "%t2", "0"});
  cond.terminator = LirCondBr{"%t3", "block_1", "block_3"};

  LirBlock block_3;
  block_3.id = LirBlockId{4};
  block_3.label = "block_3";
  block_3.insts.push_back(LirLoadOp{"%t4", "i32", "%lv.x"});
  block_3.terminator = LirRet{std::string("%t4"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  function.blocks.push_back(std::move(cond));
  function.blocks.push_back(std::move(block_3));

  module.functions.push_back(std::move(function));
  return module;
}

inline std::string make_temp_output_path(const std::string& stem) {
  const auto base = std::filesystem::temp_directory_path();
  return (base / (stem + "_" + std::to_string(::getpid()) + ".o")).string();
}

inline std::string make_temp_path(const std::string& stem, const std::string& extension) {
  const auto base = std::filesystem::temp_directory_path();
  return (base / (stem + "_" + std::to_string(::getpid()) + extension)).string();
}

inline std::vector<std::uint8_t> read_file_bytes(const std::string& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    fail("failed to open file: " + path);
  }
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input),
                                   std::istreambuf_iterator<char>());
}

inline std::vector<std::uint8_t> make_minimal_relocation_object_fixture() {
  using namespace c4c::backend::elf;

  constexpr std::uint32_t kCall26Reloc = 279;
  constexpr std::size_t kElfHeaderSize = 64;
  constexpr std::size_t kSectionHeaderSize = 64;

  std::vector<std::uint8_t> text_bytes = {
      0x1f, 0x20, 0x03, 0xd5,
      0xc0, 0x03, 0x5f, 0xd6,
  };

  std::string strtab;
  strtab.push_back('\0');
  const auto main_name = static_cast<std::uint32_t>(strtab.size());
  strtab += "main";
  strtab.push_back('\0');
  const auto helper_name = static_cast<std::uint32_t>(strtab.size());
  strtab += "helper_ext";
  strtab.push_back('\0');

  std::string shstrtab;
  shstrtab.push_back('\0');
  const auto text_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".text";
  shstrtab.push_back('\0');
  const auto rela_text_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".rela.text";
  shstrtab.push_back('\0');
  const auto symtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".symtab";
  shstrtab.push_back('\0');
  const auto strtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".strtab";
  shstrtab.push_back('\0');
  const auto shstrtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".shstrtab";
  shstrtab.push_back('\0');

  std::vector<std::uint8_t> rela_text;
  append_u64(rela_text, 4);
  append_u64(rela_text, (static_cast<std::uint64_t>(3) << 32) | kCall26Reloc);
  append_i64(rela_text, 0);

  std::vector<std::uint8_t> symtab;
  append_zeroes(symtab, 24);
  append_u32(symtab, 0);
  symtab.push_back(symbol_info(STB_LOCAL, STT_SECTION));
  symtab.push_back(0);
  append_u16(symtab, 1);
  append_u64(symtab, 0);
  append_u64(symtab, 0);
  append_u32(symtab, main_name);
  symtab.push_back(symbol_info(STB_GLOBAL, STT_FUNC));
  symtab.push_back(0);
  append_u16(symtab, 1);
  append_u64(symtab, 0);
  append_u64(symtab, text_bytes.size());
  append_u32(symtab, helper_name);
  symtab.push_back(symbol_info(STB_GLOBAL, STT_NOTYPE));
  symtab.push_back(0);
  append_u16(symtab, SHN_UNDEF);
  append_u64(symtab, 0);
  append_u64(symtab, 0);

  std::size_t offset = kElfHeaderSize;
  const auto text_offset = offset;
  offset += text_bytes.size();
  offset = align_up(offset, 8);

  const auto rela_text_offset = offset;
  offset += rela_text.size();
  offset = align_up(offset, 8);

  const auto symtab_offset = offset;
  offset += symtab.size();

  const auto strtab_offset = offset;
  offset += strtab.size();

  const auto shstrtab_offset = offset;
  offset += shstrtab.size();
  offset = align_up(offset, 8);

  const auto section_header_offset = offset;

  std::vector<std::uint8_t> out;
  out.reserve(section_header_offset + 6 * kSectionHeaderSize);
  out.insert(out.end(), ELF_MAGIC.begin(), ELF_MAGIC.end());
  out.push_back(ELFCLASS64);
  out.push_back(ELFDATA2LSB);
  out.push_back(1);
  out.push_back(0);
  out.push_back(0);
  append_zeroes(out, 7);
  append_u16(out, ET_REL);
  append_u16(out, EM_AARCH64);
  append_u32(out, 1);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, section_header_offset);
  append_u32(out, 0);
  append_u16(out, kElfHeaderSize);
  append_u16(out, 0);
  append_u16(out, 0);
  append_u16(out, kSectionHeaderSize);
  append_u16(out, 6);
  append_u16(out, 5);

  out.insert(out.end(), text_bytes.begin(), text_bytes.end());
  append_zeroes(out, align_up(out.size(), 8) - out.size());
  out.insert(out.end(), rela_text.begin(), rela_text.end());
  append_zeroes(out, align_up(out.size(), 8) - out.size());
  out.insert(out.end(), symtab.begin(), symtab.end());
  out.insert(out.end(), strtab.begin(), strtab.end());
  out.insert(out.end(), shstrtab.begin(), shstrtab.end());
  append_zeroes(out, align_up(out.size(), 8) - out.size());

  append_zeroes(out, kSectionHeaderSize);

  append_u32(out, text_name);
  append_u32(out, SHT_PROGBITS);
  append_u64(out, SHF_ALLOC | SHF_EXECINSTR);
  append_u64(out, 0);
  append_u64(out, text_offset);
  append_u64(out, text_bytes.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 4);
  append_u64(out, 0);

  append_u32(out, rela_text_name);
  append_u32(out, SHT_RELA);
  append_u64(out, SHF_INFO_LINK);
  append_u64(out, 0);
  append_u64(out, rela_text_offset);
  append_u64(out, rela_text.size());
  append_u32(out, 3);
  append_u32(out, 1);
  append_u64(out, 8);
  append_u64(out, 24);

  append_u32(out, symtab_name);
  append_u32(out, SHT_SYMTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, symtab_offset);
  append_u64(out, symtab.size());
  append_u32(out, 4);
  append_u32(out, 2);
  append_u64(out, 8);
  append_u64(out, 24);

  append_u32(out, strtab_name);
  append_u32(out, SHT_STRTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, strtab_offset);
  append_u64(out, strtab.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 1);
  append_u64(out, 0);

  append_u32(out, shstrtab_name);
  append_u32(out, SHT_STRTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, shstrtab_offset);
  append_u64(out, shstrtab.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 1);
  append_u64(out, 0);

  return out;
}

std::vector<std::uint8_t> make_single_member_archive_fixture(
    const std::vector<std::uint8_t>& member,
    const std::string& member_name_with_trailing_slash) {

  std::vector<std::uint8_t> archive;
  archive.insert(archive.end(), {'!', '<', 'a', 'r', 'c', 'h', '>', '\n'});

  const auto append_text = [&](const std::string& text) {
    archive.insert(archive.end(), text.begin(), text.end());
  };

  append_text(format_ar_field(member_name_with_trailing_slash, 16));
  append_text(format_ar_field("0", 12));
  append_text(format_ar_field("0", 6));
  append_text(format_ar_field("0", 6));
  append_text(format_ar_field("644", 8));
  append_text(format_ar_field(std::to_string(member.size()), 10));
  archive.push_back('`');
  archive.push_back('\n');
  archive.insert(archive.end(), member.begin(), member.end());
  if ((archive.size() & 1u) != 0u) archive.push_back('\n');

  return archive;
}

std::vector<std::uint8_t> make_single_member_archive_fixture() {
  return make_single_member_archive_fixture(make_minimal_relocation_object_fixture(),
                                            "minimal_reloc.o/");
}

inline std::vector<std::uint8_t> make_minimal_helper_definition_object_fixture() {
  using namespace c4c::backend::elf;

  constexpr std::size_t kElfHeaderSize = 64;
  constexpr std::size_t kSectionHeaderSize = 64;

  std::vector<std::uint8_t> text_bytes = {
      0x40, 0x05, 0x80, 0x52,
      0xc0, 0x03, 0x5f, 0xd6,
  };

  std::string strtab;
  strtab.push_back('\0');
  const auto helper_name = static_cast<std::uint32_t>(strtab.size());
  strtab += "helper_ext";
  strtab.push_back('\0');

  std::string shstrtab;
  shstrtab.push_back('\0');
  const auto text_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".text";
  shstrtab.push_back('\0');
  const auto symtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".symtab";
  shstrtab.push_back('\0');
  const auto strtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".strtab";
  shstrtab.push_back('\0');
  const auto shstrtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".shstrtab";
  shstrtab.push_back('\0');

  std::vector<std::uint8_t> symtab;
  append_zeroes(symtab, 24);
  append_u32(symtab, 0);
  symtab.push_back(symbol_info(STB_LOCAL, STT_SECTION));
  symtab.push_back(0);
  append_u16(symtab, 1);
  append_u64(symtab, 0);
  append_u64(symtab, 0);
  append_u32(symtab, helper_name);
  symtab.push_back(symbol_info(STB_GLOBAL, STT_FUNC));
  symtab.push_back(0);
  append_u16(symtab, 1);
  append_u64(symtab, 0);
  append_u64(symtab, text_bytes.size());

  std::size_t offset = kElfHeaderSize;
  const auto text_offset = offset;
  offset += text_bytes.size();
  offset = align_up(offset, 8);

  const auto symtab_offset = offset;
  offset += symtab.size();

  const auto strtab_offset = offset;
  offset += strtab.size();

  const auto shstrtab_offset = offset;
  offset += shstrtab.size();
  offset = align_up(offset, 8);

  const auto section_header_offset = offset;

  std::vector<std::uint8_t> out;
  out.reserve(section_header_offset + 5 * kSectionHeaderSize);
  out.insert(out.end(), ELF_MAGIC.begin(), ELF_MAGIC.end());
  out.push_back(ELFCLASS64);
  out.push_back(ELFDATA2LSB);
  out.push_back(1);
  out.push_back(0);
  out.push_back(0);
  append_zeroes(out, 7);
  append_u16(out, ET_REL);
  append_u16(out, EM_AARCH64);
  append_u32(out, 1);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, section_header_offset);
  append_u32(out, 0);
  append_u16(out, kElfHeaderSize);
  append_u16(out, 0);
  append_u16(out, 0);
  append_u16(out, kSectionHeaderSize);
  append_u16(out, 5);
  append_u16(out, 4);

  out.insert(out.end(), text_bytes.begin(), text_bytes.end());
  append_zeroes(out, align_up(out.size(), 8) - out.size());
  out.insert(out.end(), symtab.begin(), symtab.end());
  out.insert(out.end(), strtab.begin(), strtab.end());
  out.insert(out.end(), shstrtab.begin(), shstrtab.end());
  append_zeroes(out, align_up(out.size(), 8) - out.size());

  append_zeroes(out, kSectionHeaderSize);

  append_u32(out, text_name);
  append_u32(out, SHT_PROGBITS);
  append_u64(out, SHF_ALLOC | SHF_EXECINSTR);
  append_u64(out, 0);
  append_u64(out, text_offset);
  append_u64(out, text_bytes.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 4);
  append_u64(out, 0);

  append_u32(out, symtab_name);
  append_u32(out, SHT_SYMTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, symtab_offset);
  append_u64(out, symtab.size());
  append_u32(out, 3);
  append_u32(out, 2);
  append_u64(out, 8);
  append_u64(out, 24);

  append_u32(out, strtab_name);
  append_u32(out, SHT_STRTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, strtab_offset);
  append_u64(out, strtab.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 1);
  append_u64(out, 0);

  append_u32(out, shstrtab_name);
  append_u32(out, SHT_STRTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, shstrtab_offset);
  append_u64(out, shstrtab.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 1);
  append_u64(out, 0);

  return out;
}


inline c4c::codegen::lir::LirModule make_return_zero_module() {
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

inline c4c::codegen::lir::LirModule make_return_zero_module_with_unused_decl() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();

  LirFunction decl;
  decl.name = "foo";
  decl.signature_text = "declare i32 @foo()\n";
  decl.is_declaration = true;

  module.functions.insert(module.functions.begin(), std::move(decl));
  return module;
}

inline c4c::codegen::lir::LirModule make_return_add_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  auto& function = module.functions.front();
  auto& block = function.blocks.front();
  block.insts.push_back(LirBinOp{"%t0", "add", "i32", "2", "3"});
  block.terminator = LirRet{std::string("%t0"), "i32"};
  return module;
}

inline c4c::codegen::lir::LirModule make_return_sub_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  auto& function = module.functions.front();
  auto& block = function.blocks.front();
  block.insts.push_back(LirBinOp{"%t0", "sub", "i32", "3", "3"});
  block.terminator = LirRet{std::string("%t0"), "i32"};
  return module;
}

inline c4c::codegen::lir::LirModule make_typed_return_add_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_zero_module();
  auto& function = module.functions.front();
  auto& block = function.blocks.front();
  block.insts.push_back(LirBinOp{"%t0", LirBinaryOpcode::Add, "i32", "2", "3"});
  block.terminator = LirRet{std::string("%t0"), "i32"};
  return module;
}

inline c4c::codegen::lir::LirModule make_void_return_module() {
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

inline c4c::codegen::lir::LirModule make_declaration_module() {
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

inline c4c::codegen::lir::LirModule make_call_crossing_interval_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

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
  entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "2", "3"});
  entry.insts.push_back(LirCallOp{"%t1", "i32", "@helper", "", "i32 %t0"});
  entry.insts.push_back(LirBinOp{"%t2", "add", "i32", "%t0", "%t1"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};
  main_fn.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(decl));
  module.functions.push_back(std::move(main_fn));
  return module;
}

inline c4c::codegen::lir::LirModule make_typed_call_crossing_direct_call_module() {
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
  helper_entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "1"});
  helper_entry.terminator = LirRet{std::string("%t0"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "2", "3"});
  entry.insts.push_back(LirCallOp{"%t1", "i32", "@add_one", "(i32)", "i32 %t0"});
  entry.insts.push_back(LirBinOp{"%t2", "add", "i32", "%t0", "%t1"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};
  main_fn.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

inline c4c::codegen::lir::LirModule make_typed_call_crossing_direct_call_with_spacing_module() {
  auto module = make_typed_call_crossing_direct_call_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts[1]);
  call.callee_type_suffix = "( i32 )";
  call.args_str = "  i32   %t0  ";
  return module;
}

inline c4c::codegen::lir::LirModule make_interval_phi_join_module() {
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
  entry.terminator = LirCondBr{"%t0", "then", "else"};

  LirBlock then_block;
  then_block.id = LirBlockId{1};
  then_block.label = "then";
  then_block.insts.push_back(LirBinOp{"%t1", "add", "i32", "10", "1"});
  then_block.terminator = LirBr{"join"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.insts.push_back(LirBinOp{"%t2", "add", "i32", "20", "2"});
  else_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(LirPhiOp{"%t3", "i32", {{"%t1", "then"}, {"%t2", "else"}}});
  join_block.insts.push_back(LirBinOp{"%t4", "add", "i32", "%t3", "5"});
  join_block.terminator = LirRet{std::string("%t4"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_non_overlapping_interval_module() {
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
  entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "1", "2"});
  entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%t0", "3"});
  entry.insts.push_back(LirBinOp{"%t2", "add", "i32", "4", "5"});
  entry.insts.push_back(LirBinOp{"%t3", "add", "i32", "%t2", "6"});
  entry.terminator = LirRet{std::string("%t3"), "i32"};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_overlapping_interval_module() {
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
  entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "1", "2"});
  entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "4", "5"});
  entry.insts.push_back(LirBinOp{"%t2", "add", "i32", "%t0", "%t1"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_conditional_return_module() {
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

inline c4c::codegen::lir::LirModule make_conditional_phi_join_module() {
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
  then_block.terminator = LirBr{"join"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(LirPhiOp{
      "%t3",
      "i32",
      {
          {"7", "then"},
          {"9", "else"},
      },
  });
  join_block.terminator = LirRet{std::string("%t3"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  function.blocks.push_back(std::move(join_block));
  return module;
}

inline c4c::codegen::lir::LirModule make_conditional_phi_join_add_module() {
  using namespace c4c::codegen::lir;

  auto module = make_conditional_phi_join_module();
  auto& function = module.functions.front();
  auto& join_block = function.blocks.back();
  join_block.insts.push_back(LirBinOp{"%t4", "add", "i32", "%t3", "5"});
  join_block.terminator = LirRet{std::string("%t4"), "i32"};
  return module;
}

inline c4c::codegen::lir::LirModule make_conditional_phi_join_predecessor_add_module() {
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
  then_block.insts.push_back(LirBinOp{"%t3", "add", "i32", "7", "5"});
  then_block.terminator = LirBr{"join"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.insts.push_back(LirBinOp{"%t4", "add", "i32", "9", "4"});
  else_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(LirPhiOp{
      "%t5",
      "i32",
      {
          {"%t3", "then"},
          {"%t4", "else"},
      },
  });
  join_block.terminator = LirRet{std::string("%t5"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  function.blocks.push_back(std::move(join_block));
  return module;
}

inline c4c::codegen::lir::LirModule make_conditional_phi_join_predecessor_sub_module() {
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
  then_block.insts.push_back(LirBinOp{"%t3", "sub", "i32", "12", "5"});
  then_block.terminator = LirBr{"join"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.insts.push_back(LirBinOp{"%t4", "sub", "i32", "15", "6"});
  else_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(LirPhiOp{
      "%t5",
      "i32",
      {
          {"%t3", "then"},
          {"%t4", "else"},
      },
  });
  join_block.terminator = LirRet{std::string("%t5"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  function.blocks.push_back(std::move(join_block));
  return module;
}

inline c4c::codegen::lir::LirModule make_conditional_phi_join_mixed_predecessor_add_module() {
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
  then_block.insts.push_back(LirBinOp{"%t3", "add", "i32", "7", "5"});
  then_block.terminator = LirBr{"join"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(LirPhiOp{
      "%t4",
      "i32",
      {
          {"%t3", "then"},
          {"9", "else"},
      },
  });
  join_block.terminator = LirRet{std::string("%t4"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  function.blocks.push_back(std::move(join_block));
  return module;
}

inline c4c::codegen::lir::LirModule make_conditional_phi_join_mixed_predecessor_add_post_join_add_module() {
  using namespace c4c::codegen::lir;

  auto module = make_conditional_phi_join_mixed_predecessor_add_module();
  auto& function = module.functions.front();
  auto& join_block = function.blocks.back();
  join_block.insts.push_back(LirBinOp{"%t5", "add", "i32", "%t4", "6"});
  join_block.terminator = LirRet{std::string("%t5"), "i32"};
  return module;
}

inline c4c::codegen::lir::LirModule make_conditional_phi_join_mixed_predecessor_sub_post_join_add_module() {
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
  then_block.insts.push_back(LirBinOp{"%t3", "sub", "i32", "12", "5"});
  then_block.terminator = LirBr{"join"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(LirPhiOp{
      "%t4",
      "i32",
      {
          {"%t3", "then"},
          {"9", "else"},
      },
  });
  join_block.insts.push_back(LirBinOp{"%t5", "add", "i32", "%t4", "6"});
  join_block.terminator = LirRet{std::string("%t5"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  function.blocks.push_back(std::move(join_block));
  return module;
}

inline c4c::codegen::lir::LirModule
make_conditional_phi_join_mixed_predecessor_add_sub_chain_post_join_add_module() {
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
  then_block.insts.push_back(LirBinOp{"%t3", "add", "i32", "20", "5"});
  then_block.insts.push_back(LirBinOp{"%t4", "sub", "i32", "%t3", "3"});
  then_block.terminator = LirBr{"join"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(LirPhiOp{
      "%t5",
      "i32",
      {
          {"%t4", "then"},
          {"9", "else"},
      },
  });
  join_block.insts.push_back(LirBinOp{"%t6", "add", "i32", "%t5", "6"});
  join_block.terminator = LirRet{std::string("%t6"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  function.blocks.push_back(std::move(join_block));
  return module;
}

inline c4c::codegen::lir::LirModule
make_conditional_phi_join_mixed_predecessor_chain_and_add_post_join_add_module() {
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
  then_block.insts.push_back(LirBinOp{"%t3", "add", "i32", "20", "5"});
  then_block.insts.push_back(LirBinOp{"%t4", "sub", "i32", "%t3", "3"});
  then_block.terminator = LirBr{"join"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.insts.push_back(LirBinOp{"%t5", "add", "i32", "9", "4"});
  else_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(LirPhiOp{
      "%t6",
      "i32",
      {
          {"%t4", "then"},
          {"%t5", "else"},
      },
  });
  join_block.insts.push_back(LirBinOp{"%t7", "add", "i32", "%t6", "6"});
  join_block.terminator = LirRet{std::string("%t7"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  function.blocks.push_back(std::move(join_block));
  return module;
}

inline c4c::codegen::lir::LirModule
make_conditional_phi_join_mixed_predecessor_chain_and_chain_post_join_add_module() {
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
  then_block.insts.push_back(LirBinOp{"%t3", "add", "i32", "20", "5"});
  then_block.insts.push_back(LirBinOp{"%t4", "sub", "i32", "%t3", "3"});
  then_block.terminator = LirBr{"join"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.insts.push_back(LirBinOp{"%t5", "add", "i32", "9", "4"});
  else_block.insts.push_back(LirBinOp{"%t6", "sub", "i32", "%t5", "2"});
  else_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(LirPhiOp{
      "%t7",
      "i32",
      {
          {"%t4", "then"},
          {"%t6", "else"},
      },
  });
  join_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "%t7", "6"});
  join_block.terminator = LirRet{std::string("%t8"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  function.blocks.push_back(std::move(join_block));
  return module;
}

inline c4c::codegen::lir::LirModule
make_conditional_phi_join_mixed_predecessor_deeper_chain_and_chain_post_join_add_module() {
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
  then_block.insts.push_back(LirBinOp{"%t3", "add", "i32", "20", "5"});
  then_block.insts.push_back(LirBinOp{"%t4", "sub", "i32", "%t3", "3"});
  then_block.insts.push_back(LirBinOp{"%t5", "add", "i32", "%t4", "8"});
  then_block.terminator = LirBr{"join"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.insts.push_back(LirBinOp{"%t6", "add", "i32", "9", "4"});
  else_block.insts.push_back(LirBinOp{"%t7", "sub", "i32", "%t6", "2"});
  else_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(LirPhiOp{
      "%t8",
      "i32",
      {
          {"%t5", "then"},
          {"%t7", "else"},
      },
  });
  join_block.insts.push_back(LirBinOp{"%t9", "add", "i32", "%t8", "6"});
  join_block.terminator = LirRet{std::string("%t9"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  function.blocks.push_back(std::move(join_block));
  return module;
}

inline c4c::codegen::lir::LirModule
make_conditional_phi_join_mixed_predecessor_deeper_chain_and_deeper_chain_post_join_add_module() {
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
  then_block.insts.push_back(LirBinOp{"%t3", "add", "i32", "20", "5"});
  then_block.insts.push_back(LirBinOp{"%t4", "sub", "i32", "%t3", "3"});
  then_block.insts.push_back(LirBinOp{"%t5", "add", "i32", "%t4", "8"});
  then_block.terminator = LirBr{"join"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.insts.push_back(LirBinOp{"%t6", "add", "i32", "9", "4"});
  else_block.insts.push_back(LirBinOp{"%t7", "sub", "i32", "%t6", "2"});
  else_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "%t7", "11"});
  else_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(LirPhiOp{
      "%t9",
      "i32",
      {
          {"%t5", "then"},
          {"%t8", "else"},
      },
  });
  join_block.insts.push_back(LirBinOp{"%t10", "add", "i32", "%t9", "6"});
  join_block.terminator = LirRet{std::string("%t10"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  function.blocks.push_back(std::move(join_block));
  return module;
}

inline c4c::codegen::lir::LirModule
make_conditional_phi_join_mixed_predecessor_four_op_chain_and_deeper_chain_post_join_add_module() {
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
  then_block.insts.push_back(LirBinOp{"%t3", "add", "i32", "20", "5"});
  then_block.insts.push_back(LirBinOp{"%t4", "sub", "i32", "%t3", "3"});
  then_block.insts.push_back(LirBinOp{"%t5", "add", "i32", "%t4", "8"});
  then_block.insts.push_back(LirBinOp{"%t6", "sub", "i32", "%t5", "4"});
  then_block.terminator = LirBr{"join"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.insts.push_back(LirBinOp{"%t7", "add", "i32", "9", "4"});
  else_block.insts.push_back(LirBinOp{"%t8", "sub", "i32", "%t7", "2"});
  else_block.insts.push_back(LirBinOp{"%t9", "add", "i32", "%t8", "11"});
  else_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(LirPhiOp{
      "%t10",
      "i32",
      {
          {"%t6", "then"},
          {"%t9", "else"},
      },
  });
  join_block.insts.push_back(LirBinOp{"%t11", "add", "i32", "%t10", "6"});
  join_block.terminator = LirRet{std::string("%t11"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  function.blocks.push_back(std::move(join_block));
  return module;
}

inline c4c::codegen::lir::LirModule
make_conditional_phi_join_mixed_predecessor_four_op_chain_and_four_op_chain_post_join_add_module() {
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
  then_block.insts.push_back(LirBinOp{"%t3", "add", "i32", "20", "5"});
  then_block.insts.push_back(LirBinOp{"%t4", "sub", "i32", "%t3", "3"});
  then_block.insts.push_back(LirBinOp{"%t5", "add", "i32", "%t4", "8"});
  then_block.insts.push_back(LirBinOp{"%t6", "sub", "i32", "%t5", "4"});
  then_block.terminator = LirBr{"join"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.insts.push_back(LirBinOp{"%t7", "add", "i32", "9", "4"});
  else_block.insts.push_back(LirBinOp{"%t8", "sub", "i32", "%t7", "2"});
  else_block.insts.push_back(LirBinOp{"%t9", "add", "i32", "%t8", "11"});
  else_block.insts.push_back(LirBinOp{"%t10", "sub", "i32", "%t9", "6"});
  else_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(LirPhiOp{
      "%t11",
      "i32",
      {
          {"%t6", "then"},
          {"%t10", "else"},
      },
  });
  join_block.insts.push_back(LirBinOp{"%t12", "add", "i32", "%t11", "6"});
  join_block.terminator = LirRet{std::string("%t12"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  function.blocks.push_back(std::move(join_block));
  return module;
}

inline c4c::codegen::lir::LirModule
make_conditional_phi_join_mixed_predecessor_five_op_chain_and_four_op_chain_post_join_add_module() {
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
  then_block.insts.push_back(LirBinOp{"%t3", "add", "i32", "20", "5"});
  then_block.insts.push_back(LirBinOp{"%t4", "sub", "i32", "%t3", "3"});
  then_block.insts.push_back(LirBinOp{"%t5", "add", "i32", "%t4", "8"});
  then_block.insts.push_back(LirBinOp{"%t6", "sub", "i32", "%t5", "4"});
  then_block.insts.push_back(LirBinOp{"%t7", "add", "i32", "%t6", "10"});
  then_block.terminator = LirBr{"join"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "9", "4"});
  else_block.insts.push_back(LirBinOp{"%t9", "sub", "i32", "%t8", "2"});
  else_block.insts.push_back(LirBinOp{"%t10", "add", "i32", "%t9", "11"});
  else_block.insts.push_back(LirBinOp{"%t11", "sub", "i32", "%t10", "6"});
  else_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(LirPhiOp{
      "%t12",
      "i32",
      {
          {"%t7", "then"},
          {"%t11", "else"},
      },
  });
  join_block.insts.push_back(LirBinOp{"%t13", "add", "i32", "%t12", "6"});
  join_block.terminator = LirRet{std::string("%t13"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  function.blocks.push_back(std::move(join_block));
  return module;
}

inline c4c::codegen::lir::LirModule
make_conditional_phi_join_mixed_predecessor_five_op_chain_and_five_op_chain_post_join_add_module() {
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
  then_block.insts.push_back(LirBinOp{"%t3", "add", "i32", "20", "5"});
  then_block.insts.push_back(LirBinOp{"%t4", "sub", "i32", "%t3", "3"});
  then_block.insts.push_back(LirBinOp{"%t5", "add", "i32", "%t4", "8"});
  then_block.insts.push_back(LirBinOp{"%t6", "sub", "i32", "%t5", "4"});
  then_block.insts.push_back(LirBinOp{"%t7", "add", "i32", "%t6", "10"});
  then_block.terminator = LirBr{"join"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "9", "4"});
  else_block.insts.push_back(LirBinOp{"%t9", "sub", "i32", "%t8", "2"});
  else_block.insts.push_back(LirBinOp{"%t10", "add", "i32", "%t9", "11"});
  else_block.insts.push_back(LirBinOp{"%t11", "sub", "i32", "%t10", "6"});
  else_block.insts.push_back(LirBinOp{"%t12", "add", "i32", "%t11", "13"});
  else_block.terminator = LirBr{"join"};

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(LirPhiOp{
      "%t13",
      "i32",
      {
          {"%t7", "then"},
          {"%t12", "else"},
      },
  });
  join_block.insts.push_back(LirBinOp{"%t14", "add", "i32", "%t13", "6"});
  join_block.terminator = LirRet{std::string("%t14"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  function.blocks.push_back(std::move(join_block));
  return module;
}

inline c4c::codegen::lir::LirModule make_conditional_return_le_module() {
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

inline c4c::codegen::lir::LirModule make_conditional_return_gt_module() {
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

inline c4c::codegen::lir::LirModule make_conditional_return_ge_module() {
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

inline c4c::codegen::lir::LirModule make_conditional_return_eq_module() {
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
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "2", "2"});
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

inline c4c::codegen::lir::LirModule make_conditional_return_ne_module() {
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
  entry.insts.push_back(LirCmpOp{"%t0", false, "ne", "i32", "2", "3"});
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

inline c4c::codegen::lir::LirModule make_conditional_return_ult_module() {
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
  entry.insts.push_back(LirCmpOp{"%t0", false, "ult", "i32", "2", "3"});
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

inline c4c::codegen::lir::LirModule make_conditional_return_ule_module() {
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
  entry.insts.push_back(LirCmpOp{"%t0", false, "ule", "i32", "2", "3"});
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

inline c4c::codegen::lir::LirModule make_conditional_return_ugt_module() {
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
  entry.insts.push_back(LirCmpOp{"%t0", false, "ugt", "i32", "3", "2"});
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

inline c4c::codegen::lir::LirModule make_conditional_return_uge_module() {
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
  entry.insts.push_back(LirCmpOp{"%t0", false, "uge", "i32", "3", "2"});
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

inline c4c::codegen::lir::LirModule make_direct_call_module() {
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

inline c4c::codegen::lir::LirModule make_void_direct_call_imm_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");

  LirFunction helper;
  helper.name = "voidfn";
  helper.signature_text = "define void @voidfn()\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.terminator = LirRet{std::nullopt, "void"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirCallOp{"", "void", "@voidfn", "", ""});
  main_entry.terminator = LirRet{std::string("0"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
  return module;
}

inline c4c::codegen::lir::LirModule make_local_temp_module() {
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

inline c4c::codegen::lir::LirModule make_local_temp_sub_module() {
  using namespace c4c::codegen::lir;

  auto module = make_local_temp_module();
  auto& function = module.functions.front();
  auto& entry = function.blocks.front();
  entry.insts.push_back(LirBinOp{"%t1", "sub", "i32", "%t0", "4"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  return module;
}

inline c4c::codegen::lir::LirModule make_local_temp_arithmetic_chain_module() {
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

inline c4c::codegen::lir::LirModule make_two_local_temp_return_module() {
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

inline c4c::codegen::lir::LirModule make_local_pointer_temp_return_module() {
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



inline c4c::codegen::lir::LirModule make_typed_countdown_while_return_module() {
  using namespace c4c::codegen::lir;

  auto module = make_countdown_while_return_module();
  auto& function = module.functions.front();
  auto& cond_block = function.blocks[1];
  auto& body_block = function.blocks[2];
  std::get<LirCmpOp>(cond_block.insts[1]).predicate = LirCmpPredicate::Ne;
  std::get<LirBinOp>(body_block.insts[1]).opcode = LirBinaryOpcode::Sub;
  return module;
}

inline c4c::codegen::lir::LirModule make_typed_conditional_return_module() {
  using namespace c4c::codegen::lir;

  auto module = make_conditional_return_module();
  auto& function = module.functions.front();
  auto& entry = function.blocks.front();
  std::get<LirCmpOp>(entry.insts[0]).predicate = LirCmpPredicate::Slt;
  std::get<LirCmpOp>(entry.insts[2]).predicate = LirCmpPredicate::Ne;
  return module;
}


inline c4c::codegen::lir::LirModule make_typed_direct_call_local_arg_with_spacing_module() {
  auto module = make_typed_direct_call_local_arg_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.back());
  call.args_str = "  i32   %t0  ";
  return module;
}

inline c4c::codegen::lir::LirModule make_typed_direct_call_local_arg_with_suffix_spacing_module() {
  auto module = make_typed_direct_call_local_arg_with_spacing_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.back());
  call.callee_type_suffix = "( i32 )";
  return module;
}


inline c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_local_arg_with_spacing_module() {
  auto module = make_typed_direct_call_two_arg_local_arg_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.back());
  call.callee_type_suffix = "( i32 , i32 )";
  call.args_str = "  i32   %t0 ,   i32  7  ";
  return module;
}



inline c4c::codegen::lir::LirModule
make_typed_direct_call_two_arg_first_local_rewrite_with_spacing_module() {
  auto module = make_typed_direct_call_two_arg_first_local_rewrite_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.back());
  call.args_str = " i32   %t2 ,   i32 7 ";
  return module;
}

inline c4c::codegen::lir::LirModule
make_typed_direct_call_two_arg_first_local_rewrite_with_suffix_spacing_module() {
  auto module = make_typed_direct_call_two_arg_first_local_rewrite_with_spacing_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.back());
  call.callee_type_suffix = "( i32 , i32 )";
  return module;
}



inline c4c::codegen::lir::LirModule make_typed_global_char_pointer_diff_module() {
  using namespace c4c::codegen::lir;

  auto module = make_global_char_pointer_diff_module();
  auto& entry = module.functions.front().blocks.front();
  std::get<LirBinOp>(entry.insts[8]).opcode = LirBinaryOpcode::Sub;
  std::get<LirCmpOp>(entry.insts[10]).predicate = LirCmpPredicate::Eq;
  return module;
}


inline c4c::codegen::lir::LirModule make_typed_global_int_pointer_diff_module() {
  using namespace c4c::codegen::lir;

  auto module = make_global_int_pointer_diff_module();
  auto& entry = module.functions.front().blocks.front();
  std::get<LirBinOp>(entry.insts[8]).opcode = LirBinaryOpcode::Sub;
  std::get<LirBinOp>(entry.insts[9]).opcode = LirBinaryOpcode::SDiv;
  std::get<LirCmpOp>(entry.insts[11]).predicate = LirCmpPredicate::Eq;
  return module;
}

inline c4c::codegen::lir::LirModule make_disjoint_block_call_arg_alloca_candidate_module() {
  auto module = make_disjoint_block_local_alloca_candidate_module();
  auto& else_block = module.functions.front().blocks[2];
  else_block.insts.insert(
      else_block.insts.begin() + 1,
      c4c::codegen::lir::LirCallOp{"", "void", "@sink", "(ptr)", "  ptr   %lv.else  "});
  return module;
}

inline c4c::codegen::lir::LirModule make_x86_extern_decl_object_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-n8:16:32:64-S128";
  module.extern_decls.push_back(LirExternDecl{"helper_ext", "i32"});

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

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_x86_extern_decl_inferred_param_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-n8:16:32:64-S128";
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

inline c4c::codegen::lir::LirModule make_x86_extern_decl_inconsistent_param_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-n8:16:32:64-S128";
  module.extern_decls.push_back(LirExternDecl{"helper_ext", "i32"});

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{"%t0", "i32", "@helper_ext", "", "i32 5"});
  entry.insts.push_back(LirCallOp{"%t1", "i32", "@helper_ext", "", "ptr @helper_ext"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

inline c4c::codegen::lir::LirModule make_x86_global_int_pointer_roundtrip_module() {
  auto module = make_global_int_pointer_roundtrip_module();
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  return module;
}
