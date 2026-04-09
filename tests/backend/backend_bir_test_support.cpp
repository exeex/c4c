#include "backend_bir_test_support.hpp"

#include "backend_test_fixtures.hpp"

namespace {

c4c::codegen::lir::LirModule make_bir_fixture_module(
    const TestLirTargetProfile& profile = backend_test_riscv64_profile()) {
  c4c::codegen::lir::LirModule module;
  apply_test_lir_target_profile(module, profile);
  return module;
}

}  // namespace

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

c4c::codegen::lir::LirModule make_bir_return_sext_module() {
  using namespace c4c::codegen::lir;

  auto module = make_return_add_module();
  auto& function = module.functions.front();
  function.name = "widen_signed";
  function.signature_text = "define i64 @widen_signed(i32 %p.x)\n";
  function.return_type.base = c4c::TB_LONGLONG;
  function.params.clear();
  c4c::TypeSpec sext_param_type{};
  sext_param_type.base = c4c::TB_INT;
  function.params.push_back({"%p.x", sext_param_type});
  auto& entry = function.blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirCastOp{"%t0", LirCastKind::SExt, "i32", "%p.x", "i64"});
  entry.terminator = LirRet{std::string("%t0"), "i64"};
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_zext_module() {
  using namespace c4c::codegen::lir;

  LirModule module = make_bir_fixture_module();

  LirFunction function;
  function.name = "widen_unsigned";
  function.signature_text = "define i32 @widen_unsigned(i8 %p.x)\n";
  function.return_type.base = c4c::TB_INT;
  c4c::TypeSpec zext_param_type{};
  zext_param_type.base = c4c::TB_UCHAR;
  function.params.push_back({"%p.x", zext_param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirCastOp{"%t0", LirCastKind::ZExt, LirTypeRef::integer(8), "%p.x", LirTypeRef::integer(32)});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_return_trunc_module() {
  using namespace c4c::codegen::lir;

  auto module = make_bir_return_sext_module();
  auto& function = module.functions.front();
  function.name = "narrow_signed";
  function.signature_text = "define i32 @narrow_signed(i64 %p.x)\n";
  function.return_type.base = c4c::TB_INT;
  function.params.clear();
  c4c::TypeSpec trunc_param_type{};
  trunc_param_type.base = c4c::TB_LONGLONG;
  function.params.push_back({"%p.x", trunc_param_type});
  auto& entry = function.blocks.front();
  entry.insts.clear();
  entry.insts.push_back(LirCastOp{"%t0", LirCastKind::Trunc, "i64", "%p.x", "i32"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
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

c4c::codegen::lir::LirModule make_bir_i8_return_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_add_u";
  function.signature_text = "define i8 @choose_add_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "add", "i32", "2", "3"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::Trunc, "i32", "%t0", "i8"});
  entry.terminator = LirRet{std::string("%t1"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_sub_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_sub_u";
  function.signature_text = "define i8 @choose_sub_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "sub", "i32", "9", "4"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::Trunc, "i32", "%t0", "i8"});
  entry.terminator = LirRet{std::string("%t1"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_mul_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_mul_u";
  function.signature_text = "define i8 @choose_mul_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "mul", "i32", "6", "7"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::Trunc, "i32", "%t0", "i8"});
  entry.terminator = LirRet{std::string("%t1"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_and_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_and_u";
  function.signature_text = "define i8 @choose_and_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "and", "i32", "14", "11"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::Trunc, "i32", "%t0", "i8"});
  entry.terminator = LirRet{std::string("%t1"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_or_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_or_u";
  function.signature_text = "define i8 @choose_or_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "or", "i32", "12", "3"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::Trunc, "i32", "%t0", "i8"});
  entry.terminator = LirRet{std::string("%t1"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_xor_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_xor_u";
  function.signature_text = "define i8 @choose_xor_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "xor", "i32", "12", "10"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::Trunc, "i32", "%t0", "i8"});
  entry.terminator = LirRet{std::string("%t1"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_shl_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_shl_u";
  function.signature_text = "define i8 @choose_shl_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "shl", "i32", "3", "4"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::Trunc, "i32", "%t0", "i8"});
  entry.terminator = LirRet{std::string("%t1"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_lshr_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_lshr_u";
  function.signature_text = "define i8 @choose_lshr_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "lshr", "i32", "16", "2"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::Trunc, "i32", "%t0", "i8"});
  entry.terminator = LirRet{std::string("%t1"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_ashr_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_ashr_u";
  function.signature_text = "define i8 @choose_ashr_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "ashr", "i32", "-16", "2"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::Trunc, "i32", "%t0", "i8"});
  entry.terminator = LirRet{std::string("%t1"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_sdiv_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_sdiv_u";
  function.signature_text = "define i8 @choose_sdiv_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "sdiv", "i32", "12", "3"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::Trunc, "i32", "%t0", "i8"});
  entry.terminator = LirRet{std::string("%t1"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_udiv_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_udiv_u";
  function.signature_text = "define i8 @choose_udiv_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "udiv", "i32", "12", "3"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::Trunc, "i32", "%t0", "i8"});
  entry.terminator = LirRet{std::string("%t1"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_srem_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_srem_u";
  function.signature_text = "define i8 @choose_srem_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "srem", "i32", "14", "5"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::Trunc, "i32", "%t0", "i8"});
  entry.terminator = LirRet{std::string("%t1"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_urem_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_urem_u";
  function.signature_text = "define i8 @choose_urem_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "urem", "i32", "14", "5"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::Trunc, "i32", "%t0", "i8"});
  entry.terminator = LirRet{std::string("%t1"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_eq_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_eq_u";
  function.signature_text = "define i8 @choose_eq_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "eq", "i32", "7", "7"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::Trunc, "i32", "%t1", "i8"});
  entry.terminator = LirRet{std::string("%t2"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_ne_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_ne_u";
  function.signature_text = "define i8 @choose_ne_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "ne", "i32", "7", "3"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::Trunc, "i32", "%t1", "i8"});
  entry.terminator = LirRet{std::string("%t2"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_ult_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_ult_u";
  function.signature_text = "define i8 @choose_ult_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "ult", "i32", "3", "7"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::Trunc, "i32", "%t1", "i8"});
  entry.terminator = LirRet{std::string("%t2"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_ule_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_ule_u";
  function.signature_text = "define i8 @choose_ule_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "ule", "i32", "7", "7"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::Trunc, "i32", "%t1", "i8"});
  entry.terminator = LirRet{std::string("%t2"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_ugt_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_ugt_u";
  function.signature_text = "define i8 @choose_ugt_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "ugt", "i32", "7", "3"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::Trunc, "i32", "%t1", "i8"});
  entry.terminator = LirRet{std::string("%t2"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_uge_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_uge_u";
  function.signature_text = "define i8 @choose_uge_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{"%t0", false, "uge", "i32", "7", "7"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i1", "%t0", "i32"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::Trunc, "i32", "%t1", "i8"});
  entry.terminator = LirRet{std::string("%t2"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_slt_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_slt_u";
  function.signature_text = "define i8 @choose_slt_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCastOp{"%t0", LirCastKind::Trunc, "i32", "3", "i8"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::Trunc, "i32", "7", "i8"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::SExt, "i8", "%t0", "i32"});
  entry.insts.push_back(LirCastOp{"%t3", LirCastKind::SExt, "i8", "%t1", "i32"});
  entry.insts.push_back(LirCmpOp{"%t4", false, "slt", "i32", "%t2", "%t3"});
  entry.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i1", "%t4", "i32"});
  entry.insts.push_back(LirCastOp{"%t6", LirCastKind::Trunc, "i32", "%t5", "i8"});
  entry.terminator = LirRet{std::string("%t6"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_sle_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_sle_u";
  function.signature_text = "define i8 @choose_sle_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCastOp{"%t0", LirCastKind::Trunc, "i32", "7", "i8"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::Trunc, "i32", "7", "i8"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::SExt, "i8", "%t0", "i32"});
  entry.insts.push_back(LirCastOp{"%t3", LirCastKind::SExt, "i8", "%t1", "i32"});
  entry.insts.push_back(LirCmpOp{"%t4", false, "sle", "i32", "%t2", "%t3"});
  entry.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i1", "%t4", "i32"});
  entry.insts.push_back(LirCastOp{"%t6", LirCastKind::Trunc, "i32", "%t5", "i8"});
  entry.terminator = LirRet{std::string("%t6"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_sgt_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_sgt_u";
  function.signature_text = "define i8 @choose_sgt_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCastOp{"%t0", LirCastKind::Trunc, "i32", "7", "i8"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::Trunc, "i32", "3", "i8"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::SExt, "i8", "%t0", "i32"});
  entry.insts.push_back(LirCastOp{"%t3", LirCastKind::SExt, "i8", "%t1", "i32"});
  entry.insts.push_back(LirCmpOp{"%t4", false, "sgt", "i32", "%t2", "%t3"});
  entry.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i1", "%t4", "i32"});
  entry.insts.push_back(LirCastOp{"%t6", LirCastKind::Trunc, "i32", "%t5", "i8"});
  entry.terminator = LirRet{std::string("%t6"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_sge_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_sge_u";
  function.signature_text = "define i8 @choose_sge_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCastOp{"%t0", LirCastKind::Trunc, "i32", "7", "i8"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::Trunc, "i32", "7", "i8"});
  entry.insts.push_back(LirCastOp{"%t2", LirCastKind::SExt, "i8", "%t0", "i32"});
  entry.insts.push_back(LirCastOp{"%t3", LirCastKind::SExt, "i8", "%t1", "i32"});
  entry.insts.push_back(LirCmpOp{"%t4", false, "sge", "i32", "%t2", "%t3"});
  entry.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i1", "%t4", "i32"});
  entry.insts.push_back(LirCastOp{"%t6", LirCastKind::Trunc, "i32", "%t5", "i8"});
  entry.terminator = LirRet{std::string("%t6"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_i8_return_immediate_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose_const_u";
  function.signature_text = "define i8 @choose_const_u()\n";
  function.return_type.base = c4c::TB_UCHAR;
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCastOp{"%t0", LirCastKind::Trunc, "i32", "11", "i8"});
  entry.terminator = LirRet{std::string("%t0"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_single_param_select_eq_branch_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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

c4c::codegen::lir::LirModule make_bir_two_param_select_ne_branch_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose2_ne_branch_u";
  function.signature_text = "define i8 @choose2_ne_branch_u(i8 %p.x, i8 %p.y)\n";
  function.return_type.base = c4c::TB_UCHAR;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_UCHAR;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirCastOp{"%t0", LirCastKind::ZExt, LirTypeRef::integer(8), "%p.x", LirTypeRef::integer(32)});
  entry.insts.push_back(
      LirCastOp{"%t1", LirCastKind::ZExt, LirTypeRef::integer(8), "%p.y", LirTypeRef::integer(32)});
  entry.insts.push_back(
      LirCmpOp{"%t2", false, "ne", LirTypeRef::integer(32), "%t0", "%t1"});
  entry.insts.push_back(LirCastOp{"%t3", LirCastKind::ZExt, "i1", "%t2", LirTypeRef::integer(32)});
  entry.insts.push_back(
      LirCmpOp{"%t4", false, "ne", LirTypeRef::integer(32), "%t3", "0"});
  entry.terminator = LirCondBr{"%t4", "tern.then.5", "tern.else.7"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.5";
  true_block.terminator = LirRet{std::string("%p.x"), LirTypeRef::integer(8)};
  function.blocks.push_back(std::move(true_block));

  LirBlock false_block;
  false_block.id = LirBlockId{2};
  false_block.label = "tern.else.7";
  false_block.terminator = LirRet{std::string("%p.y"), LirTypeRef::integer(8)};
  function.blocks.push_back(std::move(false_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_two_param_select_ne_phi_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose2_ne_u";
  function.signature_text = "define i8 @choose2_ne_u(i8 %p.x, i8 %p.y)\n";
  function.return_type.base = c4c::TB_UCHAR;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_UCHAR;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(
      LirCastOp{"%t0", LirCastKind::ZExt, LirTypeRef::integer(8), "%p.x", LirTypeRef::integer(32)});
  entry.insts.push_back(
      LirCastOp{"%t1", LirCastKind::ZExt, LirTypeRef::integer(8), "%p.y", LirTypeRef::integer(32)});
  entry.insts.push_back(
      LirCmpOp{"%t2", false, "ne", LirTypeRef::integer(32), "%t0", "%t1"});
  entry.insts.push_back(LirCastOp{"%t3", LirCastKind::ZExt, "i1", "%t2", LirTypeRef::integer(32)});
  entry.insts.push_back(
      LirCmpOp{"%t4", false, "ne", LirTypeRef::integer(32), "%t3", "0"});
  entry.terminator = LirCondBr{"%t4", "tern.then.5", "tern.else.7"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.5";
  true_block.terminator = LirBr{"tern.then.end.6"};
  function.blocks.push_back(std::move(true_block));

  LirBlock true_end_block;
  true_end_block.id = LirBlockId{2};
  true_end_block.label = "tern.then.end.6";
  true_end_block.terminator = LirBr{"tern.end.9"};
  function.blocks.push_back(std::move(true_end_block));

  LirBlock false_block;
  false_block.id = LirBlockId{3};
  false_block.label = "tern.else.7";
  false_block.terminator = LirBr{"tern.else.end.8"};
  function.blocks.push_back(std::move(false_block));

  LirBlock false_end_block;
  false_end_block.id = LirBlockId{4};
  false_end_block.label = "tern.else.end.8";
  false_end_block.terminator = LirBr{"tern.end.9"};
  function.blocks.push_back(std::move(false_end_block));

  LirBlock join_block;
  join_block.id = LirBlockId{5};
  join_block.label = "tern.end.9";
  join_block.insts.push_back(
      LirPhiOp{"%t5", LirTypeRef::integer(8), {{"%p.x", "tern.then.end.6"}, {"%p.y", "tern.else.end.8"}}});
  join_block.terminator = LirRet{std::string("%t5"), LirTypeRef::integer(8)};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_u8_select_ne_predecessor_add_phi_post_join_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose2_add_post_ne_u";
  function.signature_text =
      "define i8 @choose2_add_post_ne_u(i8 %p.x, i8 %p.y)\n";
  function.return_type.base = c4c::TB_UCHAR;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_UCHAR;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCastOp{"%t0", LirCastKind::ZExt, "i8", "%p.x", "i32"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i8", "%p.y", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t0", "%t1"});
  entry.insts.push_back(LirCastOp{"%t3", LirCastKind::ZExt, "i1", "%t2", "i32"});
  entry.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  entry.terminator = LirCondBr{"%t4", "tern.then", "tern.else"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then";
  true_block.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i8", "%p.x", "i32"});
  true_block.insts.push_back(LirBinOp{"%t6", "add", "i32", "%t5", "5"});
  true_block.insts.push_back(LirCastOp{"%t7", LirCastKind::Trunc, "i32", "%t6", "i8"});
  true_block.terminator = LirBr{"tern.end"};
  function.blocks.push_back(std::move(true_block));

  LirBlock false_block;
  false_block.id = LirBlockId{2};
  false_block.label = "tern.else";
  false_block.insts.push_back(LirCastOp{"%t8", LirCastKind::ZExt, "i8", "%p.y", "i32"});
  false_block.insts.push_back(LirBinOp{"%t9", "add", "i32", "%t8", "9"});
  false_block.insts.push_back(LirCastOp{"%t10", LirCastKind::Trunc, "i32", "%t9", "i8"});
  false_block.terminator = LirBr{"tern.end"};
  function.blocks.push_back(std::move(false_block));

  LirBlock join_block;
  join_block.id = LirBlockId{3};
  join_block.label = "tern.end";
  join_block.insts.push_back(
      LirPhiOp{"%t11", "i32", {{"%t7", "tern.then"}, {"%t10", "tern.else"}}});
  join_block.insts.push_back(LirBinOp{"%t12", "add", "i32", "%t11", "6"});
  join_block.insts.push_back(LirCastOp{"%t13", LirCastKind::Trunc, "i32", "%t12", "i8"});
  join_block.terminator = LirRet{std::string("%t13"), "i8"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_u8_select_ne_split_predecessor_add_phi_post_join_add_sub_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose2_add_post_chain_ne_u";
  function.signature_text =
      "define i8 @choose2_add_post_chain_ne_u(i8 %p.x, i8 %p.y)\n";
  function.return_type.base = c4c::TB_UCHAR;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_UCHAR;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCastOp{"%t0", LirCastKind::ZExt, "i8", "%p.x", "i32"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i8", "%p.y", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t0", "%t1"});
  entry.insts.push_back(LirCastOp{"%t3", LirCastKind::ZExt, "i1", "%t2", "i32"});
  entry.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  entry.terminator = LirCondBr{"%t4", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i8", "%p.x", "i32"});
  true_block.insts.push_back(LirBinOp{"%t6", "add", "i32", "%t5", "5"});
  true_block.insts.push_back(LirCastOp{"%t7", LirCastKind::Trunc, "i32", "%t6", "i8"});
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
  false_block.insts.push_back(LirCastOp{"%t8", LirCastKind::ZExt, "i8", "%p.y", "i32"});
  false_block.insts.push_back(LirBinOp{"%t9", "add", "i32", "%t8", "9"});
  false_block.insts.push_back(LirCastOp{"%t10", LirCastKind::Trunc, "i32", "%t9", "i8"});
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
      LirPhiOp{"%t11", "i32", {{"%t7", "tern.then.end.4"}, {"%t10", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t12", "add", "i32", "%t11", "6"});
  join_block.insts.push_back(LirBinOp{"%t13", "sub", "i32", "%t12", "2"});
  join_block.insts.push_back(LirCastOp{"%t14", LirCastKind::Trunc, "i32", "%t13", "i8"});
  join_block.terminator = LirRet{std::string("%t14"), "i8"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_u8_select_ne_split_predecessor_add_phi_post_join_add_sub_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose2_add_post_chain_tail_ne_u";
  function.signature_text =
      "define i8 @choose2_add_post_chain_tail_ne_u(i8 %p.x, i8 %p.y)\n";
  function.return_type.base = c4c::TB_UCHAR;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_UCHAR;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCastOp{"%t0", LirCastKind::ZExt, "i8", "%p.x", "i32"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i8", "%p.y", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t0", "%t1"});
  entry.insts.push_back(LirCastOp{"%t3", LirCastKind::ZExt, "i1", "%t2", "i32"});
  entry.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  entry.terminator = LirCondBr{"%t4", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i8", "%p.x", "i32"});
  true_block.insts.push_back(LirBinOp{"%t6", "add", "i32", "%t5", "5"});
  true_block.insts.push_back(LirCastOp{"%t7", LirCastKind::Trunc, "i32", "%t6", "i8"});
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
  false_block.insts.push_back(LirCastOp{"%t8", LirCastKind::ZExt, "i8", "%p.y", "i32"});
  false_block.insts.push_back(LirBinOp{"%t9", "add", "i32", "%t8", "9"});
  false_block.insts.push_back(LirCastOp{"%t10", LirCastKind::Trunc, "i32", "%t9", "i8"});
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
      LirPhiOp{"%t11", "i32", {{"%t7", "tern.then.end.4"}, {"%t10", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t12", "add", "i32", "%t11", "6"});
  join_block.insts.push_back(LirBinOp{"%t13", "sub", "i32", "%t12", "2"});
  join_block.insts.push_back(LirBinOp{"%t14", "add", "i32", "%t13", "9"});
  join_block.insts.push_back(LirCastOp{"%t15", LirCastKind::Trunc, "i32", "%t14", "i8"});
  join_block.terminator = LirRet{std::string("%t15"), "i8"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_u8_select_ne_split_predecessor_mixed_affine_phi_post_join_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose2_mixed_post_ne_u";
  function.signature_text =
      "define i8 @choose2_mixed_post_ne_u(i8 %p.x, i8 %p.y)\n";
  function.return_type.base = c4c::TB_UCHAR;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_UCHAR;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCastOp{"%t0", LirCastKind::ZExt, "i8", "%p.x", "i32"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i8", "%p.y", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t0", "%t1"});
  entry.insts.push_back(LirCastOp{"%t3", LirCastKind::ZExt, "i1", "%t2", "i32"});
  entry.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  entry.terminator = LirCondBr{"%t4", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i8", "%p.x", "i32"});
  true_block.insts.push_back(LirBinOp{"%t6", "add", "i32", "%t5", "8"});
  true_block.insts.push_back(LirBinOp{"%t7", "sub", "i32", "%t6", "3"});
  true_block.insts.push_back(LirCastOp{"%t8", LirCastKind::Trunc, "i32", "%t7", "i8"});
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
  false_block.insts.push_back(LirCastOp{"%t9", LirCastKind::ZExt, "i8", "%p.y", "i32"});
  false_block.insts.push_back(LirBinOp{"%t10", "add", "i32", "%t9", "11"});
  false_block.insts.push_back(LirBinOp{"%t11", "sub", "i32", "%t10", "4"});
  false_block.insts.push_back(LirCastOp{"%t12", LirCastKind::Trunc, "i32", "%t11", "i8"});
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
      LirPhiOp{"%t13", "i32", {{"%t8", "tern.then.end.4"}, {"%t12", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t14", "add", "i32", "%t13", "6"});
  join_block.insts.push_back(LirCastOp{"%t15", LirCastKind::Trunc, "i32", "%t14", "i8"});
  join_block.terminator = LirRet{std::string("%t15"), "i8"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_u8_select_ne_split_predecessor_mixed_affine_phi_post_join_add_sub_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose2_mixed_post_chain_ne_u";
  function.signature_text =
      "define i8 @choose2_mixed_post_chain_ne_u(i8 %p.x, i8 %p.y)\n";
  function.return_type.base = c4c::TB_UCHAR;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_UCHAR;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCastOp{"%t0", LirCastKind::ZExt, "i8", "%p.x", "i32"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i8", "%p.y", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t0", "%t1"});
  entry.insts.push_back(LirCastOp{"%t3", LirCastKind::ZExt, "i1", "%t2", "i32"});
  entry.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  entry.terminator = LirCondBr{"%t4", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i8", "%p.x", "i32"});
  true_block.insts.push_back(LirBinOp{"%t6", "add", "i32", "%t5", "8"});
  true_block.insts.push_back(LirBinOp{"%t7", "sub", "i32", "%t6", "3"});
  true_block.insts.push_back(LirCastOp{"%t8", LirCastKind::Trunc, "i32", "%t7", "i8"});
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
  false_block.insts.push_back(LirCastOp{"%t9", LirCastKind::ZExt, "i8", "%p.y", "i32"});
  false_block.insts.push_back(LirBinOp{"%t10", "add", "i32", "%t9", "11"});
  false_block.insts.push_back(LirBinOp{"%t11", "sub", "i32", "%t10", "4"});
  false_block.insts.push_back(LirCastOp{"%t12", LirCastKind::Trunc, "i32", "%t11", "i8"});
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
      LirPhiOp{"%t13", "i32", {{"%t8", "tern.then.end.4"}, {"%t12", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t14", "add", "i32", "%t13", "6"});
  join_block.insts.push_back(LirBinOp{"%t15", "sub", "i32", "%t14", "2"});
  join_block.insts.push_back(LirCastOp{"%t16", LirCastKind::Trunc, "i32", "%t15", "i8"});
  join_block.terminator = LirRet{std::string("%t16"), "i8"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_u8_select_eq_split_predecessor_mixed_affine_phi_post_join_add_sub_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose2_mixed_post_chain_tail_u";
  function.signature_text =
      "define i8 @choose2_mixed_post_chain_tail_u(i8 %p.x, i8 %p.y)\n";
  function.return_type.base = c4c::TB_UCHAR;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_UCHAR;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCastOp{"%t0", LirCastKind::ZExt, "i8", "%p.x", "i32"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i8", "%p.y", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "eq", "i32", "%t0", "%t1"});
  entry.insts.push_back(LirCastOp{"%t3", LirCastKind::ZExt, "i1", "%t2", "i32"});
  entry.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  entry.terminator = LirCondBr{"%t4", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i8", "%p.x", "i32"});
  true_block.insts.push_back(LirBinOp{"%t6", "add", "i32", "%t5", "8"});
  true_block.insts.push_back(LirBinOp{"%t7", "sub", "i32", "%t6", "3"});
  true_block.insts.push_back(LirCastOp{"%t8", LirCastKind::Trunc, "i32", "%t7", "i8"});
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
  false_block.insts.push_back(LirCastOp{"%t9", LirCastKind::ZExt, "i8", "%p.y", "i32"});
  false_block.insts.push_back(LirBinOp{"%t10", "add", "i32", "%t9", "11"});
  false_block.insts.push_back(LirBinOp{"%t11", "sub", "i32", "%t10", "4"});
  false_block.insts.push_back(LirCastOp{"%t12", LirCastKind::Trunc, "i32", "%t11", "i8"});
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
      LirPhiOp{"%t13", "i32", {{"%t8", "tern.then.end.4"}, {"%t12", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t14", "add", "i32", "%t13", "6"});
  join_block.insts.push_back(LirBinOp{"%t15", "sub", "i32", "%t14", "2"});
  join_block.insts.push_back(LirBinOp{"%t16", "add", "i32", "%t15", "9"});
  join_block.insts.push_back(LirCastOp{"%t17", LirCastKind::Trunc, "i32", "%t16", "i8"});
  join_block.terminator = LirRet{std::string("%t17"), "i8"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_phi_post_join_add_sub_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose2_deeper_post_chain_tail_u";
  function.signature_text =
      "define i8 @choose2_deeper_post_chain_tail_u(i8 %p.x, i8 %p.y)\n";
  function.return_type.base = c4c::TB_UCHAR;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_UCHAR;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCastOp{"%t0", LirCastKind::ZExt, "i8", "%p.x", "i32"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i8", "%p.y", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "eq", "i32", "%t0", "%t1"});
  entry.insts.push_back(LirCastOp{"%t3", LirCastKind::ZExt, "i1", "%t2", "i32"});
  entry.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  entry.terminator = LirCondBr{"%t4", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i8", "%p.x", "i32"});
  true_block.insts.push_back(LirBinOp{"%t6", "add", "i32", "%t5", "8"});
  true_block.insts.push_back(LirBinOp{"%t7", "sub", "i32", "%t6", "3"});
  true_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "%t7", "5"});
  true_block.insts.push_back(LirCastOp{"%t9", LirCastKind::Trunc, "i32", "%t8", "i8"});
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
  false_block.insts.push_back(LirCastOp{"%t10", LirCastKind::ZExt, "i8", "%p.y", "i32"});
  false_block.insts.push_back(LirBinOp{"%t11", "add", "i32", "%t10", "11"});
  false_block.insts.push_back(LirBinOp{"%t12", "sub", "i32", "%t11", "4"});
  false_block.insts.push_back(LirCastOp{"%t13", LirCastKind::Trunc, "i32", "%t12", "i8"});
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
      LirPhiOp{"%t14", "i32", {{"%t9", "tern.then.end.4"}, {"%t13", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t15", "add", "i32", "%t14", "6"});
  join_block.insts.push_back(LirBinOp{"%t16", "sub", "i32", "%t15", "2"});
  join_block.insts.push_back(LirBinOp{"%t17", "add", "i32", "%t16", "9"});
  join_block.insts.push_back(LirCastOp{"%t18", LirCastKind::Trunc, "i32", "%t17", "i8"});
  join_block.terminator = LirRet{std::string("%t18"), "i8"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_u8_select_eq_split_predecessor_deeper_affine_phi_post_join_add_sub_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose2_deeper_both_post_chain_tail_u";
  function.signature_text =
      "define i8 @choose2_deeper_both_post_chain_tail_u(i8 %p.x, i8 %p.y)\n";
  function.return_type.base = c4c::TB_UCHAR;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_UCHAR;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCastOp{"%t0", LirCastKind::ZExt, "i8", "%p.x", "i32"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i8", "%p.y", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "eq", "i32", "%t0", "%t1"});
  entry.insts.push_back(LirCastOp{"%t3", LirCastKind::ZExt, "i1", "%t2", "i32"});
  entry.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  entry.terminator = LirCondBr{"%t4", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i8", "%p.x", "i32"});
  true_block.insts.push_back(LirBinOp{"%t6", "add", "i32", "%t5", "8"});
  true_block.insts.push_back(LirBinOp{"%t7", "sub", "i32", "%t6", "3"});
  true_block.insts.push_back(LirBinOp{"%t8", "add", "i32", "%t7", "5"});
  true_block.insts.push_back(LirCastOp{"%t9", LirCastKind::Trunc, "i32", "%t8", "i8"});
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
  false_block.insts.push_back(LirCastOp{"%t10", LirCastKind::ZExt, "i8", "%p.y", "i32"});
  false_block.insts.push_back(LirBinOp{"%t11", "add", "i32", "%t10", "11"});
  false_block.insts.push_back(LirBinOp{"%t12", "sub", "i32", "%t11", "4"});
  false_block.insts.push_back(LirBinOp{"%t13", "add", "i32", "%t12", "7"});
  false_block.insts.push_back(LirCastOp{"%t14", LirCastKind::Trunc, "i32", "%t13", "i8"});
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
      LirPhiOp{"%t15", "i32", {{"%t9", "tern.then.end.4"}, {"%t14", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t16", "add", "i32", "%t15", "6"});
  join_block.insts.push_back(LirBinOp{"%t17", "sub", "i32", "%t16", "2"});
  join_block.insts.push_back(LirBinOp{"%t18", "add", "i32", "%t17", "9"});
  join_block.insts.push_back(LirCastOp{"%t19", LirCastKind::Trunc, "i32", "%t18", "i8"});
  join_block.terminator = LirRet{std::string("%t19"), "i8"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule
make_bir_two_param_u8_select_ne_split_predecessor_mixed_then_deeper_affine_phi_post_join_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "choose2_mixed_then_deeper_post_ne_u";
  function.signature_text =
      "define i8 @choose2_mixed_then_deeper_post_ne_u(i8 %p.x, i8 %p.y)\n";
  function.return_type.base = c4c::TB_UCHAR;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_UCHAR;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirCastOp{"%t0", LirCastKind::ZExt, "i8", "%p.x", "i32"});
  entry.insts.push_back(LirCastOp{"%t1", LirCastKind::ZExt, "i8", "%p.y", "i32"});
  entry.insts.push_back(LirCmpOp{"%t2", false, "ne", "i32", "%t0", "%t1"});
  entry.insts.push_back(LirCastOp{"%t3", LirCastKind::ZExt, "i1", "%t2", "i32"});
  entry.insts.push_back(LirCmpOp{"%t4", false, "ne", "i32", "%t3", "0"});
  entry.terminator = LirCondBr{"%t4", "tern.then.3", "tern.else.5"};
  function.blocks.push_back(std::move(entry));

  LirBlock true_block;
  true_block.id = LirBlockId{1};
  true_block.label = "tern.then.3";
  true_block.insts.push_back(LirCastOp{"%t5", LirCastKind::ZExt, "i8", "%p.x", "i32"});
  true_block.insts.push_back(LirBinOp{"%t6", "add", "i32", "%t5", "8"});
  true_block.insts.push_back(LirBinOp{"%t7", "sub", "i32", "%t6", "3"});
  true_block.insts.push_back(LirCastOp{"%t8", LirCastKind::Trunc, "i32", "%t7", "i8"});
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
  false_block.insts.push_back(LirCastOp{"%t9", LirCastKind::ZExt, "i8", "%p.y", "i32"});
  false_block.insts.push_back(LirBinOp{"%t10", "add", "i32", "%t9", "11"});
  false_block.insts.push_back(LirBinOp{"%t11", "sub", "i32", "%t10", "4"});
  false_block.insts.push_back(LirBinOp{"%t12", "add", "i32", "%t11", "7"});
  false_block.insts.push_back(LirCastOp{"%t13", LirCastKind::Trunc, "i32", "%t12", "i8"});
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
      LirPhiOp{"%t14", "i32", {{"%t8", "tern.then.end.4"}, {"%t13", "tern.else.end.6"}}});
  join_block.insts.push_back(LirBinOp{"%t15", "add", "i32", "%t14", "6"});
  join_block.insts.push_back(LirCastOp{"%t16", LirCastKind::Trunc, "i32", "%t15", "i8"});
  join_block.terminator = LirRet{std::string("%t16"), "i8"};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_two_param_select_eq_predecessor_add_phi_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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

  auto module = make_bir_fixture_module(backend_test_x86_64_bir_pipeline_profile());

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
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

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

c4c::codegen::lir::LirModule make_bir_i8_two_param_add_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  apply_test_lir_target_profile(module, backend_test_riscv64_profile());

  LirFunction function;
  function.name = "add_pair_u";
  function.signature_text = "define i8 @add_pair_u(i8 %p.x, i8 %p.y)\n";
  function.return_type.base = c4c::TB_UCHAR;
  c4c::TypeSpec param_type{};
  param_type.base = c4c::TB_UCHAR;
  function.params.push_back({"%p.x", param_type});
  function.params.push_back({"%p.y", param_type});
  function.entry = LirBlockId{0};

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{"%t0", "add", "i8", "%p.x", "%p.y"});
  entry.terminator = LirRet{std::string("%t0"), "i8"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_bir_two_param_add_module() {
  using namespace c4c::codegen::lir;

  auto module = make_bir_fixture_module(backend_test_x86_64_bir_pipeline_profile());

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
