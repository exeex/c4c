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
