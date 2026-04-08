#include "backend_bir_test_support.hpp"

#include "../../src/backend/lowering/lir_to_bir.hpp"

#include <stdexcept>

namespace {

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
    return;
  }

  fail("x86 direct BIR input should fail explicitly once the emitter-side bir_to_backend_ir fallback is removed");
}

}  // namespace

void run_backend_bir_pipeline_x86_64_tests() {
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_minimal_direct_call_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_declared_direct_call_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_minimal_two_arg_direct_call_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_minimal_direct_call_add_imm_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_direct_bir_minimal_direct_call_identity_arg_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_direct_call_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_declared_direct_call_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_two_arg_direct_call_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_direct_call_add_imm_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_direct_call_identity_arg_through_bir_end_to_end);
  RUN_TEST(test_backend_bir_pipeline_drives_x86_lir_minimal_folded_two_arg_direct_call_through_bir_end_to_end);
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
}
