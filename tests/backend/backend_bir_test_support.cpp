#include "backend_bir_test_support.hpp"

#include "backend_test_fixtures.hpp"

c4c::codegen::lir::LirModule make_bir_return_add_module() {
  return make_return_add_module();
}

c4c::codegen::lir::LirModule make_bir_return_sub_module() {
  return make_return_sub_module();
}

c4c::codegen::lir::LirModule make_bir_return_add_sub_chain_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.push_back(LirBinOp{"%t1", "sub", "i32", "%t0", "1"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_staged_constant_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "2", "3"});
  entry.insts.push_back(LirBinOp{"%t1", "sub", "i32", "%t0", "1"});
  entry.insts.push_back(LirBinOp{"%t2", "add", "i32", "%t1", "4"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_mul_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirBinOp{"%t0", "mul", "i32", "6", "7"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_and_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirBinOp{"%t0", "and", "i32", "14", "11"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_or_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirBinOp{"%t0", "or", "i32", "12", "3"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_xor_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirBinOp{"%t0", "xor", "i32", "12", "10"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_shl_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirBinOp{"%t0", "shl", "i32", "3", "4"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_lshr_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirBinOp{"%t0", "lshr", "i32", "16", "2"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_ashr_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirBinOp{"%t0", "ashr", "i32", "-16", "2"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_sdiv_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirBinOp{"%t0", "sdiv", "i32", "12", "3"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_udiv_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirBinOp{"%t0", "udiv", "i32", "12", "3"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_srem_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirBinOp{"%t0", "srem", "i32", "14", "5"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_urem_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirBinOp{"%t0", "urem", "i32", "14", "5"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_eq_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "7", "7"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_ne_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirCmpOp{"%t0", false, "ne", "i32", "7", "3"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_slt_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirCmpOp{"%t0", false, "slt", "i32", "3", "7"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_sle_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirCmpOp{"%t0", false, "sle", "i32", "7", "7"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_sgt_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirCmpOp{"%t0", false, "sgt", "i32", "7", "3"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_sge_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirCmpOp{"%t0", false, "sge", "i32", "7", "7"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_ult_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirCmpOp{"%t0", false, "ult", "i32", "3", "7"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_ule_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirCmpOp{"%t0", false, "ule", "i32", "7", "7"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_ugt_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirCmpOp{"%t0", false, "ugt", "i32", "7", "3"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_uge_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirCmpOp{"%t0", false, "uge", "i32", "7", "7"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_select_eq_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "7", "7"});
  entry.insts.push_back(LirSelectOp{"%t1", "i32", "%t0", "11", "4"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_single_param_select_eq_branch_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "choose";
  function.signature_text = "define i32 @choose(i32 %p.x)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "%p.x", "7"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "then", "else"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "then";
  true_block.terminator = LirRet{std::string("11"), "i32"};
  function.blocks.push_back(std::move(true_block));

  LirBlock false_block;
  false_block.id = LirBlockId{2};
  false_block.label = "else";
  false_block.terminator = LirRet{std::string("4"), "i32"};
  function.blocks.push_back(std::move(false_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_single_param_select_eq_phi_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "choose";
  function.signature_text = "define i32 @choose(i32 %p.x)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "%p.x", "7"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.terminator = LirBr{"tern.then.end.4"};
  function.blocks.push_back(std::move(true_block));

  LirBlock true_end_block;
  true_end_block.id = LirBlockId{2};
  true_end_block.label = "tern.then.end.4";
  true_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(true_end_block));

  LirBlock false_block;
  false_block.id = LirBlockId{3};
  false_block.label = "tern.else.5";
  false_block.terminator = LirBr{"tern.else.end.6"};
  function.blocks.push_back(std::move(false_block));

  LirBlock false_end_block;
  false_end_block.id = LirBlockId{4};
  false_end_block.label = "tern.else.end.6";
  false_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(false_end_block));

  LirBlock join_block;
  join_block.id = LirBlockId{5};
  join_block.label = "tern.end.7";
  join_block.insts.push_back(
      LirPhiOp{"%t8", "i32", {{"11", "tern.then.end.4"}, {"4", "tern.else.end.6"}}});
  join_block.terminator = LirRet{std::string("%t8"), "i32"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_two_param_select_eq_phi_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "choose2";
  function.signature_text = "define i32 @choose2(i32 %p.x, i32 %p.y)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "%p.x", "%p.y"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.terminator = LirBr{"tern.then.end.4"};
  function.blocks.push_back(std::move(true_block));

  LirBlock true_end_block;
  true_end_block.id = LirBlockId{2};
  true_end_block.label = "tern.then.end.4";
  true_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(true_end_block));

  LirBlock false_block;
  false_block.id = LirBlockId{3};
  false_block.label = "tern.else.5";
  false_block.terminator = LirBr{"tern.else.end.6"};
  function.blocks.push_back(std::move(false_block));

  LirBlock false_end_block;
  false_end_block.id = LirBlockId{4};
  false_end_block.label = "tern.else.end.6";
  false_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(false_end_block));

  LirBlock join_block;
  join_block.id = LirBlockId{5};
  join_block.label = "tern.end.7";
  join_block.insts.push_back(
      LirPhiOp{"%t8", "i32", {{"%p.x", "tern.then.end.4"}, {"%p.y", "tern.else.end.6"}}});
  join_block.terminator = LirRet{std::string("%t8"), "i32"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_two_param_select_eq_predecessor_add_phi_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "choose2_add";
  function.signature_text = "define i32 @choose2_add(i32 %p.x, i32 %p.y)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "%p.x", "%p.y"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "tern.then", "tern.else"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then";
  true_block.insts.push_back(LirBinOp{"%t3", "add", "i32", "%p.x", "5"});
  true_block.terminator = LirBr{"tern.end"};
  function.blocks.push_back(std::move(true_block));

  LirBlock false_block;
  false_block.id = LirBlockId{2};
  false_block.label = "tern.else";
  false_block.insts.push_back(LirBinOp{"%t4", "add", "i32", "%p.y", "9"});
  false_block.terminator = LirBr{"tern.end"};
  function.blocks.push_back(std::move(false_block));

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "tern.end";
  join_block.insts.push_back(
      LirPhiOp{"%t5", "i32", {{"%t3", "tern.then"}, {"%t4", "tern.else"}}});
  join_block.terminator = LirRet{std::string("%t5"), "i32"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_select_eq_split_predecessor_add_phi_post_join_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "choose2_add_post";
  function.signature_text = "define i32 @choose2_add_post(i32 %p.x, i32 %p.y)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "%p.x", "%p.y"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "%p.x", "5"});
  true_block.terminator = LirBr{"tern.then.end.4"};
  function.blocks.push_back(std::move(true_block));

  LirBlock true_end_block;
  true_end_block.id = LirBlockId{2};
  true_end_block.label = "tern.then.end.4";
  true_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(true_end_block));

  LirBlock false_block;
  false_block.id = LirBlockId{3};
  false_block.label = "tern.else.5";
  false_block.insts.push_back(LirBinOp{"%t9", "add", "i32", "%p.y", "9"});
  false_block.terminator = LirBr{"tern.else.end.6"};
  function.blocks.push_back(std::move(false_block));

  LirBlock false_end_block;
  false_end_block.id = LirBlockId{4};
  false_end_block.label = "tern.else.end.6";
  false_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(false_end_block));

  LirBlock join_block;
  join_block.id = LirBlockId{5};
  join_block.label = "tern.end.7";
  join_block.insts.push_back(
      LirPhiOp{"%t10", "i32", {{"%t8", "tern.then.end.4"}, {"%t9", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t11", "add", "i32", "%t10", "6"});
  join_block.terminator = LirRet{std::string("%t11"), "i32"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_select_eq_split_predecessor_add_phi_post_join_add_sub_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "choose2_add_post_chain";
  function.signature_text = "define i32 @choose2_add_post_chain(i32 %p.x, i32 %p.y)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "%p.x", "%p.y"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "%p.x", "5"});
  true_block.terminator = LirBr{"tern.then.end.4"};
  function.blocks.push_back(std::move(true_block));

  LirBlock true_end_block;
  true_end_block.id = LirBlockId{2};
  true_end_block.label = "tern.then.end.4";
  true_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(true_end_block));

  LirBlock false_block;
  false_block.id = LirBlockId{3};
  false_block.label = "tern.else.5";
  false_block.insts.push_back(LirBinOp{"%t9", "add", "i32", "%p.y", "9"});
  false_block.terminator = LirBr{"tern.else.end.6"};
  function.blocks.push_back(std::move(false_block));

  LirBlock false_end_block;
  false_end_block.id = LirBlockId{4};
  false_end_block.label = "tern.else.end.6";
  false_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(false_end_block));

  LirBlock join_block;
  join_block.id = LirBlockId{5};
  join_block.label = "tern.end.7";
  join_block.insts.push_back(
      LirPhiOp{"%t10", "i32", {{"%t8", "tern.then.end.4"}, {"%t9", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t11", "add", "i32", "%t10", "6"});
  join_block.insts.push_back(LirBinOp{"%t12", "sub", "i32", "%t11", "2"});
  join_block.terminator = LirRet{std::string("%t12"), "i32"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_select_eq_split_predecessor_add_phi_post_join_add_sub_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "choose2_add_post_chain_tail";
  function.signature_text = "define i32 @choose2_add_post_chain_tail(i32 %p.x, i32 %p.y)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "%p.x", "%p.y"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "%p.x", "5"});
  true_block.terminator = LirBr{"tern.then.end.4"};
  function.blocks.push_back(std::move(true_block));

  LirBlock true_end_block;
  true_end_block.id = LirBlockId{2};
  true_end_block.label = "tern.then.end.4";
  true_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(true_end_block));

  LirBlock false_block;
  false_block.id = LirBlockId{3};
  false_block.label = "tern.else.5";
  false_block.insts.push_back(LirBinOp{"%t9", "add", "i32", "%p.y", "9"});
  false_block.terminator = LirBr{"tern.else.end.6"};
  function.blocks.push_back(std::move(false_block));

  LirBlock false_end_block;
  false_end_block.id = LirBlockId{4};
  false_end_block.label = "tern.else.end.6";
  false_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(false_end_block));

  LirBlock join_block;
  join_block.id = LirBlockId{5};
  join_block.label = "tern.end.7";
  join_block.insts.push_back(
      LirPhiOp{"%t10", "i32", {{"%t8", "tern.then.end.4"}, {"%t9", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t11", "add", "i32", "%t10", "6"});
  join_block.insts.push_back(LirBinOp{"%t12", "sub", "i32", "%t11", "2"});
  join_block.insts.push_back(LirBinOp{"%t13", "add", "i32", "%t12", "9"});
  join_block.terminator = LirRet{std::string("%t13"), "i32"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_select_eq_split_predecessor_deeper_affine_phi_post_join_add_sub_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "choose2_deeper_both_post_chain";
  function.signature_text =
      "define i32 @choose2_deeper_both_post_chain(i32 %p.x, i32 %p.y)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "%p.x", "%p.y"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "%p.x", "8"});
  true_block.insts.push_back(LirBinOp{"%t9", "sub", "i32", "%t8", "3"});
  true_block.insts.push_back(LirBinOp{"%t10", "add", "i32", "%t9", "5"});
  true_block.terminator = LirBr{"tern.then.end.4"};
  function.blocks.push_back(std::move(true_block));

  LirBlock true_end_block;
  true_end_block.id = LirBlockId{2};
  true_end_block.label = "tern.then.end.4";
  true_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(true_end_block));

  LirBlock false_block;
  false_block.id = LirBlockId{3};
  false_block.label = "tern.else.5";
  false_block.insts.push_back(LirBinOp{"%t11", "add", "i32", "%p.y", "11"});
  false_block.insts.push_back(LirBinOp{"%t12", "sub", "i32", "%t11", "4"});
  false_block.insts.push_back(LirBinOp{"%t13", "add", "i32", "%t12", "7"});
  false_block.terminator = LirBr{"tern.else.end.6"};
  function.blocks.push_back(std::move(false_block));

  LirBlock false_end_block;
  false_end_block.id = LirBlockId{4};
  false_end_block.label = "tern.else.end.6";
  false_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(false_end_block));

  LirBlock join_block;
  join_block.id = LirBlockId{5};
  join_block.label = "tern.end.7";
  join_block.insts.push_back(
      LirPhiOp{"%t14", "i32", {{"%t10", "tern.then.end.4"}, {"%t13", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t15", "add", "i32", "%t14", "6"});
  join_block.insts.push_back(LirBinOp{"%t16", "sub", "i32", "%t15", "2"});
  join_block.terminator = LirRet{std::string("%t16"), "i32"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_select_eq_split_predecessor_mixed_affine_phi_post_join_add_sub_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "choose2_mixed_post_chain";
  function.signature_text =
      "define i32 @choose2_mixed_post_chain(i32 %p.x, i32 %p.y)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "%p.x", "%p.y"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "%p.x", "8"});
  true_block.insts.push_back(LirBinOp{"%t9", "sub", "i32", "%t8", "3"});
  true_block.terminator = LirBr{"tern.then.end.4"};
  function.blocks.push_back(std::move(true_block));

  LirBlock true_end_block;
  true_end_block.id = LirBlockId{2};
  true_end_block.label = "tern.then.end.4";
  true_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(true_end_block));

  LirBlock false_block;
  false_block.id = LirBlockId{3};
  false_block.label = "tern.else.5";
  false_block.insts.push_back(LirBinOp{"%t10", "add", "i32", "%p.y", "11"});
  false_block.insts.push_back(LirBinOp{"%t11", "sub", "i32", "%t10", "4"});
  false_block.terminator = LirBr{"tern.else.end.6"};
  function.blocks.push_back(std::move(false_block));

  LirBlock false_end_block;
  false_end_block.id = LirBlockId{4};
  false_end_block.label = "tern.else.end.6";
  false_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(false_end_block));

  LirBlock join_block;
  join_block.id = LirBlockId{5};
  join_block.label = "tern.end.7";
  join_block.insts.push_back(
      LirPhiOp{"%t12", "i32", {{"%t9", "tern.then.end.4"}, {"%t11", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t13", "add", "i32", "%t12", "6"});
  join_block.insts.push_back(LirBinOp{"%t14", "sub", "i32", "%t13", "2"});
  join_block.terminator = LirRet{std::string("%t14"), "i32"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_select_eq_split_predecessor_mixed_affine_phi_post_join_add_sub_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "choose2_mixed_post_chain_tail";
  function.signature_text =
      "define i32 @choose2_mixed_post_chain_tail(i32 %p.x, i32 %p.y)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "%p.x", "%p.y"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "%p.x", "8"});
  true_block.insts.push_back(LirBinOp{"%t9", "sub", "i32", "%t8", "3"});
  true_block.terminator = LirBr{"tern.then.end.4"};
  function.blocks.push_back(std::move(true_block));

  LirBlock true_end_block;
  true_end_block.id = LirBlockId{2};
  true_end_block.label = "tern.then.end.4";
  true_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(true_end_block));

  LirBlock false_block;
  false_block.id = LirBlockId{3};
  false_block.label = "tern.else.5";
  false_block.insts.push_back(LirBinOp{"%t10", "add", "i32", "%p.y", "11"});
  false_block.insts.push_back(LirBinOp{"%t11", "sub", "i32", "%t10", "4"});
  false_block.terminator = LirBr{"tern.else.end.6"};
  function.blocks.push_back(std::move(false_block));

  LirBlock false_end_block;
  false_end_block.id = LirBlockId{4};
  false_end_block.label = "tern.else.end.6";
  false_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(false_end_block));

  LirBlock join_block;
  join_block.id = LirBlockId{5};
  join_block.label = "tern.end.7";
  join_block.insts.push_back(
      LirPhiOp{"%t12", "i32", {{"%t9", "tern.then.end.4"}, {"%t11", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t13", "add", "i32", "%t12", "6"});
  join_block.insts.push_back(LirBinOp{"%t14", "sub", "i32", "%t13", "2"});
  join_block.insts.push_back(LirBinOp{"%t15", "add", "i32", "%t14", "9"});
  join_block.terminator = LirRet{std::string("%t15"), "i32"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_select_eq_split_predecessor_mixed_affine_phi_post_join_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "choose2_mixed_post";
  function.signature_text = "define i32 @choose2_mixed_post(i32 %p.x, i32 %p.y)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "%p.x", "%p.y"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "%p.x", "8"});
  true_block.insts.push_back(LirBinOp{"%t9", "sub", "i32", "%t8", "3"});
  true_block.terminator = LirBr{"tern.then.end.4"};
  function.blocks.push_back(std::move(true_block));

  LirBlock true_end_block;
  true_end_block.id = LirBlockId{2};
  true_end_block.label = "tern.then.end.4";
  true_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(true_end_block));

  LirBlock false_block;
  false_block.id = LirBlockId{3};
  false_block.label = "tern.else.5";
  false_block.insts.push_back(LirBinOp{"%t10", "add", "i32", "%p.y", "11"});
  false_block.insts.push_back(LirBinOp{"%t11", "sub", "i32", "%t10", "4"});
  false_block.terminator = LirBr{"tern.else.end.6"};
  function.blocks.push_back(std::move(false_block));

  LirBlock false_end_block;
  false_end_block.id = LirBlockId{4};
  false_end_block.label = "tern.else.end.6";
  false_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(false_end_block));

  LirBlock join_block;
  join_block.id = LirBlockId{5};
  join_block.label = "tern.end.7";
  join_block.insts.push_back(
      LirPhiOp{"%t12", "i32", {{"%t9", "tern.then.end.4"}, {"%t11", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t13", "add", "i32", "%t12", "6"});
  join_block.terminator = LirRet{std::string("%t13"), "i32"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "choose2_deeper_post";
  function.signature_text = "define i32 @choose2_deeper_post(i32 %p.x, i32 %p.y)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "%p.x", "%p.y"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "%p.x", "8"});
  true_block.insts.push_back(LirBinOp{"%t9", "sub", "i32", "%t8", "3"});
  true_block.insts.push_back(LirBinOp{"%t10", "add", "i32", "%t9", "5"});
  true_block.terminator = LirBr{"tern.then.end.4"};
  function.blocks.push_back(std::move(true_block));

  LirBlock true_end_block;
  true_end_block.id = LirBlockId{2};
  true_end_block.label = "tern.then.end.4";
  true_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(true_end_block));

  LirBlock false_block;
  false_block.id = LirBlockId{3};
  false_block.label = "tern.else.5";
  false_block.insts.push_back(LirBinOp{"%t11", "add", "i32", "%p.y", "11"});
  false_block.insts.push_back(LirBinOp{"%t12", "sub", "i32", "%t11", "4"});
  false_block.terminator = LirBr{"tern.else.end.6"};
  function.blocks.push_back(std::move(false_block));

  LirBlock false_end_block;
  false_end_block.id = LirBlockId{4};
  false_end_block.label = "tern.else.end.6";
  false_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(false_end_block));

  LirBlock join_block;
  join_block.id = LirBlockId{5};
  join_block.label = "tern.end.7";
  join_block.insts.push_back(
      LirPhiOp{"%t13", "i32", {{"%t10", "tern.then.end.4"}, {"%t12", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t14", "add", "i32", "%t13", "6"});
  join_block.terminator = LirRet{std::string("%t14"), "i32"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "choose2_deeper_post_chain";
  function.signature_text =
      "define i32 @choose2_deeper_post_chain(i32 %p.x, i32 %p.y)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "%p.x", "%p.y"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "%p.x", "8"});
  true_block.insts.push_back(LirBinOp{"%t9", "sub", "i32", "%t8", "3"});
  true_block.insts.push_back(LirBinOp{"%t10", "add", "i32", "%t9", "5"});
  true_block.terminator = LirBr{"tern.then.end.4"};
  function.blocks.push_back(std::move(true_block));

  LirBlock true_end_block;
  true_end_block.id = LirBlockId{2};
  true_end_block.label = "tern.then.end.4";
  true_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(true_end_block));

  LirBlock false_block;
  false_block.id = LirBlockId{3};
  false_block.label = "tern.else.5";
  false_block.insts.push_back(LirBinOp{"%t11", "add", "i32", "%p.y", "11"});
  false_block.insts.push_back(LirBinOp{"%t12", "sub", "i32", "%t11", "4"});
  false_block.terminator = LirBr{"tern.else.end.6"};
  function.blocks.push_back(std::move(false_block));

  LirBlock false_end_block;
  false_end_block.id = LirBlockId{4};
  false_end_block.label = "tern.else.end.6";
  false_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(false_end_block));

  LirBlock join_block;
  join_block.id = LirBlockId{5};
  join_block.label = "tern.end.7";
  join_block.insts.push_back(
      LirPhiOp{"%t13", "i32", {{"%t10", "tern.then.end.4"}, {"%t12", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t14", "add", "i32", "%t13", "6"});
  join_block.insts.push_back(LirBinOp{"%t15", "sub", "i32", "%t14", "2"});
  join_block.terminator = LirRet{std::string("%t15"), "i32"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "choose2_deeper_post_chain_tail";
  function.signature_text =
      "define i32 @choose2_deeper_post_chain_tail(i32 %p.x, i32 %p.y)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "%p.x", "%p.y"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "%p.x", "8"});
  true_block.insts.push_back(LirBinOp{"%t9", "sub", "i32", "%t8", "3"});
  true_block.insts.push_back(LirBinOp{"%t10", "add", "i32", "%t9", "5"});
  true_block.terminator = LirBr{"tern.then.end.4"};
  function.blocks.push_back(std::move(true_block));

  LirBlock true_end_block;
  true_end_block.id = LirBlockId{2};
  true_end_block.label = "tern.then.end.4";
  true_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(true_end_block));

  LirBlock false_block;
  false_block.id = LirBlockId{3};
  false_block.label = "tern.else.5";
  false_block.insts.push_back(LirBinOp{"%t11", "add", "i32", "%p.y", "11"});
  false_block.insts.push_back(LirBinOp{"%t12", "sub", "i32", "%t11", "4"});
  false_block.terminator = LirBr{"tern.else.end.6"};
  function.blocks.push_back(std::move(false_block));

  LirBlock false_end_block;
  false_end_block.id = LirBlockId{4};
  false_end_block.label = "tern.else.end.6";
  false_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(false_end_block));

  LirBlock join_block;
  join_block.id = LirBlockId{5};
  join_block.label = "tern.end.7";
  join_block.insts.push_back(
      LirPhiOp{"%t13", "i32", {{"%t10", "tern.then.end.4"}, {"%t12", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t14", "add", "i32", "%t13", "6"});
  join_block.insts.push_back(LirBinOp{"%t15", "sub", "i32", "%t14", "2"});
  join_block.insts.push_back(LirBinOp{"%t16", "add", "i32", "%t15", "9"});
  join_block.terminator = LirRet{std::string("%t16"), "i32"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "choose2_mixed_then_deeper_post";
  function.signature_text =
      "define i32 @choose2_mixed_then_deeper_post(i32 %p.x, i32 %p.y)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "%p.x", "%p.y"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "%p.x", "8"});
  true_block.insts.push_back(LirBinOp{"%t9", "sub", "i32", "%t8", "3"});
  true_block.terminator = LirBr{"tern.then.end.4"};
  function.blocks.push_back(std::move(true_block));

  LirBlock true_end_block;
  true_end_block.id = LirBlockId{2};
  true_end_block.label = "tern.then.end.4";
  true_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(true_end_block));

  LirBlock false_block;
  false_block.id = LirBlockId{3};
  false_block.label = "tern.else.5";
  false_block.insts.push_back(LirBinOp{"%t10", "add", "i32", "%p.y", "11"});
  false_block.insts.push_back(LirBinOp{"%t11", "sub", "i32", "%t10", "4"});
  false_block.insts.push_back(LirBinOp{"%t12", "add", "i32", "%t11", "7"});
  false_block.terminator = LirBr{"tern.else.end.6"};
  function.blocks.push_back(std::move(false_block));

  LirBlock false_end_block;
  false_end_block.id = LirBlockId{4};
  false_end_block.label = "tern.else.end.6";
  false_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(false_end_block));

  LirBlock join_block;
  join_block.id = LirBlockId{5};
  join_block.label = "tern.end.7";
  join_block.insts.push_back(
      LirPhiOp{"%t13", "i32", {{"%t9", "tern.then.end.4"}, {"%t12", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t14", "add", "i32", "%t13", "6"});
  join_block.terminator = LirRet{std::string("%t14"), "i32"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "choose2_mixed_then_deeper_post_chain";
  function.signature_text =
      "define i32 @choose2_mixed_then_deeper_post_chain(i32 %p.x, i32 %p.y)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "%p.x", "%p.y"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "%p.x", "8"});
  true_block.insts.push_back(LirBinOp{"%t9", "sub", "i32", "%t8", "3"});
  true_block.terminator = LirBr{"tern.then.end.4"};
  function.blocks.push_back(std::move(true_block));

  LirBlock true_end_block;
  true_end_block.id = LirBlockId{2};
  true_end_block.label = "tern.then.end.4";
  true_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(true_end_block));

  LirBlock false_block;
  false_block.id = LirBlockId{3};
  false_block.label = "tern.else.5";
  false_block.insts.push_back(LirBinOp{"%t10", "add", "i32", "%p.y", "11"});
  false_block.insts.push_back(LirBinOp{"%t11", "sub", "i32", "%t10", "4"});
  false_block.insts.push_back(LirBinOp{"%t12", "add", "i32", "%t11", "7"});
  false_block.terminator = LirBr{"tern.else.end.6"};
  function.blocks.push_back(std::move(false_block));

  LirBlock false_end_block;
  false_end_block.id = LirBlockId{4};
  false_end_block.label = "tern.else.end.6";
  false_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(false_end_block));

  LirBlock join_block;
  join_block.id = LirBlockId{5};
  join_block.label = "tern.end.7";
  join_block.insts.push_back(
      LirPhiOp{"%t13", "i32", {{"%t9", "tern.then.end.4"}, {"%t12", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t14", "add", "i32", "%t13", "6"});
  join_block.insts.push_back(LirBinOp{"%t15", "sub", "i32", "%t14", "2"});
  join_block.terminator = LirRet{std::string("%t15"), "i32"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_sub_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "choose2_mixed_then_deeper_post_chain_tail";
  function.signature_text =
      "define i32 @choose2_mixed_then_deeper_post_chain_tail(i32 %p.x, i32 %p.y)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "%p.x", "%p.y"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "%p.x", "8"});
  true_block.insts.push_back(LirBinOp{"%t9", "sub", "i32", "%t8", "3"});
  true_block.terminator = LirBr{"tern.then.end.4"};
  function.blocks.push_back(std::move(true_block));

  LirBlock true_end_block;
  true_end_block.id = LirBlockId{2};
  true_end_block.label = "tern.then.end.4";
  true_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(true_end_block));

  LirBlock false_block;
  false_block.id = LirBlockId{3};
  false_block.label = "tern.else.5";
  false_block.insts.push_back(LirBinOp{"%t10", "add", "i32", "%p.y", "11"});
  false_block.insts.push_back(LirBinOp{"%t11", "sub", "i32", "%t10", "4"});
  false_block.insts.push_back(LirBinOp{"%t12", "add", "i32", "%t11", "7"});
  false_block.terminator = LirBr{"tern.else.end.6"};
  function.blocks.push_back(std::move(false_block));

  LirBlock false_end_block;
  false_end_block.id = LirBlockId{4};
  false_end_block.label = "tern.else.end.6";
  false_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(false_end_block));

  LirBlock join_block;
  join_block.id = LirBlockId{5};
  join_block.label = "tern.end.7";
  join_block.insts.push_back(
      LirPhiOp{"%t13", "i32", {{"%t9", "tern.then.end.4"}, {"%t12", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t14", "add", "i32", "%t13", "6"});
  join_block.insts.push_back(LirBinOp{"%t15", "sub", "i32", "%t14", "2"});
  join_block.insts.push_back(LirBinOp{"%t16", "add", "i32", "%t15", "9"});
  join_block.terminator = LirRet{std::string("%t16"), "i32"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_select_eq_split_predecessor_deeper_affine_phi_post_join_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "choose2_deeper_both_post";
  function.signature_text =
      "define i32 @choose2_deeper_both_post(i32 %p.x, i32 %p.y)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "%p.x", "%p.y"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "%p.x", "8"});
  true_block.insts.push_back(LirBinOp{"%t9", "sub", "i32", "%t8", "3"});
  true_block.insts.push_back(LirBinOp{"%t10", "add", "i32", "%t9", "5"});
  true_block.terminator = LirBr{"tern.then.end.4"};
  function.blocks.push_back(std::move(true_block));

  LirBlock true_end_block;
  true_end_block.id = LirBlockId{2};
  true_end_block.label = "tern.then.end.4";
  true_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(true_end_block));

  LirBlock false_block;
  false_block.id = LirBlockId{3};
  false_block.label = "tern.else.5";
  false_block.insts.push_back(LirBinOp{"%t11", "add", "i32", "%p.y", "11"});
  false_block.insts.push_back(LirBinOp{"%t12", "sub", "i32", "%t11", "4"});
  false_block.insts.push_back(LirBinOp{"%t13", "add", "i32", "%t12", "7"});
  false_block.terminator = LirBr{"tern.else.end.6"};
  function.blocks.push_back(std::move(false_block));

  LirBlock false_end_block;
  false_end_block.id = LirBlockId{4};
  false_end_block.label = "tern.else.end.6";
  false_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(false_end_block));

  LirBlock join_block;
  join_block.id = LirBlockId{5};
  join_block.label = "tern.end.7";
  join_block.insts.push_back(
      LirPhiOp{"%t14", "i32", {{"%t10", "tern.then.end.4"}, {"%t13", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t15", "add", "i32", "%t14", "6"});
  join_block.terminator = LirRet{std::string("%t15"), "i32"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_select_eq_split_predecessor_deeper_affine_phi_post_join_add_sub_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "choose2_deeper_both_post_chain_tail";
  function.signature_text =
      "define i32 @choose2_deeper_both_post_chain_tail(i32 %p.x, i32 %p.y)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "%p.x", "%p.y"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "%p.x", "8"});
  true_block.insts.push_back(LirBinOp{"%t9", "sub", "i32", "%t8", "3"});
  true_block.insts.push_back(LirBinOp{"%t10", "add", "i32", "%t9", "5"});
  true_block.terminator = LirBr{"tern.then.end.4"};
  function.blocks.push_back(std::move(true_block));

  LirBlock true_end_block;
  true_end_block.id = LirBlockId{2};
  true_end_block.label = "tern.then.end.4";
  true_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(true_end_block));

  LirBlock false_block;
  false_block.id = LirBlockId{3};
  false_block.label = "tern.else.5";
  false_block.insts.push_back(LirBinOp{"%t11", "add", "i32", "%p.y", "11"});
  false_block.insts.push_back(LirBinOp{"%t12", "sub", "i32", "%t11", "4"});
  false_block.insts.push_back(LirBinOp{"%t13", "add", "i32", "%t12", "7"});
  false_block.terminator = LirBr{"tern.else.end.6"};
  function.blocks.push_back(std::move(false_block));

  LirBlock false_end_block;
  false_end_block.id = LirBlockId{4};
  false_end_block.label = "tern.else.end.6";
  false_end_block.terminator = LirBr{"tern.end.7"};
  function.blocks.push_back(std::move(false_end_block));

  LirBlock join_block;
  join_block.id = LirBlockId{5};
  join_block.label = "tern.end.7";
  join_block.insts.push_back(
      LirPhiOp{"%t14", "i32", {{"%t10", "tern.then.end.4"}, {"%t13", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t15", "add", "i32", "%t14", "6"});
  join_block.insts.push_back(LirBinOp{"%t16", "sub", "i32", "%t15", "2"});
  join_block.insts.push_back(LirBinOp{"%t17", "add", "i32", "%t16", "9"});
  join_block.terminator = LirRet{std::string("%t17"), "i32"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_mixed_predecessor_add_phi_post_join_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "choose_mixed_add";
  function.signature_text = "define i32 @choose_mixed_add()\n";
  function.return_type.base = c4c::TB_INT;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "slt", "i32", "2", "3"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t1", "0"});
  entry.terminator = LirCondBr{"%t2", "then", "else"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "then";
  true_block.insts.push_back(LirBinOp{"%t3", "add", "i32", "7", "5"});
  true_block.terminator = LirBr{"join"};
  function.blocks.push_back(std::move(true_block));

  LirBlock false_block;
  false_block.id = LirBlockId{2};
  false_block.label = "else";
  false_block.terminator = LirBr{"join"};
  function.blocks.push_back(std::move(false_block));

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "join";
  join_block.insts.push_back(LirPhiOp{"%t4", "i32", {{"%t3", "then"}, {"9", "else"}}});
  join_block.insts.push_back(LirBinOp{"%t5", "add", "i32", "%t4", "6"});
  join_block.terminator = LirRet{std::string("%t5"), "i32"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_single_param_add_sub_chain_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128";

  LirFunction function;
  function.name = "add_one";
  function.signature_text = "define i32 @add_one(i32 %p.x)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "2"});
  entry.insts.push_back(LirBinOp{"%t1", "sub", "i32", "%t0", "1"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_add_sub_chain_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "tiny_char";
  function.signature_text = "define i8 @tiny_char(i8 %p.x)\n";
  function.return_type.base = c4c::TB_CHAR;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_CHAR;
  function.params.push_back({"%p.x", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "add", "i8", "%p.x", "2"});
  entry.insts.push_back(LirBinOp{"%t1", "sub", "i8", "%t0", "1"});
  entry.terminator = LirRet{std::string("%t1"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i64_return_add_sub_chain_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p:64:64-i64:64-n32:64-S128";

  LirFunction function;
  function.name = "wide_add";
  function.signature_text = "define i64 @wide_add(i64 %p.x)\n";
  function.return_type.base = c4c::TB_LONGLONG;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_LONGLONG;
  function.params.push_back({"%p.x", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "add", "i64", "%p.x", "2"});
  entry.insts.push_back(LirBinOp{"%t1", "sub", "i64", "%t0", "1"});
  entry.terminator = LirRet{std::string("%t1"), "i64"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_two_param_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128";

  LirFunction function;
  function.name = "add_pair";
  function.signature_text = "define i32 @add_pair(i32 %p.x, i32 %p.y)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "%p.y"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_two_param_add_sub_chain_module() {
  using namespace c4c::codegen::lir;

  auto module = make_bir_two_param_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.push_back(LirBinOp{"%t1", "sub", "i32", "%t0", "1"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_two_param_staged_affine_module() {
  using namespace c4c::codegen::lir;

  auto module = make_bir_two_param_add_module();
  auto& entry = module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "%p.x", "2"});
  entry.insts.push_back(LirBinOp{"%t1", "add", "i32", "%t0", "%p.y"});
  entry.insts.push_back(LirBinOp{"%t2", "sub", "i32", "%t1", "1"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};
  return module;
}
