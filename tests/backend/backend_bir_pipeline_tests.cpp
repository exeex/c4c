#include "backend_bir_test_support.hpp"

#include "../../src/backend/aarch64/codegen/emit.hpp"
#include "../../src/backend/bir_printer.hpp"
#include "../../src/backend/lowering/lir_to_bir.hpp"
#include "../../src/backend/x86/codegen/x86_codegen.hpp"

namespace {

using I8ModuleFactory = c4c::codegen::lir::LirModule (*)();

constexpr std::string_view kExpectedX86UnsupportedDirectLir =
    "x86 backend emitter does not support this direct LIR module; only direct-LIR slices that lower natively or through direct BIR are currently supported";
constexpr std::string_view kExpectedAarch64UnsupportedDirectLir =
    "aarch64 backend emitter does not support this direct LIR module; only direct-LIR slices that lower natively or through direct BIR are currently supported";

c4c::codegen::lir::LirModule make_unsupported_local_array_gep_lir_module() {
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
  entry.insts.push_back(LirGepOp{"%t0", "[2 x i32]", "%lv.arr", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::SExt, "i32", "0", "i64"});
  entry.insts.push_back(LirGepOp{"%t2", "i32", "%t0", false, {"i64 %t1"}});
  entry.insts.push_back(LirStoreOp{"i32", "4", "%t2"});
  entry.insts.push_back(LirGepOp{"%t3", "[2 x i32]", "%lv.arr", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t4", LirCastKind::SExt, "i32", "1", "i64"});
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

c4c::codegen::lir::LirModule make_supported_x86_void_direct_call_return_lir_module() {
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

c4c::codegen::lir::LirModule make_pointer_phi_join_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "choose_ptr";
  function.signature_text = "define i32 @choose_ptr()";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.a", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.b", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "11", "%lv.a"});
  entry.insts.push_back(LirStoreOp{"i32", "4", "%lv.b"});
  entry.insts.push_back(
      LirCmpOp{"%t0", false, LirCmpPredicateRef{"eq"}, "i32", "0", "0"});
  entry.terminator = LirCondBr{"%t0", "then", "else"};

  LirBlock then_block;
  then_block.id = LirBlockId{1};
  then_block.label = "then";
  then_block.terminator = LirBr{"join"};

  LirBlock else_block;
  else_block.id = LirBlockId{2};
  else_block.label = "else";
  else_block.terminator = LirBr{"join"};

  LirBlock join;
  join.id = LirBlockId{3};
  join.label = "join";
  join.insts.push_back(
      LirPhiOp{"%t1", "ptr", {{"%lv.a", "then"}, {"%lv.b", "else"}}});
  join.insts.push_back(LirLoadOp{"%t2", "i32", "%t1"});
  join.terminator = LirRet{std::string("%t2"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_unsupported_x86_double_return_lir_module() {
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

c4c::codegen::lir::LirModule make_unsupported_aarch64_double_return_lir_module() {
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

c4c::codegen::lir::LirModule make_supported_x86_string_literal_char_lir_module() {
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

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "[3 x i8]", "@.str0", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t2", "i8", "%t0", false, {"i64 %t1"}});
  entry.insts.push_back(LirLoadOp{"%t3", "i8", "%t2"});
  entry.insts.push_back(LirCastOp{"%t4", LirCastKind::SExt, "i8", "%t3", "i32"});
  entry.terminator = LirRet{std::string("%t4"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_supported_aarch64_string_literal_char_lir_module() {
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
  entry.insts.push_back(LirCastOp{"%t4", LirCastKind::SExt, "i8", "%t3", "i32"});
  entry.terminator = LirRet{std::string("%t4"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_supported_x86_source_like_repeated_printf_immediates_lir_module() {
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

c4c::codegen::lir::LirModule make_supported_x86_source_like_local_buffer_string_copy_printf_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  module.string_pool.push_back(LirStringConst{"@.str0", "abcdef", 7});
  module.string_pool.push_back(LirStringConst{"@.str1", "%s\n", 4});
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
  module.extern_decls.push_back(LirExternDecl{"strcpy", "ptr", "ptr"});
  module.extern_decls.push_back(LirExternDecl{"printf", "i32", "i32"});
  module.extern_decls.push_back(LirExternDecl{"puts", "i32", "i32"});

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.buf", "[10 x i8]", "", 1});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirGepOp{"%t0", "[10 x i8]", "%lv.buf", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(
      LirGepOp{"%t1", "[7 x i8]", "@.str0", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCallOp{"%t2", "ptr", "@strcpy", "(ptr, ptr)", "ptr %t0, ptr %t1"});
  entry.insts.push_back(
      LirGepOp{"%t3", "[4 x i8]", "@.str1", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(
      LirGepOp{"%t4", "[10 x i8]", "%lv.buf", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirCastOp{"%t5", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(LirGepOp{"%t6", "i8", "%t4", false, {"i64 %t5"}});
  entry.insts.push_back(LirCallOp{"%t7", "i32", "@printf", "(ptr, ...)", "ptr %t3, ptr %t6"});
  entry.terminator = LirRet{std::string("0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_supported_x86_minimal_counted_printf_ternary_loop_lir_module() {
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

c4c::codegen::lir::LirModule make_supported_x86_variadic_sum2_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction sum2;
  sum2.name = "sum2";
  sum2.signature_text = "define i32 @sum2(i32 %p.first, ...)";
  sum2.entry = LirBlockId{0};
  sum2.alloca_insts.push_back(LirAllocaOp{"%lv.ap", "%struct.__va_list_tag_", "", 16});
  sum2.alloca_insts.push_back(LirAllocaOp{"%lv.second", "i32", "", 4});
  sum2.alloca_insts.push_back(LirAllocaOp{"%t13", "i32", "", 4});
  sum2.alloca_insts.push_back(LirAllocaOp{"%t17", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirVaStartOp{"%lv.ap"});
  entry.insts.push_back(LirGepOp{"%t0", "%struct.__va_list_tag_", "%lv.ap", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(LirGepOp{"%t1", "%struct.__va_list_tag_", "%lv.ap", false, {"i32 0", "i32 1"}});
  entry.insts.push_back(LirGepOp{"%t2", "%struct.__va_list_tag_", "%lv.ap", false, {"i32 0", "i32 2"}});
  entry.insts.push_back(LirGepOp{"%t3", "%struct.__va_list_tag_", "%lv.ap", false, {"i32 0", "i32 3"}});
  entry.insts.push_back(LirLoadOp{"%t4", "ptr", "%t3"});
  entry.insts.push_back(LirLoadOp{"%t5", "i32", "%t0"});
  entry.insts.push_back(LirCmpOp{"%t6", false, LirCmpPredicateRef{"sle"}, "i32", "%t5", "40"});
  entry.terminator = LirCondBr{"%t6", "vaarg.amd64.reg.7", "vaarg.amd64.stack.8"};

  LirBlock reg;
  reg.id = LirBlockId{1};
  reg.label = "vaarg.amd64.reg.7";
  reg.insts.push_back(LirCastOp{"%t10", LirCastKind::SExt, "i32", "%t5", "i64"});
  reg.insts.push_back(LirGepOp{"%t11", "i8", "%t4", false, {"i64 %t10"}});
  reg.insts.push_back(LirBinOp{"%t12", "add", "i32", "%t5", "8"});
  reg.insts.push_back(LirStoreOp{"i32", "%t12", "%t0"});
  reg.insts.push_back(LirMemcpyOp{"%t13", "%t11", "4", false});
  reg.insts.push_back(LirLoadOp{"%t14", "i32", "%t13"});
  reg.terminator = LirBr{"vaarg.amd64.join.9"};

  LirBlock stack;
  stack.id = LirBlockId{2};
  stack.label = "vaarg.amd64.stack.8";
  stack.insts.push_back(LirLoadOp{"%t15", "ptr", "%t2"});
  stack.insts.push_back(LirGepOp{"%t16", "i8", "%t15", false, {"i64 8"}});
  stack.insts.push_back(LirStoreOp{"ptr", "%t16", "%t2"});
  stack.insts.push_back(LirMemcpyOp{"%t17", "%t15", "4", false});
  stack.insts.push_back(LirLoadOp{"%t18", "i32", "%t17"});
  stack.terminator = LirBr{"vaarg.amd64.join.9"};

  LirBlock join;
  join.id = LirBlockId{3};
  join.label = "vaarg.amd64.join.9";
  join.insts.push_back(LirPhiOp{"%t19", "i32", {{"%t14", "vaarg.amd64.reg.7"}, {"%t18", "vaarg.amd64.stack.8"}}});
  join.insts.push_back(LirStoreOp{"i32", "%t19", "%lv.second"});
  join.insts.push_back(LirVaEndOp{"%lv.ap"});
  join.insts.push_back(LirLoadOp{"%t20", "i32", "%lv.second"});
  join.insts.push_back(LirBinOp{"%t21", "add", "i32", "%p.first", "%t20"});
  join.terminator = LirRet{std::string("%t21"), "i32"};

  sum2.blocks.push_back(std::move(entry));
  sum2.blocks.push_back(std::move(reg));
  sum2.blocks.push_back(std::move(stack));
  sum2.blocks.push_back(std::move(join));

  LirFunction main;
  main.name = "main";
  main.signature_text = "define i32 @main()";
  main.entry = LirBlockId{0};

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(LirCallOp{"%t0", "i32", "@sum2", "(i32, ...)", "i32 10, i32 32"});
  main_entry.terminator = LirRet{std::string("%t0"), "i32"};
  main.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(sum2));
  module.functions.push_back(std::move(main));
  return module;
}

c4c::codegen::lir::LirModule make_supported_x86_variadic_double_bytes_lir_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction helper;
  helper.name = "variadic_double_bytes";
  helper.signature_text = "define i32 @variadic_double_bytes(i32 %p.seed, ...)";
  helper.entry = LirBlockId{0};
  helper.alloca_insts.push_back(LirAllocaOp{"%lv.ap", "%struct.__va_list_tag_", "", 16});
  helper.alloca_insts.push_back(LirAllocaOp{"%lv.second", "double", "", 8});
  helper.alloca_insts.push_back(LirAllocaOp{"%lv.bytes", "ptr", "", 8});
  helper.alloca_insts.push_back(LirAllocaOp{"%t13", "double", "", 8});
  helper.alloca_insts.push_back(LirAllocaOp{"%t17", "double", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirVaStartOp{"%lv.ap"});
  entry.insts.push_back(LirGepOp{"%t0", "%struct.__va_list_tag_", "%lv.ap", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(LirGepOp{"%t1", "%struct.__va_list_tag_", "%lv.ap", false, {"i32 0", "i32 1"}});
  entry.insts.push_back(LirGepOp{"%t2", "%struct.__va_list_tag_", "%lv.ap", false, {"i32 0", "i32 2"}});
  entry.insts.push_back(LirGepOp{"%t3", "%struct.__va_list_tag_", "%lv.ap", false, {"i32 0", "i32 3"}});
  entry.insts.push_back(LirLoadOp{"%t4", "ptr", "%t3"});
  entry.insts.push_back(LirLoadOp{"%t5", "i32", "%t1"});
  entry.insts.push_back(LirCmpOp{"%t6", false, LirCmpPredicateRef{"sle"}, "i32", "%t5", "160"});
  entry.terminator = LirCondBr{"%t6", "vaarg.amd64.reg.7", "vaarg.amd64.stack.8"};

  LirBlock reg;
  reg.id = LirBlockId{1};
  reg.label = "vaarg.amd64.reg.7";
  reg.insts.push_back(LirCastOp{"%t10", LirCastKind::SExt, "i32", "%t5", "i64"});
  reg.insts.push_back(LirGepOp{"%t11", "i8", "%t4", false, {"i64 %t10"}});
  reg.insts.push_back(LirBinOp{"%t12", "add", "i32", "%t5", "16"});
  reg.insts.push_back(LirStoreOp{"i32", "%t12", "%t1"});
  reg.insts.push_back(LirMemcpyOp{"%t13", "%t11", "8", false});
  reg.insts.push_back(LirLoadOp{"%t14", "double", "%t13"});
  reg.terminator = LirBr{"vaarg.amd64.join.9"};

  LirBlock stack;
  stack.id = LirBlockId{2};
  stack.label = "vaarg.amd64.stack.8";
  stack.insts.push_back(LirLoadOp{"%t15", "ptr", "%t2"});
  stack.insts.push_back(LirGepOp{"%t16", "i8", "%t15", false, {"i64 8"}});
  stack.insts.push_back(LirStoreOp{"ptr", "%t16", "%t2"});
  stack.insts.push_back(LirMemcpyOp{"%t17", "%t15", "8", false});
  stack.insts.push_back(LirLoadOp{"%t18", "double", "%t17"});
  stack.terminator = LirBr{"vaarg.amd64.join.9"};

  LirBlock join;
  join.id = LirBlockId{3};
  join.label = "vaarg.amd64.join.9";
  join.insts.push_back(LirPhiOp{"%t19", "double", {{"%t14", "vaarg.amd64.reg.7"}, {"%t18", "vaarg.amd64.stack.8"}}});
  join.insts.push_back(LirStoreOp{"double", "%t19", "%lv.second"});
  join.insts.push_back(LirVaEndOp{"%lv.ap"});
  join.insts.push_back(LirStoreOp{"ptr", "%lv.second", "%lv.bytes"});
  join.insts.push_back(LirLoadOp{"%t20", "ptr", "%lv.bytes"});
  join.insts.push_back(LirGepOp{"%t22", "i8", "%t20", false, {"i64 6"}});
  join.insts.push_back(LirLoadOp{"%t23", "i8", "%t22"});
  join.insts.push_back(LirCastOp{"%t24", LirCastKind::ZExt, "i8", "%t23", "i32"});
  join.insts.push_back(LirBinOp{"%t25", "add", "i32", "%p.seed", "%t24"});
  join.insts.push_back(LirLoadOp{"%t26", "ptr", "%lv.bytes"});
  join.insts.push_back(LirGepOp{"%t28", "i8", "%t26", false, {"i64 7"}});
  join.insts.push_back(LirLoadOp{"%t29", "i8", "%t28"});
  join.insts.push_back(LirCastOp{"%t30", LirCastKind::ZExt, "i8", "%t29", "i32"});
  join.insts.push_back(LirBinOp{"%t31", "add", "i32", "%t25", "%t30"});
  join.terminator = LirRet{std::string("%t31"), "i32"};

  helper.blocks.push_back(std::move(entry));
  helper.blocks.push_back(std::move(reg));
  helper.blocks.push_back(std::move(stack));
  helper.blocks.push_back(std::move(join));

  LirFunction main;
  main.name = "main";
  main.signature_text = "define i32 @main()";
  main.entry = LirBlockId{0};

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(
      LirCallOp{"%t0", "i32", "@variadic_double_bytes", "(i32, ...)",
                "i32 1, double 0x4002000000000000"});
  main_entry.terminator = LirRet{std::string("%t0"), "i32"};
  main.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main));
  return module;
}

c4c::codegen::lir::LirModule make_dead_local_add_store_return_immediate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout =
      "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.foo", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.bar", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.foobar", "i32", "", 4});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.foo"});
  entry.insts.push_back(LirLoadOp{"%t1", "i32", "%lv.bar"});
  entry.insts.push_back(LirBinOp{"%t2", "add", "i32", "%t0", "%t1"});
  entry.insts.push_back(LirStoreOp{"i32", "%t2", "%lv.foobar"});
  entry.insts.push_back(LirLoadOp{"%t3", "i32", "%lv.foo"});
  entry.insts.push_back(LirLoadOp{"%t4", "i32", "%lv.bar"});
  entry.insts.push_back(LirBinOp{"%t5", "add", "i32", "%t3", "%t4"});
  entry.insts.push_back(LirStoreOp{"i32", "%t5", "%lv.foobar"});
  entry.terminator = LirRet{std::string("0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

void expect_i8_bir_route(I8ModuleFactory make_module,
                         std::string_view signature,
                         std::string_view op_text,
                         std::string_view message_prefix) {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, std::string(signature),
                  std::string(message_prefix) +
                      " should preserve the widened i8 signature on the BIR text path");
  expect_contains(rendered, std::string(op_text),
                  std::string(message_prefix) +
                      " should expose the widened i8 operation on the BIR text path");
}

void expect_i8_bir_immediate_route(I8ModuleFactory make_module,
                                   std::string_view signature,
                                   std::string_view ret_text,
                                   std::string_view message_prefix) {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, std::string(signature),
                  std::string(message_prefix) +
                      " should preserve the widened i8 signature on the BIR text path");
  expect_contains(rendered, std::string(ret_text),
                  std::string(message_prefix) +
                      " should keep the widened i8 immediate return on the BIR text path");
}

void test_backend_default_input_keeps_lir_entry_observable() {
  const c4c::backend::BackendModuleInput input{make_bir_return_add_module()};

  expect_true(input.holds_lir_module(),
              "default backend input should keep fresh LIR observable at the shared entry seam");
  expect_true(!input.holds_bir_module(),
              "default backend input should not synthesize prelowered BIR ownership before lowering runs");
}

void test_backend_direct_bir_input_keeps_bir_ownership_observable() {
  const c4c::backend::BackendModuleInput input{make_return_immediate_module()};

  expect_true(input.holds_bir_module(),
              "direct BIR input should remain observable as BIR-owned input at the shared backend entry");
  expect_true(!input.holds_lir_module(),
              "direct BIR input should not advertise a dead LIR fallback payload");
}

void test_backend_default_path_uses_bir_when_bir_pipeline_is_not_selected() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_add_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Riscv64});

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "default backend flow should expose the supported tiny add slice on the BIR text path");
  expect_contains(rendered, "%t0 = bir.add i32 2, 3",
                  "default backend flow should lower supported LIR input directly through the BIR seam");
}

void test_backend_riscv64_path_keeps_unsupported_lir_on_llvm_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_unsupported_local_array_gep_lir_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Riscv64});

  expect_contains(rendered, "define i32 @main()",
                  "riscv64 backend text flow should keep unsupported LIR input on the LLVM text surface when shared BIR lowering cannot represent it");
  expect_not_contains(rendered, "bir.func",
                      "unsupported LIR input should not pretend to have lowered through the BIR scaffold");
}

void test_backend_riscv64_path_keeps_unsupported_direct_lir_on_llvm_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_unsupported_x86_double_return_lir_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Riscv64});

  expect_contains(rendered, "define double @main()",
                  "riscv64 backend text flow should keep unsupported direct-LIR input on the LLVM text surface even when native x86 entry rejects the same module");
  expect_not_contains(rendered, "bir.func",
                      "unsupported direct LIR should not be rendered as BIR on the riscv64 text path");
}

void test_backend_entry_rejects_unsupported_direct_lir_input_on_x86() {
  try {
    (void)c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{make_unsupported_x86_double_return_lir_module()},
        c4c::backend::BackendOptions{c4c::backend::Target::X86_64});
  } catch (const std::invalid_argument& ex) {
    expect_true(ex.what() == kExpectedX86UnsupportedDirectLir,
                "shared backend entry should preserve the x86 direct-LIR rejection contract after prepared-LIR throw ownership moves out of the target emitter");
    expect_contains(ex.what(), "direct LIR module",
                    "backend entry should now expose the native x86 direct-LIR subset rejection instead of rescuing unsupported input through LLVM text");
    expect_not_contains(ex.what(), "legacy backend IR",
                        "backend entry should not mention the deleted legacy backend IR route when x86 rejects unsupported direct LIR");
    return;
  }

  fail("backend entry should reject unsupported x86 direct LIR input once the backend.cpp LLVM rescue seam is removed");
}

void test_backend_entry_rejects_unsupported_direct_lir_input_on_aarch64() {
  try {
    (void)c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{make_unsupported_aarch64_double_return_lir_module()},
        c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  } catch (const std::invalid_argument& ex) {
    expect_true(ex.what() == kExpectedAarch64UnsupportedDirectLir,
                "shared backend entry should preserve the aarch64 direct-LIR rejection contract after prepared-LIR throw ownership moves out of the target emitter");
    expect_contains(ex.what(), "direct LIR module",
                    "backend entry should now expose the native aarch64 direct-LIR subset rejection instead of rescuing unsupported input through LLVM text");
    expect_not_contains(ex.what(), "legacy backend IR",
                        "backend entry should not mention the deleted legacy backend IR route when aarch64 rejects unsupported direct LIR");
    return;
  }

  fail("backend entry should reject unsupported aarch64 direct LIR input once the backend.cpp LLVM rescue seam is removed");
}

void test_x86_try_emit_module_reports_direct_bir_support_explicitly() {
  const auto supported = c4c::backend::x86::try_emit_module(make_return_immediate_module());
  expect_true(supported.has_value(),
              "x86 direct BIR support probing should report a bounded affine-return module without requiring a thrown rejection string");
  expect_contains(*supported, ".intel_syntax noprefix",
                  "x86 direct BIR support probing should emit Intel-syntax asm so the host assembler can consume the native backend text");
  expect_contains(*supported, ".text",
                  "x86 direct BIR support probing should place the bounded affine-return module in the text section before the function body");
  expect_contains(*supported, "mov eax, 7",
                  "x86 direct BIR support probing should still render the bounded affine-return module natively");

  const auto unsupported =
      c4c::backend::x86::try_emit_module(make_unsupported_multi_function_bir_module());
  expect_true(!unsupported.has_value(),
              "x86 direct BIR support probing should return no native rendering for unsupported multi-function modules instead of requiring exception-text classification");
}

void test_aarch64_try_emit_module_reports_direct_bir_support_explicitly() {
  const auto supported =
      c4c::backend::aarch64::try_emit_module(make_return_immediate_module());
  expect_true(supported.has_value(),
              "aarch64 direct BIR support probing should report a bounded affine-return module without requiring a thrown rejection string");
  expect_contains(*supported, "mov w0, #7",
                  "aarch64 direct BIR support probing should still render the bounded affine-return module natively");

  const auto unsupported =
      c4c::backend::aarch64::try_emit_module(make_unsupported_multi_function_bir_module());
  expect_true(!unsupported.has_value(),
              "aarch64 direct BIR support probing should return no native rendering for unsupported multi-function modules instead of requiring exception-text classification");
}

void test_x86_try_emit_prepared_lir_module_reports_direct_lir_support_explicitly() {
  const auto prepared_supported = c4c::backend::prepare_lir_module_for_target(
      make_supported_x86_string_literal_char_lir_module(), c4c::backend::Target::X86_64);
  const auto supported = c4c::backend::x86::try_emit_prepared_lir_module(prepared_supported);
  expect_true(supported.has_value(),
              "x86 prepared direct-LIR support probing should report a bounded string-literal module without requiring a thrown rejection string");
  expect_contains(*supported, ".intel_syntax noprefix",
                  "x86 prepared direct-LIR support probing should emit Intel-syntax asm so native string-literal helpers assemble on the host toolchain");
  expect_contains(*supported, ".text",
                  "x86 prepared direct-LIR support probing should return to the text section before emitting the helper body");
  expect_contains(*supported, "movsx eax",
                  "x86 prepared direct-LIR support probing should still render the bounded string-literal module natively");

  const auto prepared_unsupported = c4c::backend::prepare_lir_module_for_target(
      make_unsupported_x86_double_return_lir_module(), c4c::backend::Target::X86_64);
  const auto unsupported =
      c4c::backend::x86::try_emit_prepared_lir_module(prepared_unsupported);
  expect_true(!unsupported.has_value(),
              "x86 prepared direct-LIR support probing should return no native rendering for unsupported floating-return modules instead of requiring exception-text classification");
}

void test_x86_try_emit_prepared_lir_module_accepts_variadic_sum2_runtime_slice() {
  const auto prepared = c4c::backend::prepare_lir_module_for_target(
      make_supported_x86_variadic_sum2_lir_module(), c4c::backend::Target::X86_64);
  const auto rendered = c4c::backend::x86::try_emit_prepared_lir_module(prepared);

  expect_true(rendered.has_value(),
              "x86 prepared direct-LIR probing should accept the bounded variadic i32 runtime slice instead of rejecting it at the unsupported-module boundary");
  expect_contains(*rendered, "lea eax, [rdi + rsi]",
                  "x86 prepared direct-LIR probing should lower the bounded variadic i32 helper through the native integer register path");
  expect_contains(*rendered, "xor eax, eax",
                  "x86 prepared direct-LIR probing should clear the SysV variadic SSE-count register on integer-only call sites");
}

void test_x86_try_emit_prepared_lir_module_accepts_variadic_double_bytes_runtime_slice() {
  const auto prepared = c4c::backend::prepare_lir_module_for_target(
      make_supported_x86_variadic_double_bytes_lir_module(), c4c::backend::Target::X86_64);
  const auto rendered = c4c::backend::x86::try_emit_prepared_lir_module(prepared);

  expect_true(rendered.has_value(),
              "x86 prepared direct-LIR probing should accept the bounded variadic double-byte runtime slice instead of rejecting it at the unsupported-module boundary");
  expect_contains(*rendered, "movsd QWORD PTR [rbp - 8], xmm0",
                  "x86 prepared direct-LIR probing should preserve the incoming variadic double in the bounded helper frame");
  expect_contains(*rendered, "mov eax, 1",
                  "x86 prepared direct-LIR probing should set the SysV variadic SSE-count register for the double call site");
}

void test_x86_direct_variadic_helper_accepts_variadic_sum2_runtime_slice() {
  const auto prepared = c4c::backend::prepare_lir_module_for_target(
      make_supported_x86_variadic_sum2_lir_module(), c4c::backend::Target::X86_64);
  const auto rendered = c4c::backend::x86::try_emit_minimal_variadic_sum2_module(prepared);

  expect_true(rendered.has_value(),
              "the direct x86 variadic helper seam should accept the bounded variadic i32 runtime slice after ownership moves out of emit.cpp");
  expect_contains(*rendered, ".globl sum2",
                  "the direct x86 variadic helper seam should still emit the helper definition after the Step 4 ownership move");
  expect_contains(*rendered, "lea eax, [rdi + rsi]",
                  "the direct x86 variadic helper seam should still lower the bounded variadic helper through the native integer register path");
  expect_contains(*rendered, "xor eax, eax",
                  "the direct x86 variadic helper seam should still clear the SysV variadic SSE-count register on integer-only call sites");
}

void test_x86_direct_printf_helper_accepts_repeated_printf_immediates_slice() {
  const auto prepared = c4c::backend::prepare_lir_module_for_target(
      make_supported_x86_source_like_repeated_printf_immediates_lir_module(),
      c4c::backend::Target::X86_64);
  const auto rendered =
      c4c::backend::x86::try_emit_minimal_repeated_printf_immediates_module(prepared);

  expect_true(rendered.has_value(),
              "the direct x86 printf helper seam should accept the bounded repeated-printf immediate slice after ownership moves out of emit.cpp");
  expect_contains(*rendered, ".asciz \"%d %d\\n\"",
                  "the direct x86 printf helper seam should still materialize the shared format bytes after the Step 4 ownership move");
  expect_contains(*rendered, "mov rsi, 1",
                  "the direct x86 printf helper seam should still lower the first bounded printf payload through the integer register path");
  expect_contains(*rendered, "mov rsi, 2",
                  "the direct x86 printf helper seam should still lower the second bounded printf payload through the integer register path");
}

void test_x86_direct_printf_helper_accepts_local_buffer_string_copy_printf_slice() {
  const auto prepared = c4c::backend::prepare_lir_module_for_target(
      make_supported_x86_source_like_local_buffer_string_copy_printf_lir_module(),
      c4c::backend::Target::X86_64);
  const auto rendered =
      c4c::backend::x86::try_emit_minimal_local_buffer_string_copy_printf_module(prepared);

  expect_true(rendered.has_value(),
              "the direct x86 printf helper seam should accept the bounded local-buffer copy/printf slice after ownership moves out of emit.cpp");
  expect_contains(*rendered, ".asciz \"abcdef\"",
                  "the direct x86 printf helper seam should still materialize the copy-source bytes after the Step 4 ownership move");
  expect_contains(*rendered, "call strcpy",
                  "the direct x86 printf helper seam should still lower the bounded stack-buffer copy through the native libc call path");
  expect_contains(*rendered, "lea rsi, [rsp + 9]",
                  "the direct x86 printf helper seam should still preserve the one-byte stack-buffer offset passed to printf");
}

void test_x86_direct_printf_helper_accepts_counted_printf_ternary_loop_slice() {
  const auto prepared = c4c::backend::prepare_lir_module_for_target(
      make_supported_x86_minimal_counted_printf_ternary_loop_lir_module(),
      c4c::backend::Target::X86_64);
  const auto rendered =
      c4c::backend::x86::try_emit_minimal_counted_printf_ternary_loop_module(prepared);

  expect_true(rendered.has_value(),
              "the direct x86 printf helper seam should accept the bounded counted ternary-loop printf slice after ownership moves out of emit.cpp");
  expect_contains(*rendered, ".asciz \"%d\\n\"",
                  "the direct x86 printf helper seam should still materialize the shared ternary-loop format bytes after the Step 4 ownership move");
  expect_contains(*rendered, "cmp eax, 10",
                  "the direct x86 printf helper seam should still preserve the bounded loop exit compare on the native x86 path");
  expect_contains(*rendered, "imul esi, eax",
                  "the direct x86 printf helper seam should still lower the square arm through the native integer register path");
  expect_contains(*rendered, "lea esi, [rax + rax*2]",
                  "the direct x86 printf helper seam should still lower the times-three arm through the native integer register path");
}

void test_x86_direct_printf_helper_accepts_string_literal_char_slice() {
  const auto prepared = c4c::backend::prepare_lir_module_for_target(
      make_supported_x86_string_literal_char_lir_module(), c4c::backend::Target::X86_64);
  const auto rendered =
      c4c::backend::x86::try_emit_minimal_string_literal_char_module(prepared);

  expect_true(rendered.has_value(),
              "the direct x86 printf helper seam should accept the bounded string-literal indexed-char slice after ownership moves out of emit.cpp");
  expect_contains(*rendered, ".asciz \"hi\"",
                  "the direct x86 printf helper seam should still materialize the string bytes after the Step 4 ownership move");
  expect_contains(*rendered, "lea rax, .L.str0[rip]",
                  "the direct x86 printf helper seam should still address the private string pool label on the native x86 path");
  expect_contains(*rendered, "movsx eax, byte ptr [rax + 1]",
                  "the direct x86 printf helper seam should still lower the indexed char load through the native byte-load path");
}

void test_x86_direct_void_helper_accepts_void_direct_call_return_slice() {
  const auto prepared = c4c::backend::prepare_lir_module_for_target(
      make_supported_x86_void_direct_call_return_lir_module(), c4c::backend::Target::X86_64);
  const auto rendered = c4c::backend::x86::try_emit_minimal_void_helper_call_module(prepared);

  expect_true(rendered.has_value(),
              "the direct x86 void helper seam should accept the bounded helper-call-plus-immediate-return slice after ownership moves out of emit.cpp");
  expect_contains(*rendered, ".globl voidfn",
                  "the direct x86 void helper seam should still emit the helper definition after the Step 4 ownership move");
  expect_contains(*rendered, "voidfn:\n  ret\n",
                  "the direct x86 void helper seam should still lower the bounded helper body to a bare return");
  expect_contains(*rendered, "main:\n  call voidfn\n  mov eax, 0\n  ret\n",
                  "the direct x86 void helper seam should still lower the bounded helper call and immediate return on the native x86 path");
}

void test_aarch64_try_emit_prepared_lir_module_reports_direct_lir_support_explicitly() {
  const auto prepared_supported = c4c::backend::prepare_lir_module_for_target(
      make_supported_aarch64_string_literal_char_lir_module(),
      c4c::backend::Target::Aarch64);
  const auto supported =
      c4c::backend::aarch64::try_emit_prepared_lir_module(prepared_supported);
  expect_true(supported.has_value(),
              "aarch64 prepared direct-LIR support probing should report a bounded string-literal module without requiring a thrown rejection string");
  expect_contains(*supported, "ldrb w0",
                  "aarch64 prepared direct-LIR support probing should still render the bounded string-literal module natively");

  const auto prepared_unsupported = c4c::backend::prepare_lir_module_for_target(
      make_unsupported_aarch64_double_return_lir_module(), c4c::backend::Target::Aarch64);
  const auto unsupported =
      c4c::backend::aarch64::try_emit_prepared_lir_module(prepared_unsupported);
  expect_true(!unsupported.has_value(),
              "aarch64 prepared direct-LIR support probing should return no native rendering for unsupported floating-return modules instead of requiring exception-text classification");
}

void test_aarch64_try_emit_prepared_lir_module_accepts_pointer_phi_join_modules() {
  const auto prepared = c4c::backend::prepare_lir_module_for_target(
      make_pointer_phi_join_lir_module(), c4c::backend::Target::Aarch64);
  const auto rendered = c4c::backend::aarch64::try_emit_prepared_lir_module(prepared);

  expect_true(rendered.has_value(),
              "aarch64 prepared direct-LIR support probing should keep bounded pointer-phi join modules on the native emitter path instead of rejecting them outright");
  expect_contains(*rendered, ".choose_ptr.join:",
                  "aarch64 prepared direct-LIR support probing should still emit the join block after materializing phi-edge copies");
  expect_not_contains(*rendered, "target triple =",
                      "aarch64 prepared direct-LIR support probing should not fall back to LLVM text when handling the bounded pointer-phi join path");
}

void test_x86_public_bir_emitter_delegates_direct_bir_route_to_shared_backend() {
  const auto shared_rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{make_return_immediate_module()},
                                make_bir_pipeline_options(c4c::backend::Target::X86_64));
  const auto x86_rendered = c4c::backend::x86::emit_module(make_return_immediate_module());

  expect_true(x86_rendered == shared_rendered,
              "x86 public direct-BIR entry should delegate to the shared backend helper instead of owning its own try-or-throw route policy");
  expect_contains(x86_rendered, "mov eax, 7",
                  "x86 public direct-BIR entry should preserve the current affine-return native emission contract after routing moves into backend.cpp");

  try {
    (void)c4c::backend::x86::emit_module(make_unsupported_multi_function_bir_module());
  } catch (const std::invalid_argument& ex) {
    expect_true(
        std::string_view(ex.what()) ==
            "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively",
        "x86 public direct-BIR entry should preserve the existing unsupported-module rejection contract after delegating to the shared backend helper");
    return;
  }

  fail("x86 public direct-BIR entry should still reject unsupported multi-function modules");
}

void test_aarch64_public_bir_emitter_delegates_direct_bir_route_to_shared_backend() {
  const auto shared_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_return_immediate_module()},
      make_bir_pipeline_options(c4c::backend::Target::Aarch64));
  const auto aarch64_rendered =
      c4c::backend::aarch64::emit_module(make_return_immediate_module());

  expect_true(aarch64_rendered == shared_rendered,
              "aarch64 public direct-BIR entry should delegate to the shared backend helper instead of owning its own try-or-throw route policy");
  expect_contains(aarch64_rendered, "mov w0, #7",
                  "aarch64 public direct-BIR entry should preserve the current affine-return native emission contract after routing moves into backend.cpp");

  try {
    (void)c4c::backend::aarch64::emit_module(make_unsupported_multi_function_bir_module());
  } catch (const std::invalid_argument& ex) {
    expect_true(
        std::string_view(ex.what()) ==
            "aarch64 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively",
        "aarch64 public direct-BIR entry should preserve the existing unsupported-module rejection contract after delegating to the shared backend helper");
    return;
  }

  fail("aarch64 public direct-BIR entry should still reject unsupported multi-function modules");
}

void test_backend_bir_pipeline_is_opt_in_through_backend_options() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit backend options should select the BIR pipeline");
  expect_contains(rendered, "%t0 = bir.add i32 2, 3",
                  "explicit BIR selection should route the tiny add slice through the BIR lowering seam");
}

void test_backend_bir_pipeline_accepts_direct_bir_module_input() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_return_immediate_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @tiny_ret() -> i32 {",
                  "backend input should accept a direct BIR module without forcing a legacy LIR wrapper");
  expect_contains(rendered, "bir.ret i32 7",
                  "direct BIR input should flow through the BIR text surface unchanged");
}

void test_backend_bir_pipeline_routes_sub_cluster_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should keep the narrow sub slice on the BIR path");
  expect_contains(rendered, "%t0 = bir.sub i32 3, 3",
                  "explicit BIR selection should expose sub through the BIR-specific route surface");
}

void test_backend_bir_pipeline_routes_sext_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_sext_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @widen_signed(i32 %p.x) -> i64 {",
                  "explicit BIR selection should preserve the widened sext-return signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.sext i32 %p.x to i64",
                  "explicit BIR selection should expose the straight-line sext slice on the BIR text path");
  expect_contains(rendered, "bir.ret i64 %t0",
                  "explicit BIR selection should keep the sext result on the BIR text path");
}

void test_backend_bir_pipeline_routes_zext_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_zext_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @widen_unsigned(i8 %p.x) -> i32 {",
                  "explicit BIR selection should preserve the bounded zext-return signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.zext i8 %p.x to i32",
                  "explicit BIR selection should expose the straight-line zext slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the zext result on the BIR text path");
}

void test_backend_bir_pipeline_routes_trunc_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_trunc_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @narrow_signed(i64 %p.x) -> i32 {",
                  "explicit BIR selection should preserve the bounded trunc-return signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.trunc i64 %p.x to i32",
                  "explicit BIR selection should expose the straight-line trunc slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the trunc result on the BIR text path");
}

void test_backend_bir_pipeline_routes_straight_line_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "%t0 = bir.add i32 2, 3",
                  "explicit BIR selection should preserve the head of the straight-line arithmetic chain");
  expect_contains(rendered, "%t1 = bir.sub i32 %t0, 1",
                  "explicit BIR selection should let later chain instructions consume prior BIR values");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should expose the chain tail on the BIR route surface");
}

void test_backend_bir_pipeline_routes_zero_param_staged_constant_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_staged_constant_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the zero-parameter staged constant signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.add i32 2, 3",
                  "explicit BIR selection should expose the staged constant head on the BIR text path");
  expect_contains(rendered, "%t1 = bir.sub i32 %t0, 1",
                  "explicit BIR selection should preserve the middle subtraction of the staged constant chain");
  expect_contains(rendered, "%t2 = bir.add i32 %t1, 4",
                  "explicit BIR selection should preserve the tail add of the staged constant chain");
}

void test_backend_bir_pipeline_routes_mul_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_mul_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny mul signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.mul i32 6, 7",
                  "explicit BIR selection should expose the widened mul slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the mul result on the BIR text path");
}

void test_backend_bir_pipeline_routes_and_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_and_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny and signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.and i32 14, 11",
                  "explicit BIR selection should expose the widened and slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the and result on the BIR text path");
}

void test_backend_bir_pipeline_routes_or_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_or_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny or signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.or i32 12, 3",
                  "explicit BIR selection should expose the widened or slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the or result on the BIR text path");
}

void test_backend_bir_pipeline_routes_xor_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_xor_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny xor signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.xor i32 12, 10",
                  "explicit BIR selection should expose the widened xor slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the xor result on the BIR text path");
}

void test_backend_bir_pipeline_routes_shl_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_shl_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny shl signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.shl i32 3, 4",
                  "explicit BIR selection should expose the widened shl slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the shl result on the BIR text path");
}

void test_backend_bir_pipeline_routes_lshr_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_lshr_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny lshr signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.lshr i32 16, 2",
                  "explicit BIR selection should expose the widened lshr slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the lshr result on the BIR text path");
}

void test_backend_bir_pipeline_routes_ashr_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_ashr_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny ashr signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.ashr i32 -16, 2",
                  "explicit BIR selection should expose the widened ashr slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the ashr result on the BIR text path");
}

void test_backend_bir_pipeline_routes_sdiv_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_sdiv_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny signed-division signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.sdiv i32 12, 3",
                  "explicit BIR selection should expose the widened signed-division slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the signed-division result on the BIR text path");
}

void test_backend_bir_pipeline_routes_udiv_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_udiv_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny unsigned-division signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.udiv i32 12, 3",
                  "explicit BIR selection should expose the widened unsigned-division slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the unsigned-division result on the BIR text path");
}

void test_backend_bir_pipeline_routes_srem_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_srem_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny signed-remainder signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.srem i32 14, 5",
                  "explicit BIR selection should expose the widened signed-remainder slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the signed-remainder result on the BIR text path");
}

void test_backend_bir_pipeline_routes_urem_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_urem_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny unsigned-remainder signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.urem i32 14, 5",
                  "explicit BIR selection should expose the widened unsigned-remainder slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t0",
                  "explicit BIR selection should keep the unsigned-remainder result on the BIR text path");
}

void test_backend_bir_pipeline_routes_eq_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_eq_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny compare-return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.eq i32 7, 7",
                  "explicit BIR selection should expose the bounded compare materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the widened compare result on the BIR text path");
}

void test_backend_bir_pipeline_routes_ne_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_ne_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny inequality-return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.ne i32 7, 3",
                  "explicit BIR selection should expose the bounded inequality materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the widened inequality result on the BIR text path");
}

void test_backend_bir_pipeline_routes_slt_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_slt_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny signed relational compare-return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.slt i32 3, 7",
                  "explicit BIR selection should expose the bounded signed relational compare materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the widened signed relational compare result on the BIR text path");
}

void test_backend_bir_pipeline_routes_sle_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_sle_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny signed less-than-or-equal compare-return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.sle i32 7, 7",
                  "explicit BIR selection should expose the bounded signed less-than-or-equal compare materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the widened signed less-than-or-equal compare result on the BIR text path");
}

void test_backend_bir_pipeline_routes_sgt_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_sgt_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny signed greater-than compare-return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.sgt i32 7, 3",
                  "explicit BIR selection should expose the bounded signed greater-than compare materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the widened signed greater-than compare result on the BIR text path");
}

void test_backend_bir_pipeline_routes_sge_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_sge_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny signed greater-than-or-equal compare-return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.sge i32 7, 7",
                  "explicit BIR selection should expose the bounded signed greater-than-or-equal compare materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the widened signed greater-than-or-equal compare result on the BIR text path");
}

void test_backend_bir_pipeline_routes_ult_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_ult_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny unsigned relational compare-return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.ult i32 3, 7",
                  "explicit BIR selection should expose the bounded unsigned relational compare materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the widened unsigned relational compare result on the BIR text path");
}

void test_backend_bir_pipeline_routes_ule_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_ule_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny unsigned less-than-or-equal compare-return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.ule i32 7, 7",
                  "explicit BIR selection should expose the bounded unsigned less-than-or-equal compare materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the widened unsigned less-than-or-equal compare result on the BIR text path");
}

void test_backend_bir_pipeline_routes_ugt_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_ugt_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny unsigned greater-than compare-return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.ugt i32 7, 3",
                  "explicit BIR selection should expose the bounded unsigned greater-than compare materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the widened unsigned greater-than compare result on the BIR text path");
}

void test_backend_bir_pipeline_routes_uge_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_uge_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny unsigned greater-than-or-equal compare-return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.uge i32 7, 7",
                  "explicit BIR selection should expose the bounded unsigned greater-than-or-equal compare materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the widened unsigned greater-than-or-equal compare result on the BIR text path");
}

void test_backend_bir_pipeline_routes_select_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_return_select_eq_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the tiny compare-fed select return signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.select eq i32 7, 7, 11, 4",
                  "explicit BIR selection should expose the bounded select materialization slice on the BIR text path");
  expect_contains(rendered, "bir.ret i32 %t1",
                  "explicit BIR selection should keep the fused select result on the BIR text path");
}

void test_backend_bir_pipeline_routes_single_param_select_branch_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_single_param_select_eq_branch_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose(i32 %p.x) -> i32 {",
                  "explicit BIR selection should preserve the one-parameter ternary signature on the BIR text path");
  expect_contains(rendered, "%t.select = bir.select eq i32 %p.x, 7, 11, 4",
                  "explicit BIR selection should collapse the bounded branch-return ternary into the BIR select surface");
  expect_contains(rendered, "bir.ret i32 %t.select",
                  "explicit BIR selection should keep the fused select result on the BIR text path");
}

void test_backend_bir_pipeline_routes_single_param_select_phi_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_single_param_select_eq_phi_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose(i32 %p.x) -> i32 {",
                  "explicit BIR selection should preserve the one-parameter phi-join ternary signature on the BIR text path");
  expect_contains(rendered, "%t8 = bir.select eq i32 %p.x, 7, 11, 4",
                  "explicit BIR selection should collapse the empty branch-only goto chain plus phi join into the BIR select surface");
  expect_contains(rendered, "bir.ret i32 %t8",
                  "explicit BIR selection should keep the fused select result on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_phi_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_select_eq_phi_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose2(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the two-parameter phi-join ternary signature on the BIR text path");
  expect_contains(rendered, "%t8 = bir.select eq i32 %p.x, %p.y, %p.x, %p.y",
                  "explicit BIR selection should collapse the two-parameter phi-join ternary into the BIR select surface");
  expect_contains(rendered, "bir.ret i32 %t8",
                  "explicit BIR selection should keep the fused two-parameter select result on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_predecessor_add_phi_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_select_eq_predecessor_add_phi_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose2_add(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the widened two-parameter predecessor-compute ternary signature on the BIR text path");
  expect_contains(rendered, "%t3 = bir.add i32 %p.x, 5",
                  "explicit BIR selection should keep the then-arm predecessor arithmetic on the BIR text path");
  expect_contains(rendered, "%t4 = bir.add i32 %p.y, 9",
                  "explicit BIR selection should keep the else-arm predecessor arithmetic on the BIR text path");
  expect_contains(rendered, "%t5 = bir.select eq i32 %p.x, %p.y, %t3, %t4",
                  "explicit BIR selection should collapse the predecessor-compute phi join into the bounded BIR select surface");
  expect_contains(rendered, "bir.ret i32 %t5",
                  "explicit BIR selection should keep the fused predecessor-compute select result on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_add_phi_post_join_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_add_phi_post_join_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose2_add_post(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the widened split-predecessor ternary signature on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 5",
                  "explicit BIR selection should keep the then-arm predecessor arithmetic on the BIR text path even when it is separated from the phi by an empty end block");
  expect_contains(rendered, "%t9 = bir.add i32 %p.y, 9",
                  "explicit BIR selection should keep the else-arm predecessor arithmetic on the BIR text path even when it is separated from the phi by an empty end block");
  expect_contains(rendered, "%t10 = bir.select eq i32 %p.x, %p.y, %t8, %t9",
                  "explicit BIR selection should collapse the split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t11 = bir.add i32 %t10, 6",
                  "explicit BIR selection should preserve the bounded post-join add after the fused select for the split-predecessor form");
  expect_contains(rendered, "bir.ret i32 %t11",
                  "explicit BIR selection should keep the split-predecessor join-local arithmetic result on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_add_phi_post_join_add_sub_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_add_phi_post_join_add_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered,
                  "bir.func @choose2_add_post_chain(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the widened simple split-predecessor ternary signature while extending the join-local arithmetic tail to the nearby + 6 - 2 parity shape on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 5",
                  "explicit BIR selection should keep the then-arm predecessor arithmetic for the simple split-predecessor family on the BIR text path");
  expect_contains(rendered, "%t9 = bir.add i32 %p.y, 9",
                  "explicit BIR selection should keep the else-arm predecessor arithmetic for the simple split-predecessor family on the BIR text path");
  expect_contains(rendered, "%t10 = bir.select eq i32 %p.x, %p.y, %t8, %t9",
                  "explicit BIR selection should collapse the simple split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t11 = bir.add i32 %t10, 6",
                  "explicit BIR selection should preserve the first join-local arithmetic step after the simple split-predecessor form");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 2",
                  "explicit BIR selection should preserve the trailing join-local subtraction after the fused select for the simple split-predecessor family");
  expect_contains(rendered, "bir.ret i32 %t12",
                  "explicit BIR selection should keep the widened simple split-predecessor join-local add/sub chain on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_add_phi_post_join_add_sub_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_add_phi_post_join_add_sub_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered,
                  "bir.func @choose2_add_post_chain_tail(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the simple split-predecessor ternary signature while extending the join-local arithmetic tail to the nearby + 6 - 2 + 9 parity shape on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 5",
                  "explicit BIR selection should keep the then-arm predecessor arithmetic for the simple split-predecessor tail-extension slice on the BIR text path");
  expect_contains(rendered, "%t9 = bir.add i32 %p.y, 9",
                  "explicit BIR selection should keep the else-arm predecessor arithmetic for the simple split-predecessor tail-extension slice on the BIR text path");
  expect_contains(rendered, "%t10 = bir.select eq i32 %p.x, %p.y, %t8, %t9",
                  "explicit BIR selection should collapse the simple split-predecessor phi join into the bounded BIR select surface before the longer join-local tail");
  expect_contains(rendered, "%t11 = bir.add i32 %t10, 6",
                  "explicit BIR selection should preserve the first join-local arithmetic step after the simple split-predecessor form");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 2",
                  "explicit BIR selection should preserve the middle join-local subtraction after the fused select for the simple split-predecessor family");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 9",
                  "explicit BIR selection should preserve the trailing join-local add after the fused select for the simple split-predecessor family");
  expect_contains(rendered, "bir.ret i32 %t13",
                  "explicit BIR selection should keep the widened simple split-predecessor join-local add/sub/add chain on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_affine_phi_post_join_add_sub_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_deeper_affine_phi_post_join_add_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered,
                  "bir.func @choose2_deeper_both_post_chain(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the symmetric deeper split-predecessor ternary signature while widening the join-local arithmetic tail on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the deeper split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the deeper split then-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %t9, 5",
                  "explicit BIR selection should keep the deeper split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t11 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the deeper split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 4",
                  "explicit BIR selection should keep the deeper split else-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 7",
                  "explicit BIR selection should keep the deeper split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t14 = bir.select eq i32 %p.x, %p.y, %t10, %t13",
                  "explicit BIR selection should collapse the symmetric deeper split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t15 = bir.add i32 %t14, 6",
                  "explicit BIR selection should preserve the first join-local arithmetic step after the symmetric deeper split-predecessor form");
  expect_contains(rendered, "%t16 = bir.sub i32 %t15, 2",
                  "explicit BIR selection should preserve the trailing join-local subtraction after the fused select");
  expect_contains(rendered, "bir.ret i32 %t16",
                  "explicit BIR selection should keep the widened join-local add/sub chain on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_affine_phi_post_join_add_sub_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_mixed_affine_phi_post_join_add_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose2_mixed_post_chain(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the split-predecessor mixed-affine ternary signature while widening the join-local arithmetic tail on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t11 = bir.sub i32 %t10, 4",
                  "explicit BIR selection should keep the split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t12 = bir.select eq i32 %p.x, %p.y, %t9, %t11",
                  "explicit BIR selection should collapse the mixed split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 6",
                  "explicit BIR selection should preserve the first join-local arithmetic step after the mixed split-predecessor form");
  expect_contains(rendered, "%t14 = bir.sub i32 %t13, 2",
                  "explicit BIR selection should preserve the trailing join-local subtraction after the fused select");
  expect_contains(rendered, "bir.ret i32 %t14",
                  "explicit BIR selection should keep the widened mixed split-predecessor join-local add/sub chain on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_affine_phi_post_join_add_sub_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_mixed_affine_phi_post_join_add_sub_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered,
                  "bir.func @choose2_mixed_post_chain_tail(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the split-predecessor mixed-affine ternary signature while extending the join-local arithmetic tail to the nearby + 6 - 2 + 9 parity shape on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t11 = bir.sub i32 %t10, 4",
                  "explicit BIR selection should keep the split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t12 = bir.select eq i32 %p.x, %p.y, %t9, %t11",
                  "explicit BIR selection should collapse the mixed split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 6",
                  "explicit BIR selection should preserve the first join-local arithmetic step after the mixed split-predecessor form");
  expect_contains(rendered, "%t14 = bir.sub i32 %t13, 2",
                  "explicit BIR selection should preserve the middle join-local subtraction after the fused select");
  expect_contains(rendered, "%t15 = bir.add i32 %t14, 9",
                  "explicit BIR selection should preserve the trailing join-local add after the fused select");
  expect_contains(rendered, "bir.ret i32 %t15",
                  "explicit BIR selection should keep the widened mixed split-predecessor join-local add/sub/add chain on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_affine_phi_post_join_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_mixed_affine_phi_post_join_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose2_mixed_post(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the richer split-predecessor ternary signature on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t11 = bir.sub i32 %t10, 4",
                  "explicit BIR selection should keep the split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t12 = bir.select eq i32 %p.x, %p.y, %t9, %t11",
                  "explicit BIR selection should collapse the richer split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 6",
                  "explicit BIR selection should preserve the bounded post-join add after the richer split-predecessor form");
  expect_contains(rendered, "bir.ret i32 %t13",
                  "explicit BIR selection should keep the richer split-predecessor join-local arithmetic result on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose2_deeper_post(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the next richer split-predecessor ternary signature on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the deeper split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the deeper split then-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %t9, 5",
                  "explicit BIR selection should keep the deeper split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t11 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the mixed split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 4",
                  "explicit BIR selection should keep the mixed split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t13 = bir.select eq i32 %p.x, %p.y, %t10, %t12",
                  "explicit BIR selection should collapse the next richer split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t14 = bir.add i32 %t13, 6",
                  "explicit BIR selection should preserve the bounded post-join add after the next richer split-predecessor form");
  expect_contains(rendered, "bir.ret i32 %t14",
                  "explicit BIR selection should keep the next richer split-predecessor join-local arithmetic result on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered,
                  "bir.func @choose2_deeper_post_chain(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the asymmetric deeper-then split-predecessor ternary signature while widening the join-local arithmetic tail on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the deeper split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the deeper split then-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %t9, 5",
                  "explicit BIR selection should keep the deeper split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t11 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the mixed split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 4",
                  "explicit BIR selection should keep the mixed split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t13 = bir.select eq i32 %p.x, %p.y, %t10, %t12",
                  "explicit BIR selection should collapse the asymmetric deeper-then split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t14 = bir.add i32 %t13, 6",
                  "explicit BIR selection should preserve the first join-local arithmetic step after the asymmetric deeper-then split-predecessor form");
  expect_contains(rendered, "%t15 = bir.sub i32 %t14, 2",
                  "explicit BIR selection should preserve the trailing join-local subtraction after the fused select");
  expect_contains(rendered, "bir.ret i32 %t15",
                  "explicit BIR selection should keep the widened asymmetric deeper-then join-local add/sub chain on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(
      rendered,
      "bir.func @choose2_deeper_post_chain_tail(i32 %p.x, i32 %p.y) -> i32 {",
      "explicit BIR selection should preserve the asymmetric deeper-then split-predecessor ternary signature while extending the join-local affine tail by one more bounded step on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the deeper split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the deeper split then-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %t9, 5",
                  "explicit BIR selection should keep the deeper split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t11 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the mixed split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 4",
                  "explicit BIR selection should keep the mixed split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t13 = bir.select eq i32 %p.x, %p.y, %t10, %t12",
                  "explicit BIR selection should collapse the asymmetric deeper-then split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t14 = bir.add i32 %t13, 6",
                  "explicit BIR selection should preserve the first join-local arithmetic step after the asymmetric deeper-then split-predecessor form");
  expect_contains(rendered, "%t15 = bir.sub i32 %t14, 2",
                  "explicit BIR selection should preserve the second join-local arithmetic step after the fused select");
  expect_contains(rendered, "%t16 = bir.add i32 %t15, 9",
                  "explicit BIR selection should preserve the added third join-local arithmetic step after the fused select");
  expect_contains(rendered, "bir.ret i32 %t16",
                  "explicit BIR selection should keep the extended asymmetric deeper-then join-local arithmetic result on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(
      rendered,
      "bir.func @choose2_mixed_then_deeper_post(i32 %p.x, i32 %p.y) -> i32 {",
      "explicit BIR selection should preserve the mirrored asymmetric deeper-else split-predecessor ternary signature on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the mixed split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the mixed split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the deeper split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t11 = bir.sub i32 %t10, 4",
                  "explicit BIR selection should keep the deeper split else-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t12 = bir.add i32 %t11, 7",
                  "explicit BIR selection should keep the deeper split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t13 = bir.select eq i32 %p.x, %p.y, %t9, %t12",
                  "explicit BIR selection should collapse the mirrored asymmetric split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t14 = bir.add i32 %t13, 6",
                  "explicit BIR selection should preserve the bounded post-join add after the mirrored asymmetric split-predecessor form");
  expect_contains(rendered, "bir.ret i32 %t14",
                  "explicit BIR selection should keep the mirrored asymmetric deeper-else join-local arithmetic result on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(
      rendered,
      "bir.func @choose2_mixed_then_deeper_post_chain(i32 %p.x, i32 %p.y) -> i32 {",
      "explicit BIR selection should preserve the mirrored asymmetric deeper-else split-predecessor ternary signature while widening the join-local arithmetic tail on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the mixed split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the mixed split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the deeper split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t11 = bir.sub i32 %t10, 4",
                  "explicit BIR selection should keep the deeper split else-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t12 = bir.add i32 %t11, 7",
                  "explicit BIR selection should keep the deeper split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t13 = bir.select eq i32 %p.x, %p.y, %t9, %t12",
                  "explicit BIR selection should collapse the mirrored asymmetric split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t14 = bir.add i32 %t13, 6",
                  "explicit BIR selection should preserve the first join-local arithmetic step after the mirrored asymmetric split-predecessor form");
  expect_contains(rendered, "%t15 = bir.sub i32 %t14, 2",
                  "explicit BIR selection should preserve the trailing join-local subtraction after the fused select");
  expect_contains(rendered, "bir.ret i32 %t15",
                  "explicit BIR selection should keep the widened mirrored asymmetric deeper-else join-local add/sub chain on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(
      rendered,
      "bir.func @choose2_mixed_then_deeper_post_chain_tail(i32 %p.x, i32 %p.y) -> i32 {",
      "explicit BIR selection should preserve the mirrored asymmetric deeper-else split-predecessor ternary signature while extending the join-local affine tail by one more bounded step on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the mixed split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the mixed split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the deeper split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t11 = bir.sub i32 %t10, 4",
                  "explicit BIR selection should keep the deeper split else-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t12 = bir.add i32 %t11, 7",
                  "explicit BIR selection should keep the deeper split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t13 = bir.select eq i32 %p.x, %p.y, %t9, %t12",
                  "explicit BIR selection should collapse the mirrored asymmetric split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t14 = bir.add i32 %t13, 6",
                  "explicit BIR selection should preserve the first join-local arithmetic step after the mirrored asymmetric split-predecessor form");
  expect_contains(rendered, "%t15 = bir.sub i32 %t14, 2",
                  "explicit BIR selection should preserve the second join-local arithmetic step after the fused select");
  expect_contains(rendered, "%t16 = bir.add i32 %t15, 9",
                  "explicit BIR selection should preserve the added third join-local arithmetic step after the fused select");
  expect_contains(rendered, "bir.ret i32 %t16",
                  "explicit BIR selection should keep the extended mirrored asymmetric deeper-else join-local arithmetic result on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_affine_phi_post_join_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_deeper_affine_phi_post_join_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose2_deeper_both_post(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the symmetric deeper split-predecessor ternary signature on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the deeper split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the deeper split then-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %t9, 5",
                  "explicit BIR selection should keep the deeper split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t11 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the deeper split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 4",
                  "explicit BIR selection should keep the deeper split else-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 7",
                  "explicit BIR selection should keep the deeper split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t14 = bir.select eq i32 %p.x, %p.y, %t10, %t13",
                  "explicit BIR selection should collapse the symmetric deeper split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t15 = bir.add i32 %t14, 6",
                  "explicit BIR selection should preserve the bounded post-join add after the symmetric deeper split-predecessor form");
  expect_contains(rendered, "bir.ret i32 %t15",
                  "explicit BIR selection should keep the symmetric deeper split-predecessor join-local arithmetic result on the BIR text path");
}

void test_backend_bir_pipeline_routes_mixed_predecessor_select_post_join_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_mixed_predecessor_add_phi_post_join_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_mixed_add() -> i32 {",
                  "explicit BIR selection should preserve the bounded mixed predecessor/immediate conditional signature on the BIR text path");
  expect_contains(rendered, "%t3 = bir.add i32 7, 5",
                  "explicit BIR selection should keep the computed predecessor input on the fused BIR block");
  expect_contains(rendered, "%t4 = bir.select slt i32 2, 3, %t3, 9",
                  "explicit BIR selection should collapse the mixed predecessor/immediate phi join into a bounded BIR select");
  expect_contains(rendered, "%t5 = bir.add i32 %t4, 6",
                  "explicit BIR selection should preserve the bounded post-join add on the fused select result");
  expect_contains(rendered, "bir.ret i32 %t5",
                  "explicit BIR selection should keep the join-local arithmetic result on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_affine_phi_post_join_add_sub_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_bir_two_param_select_eq_split_predecessor_deeper_affine_phi_post_join_add_sub_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(
      rendered,
      "bir.func @choose2_deeper_both_post_chain_tail(i32 %p.x, i32 %p.y) -> i32 {",
      "explicit BIR selection should preserve the symmetric deeper split-predecessor ternary signature while extending the join-local affine tail by one more bounded step on the BIR text path");
  expect_contains(rendered, "%t8 = bir.add i32 %p.x, 8",
                  "explicit BIR selection should keep the deeper split then-arm affine head on the BIR text path");
  expect_contains(rendered, "%t9 = bir.sub i32 %t8, 3",
                  "explicit BIR selection should keep the deeper split then-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t10 = bir.add i32 %t9, 5",
                  "explicit BIR selection should keep the deeper split then-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t11 = bir.add i32 %p.y, 11",
                  "explicit BIR selection should keep the deeper split else-arm affine head on the BIR text path");
  expect_contains(rendered, "%t12 = bir.sub i32 %t11, 4",
                  "explicit BIR selection should keep the deeper split else-arm middle affine step on the BIR text path");
  expect_contains(rendered, "%t13 = bir.add i32 %t12, 7",
                  "explicit BIR selection should keep the deeper split else-arm affine tail on the BIR text path");
  expect_contains(rendered, "%t14 = bir.select eq i32 %p.x, %p.y, %t10, %t13",
                  "explicit BIR selection should collapse the symmetric deeper split-predecessor phi join into the bounded BIR select surface");
  expect_contains(rendered, "%t15 = bir.add i32 %t14, 6",
                  "explicit BIR selection should preserve the first join-local arithmetic step after the fused select");
  expect_contains(rendered, "%t16 = bir.sub i32 %t15, 2",
                  "explicit BIR selection should preserve the second join-local arithmetic step after the fused select");
  expect_contains(rendered, "%t17 = bir.add i32 %t16, 9",
                  "explicit BIR selection should preserve the added third join-local arithmetic step after the fused select");
  expect_contains(rendered, "bir.ret i32 %t17",
                  "explicit BIR selection should keep the extended join-local arithmetic result on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @tiny_char(i8 %p.x) -> i8 {",
                  "explicit BIR selection should preserve widened i8 function signatures on the BIR text path");
  expect_contains(rendered, "%t0 = bir.add i8 %p.x, 2",
                  "explicit BIR selection should route i8 arithmetic through the BIR text surface");
  expect_contains(rendered, "%t1 = bir.sub i8 %t0, 1",
                  "explicit BIR selection should preserve the trailing i8 adjustment on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_two_param_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_two_param_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @add_pair_u(i8 %p.x, i8 %p.y) -> i8 {",
                  "explicit BIR selection should preserve the widened i8 two-parameter signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.add i8 %p.x, %p.y",
                  "explicit BIR selection should route widened i8 two-parameter arithmetic through the BIR surface");
  expect_contains(rendered, "bir.ret i8 %t0",
                  "explicit BIR selection should keep the widened i8 two-parameter result on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_eq_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_eq_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_eq_u() -> i8 {",
                  "explicit BIR selection should preserve the widened i8 equality signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.eq i8 7, 7",
                  "explicit BIR selection should expose the widened i8 equality compare materialization on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_ne_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_ne_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_ne_u() -> i8 {",
                  "explicit BIR selection should preserve the widened i8 inequality signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.ne i8 7, 3",
                  "explicit BIR selection should expose the widened i8 inequality compare materialization on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_ult_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_ult_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_ult_u() -> i8 {",
                  "explicit BIR selection should preserve the widened i8 unsigned-less-than signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.ult i8 3, 7",
                  "explicit BIR selection should expose the widened i8 unsigned-less-than compare materialization on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_ule_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_ule_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_ule_u() -> i8 {",
                  "explicit BIR selection should preserve the widened i8 unsigned-less-than-or-equal signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.ule i8 7, 7",
                  "explicit BIR selection should expose the widened i8 unsigned-less-than-or-equal compare materialization on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_ugt_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_ugt_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_ugt_u() -> i8 {",
                  "explicit BIR selection should preserve the widened i8 unsigned-greater-than signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.ugt i8 7, 3",
                  "explicit BIR selection should expose the widened i8 unsigned-greater-than compare materialization on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_uge_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_uge_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_uge_u() -> i8 {",
                  "explicit BIR selection should preserve the widened i8 unsigned-greater-than-or-equal signature on the BIR text path");
  expect_contains(rendered, "%t1 = bir.uge i8 7, 7",
                  "explicit BIR selection should expose the widened i8 unsigned-greater-than-or-equal compare materialization on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_slt_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_slt_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_slt_u() -> i8 {",
                  "explicit BIR selection should preserve the widened i8 signed-less-than signature on the BIR text path");
  expect_contains(rendered, "%t5 = bir.slt i8 3, 7",
                  "explicit BIR selection should expose the widened i8 signed-less-than compare materialization on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_sle_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_sle_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_sle_u() -> i8 {",
                  "explicit BIR selection should preserve the widened i8 signed-less-than-or-equal signature on the BIR text path");
  expect_contains(rendered, "%t5 = bir.sle i8 7, 7",
                  "explicit BIR selection should expose the widened i8 signed-less-than-or-equal compare materialization on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_sgt_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_sgt_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_sgt_u() -> i8 {",
                  "explicit BIR selection should preserve the widened i8 signed-greater-than signature on the BIR text path");
  expect_contains(rendered, "%t5 = bir.sgt i8 7, 3",
                  "explicit BIR selection should expose the widened i8 signed-greater-than compare materialization on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_sge_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i8_return_sge_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @choose_sge_u() -> i8 {",
                  "explicit BIR selection should preserve the widened i8 signed-greater-than-or-equal signature on the BIR text path");
  expect_contains(rendered, "%t5 = bir.sge i8 7, 7",
                  "explicit BIR selection should expose the widened i8 signed-greater-than-or-equal compare materialization on the BIR text path");
}

void test_backend_bir_pipeline_routes_i8_add_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_add_module,
                      "bir.func @choose_add_u() -> i8 {",
                      "%t0 = bir.add i8 2, 3",
                      "explicit BIR selection for widened i8 add");
}

void test_backend_bir_pipeline_routes_i8_sub_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_sub_module,
                      "bir.func @choose_sub_u() -> i8 {",
                      "%t0 = bir.sub i8 9, 4",
                      "explicit BIR selection for widened i8 sub");
}

void test_backend_bir_pipeline_routes_i8_mul_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_mul_module,
                      "bir.func @choose_mul_u() -> i8 {",
                      "%t0 = bir.mul i8 6, 7",
                      "explicit BIR selection for widened i8 mul");
}

void test_backend_bir_pipeline_routes_i8_and_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_and_module,
                      "bir.func @choose_and_u() -> i8 {",
                      "%t0 = bir.and i8 14, 11",
                      "explicit BIR selection for widened i8 and");
}

void test_backend_bir_pipeline_routes_i8_or_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_or_module,
                      "bir.func @choose_or_u() -> i8 {",
                      "%t0 = bir.or i8 12, 3",
                      "explicit BIR selection for widened i8 or");
}

void test_backend_bir_pipeline_routes_i8_xor_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_xor_module,
                      "bir.func @choose_xor_u() -> i8 {",
                      "%t0 = bir.xor i8 12, 10",
                      "explicit BIR selection for widened i8 xor");
}

void test_backend_bir_pipeline_routes_i8_shl_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_shl_module,
                      "bir.func @choose_shl_u() -> i8 {",
                      "%t0 = bir.shl i8 3, 4",
                      "explicit BIR selection for widened i8 shl");
}

void test_backend_bir_pipeline_routes_i8_lshr_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_lshr_module,
                      "bir.func @choose_lshr_u() -> i8 {",
                      "%t0 = bir.lshr i8 16, 2",
                      "explicit BIR selection for widened i8 lshr");
}

void test_backend_bir_pipeline_routes_i8_ashr_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_ashr_module,
                      "bir.func @choose_ashr_u() -> i8 {",
                      "%t0 = bir.ashr i8 -16, 2",
                      "explicit BIR selection for widened i8 ashr");
}

void test_backend_bir_pipeline_routes_i8_sdiv_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_sdiv_module,
                      "bir.func @choose_sdiv_u() -> i8 {",
                      "%t0 = bir.sdiv i8 12, 3",
                      "explicit BIR selection for widened i8 sdiv");
}

void test_backend_bir_pipeline_routes_i8_udiv_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_udiv_module,
                      "bir.func @choose_udiv_u() -> i8 {",
                      "%t0 = bir.udiv i8 12, 3",
                      "explicit BIR selection for widened i8 udiv");
}

void test_backend_bir_pipeline_routes_i8_srem_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_srem_module,
                      "bir.func @choose_srem_u() -> i8 {",
                      "%t0 = bir.srem i8 14, 5",
                      "explicit BIR selection for widened i8 srem");
}

void test_backend_bir_pipeline_routes_i8_urem_through_bir_text_surface() {
  expect_i8_bir_route(make_bir_i8_return_urem_module,
                      "bir.func @choose_urem_u() -> i8 {",
                      "%t0 = bir.urem i8 14, 5",
                      "explicit BIR selection for widened i8 urem");
}

void test_backend_bir_pipeline_routes_i8_immediate_through_bir_text_surface() {
  expect_i8_bir_immediate_route(make_bir_i8_return_immediate_module,
                                "bir.func @choose_const_u() -> i8 {",
                                "bir.ret i8 11",
                                "explicit BIR selection for widened i8 immediate");
}

void test_backend_bir_pipeline_routes_i64_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_i64_return_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @wide_add(i64 %p.x) -> i64 {",
                  "explicit BIR selection should preserve widened i64 function signatures on the BIR text path");
  expect_contains(rendered, "%t0 = bir.add i64 %p.x, 2",
                  "explicit BIR selection should route i64 arithmetic through the BIR text surface");
  expect_contains(rendered, "%t1 = bir.sub i64 %t0, 1",
                  "explicit BIR selection should preserve the trailing i64 adjustment on the BIR text path");
}

void test_backend_bir_pipeline_routes_single_param_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_single_param_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @add_one(i32 %p.x) -> i32 {",
                  "explicit BIR selection should preserve the bounded one-parameter signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.add i32 %p.x, 2",
                  "explicit BIR selection should route parameter-fed arithmetic through the BIR surface");
  expect_contains(rendered, "%t1 = bir.sub i32 %t0, 1",
                  "explicit BIR selection should keep the bounded parameter-fed chain in BIR text form");
}

void test_backend_bir_pipeline_routes_two_param_add_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_add_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @add_pair(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the bounded two-parameter signature on the BIR text path");
  expect_contains(rendered, "%t0 = bir.add i32 %p.x, %p.y",
                  "explicit BIR selection should route two-parameter affine arithmetic through the BIR surface");
}

void test_backend_bir_pipeline_routes_two_param_add_sub_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_add_sub_chain_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @add_pair(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the bounded two-parameter signature for the affine chain");
  expect_contains(rendered, "%t0 = bir.add i32 %p.x, %p.y",
                  "explicit BIR selection should keep the two-parameter add head of the bounded affine chain on the BIR text path");
  expect_contains(rendered, "%t1 = bir.sub i32 %t0, 1",
                  "explicit BIR selection should expose the trailing immediate adjustment on the BIR text path");
}

void test_backend_bir_pipeline_routes_two_param_staged_affine_chain_through_bir_text_surface() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_bir_two_param_staged_affine_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @add_pair(i32 %p.x, i32 %p.y) -> i32 {",
                  "explicit BIR selection should preserve the two-parameter signature for the staged affine chain");
  expect_contains(rendered, "%t0 = bir.add i32 %p.x, 2",
                  "explicit BIR selection should expose the staged immediate head on the BIR text path");
  expect_contains(rendered, "%t1 = bir.add i32 %t0, %p.y",
                  "explicit BIR selection should keep the later second-parameter add on the BIR text path");
  expect_contains(rendered, "%t2 = bir.sub i32 %t1, 1",
                  "explicit BIR selection should expose the trailing staged subtraction on the BIR text path");
}

void test_backend_bir_pipeline_routes_dead_local_add_store_chain_to_immediate_return() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_dead_local_add_store_return_immediate_module()},
      make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_contains(rendered, "bir.func @main() -> i32 {",
                  "explicit BIR selection should preserve the dead-local add/store test signature on the BIR text path");
  expect_contains(rendered, "bir.ret i32 0",
                  "explicit BIR selection should fold the dead local-slot add/store chain to the fixed immediate return");
  expect_not_contains(rendered, "bir.add",
                      "explicit BIR selection should not keep dead local-slot arithmetic once the chain is proven unobserved");
}

void test_backend_bir_pipeline_selection_only_applies_at_lir_entry_input() {
  const auto lir_module = make_bir_return_add_module();
  const auto bir_module = c4c::backend::lower_to_bir(lir_module);
  const auto expected = c4c::backend::bir::print(bir_module);

  const auto rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{bir_module},
                                make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_true(rendered == expected,
              "explicit prelowered BIR emission should stay on the BIR-owned passthrough text surface");
  expect_contains(rendered, "bir.func",
                  "explicit prelowered BIR emission should stay on the BIR text path");
}

void test_backend_lowered_riscv_passthrough_is_detached_from_lir_metadata() {
  const auto bir_module = c4c::backend::lower_to_bir(make_bir_return_add_module());
  const auto expected = c4c::backend::bir::print(bir_module);

  const auto rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{bir_module},
                                make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_true(rendered == expected,
              "once callers emit an explicit prelowered RISC-V BIR module, the output should come only from the BIR-owned passthrough text surface");
  expect_contains(rendered, "bir.func",
                  "explicit prelowered RISC-V BIR emission should stay on the BIR text surface");
}

void test_backend_riscv64_supported_lir_and_direct_bir_share_text_route() {
  const auto lir_module = make_bir_return_add_module();
  const auto bir_module = c4c::backend::lower_to_bir(lir_module);
  const auto lir_rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{lir_module},
                                make_bir_pipeline_options(c4c::backend::Target::Riscv64));
  const auto bir_rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{bir_module},
                                make_bir_pipeline_options(c4c::backend::Target::Riscv64));

  expect_true(lir_rendered == bir_rendered,
              "supported riscv64 backend entry should render the same BIR text whether callers pass fresh LIR or prelowered BIR");
  expect_contains(lir_rendered, "bir.func",
                  "supported riscv64 backend entry should stay on the shared BIR text surface");
}

void test_backend_direct_bir_i686_target_uses_x86_stack_arg_emitter() {
  auto bir_module = c4c::backend::lower_to_bir(make_bir_two_param_add_module());
  bir_module.target_triple = "i686-unknown-linux-gnu";

  const auto rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{bir_module},
                                make_bir_pipeline_options(c4c::backend::Target::I686));

  expect_contains(rendered, "DWORD PTR [esp + 4]",
                  "direct i686 BIR emission should still dispatch through the x86 stack-arg path");
  expect_contains(rendered, "DWORD PTR [esp + 8]",
                  "direct i686 BIR emission should keep the second stack-arg load");
}

void test_x86_public_lir_emitter_keeps_i686_target_selection_on_shared_backend_entry() {
  auto lir_module = make_bir_two_param_add_module();
  lir_module.target_triple = "i686-unknown-linux-gnu";

  const auto shared_rendered =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{lir_module},
                                make_bir_pipeline_options(c4c::backend::Target::I686));
  const auto x86_rendered = c4c::backend::x86::emit_module(lir_module);

  expect_true(x86_rendered == shared_rendered,
              "x86 public LIR entry should delegate i686 target selection through the shared backend helper instead of owning a separate compatibility-only route policy");
  expect_contains(x86_rendered, "DWORD PTR [esp + 4]",
                  "x86 public LIR entry should still preserve the i686 stack-arg emission contract after shared target selection moves into backend.cpp");
  expect_contains(x86_rendered, "DWORD PTR [esp + 8]",
                  "x86 public LIR entry should still preserve the second i686 stack-arg load after shared target selection moves into backend.cpp");
}

}  // namespace

void run_backend_bir_pipeline_tests() {
  RUN_TEST(test_backend_default_input_keeps_lir_entry_observable);
  RUN_TEST(test_backend_direct_bir_input_keeps_bir_ownership_observable);
  RUN_TEST(test_backend_default_path_uses_bir_when_bir_pipeline_is_not_selected);
  RUN_TEST(test_backend_riscv64_path_keeps_unsupported_lir_on_llvm_text_surface);
  RUN_TEST(test_backend_riscv64_path_keeps_unsupported_direct_lir_on_llvm_text_surface);
  RUN_TEST(test_backend_entry_rejects_unsupported_direct_lir_input_on_x86);
  RUN_TEST(test_backend_entry_rejects_unsupported_direct_lir_input_on_aarch64);
  RUN_TEST(test_x86_try_emit_module_reports_direct_bir_support_explicitly);
  RUN_TEST(test_aarch64_try_emit_module_reports_direct_bir_support_explicitly);
  RUN_TEST(test_x86_try_emit_prepared_lir_module_reports_direct_lir_support_explicitly);
  RUN_TEST(test_x86_try_emit_prepared_lir_module_accepts_variadic_sum2_runtime_slice);
  RUN_TEST(test_x86_try_emit_prepared_lir_module_accepts_variadic_double_bytes_runtime_slice);
  RUN_TEST(test_x86_direct_variadic_helper_accepts_variadic_sum2_runtime_slice);
  RUN_TEST(test_x86_direct_printf_helper_accepts_repeated_printf_immediates_slice);
  RUN_TEST(test_x86_direct_printf_helper_accepts_local_buffer_string_copy_printf_slice);
  RUN_TEST(test_x86_direct_printf_helper_accepts_counted_printf_ternary_loop_slice);
  RUN_TEST(test_x86_direct_printf_helper_accepts_string_literal_char_slice);
  RUN_TEST(test_x86_direct_void_helper_accepts_void_direct_call_return_slice);
  RUN_TEST(test_aarch64_try_emit_prepared_lir_module_reports_direct_lir_support_explicitly);
  RUN_TEST(test_aarch64_try_emit_prepared_lir_module_accepts_pointer_phi_join_modules);
  RUN_TEST(test_x86_public_bir_emitter_delegates_direct_bir_route_to_shared_backend);
  RUN_TEST(test_aarch64_public_bir_emitter_delegates_direct_bir_route_to_shared_backend);
  RUN_TEST(test_backend_bir_pipeline_is_opt_in_through_backend_options);
  RUN_TEST(test_backend_bir_pipeline_accepts_direct_bir_module_input);
  RUN_TEST(test_backend_bir_pipeline_routes_sub_cluster_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_sext_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_zext_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_trunc_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_straight_line_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_zero_param_staged_constant_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_mul_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_and_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_or_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_xor_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_shl_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_lshr_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_ashr_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_sdiv_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_udiv_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_srem_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_urem_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_eq_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_ne_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_slt_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_sle_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_sgt_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_sge_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_ult_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_ule_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_ugt_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_uge_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_select_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_single_param_select_branch_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_single_param_select_phi_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_phi_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_predecessor_add_phi_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_add_phi_post_join_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_add_phi_post_join_add_sub_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_add_phi_post_join_add_sub_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_affine_phi_post_join_add_sub_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_affine_phi_post_join_add_sub_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_affine_phi_post_join_add_sub_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_affine_phi_post_join_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_affine_phi_post_join_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_select_split_predecessor_deeper_affine_phi_post_join_add_sub_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_mixed_predecessor_select_post_join_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_two_param_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_eq_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_ne_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_ult_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_ule_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_ugt_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_uge_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_slt_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_sle_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_sgt_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_sge_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_sub_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_mul_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_and_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_or_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_xor_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_shl_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_lshr_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_ashr_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_sdiv_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_udiv_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_srem_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_urem_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i8_immediate_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_i64_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_single_param_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_add_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_add_sub_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_two_param_staged_affine_chain_through_bir_text_surface);
  RUN_TEST(test_backend_bir_pipeline_routes_dead_local_add_store_chain_to_immediate_return);
  RUN_TEST(test_backend_bir_pipeline_selection_only_applies_at_lir_entry_input);
  RUN_TEST(test_backend_lowered_riscv_passthrough_is_detached_from_lir_metadata);
  RUN_TEST(test_backend_riscv64_supported_lir_and_direct_bir_share_text_route);
  RUN_TEST(test_backend_direct_bir_i686_target_uses_x86_stack_arg_emitter);
  RUN_TEST(test_x86_public_lir_emitter_keeps_i686_target_selection_on_shared_backend_entry);
}
