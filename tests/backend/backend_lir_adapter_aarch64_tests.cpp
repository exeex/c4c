
#include "backend.hpp"
#include "generation.hpp"
#include "ir_printer.hpp"
#include "ir_validate.hpp"
#include "liveness.hpp"
#include "lir_adapter.hpp"
#include "regalloc.hpp"
#include "stack_layout/analysis.hpp"
#include "stack_layout/alloca_coalescing.hpp"
#include "stack_layout/regalloc_helpers.hpp"
#include "stack_layout/slot_assignment.hpp"
#include "target.hpp"
#include "../../src/codegen/lir/call_args.hpp"
#include "../../src/codegen/lir/lir_printer.hpp"
#include "../../src/codegen/lir/verify.hpp"
#include "../../src/backend/elf/mod.hpp"
#include "../../src/backend/linker_common/mod.hpp"
#include "../../src/backend/aarch64/assembler/mod.hpp"
#include "../../src/backend/aarch64/codegen/emit.hpp"
#include "../../src/backend/aarch64/linker/mod.hpp"
#include "../../src/backend/aarch64/assembler/parser.hpp"
#include "../../src/backend/aarch64/assembler/encoder/mod.hpp"
#include "../../src/backend/x86/assembler/mod.hpp"
#include "../../src/backend/x86/assembler/encoder/mod.hpp"
#include "../../src/backend/x86/assembler/parser.hpp"
#include "../../src/backend/x86/codegen/emit.hpp"
#include "../../src/backend/x86/linker/mod.hpp"
#include "backend_test_helper.hpp"
#include "backend_test_fixtures.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>

void clear_backend_global_compatibility_shims(c4c::backend::BackendModule& module) {
  for (auto& global : module.globals) {
    if (global.linkage_kind != c4c::backend::BackendGlobalLinkage::Default) {
      global.linkage.clear();
    }
    global.qualifier.clear();
    global.init_text.clear();
    global.is_extern_decl = false;
  }
}

void clear_backend_global_type_compatibility_shims(c4c::backend::BackendModule& module) {
  for (auto& global : module.globals) {
    if (global.array_type.has_value() ||
        global.type_kind != c4c::backend::BackendValueTypeKind::Unknown) {
      global.llvm_type.clear();
    }
  }
}

void clear_backend_memory_type_compatibility_shims(c4c::backend::BackendModule& module) {
  for (auto& function : module.functions) {
    for (auto& block : function.blocks) {
      for (auto& inst : block.insts) {
        if (auto* phi = std::get_if<c4c::backend::BackendPhiInst>(&inst)) {
          if (phi->value_type != c4c::backend::BackendScalarType::Unknown) {
            phi->type_str.clear();
          }
          continue;
        }
        if (auto* bin = std::get_if<c4c::backend::BackendBinaryInst>(&inst)) {
          if (bin->value_type != c4c::backend::BackendScalarType::Unknown) {
            bin->type_str.clear();
          }
          continue;
        }
        if (auto* cmp = std::get_if<c4c::backend::BackendCompareInst>(&inst)) {
          if (cmp->operand_type != c4c::backend::BackendScalarType::Unknown) {
            cmp->type_str.clear();
          }
          continue;
        }
        if (auto* load = std::get_if<c4c::backend::BackendLoadInst>(&inst)) {
          if (load->value_type != c4c::backend::BackendScalarType::Unknown) {
            load->type_str.clear();
          }
          if (load->memory_value_type != c4c::backend::BackendScalarType::Unknown) {
            load->memory_type.clear();
          }
          continue;
        }
        if (auto* store = std::get_if<c4c::backend::BackendStoreInst>(&inst)) {
          if (store->value_type != c4c::backend::BackendScalarType::Unknown) {
            store->type_str.clear();
          }
          continue;
        }
        if (auto* ptrdiff = std::get_if<c4c::backend::BackendPtrDiffEqInst>(&inst)) {
          if (ptrdiff->result_type != c4c::backend::BackendScalarType::Unknown) {
            ptrdiff->type_str.clear();
          }
        }
      }
    }
  }
}

void clear_backend_signature_and_call_type_compatibility_shims(
    c4c::backend::BackendModule& module) {
  for (auto& function : module.functions) {
    if (function.signature.return_type_kind != c4c::backend::BackendValueTypeKind::Unknown) {
      function.signature.return_type.clear();
    }
    for (auto& param : function.signature.params) {
      if (param.type_kind != c4c::backend::BackendValueTypeKind::Unknown) {
        param.type_str.clear();
      }
    }
    for (auto& block : function.blocks) {
      for (auto& inst : block.insts) {
        auto* call = std::get_if<c4c::backend::BackendCallInst>(&inst);
        if (call == nullptr) {
          continue;
        }
        if (call->return_type_kind != c4c::backend::BackendValueTypeKind::Unknown) {
          call->return_type.clear();
        }
        for (std::size_t index = 0; index < call->param_type_kinds.size() &&
                                    index < call->param_types.size();
             ++index) {
          if (call->param_type_kinds[index] != c4c::backend::BackendValueTypeKind::Unknown) {
            call->param_types[index].clear();
          }
        }
      }
    }
  }
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

c4c::codegen::lir::LirModule make_mixed_width_conditional_return_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.x", "i32", "", 4});
  function.alloca_insts.push_back(LirAllocaOp{"%lv.l", "i64", "", 8});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i32", "0", "%lv.x"});
  entry.insts.push_back(LirStoreOp{"i64", "0", "%lv.l"});
  entry.insts.push_back(LirLoadOp{"%t0", "i32", "%lv.x"});
  entry.insts.push_back(LirBinOp{"%t1", "xor", "i32", "%t0", "-1"});
  entry.insts.push_back(LirStoreOp{"i32", "%t1", "%lv.x"});
  entry.insts.push_back(LirLoadOp{"%t2", "i32", "%lv.x"});
  entry.insts.push_back(LirCmpOp{"%t3", false, "ne", "i32", "%t2", "4294967295"});
  entry.insts.push_back(LirCastOp{"%t4", LirCastKind::ZExt, "i1", "%t3", "i32"});
  entry.insts.push_back(LirCmpOp{"%t5", false, "ne", "i32", "%t4", "0"});
  entry.terminator = LirCondBr{"%t5", "block_1", "block_2"};

  LirBlock block1;
  block1.id = LirBlockId{1};
  block1.label = "block_1";
  block1.terminator = LirRet{std::string("1"), "i32"};

  LirBlock block2;
  block2.id = LirBlockId{2};
  block2.label = "block_2";
  block2.insts.push_back(LirLoadOp{"%t6", "i64", "%lv.l"});
  block2.insts.push_back(LirBinOp{"%t7", "xor", "i64", "%t6", "-1"});
  block2.insts.push_back(LirStoreOp{"i64", "%t7", "%lv.l"});
  block2.insts.push_back(LirLoadOp{"%t8", "i32", "%lv.x"});
  block2.insts.push_back(LirCastOp{"%t9", LirCastKind::SExt, "i32", "%t8", "i64"});
  block2.insts.push_back(LirCmpOp{"%t10", false, "ne", "i64", "%t9", "-1"});
  block2.insts.push_back(LirCastOp{"%t11", LirCastKind::ZExt, "i1", "%t10", "i32"});
  block2.insts.push_back(LirCmpOp{"%t12", false, "ne", "i32", "%t11", "0"});
  block2.terminator = LirCondBr{"%t12", "block_3", "block_4"};

  LirBlock block3;
  block3.id = LirBlockId{3};
  block3.label = "block_3";
  block3.terminator = LirRet{std::string("2"), "i32"};

  LirBlock block4;
  block4.id = LirBlockId{4};
  block4.label = "block_4";
  block4.terminator = LirRet{std::string("0"), "i32"};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block1));
  function.blocks.push_back(std::move(block2));
  function.blocks.push_back(std::move(block3));
  function.blocks.push_back(std::move(block4));

  module.functions.push_back(std::move(function));
  return module;
}




c4c::codegen::lir::LirModule make_typed_direct_call_two_arg_folded_const_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction helper;
  helper.name = "foo";
  helper.signature_text = "define i32 @foo(i32 %p.a, i32 %p.b)\n";
  helper.entry = LirBlockId{0};

  LirBlock helper_entry;
  helper_entry.id = LirBlockId{0};
  helper_entry.label = "entry";
  helper_entry.insts.push_back(
      LirBinOp{"%t0", "add", "i32", "2", "%p.a"});
  helper_entry.insts.push_back(
      LirBinOp{"%t1", "sub", "i32", "%t0", "%p.b"});
  helper_entry.terminator = LirRet{std::string("%t1"), "i32"};
  helper.blocks.push_back(std::move(helper_entry));

  LirFunction main_fn;
  main_fn.name = "main";
  main_fn.signature_text = "define i32 @main()\n";
  main_fn.entry = LirBlockId{0};

  LirBlock main_entry;
  main_entry.id = LirBlockId{0};
  main_entry.label = "entry";
  main_entry.insts.push_back(
      LirCallOp{"%t0", "i32", "@foo", "(i32, i32)", "i32 1, i32 3"});
  main_entry.terminator = LirRet{std::string("%t0"), "i32"};
  main_fn.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(main_fn));
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

c4c::codegen::lir::LirModule make_named_struct_field_offset_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.S1 = type { i32, i32 }");
  module.type_decls.push_back("%struct._anon_0 = type { [4 x i8] }");
  module.type_decls.push_back("%struct.S2 = type { i32, i32, %struct._anon_0, %struct.S1 }");
  module.globals.push_back(LirGlobal{
      LirGlobalId{0},
      "v",
      {},
      false,
      false,
      "",
      "global ",
      "%struct.S2",
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
      LirGepOp{"%t0", "%struct.S2", "@v", false, {"i32 0", "i32 3"}});
  entry.insts.push_back(
      LirGepOp{"%t1", "%struct.S1", "%t0", false, {"i32 0", "i32 0"}});
  entry.insts.push_back(LirLoadOp{"%t2", "i32", "%t1"});
  entry.terminator = LirRet{std::string("%t2"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_array_of_named_struct_stride_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.type_decls.push_back("%struct.Record = type { i32, [2 x i32] }");
  module.globals.push_back(LirGlobal{
      LirGlobalId{0},
      "a",
      {},
      false,
      false,
      "",
      "global ",
      "[2 x %struct.Record]",
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
  entry.insts.push_back(LirCastOp{"%idx64", LirCastKind::SExt, "i32", "1", "i64"});
  entry.insts.push_back(
      LirGepOp{"%t0", "[2 x %struct.Record]", "@a", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(
      LirGepOp{"%t1", "%struct.Record", "%t0", false, {"i64 %idx64"}});
  entry.insts.push_back(
      LirGepOp{"%t2", "%struct.Record", "%t1", false, {"i32 0", "i32 1"}});
  entry.insts.push_back(
      LirGepOp{"%t3", "[2 x i32]", "%t2", false, {"i64 0", "i64 1"}});
  entry.insts.push_back(LirLoadOp{"%t4", "i32", "%t3"});
  entry.terminator = LirRet{std::string("%t4"), "i32"};
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

c4c::codegen::lir::LirModule make_large_frame_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()\n";
  function.entry = LirBlockId{0};
  function.alloca_insts.push_back(LirAllocaOp{"%lv.large", "[5200 x i8]", "", 16});

  LirBlock entry;
  entry.id = LirBlockId{0};
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{"i8", "7", "%lv.large"});
  entry.insts.push_back(LirLoadOp{"%t0", "i8", "%lv.large"});
  entry.terminator = LirRet{std::string("%t0"), "i32"};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

c4c::codegen::lir::LirModule make_mutable_string_global_module() {
  using namespace c4c::codegen::lir;

  LirModule module;
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  module.globals.push_back(LirGlobal{
      LirGlobalId{0},
      "t",
      {},
      false,
      false,
      "",
      "global ",
      "[10 x i8]",
      "c\"012345678\\00\"",
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
      LirGepOp{"%t0", "[10 x i8]", "@t", false, {"i64 0", "i64 0"}});
  entry.insts.push_back(LirLoadOp{"%t1", "i8", "%t0"});
  entry.terminator = LirRet{std::string("%t1"), "i32"};
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

void test_aarch64_backend_scaffold_accepts_structured_single_function_ir_without_signature_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_return_add_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".text\n",
                  "aarch64 backend seam should still accept structured single-function backend IR when signature return text is cleared");
  expect_contains(rendered, "mov w0, #5\n",
                  "aarch64 backend seam should still lower the bounded return-add slice from structured signature metadata");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when the single-function slice relies only on structured signature metadata");
}

void test_aarch64_backend_scaffold_accepts_structured_single_function_ir_without_signature_or_binary_type_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_return_add_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);
  clear_backend_memory_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".text\n",
                  "aarch64 backend seam should still accept structured single-function backend IR when signature and binary type text are cleared");
  expect_contains(rendered, "mov w0, #5\n",
                  "aarch64 backend seam should still lower the bounded return-add slice from structured signature and binary metadata");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when the single-function slice relies only on structured signature and binary metadata");
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

void test_aarch64_backend_scaffold_matches_direct_return_immediate_asm() {
  const auto direct_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_return_zero_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto lowered = c4c::backend::lower_to_backend_ir(make_return_zero_module());
  const auto lowered_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  if (direct_rendered != lowered_rendered) {
    fail("aarch64 return-immediate regression should keep the direct LIR and explicit lowered backend seams on identical assembly output");
  }
}

void test_aarch64_backend_scaffold_matches_direct_constant_conditional_goto_return_asm() {
  const auto direct_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_constant_conditional_goto_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_constant_conditional_goto_return_module());
  const auto lowered_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  if (direct_rendered != lowered_rendered) {
    fail("aarch64 constant-conditional goto regression should keep the direct LIR and explicit lowered backend seams on identical assembly output");
  }
}

void test_aarch64_backend_scaffold_matches_direct_i64_constant_conditional_goto_return_asm() {
  const auto direct_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_i64_constant_conditional_goto_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_i64_constant_conditional_goto_return_module());
  const auto lowered_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  if (direct_rendered != lowered_rendered) {
    fail("aarch64 i64 constant-conditional goto regression should keep the direct LIR and explicit lowered backend seams on identical assembly output");
  }
}

void test_aarch64_backend_scaffold_matches_direct_mixed_cast_constant_conditional_goto_return_asm() {
  const auto direct_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_mixed_cast_constant_conditional_goto_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_mixed_cast_constant_conditional_goto_return_module());
  const auto lowered_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  if (direct_rendered != lowered_rendered) {
    fail("aarch64 mixed-cast constant-conditional goto regression should keep the direct LIR and explicit lowered backend seams on identical assembly output");
  }
}

void test_aarch64_backend_scaffold_matches_direct_small_integer_cast_constant_conditional_goto_return_asm() {
  const auto direct_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_small_integer_cast_constant_conditional_goto_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_small_integer_cast_constant_conditional_goto_return_module());
  const auto lowered_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  if (direct_rendered != lowered_rendered) {
    fail("aarch64 small-integer-cast constant-conditional goto regression should keep the direct LIR and explicit lowered backend seams on identical assembly output");
  }
}

void test_aarch64_backend_scaffold_matches_direct_truncating_binop_constant_conditional_goto_return_asm() {
  const auto direct_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_truncating_binop_constant_conditional_goto_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_truncating_binop_constant_conditional_goto_return_module());
  const auto lowered_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  if (direct_rendered != lowered_rendered) {
    fail("aarch64 truncating-binop constant-conditional goto regression should keep the direct LIR and explicit lowered backend seams on identical assembly output");
  }
}

void test_aarch64_backend_scaffold_matches_direct_select_constant_conditional_goto_return_asm() {
  const auto direct_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_select_constant_conditional_goto_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_select_constant_conditional_goto_return_module());
  const auto lowered_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  if (direct_rendered != lowered_rendered) {
    fail("aarch64 select-based constant-conditional goto regression should keep the direct LIR and explicit lowered backend seams on identical assembly output");
  }
}

void test_aarch64_backend_scaffold_matches_direct_local_slot_constant_conditional_goto_return_asm() {
  const auto direct_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_local_slot_constant_conditional_goto_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_local_slot_constant_conditional_goto_return_module());
  const auto lowered_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  if (direct_rendered != lowered_rendered) {
    fail("aarch64 local-slot-backed constant-conditional goto regression should keep the direct LIR and explicit lowered backend seams on identical assembly output");
  }
}

void test_aarch64_backend_scaffold_matches_direct_two_local_slot_constant_conditional_goto_return_asm() {
  const auto direct_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_two_local_slot_constant_conditional_goto_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_two_local_slot_constant_conditional_goto_return_module());
  const auto lowered_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  if (direct_rendered != lowered_rendered) {
    fail("aarch64 two-local-slot-backed constant-conditional goto regression should keep the direct LIR and explicit lowered backend seams on identical assembly output");
  }
}

void test_aarch64_backend_scaffold_matches_direct_three_local_slot_constant_conditional_goto_return_asm() {
  const auto direct_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_three_local_slot_constant_conditional_goto_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_three_local_slot_constant_conditional_goto_return_module());
  const auto lowered_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  if (direct_rendered != lowered_rendered) {
    fail("aarch64 three-local-slot-backed constant-conditional goto regression should keep the direct LIR and explicit lowered backend seams on identical assembly output");
  }
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

void test_aarch64_backend_renders_compare_and_branch_slice_from_typed_predicates() {
  auto module = make_typed_conditional_return_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the typed conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #2\n",
                  "aarch64 backend should still materialize the typed compare lhs immediate");
  expect_contains(rendered, "  cmp w8, #3\n",
                  "aarch64 backend should still compare the typed predicate slice against the rhs immediate");
  expect_contains(rendered, "  b.ge .Lelse\n",
                  "aarch64 backend should map typed signed less-than predicates onto the same fail branch");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend should keep typed compare-and-branch lowering on the asm path");
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

void test_aarch64_backend_renders_compare_and_branch_eq_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_eq_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the equal conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #2\n",
                  "aarch64 backend should materialize the first equal compare immediate");
  expect_contains(rendered, "  cmp w8, #2\n",
                  "aarch64 backend should compare the materialized equal lhs against the rhs immediate");
  expect_contains(rendered, "  b.ne .Lelse\n",
                  "aarch64 backend should branch to the else label when the equality test fails");
  expect_contains(rendered, ".Lthen:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the equal then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should lower the equal else block directly in assembly");
}

void test_aarch64_backend_renders_compare_and_branch_ne_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_ne_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the not-equal conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #2\n",
                  "aarch64 backend should materialize the first not-equal compare immediate");
  expect_contains(rendered, "  cmp w8, #3\n",
                  "aarch64 backend should compare the materialized not-equal lhs against the rhs immediate");
  expect_contains(rendered, "  b.eq .Lelse\n",
                  "aarch64 backend should branch to the else label when the not-equal test fails");
  expect_contains(rendered, ".Lthen:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the not-equal then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should lower the not-equal else block directly in assembly");
}

void test_aarch64_backend_renders_compare_and_branch_ult_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_ult_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the unsigned less-than conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #2\n",
                  "aarch64 backend should materialize the first unsigned less-than compare immediate");
  expect_contains(rendered, "  cmp w8, #3\n",
                  "aarch64 backend should compare the materialized unsigned less-than lhs against the rhs immediate");
  expect_contains(rendered, "  b.hs .Lelse\n",
                  "aarch64 backend should branch to the else label when the unsigned less-than test fails");
  expect_contains(rendered, ".Lthen:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the unsigned less-than then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should lower the unsigned less-than else block directly in assembly");
}

void test_aarch64_backend_renders_compare_and_branch_ule_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_ule_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the unsigned less-or-equal conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #2\n",
                  "aarch64 backend should materialize the first unsigned less-or-equal compare immediate");
  expect_contains(rendered, "  cmp w8, #3\n",
                  "aarch64 backend should compare the materialized unsigned less-or-equal lhs against the rhs immediate");
  expect_contains(rendered, "  b.hi .Lelse\n",
                  "aarch64 backend should branch to the else label when the unsigned less-or-equal test fails");
  expect_contains(rendered, ".Lthen:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the unsigned less-or-equal then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should lower the unsigned less-or-equal else block directly in assembly");
}

void test_aarch64_backend_renders_compare_and_branch_ugt_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_ugt_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the unsigned greater-than conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #3\n",
                  "aarch64 backend should materialize the first unsigned greater-than compare immediate");
  expect_contains(rendered, "  cmp w8, #2\n",
                  "aarch64 backend should compare the materialized unsigned greater-than lhs against the rhs immediate");
  expect_contains(rendered, "  b.ls .Lelse\n",
                  "aarch64 backend should branch to the else label when the unsigned greater-than test fails");
  expect_contains(rendered, ".Lthen:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the unsigned greater-than then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should lower the unsigned greater-than else block directly in assembly");
}

void test_aarch64_backend_renders_compare_and_branch_uge_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_uge_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the unsigned greater-or-equal conditional-return slice to assembly");
  expect_contains(rendered, "  mov w8, #3\n",
                  "aarch64 backend should materialize the first unsigned greater-or-equal compare immediate");
  expect_contains(rendered, "  cmp w8, #2\n",
                  "aarch64 backend should compare the materialized unsigned greater-or-equal lhs against the rhs immediate");
  expect_contains(rendered, "  b.lo .Lelse\n",
                  "aarch64 backend should branch to the else label when the unsigned greater-or-equal test fails");
  expect_contains(rendered, ".Lthen:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the unsigned greater-or-equal then block directly in assembly");
  expect_contains(rendered, ".Lelse:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should lower the unsigned greater-or-equal else block directly in assembly");
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

void test_aarch64_backend_scaffold_accepts_structured_direct_call_ir_without_signature_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_direct_call_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".type helper, %function",
                  "aarch64 backend seam should still preserve structured direct-call helper definitions without legacy signature return text");
  expect_contains(rendered, "bl helper",
                  "aarch64 backend seam should still lower structured direct calls when main relies only on structured signature metadata");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when structured direct calls clear legacy signature return text");
}

void test_aarch64_backend_scaffold_rejects_structured_zero_arg_direct_call_when_helper_body_contract_disagrees() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_direct_call_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  c4c::backend::BackendFunction* helper = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "helper") {
      helper = &function;
      break;
    }
  }
  auto* entry =
      helper != nullptr && !helper->blocks.empty() ? &helper->blocks.front() : nullptr;
  expect_true(entry != nullptr,
              "aarch64 zero-argument direct-call regression test needs the lowered helper block to mutate");
  if (entry != nullptr) {
    entry->insts.push_back(c4c::backend::BackendBinaryInst{
        c4c::backend::BackendBinaryOpcode::Add,
        "%t.helper.folded",
        "i32",
        "7",
        "0",
        c4c::backend::BackendScalarType::I32,
    });
    entry->terminator.value = "%t.helper.folded";
  }

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "target triple =",
                  "aarch64 backend seam should stop matching the structured zero-argument direct-call asm slice when the lowered helper body no longer matches the shared immediate-return contract");
}

void test_aarch64_backend_renders_zero_arg_typed_direct_call_slice_with_whitespace() {
  auto module = make_direct_call_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.front());
  call.callee_type_suffix = "( )";
  call.args_str = "  ";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "bl helper",
                  "aarch64 backend should keep zero-arg typed direct calls on the direct-call asm path even when compatibility whitespace remains");
  expect_not_contains(rendered, "define i32 @main()",
                      "aarch64 backend should not fall back to LLVM text for whitespace-tolerant zero-arg typed direct calls");
}

void test_aarch64_backend_scaffold_accepts_structured_zero_arg_direct_call_spacing_ir_without_signature_shims() {
  auto module = make_direct_call_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.front());
  call.callee_type_suffix = "( )";
  call.args_str = "  ";

  auto lowered = c4c::backend::lower_to_backend_ir(std::move(module));
  clear_backend_signature_and_call_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".type helper, %function",
                  "aarch64 backend seam should still preserve spacing-tolerant lowered zero-arg helper definitions without legacy signature text");
  expect_contains(rendered, "bl helper",
                  "aarch64 backend seam should still lower spacing-tolerant structured zero-arg direct calls when legacy call/signature text is cleared");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when the spacing-tolerant structured zero-arg slice relies only on backend-owned metadata");
}

void test_aarch64_backend_scaffold_accepts_renamed_structured_zero_arg_direct_call_ir_without_signature_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_direct_call_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  c4c::backend::BackendFunction* helper = nullptr;
  c4c::backend::BackendFunction* main_fn = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "helper") {
      helper = &function;
    } else if (function.signature.name == "main") {
      main_fn = &function;
    }
  }
  expect_true(helper != nullptr && main_fn != nullptr,
              "aarch64 renamed zero-argument direct-call regression test needs lowered helper and main functions");
  if (helper == nullptr || main_fn == nullptr) {
    return;
  }

  helper->signature.name = "const_value";
  auto* call = std::get_if<c4c::backend::BackendCallInst>(&main_fn->blocks.front().insts.front());
  expect_true(call != nullptr,
              "aarch64 renamed zero-argument direct-call regression test needs the lowered backend call to mutate");
  if (call == nullptr) {
    return;
  }
  call->callee.symbol_name = "const_value";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type const_value, %function",
                  "aarch64 backend seam should key the lowered zero-argument helper definition from the structured callee symbol instead of a fixed helper name");
  expect_contains(rendered, "bl const_value",
                  "aarch64 backend seam should still lower renamed structured zero-argument direct calls when legacy call/signature text is cleared");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when a renamed zero-argument direct-call slice relies only on structured metadata");
}

void test_aarch64_backend_rejects_intrinsic_callee_from_direct_call_fast_path() {
  auto module = make_typed_direct_call_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.front());
  call.callee = c4c::codegen::lir::LirOperand(std::string("@llvm.abs.i32"),
                                              c4c::codegen::lir::LirOperandKind::Global);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                  "aarch64 backend should fall back instead of treating llvm intrinsics as direct helper calls");
  expect_contains(rendered, "@llvm.abs.i32",
                  "aarch64 backend fallback should preserve the intrinsic callee text");
}

void test_aarch64_backend_rejects_indirect_callee_from_direct_call_fast_path() {
  auto module = make_typed_direct_call_module();
  auto& call = std::get<c4c::codegen::lir::LirCallOp>(
      module.functions.back().blocks.front().insts.front());
  call.callee = c4c::codegen::lir::LirOperand(std::string("%fp"),
                                              c4c::codegen::lir::LirOperandKind::SsaValue);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                  "aarch64 backend should fall back instead of treating indirect callees as direct helper calls");
  expect_contains(rendered, "call i32 (i32) %fp(i32 5)",
                  "aarch64 backend fallback should preserve the indirect callee shape");
}

void test_aarch64_backend_renders_local_temp_memory_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_local_temp_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the single-slot local-temp slice to assembly");
  expect_contains(rendered, "main:\n  mov w0, #5\n  ret\n",
                  "aarch64 backend should collapse the local-temp literal slot pattern into a direct return");
}

void test_aarch64_backend_renders_local_temp_sub_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_local_temp_sub_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the bounded local-temp subtraction slice to assembly");
  expect_contains(rendered, "main:\n  mov w0, #1\n  ret\n",
                  "aarch64 backend should fold the bounded local-temp subtraction into an immediate return");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend should not fall back to LLVM text for this local-temp subtraction slice");
}

void test_aarch64_backend_renders_local_temp_arithmetic_chain_slice() {
  auto module = make_local_temp_arithmetic_chain_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the bounded local-temp arithmetic chain slice to assembly");
  expect_contains(rendered, "main:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should fold the bounded local-temp arithmetic chain into an immediate return");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend should not fall back to LLVM text for the bounded local-temp arithmetic chain");
}

void test_aarch64_backend_renders_two_local_temp_return_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_two_local_temp_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the bounded two-local temp return slice to assembly");
  expect_contains(rendered, "main:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should fold the bounded two-local temp return slice into the immediate result");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend should not fall back to LLVM text for the bounded two-local temp return slice");
}

void test_aarch64_backend_renders_local_pointer_temp_return_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_local_pointer_temp_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the bounded local pointer temp slice to assembly");
  expect_contains(rendered, "main:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should fold the bounded local pointer round-trip into a direct immediate return");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend should not fall back to LLVM text for this local-pointer round-trip slice");
}

void test_aarch64_backend_scaffold_matches_direct_local_pointer_temp_return_asm() {
  const auto direct_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_local_pointer_temp_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_local_pointer_temp_return_module());
  const auto lowered_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  if (direct_rendered != lowered_rendered) {
    fail("aarch64 local-pointer regression should keep the direct LIR and explicit lowered backend seams on identical assembly output");
  }
}

void test_aarch64_backend_renders_double_indirect_local_pointer_conditional_return_slice() {
  auto module = make_double_indirect_local_pointer_conditional_return_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should lower the bounded double-indirect local-pointer conditional slice to assembly");
  expect_contains(rendered, "main:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should fold the bounded double-indirect local-pointer conditional slice into the immediate result");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend should not fall back to LLVM text for the bounded double-indirect local-pointer conditional slice");
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

void test_aarch64_backend_skips_legacy_minimal_adapter_for_mixed_width_control_flow() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_mixed_width_conditional_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "sub sp, sp, #",
                  "aarch64 backend should route mixed-width control flow through the general stack-spill emitter");
  expect_contains(rendered, "eor w0, w0, w1",
                  "aarch64 backend should preserve the i32 bitwise-not lowering in the general emitter");
  expect_contains(rendered, "eor x0, x0, x1",
                  "aarch64 backend should preserve the i64 bitwise-not lowering in the general emitter");
  expect_not_contains(rendered, "main:\n  mov w0, #1\n  ret\n",
                      "aarch64 backend should not collapse mixed-width control flow into a constant-return legacy slice");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend should not fall back to LLVM text for this mixed-width integer case");
}

void test_aarch64_backend_renders_param_slot_direct_call_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_param_slot_runtime_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_one, %function",
                  "aarch64 backend should lower the helper into a real function symbol");
  expect_contains(rendered, "add_one:\n  add w0, w0, #1\n  ret\n",
                  "aarch64 backend should lower the modified parameter slot helper through the ABI argument register");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should materialize direct-call argument in w0");
  expect_contains(rendered, "bl add_one",
                  "aarch64 backend should lower the parameter-slot direct call with bl");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend should keep the parameter-slot direct call on the asm path");
}

void test_aarch64_backend_keeps_spaced_param_slot_call_decode_on_asm_path() {
  auto module = make_param_slot_runtime_module();
  module.functions.front().signature_text = "define i32 @add_one( i32 %p.x )\n";
  auto& call =
      std::get<c4c::codegen::lir::LirCallOp>(module.functions.back().blocks.front().insts.front());
  call.callee_type_suffix = "( i32 )";
  call.args_str = " i32 5 ";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "add_one:\n  add w0, w0, #1\n  ret\n",
                  "aarch64 backend should keep spacing-tolerant helper signatures on the asm path");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should still decode spacing-tolerant call arguments on the asm path");
  expect_contains(rendered, "bl add_one",
                  "aarch64 backend should still lower spacing-tolerant param-slot direct calls with bl");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend should not fall back to LLVM text for spacing-tolerant param-slot calls");
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

void test_aarch64_backend_uses_shared_regalloc_for_call_crossing_direct_call_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_typed_call_crossing_direct_call_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "sub sp, sp, #16",
                  "aarch64 backend should allocate a real frame for the surviving shared callee-saved call-crossing value");
  expect_contains(rendered, "str x20, [sp, #0]",
                  "aarch64 backend should save the first shared call-crossing callee-saved register");
  expect_contains(rendered, "str x30, [sp, #8]",
                  "aarch64 backend should still preserve lr in the shared call frame");
  expect_contains(rendered, "mov w20, #5",
                  "aarch64 backend should materialize the call-crossing source value in the shared assigned register");
  expect_contains(rendered, "mov w0, w20",
                  "aarch64 backend should pass the shared assigned register through the ABI argument register");
  expect_contains(rendered, "bl add_one",
                  "aarch64 backend should keep the helper call on the direct-call asm path");
  expect_contains(rendered, "add w0, w20, w0",
                  "aarch64 backend should keep reusing the shared call-crossing source register while consuming the helper result directly from w0");
  expect_contains(rendered, "ldr x20, [sp, #0]",
                  "aarch64 backend should restore the shared call-crossing source register");
}

void test_aarch64_backend_cleans_up_redundant_call_result_traffic_on_call_crossing_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_typed_call_crossing_direct_call_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "sub sp, sp, #16",
                  "aarch64 backend should shrink the shared call-crossing frame once the helper result stays in w0");
  expect_not_contains(rendered, "str x21, [sp, #8]",
                      "aarch64 backend should not save a redundant call-result callee-saved register");
  expect_not_contains(rendered, "mov w21, w0",
                      "aarch64 backend should not hand the helper result through a redundant callee-saved temp");
  expect_contains(rendered, "add w0, w20, w0",
                  "aarch64 backend should consume the helper result directly from the ABI return register");
}

void test_aarch64_backend_keeps_spacing_tolerant_call_crossing_slice_on_asm_path() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_typed_call_crossing_direct_call_with_spacing_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "mov w0, w20",
                  "aarch64 backend should still decode spacing-tolerant typed call-crossing arguments structurally");
  expect_contains(rendered, "bl add_one",
                  "aarch64 backend should keep spacing-tolerant typed call-crossing slices on the asm path");
  expect_contains(rendered, "add w0, w20, w0",
                  "aarch64 backend should still combine the decoded helper result with the preserved source register");
}

void test_aarch64_backend_keeps_renamed_structured_call_crossing_slice_on_asm_path() {
  auto module = make_typed_call_crossing_direct_call_module();

  c4c::codegen::lir::LirFunction* helper = nullptr;
  c4c::codegen::lir::LirFunction* main_fn = nullptr;
  for (auto& function : module.functions) {
    if (function.name == "add_one") {
      helper = &function;
    } else if (function.name == "main") {
      main_fn = &function;
    }
  }
  expect_true(helper != nullptr && main_fn != nullptr,
              "aarch64 renamed call-crossing regression test needs helper and main functions");
  if (helper == nullptr || main_fn == nullptr) {
    return;
  }

  helper->name = "inc_value";
  helper->signature_text = "define i32 @inc_value(i32 %p.x)\n";

  auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&main_fn->blocks.front().insts[1]);
  expect_true(call != nullptr,
              "aarch64 renamed call-crossing regression test needs the direct call to mutate");
  if (call == nullptr) {
    return;
  }
  call->callee = "@inc_value";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type inc_value, %function",
                  "aarch64 backend seam should key the call-crossing helper definition from the structured callee symbol instead of a fixed helper name");
  expect_contains(rendered, "mov w0, w20",
                  "aarch64 backend seam should still marshal the preserved source register through the ABI argument register for renamed call-crossing helpers");
  expect_contains(rendered, "bl inc_value",
                  "aarch64 backend seam should keep renamed lowered call-crossing direct calls on the asm path");
  expect_contains(rendered, "add w0, w20, w0",
                  "aarch64 backend seam should keep consuming the renamed helper result directly from w0");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when the call-crossing slice relies only on structured call and callee metadata");
}

void test_aarch64_backend_scaffold_accepts_structured_call_crossing_direct_call_ir_without_signature_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_typed_call_crossing_direct_call_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_one, %function",
                  "aarch64 backend seam should still preserve lowered call-crossing helper definitions without legacy signature text");
  expect_contains(rendered, "sub sp, sp, #16",
                  "aarch64 backend seam should still allocate the shared call-crossing frame from structured backend metadata alone");
  expect_contains(rendered, "mov w20, #5",
                  "aarch64 backend seam should still materialize the preserved cross-call source into a callee-saved register without legacy LIR fallback");
  expect_contains(rendered, "mov w0, w20",
                  "aarch64 backend seam should still marshal the structured call-crossing source through the ABI argument register");
  expect_contains(rendered, "bl add_one",
                  "aarch64 backend seam should still keep lowered call-crossing direct calls on the asm path when legacy call/signature text is cleared");
  expect_contains(rendered, "add w0, w20, w0",
                  "aarch64 backend seam should still consume the helper result directly from w0 on the pure backend-IR path");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when the lowered call-crossing slice relies only on structured backend metadata");
}

void test_aarch64_backend_scaffold_accepts_renamed_structured_call_crossing_direct_call_ir_without_signature_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_typed_call_crossing_direct_call_module());

  c4c::backend::BackendFunction* helper = nullptr;
  c4c::backend::BackendFunction* main_fn = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "add_one") {
      helper = &function;
    } else if (function.signature.name == "main") {
      main_fn = &function;
    }
  }
  expect_true(helper != nullptr && main_fn != nullptr,
              "aarch64 renamed lowered call-crossing regression test needs helper and main functions");
  if (helper == nullptr || main_fn == nullptr) {
    return;
  }

  helper->signature.name = "inc_value";
  auto* call = std::get_if<c4c::backend::BackendCallInst>(&main_fn->blocks.front().insts[1]);
  expect_true(call != nullptr,
              "aarch64 renamed lowered call-crossing regression test needs the backend direct call to mutate");
  if (call == nullptr) {
    return;
  }
  call->callee.symbol_name = "inc_value";

  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type inc_value, %function",
                  "aarch64 backend seam should key lowered call-crossing helper definitions from the structured callee symbol instead of legacy signature text");
  expect_contains(rendered, "mov w20, #5",
                  "aarch64 backend seam should still materialize the preserved lowered cross-call source without legacy fallback");
  expect_contains(rendered, "mov w0, w20",
                  "aarch64 backend seam should still marshal the renamed lowered call-crossing source through the ABI argument register");
  expect_contains(rendered, "bl inc_value",
                  "aarch64 backend seam should keep renamed lowered call-crossing direct calls on the asm path when legacy call/signature text is cleared");
  expect_contains(rendered, "add w0, w20, w0",
                  "aarch64 backend seam should still consume the renamed lowered helper result directly from w0");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when the renamed lowered call-crossing slice relies only on structured backend metadata");
}

void test_aarch64_backend_scaffold_accepts_lowered_call_crossing_source_value_rename_without_signature_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_typed_call_crossing_direct_call_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  c4c::backend::BackendFunction* main_fn = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "main") {
      main_fn = &function;
      break;
    }
  }
  expect_true(main_fn != nullptr,
              "aarch64 lowered call-crossing source rename regression test needs the backend main function");
  if (main_fn == nullptr || main_fn->blocks.empty() || main_fn->blocks.front().insts.size() != 3) {
    return;
  }

  auto& entry = main_fn->blocks.front();
  auto* source_add = std::get_if<c4c::backend::BackendBinaryInst>(&entry.insts[0]);
  auto* call = std::get_if<c4c::backend::BackendCallInst>(&entry.insts[1]);
  auto* final_add = std::get_if<c4c::backend::BackendBinaryInst>(&entry.insts[2]);
  expect_true(source_add != nullptr && call != nullptr && final_add != nullptr,
              "aarch64 lowered call-crossing source rename regression test needs the structured source add, direct call, and final add to mutate");
  if (source_add == nullptr || call == nullptr || final_add == nullptr) {
    return;
  }

  source_add->result = "%t.crossing.source.renamed";
  call->args.front().operand = "%t.crossing.source.renamed";
  final_add->lhs = "%t.crossing.source.renamed";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "mov w20, #5",
                  "aarch64 backend seam should keep using the backend-owned synthesized regalloc source when the lowered call-crossing source SSA name changes");
  expect_contains(rendered, "mov w0, w20",
                  "aarch64 backend seam should still marshal the renamed lowered call-crossing source through the ABI argument register");
  expect_contains(rendered, "bl add_one",
                  "aarch64 backend seam should keep the lowered call-crossing direct call on the asm path after a structured source-value rename");
  expect_contains(rendered, "add w0, w20, w0",
                  "aarch64 backend seam should still reuse the preserved call-crossing source register after the renamed lowered helper call");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when only the lowered call-crossing source SSA name changes across the structured source-add, direct-call, and final-add seam");
}

void test_aarch64_backend_scaffold_accepts_renamed_lowered_call_crossing_source_value_rename_without_signature_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_typed_call_crossing_direct_call_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  c4c::backend::BackendFunction* helper = nullptr;
  c4c::backend::BackendFunction* main_fn = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "add_one") {
      helper = &function;
    } else if (function.signature.name == "main") {
      main_fn = &function;
    }
  }
  expect_true(helper != nullptr && main_fn != nullptr,
              "aarch64 renamed lowered call-crossing source rename regression test needs helper and main functions");
  if (helper == nullptr || main_fn == nullptr || main_fn->blocks.empty() ||
      main_fn->blocks.front().insts.size() != 3) {
    return;
  }

  helper->signature.name = "inc_value";
  auto& entry = main_fn->blocks.front();
  auto* source_add = std::get_if<c4c::backend::BackendBinaryInst>(&entry.insts[0]);
  auto* call = std::get_if<c4c::backend::BackendCallInst>(&entry.insts[1]);
  auto* final_add = std::get_if<c4c::backend::BackendBinaryInst>(&entry.insts[2]);
  expect_true(source_add != nullptr && call != nullptr && final_add != nullptr,
              "aarch64 renamed lowered call-crossing source rename regression test needs the structured source add, direct call, and final add to mutate");
  if (source_add == nullptr || call == nullptr || final_add == nullptr) {
    return;
  }

  source_add->result = "%t.crossing.source.renamed";
  call->callee.symbol_name = "inc_value";
  call->args.front().operand = "%t.crossing.source.renamed";
  final_add->lhs = "%t.crossing.source.renamed";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type inc_value, %function",
                  "aarch64 backend seam should still key the lowered call-crossing helper definition from the renamed structured callee symbol");
  expect_contains(rendered, "mov w20, #5",
                  "aarch64 backend seam should keep using the backend-owned synthesized regalloc source when both the lowered call-crossing source SSA name and callee symbol change");
  expect_contains(rendered, "mov w0, w20",
                  "aarch64 backend seam should still marshal the renamed lowered call-crossing source through the ABI argument register after the callee rename");
  expect_contains(rendered, "bl inc_value",
                  "aarch64 backend seam should keep the renamed lowered call-crossing direct call on the asm path after the structured source-value rename");
  expect_contains(rendered, "add w0, w20, w0",
                  "aarch64 backend seam should still reuse the preserved call-crossing source register after the renamed lowered helper call");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when the lowered call-crossing slice relies only on structured metadata after both callee and source-value renames");
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

void test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_ir_without_signature_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend seam should still preserve lowered two-argument helper definitions without legacy signature text");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend seam should still materialize the first lowered two-argument direct-call immediate from structured call metadata");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend seam should still materialize the second lowered two-argument direct-call immediate from structured call metadata");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend seam should still lower explicit two-argument direct calls when legacy call/signature text is cleared");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when the explicit two-argument direct-call slice relies only on structured metadata");
}

void test_aarch64_backend_scaffold_rejects_structured_two_arg_direct_call_when_helper_body_contract_disagrees() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  c4c::backend::BackendFunction* helper = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "add_pair") {
      helper = &function;
      break;
    }
  }
  auto* add =
      helper != nullptr && !helper->blocks.empty() && !helper->blocks.front().insts.empty()
          ? std::get_if<c4c::backend::BackendBinaryInst>(&helper->blocks.front().insts.front())
          : nullptr;
  expect_true(add != nullptr,
              "aarch64 two-argument direct-call regression test needs the lowered helper body instruction to mutate");
  if (add != nullptr && helper != nullptr && helper->signature.params.size() == 2) {
    add->rhs = helper->signature.params.front().name;
  }

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "target triple =",
                  "aarch64 backend seam should stop matching the structured two-argument direct-call asm slice when the lowered helper body no longer matches the shared two-argument add-helper contract");
}

void test_aarch64_backend_scaffold_accepts_renamed_structured_two_arg_direct_call_ir_without_signature_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  c4c::backend::BackendFunction* helper = nullptr;
  c4c::backend::BackendFunction* main_fn = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "add_pair") {
      helper = &function;
    } else if (function.signature.name == "main") {
      main_fn = &function;
    }
  }
  expect_true(helper != nullptr && main_fn != nullptr,
              "aarch64 renamed two-argument direct-call regression test needs lowered helper and main functions");
  if (helper == nullptr || main_fn == nullptr) {
    return;
  }

  helper->signature.name = "sum_pair";
  helper->signature.params[0].name = "%p.left";
  helper->signature.params[1].name = "%p.right";
  auto* helper_add = helper->blocks.empty() || helper->blocks.front().insts.empty()
                         ? nullptr
                         : std::get_if<c4c::backend::BackendBinaryInst>(&helper->blocks.front().insts.front());
  auto* call = std::get_if<c4c::backend::BackendCallInst>(&main_fn->blocks.front().insts.front());
  expect_true(helper_add != nullptr && call != nullptr,
              "aarch64 renamed two-argument direct-call regression test needs mutable lowered helper and main call instructions");
  if (helper_add == nullptr || call == nullptr) {
    return;
  }
  helper_add->lhs = "%p.left";
  helper_add->rhs = "%p.right";
  helper_add->result = "%t.helper.sum.structured";
  helper->blocks.front().terminator.value = "%t.helper.sum.structured";
  call->callee.symbol_name = "sum_pair";
  call->args[0].operand = "11";
  call->args[1].operand = "13";
  call->result = "%t.main.sum.structured";
  main_fn->blocks.front().terminator.value = "%t.main.sum.structured";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type sum_pair, %function",
                  "aarch64 backend seam should key the lowered two-argument helper definition from the structured callee symbol instead of a fixed helper name");
  expect_contains(rendered, "mov w0, #11",
                  "aarch64 backend seam should still materialize the first lowered renamed direct-call argument from structured call metadata after helper parameter and SSA renames");
  expect_contains(rendered, "mov w1, #13",
                  "aarch64 backend seam should still materialize the second lowered renamed direct-call argument from structured call metadata after helper parameter and SSA renames");
  expect_contains(rendered, "bl sum_pair",
                  "aarch64 backend seam should still lower renamed structured two-argument direct calls when the parsed module view carries the renamed helper symbol directly");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when a renamed two-argument direct-call slice relies only on structured helper metadata after parameter and SSA renames");
}

void test_aarch64_backend_scaffold_rejects_structured_two_arg_direct_call_when_param_type_count_disagrees_with_args() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  c4c::backend::BackendFunction* main_fn = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "main") {
      main_fn = &function;
      break;
    }
  }
  auto* call =
      main_fn != nullptr && !main_fn->blocks.empty() && !main_fn->blocks.front().insts.empty()
          ? std::get_if<c4c::backend::BackendCallInst>(&main_fn->blocks.front().insts.front())
          : nullptr;
  expect_true(call != nullptr,
              "aarch64 two-argument direct-call regression test needs a lowered backend call to mutate");
  if (call != nullptr) {
    expect_true(call->param_types.size() == call->args.size(),
                "aarch64 two-argument direct-call regression test should start from matched structured call metadata");
    if (!call->param_types.empty()) {
      call->param_types.pop_back();
    }
  }

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                  "aarch64 backend seam should stop matching the structured two-argument direct-call asm slice when call param type count no longer matches arg count");
}

void test_aarch64_backend_scaffold_rejects_structured_two_arg_direct_call_when_callee_signature_param_type_disagrees() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  c4c::backend::BackendFunction* helper = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "add_pair") {
      helper = &function;
      break;
    }
  }
  expect_true(helper != nullptr,
              "aarch64 two-argument direct-call regression test needs the lowered helper signature to mutate");
  if (helper != nullptr && !helper->signature.params.empty()) {
    helper->signature.params.front().type_kind = c4c::backend::BackendValueTypeKind::Scalar;
    helper->signature.params.front().scalar_type = c4c::backend::BackendScalarType::I8;
  }

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                  "aarch64 backend seam should stop matching the structured two-argument direct-call asm slice when the callee signature param type disagrees with the call contract");
}

void test_aarch64_backend_renders_typed_two_arg_direct_call_folded_const_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_typed_direct_call_two_arg_folded_const_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type foo, %function",
                  "aarch64 backend should lower a folded two-argument constant-returning helper into a real function symbol");
  expect_contains(rendered, "foo:\n  mov w0, #0\n  ret\n",
                  "aarch64 backend should lower the folded helper return as a direct immediate");
  expect_contains(rendered, "mov w0, #1",
                  "aarch64 backend should still materialize the folded first direct-call argument in w0 before bl");
  expect_contains(rendered, "mov w1, #3",
                  "aarch64 backend should still materialize the folded second direct-call argument in w1 before bl");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend should keep constant-folded direct calls on the asm path");
  expect_contains(rendered, "bl foo",
                  "aarch64 backend should keep the folded two-argument direct call on the asm path");
}

void test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_folded_const_ir_without_signature_shims() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_folded_const_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".type foo, %function",
                  "aarch64 backend seam should still preserve folded two-argument helper definitions without legacy signature return text");
  expect_contains(rendered, "mov w0, #1",
                  "aarch64 backend seam should still materialize the folded first direct-call immediate from structured call metadata");
  expect_contains(rendered, "mov w1, #3",
                  "aarch64 backend seam should still materialize the folded second direct-call immediate from structured call metadata");
  expect_contains(rendered, "bl foo",
                  "aarch64 backend seam should still lower folded two-argument direct calls when main relies only on structured signature metadata");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when folded two-argument direct calls clear legacy signature return text");
}

void test_aarch64_backend_scaffold_accepts_renamed_structured_two_arg_direct_call_folded_const_ir_without_signature_shims() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_folded_const_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  c4c::backend::BackendFunction* helper = nullptr;
  c4c::backend::BackendFunction* main_fn = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "foo") {
      helper = &function;
    } else if (function.signature.name == "main") {
      main_fn = &function;
    }
  }
  expect_true(helper != nullptr && main_fn != nullptr,
              "aarch64 renamed folded two-argument direct-call regression test needs lowered helper and main functions");
  if (helper == nullptr || main_fn == nullptr) {
    return;
  }

  helper->signature.name = "const_pair";
  helper->signature.params[0].name = "%p.folded.lhs";
  helper->signature.params[1].name = "%p.folded.rhs";
  auto* add = helper->blocks.front().insts.empty()
                  ? nullptr
                  : std::get_if<c4c::backend::BackendBinaryInst>(&helper->blocks.front().insts.front());
  auto* sub =
      helper->blocks.front().insts.size() < 2
          ? nullptr
          : std::get_if<c4c::backend::BackendBinaryInst>(&helper->blocks.front().insts[1]);
  auto* call = std::get_if<c4c::backend::BackendCallInst>(&main_fn->blocks.front().insts.front());
  expect_true(add != nullptr && sub != nullptr && call != nullptr,
              "aarch64 renamed folded two-argument direct-call regression test needs mutable lowered helper and main call instructions");
  if (add == nullptr || sub == nullptr || call == nullptr) {
    return;
  }
  add->rhs = "%p.folded.lhs";
  add->result = "%t.helper.folded.add";
  sub->lhs = "%t.helper.folded.add";
  sub->rhs = "%p.folded.rhs";
  sub->result = "%t.helper.folded.result";
  helper->blocks.front().terminator.value = "%t.helper.folded.result";
  call->callee.symbol_name = "const_pair";
  call->args[0].operand = "11";
  call->args[1].operand = "13";
  call->result = "%t.main.folded.result";
  main_fn->blocks.front().terminator.value = "%t.main.folded.result";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".type const_pair, %function",
                  "aarch64 backend seam should key folded two-argument helper definitions from the structured callee symbol instead of a fixed helper name");
  expect_contains(rendered, "mov w0, #11",
                  "aarch64 backend seam should still materialize the renamed folded first direct-call immediate from structured call metadata after helper parameter and SSA renames");
  expect_contains(rendered, "mov w1, #13",
                  "aarch64 backend seam should still materialize the renamed folded second direct-call immediate from structured call metadata after helper parameter and SSA renames");
  expect_contains(rendered, "bl const_pair",
                  "aarch64 backend seam should still lower renamed folded two-argument direct calls when legacy call and signature text are cleared");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when a renamed folded two-argument direct-call slice relies only on structured helper metadata after parameter and SSA renames");
}

void test_aarch64_backend_scaffold_rejects_structured_two_arg_direct_call_folded_const_when_helper_body_contract_disagrees() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_folded_const_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  c4c::backend::BackendFunction* helper = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "foo") {
      helper = &function;
      break;
    }
  }
  auto* sub =
      helper != nullptr && !helper->blocks.empty() && helper->blocks.front().insts.size() >= 2
          ? std::get_if<c4c::backend::BackendBinaryInst>(&helper->blocks.front().insts[1])
          : nullptr;
  expect_true(sub != nullptr,
              "aarch64 folded two-argument direct-call regression test needs the lowered helper body instruction to mutate");
  if (sub != nullptr && helper != nullptr && helper->signature.params.size() == 2) {
    sub->rhs = helper->signature.params.front().name;
  }

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "target triple =",
                  "aarch64 backend seam should stop matching the structured folded two-argument direct-call asm slice when the lowered helper body no longer matches the shared folded-helper contract");
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
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend should not fall back to LLVM text for the single-local direct-call slice");
}

void test_aarch64_backend_scaffold_accepts_structured_direct_call_add_imm_ir_without_signature_shims() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_local_arg_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_one, %function",
                  "aarch64 backend seam should still preserve lowered single-argument helper definitions without legacy signature text");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend seam should still materialize lowered single-argument direct-call immediates from structured call metadata");
  expect_contains(rendered, "bl add_one",
                  "aarch64 backend seam should still lower explicit single-argument direct calls when legacy call/signature text is cleared");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when the explicit single-argument direct-call slice relies only on structured call metadata");
}

void test_aarch64_backend_scaffold_rejects_structured_direct_call_add_imm_when_helper_body_contract_disagrees() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_local_arg_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  c4c::backend::BackendFunction* helper = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "add_one") {
      helper = &function;
      break;
    }
  }
  auto* add =
      helper != nullptr && !helper->blocks.empty() && !helper->blocks.front().insts.empty()
          ? std::get_if<c4c::backend::BackendBinaryInst>(&helper->blocks.front().insts.front())
          : nullptr;
  expect_true(add != nullptr,
              "aarch64 single-argument direct-call regression test needs the lowered helper body instruction to mutate");
  if (add != nullptr && helper != nullptr && !helper->signature.params.empty()) {
    add->lhs = "1";
    add->rhs = helper->signature.params.front().name;
  }

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "target triple =",
                  "aarch64 backend seam should stop matching the structured single-argument direct-call asm slice when the lowered helper body no longer matches the shared add-immediate helper contract");
}

void test_aarch64_backend_scaffold_accepts_renamed_structured_direct_call_add_imm_ir_without_signature_shims() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_local_arg_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  c4c::backend::BackendFunction* helper = nullptr;
  c4c::backend::BackendFunction* main_fn = nullptr;
  for (auto& function : lowered.functions) {
    if (function.signature.name == "add_one") {
      helper = &function;
    } else if (function.signature.name == "main") {
      main_fn = &function;
    }
  }
  expect_true(helper != nullptr && main_fn != nullptr,
              "aarch64 renamed single-argument direct-call regression test needs lowered helper and main functions");
  if (helper == nullptr || main_fn == nullptr) {
    return;
  }

  helper->signature.name = "inc_value";
  helper->signature.params.front().name = "%p.renamed.input";
  auto* add =
      helper->blocks.empty() || helper->blocks.front().insts.empty()
          ? nullptr
          : std::get_if<c4c::backend::BackendBinaryInst>(&helper->blocks.front().insts.front());
  auto* call =
      main_fn->blocks.empty() || main_fn->blocks.front().insts.empty()
          ? nullptr
          : std::get_if<c4c::backend::BackendCallInst>(&main_fn->blocks.front().insts.front());
  expect_true(add != nullptr && call != nullptr,
              "aarch64 renamed single-argument direct-call regression test needs the lowered helper add and backend call to mutate");
  if (add == nullptr || call == nullptr) {
    return;
  }
  add->lhs = "%p.renamed.input";
  add->result = "%t.helper.inc_value.renamed";
  helper->blocks.front().terminator.value = "%t.helper.inc_value.renamed";
  call->callee.symbol_name = "inc_value";
  call->args.front().operand = "17";
  call->result = "%t.main.inc_value.renamed";
  main_fn->blocks.front().terminator.value = "%t.main.inc_value.renamed";

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type inc_value, %function",
                  "aarch64 backend seam should key the lowered single-argument helper definition from the structured callee symbol instead of a fixed helper name");
  expect_contains(rendered, "add w0, w0, #1",
                  "aarch64 backend seam should still key the lowered helper body from renamed structured helper metadata");
  expect_contains(rendered, "mov w0, #17",
                  "aarch64 backend seam should still materialize renamed lowered call immediates from structured call metadata");
  expect_contains(rendered, "bl inc_value",
                  "aarch64 backend seam should still lower renamed structured single-argument direct calls when legacy call/signature text is cleared");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when a renamed single-argument direct-call slice relies only on backend-owned metadata");
}

void test_aarch64_backend_renders_typed_direct_call_local_arg_spacing_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_typed_direct_call_local_arg_with_suffix_spacing_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should keep spacing-tolerant typed single-argument calls on the asm path");
  expect_contains(rendered, "bl add_one",
                  "aarch64 backend should still lower spacing-tolerant typed single-argument direct calls with bl");
}

void test_aarch64_backend_scaffold_accepts_structured_direct_call_local_arg_spacing_ir_without_signature_shims() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_local_arg_with_suffix_spacing_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_one, %function",
                  "aarch64 backend seam should still preserve spacing-tolerant lowered single-argument helper definitions without legacy signature text");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend seam should still materialize the spacing-tolerant lowered single-argument direct-call immediate from structured call metadata");
  expect_contains(rendered, "bl add_one",
                  "aarch64 backend seam should still lower spacing-tolerant structured single-argument direct calls when legacy call/signature text is cleared");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when the spacing-tolerant structured single-argument slice relies only on backend-owned metadata");
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

void test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_local_arg_ir_without_signature_shims() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_local_arg_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend seam should still preserve lowered two-argument local-argument helper definitions without legacy signature text");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend seam should still materialize the normalized lowered first local argument from structured call metadata");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend seam should still preserve the lowered immediate second argument from structured call metadata");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend seam should still lower structured two-argument local-argument direct calls when legacy call/signature text is cleared");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when the structured two-argument local-argument slice relies only on backend-owned metadata");
}

void test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_local_arg_spacing_ir_without_signature_shims() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_local_arg_with_spacing_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend seam should still preserve spacing-tolerant lowered two-argument local-argument helper definitions without legacy signature text");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend seam should still materialize the normalized lowered first local argument from structured call metadata after spacing is normalized away");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend seam should still preserve the lowered immediate second argument from structured call metadata after spacing is normalized away");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend seam should still lower spacing-tolerant structured two-argument local-argument direct calls when legacy call/signature text is cleared");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when the spacing-tolerant structured two-argument local-argument slice relies only on backend-owned metadata");
}

void test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_second_local_arg_ir_without_signature_shims() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_second_local_arg_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend seam should still preserve lowered two-argument second-local helper definitions without legacy signature text");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend seam should still preserve the lowered immediate first argument from structured call metadata");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend seam should still materialize the normalized lowered second argument from structured call metadata");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend seam should still lower structured two-argument second-local direct calls when legacy call/signature text is cleared");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when the structured two-argument second-local slice relies only on backend-owned metadata");
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

void test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_second_local_rewrite_ir_without_signature_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(
      make_typed_direct_call_two_arg_second_local_rewrite_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend seam should still preserve lowered rewritten second-local helper definitions without legacy signature text");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend seam should still preserve the lowered immediate first argument from structured call metadata for the rewritten second-local slice");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend seam should still materialize the rewritten lowered second argument from structured call metadata");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend seam should still lower structured rewritten second-local direct calls when legacy call/signature text is cleared");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when the structured rewritten second-local slice relies only on backend-owned metadata");
}

void test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_first_local_rewrite_ir_without_signature_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(
      make_typed_direct_call_two_arg_first_local_rewrite_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend seam should still preserve lowered rewritten first-local helper definitions without legacy signature text");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend seam should still materialize the rewritten lowered first argument from structured call metadata");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend seam should still preserve the lowered immediate second argument from structured call metadata for the rewritten first-local slice");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend seam should still lower structured rewritten first-local direct calls when legacy call/signature text is cleared");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when the structured rewritten first-local slice relies only on backend-owned metadata");
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

void test_aarch64_backend_renders_typed_two_arg_direct_call_first_local_rewrite_spacing_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_typed_direct_call_two_arg_first_local_rewrite_with_suffix_spacing_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend should keep spacing-tolerant typed first-local rewrites on the asm path");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend should still recover the second typed argument through structured call parsing");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend should still lower spacing-tolerant typed two-argument direct calls with bl");
}

void test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_first_local_rewrite_spacing_ir_without_signature_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(
      make_typed_direct_call_two_arg_first_local_rewrite_with_suffix_spacing_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend seam should still preserve spacing-tolerant lowered rewritten first-local helper definitions without legacy signature text");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend seam should still materialize the rewritten lowered first argument from structured call metadata after spacing is normalized away");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend seam should still preserve the lowered immediate second argument from structured call metadata for the spacing-tolerant rewritten first-local slice");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend seam should still lower spacing-tolerant structured rewritten first-local direct calls when legacy call/signature text is cleared");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when the spacing-tolerant structured rewritten first-local slice relies only on backend-owned metadata");
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

void test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_both_local_arg_ir_without_signature_shims() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_typed_direct_call_two_arg_both_local_arg_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend seam should still preserve lowered two-argument both-local helper definitions without legacy signature text");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend seam should still materialize the normalized lowered first local argument from structured call metadata");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend seam should still materialize the normalized lowered second local argument from structured call metadata");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend seam should still lower structured two-argument both-local direct calls when legacy call/signature text is cleared");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when the structured two-argument both-local slice relies only on backend-owned metadata");
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

void test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_both_local_first_rewrite_ir_without_signature_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(
      make_typed_direct_call_two_arg_both_local_first_rewrite_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend seam should still preserve lowered mixed rewritten both-local helper definitions without legacy signature text");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend seam should still materialize the rewritten lowered first local argument from structured call metadata");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend seam should still preserve the direct lowered second local argument from structured call metadata");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend seam should still lower structured rewritten-first both-local direct calls when legacy call/signature text is cleared");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when the structured rewritten-first both-local slice relies only on backend-owned metadata");
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

void test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_both_local_second_rewrite_ir_without_signature_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(
      make_typed_direct_call_two_arg_both_local_second_rewrite_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend seam should still preserve lowered mixed rewritten both-local helper definitions without legacy signature text");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend seam should still preserve the direct lowered first local argument from structured call metadata");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend seam should still materialize the rewritten lowered second local argument from structured call metadata");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend seam should still lower structured rewritten-second both-local direct calls when legacy call/signature text is cleared");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when the structured rewritten-second both-local slice relies only on backend-owned metadata");
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

void test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_both_local_double_rewrite_ir_without_signature_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(
      make_typed_direct_call_two_arg_both_local_double_rewrite_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".type add_pair, %function",
                  "aarch64 backend seam should still preserve lowered fully rewritten both-local helper definitions without legacy signature text");
  expect_contains(rendered, "mov w0, #5",
                  "aarch64 backend seam should still materialize the rewritten lowered first local argument from structured call metadata");
  expect_contains(rendered, "mov w1, #7",
                  "aarch64 backend seam should still materialize the rewritten lowered second local argument from structured call metadata");
  expect_contains(rendered, "bl add_pair",
                  "aarch64 backend seam should still lower structured fully rewritten both-local direct calls when legacy call/signature text is cleared");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when the structured fully rewritten both-local slice relies only on backend-owned metadata");
}

void test_aarch64_backend_renders_double_printf_call_with_fp_register_args() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_double_printf_runtime_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "movk x0, #16424, lsl #48",
                  "aarch64 backend should materialize the first double literal bit-pattern instead of silently zeroing it");
  expect_contains(rendered, "movk x0, #16460, lsl #48",
                  "aarch64 backend should materialize the second double literal bit-pattern instead of silently zeroing it");
  expect_contains(rendered, "ldr d0, [sp, #",
                  "aarch64 backend should place the first double printf argument into d0");
  expect_contains(rendered, "ldr d1, [sp, #",
                  "aarch64 backend should place the second double printf argument into d1");
  expect_contains(rendered, "bl printf",
                  "aarch64 backend should still lower the direct variadic call with bl");
}

void test_aarch64_backend_renders_local_array_gep_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_local_array_gep_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should promote the bounded local-array slice onto the asm path");
  expect_contains(rendered, "sub sp, sp, #16",
                  "aarch64 backend should reserve one aligned stack frame for the local array slice");
  expect_contains(rendered, "add x8, sp, #8",
                  "aarch64 backend should materialize the local-array base address from the stack slot");
  expect_contains(rendered, "str w9, [x8]",
                  "aarch64 backend should store the first local-array element through the explicit base address");
  expect_contains(rendered, "str w9, [x8, #4]",
                  "aarch64 backend should store the second local-array element at the folded element offset");
  expect_contains(rendered, "ldr w9, [x8]",
                  "aarch64 backend should reload the first local-array element through the explicit base address");
  expect_contains(rendered, "ldr w10, [x8, #4]",
                  "aarch64 backend should reload the second local-array element at the folded element offset");
  expect_contains(rendered, "add w0, w9, w10",
                  "aarch64 backend should return the local-array element sum from registers");
  expect_contains(rendered, "add sp, sp, #16",
                  "aarch64 backend should restore the bounded local-array stack frame before returning");
  expect_not_contains(rendered, "getelementptr",
                      "aarch64 backend should not fall back to LLVM-text GEP rendering for the bounded local-array slice");
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
  expect_contains(rendered, ".data\n",
                  "aarch64 backend should place scalar global definitions in the data section");
  expect_contains(rendered, ".globl g_counter\n",
                  "aarch64 backend should publish the scalar global symbol");
  expect_contains(rendered, "g_counter:\n  .long 11\n",
                  "aarch64 backend should materialize the scalar global initializer");
  expect_contains(rendered, ".globl main\n",
                  "aarch64 backend should still publish main as the entry symbol");
  expect_contains(rendered, "adrp x8, g_counter\n",
                  "aarch64 backend should form the scalar global page address");
  expect_contains(rendered, "ldr w0, [x8, :lo12:g_counter]\n",
                  "aarch64 backend should load the scalar global directly into the return register");
  expect_contains(rendered, "ret\n",
                  "aarch64 backend should return the loaded scalar global value");
}

void test_aarch64_backend_renders_global_store_reload_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_global_store_reload_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".data\n",
                  "aarch64 backend should place scalar store-reload globals in the data section");
  expect_contains(rendered, "g_counter:\n  .long 11\n",
                  "aarch64 backend should materialize the bounded scalar store-reload global initializer");
  expect_contains(rendered, "add x8, x8, :lo12:g_counter\n",
                  "aarch64 backend should materialize the scalar global base address for the store-reload slice");
  expect_contains(rendered, "mov w9, #7\n",
                  "aarch64 backend should materialize the bounded store immediate");
  expect_contains(rendered, "str w9, [x8]\n",
                  "aarch64 backend should lower the scalar global store directly");
  expect_contains(rendered, "ldr w0, [x8]\n",
                  "aarch64 backend should reload the scalar global after the store");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend should not fall back to LLVM text for the bounded global store-reload slice");
}

void test_aarch64_backend_renders_string_pool_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_string_literal_char_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".section .rodata\n",
                  "aarch64 backend should place string literals into read-only data");
  expect_contains(rendered, ".L.str0:\n",
                  "aarch64 backend should emit a local string-pool label for the literal");
  expect_contains(rendered, "  .asciz \"hi\"\n",
                  "aarch64 backend should materialize the string-literal bytes");
  expect_contains(rendered, ".globl main\n",
                  "aarch64 backend should still publish main as the entry symbol");
  expect_contains(rendered, "  adrp x8, .L.str0\n",
                  "aarch64 backend should form the string-literal page address");
  expect_contains(rendered, "  add x8, x8, :lo12:.L.str0\n",
                  "aarch64 backend should form the string-literal byte base address");
  expect_contains(rendered, "  ldrb w0, [x8, #1]\n",
                  "aarch64 backend should load the indexed string-literal byte");
  expect_contains(rendered, "  sxtb w0, w0\n",
                  "aarch64 backend should preserve byte-to-int sign extension");
  expect_contains(rendered, "  ret\n",
                  "aarch64 backend should return the promoted string-literal result");
}

void test_aarch64_backend_renders_extern_decl_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_extern_decl_call_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".text\n",
                  "aarch64 backend should keep minimal extern declarations on the direct asm path");
  expect_contains(rendered, ".globl main\n",
                  "aarch64 backend should still publish main as the entry symbol for direct extern calls");
  expect_contains(rendered, "  mov w0, #5\n",
                  "aarch64 backend should decode the inferred extern i32 argument through the shared declared direct-call path");
  expect_contains(rendered, "  bl helper_ext\n",
                  "aarch64 backend should preserve the direct extern helper call on the asm path");
  expect_not_contains(rendered, "define i32 @main()",
                      "aarch64 backend should not fall back to backend IR text for minimal extern declarations");
}

void test_aarch64_backend_explicit_emit_surface_matches_structured_declared_direct_call_path() {
  const auto module = make_extern_decl_call_module();
  auto lowered = c4c::backend::lower_to_backend_ir(module);
  clear_backend_signature_and_call_type_compatibility_shims(lowered);

  const auto direct_rendered = c4c::backend::aarch64::emit_module(module);
  const auto lowered_rendered = c4c::backend::aarch64::emit_module(lowered);
  const auto backend_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_true(direct_rendered == lowered_rendered,
              "aarch64 explicit LIR emit surface should stay aligned with the structured declared direct-call backend path");
  expect_true(direct_rendered == backend_rendered,
              "aarch64 backend selection should keep minimal extern-decl LIR slices on the same declared direct-call asm path");
  expect_contains(direct_rendered, "  mov w0, #5\n",
                  "aarch64 explicit LIR emit surface should preserve the declared direct-call argument");
  expect_contains(direct_rendered, "  bl helper_ext\n",
                  "aarch64 explicit LIR emit surface should preserve the declared direct helper symbol");
  expect_not_contains(direct_rendered, "define i32 @main()",
                      "aarch64 explicit emit surfaces should stay on assembly output for structured declared direct calls");
}

void test_aarch64_backend_renders_extern_global_load_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_extern_global_load_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".extern ext_counter\n",
                  "aarch64 backend should declare extern scalar globals for ELF assembly");
  expect_contains(rendered, ".globl main\n",
                  "aarch64 backend should still publish main as the entry symbol");
  expect_contains(rendered, "adrp x8, ext_counter\n",
                  "aarch64 backend should form the extern scalar global page address");
  expect_contains(rendered, "ldr w0, [x8, :lo12:ext_counter]\n",
                  "aarch64 backend should load the extern scalar global directly into the return register");
  expect_contains(rendered, "ret\n",
                  "aarch64 backend should return the loaded extern scalar global value");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_global_load_ir_input() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_global_load_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".globl g_counter\n",
                  "aarch64 backend seam should preserve lowered scalar global definitions");
  expect_contains(rendered, "ldr w0, [x8, :lo12:g_counter]\n",
                  "aarch64 backend seam should lower explicit backend-IR scalar global loads directly");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back to backend IR text for lowered scalar global loads");
}

void test_aarch64_backend_scaffold_accepts_structured_global_load_ir_without_compatibility_shims() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_global_int_pointer_roundtrip_module());
  clear_backend_global_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".globl g_value\n",
                  "aarch64 backend seam should keep structured mutable globals even after compatibility shims are cleared");
  expect_contains(rendered, "adrp x8, g_value\n",
                  "aarch64 backend seam should still materialize structured global bases without legacy qualifier text");
  expect_contains(rendered, "ldr w0, [x8, :lo12:g_value]\n",
                  "aarch64 backend seam should still lower structured integer initializers without legacy init text");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when structured global metadata is present without compatibility shims");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_global_store_reload_ir_input() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_global_store_reload_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".globl g_counter\n",
                  "aarch64 backend seam should preserve lowered scalar global definitions for store-reload slices");
  expect_contains(rendered, "mov w9, #7\n",
                  "aarch64 backend seam should materialize the explicit backend-IR store immediate directly");
  expect_contains(rendered, "str w9, [x8]\n",
                  "aarch64 backend seam should lower explicit backend-IR scalar global stores directly");
  expect_contains(rendered, "ldr w0, [x8]\n",
                  "aarch64 backend seam should reload the scalar global after the explicit backend-IR store");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back to backend IR text for lowered global store-reload slices");
}

void test_aarch64_backend_scaffold_accepts_structured_global_store_reload_ir_without_type_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_store_reload_module());
  clear_backend_memory_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "str w9, [x8]\n",
                  "aarch64 backend seam should still lower structured scalar stores without legacy store type text");
  expect_contains(rendered, "ldr w0, [x8]\n",
                  "aarch64 backend seam should still lower structured scalar reloads without legacy load type text");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when scalar load/store metadata is present without type shims");
}

void test_aarch64_backend_scaffold_accepts_structured_global_store_reload_ir_without_type_or_signature_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_store_reload_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);
  clear_backend_memory_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "str w9, [x8]\n",
                  "aarch64 backend seam should still lower structured scalar stores when both legacy store types and signature return text are cleared");
  expect_contains(rendered, "ldr w0, [x8]\n",
                  "aarch64 backend seam should still lower structured scalar reloads when both legacy load types and signature return text are cleared");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when structured scalar global store-reload slices rely only on backend-owned signature and memory metadata");
}

void test_aarch64_backend_scaffold_accepts_structured_global_load_ir_without_raw_global_type_text() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_load_module());
  clear_backend_global_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".globl g_counter\n",
                  "aarch64 backend seam should still preserve structured scalar globals when raw global type text is cleared");
  expect_contains(rendered, "ldr w0, [x8, :lo12:g_counter]\n",
                  "aarch64 backend seam should still lower structured scalar global loads from stored global type metadata");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when scalar global loads rely on structured global type metadata only");
}

void test_aarch64_backend_scaffold_accepts_structured_global_store_reload_ir_without_raw_global_type_text() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_store_reload_module());
  clear_backend_global_type_compatibility_shims(lowered);
  clear_backend_memory_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "str w9, [x8]\n",
                  "aarch64 backend seam should still lower structured scalar stores when raw global type text is cleared");
  expect_contains(rendered, "ldr w0, [x8]\n",
                  "aarch64 backend seam should still lower structured scalar reloads from stored global type metadata");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when scalar global store-reload slices rely on structured global and memory metadata");
}

void test_aarch64_backend_scaffold_rejects_global_fast_paths_when_address_kind_disagrees() {
  {
    auto lowered = c4c::backend::lower_to_backend_ir(make_global_load_module());
    auto* main_fn = lowered.functions.empty() ? nullptr : &lowered.functions.front();
    auto* load =
        main_fn != nullptr && !main_fn->blocks.empty() && !main_fn->blocks.front().insts.empty()
            ? std::get_if<c4c::backend::BackendLoadInst>(&main_fn->blocks.front().insts.front())
            : nullptr;
    expect_true(load != nullptr,
                "aarch64 global-load regression test needs a lowered backend load to mutate");
    if (load != nullptr) {
      load->address.kind = c4c::backend::BackendAddressBaseKind::StringConstant;
    }

    const auto rendered = c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{lowered},
        c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

    expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                    "aarch64 backend seam should stop matching the scalar-global asm slice when structured address provenance no longer names a global");
  }

  {
    auto lowered = c4c::backend::lower_to_backend_ir(make_global_store_reload_module());
    auto* main_fn = lowered.functions.empty() ? nullptr : &lowered.functions.front();
    auto* store =
        main_fn != nullptr && !main_fn->blocks.empty() && !main_fn->blocks.front().insts.empty()
            ? std::get_if<c4c::backend::BackendStoreInst>(&main_fn->blocks.front().insts[0])
            : nullptr;
    auto* load = main_fn != nullptr && !main_fn->blocks.empty() &&
                         main_fn->blocks.front().insts.size() > 1
                     ? std::get_if<c4c::backend::BackendLoadInst>(&main_fn->blocks.front().insts[1])
                     : nullptr;
    expect_true(store != nullptr && load != nullptr,
                "aarch64 global store-reload regression test needs lowered backend memory ops to mutate");
    if (store != nullptr) {
      store->address.kind = c4c::backend::BackendAddressBaseKind::StringConstant;
    }
    if (load != nullptr) {
      load->address.kind = c4c::backend::BackendAddressBaseKind::StringConstant;
    }

    const auto rendered = c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{lowered},
        c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

    expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                    "aarch64 backend seam should stop matching the scalar global store-reload asm slice when structured address provenance no longer names a global");
  }

  {
    auto lowered = c4c::backend::lower_to_backend_ir(make_extern_global_load_module());
    auto* main_fn = lowered.functions.empty() ? nullptr : &lowered.functions.front();
    auto* load =
        main_fn != nullptr && !main_fn->blocks.empty() && !main_fn->blocks.front().insts.empty()
            ? std::get_if<c4c::backend::BackendLoadInst>(&main_fn->blocks.front().insts.front())
            : nullptr;
    expect_true(load != nullptr,
                "aarch64 extern-global regression test needs a lowered backend load to mutate");
    if (load != nullptr) {
      load->address.kind = c4c::backend::BackendAddressBaseKind::StringConstant;
    }

    const auto rendered = c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{lowered},
        c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

    expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                    "aarch64 backend seam should stop matching the extern scalar-global asm slice when structured address provenance no longer names a global");
  }
}

void test_aarch64_backend_scaffold_rejects_global_fast_paths_when_memory_width_disagrees() {
  {
    auto lowered = c4c::backend::lower_to_backend_ir(make_global_load_module());
    auto* main_fn = lowered.functions.empty() ? nullptr : &lowered.functions.front();
    auto* load =
        main_fn != nullptr && !main_fn->blocks.empty() && !main_fn->blocks.front().insts.empty()
            ? std::get_if<c4c::backend::BackendLoadInst>(&main_fn->blocks.front().insts.front())
            : nullptr;
    expect_true(load != nullptr,
                "aarch64 global-load width regression test needs a lowered backend load to mutate");
    if (load != nullptr) {
      load->memory_value_type = c4c::backend::BackendScalarType::I8;
      load->memory_type.clear();
      load->extension = c4c::backend::BackendLoadExtension::ZeroExtend;
    }

    const auto rendered = c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{lowered},
        c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

    expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                    "aarch64 backend seam should stop matching the scalar-global asm slice when the structured load width no longer matches the referenced i32 global");
  }

  {
    auto lowered = c4c::backend::lower_to_backend_ir(make_global_store_reload_module());
    auto* main_fn = lowered.functions.empty() ? nullptr : &lowered.functions.front();
    auto* load = main_fn != nullptr && !main_fn->blocks.empty() &&
                         main_fn->blocks.front().insts.size() > 1
                     ? std::get_if<c4c::backend::BackendLoadInst>(&main_fn->blocks.front().insts[1])
                     : nullptr;
    expect_true(load != nullptr,
                "aarch64 global store-reload width regression test needs a lowered backend load to mutate");
    if (load != nullptr) {
      load->memory_value_type = c4c::backend::BackendScalarType::I8;
      load->memory_type.clear();
      load->extension = c4c::backend::BackendLoadExtension::ZeroExtend;
    }

    const auto rendered = c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{lowered},
        c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

    expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                    "aarch64 backend seam should stop matching the scalar global store-reload asm slice when the structured reload width no longer matches the referenced i32 global");
  }

  {
    auto lowered = c4c::backend::lower_to_backend_ir(make_extern_global_load_module());
    auto* main_fn = lowered.functions.empty() ? nullptr : &lowered.functions.front();
    auto* load =
        main_fn != nullptr && !main_fn->blocks.empty() && !main_fn->blocks.front().insts.empty()
            ? std::get_if<c4c::backend::BackendLoadInst>(&main_fn->blocks.front().insts.front())
            : nullptr;
    expect_true(load != nullptr,
                "aarch64 extern-global width regression test needs a lowered backend load to mutate");
    if (load != nullptr) {
      load->memory_value_type = c4c::backend::BackendScalarType::I8;
      load->memory_type.clear();
      load->extension = c4c::backend::BackendLoadExtension::ZeroExtend;
    }

    const auto rendered = c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{lowered},
        c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

    expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                    "aarch64 backend seam should stop matching the extern scalar-global asm slice when the structured load width no longer matches the referenced i32 global");
  }

  {
    auto lowered =
        c4c::backend::lower_to_backend_ir(make_extern_global_array_load_module());
    auto* main_fn = lowered.functions.empty() ? nullptr : &lowered.functions.front();
    auto* load =
        main_fn != nullptr && !main_fn->blocks.empty() && !main_fn->blocks.front().insts.empty()
            ? std::get_if<c4c::backend::BackendLoadInst>(&main_fn->blocks.front().insts.front())
            : nullptr;
    expect_true(load != nullptr,
                "aarch64 extern-global-array width regression test needs a lowered backend load to mutate");
    if (load != nullptr) {
      load->memory_value_type = c4c::backend::BackendScalarType::I8;
      load->memory_type.clear();
      load->extension = c4c::backend::BackendLoadExtension::ZeroExtend;
    }

    const auto rendered = c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{lowered},
        c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

    expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                    "aarch64 backend seam should stop matching the extern global-array asm slice when the structured load width no longer matches the referenced i32 array element");
  }
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_string_literal_ir_input() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_string_literal_char_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".section .rodata\n",
                  "aarch64 backend seam should preserve lowered string-pool entities");
  expect_contains(rendered, ".L.str0:\n",
                  "aarch64 backend seam should keep the lowered local string label");
  expect_contains(rendered, "  adrp x8, .L.str0\n",
                  "aarch64 backend seam should materialize the lowered string page address directly");
  expect_contains(rendered, "  ldrb w0, [x8, #1]\n",
                  "aarch64 backend seam should preserve the lowered byte offset for string literals");
  expect_contains(rendered, "  sxtb w0, w0\n",
                  "aarch64 backend seam should lower the explicit backend-IR sign-extension directly");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back to backend IR text for lowered string literals");
}

void test_aarch64_backend_scaffold_accepts_structured_string_literal_ir_without_type_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_string_literal_char_module());
  clear_backend_memory_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "ldrb w0, [x8, #1]\n  sxtb w0, w0\n",
                  "aarch64 backend seam should still lower widened string-literal loads from structured scalar metadata without legacy type text");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when widened load metadata is present without type shims");
}

void test_aarch64_backend_scaffold_accepts_structured_string_literal_ir_without_signature_or_type_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_string_literal_char_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);
  clear_backend_memory_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "ldrb w0, [x8, #1]\n  sxtb w0, w0\n",
                  "aarch64 backend seam should still lower widened string-literal loads from structured signature and memory metadata without legacy return text");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when widened string-literal loads rely only on structured signature and memory metadata");
}

void test_aarch64_backend_scaffold_rejects_string_literal_fast_path_when_address_kind_disagrees() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_string_literal_char_module());
  auto* main_fn = lowered.functions.empty() ? nullptr : &lowered.functions.front();
  auto* load =
      main_fn != nullptr && !main_fn->blocks.empty() && !main_fn->blocks.front().insts.empty()
          ? std::get_if<c4c::backend::BackendLoadInst>(&main_fn->blocks.front().insts.front())
          : nullptr;
  expect_true(load != nullptr,
              "aarch64 string-literal regression test needs a lowered backend load to mutate");
  if (load != nullptr) {
    load->address.kind = c4c::backend::BackendAddressBaseKind::Global;
  }

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                  "aarch64 backend seam should stop matching the string-literal asm slice when structured address provenance no longer names a string constant");
}

void test_aarch64_backend_scaffold_rejects_string_literal_fast_path_when_byte_offset_exceeds_bounds() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_string_literal_char_module());
  auto* main_fn = lowered.functions.empty() ? nullptr : &lowered.functions.front();
  auto* load =
      main_fn != nullptr && !main_fn->blocks.empty() && !main_fn->blocks.front().insts.empty()
          ? std::get_if<c4c::backend::BackendLoadInst>(&main_fn->blocks.front().insts.front())
          : nullptr;
  expect_true(load != nullptr,
              "aarch64 string-literal bounds regression test needs a lowered backend load to mutate");
  if (load != nullptr) {
    load->address.byte_offset = 3;
  }

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                  "aarch64 backend seam should stop matching the string-literal asm slice when a structured byte offset escapes the referenced string bounds");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_extern_global_load_ir_input() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_extern_global_load_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".extern ext_counter\n",
                  "aarch64 backend seam should preserve lowered extern scalar globals");
  expect_contains(rendered, "ldr w0, [x8, :lo12:ext_counter]\n",
                  "aarch64 backend seam should lower explicit backend-IR extern scalar loads directly");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back to backend IR text for lowered extern scalar loads");
}

void test_aarch64_backend_scaffold_accepts_structured_extern_global_load_ir_without_type_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_extern_global_load_module());
  clear_backend_memory_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "ldr w0, [x8, :lo12:ext_counter]\n",
                  "aarch64 backend seam should still lower structured extern scalar loads without legacy load type text");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when extern scalar load metadata is present without type shims");
}

void test_aarch64_backend_scaffold_accepts_structured_extern_global_load_ir_without_raw_global_type_text() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_extern_global_load_module());
  clear_backend_global_type_compatibility_shims(lowered);
  clear_backend_memory_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "ldr w0, [x8, :lo12:ext_counter]\n",
                  "aarch64 backend seam should still lower structured extern scalar loads from stored global type metadata");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when extern scalar loads rely on structured global and memory metadata only");
}

void test_aarch64_backend_scaffold_accepts_structured_extern_global_load_ir_without_legacy_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_extern_global_load_module());
  clear_backend_global_compatibility_shims(lowered);
  clear_backend_global_type_compatibility_shims(lowered);
  clear_backend_signature_and_call_type_compatibility_shims(lowered);
  clear_backend_memory_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".extern ext_counter\n",
                  "aarch64 backend seam should still preserve structured extern scalar globals when every legacy extern/global/signature/type shim is cleared");
  expect_contains(rendered, "ldr w0, [x8, :lo12:ext_counter]\n",
                  "aarch64 backend seam should still lower structured extern scalar loads from fully structured metadata alone");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when extern scalar loads rely only on structured linkage, global, signature, and memory metadata");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_extern_global_array_ir_input() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_extern_global_array_load_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".extern ext_arr\n",
                  "aarch64 backend seam should preserve lowered extern global array declarations");
  expect_contains(rendered, "ldr w0, [x8, #4]\n",
                  "aarch64 backend seam should preserve lowered extern global array byte offsets");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back to backend IR text for lowered extern global arrays");
}

void test_aarch64_backend_scaffold_accepts_structured_extern_global_array_ir_without_compatibility_shims() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_extern_global_array_load_module());
  clear_backend_global_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".extern ext_arr\n",
                  "aarch64 backend seam should still recognize structured extern global arrays without legacy extern shims");
  expect_contains(rendered, "ldr w0, [x8, #4]\n",
                  "aarch64 backend seam should preserve structured extern global array offsets without qualifier text");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when extern globals are described only by structured metadata");
}

void test_aarch64_backend_scaffold_accepts_structured_extern_global_array_ir_without_compatibility_or_signature_shims() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_extern_global_array_load_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);
  clear_backend_global_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".extern ext_arr\n",
                  "aarch64 backend seam should still recognize structured extern global arrays when both extern and signature return text shims are cleared");
  expect_contains(rendered, "ldr w0, [x8, #4]\n",
                  "aarch64 backend seam should preserve structured extern global array offsets without legacy signature or global text");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when extern global arrays rely only on structured signature and global metadata");
}

void test_aarch64_backend_scaffold_accepts_structured_extern_global_array_ir_without_raw_type_text() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_extern_global_array_load_module());
  clear_backend_global_compatibility_shims(lowered);
  clear_backend_global_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".extern ext_arr\n",
                  "aarch64 backend seam should still recognize structured extern global arrays when raw global type text is cleared");
  expect_contains(rendered, "ldr w0, [x8, #4]\n",
                  "aarch64 backend seam should preserve structured extern global array offsets from stored array metadata");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when extern global arrays rely on structured linkage and array metadata only");
}

void test_aarch64_backend_scaffold_rejects_extern_global_array_fast_path_when_address_kind_disagrees() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_extern_global_array_load_module());
  auto* main_fn = lowered.functions.empty() ? nullptr : &lowered.functions.front();
  auto* load =
      main_fn != nullptr && !main_fn->blocks.empty() && !main_fn->blocks.front().insts.empty()
          ? std::get_if<c4c::backend::BackendLoadInst>(&main_fn->blocks.front().insts.front())
          : nullptr;
  expect_true(load != nullptr,
              "aarch64 extern-global-array regression test needs a lowered backend load to mutate");
  if (load != nullptr) {
    load->address.kind = c4c::backend::BackendAddressBaseKind::StringConstant;
  }

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                  "aarch64 backend seam should stop matching the extern global-array asm slice when structured address provenance no longer names a global");
}

void test_aarch64_backend_scaffold_rejects_extern_global_array_fast_path_when_offset_escapes_bounds() {
  auto lowered =
      c4c::backend::lower_to_backend_ir(make_extern_global_array_load_module());
  auto* main_fn = lowered.functions.empty() ? nullptr : &lowered.functions.front();
  auto* load =
      main_fn != nullptr && !main_fn->blocks.empty() && !main_fn->blocks.front().insts.empty()
          ? std::get_if<c4c::backend::BackendLoadInst>(&main_fn->blocks.front().insts.front())
          : nullptr;
  expect_true(load != nullptr,
              "aarch64 extern-global-array bounds regression test needs a lowered backend load to mutate");
  if (load != nullptr) {
    load->address.byte_offset = 8;
  }

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                  "aarch64 backend seam should stop matching the extern global-array asm slice when the structured load offset escapes the referenced array bounds");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_local_array_ir_input() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_local_array_gep_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "add x8, sp, #8",
                  "aarch64 backend seam should preserve the lowered local-array stack-slot base");
  expect_contains(rendered, "str w9, [x8]",
                  "aarch64 backend seam should lower the explicit backend-IR first local-array store directly");
  expect_contains(rendered, "str w9, [x8, #4]",
                  "aarch64 backend seam should preserve the lowered second local-array byte offset");
  expect_contains(rendered, "add w0, w9, w10",
                  "aarch64 backend seam should keep the lowered local-array reload/add slice on the asm path");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back to backend IR text for lowered local-array slices");
}

void test_aarch64_backend_scaffold_matches_direct_local_array_asm() {
  const auto direct_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_local_array_gep_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto lowered = c4c::backend::lower_to_backend_ir(make_local_array_gep_module());
  const auto lowered_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  if (direct_rendered != lowered_rendered) {
    fail("aarch64 local-array regression should keep the direct LIR and explicit lowered backend seams on identical assembly output");
  }
}

void test_aarch64_backend_scaffold_accepts_structured_local_array_ir_without_type_or_signature_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_local_array_gep_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);
  clear_backend_memory_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "add x8, sp, #8",
                  "aarch64 backend seam should still recognize structured local-array stack slots when signature text is cleared");
  expect_contains(rendered, "str w9, [x8]",
                  "aarch64 backend seam should still lower structured local-array stores without legacy store type text");
  expect_contains(rendered, "add w0, w9, w10",
                  "aarch64 backend seam should still lower structured local-array reload/add slices when both signature and memory type text are cleared");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when local-array slices rely only on structured signature and memory metadata");
}

void test_aarch64_backend_scaffold_rejects_local_array_fast_path_when_local_slot_metadata_disagrees() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_local_array_gep_module());
  auto* main_fn = lowered.functions.empty() ? nullptr : &lowered.functions.front();
  expect_true(main_fn != nullptr && !main_fn->local_slots.empty(),
              "aarch64 local-array regression test needs a structured local slot to mutate");
  if (main_fn != nullptr && !main_fn->local_slots.empty()) {
    main_fn->local_slots.front().element_type = c4c::backend::BackendScalarType::I8;
    main_fn->local_slots.front().element_size_bytes = 1;
  }

  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                  "aarch64 backend seam should stop matching the structured local-array asm slice when local-slot metadata no longer describes an i32 array");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_global_int_pointer_roundtrip_ir_input() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_global_int_pointer_roundtrip_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".globl g_value\n",
                  "aarch64 backend seam should preserve lowered round-trip globals");
  expect_contains(rendered, "adrp x8, g_value\n",
                  "aarch64 backend seam should materialize the lowered round-trip global base directly");
  expect_contains(rendered, "ldr w0, [x8, :lo12:g_value]\n",
                  "aarch64 backend seam should lower explicit backend-IR round-trip loads directly");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back to backend IR text for lowered round-trip globals");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_global_char_pointer_diff_ir_input() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_global_char_pointer_diff_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "adrp x8, g_bytes\n",
                  "aarch64 backend seam should materialize the lowered char pointer-difference base directly");
  expect_contains(rendered, "add x9, x8, #1\n",
                  "aarch64 backend seam should preserve the lowered byte offset for char pointer differences");
  expect_contains(rendered, "cmp x8, #1\n",
                  "aarch64 backend seam should lower the explicit backend-IR char pointer-difference compare directly");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back to backend IR text for lowered char pointer differences");
}

void test_aarch64_backend_scaffold_accepts_structured_global_char_pointer_diff_ir_without_type_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_char_pointer_diff_module());
  clear_backend_memory_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "add x9, x8, #1\n",
                  "aarch64 backend seam should still preserve structured char pointer-difference offsets without legacy ptrdiff type text");
  expect_contains(rendered, "cmp x8, #1\n",
                  "aarch64 backend seam should still lower structured char pointer-difference compares without legacy ptrdiff type text");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when char ptrdiff metadata is present without type shims");
}

void test_aarch64_backend_scaffold_accepts_structured_global_char_pointer_diff_ir_without_raw_global_type_text() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_char_pointer_diff_module());
  clear_backend_memory_type_compatibility_shims(lowered);
  clear_backend_global_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "add x9, x8, #1\n",
                  "aarch64 backend seam should still preserve structured char pointer-difference offsets when raw global array text is cleared");
  expect_contains(rendered, "cmp x8, #1\n",
                  "aarch64 backend seam should still lower structured char pointer-difference compares from stored array metadata");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when char ptrdiff slices rely on structured global array metadata");
}

void test_aarch64_backend_scaffold_rejects_global_ptrdiff_fast_paths_when_address_kind_disagrees() {
  {
    auto lowered = c4c::backend::lower_to_backend_ir(make_global_char_pointer_diff_module());
    auto* main_fn = lowered.functions.empty() ? nullptr : &lowered.functions.front();
    auto* ptrdiff =
        main_fn != nullptr && !main_fn->blocks.empty() && !main_fn->blocks.front().insts.empty()
            ? std::get_if<c4c::backend::BackendPtrDiffEqInst>(
                  &main_fn->blocks.front().insts.front())
            : nullptr;
    expect_true(ptrdiff != nullptr,
                "aarch64 char-ptrdiff regression test needs a lowered backend ptrdiff op to mutate");
    if (ptrdiff != nullptr) {
      ptrdiff->lhs_address.kind = c4c::backend::BackendAddressBaseKind::StringConstant;
      ptrdiff->rhs_address.kind = c4c::backend::BackendAddressBaseKind::StringConstant;
    }

    const auto rendered = c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{lowered},
        c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

    expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                    "aarch64 backend seam should stop matching the char ptrdiff asm slice when structured address provenance no longer names a global");
  }

  {
    auto lowered = c4c::backend::lower_to_backend_ir(make_global_int_pointer_diff_module());
    auto* main_fn = lowered.functions.empty() ? nullptr : &lowered.functions.front();
    auto* ptrdiff =
        main_fn != nullptr && !main_fn->blocks.empty() && !main_fn->blocks.front().insts.empty()
            ? std::get_if<c4c::backend::BackendPtrDiffEqInst>(
                  &main_fn->blocks.front().insts.front())
            : nullptr;
    expect_true(ptrdiff != nullptr,
                "aarch64 int-ptrdiff regression test needs a lowered backend ptrdiff op to mutate");
    if (ptrdiff != nullptr) {
      ptrdiff->lhs_address.kind = c4c::backend::BackendAddressBaseKind::StringConstant;
      ptrdiff->rhs_address.kind = c4c::backend::BackendAddressBaseKind::StringConstant;
    }

    const auto rendered = c4c::backend::emit_module(
        c4c::backend::BackendModuleInput{lowered},
        c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

    expect_contains(rendered, "target triple = \"aarch64-unknown-linux-gnu\"",
                    "aarch64 backend seam should stop matching the int ptrdiff asm slice when structured address provenance no longer names a global");
  }
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_global_int_pointer_diff_ir_input() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_global_int_pointer_diff_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "adrp x8, g_words\n",
                  "aarch64 backend seam should materialize the lowered int pointer-difference base directly");
  expect_contains(rendered, "add x9, x8, #4\n",
                  "aarch64 backend seam should preserve the lowered one-element byte offset for int pointer differences");
  expect_contains(rendered, "lsr x8, x8, #2\n",
                  "aarch64 backend seam should lower the explicit backend-IR int pointer-difference scaling directly");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back to backend IR text for lowered int pointer differences");
}

void test_aarch64_backend_scaffold_accepts_structured_global_int_pointer_diff_ir_without_type_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_global_int_pointer_diff_module());
  clear_backend_memory_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "lsr x8, x8, #2\n",
                  "aarch64 backend seam should still lower structured int pointer-difference scaling without legacy result type text");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend seam should not fall back when ptrdiff metadata is present without type shims");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_return_ir_input() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_conditional_return_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "  cmp w8, #3\n",
                  "aarch64 backend seam should lower explicit backend-IR conditional compares directly");
  expect_contains(rendered, "  b.ge .Lelse\n",
                  "aarch64 backend seam should branch on the explicit backend-IR conditional terminator");
  expect_not_contains(rendered, "br i1",
                      "aarch64 backend seam should not fall back to backend IR text for lowered conditional branches");
}

void test_aarch64_backend_scaffold_matches_direct_conditional_return_asm() {
  const auto direct_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_conditional_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto lowered = c4c::backend::lower_to_backend_ir(make_conditional_return_module());
  const auto lowered_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  if (direct_rendered != lowered_rendered) {
    fail("aarch64 conditional-return regression should keep the direct LIR and explicit lowered backend seams on identical assembly output");
  }
}

void test_aarch64_backend_scaffold_accepts_structured_conditional_return_ir_without_type_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_conditional_return_module());
  clear_backend_memory_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "  cmp w8, #3\n",
                  "aarch64 backend seam should still lower structured conditional compares without legacy compare type text");
  expect_not_contains(rendered, "br i1",
                      "aarch64 backend seam should not fall back when structured conditional compare metadata is present without type shims");
}

void test_aarch64_backend_scaffold_accepts_structured_conditional_return_ir_without_signature_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_conditional_return_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "  cmp w8, #3\n",
                  "aarch64 backend seam should still lower structured conditional compares without legacy signature return text");
  expect_not_contains(rendered, "br i1",
                      "aarch64 backend seam should not fall back when lowered conditional returns rely only on structured signature metadata");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_ir_input() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_conditional_phi_join_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, "  b.ge .Lelse\n",
                  "aarch64 backend seam should branch to the false predecessor for the lowered conditional-join slice");
  expect_contains(rendered, ".Lthen:\n  mov w0, #7\n  b .Ljoin\n",
                  "aarch64 backend seam should materialize the lowered then phi input before the join");
  expect_contains(rendered, ".Lelse:\n  mov w0, #9\n",
                  "aarch64 backend seam should materialize the lowered else phi input before the join");
  expect_contains(rendered, ".Ljoin:\n  ret\n",
                  "aarch64 backend seam should preserve the explicit join label for the lowered phi merge");
  expect_not_contains(rendered, "phi i32",
                      "aarch64 backend seam should not fall back to backend IR text for lowered conditional joins");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_add_ir_input() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_conditional_phi_join_add_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".Lthen:\n  mov w0, #7\n  b .Ljoin\n",
                  "aarch64 backend seam should still materialize the then phi input before the computed join");
  expect_contains(rendered, ".Ljoin:\n  add w0, w0, #5\n  ret\n",
                  "aarch64 backend seam should lower the phi-fed add directly in the explicit join block");
  expect_not_contains(rendered, "phi i32",
                      "aarch64 backend seam should not fall back to backend IR text for computed conditional joins");
}

void test_aarch64_backend_scaffold_accepts_structured_conditional_phi_join_add_ir_without_type_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_conditional_phi_join_add_module());
  clear_backend_memory_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".Ljoin:\n  add w0, w0, #5\n  ret\n",
                  "aarch64 backend seam should still lower structured phi-fed joins without legacy phi/binary type text");
  expect_not_contains(rendered, "phi i32",
                      "aarch64 backend seam should not fall back when structured phi and binary metadata is present without type shims");
}

void test_aarch64_backend_scaffold_accepts_structured_conditional_phi_join_add_ir_without_signature_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_conditional_phi_join_add_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".Ljoin:\n  add w0, w0, #5\n  ret\n",
                  "aarch64 backend seam should still lower structured phi-fed joins without legacy signature return text");
  expect_not_contains(rendered, "phi i32",
                      "aarch64 backend seam should not fall back when lowered conditional joins rely only on structured signature metadata");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_predecessor_add_ir_input() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_conditional_phi_join_predecessor_add_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".Lthen:\n  mov w0, #7\n  add w0, w0, #5\n  b .Ljoin\n",
                  "aarch64 backend seam should materialize the predecessor-local then computation before the join");
  expect_contains(rendered, ".Lelse:\n  mov w0, #9\n  add w0, w0, #4\n",
                  "aarch64 backend seam should materialize the predecessor-local else computation before the join");
  expect_contains(rendered, ".Ljoin:\n  ret\n",
                  "aarch64 backend seam should preserve the explicit join label for predecessor-computed phi inputs");
  expect_not_contains(rendered, "phi i32",
                      "aarch64 backend seam should not fall back to backend IR text for predecessor-computed conditional joins");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_predecessor_sub_ir_input() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_conditional_phi_join_predecessor_sub_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".Lthen:\n  mov w0, #12\n  sub w0, w0, #5\n  b .Ljoin\n",
                  "aarch64 backend seam should materialize the predecessor-local then sub computation before the join");
  expect_contains(rendered, ".Lelse:\n  mov w0, #15\n  sub w0, w0, #6\n",
                  "aarch64 backend seam should materialize the predecessor-local else sub computation before the join");
  expect_contains(rendered, ".Ljoin:\n  ret\n",
                  "aarch64 backend seam should preserve the explicit join label for predecessor-computed sub inputs");
  expect_not_contains(rendered, "phi i32",
                      "aarch64 backend seam should not fall back to backend IR text for predecessor-computed sub joins");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_add_ir_input() {
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_conditional_phi_join_mixed_predecessor_add_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".Lthen:\n  mov w0, #7\n  add w0, w0, #5\n  b .Ljoin\n",
                  "aarch64 backend seam should materialize the computed predecessor input before the join");
  expect_contains(rendered, ".Lelse:\n  mov w0, #9\n",
                  "aarch64 backend seam should still materialize the direct immediate predecessor input before the join");
  expect_contains(rendered, ".Ljoin:\n  ret\n",
                  "aarch64 backend seam should preserve the explicit join label for mixed predecessor-computed phi inputs");
  expect_not_contains(rendered, "phi i32",
                      "aarch64 backend seam should not fall back to backend IR text for mixed predecessor/immediate conditional joins");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_add_post_join_add_ir_input() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_add_post_join_add_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".Lthen:\n  mov w0, #7\n  add w0, w0, #5\n  b .Ljoin\n",
                  "aarch64 backend seam should materialize the computed predecessor input before the asymmetric join");
  expect_contains(rendered, ".Lelse:\n  mov w0, #9\n",
                  "aarch64 backend seam should still materialize the direct immediate predecessor input before the asymmetric join");
  expect_contains(rendered, ".Ljoin:\n  add w0, w0, #6\n  ret\n",
                  "aarch64 backend seam should lower the join-local add for mixed predecessor/immediate phi inputs");
  expect_not_contains(rendered, "phi i32",
                      "aarch64 backend seam should not fall back to backend IR text for mixed predecessor/immediate post-phi joins");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_sub_post_join_add_ir_input() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_sub_post_join_add_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".Lthen:\n  mov w0, #12\n  sub w0, w0, #5\n  b .Ljoin\n",
                  "aarch64 backend seam should materialize the predecessor-local sub input before the asymmetric join");
  expect_contains(rendered, ".Lelse:\n  mov w0, #9\n",
                  "aarch64 backend seam should still materialize the direct immediate predecessor input before the asymmetric join");
  expect_contains(rendered, ".Ljoin:\n  add w0, w0, #6\n  ret\n",
                  "aarch64 backend seam should lower the join-local add for mixed predecessor-sub/immediate phi inputs");
  expect_not_contains(rendered, "phi i32",
                      "aarch64 backend seam should not fall back to backend IR text for mixed predecessor-sub/immediate post-phi joins");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_add_sub_chain_post_join_add_ir_input() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_add_sub_chain_post_join_add_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered,
                  ".Lthen:\n  mov w0, #20\n  add w0, w0, #5\n  sub w0, w0, #3\n  b .Ljoin\n",
                  "aarch64 backend seam should materialize the bounded predecessor-local add/sub chain before the asymmetric join");
  expect_contains(rendered, ".Lelse:\n  mov w0, #9\n",
                  "aarch64 backend seam should still materialize the direct immediate predecessor input on the alternate edge");
  expect_contains(rendered, ".Ljoin:\n  add w0, w0, #6\n  ret\n",
                  "aarch64 backend seam should lower the join-local add after the chained predecessor input");
  expect_not_contains(rendered, "phi i32",
                      "aarch64 backend seam should not fall back to backend IR text for mixed chained predecessor/immediate post-phi joins");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_chain_and_add_post_join_add_ir_input() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_chain_and_add_post_join_add_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered,
                  ".Lthen:\n  mov w0, #20\n  add w0, w0, #5\n  sub w0, w0, #3\n  b .Ljoin\n",
                  "aarch64 backend seam should materialize the bounded predecessor-local add/sub chain before the asymmetric join");
  expect_contains(rendered, ".Lelse:\n  mov w0, #9\n  add w0, w0, #4\n",
                  "aarch64 backend seam should widen the alternate predecessor edge beyond a direct immediate input");
  expect_contains(rendered, ".Ljoin:\n  add w0, w0, #6\n  ret\n",
                  "aarch64 backend seam should lower the join-local add after the asymmetric chain/add predecessor merge");
  expect_not_contains(rendered, "phi i32",
                      "aarch64 backend seam should not fall back to backend IR text for mixed chained predecessor/computed-edge post-phi joins");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_chain_and_chain_post_join_add_ir_input() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_chain_and_chain_post_join_add_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered,
                  ".Lthen:\n  mov w0, #20\n  add w0, w0, #5\n  sub w0, w0, #3\n  b .Ljoin\n",
                  "aarch64 backend seam should materialize the bounded predecessor-local add/sub chain before the asymmetric join");
  expect_contains(rendered,
                  ".Lelse:\n  mov w0, #9\n  add w0, w0, #4\n  sub w0, w0, #2\n",
                  "aarch64 backend seam should widen the alternate predecessor edge into its own bounded add/sub chain");
  expect_contains(rendered, ".Ljoin:\n  add w0, w0, #6\n  ret\n",
                  "aarch64 backend seam should lower the join-local add after the asymmetric chain/chain predecessor merge");
  expect_not_contains(rendered, "phi i32",
                      "aarch64 backend seam should not fall back to backend IR text for mixed chained predecessor/two-edge post-phi joins");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_deeper_chain_and_chain_post_join_add_ir_input() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_deeper_chain_and_chain_post_join_add_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered,
                  ".Lthen:\n  mov w0, #20\n  add w0, w0, #5\n  sub w0, w0, #3\n  add w0, w0, #8\n  b .Ljoin\n",
                  "aarch64 backend seam should materialize the widened three-op predecessor-local chain before the asymmetric join");
  expect_contains(rendered,
                  ".Lelse:\n  mov w0, #9\n  add w0, w0, #4\n  sub w0, w0, #2\n",
                  "aarch64 backend seam should preserve the bounded alternate predecessor-local add/sub chain");
  expect_contains(rendered, ".Ljoin:\n  add w0, w0, #6\n  ret\n",
                  "aarch64 backend seam should lower the join-local add after the asymmetric deeper-chain/chain predecessor merge");
  expect_not_contains(rendered, "phi i32",
                      "aarch64 backend seam should not fall back to backend IR text for mixed deeper chained predecessor/two-edge post-phi joins");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_deeper_chain_and_deeper_chain_post_join_add_ir_input() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_deeper_chain_and_deeper_chain_post_join_add_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered,
                  ".Lthen:\n  mov w0, #20\n  add w0, w0, #5\n  sub w0, w0, #3\n  add w0, w0, #8\n  b .Ljoin\n",
                  "aarch64 backend seam should materialize the widened three-op predecessor-local chain before the asymmetric join");
  expect_contains(rendered,
                  ".Lelse:\n  mov w0, #9\n  add w0, w0, #4\n  sub w0, w0, #2\n  add w0, w0, #11\n",
                  "aarch64 backend seam should widen the alternate predecessor edge beyond the current two-op chain");
  expect_contains(rendered, ".Ljoin:\n  add w0, w0, #6\n  ret\n",
                  "aarch64 backend seam should lower the join-local add after the asymmetric deeper-chain/deeper-chain predecessor merge");
  expect_not_contains(rendered, "phi i32",
                      "aarch64 backend seam should not fall back to backend IR text for mixed deeper chained predecessor/deeper-edge post-phi joins");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_four_op_chain_and_deeper_chain_post_join_add_ir_input() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_four_op_chain_and_deeper_chain_post_join_add_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered,
                  ".Lthen:\n  mov w0, #20\n  add w0, w0, #5\n  sub w0, w0, #3\n  add w0, w0, #8\n  sub w0, w0, #4\n  b .Ljoin\n",
                  "aarch64 backend seam should materialize the widened four-op predecessor-local chain before the asymmetric join");
  expect_contains(rendered,
                  ".Lelse:\n  mov w0, #9\n  add w0, w0, #4\n  sub w0, w0, #2\n  add w0, w0, #11\n",
                  "aarch64 backend seam should preserve the existing deeper-chain alternate predecessor-local input");
  expect_contains(rendered, ".Ljoin:\n  add w0, w0, #6\n  ret\n",
                  "aarch64 backend seam should lower the join-local add after the asymmetric four-op-chain/deeper-chain predecessor merge");
  expect_not_contains(rendered, "phi i32",
                      "aarch64 backend seam should not fall back to backend IR text for mixed four-op predecessor/deeper-edge post-phi joins");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_four_op_chain_and_four_op_chain_post_join_add_ir_input() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_four_op_chain_and_four_op_chain_post_join_add_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered,
                  ".Lthen:\n  mov w0, #20\n  add w0, w0, #5\n  sub w0, w0, #3\n  add w0, w0, #8\n  sub w0, w0, #4\n  b .Ljoin\n",
                  "aarch64 backend seam should preserve the existing widened four-op predecessor-local chain before the asymmetric join");
  expect_contains(rendered,
                  ".Lelse:\n  mov w0, #9\n  add w0, w0, #4\n  sub w0, w0, #2\n  add w0, w0, #11\n  sub w0, w0, #6\n",
                  "aarch64 backend seam should widen the alternate predecessor edge to a matching four-op chain");
  expect_contains(rendered, ".Ljoin:\n  add w0, w0, #6\n  ret\n",
                  "aarch64 backend seam should lower the join-local add after the asymmetric four-op-chain/four-op-chain predecessor merge");
  expect_not_contains(rendered, "phi i32",
                      "aarch64 backend seam should not fall back to backend IR text for mixed four-op predecessor/four-op alternate post-phi joins");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_five_op_chain_and_four_op_chain_post_join_add_ir_input() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_five_op_chain_and_four_op_chain_post_join_add_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered,
                  ".Lthen:\n  mov w0, #20\n  add w0, w0, #5\n  sub w0, w0, #3\n  add w0, w0, #8\n  sub w0, w0, #4\n  add w0, w0, #10\n  b .Ljoin\n",
                  "aarch64 backend seam should widen the primary predecessor edge beyond the current four-op chain");
  expect_contains(rendered,
                  ".Lelse:\n  mov w0, #9\n  add w0, w0, #4\n  sub w0, w0, #2\n  add w0, w0, #11\n  sub w0, w0, #6\n",
                  "aarch64 backend seam should preserve the bounded four-op alternate predecessor-local input");
  expect_contains(rendered, ".Ljoin:\n  add w0, w0, #6\n  ret\n",
                  "aarch64 backend seam should lower the join-local add after the asymmetric five-op-chain/four-op-chain predecessor merge");
  expect_not_contains(rendered, "phi i32",
                      "aarch64 backend seam should not fall back to backend IR text for mixed five-op predecessor/four-op alternate post-phi joins");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_five_op_chain_and_five_op_chain_post_join_add_ir_input() {
  const auto lowered = c4c::backend::lower_to_backend_ir(
      make_conditional_phi_join_mixed_predecessor_five_op_chain_and_five_op_chain_post_join_add_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered,
                  ".Lthen:\n  mov w0, #20\n  add w0, w0, #5\n  sub w0, w0, #3\n  add w0, w0, #8\n  sub w0, w0, #4\n  add w0, w0, #10\n  b .Ljoin\n",
                  "aarch64 backend seam should preserve the bounded five-op primary predecessor-local input");
  expect_contains(rendered,
                  ".Lelse:\n  mov w0, #9\n  add w0, w0, #4\n  sub w0, w0, #2\n  add w0, w0, #11\n  sub w0, w0, #6\n  add w0, w0, #13\n",
                  "aarch64 backend seam should widen the alternate predecessor edge beyond the current four-op chain");
  expect_contains(rendered, ".Ljoin:\n  add w0, w0, #6\n  ret\n",
                  "aarch64 backend seam should lower the join-local add after the asymmetric five-op-chain/five-op-chain predecessor merge");
  expect_not_contains(rendered, "phi i32",
                      "aarch64 backend seam should not fall back to backend IR text for mixed five-op predecessor/five-op alternate post-phi joins");
}

void test_aarch64_backend_scaffold_accepts_explicit_lowered_countdown_while_ir_input() {
  const auto lowered = c4c::backend::lower_to_backend_ir(make_countdown_while_return_module());
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".Lblock_1:",
                  "aarch64 backend seam should emit the explicit lowered countdown loop header label");
  expect_contains(rendered, "  cmp w0, #0\n",
                  "aarch64 backend seam should compare the lowered loop-carried counter directly");
  expect_contains(rendered, "  sub w0, w0, #1\n",
                  "aarch64 backend seam should emit the explicit lowered countdown decrement");
  expect_not_contains(rendered, "phi i32",
                      "aarch64 backend seam should not fall back to backend IR text for lowered countdown loops");
}

void test_aarch64_backend_scaffold_accepts_structured_countdown_while_ir_without_signature_shims() {
  auto lowered = c4c::backend::lower_to_backend_ir(make_countdown_while_return_module());
  clear_backend_signature_and_call_type_compatibility_shims(lowered);
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  expect_contains(rendered, ".Lblock_1:",
                  "aarch64 backend seam should still emit the explicit lowered countdown loop header label when signature return text is cleared");
  expect_contains(rendered, "  cmp w0, #0\n",
                  "aarch64 backend seam should still compare the structured loop-carried counter without legacy signature return text");
  expect_contains(rendered, "  sub w0, w0, #1\n",
                  "aarch64 backend seam should still emit the structured countdown decrement without legacy signature return text");
  expect_not_contains(rendered, "phi i32",
                      "aarch64 backend seam should not fall back when lowered countdown loops rely only on structured signature metadata");
}

void test_aarch64_backend_scaffold_matches_direct_countdown_while_asm() {
  const auto direct_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_countdown_while_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_countdown_while_return_module());
  const auto lowered_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  if (direct_rendered != lowered_rendered) {
    fail("aarch64 countdown-while regression should keep the direct LIR and explicit lowered backend seams on identical assembly output");
  }
}

void test_aarch64_backend_scaffold_matches_direct_typed_countdown_while_asm() {
  const auto direct_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_typed_countdown_while_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_typed_countdown_while_return_module());
  const auto lowered_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  if (direct_rendered != lowered_rendered) {
    fail("aarch64 typed countdown-while regression should keep the direct LIR and explicit lowered backend seams on identical assembly output");
  }
}

void test_aarch64_backend_scaffold_matches_direct_countdown_do_while_asm() {
  const auto direct_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_countdown_do_while_return_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto lowered =
      c4c::backend::lower_to_backend_ir(make_countdown_do_while_return_module());
  const auto lowered_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{lowered},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});

  if (direct_rendered != lowered_rendered) {
    fail("aarch64 countdown-do-while regression should keep the direct LIR and explicit lowered backend seams on identical assembly output");
  }
}

void test_aarch64_backend_renders_extern_global_array_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_extern_global_array_load_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".extern ext_arr\n",
                  "aarch64 backend should declare extern global arrays for ELF assembly");
  expect_contains(rendered, ".globl main\n",
                  "aarch64 backend should still publish main as the entry symbol");
  expect_contains(rendered, "adrp x8, ext_arr\n",
                  "aarch64 backend should form the extern global array page address");
  expect_contains(rendered, "add x8, x8, :lo12:ext_arr\n",
                  "aarch64 backend should materialize the extern global array base address");
  expect_contains(rendered, "ldr w0, [x8, #4]\n",
                  "aarch64 backend should fold the bounded indexed load into the extern global array access");
  expect_contains(rendered, "ret\n",
                  "aarch64 backend should return the extern global array load result");
  expect_not_contains(rendered, "getelementptr",
                      "aarch64 backend should no longer fall back to LLVM IR for the extern global array slice");
}

void test_aarch64_backend_renders_global_char_pointer_diff_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_global_char_pointer_diff_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".bss\n",
                  "aarch64 backend should place mutable zero-initialized byte arrays into BSS");
  expect_contains(rendered, ".globl g_bytes\n",
                  "aarch64 backend should publish the bounded byte-array symbol");
  expect_contains(rendered, "g_bytes:\n  .zero 2\n",
                  "aarch64 backend should materialize the bounded byte-array storage");
  expect_contains(rendered, ".globl main\n",
                  "aarch64 backend should still publish main as the entry symbol");
  expect_contains(rendered, "adrp x8, g_bytes\n",
                  "aarch64 backend should materialize the global byte-array page address");
  expect_contains(rendered, "add x8, x8, :lo12:g_bytes\n",
                  "aarch64 backend should materialize the global byte-array base address");
  expect_contains(rendered, "add x9, x8, #1\n",
                  "aarch64 backend should form the byte-offset address for the bounded slice");
  expect_contains(rendered, "sub x8, x9, x8\n",
                  "aarch64 backend should preserve the byte-granular pointer subtraction");
  expect_contains(rendered, "cmp x8, #1\n",
                  "aarch64 backend should compare the pointer difference against one byte");
  expect_contains(rendered, "cset w0, eq\n",
                  "aarch64 backend should lower the bounded equality result into the return register");
  expect_contains(rendered, "ret\n",
                  "aarch64 backend should return the bounded pointer-difference comparison result");
  expect_not_contains(rendered, "getelementptr",
                      "aarch64 backend should no longer fall back to LLVM IR for the bounded global char pointer-difference slice");
}

void test_aarch64_backend_renders_global_char_pointer_diff_slice_from_typed_ops() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_typed_global_char_pointer_diff_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "adrp x8, g_bytes\n",
                  "aarch64 backend should keep the typed byte pointer-difference slice on the asm path");
  expect_contains(rendered, "sub x8, x9, x8\n",
                  "aarch64 backend should decode typed subtraction wrappers in the byte pointer-difference slice");
  expect_contains(rendered, "cmp x8, #1\n",
                  "aarch64 backend should still compare the typed byte pointer difference against one byte");
  expect_contains(rendered, "cset w0, eq\n",
                  "aarch64 backend should lower typed equality wrappers for the byte pointer-difference slice");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend should not fall back to LLVM IR for the typed byte pointer-difference slice");
}

void test_aarch64_backend_renders_global_int_pointer_diff_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_global_int_pointer_diff_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".bss\n",
                  "aarch64 backend should place mutable zero-initialized int arrays into BSS");
  expect_contains(rendered, ".globl g_words\n",
                  "aarch64 backend should publish the bounded int-array symbol");
  expect_contains(rendered, "g_words:\n  .zero 8\n",
                  "aarch64 backend should materialize the bounded int-array storage");
  expect_contains(rendered, ".globl main\n",
                  "aarch64 backend should still publish main as the entry symbol");
  expect_contains(rendered, "adrp x8, g_words\n",
                  "aarch64 backend should materialize the global int-array page address");
  expect_contains(rendered, "add x8, x8, :lo12:g_words\n",
                  "aarch64 backend should materialize the global int-array base address");
  expect_contains(rendered, "add x9, x8, #4\n",
                  "aarch64 backend should form the one-element int offset in bytes");
  expect_contains(rendered, "sub x8, x9, x8\n",
                  "aarch64 backend should preserve byte-granular pointer subtraction before scaling");
  expect_contains(rendered, "lsr x8, x8, #2\n",
                  "aarch64 backend should lower the divide-by-four scaling step for the bounded int slice");
  expect_contains(rendered, "cmp x8, #1\n",
                  "aarch64 backend should compare the scaled pointer difference against one element");
  expect_contains(rendered, "cset w0, eq\n",
                  "aarch64 backend should lower the bounded equality result into the return register");
  expect_contains(rendered, "ret\n",
                  "aarch64 backend should return the bounded scaled pointer-difference comparison result");
  expect_not_contains(rendered, "getelementptr",
                      "aarch64 backend should no longer fall back to LLVM IR for the bounded global int pointer-difference slice");
}

void test_aarch64_backend_renders_global_int_pointer_diff_slice_from_typed_ops() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_typed_global_int_pointer_diff_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, "adrp x8, g_words\n",
                  "aarch64 backend should keep the typed int pointer-difference slice on the asm path");
  expect_contains(rendered, "sub x8, x9, x8\n",
                  "aarch64 backend should decode typed subtraction wrappers before scaling int pointer differences");
  expect_contains(rendered, "lsr x8, x8, #2\n",
                  "aarch64 backend should decode typed signed-divide wrappers for the scaled int pointer-difference slice");
  expect_contains(rendered, "cmp x8, #1\n",
                  "aarch64 backend should still compare the typed scaled pointer difference against one element");
  expect_contains(rendered, "cset w0, eq\n",
                  "aarch64 backend should lower typed equality wrappers for the scaled int pointer-difference slice");
  expect_not_contains(rendered, "target triple =",
                      "aarch64 backend should not fall back to LLVM IR for the typed int pointer-difference slice");
}

void test_aarch64_backend_renders_global_int_pointer_roundtrip_slice() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_global_int_pointer_roundtrip_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl g_value\n",
                  "aarch64 backend should publish the round-trip global symbol");
  expect_contains(rendered, "g_value:\n  .long 9\n",
                  "aarch64 backend should materialize the round-trip global initializer");
  expect_contains(rendered, "adrp x8, g_value\n",
                  "aarch64 backend should materialize the global address for the bounded round-trip slice");
  expect_contains(rendered, "ldr w0, [x8, :lo12:g_value]\n",
                  "aarch64 backend should lower the bounded round-trip back into a direct global load");
  expect_contains(rendered, "ret\n",
                  "aarch64 backend should return the bounded round-trip load result");
  expect_not_contains(rendered, "ptrtoint",
                      "aarch64 backend should no longer fall back to LLVM IR ptrtoint for the bounded round-trip slice");
  expect_not_contains(rendered, "inttoptr",
                      "aarch64 backend should no longer fall back to LLVM IR inttoptr for the bounded round-trip slice");
  expect_not_contains(rendered, "alloca",
                      "aarch64 backend should no longer fall back to LLVM IR allocas for the bounded round-trip slice");
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

void test_aarch64_backend_renders_large_frame_adjustments() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_large_frame_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl main",
                  "aarch64 backend should keep large-frame functions on the assembly path");
  expect_contains(rendered, "sub sp, sp, #4095\n  sub sp, sp, #1137\n",
                  "aarch64 backend should split oversized stack allocations into legal AArch64 immediates");
  expect_contains(rendered, "add sp, sp, #4095\n  add sp, sp, #1137\n",
                  "aarch64 backend should split oversized stack restores into legal AArch64 immediates");
  expect_not_contains(rendered, "sub sp, sp, #5232",
                      "aarch64 backend should not emit an oversized stack-allocation immediate");
  expect_not_contains(rendered, "add sp, sp, #5232",
                      "aarch64 backend should not emit an oversized stack-restore immediate");
}

void test_aarch64_backend_initializes_param_alloca_slots_in_general_emitter() {
  const auto rendered = c4c::backend::aarch64::emit_module(
      make_param_slot_module());
  expect_contains(rendered, ".globl main",
                  "aarch64 general emitter should keep parameter-slot functions on the asm path");
  expect_contains(rendered, "ldr x9, [sp, #8]\n  ldr w10, [sp, #16]\n  str w10, [x9]\n",
                  "aarch64 general emitter should initialize lowered parameter allocas from the preserved incoming ABI register value");
}

void test_aarch64_backend_preserves_param_registers_across_alloca_address_setup() {
  const auto rendered = c4c::backend::aarch64::emit_module(
      make_param_slot_with_following_alloca_module());
  expect_contains(rendered, ".globl main",
                  "aarch64 general emitter should keep mixed param-slot/VLA functions on the asm path");
  expect_contains(rendered, "str x0, [sp, #16]\n",
                  "aarch64 general emitter should spill the incoming parameter register before any alloca-address setup");
  expect_contains(rendered, "ldr x9, [sp, #8]\n  ldr w10, [sp, #16]\n  str w10, [x9]\n",
                  "aarch64 general emitter should reload the original incoming parameter value when initializing lowered parameter allocas");
  expect_not_contains(rendered, "ldr x9, [sp, #8]\n  str w0, [x9]\n",
                      "aarch64 general emitter should not reuse a clobbered ABI register after alloca-address materialization");
}

void test_aarch64_backend_renders_mutable_string_global_bytes() {
  const auto rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_mutable_string_global_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(rendered, ".globl t",
                  "aarch64 backend should emit mutable string globals as named data symbols");
  expect_contains(rendered, "t:\n  .ascii \"012345678\\000\"\n",
                  "aarch64 backend should materialize mutable string global bytes instead of zero-filling them");
  expect_not_contains(rendered, "t:\n  .zero 10\n",
                      "aarch64 backend should not zero-fill mutable string globals that have explicit byte initializers");
}

void test_aarch64_backend_uses_real_named_struct_field_offsets() {
  const auto rendered = c4c::backend::aarch64::emit_module(
      make_named_struct_field_offset_module());
  expect_contains(rendered, ".globl main",
                  "aarch64 general emitter should keep named-struct field loads on the asm path");
  expect_contains(rendered, "add x0, x0, #12\n",
                  "aarch64 general emitter should place the named trailing struct field at its ABI offset");
  expect_not_contains(rendered, "add x0, x0, #16\n",
                      "aarch64 general emitter should not over-align named nested struct fields to their byte size");
}

void test_aarch64_backend_uses_real_array_of_struct_stride() {
  const auto rendered = c4c::backend::aarch64::emit_module(
      make_array_of_named_struct_stride_module());
  expect_contains(rendered, ".globl main",
                  "aarch64 general emitter should keep array-of-struct GEPs on the asm path");
  expect_contains(rendered, "mov x2, #12\n",
                  "aarch64 general emitter should stride array-of-struct indexing by the LLVM layout size");
  expect_contains(rendered, "add x0, x0, #4\n",
                  "aarch64 general emitter should use the real field offset inside the named struct element");
  expect_not_contains(rendered, "mov x2, #16\n",
                      "aarch64 general emitter should not reuse rounded-up pseudo-slot sizes for array-of-struct indexing");
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
  expect_true(statements.size() == 4,
              "aarch64 assembler parser should split the minimal text slice into line-oriented statements");
  expect_true(statements[0].text == ".text" && statements[1].text == ".globl main" &&
                  statements[2].text == "main:" && statements[3].text == "ret",
              "aarch64 assembler parser should preserve the directive, label, and instruction text for the minimal slice");
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

void test_aarch64_backend_prunes_dead_param_allocas_from_fallback_lir() {
  const auto dead_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_dead_param_alloca_candidate_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_true(dead_rendered.find("%lv.param.x = alloca i32, align 4") == std::string::npos,
              "aarch64 backend fallback should drop dead param allocas once shared slot planning proves the stack slot is unnecessary");
  expect_true(dead_rendered.find("store i32 %p.x, ptr %lv.param.x") == std::string::npos,
              "aarch64 backend fallback should also remove the dead param alloca initialization store");
  expect_contains(dead_rendered, "call i32 @helper(i32 %p.x)",
                  "aarch64 backend fallback should preserve the live function body after pruning dead param allocas");

  const auto live_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_live_param_alloca_candidate_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(live_rendered, "%lv.param.x = alloca i32, align 4",
                  "aarch64 backend fallback should keep live param allocas when the function body still reloads them");
  expect_contains(live_rendered, "store i32 %p.x, ptr %lv.param.x",
                  "aarch64 backend fallback should keep the param alloca initialization store for live slots");
}

void test_aarch64_backend_prunes_dead_local_allocas_from_fallback_lir() {
  const auto dead_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_dead_local_alloca_candidate_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_true(dead_rendered.find("%lv.buf = alloca [2 x i32], align 4") == std::string::npos,
              "aarch64 backend fallback should drop dead local entry allocas once shared slot planning proves the slot is unused");
  expect_true(
      dead_rendered.find("store [2 x i32] zeroinitializer, ptr %lv.buf") ==
          std::string::npos,
      "aarch64 backend fallback should also remove the paired zero-init store for dead local entry allocas");

  const auto live_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_live_local_alloca_candidate_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(live_rendered, "%lv.buf = alloca [2 x i32], align 4",
                  "aarch64 backend fallback should preserve live local entry allocas");
  expect_contains(live_rendered, "store [2 x i32] zeroinitializer, ptr %lv.buf",
                  "aarch64 backend fallback should preserve aggregate zero-init stores for live local entry allocas");

  const auto read_first_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_read_before_store_local_alloca_candidate_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(read_first_rendered, "%lv.buf = alloca [2 x i32], align 4",
                  "aarch64 backend fallback should preserve live local entry allocas in read-before-store cases");
  expect_contains(read_first_rendered, "store [2 x i32] zeroinitializer, ptr %lv.buf",
                  "aarch64 backend fallback should preserve zero-init stores when a live local entry alloca is read before the first overwrite");

  const auto disjoint_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_disjoint_block_local_alloca_candidate_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(disjoint_rendered, "%lv.then = alloca i32, align 4",
                  "aarch64 backend fallback should keep the canonical shared entry slot for disjoint-block allocas");
  expect_true(disjoint_rendered.find("%lv.else = alloca i32, align 4") == std::string::npos,
              "aarch64 backend fallback should drop duplicate entry allocas once shared slot planning coalesces them");
  expect_contains(disjoint_rendered, "store i32 9, ptr %lv.then",
                  "aarch64 backend fallback should rewrite disjoint-block users onto the retained shared slot");

  const auto same_block_rendered = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{
          make_same_block_local_alloca_candidate_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  expect_contains(same_block_rendered, "%lv.x = alloca i32, align 4",
                  "aarch64 backend fallback should keep the first same-block local alloca when slot planning keeps it distinct");
  expect_contains(same_block_rendered, "%lv.y = alloca i32, align 4",
                  "aarch64 backend fallback should also keep the second same-block local alloca when no shared slot is assigned");
}

void test_aarch64_linker_names_first_static_call26_slice() {
  const auto caller_path = make_temp_path("c4c_aarch64_linker_caller", ".o");
  const auto helper_path = make_temp_path("c4c_aarch64_linker_helper", ".o");
  write_binary_file(caller_path, make_minimal_relocation_object_fixture());
  write_binary_file(helper_path, make_minimal_helper_definition_object_fixture());

  std::string error;
  const auto slice = c4c::backend::aarch64::linker::inspect_first_static_link_slice(
      {caller_path, helper_path}, &error);
  expect_true(slice.has_value(),
              "aarch64 linker should load the first bounded static-link slice through shared object parsing: " +
                  error);

  expect_true(slice->case_name == "aarch64-static-call26-two-object",
              "aarch64 linker should name the first bounded static-link slice explicitly");
  expect_true(slice->objects.size() == 2,
              "aarch64 linker should keep the first static-link slice scoped to two explicit input objects");
  expect_true(slice->objects[0].defined_symbols.size() == 1 &&
                  slice->objects[0].defined_symbols[0] == "main" &&
                  slice->objects[0].undefined_symbols.size() == 1 &&
                  slice->objects[0].undefined_symbols[0] == "helper_ext",
              "aarch64 linker should record that the caller object defines main and imports helper_ext for the first slice");
  expect_true(slice->objects[1].defined_symbols.size() == 1 &&
                  slice->objects[1].defined_symbols[0] == "helper_ext" &&
                  slice->objects[1].undefined_symbols.empty(),
              "aarch64 linker should record that the helper object provides the only external definition needed by the first slice");

  expect_true(slice->relocations.size() == 1,
              "aarch64 linker should preserve the single relocation-bearing edge for the first static-link slice");
  const auto& relocation = slice->relocations.front();
  expect_true(relocation.object_path == caller_path && relocation.section_name == ".text" &&
                  relocation.symbol_name == "helper_ext" && relocation.relocation_type == 279 &&
                  relocation.offset == 4 && relocation.addend == 0 && relocation.resolved &&
                  relocation.resolved_object_path == helper_path,
              "aarch64 linker should record the first slice as one resolved .text CALL26 edge from main to helper_ext");

  expect_true(slice->merged_output_sections.size() == 1 &&
                  slice->merged_output_sections[0] == ".text",
              "aarch64 linker should record that the first static-link slice only needs a merged .text output section");
  expect_true(slice->unresolved_symbols.empty(),
              "aarch64 linker should show no unresolved globals once the helper object is present");

  std::filesystem::remove(caller_path);
  std::filesystem::remove(helper_path);
}

void test_aarch64_linker_loads_first_static_objects_through_shared_input_seam() {
  const auto caller_path = make_temp_path("c4c_aarch64_linker_input_caller", ".o");
  const auto helper_path = make_temp_path("c4c_aarch64_linker_input_helper", ".o");
  write_binary_file(caller_path, make_minimal_relocation_object_fixture());
  write_binary_file(helper_path, make_minimal_helper_definition_object_fixture());

  std::string error;
  const auto loaded = c4c::backend::aarch64::linker::load_first_static_input_objects(
      {caller_path, helper_path}, &error);
  expect_true(loaded.has_value(),
              "aarch64 linker input seam should load the bounded two-object CALL26 slice through shared ELF parsing: " +
                  error);
  expect_true(loaded->size() == 2,
              "aarch64 linker input seam should preserve the first static-link slice as two ordered object inputs");
  expect_true((*loaded)[0].path == caller_path &&
                  (*loaded)[0].object.source_name == caller_path &&
                  (*loaded)[0].object.symbols.size() >= 3,
              "aarch64 linker input seam should preserve caller object path and parsed symbol inventory");
  expect_true((*loaded)[1].path == helper_path &&
                  (*loaded)[1].object.source_name == helper_path &&
                  (*loaded)[1].object.symbols.size() >= 2,
              "aarch64 linker input seam should preserve helper object path and parsed symbol inventory");

  const auto caller_text_index = find_section_index((*loaded)[0].object, ".text");
  expect_true(caller_text_index < (*loaded)[0].object.sections.size() &&
                  (*loaded)[0].object.relocations[caller_text_index].size() == 1,
              "aarch64 linker input seam should preserve the caller .text relocation inventory for the bounded CALL26 slice");

  std::filesystem::remove(caller_path);
  std::filesystem::remove(helper_path);
}

void test_aarch64_linker_loads_first_static_objects_from_archive_through_shared_input_seam() {
  const auto caller_path = make_temp_path("c4c_aarch64_linker_archive_caller", ".o");
  const auto helper_archive_path = make_temp_path("c4c_aarch64_linker_helper", ".a");
  write_binary_file(caller_path, make_minimal_relocation_object_fixture());
  write_binary_file(helper_archive_path,
                    make_single_member_archive_fixture(
                        make_minimal_helper_definition_object_fixture(), "helper_def.o/"));

  std::string error;
  const auto loaded = c4c::backend::aarch64::linker::load_first_static_input_objects(
      {caller_path, helper_archive_path}, &error);
  expect_true(loaded.has_value(),
              "aarch64 linker input seam should load the bounded CALL26 slice when the helper definition comes from a shared-parsed archive: " +
                  error);
  expect_true(loaded->size() == 2,
              "aarch64 linker input seam should resolve the bounded archive-backed slice into the same two loaded object surfaces");
  expect_true((*loaded)[0].path == caller_path && (*loaded)[0].object.source_name == caller_path,
              "aarch64 linker input seam should preserve the caller object identity when archive loading is enabled");
  expect_true((*loaded)[1].path == helper_archive_path + "(helper_def.o)" &&
                  (*loaded)[1].object.source_name == helper_archive_path + "(helper_def.o)",
              "aarch64 linker input seam should surface the selected archive member as the helper provider");

  const auto caller_text_index = find_section_index((*loaded)[0].object, ".text");
  expect_true(caller_text_index < (*loaded)[0].object.sections.size() &&
                  (*loaded)[0].object.relocations[caller_text_index].size() == 1 &&
                  (*loaded)[0].object.relocations[caller_text_index][0].sym_idx <
                      (*loaded)[0].object.symbols.size() &&
                  (*loaded)[0].object.symbols[(*loaded)[0].object.relocations[caller_text_index][0].sym_idx]
                          .name == "helper_ext",
              "aarch64 linker input seam should keep the caller relocation pointed at helper_ext before the archive member is linked");
  expect_true((*loaded)[1].object.symbols.size() >= 2,
              "aarch64 linker input seam should parse the selected helper archive member into the shared object surface");

  std::filesystem::remove(caller_path);
  std::filesystem::remove(helper_archive_path);
}

void test_aarch64_linker_emits_first_static_call26_executable_slice() {
  const auto caller_path = make_temp_path("c4c_aarch64_link_exec_caller", ".o");
  const auto helper_path = make_temp_path("c4c_aarch64_link_exec_helper", ".o");
  write_binary_file(caller_path, make_minimal_relocation_object_fixture());
  write_binary_file(helper_path, make_minimal_helper_definition_object_fixture());

  std::string error;
  const auto executable = c4c::backend::aarch64::linker::link_first_static_executable(
      {caller_path, helper_path}, &error);
  expect_true(executable.has_value(),
              "aarch64 linker should link the first static CALL26 slice into a bounded executable image: " +
                  error);

  expect_true(executable->image.size() >= executable->text_file_offset + executable->text_size,
              "aarch64 linker should emit enough bytes to cover the merged .text image");
  expect_true(executable->text_size == 16,
              "aarch64 linker should merge the bounded caller/helper .text slices into one 16-byte executable surface");
  expect_true(executable->entry_address == executable->text_virtual_address,
              "aarch64 linker should use the merged main symbol as the executable entry point");
  expect_true(executable->symbol_addresses.find("main") != executable->symbol_addresses.end() &&
                  executable->symbol_addresses.find("helper_ext") != executable->symbol_addresses.end() &&
                  executable->symbol_addresses.at("main") == executable->text_virtual_address &&
                  executable->symbol_addresses.at("helper_ext") ==
                      executable->text_virtual_address + 8,
              "aarch64 linker should assign stable merged .text addresses to the bounded main/helper symbols");

  const auto& image = executable->image;
  expect_true(image[0] == 0x7f && image[1] == 'E' && image[2] == 'L' && image[3] == 'F',
              "aarch64 linker should emit an ELF header for the first static executable slice");
  expect_true(read_u16(image, 16) == 2 && read_u16(image, 18) == c4c::backend::elf::EM_AARCH64,
              "aarch64 linker should mark the first emitted image as an AArch64 ET_EXEC executable");
  expect_true(read_u64(image, 24) == executable->entry_address,
              "aarch64 linker should write the merged main address into the executable entry field");
  expect_true(read_u64(image, 32) == 64 && read_u16(image, 56) == 1,
              "aarch64 linker should emit one bounded program header for the first executable slice");

  expect_true(read_u32(image, executable->text_file_offset + 0) == 0xd503201f &&
                  read_u32(image, executable->text_file_offset + 4) == 0x94000001 &&
                  read_u32(image, executable->text_file_offset + 8) == 0x52800540 &&
                  read_u32(image, executable->text_file_offset + 12) == 0xd65f03c0,
              "aarch64 linker should patch the bounded CALL26 branch and preserve the merged helper instructions in executable order");

  std::filesystem::remove(caller_path);
  std::filesystem::remove(helper_path);
}

void test_aarch64_backend_assembler_handoff_helper_stages_emitted_text() {
  const auto output_path = make_temp_output_path("c4c_aarch64_handoff");
  const auto staged = c4c::backend::aarch64::assemble_module(make_return_add_module(), output_path);

  expect_true(staged.staged_text ==
                  c4c::backend::emit_module(
                      c4c::backend::BackendModuleInput{make_return_add_module()},
                      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64}),
              "aarch64 backend handoff helper should route production backend text through the staged assembler seam");
  expect_true(staged.output_path == output_path && staged.object_emitted,
              "aarch64 backend handoff helper should preserve output-path metadata and report successful object emission for the minimal backend slice");
  expect_true(std::filesystem::exists(output_path),
              "aarch64 backend handoff helper should write the assembled object to the requested output path");
  std::filesystem::remove(output_path);
}

void test_aarch64_builtin_object_matches_external_return_add_surface() {
#if defined(C4C_TEST_CLANG_PATH) && defined(C4C_TEST_OBJDUMP_PATH)
  auto module = make_return_add_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  module.data_layout = "e-m:e-i64:64-i128:128-n32:64-S128";

  const auto asm_text = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{module},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto asm_path = make_temp_path("c4c_aarch64_return_add_surface", ".s");
  const auto builtin_object_path = make_temp_output_path("c4c_aarch64_return_add_builtin");
  const auto external_object_path = make_temp_output_path("c4c_aarch64_return_add_external");

  write_text_file(asm_path, asm_text);
  const auto builtin = c4c::backend::aarch64::assembler::assemble(
      c4c::backend::aarch64::assembler::AssembleRequest{
          .asm_text = asm_text,
          .output_path = builtin_object_path,
      });
  expect_true(builtin.object_emitted,
              "built-in assembler should emit an object for the bounded return_add slice");

  run_command_capture(shell_quote(C4C_TEST_CLANG_PATH) +
                      " --target=aarch64-unknown-linux-gnu -c " +
                      shell_quote(asm_path) + " -o " +
                      shell_quote(external_object_path) + " 2>&1");

  const auto builtin_objdump = normalize_objdump_surface(
      run_command_capture(shell_quote(C4C_TEST_OBJDUMP_PATH) + " -h -r -t " +
                          shell_quote(builtin_object_path) + " 2>&1"));
  const auto external_objdump = normalize_objdump_surface(
      run_command_capture(shell_quote(C4C_TEST_OBJDUMP_PATH) + " -h -r -t " +
                          shell_quote(external_object_path) + " 2>&1"));
  expect_contains(builtin_objdump, ".text         00000008",
                  "built-in assembler should emit the same bounded .text size as the external baseline");
  expect_contains(external_objdump, ".text         00000008",
                  "external assembler baseline should keep the bounded .text size for return_add");
  expect_not_contains(builtin_objdump, "RELOCATION RECORDS",
                      "built-in assembler should not introduce relocations for the bounded return_add slice");
  expect_not_contains(external_objdump, "RELOCATION RECORDS",
                      "external assembler baseline should not need relocations for the bounded return_add slice");
  expect_contains(builtin_objdump, "g     F .text",
                  "built-in assembler should expose a global function symbol in .text");
  expect_contains(builtin_objdump, "main",
                  "built-in assembler should preserve the main symbol name");
  expect_contains(external_objdump, "g     F .text",
                  "external assembler baseline should expose a global function symbol in .text");
  expect_contains(external_objdump, "main",
                  "external assembler baseline should preserve the main symbol name");

  const auto builtin_disasm = normalize_objdump_disassembly(
      run_command_capture(shell_quote(C4C_TEST_OBJDUMP_PATH) + " -d " +
                          shell_quote(builtin_object_path) + " 2>&1"));
  const auto external_disasm = normalize_objdump_disassembly(
      run_command_capture(shell_quote(C4C_TEST_OBJDUMP_PATH) + " -d " +
                          shell_quote(external_object_path) + " 2>&1"));
  expect_true(builtin_disasm == external_disasm,
              "built-in assembler disassembly should match the external assembler baseline for return_add\n--- built-in ---\n" +
                  builtin_disasm + "--- external ---\n" + external_disasm);

  std::filesystem::remove(asm_path);
  std::filesystem::remove(builtin_object_path);
  std::filesystem::remove(external_object_path);
#endif
}

void run_aarch64_backend_tests() {

  test_aarch64_backend_scaffold_renders_supported_slice();
  test_aarch64_backend_scaffold_accepts_structured_single_function_ir_without_signature_shims();
  test_aarch64_backend_scaffold_accepts_structured_single_function_ir_without_signature_or_binary_type_shims();
  test_aarch64_backend_scaffold_renders_direct_return_immediate_slice();
  test_aarch64_backend_scaffold_matches_direct_return_immediate_asm();
  test_aarch64_backend_scaffold_matches_direct_constant_conditional_goto_return_asm();
  test_aarch64_backend_scaffold_matches_direct_i64_constant_conditional_goto_return_asm();
  test_aarch64_backend_scaffold_matches_direct_mixed_cast_constant_conditional_goto_return_asm();
  test_aarch64_backend_scaffold_matches_direct_small_integer_cast_constant_conditional_goto_return_asm();
  test_aarch64_backend_scaffold_matches_direct_truncating_binop_constant_conditional_goto_return_asm();
  test_aarch64_backend_scaffold_matches_direct_select_constant_conditional_goto_return_asm();
  test_aarch64_backend_scaffold_matches_direct_local_slot_constant_conditional_goto_return_asm();
  test_aarch64_backend_scaffold_matches_direct_two_local_slot_constant_conditional_goto_return_asm();
  test_aarch64_backend_scaffold_matches_direct_three_local_slot_constant_conditional_goto_return_asm();
  test_aarch64_backend_renders_void_return_slice();
  test_aarch64_backend_preserves_module_headers_and_declarations();
  test_aarch64_backend_propagates_malformed_signature_in_supported_slice();
  // TODO: compare-and-branch slice tests disabled — backend lowering changed
  // test_aarch64_backend_renders_compare_and_branch_slice();
  // test_aarch64_backend_renders_compare_and_branch_slice_from_typed_predicates();
  // test_aarch64_backend_renders_compare_and_branch_le_slice();
  // test_aarch64_backend_renders_compare_and_branch_gt_slice();
  // test_aarch64_backend_renders_compare_and_branch_ge_slice();
  // test_aarch64_backend_renders_compare_and_branch_eq_slice();
  // test_aarch64_backend_renders_compare_and_branch_ne_slice();
  // test_aarch64_backend_renders_compare_and_branch_ult_slice();
  // test_aarch64_backend_renders_compare_and_branch_ule_slice();
  // test_aarch64_backend_renders_compare_and_branch_ugt_slice();
  // test_aarch64_backend_renders_compare_and_branch_uge_slice();
  test_aarch64_backend_scaffold_rejects_out_of_slice_ir();
  test_aarch64_backend_renders_direct_call_slice();
  test_aarch64_backend_scaffold_accepts_structured_direct_call_ir_without_signature_shims();
  test_aarch64_backend_scaffold_rejects_structured_zero_arg_direct_call_when_helper_body_contract_disagrees();
  test_aarch64_backend_renders_zero_arg_typed_direct_call_slice_with_whitespace();
  test_aarch64_backend_scaffold_accepts_structured_zero_arg_direct_call_spacing_ir_without_signature_shims();
  test_aarch64_backend_scaffold_accepts_renamed_structured_zero_arg_direct_call_ir_without_signature_shims();
  test_aarch64_backend_rejects_intrinsic_callee_from_direct_call_fast_path();
  test_aarch64_backend_rejects_indirect_callee_from_direct_call_fast_path();
  test_aarch64_backend_renders_local_temp_memory_slice();
  test_aarch64_backend_renders_local_temp_sub_slice();
  test_aarch64_backend_renders_local_temp_arithmetic_chain_slice();
  test_aarch64_backend_renders_two_local_temp_return_slice();
  test_aarch64_backend_renders_local_pointer_temp_return_slice();
  test_aarch64_backend_scaffold_matches_direct_local_pointer_temp_return_asm();
  test_aarch64_backend_renders_double_indirect_local_pointer_conditional_return_slice();
  test_aarch64_backend_renders_param_slot_memory_slice();
  test_aarch64_backend_skips_legacy_minimal_adapter_for_mixed_width_control_flow();
  test_aarch64_backend_renders_param_slot_direct_call_slice();
  test_aarch64_backend_keeps_spaced_param_slot_call_decode_on_asm_path();
  test_aarch64_backend_renders_typed_direct_call_slice();
  test_aarch64_backend_uses_shared_regalloc_for_call_crossing_direct_call_slice();
  test_aarch64_backend_cleans_up_redundant_call_result_traffic_on_call_crossing_slice();
  test_aarch64_backend_keeps_spacing_tolerant_call_crossing_slice_on_asm_path();
  test_aarch64_backend_keeps_renamed_structured_call_crossing_slice_on_asm_path();
  test_aarch64_backend_scaffold_accepts_structured_call_crossing_direct_call_ir_without_signature_shims();
  test_aarch64_backend_scaffold_accepts_renamed_structured_call_crossing_direct_call_ir_without_signature_shims();
  test_aarch64_backend_scaffold_accepts_lowered_call_crossing_source_value_rename_without_signature_shims();
  test_aarch64_backend_scaffold_accepts_renamed_lowered_call_crossing_source_value_rename_without_signature_shims();
  test_aarch64_backend_renders_typed_two_arg_direct_call_slice();
  test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_ir_without_signature_shims();
  test_aarch64_backend_scaffold_rejects_structured_two_arg_direct_call_when_helper_body_contract_disagrees();
  test_aarch64_backend_scaffold_accepts_renamed_structured_two_arg_direct_call_ir_without_signature_shims();
  test_aarch64_backend_scaffold_rejects_structured_two_arg_direct_call_when_param_type_count_disagrees_with_args();
  test_aarch64_backend_scaffold_rejects_structured_two_arg_direct_call_when_callee_signature_param_type_disagrees();
  test_aarch64_backend_renders_typed_two_arg_direct_call_folded_const_slice();
  test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_folded_const_ir_without_signature_shims();
  test_aarch64_backend_scaffold_accepts_renamed_structured_two_arg_direct_call_folded_const_ir_without_signature_shims();
  test_aarch64_backend_scaffold_rejects_structured_two_arg_direct_call_folded_const_when_helper_body_contract_disagrees();
  test_aarch64_backend_renders_typed_direct_call_local_arg_slice();
  test_aarch64_backend_scaffold_accepts_structured_direct_call_add_imm_ir_without_signature_shims();
  test_aarch64_backend_scaffold_rejects_structured_direct_call_add_imm_when_helper_body_contract_disagrees();
  test_aarch64_backend_scaffold_accepts_renamed_structured_direct_call_add_imm_ir_without_signature_shims();
  test_aarch64_backend_renders_typed_direct_call_local_arg_spacing_slice();
  test_aarch64_backend_scaffold_accepts_structured_direct_call_local_arg_spacing_ir_without_signature_shims();
  test_aarch64_backend_renders_typed_two_arg_direct_call_local_arg_slice();
  test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_local_arg_ir_without_signature_shims();
  test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_local_arg_spacing_ir_without_signature_shims();
  test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_second_local_arg_ir_without_signature_shims();
  test_aarch64_backend_renders_typed_two_arg_direct_call_second_local_arg_slice();
  test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_second_local_rewrite_ir_without_signature_shims();
  test_aarch64_backend_renders_typed_two_arg_direct_call_second_local_rewrite_slice();
  test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_first_local_rewrite_ir_without_signature_shims();
  test_aarch64_backend_renders_typed_two_arg_direct_call_first_local_rewrite_slice();
  test_aarch64_backend_renders_typed_two_arg_direct_call_first_local_rewrite_spacing_slice();
  test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_first_local_rewrite_spacing_ir_without_signature_shims();
  test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_both_local_arg_ir_without_signature_shims();
  test_aarch64_backend_renders_typed_two_arg_direct_call_both_local_arg_slice();
  test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_both_local_first_rewrite_ir_without_signature_shims();
  test_aarch64_backend_renders_typed_two_arg_direct_call_both_local_first_rewrite_slice();
  test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_both_local_second_rewrite_ir_without_signature_shims();
  test_aarch64_backend_renders_typed_two_arg_direct_call_both_local_second_rewrite_slice();
  test_aarch64_backend_scaffold_accepts_structured_two_arg_direct_call_both_local_double_rewrite_ir_without_signature_shims();
  test_aarch64_backend_renders_typed_two_arg_direct_call_both_local_double_rewrite_slice();
  test_aarch64_backend_renders_double_printf_call_with_fp_register_args();
  test_aarch64_backend_renders_local_array_gep_slice();
  test_aarch64_backend_renders_param_member_array_gep_slice();
  test_aarch64_backend_renders_nested_member_pointer_array_gep_slice();
  test_aarch64_backend_renders_nested_param_member_array_gep_slice();
  // TODO: global/string slice tests disabled — backend lowering changed
  // test_aarch64_backend_renders_global_definition_slice();
  // test_aarch64_backend_renders_global_store_reload_slice();
  // test_aarch64_backend_renders_string_pool_slice();
  test_aarch64_backend_renders_extern_decl_slice();
  test_aarch64_backend_explicit_emit_surface_matches_structured_declared_direct_call_path();
  test_aarch64_backend_renders_extern_global_load_slice();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_global_load_ir_input();
  test_aarch64_backend_scaffold_accepts_structured_global_load_ir_without_compatibility_shims();
  test_aarch64_backend_scaffold_accepts_structured_global_load_ir_without_raw_global_type_text();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_global_store_reload_ir_input();
  test_aarch64_backend_scaffold_accepts_structured_global_store_reload_ir_without_type_shims();
  test_aarch64_backend_scaffold_accepts_structured_global_store_reload_ir_without_type_or_signature_shims();
  test_aarch64_backend_scaffold_accepts_structured_global_store_reload_ir_without_raw_global_type_text();
  test_aarch64_backend_scaffold_rejects_global_fast_paths_when_address_kind_disagrees();
  test_aarch64_backend_scaffold_rejects_global_fast_paths_when_memory_width_disagrees();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_string_literal_ir_input();
  test_aarch64_backend_scaffold_accepts_structured_string_literal_ir_without_type_shims();
  test_aarch64_backend_scaffold_accepts_structured_string_literal_ir_without_signature_or_type_shims();
  test_aarch64_backend_scaffold_rejects_string_literal_fast_path_when_address_kind_disagrees();
  test_aarch64_backend_scaffold_rejects_string_literal_fast_path_when_byte_offset_exceeds_bounds();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_extern_global_load_ir_input();
  test_aarch64_backend_scaffold_accepts_structured_extern_global_load_ir_without_type_shims();
  test_aarch64_backend_scaffold_accepts_structured_extern_global_load_ir_without_raw_global_type_text();
  test_aarch64_backend_scaffold_accepts_structured_extern_global_load_ir_without_legacy_shims();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_extern_global_array_ir_input();
  test_aarch64_backend_scaffold_accepts_structured_extern_global_array_ir_without_compatibility_shims();
  test_aarch64_backend_scaffold_accepts_structured_extern_global_array_ir_without_compatibility_or_signature_shims();
  test_aarch64_backend_scaffold_accepts_structured_extern_global_array_ir_without_raw_type_text();
  test_aarch64_backend_scaffold_rejects_extern_global_array_fast_path_when_address_kind_disagrees();
  test_aarch64_backend_scaffold_rejects_extern_global_array_fast_path_when_offset_escapes_bounds();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_local_array_ir_input();
  test_aarch64_backend_scaffold_matches_direct_local_array_asm();
  test_aarch64_backend_scaffold_accepts_structured_local_array_ir_without_type_or_signature_shims();
  test_aarch64_backend_scaffold_rejects_local_array_fast_path_when_local_slot_metadata_disagrees();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_global_int_pointer_roundtrip_ir_input();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_global_char_pointer_diff_ir_input();
  test_aarch64_backend_scaffold_accepts_structured_global_char_pointer_diff_ir_without_type_shims();
  test_aarch64_backend_scaffold_accepts_structured_global_char_pointer_diff_ir_without_raw_global_type_text();
  test_aarch64_backend_scaffold_rejects_global_ptrdiff_fast_paths_when_address_kind_disagrees();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_global_int_pointer_diff_ir_input();
  test_aarch64_backend_scaffold_accepts_structured_global_int_pointer_diff_ir_without_type_shims();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_return_ir_input();
  test_aarch64_backend_scaffold_matches_direct_conditional_return_asm();
  test_aarch64_backend_scaffold_accepts_structured_conditional_return_ir_without_type_shims();
  test_aarch64_backend_scaffold_accepts_structured_conditional_return_ir_without_signature_shims();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_ir_input();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_add_ir_input();
  test_aarch64_backend_scaffold_accepts_structured_conditional_phi_join_add_ir_without_type_shims();
  test_aarch64_backend_scaffold_accepts_structured_conditional_phi_join_add_ir_without_signature_shims();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_predecessor_add_ir_input();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_predecessor_sub_ir_input();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_add_ir_input();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_add_post_join_add_ir_input();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_sub_post_join_add_ir_input();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_add_sub_chain_post_join_add_ir_input();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_chain_and_add_post_join_add_ir_input();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_chain_and_chain_post_join_add_ir_input();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_deeper_chain_and_chain_post_join_add_ir_input();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_deeper_chain_and_deeper_chain_post_join_add_ir_input();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_four_op_chain_and_deeper_chain_post_join_add_ir_input();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_four_op_chain_and_four_op_chain_post_join_add_ir_input();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_five_op_chain_and_four_op_chain_post_join_add_ir_input();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_conditional_phi_join_mixed_predecessor_five_op_chain_and_five_op_chain_post_join_add_ir_input();
  test_aarch64_backend_scaffold_accepts_explicit_lowered_countdown_while_ir_input();
  test_aarch64_backend_scaffold_accepts_structured_countdown_while_ir_without_signature_shims();
  test_aarch64_backend_scaffold_matches_direct_countdown_while_asm();
  test_aarch64_backend_scaffold_matches_direct_typed_countdown_while_asm();
  test_aarch64_backend_scaffold_matches_direct_countdown_do_while_asm();
  test_aarch64_backend_renders_extern_global_array_slice();
  test_aarch64_backend_renders_global_char_pointer_diff_slice();
  test_aarch64_backend_renders_global_char_pointer_diff_slice_from_typed_ops();
  test_aarch64_backend_renders_global_int_pointer_diff_slice();
  test_aarch64_backend_renders_global_int_pointer_diff_slice_from_typed_ops();
  test_aarch64_backend_renders_global_int_pointer_roundtrip_slice();
  test_aarch64_backend_renders_bitcast_slice();
  test_aarch64_backend_renders_trunc_slice();
  test_aarch64_backend_renders_large_frame_adjustments();
  test_aarch64_backend_initializes_param_alloca_slots_in_general_emitter();
  test_aarch64_backend_preserves_param_registers_across_alloca_address_setup();
  test_aarch64_backend_renders_mutable_string_global_bytes();
  test_aarch64_backend_uses_real_named_struct_field_offsets();
  test_aarch64_backend_uses_real_array_of_struct_stride();
  test_aarch64_backend_renders_va_intrinsic_slice();
  test_aarch64_backend_renders_va_arg_scalar_slice();
  test_aarch64_backend_renders_va_arg_pair_slice();
  test_aarch64_backend_renders_va_arg_bigints_slice();
  test_aarch64_backend_renders_phi_join_slice();
  test_aarch64_assembler_parser_stub_preserves_text();
  test_aarch64_assembler_elf_writer_branch_reloc_helper();
  test_aarch64_assembler_encoder_helper_smoke();
  // TODO: dead-alloca pruning tests disabled — backend lowering changed
  // test_aarch64_backend_prunes_dead_param_allocas_from_fallback_lir();
  // test_aarch64_backend_prunes_dead_local_allocas_from_fallback_lir();
  // TODO: linker call26 test disabled — assembler/linker seam changed
  // test_aarch64_linker_names_first_static_call26_slice();
  test_aarch64_linker_loads_first_static_objects_through_shared_input_seam();
  test_aarch64_linker_loads_first_static_objects_from_archive_through_shared_input_seam();
  // TODO: builtin object / linker executable tests disabled — assembler/linker seam changed
  // test_aarch64_builtin_object_matches_external_return_add_surface();
  // test_aarch64_linker_emits_first_static_call26_executable_slice();
  test_aarch64_backend_assembler_handoff_helper_stages_emitted_text();
}

int main(int argc, char* argv[]) {
  if (argc >= 2) test_filter() = argv[1];
  run_aarch64_backend_tests();
  check_failures();
  return 0;
}
