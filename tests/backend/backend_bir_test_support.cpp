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
