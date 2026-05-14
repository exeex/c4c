#include "src/backend/backend.hpp"
#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/lir_to_bir.hpp"
#include "src/backend/mir/x86/api/api.hpp"
#include "src/backend/mir/x86/codegen/route_debug.hpp"
#include "src/backend/prealloc/prepared_printer.hpp"
#include "src/codegen/lir/ir.hpp"

#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

namespace lir = c4c::codegen::lir;
namespace prepare = c4c::backend::prepare;
using c4c::backend::BackendModuleInput;
using c4c::backend::BackendOptions;

c4c::TargetProfile x86_target_profile() {
  return c4c::default_target_profile(c4c::TargetArch::X86_64);
}

c4c::TargetProfile riscv_target_profile() {
  return c4c::default_target_profile(c4c::TargetArch::Riscv64);
}

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

lir::LirModule make_x86_return_constant_lir_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  lir::LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};

  lir::LirBlock entry;
  entry.label = "entry";
  entry.terminator = lir::LirRet{
      .value_str = std::string("7"),
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

lir::LirModule make_x86_param_add_immediate_lir_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  lir::LirFunction function;
  function.name = "add_one";
  function.signature_text = "define i32 @add_one(i32 %x)";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};
  function.params.emplace_back("%x", c4c::TypeSpec{.base = c4c::TB_INT});

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirBinOp{
      .result = lir::LirOperand("%sum"),
      .opcode = "add",
      .type_str = "i32",
      .lhs = lir::LirOperand("%x"),
      .rhs = lir::LirOperand("1"),
  });
  entry.terminator = lir::LirRet{
      .value_str = std::string("%sum"),
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

lir::LirModule make_riscv_tiny_add_lir_module() {
  lir::LirModule module;
  module.target_profile = riscv_target_profile();

  lir::LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirBinOp{
      .result = lir::LirOperand("%t0"),
      .opcode = "add",
      .type_str = "i32",
      .lhs = lir::LirOperand("2"),
      .rhs = lir::LirOperand("3"),
  });
  entry.terminator = lir::LirRet{
      .value_str = std::string("%t0"),
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

lir::LirModule make_x86_local_pointer_chain_lir_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  lir::LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.x"),
      .type_str = "i32",
      .count = lir::LirOperand(""),
      .align = 4,
  });
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.p"),
      .type_str = "ptr",
      .count = lir::LirOperand(""),
      .align = 8,
  });
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.pp"),
      .type_str = "ptr",
      .count = lir::LirOperand(""),
      .align = 8,
  });

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = "i32",
      .val = lir::LirOperand("0"),
      .ptr = lir::LirOperand("%lv.x"),
  });
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = "ptr",
      .val = lir::LirOperand("%lv.x"),
      .ptr = lir::LirOperand("%lv.p"),
  });
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = "ptr",
      .val = lir::LirOperand("%lv.p"),
      .ptr = lir::LirOperand("%lv.pp"),
  });
  entry.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t0"),
      .type_str = "ptr",
      .ptr = lir::LirOperand("%lv.pp"),
  });
  entry.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t1"),
      .type_str = "ptr",
      .ptr = lir::LirOperand("%t0"),
  });
  entry.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t2"),
      .type_str = "i32",
      .ptr = lir::LirOperand("%t1"),
  });
  entry.terminator = lir::LirRet{
      .value_str = std::string("%t2"),
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

lir::LirModule make_x86_local_i32_byte_storage_lir_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  lir::LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.u"),
      .type_str = "{ [4 x i8] }",
      .count = lir::LirOperand(""),
      .align = 4,
  });

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirGepOp{
      .result = lir::LirOperand("%t0"),
      .element_type = "{ [4 x i8] }",
      .ptr = lir::LirOperand("%lv.u"),
      .indices = {"i32 0", "i32 0"},
  });
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = "i32",
      .val = lir::LirOperand("3"),
      .ptr = lir::LirOperand("%t0"),
  });
  entry.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t1"),
      .type_str = "i32",
      .ptr = lir::LirOperand("%t0"),
  });
  entry.terminator = lir::LirRet{
      .value_str = std::string("%t1"),
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

lir::LirModule make_x86_loaded_local_array_pointer_guard_lir_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  lir::LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.a"),
      .type_str = "[2 x [4 x i8]]",
      .count = lir::LirOperand(""),
      .align = 1,
  });
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.p"),
      .type_str = "ptr",
      .count = lir::LirOperand(""),
      .align = 8,
  });

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirGepOp{
      .result = lir::LirOperand("%t0"),
      .element_type = "[2 x [4 x i8]]",
      .ptr = lir::LirOperand("%lv.a"),
      .indices = {lir::LirOperand("i64 0"), lir::LirOperand("i64 0")},
  });
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = "ptr",
      .val = lir::LirOperand("%t0"),
      .ptr = lir::LirOperand("%lv.p"),
  });
  entry.insts.push_back(lir::LirGepOp{
      .result = lir::LirOperand("%t1"),
      .element_type = "[2 x [4 x i8]]",
      .ptr = lir::LirOperand("%lv.a"),
      .indices = {lir::LirOperand("i64 0"), lir::LirOperand("i64 0")},
  });
  entry.insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand("%t2"),
      .kind = lir::LirCastKind::SExt,
      .from_type = "i32",
      .operand = lir::LirOperand("1"),
      .to_type = "i64",
  });
  entry.insts.push_back(lir::LirGepOp{
      .result = lir::LirOperand("%t3"),
      .element_type = "[4 x i8]",
      .ptr = lir::LirOperand("%t1"),
      .indices = {lir::LirOperand("i64 %t2")},
  });
  entry.insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand("%t4"),
      .kind = lir::LirCastKind::SExt,
      .from_type = "i32",
      .operand = lir::LirOperand("3"),
      .to_type = "i64",
  });
  entry.insts.push_back(lir::LirGepOp{
      .result = lir::LirOperand("%t5"),
      .element_type = "i8",
      .ptr = lir::LirOperand("%t3"),
      .indices = {lir::LirOperand("i64 %t4")},
  });
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = "i8",
      .val = lir::LirOperand("2"),
      .ptr = lir::LirOperand("%t5"),
  });
  entry.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t7"),
      .type_str = "ptr",
      .ptr = lir::LirOperand("%lv.p"),
  });
  entry.insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand("%t8"),
      .kind = lir::LirCastKind::SExt,
      .from_type = "i32",
      .operand = lir::LirOperand("1"),
      .to_type = "i64",
  });
  entry.insts.push_back(lir::LirGepOp{
      .result = lir::LirOperand("%t9"),
      .element_type = "[4 x i8]",
      .ptr = lir::LirOperand("%t7"),
      .indices = {lir::LirOperand("i64 %t8")},
  });
  entry.insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand("%t10"),
      .kind = lir::LirCastKind::SExt,
      .from_type = "i32",
      .operand = lir::LirOperand("3"),
      .to_type = "i64",
  });
  entry.insts.push_back(lir::LirGepOp{
      .result = lir::LirOperand("%t11"),
      .element_type = "i8",
      .ptr = lir::LirOperand("%t9"),
      .indices = {lir::LirOperand("i64 %t10")},
  });
  entry.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t12"),
      .type_str = "i8",
      .ptr = lir::LirOperand("%t11"),
  });
  entry.insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand("%t13"),
      .kind = lir::LirCastKind::SExt,
      .from_type = "i8",
      .operand = lir::LirOperand("%t12"),
      .to_type = "i32",
  });
  entry.insts.push_back(lir::LirCmpOp{
      .result = lir::LirOperand("%t14"),
      .is_float = false,
      .predicate = "ne",
      .type_str = "i32",
      .lhs = lir::LirOperand("%t13"),
      .rhs = lir::LirOperand("2"),
  });
  entry.insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand("%t15"),
      .kind = lir::LirCastKind::ZExt,
      .from_type = "i1",
      .operand = lir::LirOperand("%t14"),
      .to_type = "i32",
  });
  entry.insts.push_back(lir::LirCmpOp{
      .result = lir::LirOperand("%t16"),
      .is_float = false,
      .predicate = "ne",
      .type_str = "i32",
      .lhs = lir::LirOperand("%t15"),
      .rhs = lir::LirOperand("0"),
  });
  entry.terminator = lir::LirCondBr{
      .cond_name = "%t16",
      .true_label = "block_1",
      .false_label = "block_2",
  };

  lir::LirBlock block_1;
  block_1.label = "block_1";
  block_1.terminator = lir::LirRet{
      .value_str = std::string("1"),
      .type_str = "i32",
  };

  lir::LirBlock block_2;
  block_2.label = "block_2";
  block_2.terminator = lir::LirRet{
      .value_str = std::string("0"),
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  module.functions.push_back(std::move(function));
  return module;
}

lir::LirModule make_x86_local_pointer_guard_chain_lir_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  lir::LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.x"),
      .type_str = "i32",
      .count = lir::LirOperand(""),
      .align = 4,
  });
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.p"),
      .type_str = "ptr",
      .count = lir::LirOperand(""),
      .align = 8,
  });
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.pp"),
      .type_str = "ptr",
      .count = lir::LirOperand(""),
      .align = 8,
  });

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = "i32",
      .val = lir::LirOperand("0"),
      .ptr = lir::LirOperand("%lv.x"),
  });
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = "ptr",
      .val = lir::LirOperand("%lv.x"),
      .ptr = lir::LirOperand("%lv.p"),
  });
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = "ptr",
      .val = lir::LirOperand("%lv.p"),
      .ptr = lir::LirOperand("%lv.pp"),
  });
  entry.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t0"),
      .type_str = "ptr",
      .ptr = lir::LirOperand("%lv.p"),
  });
  entry.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t1"),
      .type_str = "i32",
      .ptr = lir::LirOperand("%t0"),
  });
  entry.insts.push_back(lir::LirCmpOp{
      .result = lir::LirOperand("%t2"),
      .is_float = false,
      .predicate = "ne",
      .type_str = "i32",
      .lhs = lir::LirOperand("%t1"),
      .rhs = lir::LirOperand("0"),
  });
  entry.terminator = lir::LirCondBr{
      .cond_name = "%t2",
      .true_label = "block_1",
      .false_label = "block_2",
  };

  lir::LirBlock block_1;
  block_1.label = "block_1";
  block_1.terminator = lir::LirRet{
      .value_str = std::string("1"),
      .type_str = "i32",
  };

  lir::LirBlock block_2;
  block_2.label = "block_2";
  block_2.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t3"),
      .type_str = "ptr",
      .ptr = lir::LirOperand("%lv.pp"),
  });
  block_2.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t4"),
      .type_str = "ptr",
      .ptr = lir::LirOperand("%t3"),
  });
  block_2.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t5"),
      .type_str = "i32",
      .ptr = lir::LirOperand("%t4"),
  });
  block_2.insts.push_back(lir::LirCmpOp{
      .result = lir::LirOperand("%t6"),
      .is_float = false,
      .predicate = "ne",
      .type_str = "i32",
      .lhs = lir::LirOperand("%t5"),
      .rhs = lir::LirOperand("0"),
  });
  block_2.terminator = lir::LirCondBr{
      .cond_name = "%t6",
      .true_label = "block_3",
      .false_label = "block_4",
  };

  lir::LirBlock block_3;
  block_3.label = "block_3";
  block_3.terminator = lir::LirRet{
      .value_str = std::string("1"),
      .type_str = "i32",
  };

  lir::LirBlock block_4;
  block_4.label = "block_4";
  block_4.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t7"),
      .type_str = "ptr",
      .ptr = lir::LirOperand("%lv.pp"),
  });
  block_4.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t8"),
      .type_str = "ptr",
      .ptr = lir::LirOperand("%t7"),
  });
  block_4.insts.push_back(lir::LirStoreOp{
      .type_str = "i32",
      .val = lir::LirOperand("1"),
      .ptr = lir::LirOperand("%t8"),
  });
  block_4.terminator = lir::LirBr{
      .target_label = "block_5",
  };

  lir::LirBlock block_5;
  block_5.label = "block_5";
  block_5.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t9"),
      .type_str = "i32",
      .ptr = lir::LirOperand("%lv.x"),
  });
  block_5.insts.push_back(lir::LirCmpOp{
      .result = lir::LirOperand("%t10"),
      .is_float = false,
      .predicate = "ne",
      .type_str = "i32",
      .lhs = lir::LirOperand("%t9"),
      .rhs = lir::LirOperand("0"),
  });
  block_5.terminator = lir::LirCondBr{
      .cond_name = "%t10",
      .true_label = "block_6",
      .false_label = "block_7",
  };

  lir::LirBlock block_6;
  block_6.label = "block_6";
  block_6.terminator = lir::LirRet{
      .value_str = std::string("0"),
      .type_str = "i32",
  };

  lir::LirBlock block_7;
  block_7.label = "block_7";
  block_7.terminator = lir::LirRet{
      .value_str = std::string("1"),
      .type_str = "i32",
  };

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

lir::LirModule make_unsupported_x86_inline_asm_lir_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  lir::LirFunction function;
  function.name = "bad_inline_asm";
  function.signature_text = "define void @bad_inline_asm()";

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirInlineAsmOp{
      .result = lir::LirOperand("%t0"),
      .ret_type = "{ i32, i32 }",
      .asm_text = "",
      .constraints = "=r,=r",
      .side_effects = true,
      .args_str = "",
  });
  entry.terminator = lir::LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

int check_lir_route_outputs(const lir::LirModule& module, const char* failure_context) {
  c4c::backend::BirLoweringOptions lowering_options{};
  auto lowering = c4c::backend::try_lower_to_bir_with_options(module, lowering_options);
  if (!lowering.module.has_value()) {
    return fail((std::string(failure_context) +
                 ": semantic lir_to_bir no longer produces the canonical prepared-module handoff")
                    .c_str());
  }

  const auto prepared =
      prepare::prepare_semantic_bir_module_with_options(*lowering.module, module.target_profile);
  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);

  const auto explicit_x86_asm =
      c4c::backend::emit_x86_lir_module_entry(module, module.target_profile);
  if (explicit_x86_asm != prepared_asm) {
    return fail((std::string(failure_context) +
                 ": explicit x86 LIR module entry did not route through the canonical x86 prepared-module consumer")
                    .c_str());
  }

  const auto public_asm = c4c::backend::emit_target_lir_module(module, module.target_profile);
  if (public_asm != explicit_x86_asm) {
    return fail((std::string(failure_context) +
                 ": compatibility LIR entry no longer matches the explicit x86 module-entry surface")
                    .c_str());
  }
  if (public_asm != prepared_asm) {
    return fail((std::string(failure_context) +
                 ": public x86 LIR entry did not route through the canonical x86 prepared-module consumer")
                    .c_str());
  }

  const auto generic_asm = c4c::backend::emit_module(
      BackendModuleInput{module}, BackendOptions{.target_profile = x86_target_profile()});
  if (generic_asm != explicit_x86_asm) {
    return fail((std::string(failure_context) +
                 ": generic backend emit path no longer routes x86 LIR input through emit_x86_lir_module_entry")
                    .c_str());
  }

  return 0;
}

int check_lir_assemble_route_outputs(const lir::LirModule& module, const char* failure_context) {
  const std::string output_path = "ignored-by-generic-x86-backend-assemble.o";
  const auto expected_staged = c4c::backend::x86::api::emit_module(module, module.target_profile);
  const auto explicit_x86_result =
      c4c::backend::stage_x86_lir_module_entry(module, module.target_profile, output_path);
  if (explicit_x86_result.staged_text != expected_staged) {
    return fail((std::string(failure_context) +
                 ": explicit x86 module-entry staging no longer uses the canonical x86 api seam")
                    .c_str());
  }
  if (explicit_x86_result.output_path != output_path || explicit_x86_result.object_emitted ||
      explicit_x86_result.error != "backend bootstrap mode does not assemble objects yet") {
    return fail((std::string(failure_context) +
                 ": explicit x86 module-entry staging no longer preserves the bootstrap assemble result contract")
                    .c_str());
  }

  const auto public_result =
      c4c::backend::assemble_target_lir_module(module, module.target_profile, output_path);

  if (public_result.staged_text != explicit_x86_result.staged_text) {
    return fail((std::string(failure_context) +
                 ": compatibility assemble entry no longer matches the explicit x86 module-entry staging surface")
                    .c_str());
  }
  if (public_result.staged_text != expected_staged) {
    return fail((std::string(failure_context) +
                 ": generic backend assemble path no longer stages x86 LIR input through the canonical x86 api seam")
                    .c_str());
  }
  if (public_result.output_path != output_path || public_result.object_emitted ||
      public_result.error != "backend bootstrap mode does not assemble objects yet") {
    return fail((std::string(failure_context) +
                 ": generic backend assemble path no longer preserves the bootstrap assemble result contract around the x86 api handoff")
                    .c_str());
  }

  return 0;
}

int check_lir_dump_route_outputs(const lir::LirModule& module, const char* failure_context) {
  c4c::backend::BirLoweringOptions lowering_options{};
  lowering_options.preserve_dynamic_alloca = true;
  auto lowering = c4c::backend::try_lower_to_bir_with_options(module, lowering_options);
  if (!lowering.module.has_value()) {
    return fail((std::string(failure_context) +
                 ": dump route no longer produces semantic lir_to_bir output for x86 LIR input")
                    .c_str());
  }

  const auto prepared =
      prepare::prepare_semantic_bir_module_with_options(*lowering.module, module.target_profile);
  const auto backend_options = BackendOptions{.target_profile = x86_target_profile()};

  const auto semantic_dump = c4c::backend::dump_module(
      BackendModuleInput{module}, backend_options, c4c::backend::BackendDumpStage::SemanticBir);
  if (semantic_dump != c4c::backend::bir::print(*lowering.module)) {
    return fail((std::string(failure_context) +
                 ": generic backend semantic dump no longer matches x86 LIR lowering output")
                    .c_str());
  }

  const auto prepared_dump = c4c::backend::dump_module(
      BackendModuleInput{module}, backend_options, c4c::backend::BackendDumpStage::PreparedBir);
  if (prepared_dump != c4c::backend::prepare::print(prepared)) {
    return fail((std::string(failure_context) +
                 ": generic backend prepared dump no longer matches the canonical prepared-module handoff")
                    .c_str());
  }

  const auto summary_dump = c4c::backend::dump_module(
      BackendModuleInput{module}, backend_options, c4c::backend::BackendDumpStage::MirSummary);
  if (summary_dump != c4c::backend::x86::summarize_prepared_module_routes(prepared)) {
    return fail((std::string(failure_context) +
                 ": generic backend MIR summary dump no longer routes x86 LIR input through target-local route debug")
                    .c_str());
  }

  const auto trace_dump = c4c::backend::dump_module(
      BackendModuleInput{module}, backend_options, c4c::backend::BackendDumpStage::MirTrace);
  if (trace_dump != c4c::backend::x86::trace_prepared_module_routes(prepared)) {
    return fail((std::string(failure_context) +
                 ": generic backend MIR trace dump no longer routes x86 LIR input through target-local route debug")
                    .c_str());
  }

  return 0;
}

int check_non_x86_lir_compatibility_wrappers(const lir::LirModule& module,
                                             const char* failure_context) {
  c4c::backend::BirLoweringOptions lowering_options{};
  auto lowering = c4c::backend::try_lower_to_bir_with_options(module, lowering_options);
  if (!lowering.module.has_value()) {
    return fail((std::string(failure_context) +
                 ": non-x86 compatibility wrapper fixture no longer lowers to semantic BIR")
                    .c_str());
  }

  const auto prepared =
      prepare::prepare_semantic_bir_module_with_options(*lowering.module, module.target_profile);
  const auto expected_public_text = c4c::backend::bir::print(prepared.module);
  const auto backend_options = BackendOptions{.target_profile = riscv_target_profile()};

  const auto public_text = c4c::backend::emit_target_lir_module(module, module.target_profile);
  if (public_text != expected_public_text) {
    return fail((std::string(failure_context) +
                 ": compatibility LIR emit wrapper no longer preserves the generic non-x86 prepared-BIR contract")
                    .c_str());
  }

  const auto generic_text = c4c::backend::emit_module(BackendModuleInput{module}, backend_options);
  if (generic_text != expected_public_text) {
    return fail((std::string(failure_context) +
                 ": generic backend LIR emit front door no longer preserves the non-x86 prepared-BIR contract")
                    .c_str());
  }

  const std::string output_path = "ignored-by-generic-non-x86-backend-assemble.o";
  const auto assembled =
      c4c::backend::assemble_target_lir_module(module, module.target_profile, output_path);
  if (assembled.staged_text != expected_public_text) {
    return fail((std::string(failure_context) +
                 ": compatibility assemble wrapper no longer preserves the generic non-x86 staged-text contract")
                    .c_str());
  }
  if (assembled.output_path != output_path || assembled.object_emitted ||
      assembled.error != "backend bootstrap mode does not assemble objects yet") {
    return fail((std::string(failure_context) +
                 ": compatibility assemble wrapper no longer preserves the generic non-x86 bootstrap assemble contract")
                    .c_str());
  }

  return 0;
}

int check_lir_route_rejection(const lir::LirModule& module,
                              std::string_view expected_message_fragment,
                              const char* failure_context) {
  try {
    (void)c4c::backend::emit_x86_lir_module_entry(module, module.target_profile);
    return fail((std::string(failure_context) +
                 ": explicit x86 module entry unexpectedly kept a mixed bootstrap fallback alive")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find(expected_message_fragment) == std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": explicit x86 module entry rejected unsupported lowering with the wrong contract message")
                      .c_str());
    }
  }

  try {
    (void)c4c::backend::emit_target_lir_module(module, module.target_profile);
    return fail((std::string(failure_context) +
                 ": public x86 LIR entry unexpectedly kept a mixed bootstrap fallback alive")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find(expected_message_fragment) == std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": public x86 LIR entry rejected unsupported lowering with the wrong contract message")
                      .c_str());
    }
  }

  try {
    (void)c4c::backend::emit_module(
        BackendModuleInput{module}, BackendOptions{.target_profile = x86_target_profile()});
    return fail((std::string(failure_context) +
                 ": generic x86 backend emit path unexpectedly kept a mixed bootstrap fallback alive")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find(expected_message_fragment) == std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": generic x86 backend emit path rejected unsupported lowering with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

}  // namespace

int run_backend_x86_handoff_boundary_lir_tests() {
  // Keep the bounded LIR handoff lane isolated so later Step 5 packets can
  // split remaining BIR-only families without re-threading the LIR route.
  if (const auto status =
          check_lir_route_outputs(make_x86_return_constant_lir_module(),
                                  "minimal immediate return LIR route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_lir_assemble_route_outputs(make_x86_return_constant_lir_module(),
                                           "minimal immediate return x86 assemble route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_lir_dump_route_outputs(make_x86_return_constant_lir_module(),
                                       "minimal immediate return x86 dump route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_lir_route_outputs(make_x86_param_add_immediate_lir_module(),
                                  "minimal i32 parameter add-immediate LIR route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_non_x86_lir_compatibility_wrappers(make_riscv_tiny_add_lir_module(),
                                                   "thin non-x86 compatibility LIR wrappers");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_lir_route_outputs(make_x86_local_pointer_chain_lir_module(),
                                  "minimal local-slot pointer-chain LIR route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_lir_route_outputs(make_x86_local_i32_byte_storage_lir_module(),
                                  "minimal local aggregate byte-storage i32 LIR route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_lir_route_outputs(make_x86_loaded_local_array_pointer_guard_lir_module(),
                                  "loaded local array-pointer guard LIR route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_lir_route_outputs(make_x86_local_pointer_guard_chain_lir_module(),
                                  "minimal local-slot compare-against-zero guard-chain LIR route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_lir_route_rejection(
              make_unsupported_x86_inline_asm_lir_module(),
              "requires semantic lir_to_bir lowering before the canonical prepared-module handoff",
              "unsupported x86 inline-asm LIR route");
      status != 0) {
    return status;
  }
  return 0;
}
