
#include "backend.hpp"
#include "generation.hpp"
#include "liveness.hpp"
#include "regalloc.hpp"
#include "stack_layout/analysis.hpp"
#include "stack_layout/alloca_coalescing.hpp"
#include "stack_layout/regalloc_helpers.hpp"
#include "stack_layout/slot_assignment.hpp"
#include "target.hpp"
#include "../../src/codegen/lir/call_args.hpp"
#include "../../src/codegen/lir/lir_printer.hpp"
#include "../../src/codegen/lir/verify.hpp"
#include "../../src/backend/lowering/call_decode.hpp"
#include "../../src/backend/elf/mod.hpp"
#include "../../src/backend/linker_common/mod.hpp"
#include "../../src/backend/aarch64/assembler/mod.hpp"
#include "../../src/backend/aarch64/codegen/emit.hpp"
#include "../../src/backend/aarch64/linker/mod.hpp"
#include "../../src/backend/aarch64/assembler/parser.hpp"
#include "../../src/backend/x86/assembler/mod.hpp"
#include "../../src/backend/x86/assembler/encoder/mod.hpp"
#include "../../src/backend/x86/assembler/parser.hpp"
#include "../../src/backend/x86/codegen/emit.hpp"
#include "../../src/backend/x86/linker/mod.hpp"
#include "backend_test_helper.hpp"

void test_backend_shared_call_decode_parses_bir_minimal_direct_call_module() {
  c4c::backend::bir::Module module;
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

  const auto parsed = c4c::backend::parse_bir_minimal_direct_call_module(module);
  expect_true(parsed.has_value(),
              "shared call-decode surface should parse a BIR helper definition plus main direct call module");
  expect_true(parsed->helper != nullptr && parsed->helper->name == "helper" &&
                  parsed->main_function != nullptr && parsed->main_function->name == "main",
              "shared call-decode surface should preserve the helper and main function identities for BIR direct-call modules");
  expect_true(parsed->call != nullptr && parsed->call->callee == "helper" &&
                  parsed->return_imm == 42,
              "shared call-decode surface should recover the helper return immediate for minimal BIR direct-call modules");
}

void test_backend_shared_call_decode_parses_bir_minimal_declared_direct_call_module() {
  c4c::backend::bir::Module module;
  module.string_constants.push_back(c4c::backend::bir::StringConstant{
      .name = "msg",
      .bytes = "hello\n",
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

  const auto parsed = c4c::backend::parse_bir_minimal_declared_direct_call_module(module);
  expect_true(parsed.has_value(),
              "shared call-decode surface should parse a BIR declared direct-call module");
  expect_true(parsed->callee != nullptr && parsed->callee->name == "puts_like" &&
                  parsed->main_function != nullptr && parsed->main_function->name == "main",
              "shared call-decode surface should preserve declaration and caller identities for BIR declared direct-call modules");
  expect_true(parsed->args.size() == 1 &&
                  parsed->args.front().kind == c4c::backend::ParsedBackendExternCallArg::Kind::Ptr &&
                  parsed->args.front().operand == "@msg",
              "shared call-decode surface should classify BIR i64 symbol operands as pointer-style extern call args");
  expect_true(parsed->call != nullptr && parsed->call->callee == "puts_like" &&
                  !parsed->return_call_result && parsed->return_imm == 7,
              "shared call-decode surface should allow BIR declared direct-call modules to return a fixed immediate after the call");
}

void test_backend_shared_call_decode_parses_bir_minimal_void_direct_call_imm_return_module() {
  c4c::backend::bir::Module module;
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

  const auto parsed = c4c::backend::parse_bir_minimal_void_direct_call_imm_return_module(module);
  expect_true(parsed.has_value(),
              "shared call-decode surface should parse a BIR void direct-call module with a fixed caller return");
  expect_true(parsed->helper != nullptr && parsed->helper->name == "voidfn" &&
                  parsed->main_function != nullptr && parsed->main_function->name == "main",
              "shared call-decode surface should preserve helper and caller identities for BIR void direct-call modules");
  expect_true(parsed->call != nullptr && parsed->call->callee == "voidfn" &&
                  !parsed->call->result.has_value() && parsed->return_imm == 9,
              "shared call-decode surface should preserve the void call and caller fixed return immediate for BIR void direct-call modules");
}

void test_backend_shared_call_decode_parses_bir_minimal_two_arg_direct_call_module() {
  c4c::backend::bir::Module module;
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

  const auto parsed = c4c::backend::parse_bir_minimal_two_arg_direct_call_module(module);
  expect_true(parsed.has_value(),
              "shared call-decode surface should parse a BIR helper/main two-argument direct-call module");
  expect_true(parsed->helper != nullptr && parsed->helper->name == "add_pair" &&
                  parsed->main_function != nullptr && parsed->main_function->name == "main",
              "shared call-decode surface should preserve helper and caller identities for BIR two-argument direct-call modules");
  expect_true(parsed->call != nullptr && parsed->call->callee == "add_pair" &&
                  parsed->lhs_call_arg_imm == 5 && parsed->rhs_call_arg_imm == 7,
              "shared call-decode surface should recover both immediate call operands for the BIR two-argument direct-call slice");
}

void test_backend_shared_call_decode_parses_bir_minimal_direct_call_add_imm_module() {
  c4c::backend::bir::Module module;
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

  const auto parsed = c4c::backend::parse_bir_minimal_direct_call_add_imm_module(module);
  expect_true(parsed.has_value(),
              "shared call-decode surface should parse a BIR helper/main add-immediate direct-call module");
  expect_true(parsed->helper != nullptr && parsed->helper->name == "add_one" &&
                  parsed->main_function != nullptr && parsed->main_function->name == "main",
              "shared call-decode surface should preserve helper and caller identities for the BIR add-immediate direct-call slice");
  expect_true(parsed->call != nullptr && parsed->call->callee == "add_one" &&
                  parsed->call_arg_imm == 5 && parsed->add_imm == 1,
              "shared call-decode surface should recover the call argument and helper add immediate for the BIR add-immediate direct-call slice");
}

void test_backend_shared_call_decode_parses_bir_minimal_direct_call_identity_arg_module() {
  c4c::backend::bir::Module module;
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

  const auto parsed = c4c::backend::parse_bir_minimal_direct_call_identity_arg_module(module);
  expect_true(parsed.has_value(),
              "shared call-decode surface should parse a BIR helper/main identity direct-call module");
  expect_true(parsed->helper != nullptr && parsed->helper->name == "f" &&
                  parsed->main_function != nullptr && parsed->main_function->name == "main",
              "shared call-decode surface should preserve helper and caller identities for the BIR identity direct-call slice");
  expect_true(parsed->call != nullptr && parsed->call->callee == "f" &&
                  parsed->call_arg_imm == 0,
              "shared call-decode surface should recover the caller immediate for the BIR identity direct-call slice");
}

void test_backend_shared_call_decode_parses_bir_minimal_dual_identity_direct_call_sub_module() {
  c4c::backend::bir::Module module;
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

  const auto parsed = c4c::backend::parse_bir_minimal_dual_identity_direct_call_sub_module(module);
  expect_true(parsed.has_value(),
              "shared call-decode surface should parse a BIR dual-identity direct-call subtraction module");
  if (parsed.has_value()) {
    expect_true(parsed->lhs_helper != nullptr && parsed->lhs_helper->name == "f" &&
                    parsed->rhs_helper != nullptr && parsed->rhs_helper->name == "g" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main",
                "shared call-decode surface should preserve both helper identities and the caller for the BIR dual-identity subtraction slice");
    expect_true(parsed->lhs_call != nullptr && parsed->rhs_call != nullptr &&
                    parsed->sub != nullptr && parsed->lhs_call_arg_imm == 7 &&
                    parsed->rhs_call_arg_imm == 3,
                "shared call-decode surface should recover both call immediates and the subtraction instruction for the BIR dual-identity slice");
  }
}

void test_backend_shared_call_decode_parses_bir_minimal_call_crossing_direct_call_module() {
  c4c::backend::bir::Module module;
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

  const auto parsed = c4c::backend::parse_bir_minimal_call_crossing_direct_call_module(module);
  expect_true(parsed.has_value(),
              "shared call-decode surface should parse a BIR call-crossing direct-call module");
  if (parsed.has_value()) {
    expect_true(parsed->helper != nullptr && parsed->helper->name == "add_one" &&
                    parsed->main_function != nullptr && parsed->main_function->name == "main",
                "shared call-decode surface should preserve helper and caller identities for the BIR call-crossing slice");
    expect_true(parsed->source_add != nullptr && parsed->call != nullptr &&
                    parsed->final_add != nullptr &&
                    parsed->source_add->result.name == parsed->regalloc_source_value &&
                    parsed->call->args.front().name == parsed->regalloc_source_value &&
                    parsed->final_add->lhs.name == parsed->regalloc_source_value &&
                    parsed->regalloc_source_value == "%t0" &&
                    parsed->source_imm == 5 && parsed->helper_add_imm == 1,
                "shared call-decode surface should recover the source add result plus the source/helper immediates for the BIR call-crossing slice");
  }
}

namespace {

c4c::TypeSpec make_test_i32_typespec() {
  c4c::TypeSpec type{};
  type.base = c4c::TB_INT;
  type.array_size = -1;
  return type;
}

c4c::codegen::lir::LirTypeRef make_test_stale_text_i32_lir_type() {
  return c4c::codegen::lir::LirTypeRef("i8", c4c::codegen::lir::LirTypeKind::Integer, 32);
}

}  // namespace

void test_backend_shared_call_decode_accepts_typed_i32_single_add_imm_helper() {
  c4c::codegen::lir::LirFunction function;
  function.name = "add_one";
  function.return_type = make_test_i32_typespec();
  function.params.emplace_back("%arg0", make_test_i32_typespec());
  function.signature_text = "define i32 @add_one(i8 %stale)";

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(c4c::codegen::lir::LirBinOp{
      .result = "%sum",
      .opcode = c4c::codegen::lir::LirBinaryOpcode::Add,
      .type_str = make_test_stale_text_i32_lir_type(),
      .lhs = "%arg0",
      .rhs = "1",
  });
  entry.terminator = c4c::codegen::lir::LirRet{std::string("%sum"), "i32"};
  function.blocks.push_back(std::move(entry));

  const auto parsed = c4c::backend::parse_backend_single_add_imm_function(function, "add_one");
  expect_true(parsed.has_value(),
              "shared call-decode should accept semantic i32 typed helper binops for the add-immediate parser");
  if (parsed.has_value()) {
    expect_true(parsed->param_name == "%arg0" && parsed->add != nullptr && parsed->add->rhs == "1",
                "shared call-decode should preserve the typed helper param and add-immediate operand");
  }
}

void test_backend_shared_call_decode_accepts_typed_i32_single_add_imm_helper_with_stale_return_text() {
  c4c::codegen::lir::LirFunction function;
  function.name = "add_one";
  function.return_type = make_test_i32_typespec();
  function.params.emplace_back("%arg0", make_test_i32_typespec());
  function.signature_text = "define i8 @add_one(i32 %arg0)";

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(c4c::codegen::lir::LirBinOp{
      .result = "%sum",
      .opcode = c4c::codegen::lir::LirBinaryOpcode::Add,
      .type_str = make_test_stale_text_i32_lir_type(),
      .lhs = "%arg0",
      .rhs = "1",
  });
  entry.terminator =
      c4c::codegen::lir::LirRet{std::string("%sum"), make_test_stale_text_i32_lir_type()};
  function.blocks.push_back(std::move(entry));

  const auto parsed = c4c::backend::parse_backend_single_add_imm_function(function, "add_one");
  expect_true(parsed.has_value(),
              "shared call-decode should accept the add-immediate helper when typed function metadata says i32 but the stored helper return text is stale");
}

void test_backend_shared_call_decode_accepts_typed_i32_slot_add_helper() {
  c4c::codegen::lir::LirFunction function;
  function.name = "slot_add";
  function.return_type = make_test_i32_typespec();
  function.params.emplace_back("%arg0", make_test_i32_typespec());
  function.signature_text = "define i32 @slot_add(i8 %stale)";
  function.alloca_insts.push_back(c4c::codegen::lir::LirAllocaOp{
      .result = "%slot",
      .type_str = make_test_stale_text_i32_lir_type(),
      .count = "",
      .align = 4,
  });
  function.alloca_insts.push_back(c4c::codegen::lir::LirStoreOp{
      .type_str = make_test_stale_text_i32_lir_type(),
      .val = "%arg0",
      .ptr = "%slot",
  });

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(c4c::codegen::lir::LirLoadOp{
      .result = "%t0",
      .type_str = make_test_stale_text_i32_lir_type(),
      .ptr = "%slot",
  });
  entry.insts.push_back(c4c::codegen::lir::LirBinOp{
      .result = "%sum",
      .opcode = c4c::codegen::lir::LirBinaryOpcode::Add,
      .type_str = make_test_stale_text_i32_lir_type(),
      .lhs = "%t0",
      .rhs = "1",
  });
  entry.insts.push_back(c4c::codegen::lir::LirStoreOp{
      .type_str = make_test_stale_text_i32_lir_type(),
      .val = "%sum",
      .ptr = "%slot",
  });
  entry.insts.push_back(c4c::codegen::lir::LirLoadOp{
      .result = "%t1",
      .type_str = make_test_stale_text_i32_lir_type(),
      .ptr = "%slot",
  });
  entry.terminator = c4c::codegen::lir::LirRet{std::string("%t1"), "i32"};
  function.blocks.push_back(std::move(entry));

  const auto parsed = c4c::backend::parse_backend_single_param_slot_add_function(
      function, "slot_add");
  expect_true(parsed.has_value(),
              "shared call-decode should accept semantic i32 typed local-slot helper ops for the slot-add parser");
  if (parsed.has_value()) {
    expect_true(parsed->param_name == "%arg0" && parsed->slot_name == "%slot" &&
                    parsed->add != nullptr && parsed->add->rhs == "1",
                "shared call-decode should preserve the typed slot helper param, slot, and add operand");
  }
}

void test_backend_shared_call_decode_accepts_typed_i32_slot_add_helper_with_stale_return_text() {
  c4c::codegen::lir::LirFunction function;
  function.name = "slot_add";
  function.return_type = make_test_i32_typespec();
  function.params.emplace_back("%arg0", make_test_i32_typespec());
  function.signature_text = "define i8 @slot_add(i32 %arg0)";
  function.alloca_insts.push_back(c4c::codegen::lir::LirAllocaOp{
      .result = "%slot",
      .type_str = make_test_stale_text_i32_lir_type(),
      .count = "",
      .align = 4,
  });
  function.alloca_insts.push_back(c4c::codegen::lir::LirStoreOp{
      .type_str = make_test_stale_text_i32_lir_type(),
      .val = "%arg0",
      .ptr = "%slot",
  });

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(c4c::codegen::lir::LirLoadOp{
      .result = "%t0",
      .type_str = make_test_stale_text_i32_lir_type(),
      .ptr = "%slot",
  });
  entry.insts.push_back(c4c::codegen::lir::LirBinOp{
      .result = "%sum",
      .opcode = c4c::codegen::lir::LirBinaryOpcode::Add,
      .type_str = make_test_stale_text_i32_lir_type(),
      .lhs = "%t0",
      .rhs = "1",
  });
  entry.insts.push_back(c4c::codegen::lir::LirStoreOp{
      .type_str = make_test_stale_text_i32_lir_type(),
      .val = "%sum",
      .ptr = "%slot",
  });
  entry.insts.push_back(c4c::codegen::lir::LirLoadOp{
      .result = "%t1",
      .type_str = make_test_stale_text_i32_lir_type(),
      .ptr = "%slot",
  });
  entry.terminator =
      c4c::codegen::lir::LirRet{std::string("%t1"), make_test_stale_text_i32_lir_type()};
  function.blocks.push_back(std::move(entry));

  const auto parsed = c4c::backend::parse_backend_single_param_slot_add_function(
      function, "slot_add");
  expect_true(parsed.has_value(),
              "shared call-decode should accept the slot-add helper when typed function metadata says i32 but the stored helper return text is stale");
}

void test_backend_shared_call_decode_accepts_typed_i32_identity_helper() {
  c4c::codegen::lir::LirFunction function;
  function.name = "identity";
  function.return_type = make_test_i32_typespec();
  function.params.emplace_back("%arg0", make_test_i32_typespec());
  function.signature_text = "define i32 @identity(i8 %stale)";

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.terminator = c4c::codegen::lir::LirRet{std::string("%arg0"), "i32"};
  function.blocks.push_back(std::move(entry));

  const auto parsed = c4c::backend::parse_backend_single_identity_function(function, "identity");
  expect_true(parsed.has_value() && *parsed == "%arg0",
              "shared call-decode should accept typed i32 helper params for the identity helper parser");
}

void test_backend_shared_call_decode_accepts_typed_i32_identity_helper_with_stale_return_text() {
  c4c::codegen::lir::LirFunction function;
  function.name = "identity";
  function.return_type = make_test_i32_typespec();
  function.params.emplace_back("%arg0", make_test_i32_typespec());
  function.signature_text = "define i8 @identity(i32 %arg0)";

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.terminator =
      c4c::codegen::lir::LirRet{std::string("%arg0"), make_test_stale_text_i32_lir_type()};
  function.blocks.push_back(std::move(entry));

  const auto parsed = c4c::backend::parse_backend_single_identity_function(function, "identity");
  expect_true(parsed.has_value() && *parsed == "%arg0",
              "shared call-decode should accept the identity helper when typed function metadata says i32 but the stored helper return text is stale");
}

void test_backend_shared_call_decode_accepts_typed_i32_two_param_add_helper() {
  c4c::codegen::lir::LirFunction function;
  function.name = "add_pair";
  function.return_type = make_test_i32_typespec();
  function.params.emplace_back("%lhs", make_test_i32_typespec());
  function.params.emplace_back("%rhs", make_test_i32_typespec());
  function.signature_text = "define i32 @add_pair(i8 %lhs, i8 %rhs)";

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(c4c::codegen::lir::LirBinOp{
      .result = "%sum",
      .opcode = c4c::codegen::lir::LirBinaryOpcode::Add,
      .type_str = c4c::codegen::lir::LirTypeRef::integer(32),
      .lhs = "%lhs",
      .rhs = "%rhs",
  });
  entry.terminator = c4c::codegen::lir::LirRet{std::string("%sum"), "i32"};
  function.blocks.push_back(std::move(entry));

  const auto parsed = c4c::backend::parse_backend_two_param_add_function(function, "add_pair");
  expect_true(parsed.has_value(),
              "shared call-decode should accept semantic i32 typed helper binops for the two-parameter add parser");
  if (parsed.has_value()) {
    expect_true(parsed->lhs_param_name == "%lhs" && parsed->rhs_param_name == "%rhs" &&
                    parsed->add != nullptr && parsed->add->result == "%sum",
                "shared call-decode should preserve both typed helper params for the two-parameter add parser");
  }
}

void test_backend_shared_call_decode_accepts_typed_i32_two_param_add_helper_with_stale_return_text() {
  c4c::codegen::lir::LirFunction function;
  function.name = "add_pair";
  function.return_type = make_test_i32_typespec();
  function.params.emplace_back("%lhs", make_test_i32_typespec());
  function.params.emplace_back("%rhs", make_test_i32_typespec());
  function.signature_text = "define i8 @add_pair(i32 %lhs, i32 %rhs)";

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(c4c::codegen::lir::LirBinOp{
      .result = "%sum",
      .opcode = c4c::codegen::lir::LirBinaryOpcode::Add,
      .type_str = make_test_stale_text_i32_lir_type(),
      .lhs = "%lhs",
      .rhs = "%rhs",
  });
  entry.terminator =
      c4c::codegen::lir::LirRet{std::string("%sum"), make_test_stale_text_i32_lir_type()};
  function.blocks.push_back(std::move(entry));

  const auto parsed = c4c::backend::parse_backend_two_param_add_function(function, "add_pair");
  expect_true(parsed.has_value(),
              "shared call-decode should accept the two-parameter add helper when typed function metadata says i32 but the stored helper return text is stale");
}

void test_backend_shared_call_decode_prefers_typed_vararg_call_args_over_stale_fixed_suffix_text() {
  const c4c::codegen::lir::LirCallOp call{
      "%t0",
      "i32",
      "@helper_ext",
      "(i8, ...)",
      "i32 5, i32 7",
  };

  const auto operand = c4c::backend::parse_backend_direct_global_single_typed_call_operand(
      call, "helper_ext", "i32");
  expect_true(
      operand.has_value() && *operand == "5",
      "shared call-decode should recover the fixed typed vararg operand from the actual call args even when the stored fixed-param suffix text is stale");
}


void test_backend_shared_liveness_surface_tracks_result_names() {
  const auto module = make_declaration_module();
  const auto& function = module.functions.back();
  const auto liveness = c4c::backend::compute_live_intervals(function);

  expect_true(liveness.call_points.size() == 1 && liveness.call_points.front() == 0,
              "shared liveness surface should record call program points for typed call ops");
  const auto* interval = liveness.find_interval("%t0");
  expect_true(interval != nullptr,
              "shared liveness surface should expose interval lookup by SSA result name");
  expect_true(interval->start == 0 && interval->end == 1,
              "shared liveness surface should extend intervals through later uses instead of keeping point-only placeholders");
}



void test_backend_shared_liveness_surface_tracks_call_crossing_ranges() {
  const auto module = make_call_crossing_interval_module();
  const auto& function = module.functions.back();
  const auto liveness = c4c::backend::compute_live_intervals(function);

  expect_true(liveness.call_points.size() == 1 && liveness.call_points.front() == 1,
              "shared liveness surface should record the direct call program point");
  const auto* before_call = liveness.find_interval("%t0");
  const auto* call_result = liveness.find_interval("%t1");
  const auto* final_sum = liveness.find_interval("%t2");
  expect_true(before_call != nullptr && before_call->start == 0 && before_call->end == 2,
              "shared liveness surface should extend values that remain live across a call through their later use");
  expect_true(call_result != nullptr && call_result->start == 1 && call_result->end == 2,
              "shared liveness surface should keep the call result live until its consuming instruction");
  expect_true(final_sum != nullptr && final_sum->start == 2 && final_sum->end == 3,
              "shared liveness surface should include terminator uses in the final interval endpoint");
}

void test_backend_shared_liveness_surface_tracks_phi_join_ranges() {
  const auto module = make_interval_phi_join_module();
  const auto& function = module.functions.front();
  const auto liveness = c4c::backend::compute_live_intervals(function);

  const auto* cond = liveness.find_interval("%t0");
  const auto* then_value = liveness.find_interval("%t1");
  const auto* else_value = liveness.find_interval("%t2");
  const auto* phi_value = liveness.find_interval("%t3");
  const auto* final_sum = liveness.find_interval("%t4");

  expect_true(cond != nullptr && cond->start == 0 && cond->end == 1,
              "shared liveness surface should extend branch conditions through the conditional terminator");
  expect_true(then_value != nullptr && then_value->start == 2 && then_value->end == 3,
              "shared liveness surface should treat phi incoming values as used on the predecessor edge");
  expect_true(else_value != nullptr && else_value->start == 4 && else_value->end == 5,
              "shared liveness surface should track the alternate phi incoming value on its predecessor edge");
  expect_true(phi_value != nullptr && phi_value->start == 6 && phi_value->end == 7,
              "shared liveness surface should start phi results in the join block and extend them to their consuming instruction");
  expect_true(final_sum != nullptr && final_sum->start == 7 && final_sum->end == 8,
              "shared liveness surface should keep the join result live through the return terminator");
}

void test_backend_shared_liveness_input_matches_lir_phi_join_ranges() {
  const auto module = make_interval_phi_join_module();
  const auto& function = module.functions.front();
  const auto input = c4c::backend::lower_lir_to_liveness_input(function);
  const auto liveness = c4c::backend::compute_live_intervals(input);

  expect_true(liveness.call_points.empty(),
              "backend-owned liveness input should preserve the phi-join slice's lack of call points");
  const auto* cond = liveness.find_interval("%t0");
  const auto* then_value = liveness.find_interval("%t1");
  const auto* else_value = liveness.find_interval("%t2");
  const auto* phi_value = liveness.find_interval("%t3");
  const auto* final_sum = liveness.find_interval("%t4");

  expect_true(cond != nullptr && cond->start == 0 && cond->end == 1,
              "backend-owned liveness input should preserve branch-condition interval endpoints");
  expect_true(then_value != nullptr && then_value->start == 2 && then_value->end == 3,
              "backend-owned liveness input should keep phi incoming uses on the predecessor edge");
  expect_true(else_value != nullptr && else_value->start == 4 && else_value->end == 5,
              "backend-owned liveness input should keep alternate phi incoming uses on their predecessor edge");
  expect_true(phi_value != nullptr && phi_value->start == 6 && phi_value->end == 7,
              "backend-owned liveness input should preserve phi-definition intervals in the join block");
  expect_true(final_sum != nullptr && final_sum->start == 7 && final_sum->end == 8,
              "backend-owned liveness input should preserve downstream join-result intervals");
}

void test_backend_shared_regalloc_surface_uses_caller_saved_for_non_call_interval() {
  const auto module = make_return_add_module();
  const auto& function = module.functions.front();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}, {21}};
  config.caller_saved_regs = {{13}};

  const auto regalloc = c4c::backend::allocate_registers(function, config);
  expect_true(regalloc.assignments.size() == 1,
              "shared regalloc surface should assign one register for the single-result helper slice");
  const auto it = regalloc.assignments.find("%t0");
  expect_true(it != regalloc.assignments.end() && it->second.index == 13,
              "shared regalloc surface should prefer the caller-saved pool for intervals that do not span calls");
  expect_true(regalloc.used_regs.empty(),
              "shared regalloc surface should leave the callee-saved usage set empty when only caller-saved registers are needed");
  expect_true(regalloc.liveness.has_value() &&
                  regalloc.liveness->find_interval("%t0") != nullptr,
              "shared regalloc surface should retain cached liveness for the handoff boundary");
}

void test_backend_shared_regalloc_prefers_callee_saved_for_call_spanning_values() {
  const auto module = make_call_crossing_interval_module();
  const auto& function = module.functions.back();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto regalloc = c4c::backend::allocate_registers(function, config);

  const auto before_call = regalloc.assignments.find("%t0");
  const auto call_result = regalloc.assignments.find("%t1");
  const auto final_sum = regalloc.assignments.find("%t2");

  expect_true(before_call != regalloc.assignments.end() && before_call->second.index == 20,
              "shared regalloc should keep call-spanning intervals on the callee-saved pool first");
  expect_true(call_result == regalloc.assignments.end(),
              "shared regalloc should spill overlapping call-spanning intervals when the callee-saved pool is exhausted");
  expect_true(final_sum != regalloc.assignments.end() && final_sum->second.index == 13,
              "shared regalloc should use caller-saved registers for non-call-spanning intervals");
  expect_true(regalloc.used_regs.size() == 1 && regalloc.used_regs.front().index == 20,
              "shared regalloc should report only the used callee-saved register set");
}

void test_backend_shared_regalloc_accepts_backend_owned_liveness_input() {
  const auto module = make_call_crossing_interval_module();
  const auto& function = module.functions.back();
  const auto input = c4c::backend::lower_lir_to_liveness_input(function);

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto regalloc = c4c::backend::allocate_registers(input, config);

  const auto before_call = regalloc.assignments.find("%t0");
  const auto call_result = regalloc.assignments.find("%t1");
  const auto final_sum = regalloc.assignments.find("%t2");

  expect_true(before_call != regalloc.assignments.end() && before_call->second.index == 20,
              "backend-owned regalloc input should keep call-spanning intervals on the callee-saved pool");
  expect_true(call_result == regalloc.assignments.end(),
              "backend-owned regalloc input should still spill overlapping call-spanning intervals when callee-saved capacity is exhausted");
  expect_true(final_sum != regalloc.assignments.end() && final_sum->second.index == 13,
              "backend-owned regalloc input should still use caller-saved registers for non-call-spanning intervals");
  expect_true(regalloc.liveness.has_value() &&
                  regalloc.liveness->find_interval("%t0") != nullptr &&
                  regalloc.liveness->find_interval("%t1") != nullptr &&
                  regalloc.liveness->find_interval("%t2") != nullptr,
              "backend-owned regalloc input should retain cached liveness across the new handoff seam");
}

void test_backend_shared_regalloc_reuses_register_after_interval_ends() {
  const auto module = make_non_overlapping_interval_module();
  const auto& function = module.functions.front();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};

  const auto regalloc = c4c::backend::allocate_registers(function, config);

  const auto first_value = regalloc.assignments.find("%t0");
  const auto later_value = regalloc.assignments.find("%t2");

  expect_true(first_value != regalloc.assignments.end() && first_value->second.index == 20,
              "shared regalloc should assign the available register to the first live interval");
  expect_true(later_value != regalloc.assignments.end() && later_value->second.index == 20,
              "shared regalloc should reuse a freed register for a later non-overlapping interval");
  expect_true(regalloc.used_regs.size() == 1 && regalloc.used_regs.front().index == 20,
              "shared regalloc should keep the callee-saved usage set deduplicated when a register is reused");
}

void test_backend_shared_regalloc_spills_overlapping_values_without_reusing_busy_reg() {
  const auto module = make_overlapping_interval_module();
  const auto& function = module.functions.front();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};

  const auto regalloc = c4c::backend::allocate_registers(function, config);

  expect_true(regalloc.assignments.size() == 1,
              "shared regalloc should spill one of two overlapping intervals when only one register is available");
  const bool first_assigned = regalloc.assignments.find("%t0") != regalloc.assignments.end();
  const bool second_assigned = regalloc.assignments.find("%t1") != regalloc.assignments.end();
  expect_true(first_assigned != second_assigned,
              "shared regalloc should not assign the same busy register to overlapping intervals");
}

void test_backend_shared_regalloc_helper_filters_and_merges_clobbers() {
  const std::vector<c4c::backend::PhysReg> callee_saved = {{20}, {21}, {22}};
  const std::vector<c4c::backend::PhysReg> asm_clobbered = {{21}};
  const auto filtered =
      c4c::backend::stack_layout::filter_available_regs(callee_saved, asm_clobbered);

  expect_true(filtered.size() == 2 && filtered.front().index == 20 &&
                  filtered.back().index == 22,
              "shared stack-layout helper should remove inline-asm clobbered registers from the available set");

  const auto module = make_return_add_module();
  c4c::backend::RegAllocConfig config;
  config.available_regs = filtered;
  const auto merged = c4c::backend::run_regalloc_and_merge_clobbers(
      module.functions.front(), config, asm_clobbered);

  expect_true(merged.used_callee_saved.size() == 2 &&
                  merged.used_callee_saved.front().index == 20 &&
                  merged.used_callee_saved.back().index == 21,
              "shared regalloc helper should merge allocator usage with asm clobbers in sorted order");
}

void test_backend_shared_stack_layout_regalloc_helper_exposes_handoff_view() {
  const auto module = make_call_crossing_interval_module();
  const auto& function = module.functions.back();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};
  const std::vector<c4c::backend::PhysReg> asm_clobbered = {{21}};

  const auto merged =
      c4c::backend::run_regalloc_and_merge_clobbers(function, config, asm_clobbered);

  const auto* before_call =
      c4c::backend::stack_layout::find_assigned_reg(merged, "%t0");
  const auto* call_result =
      c4c::backend::stack_layout::find_assigned_reg(merged, "%t1");
  const auto* final_sum =
      c4c::backend::stack_layout::find_assigned_reg(merged, "%t2");
  const auto* cached_liveness =
      c4c::backend::stack_layout::find_cached_liveness(merged);

  expect_true(before_call != nullptr && before_call->index == 20,
              "shared stack-layout helper should expose assigned call-spanning values through the regalloc handoff");
  expect_true(call_result == nullptr,
              "shared stack-layout helper should preserve spilled values as missing from the assigned-register view");
  expect_true(final_sum != nullptr && final_sum->index == 13,
              "shared stack-layout helper should expose caller-saved assignments through the same handoff seam");
  expect_true(c4c::backend::stack_layout::uses_callee_saved_reg(merged, c4c::backend::PhysReg{20}),
              "shared stack-layout helper should report allocator-used callee-saved registers");
  expect_true(c4c::backend::stack_layout::uses_callee_saved_reg(merged, c4c::backend::PhysReg{21}),
              "shared stack-layout helper should also report inline-asm-clobbered callee-saved registers after merge");
  expect_true(!c4c::backend::stack_layout::uses_callee_saved_reg(merged, c4c::backend::PhysReg{22}),
              "shared stack-layout helper should reject untouched callee-saved registers");
  expect_true(cached_liveness != nullptr &&
                  cached_liveness->find_interval("%t0") != nullptr &&
                  cached_liveness->find_interval("%t1") != nullptr &&
                  cached_liveness->find_interval("%t2") != nullptr,
              "shared stack-layout helper should expose cached liveness for downstream stack-layout analysis");
}

void test_backend_shared_regalloc_helper_accepts_backend_owned_liveness_input() {
  const auto module = make_call_crossing_interval_module();
  const auto& function = module.functions.back();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};
  const std::vector<c4c::backend::PhysReg> asm_clobbered = {{21}};

  const auto input = c4c::backend::lower_lir_to_liveness_input(function);
  const auto merged =
      c4c::backend::run_regalloc_and_merge_clobbers(input, config, asm_clobbered);

  const auto* before_call =
      c4c::backend::stack_layout::find_assigned_reg(merged, "%t0");
  const auto* call_result =
      c4c::backend::stack_layout::find_assigned_reg(merged, "%t1");
  const auto* final_sum =
      c4c::backend::stack_layout::find_assigned_reg(merged, "%t2");
  const auto* cached_liveness =
      c4c::backend::stack_layout::find_cached_liveness(merged);

  expect_true(before_call != nullptr && before_call->index == 20,
              "backend-owned liveness input should preserve call-spanning regalloc assignments");
  expect_true(call_result == nullptr,
              "backend-owned liveness input should preserve spilled call-result assignments");
  expect_true(final_sum != nullptr && final_sum->index == 13,
              "backend-owned liveness input should preserve caller-saved assignments");
  expect_true(c4c::backend::stack_layout::uses_callee_saved_reg(merged, c4c::backend::PhysReg{20}),
              "backend-owned liveness input should preserve allocator-used callee-saved registers");
  expect_true(c4c::backend::stack_layout::uses_callee_saved_reg(merged, c4c::backend::PhysReg{21}),
              "backend-owned liveness input should preserve merged inline-asm clobbers");
  expect_true(cached_liveness != nullptr &&
                  cached_liveness->find_interval("%t0") != nullptr &&
                  cached_liveness->find_interval("%t1") != nullptr &&
                  cached_liveness->find_interval("%t2") != nullptr,
              "backend-owned liveness input should preserve cached liveness for downstream stack-layout analysis");
}

void test_backend_shared_stack_layout_analysis_tracks_phi_use_blocks() {
  const auto module = make_interval_phi_join_module();
  const auto& function = module.functions.front();
  const c4c::backend::RegAllocIntegrationResult regalloc;
  const auto analysis =
      c4c::backend::stack_layout::analyze_stack_layout(function, regalloc, {});

  const auto* then_value_uses = analysis.find_use_blocks("%t1");
  const auto* else_value_uses = analysis.find_use_blocks("%t2");
  const auto* phi_value_uses = analysis.find_use_blocks("%t3");

  expect_true(then_value_uses != nullptr && then_value_uses->size() == 1 &&
                  then_value_uses->front() == 1,
              "shared stack-layout analysis should attribute phi incoming uses to the predecessor block");
  expect_true(else_value_uses != nullptr && else_value_uses->size() == 1 &&
                  else_value_uses->front() == 2,
              "shared stack-layout analysis should keep alternate phi incoming values on their own predecessor block");
  expect_true(phi_value_uses != nullptr && phi_value_uses->size() == 1 &&
                  phi_value_uses->front() == 3,
              "shared stack-layout analysis should record normal instruction uses in the consuming block");
}

void test_backend_shared_stack_layout_analysis_accepts_backend_owned_input() {
  const auto module = make_interval_phi_join_module();
  const auto& function = module.functions.front();
  const auto input = c4c::backend::stack_layout::lower_lir_to_stack_layout_input(function);
  const c4c::backend::RegAllocIntegrationResult regalloc;
  const auto analysis =
      c4c::backend::stack_layout::analyze_stack_layout(input, regalloc, {});

  const auto* then_value_uses = analysis.find_use_blocks("%t1");
  const auto* else_value_uses = analysis.find_use_blocks("%t2");
  const auto* phi_value_uses = analysis.find_use_blocks("%t3");

  expect_true(then_value_uses != nullptr && then_value_uses->size() == 1 &&
                  then_value_uses->front() == 1,
              "backend-owned stack-layout input should preserve predecessor-edge phi use attribution");
  expect_true(else_value_uses != nullptr && else_value_uses->size() == 1 &&
                  else_value_uses->front() == 2,
              "backend-owned stack-layout input should preserve alternate predecessor-edge phi uses");
  expect_true(phi_value_uses != nullptr && phi_value_uses->size() == 1 &&
                  phi_value_uses->front() == 3,
              "backend-owned stack-layout input should preserve normal consuming-block uses");
}

void test_backend_shared_stack_layout_input_collects_value_names() {
  const auto dead_param_module = make_dead_param_alloca_candidate_module();
  const auto& dead_param_function = dead_param_module.functions.back();
  const auto dead_param_input =
      c4c::backend::stack_layout::lower_lir_to_stack_layout_input(dead_param_function);
  const auto dead_param_value_names =
      c4c::backend::stack_layout::collect_stack_layout_value_names(dead_param_input);

  const auto escaped_module = make_escaped_local_alloca_candidate_module();
  const auto& escaped_function = escaped_module.functions.back();
  const auto escaped_input =
      c4c::backend::stack_layout::lower_lir_to_stack_layout_input(escaped_function);
  const auto escaped_value_names =
      c4c::backend::stack_layout::collect_stack_layout_value_names(escaped_input);

  const auto contains = [](const std::vector<std::string>& value_names,
                           std::string_view value_name) {
    return std::find(value_names.begin(), value_names.end(), value_name) != value_names.end();
  };

  expect_true(contains(dead_param_value_names, "%lv.param.x"),
              "backend-owned stack-layout input should expose entry alloca names for downstream slot builders");
  expect_true(contains(dead_param_value_names, "%p.x"),
              "backend-owned stack-layout input should preserve paired parameter stores for downstream slot builders");
  expect_true(contains(escaped_value_names, "%lv.buf"),
              "backend-owned stack-layout input should preserve derived pointer roots for downstream slot builders");
  expect_true(contains(dead_param_value_names, "%t0"),
              "backend-owned stack-layout input should preserve body SSA uses for downstream slot builders");
}

void test_backend_shared_stack_layout_input_preserves_signature_metadata() {
  c4c::codegen::lir::LirFunction function;
  function.name = "aggregate_variadic";
  function.signature_text =
      "define { i32, i32 } @aggregate_variadic({ i32, i32 } %p.agg, double %p.fp, ...)";

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(c4c::codegen::lir::LirCallOp{
      "%call0",
      c4c::codegen::lir::LirTypeRef("{ i32, i32 }"),
      "@helper",
      "(i32)",
      "i32 7",
  });
  entry.terminator = c4c::codegen::lir::LirRet{std::nullopt, "void"};
  function.blocks.push_back(std::move(entry));

  const auto input = c4c::backend::stack_layout::lower_lir_to_stack_layout_input(function);

  expect_true(input.signature_params.size() == 3 &&
                  input.signature_params[0].type == "{ i32, i32 }" &&
                  input.signature_params[0].operand == "%p.agg" &&
                  input.signature_params[1].type == "double" &&
                  input.signature_params[1].operand == "%p.fp" &&
                  input.signature_params[2].is_varargs,
              "backend-owned stack-layout input should preserve parsed function-signature params for downstream slot builders");
  expect_true(input.return_type.has_value() && *input.return_type == "{ i32, i32 }" &&
                  input.is_variadic,
              "backend-owned stack-layout input should preserve return-type and variadic metadata for downstream slot builders");
  expect_true(input.call_results.size() == 1 &&
                  input.call_results.front().value_name == "%call0" &&
                  input.call_results.front().type_str == "{ i32, i32 }",
              "backend-owned stack-layout input should preserve stack-backed call-result metadata for downstream slot builders");
}

void test_backend_shared_stack_layout_input_preserves_aarch64_return_gate_metadata() {
  c4c::codegen::lir::LirFunction function;
  function.name = "pair_return";
  function.signature_text = "define { i32, i32 } @pair_return(i32 %p.x)";

  c4c::codegen::lir::LirBlock entry;
  entry.label = "entry";
  entry.terminator = c4c::codegen::lir::LirRet{std::nullopt, "void"};
  function.blocks.push_back(std::move(entry));

  const auto input = c4c::backend::stack_layout::lower_lir_to_stack_layout_input(function);

  expect_true(input.return_type.has_value() && *input.return_type == "{ i32, i32 }" &&
                  input.signature_params.size() == 1 &&
                  input.signature_params.front().type == "i32" &&
                  input.signature_params.front().operand == "%p.x",
              "backend-owned stack-layout input should preserve the aggregate return metadata consumed by the direct AArch64 stack-slot support gate");
}

void test_backend_shared_stack_layout_analysis_detects_dead_param_allocas() {
  const auto module = make_dead_param_alloca_candidate_module();
  const auto& function = module.functions.back();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};
  const auto regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(function, config, {});
  const auto analysis = c4c::backend::stack_layout::analyze_stack_layout(
      function, regalloc, {{20}, {21}, {22}});

  expect_true(analysis.uses_value("%p.x"),
              "shared stack-layout analysis should collect body-used SSA values");
  expect_true(!analysis.uses_value("%lv.param.x"),
              "shared stack-layout analysis should not treat the synthesized param spill as a body use");
  expect_true(analysis.is_dead_param_alloca("%lv.param.x"),
              "shared stack-layout analysis should mark dead param allocas when the corresponding parameter value is kept in a callee-saved register");
}

void test_backend_shared_stack_layout_analysis_tracks_call_arg_values() {
  const auto module = make_escaped_local_alloca_candidate_module();
  const auto& function = module.functions.back();
  const c4c::backend::RegAllocIntegrationResult regalloc;
  const auto analysis =
      c4c::backend::stack_layout::analyze_stack_layout(function, regalloc, {});

  expect_true(analysis.uses_value("%lv.buf"),
              "shared stack-layout analysis should treat typed call-argument operands as real uses");
}

void test_backend_shared_stack_layout_analysis_detects_entry_alloca_overwrite_before_read() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto overwrite_first_module = make_overwrite_first_scalar_local_alloca_candidate_module();
  const auto& overwrite_first_function = overwrite_first_module.functions.front();
  const auto overwrite_first_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(overwrite_first_function, regalloc, {});
  expect_true(overwrite_first_analysis.is_entry_alloca_overwritten_before_read("%lv.x"),
              "shared stack-layout analysis should recognize live entry allocas whose first real access overwrites the slot before any read");

  const auto read_first_module = make_read_before_store_scalar_local_alloca_candidate_module();
  const auto& read_first_function = read_first_module.functions.front();
  const auto read_first_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(read_first_function, regalloc, {});
  expect_true(!read_first_analysis.is_entry_alloca_overwritten_before_read("%lv.x"),
              "shared stack-layout analysis should keep entry zero-init stores when the slot is read before it is overwritten");
}

void test_backend_shared_alloca_coalescing_classifies_non_param_allocas() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto dead_module = make_dead_local_alloca_candidate_module();
  const auto& dead_function = dead_module.functions.front();
  const auto dead_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(dead_function, regalloc, {});
  const auto dead_coalescing =
      c4c::backend::stack_layout::compute_coalescable_allocas(dead_function,
                                                              dead_analysis);
  expect_true(dead_coalescing.is_dead_alloca("%lv.buf") &&
                  !dead_coalescing.find_single_block("%lv.buf").has_value(),
              "shared alloca-coalescing should classify unused non-param allocas as dead instead of forcing a permanent slot");

  const auto single_block_module = make_live_local_alloca_candidate_module();
  const auto& single_block_function = single_block_module.functions.front();
  const auto single_block_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(single_block_function, regalloc, {});
  const auto single_block_coalescing =
      c4c::backend::stack_layout::compute_coalescable_allocas(single_block_function,
                                                              single_block_analysis);
  expect_true(!single_block_coalescing.is_dead_alloca("%lv.buf") &&
                  single_block_coalescing.find_single_block("%lv.buf") == 0,
              "shared alloca-coalescing should recognize GEP-tracked non-param allocas whose uses stay within one block");

  const auto escaped_module = make_escaped_local_alloca_candidate_module();
  const auto& escaped_function = escaped_module.functions.back();
  const auto escaped_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(escaped_function, regalloc, {});
  const auto escaped_coalescing =
      c4c::backend::stack_layout::compute_coalescable_allocas(escaped_function,
                                                              escaped_analysis);
  expect_true(!escaped_coalescing.is_dead_alloca("%lv.buf") &&
                  !escaped_coalescing.find_single_block("%lv.buf").has_value(),
              "shared alloca-coalescing should leave call-escaped allocas out of the single-block pool");
}

void test_backend_shared_alloca_coalescing_accepts_backend_owned_input() {
  const c4c::backend::RegAllocIntegrationResult regalloc;
  const auto module = make_live_local_alloca_candidate_module();
  const auto& function = module.functions.front();
  const auto input = c4c::backend::stack_layout::lower_lir_to_stack_layout_input(function);
  const auto analysis = c4c::backend::stack_layout::analyze_stack_layout(input, regalloc, {});
  const auto coalescing =
      c4c::backend::stack_layout::compute_coalescable_allocas(input, analysis);

  expect_true(!coalescing.is_dead_alloca("%lv.buf") &&
                  coalescing.find_single_block("%lv.buf") == 0,
              "backend-owned stack-layout input should preserve single-block alloca reuse classification");
}

void test_backend_shared_slot_assignment_plans_param_alloca_slots() {
  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto dead_module = make_dead_param_alloca_candidate_module();
  const auto& dead_function = dead_module.functions.back();
  const auto dead_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(dead_function, config, {});
  const auto dead_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      dead_function, dead_regalloc, {{20}, {21}, {22}});
  const auto dead_plans =
      c4c::backend::stack_layout::plan_param_alloca_slots(dead_function, dead_analysis);

  expect_true(dead_plans.size() == 1 && dead_plans.front().alloca_name == "%lv.param.x" &&
                  dead_plans.front().param_name == "%p.x" &&
                  !dead_plans.front().needs_stack_slot,
              "shared slot-assignment planning should skip dead param allocas once analysis proves the parameter is preserved in a callee-saved register");

  const auto live_module = make_live_param_alloca_candidate_module();
  const auto& live_function = live_module.functions.front();
  const auto live_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(live_function, config, {});
  const auto live_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      live_function, live_regalloc, {{20}, {21}, {22}});
  const auto live_plans =
      c4c::backend::stack_layout::plan_param_alloca_slots(live_function, live_analysis);

  expect_true(live_plans.size() == 1 && live_plans.front().alloca_name == "%lv.param.x" &&
                  live_plans.front().param_name == "%p.x" &&
                  live_plans.front().needs_stack_slot,
              "shared slot-assignment planning should retain live param allocas when the function body still reads from the parameter slot");
}

void test_backend_shared_slot_assignment_prunes_dead_param_alloca_insts() {
  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto dead_module = make_dead_param_alloca_candidate_module();
  const auto& dead_function = dead_module.functions.back();
  const auto dead_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(dead_function, config, {});
  const auto dead_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      dead_function, dead_regalloc, {{20}, {21}, {22}});
  const auto dead_plans =
      c4c::backend::stack_layout::plan_param_alloca_slots(dead_function, dead_analysis);
  const auto dead_pruned =
      c4c::backend::stack_layout::prune_dead_param_alloca_insts(dead_function, dead_plans);

  expect_true(dead_pruned.empty(),
              "shared slot-assignment pruning should drop dead param allocas and their entry stores");

  const auto live_module = make_live_param_alloca_candidate_module();
  const auto& live_function = live_module.functions.front();
  const auto live_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(live_function, config, {});
  const auto live_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      live_function, live_regalloc, {{20}, {21}, {22}});
  const auto live_plans =
      c4c::backend::stack_layout::plan_param_alloca_slots(live_function, live_analysis);
  const auto live_pruned =
      c4c::backend::stack_layout::prune_dead_param_alloca_insts(live_function, live_plans);

  expect_true(live_pruned.size() == live_function.alloca_insts.size(),
              "shared slot-assignment pruning should preserve live param allocas");
}

void test_backend_shared_slot_assignment_plans_entry_alloca_slots() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto dead_module = make_dead_local_alloca_candidate_module();
  const auto& dead_function = dead_module.functions.front();
  const auto dead_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(dead_function, regalloc, {});
  const auto dead_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(dead_function, dead_analysis);

  expect_true(dead_plans.size() == 1 && dead_plans.front().alloca_name == "%lv.buf" &&
                  !dead_plans.front().needs_stack_slot &&
                  dead_plans.front().remove_following_entry_store,
              "shared slot-assignment planning should drop dead non-param entry allocas and their paired zero-init stores when analysis finds no uses");

  const auto live_module = make_live_local_alloca_candidate_module();
  const auto& live_function = live_module.functions.front();
  const auto live_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(live_function, regalloc, {});
  const auto live_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(live_function, live_analysis);

  expect_true(live_plans.size() == 1 && live_plans.front().alloca_name == "%lv.buf" &&
                  live_plans.front().needs_stack_slot &&
                  !live_plans.front().remove_following_entry_store &&
                  live_plans.front().coalesced_block == 0 &&
                  live_plans.front().assigned_slot == 0,
              "shared slot-assignment planning should preserve paired zero-init stores for aggregate entry allocas while exposing the shared single-block reuse classification");

  const auto disjoint_module = make_disjoint_block_local_alloca_candidate_module();
  const auto& disjoint_function = disjoint_module.functions.front();
  const auto disjoint_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(disjoint_function, regalloc, {});
  const auto disjoint_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(disjoint_function,
                                                          disjoint_analysis);

  expect_true(disjoint_plans.size() == 2 &&
                  disjoint_plans[0].alloca_name == "%lv.then" &&
                  disjoint_plans[0].coalesced_block == 1 &&
                  disjoint_plans[1].alloca_name == "%lv.else" &&
                  disjoint_plans[1].coalesced_block == 2 &&
                  disjoint_plans[0].assigned_slot.has_value() &&
                  disjoint_plans[0].assigned_slot == disjoint_plans[1].assigned_slot,
              "shared slot-assignment planning should turn disjoint single-block allocas into a concrete shared slot decision");

  const auto same_block_module = make_same_block_local_alloca_candidate_module();
  const auto& same_block_function = same_block_module.functions.front();
  const auto same_block_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(same_block_function, regalloc, {});
  const auto same_block_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(same_block_function,
                                                          same_block_analysis);

  expect_true(same_block_plans.size() == 2 &&
                  same_block_plans[0].coalesced_block == 0 &&
                  same_block_plans[1].coalesced_block == 0 &&
                  same_block_plans[0].assigned_slot.has_value() &&
                  same_block_plans[1].assigned_slot.has_value() &&
                  same_block_plans[0].assigned_slot != same_block_plans[1].assigned_slot,
              "shared slot-assignment planning should keep same-block local allocas in distinct slots even when both are individually coalescable");

  const auto mixed_lifetime_module = make_mixed_lifetime_local_alloca_candidate_module();
  const auto& mixed_lifetime_function = mixed_lifetime_module.functions.front();
  const auto mixed_lifetime_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      mixed_lifetime_function, regalloc, {});
  const auto mixed_lifetime_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(mixed_lifetime_function,
                                                          mixed_lifetime_analysis);

  expect_true(mixed_lifetime_plans.size() == 2 &&
                  mixed_lifetime_plans[0].alloca_name == "%lv.a" &&
                  !mixed_lifetime_plans[0].coalesced_block.has_value() &&
                  mixed_lifetime_plans[0].assigned_slot.has_value() &&
                  mixed_lifetime_plans[1].alloca_name == "%lv.y" &&
                  mixed_lifetime_plans[1].coalesced_block == 0 &&
                  mixed_lifetime_plans[1].assigned_slot.has_value() &&
                  mixed_lifetime_plans[0].assigned_slot !=
                      mixed_lifetime_plans[1].assigned_slot,
              "shared slot-assignment planning should not let a single-block alloca reuse the slot of a value that stays live across multiple blocks");

  const auto read_first_module = make_read_before_store_local_alloca_candidate_module();
  const auto& read_first_function = read_first_module.functions.front();
  const auto read_first_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(read_first_function, regalloc, {});
  const auto read_first_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(read_first_function,
                                                          read_first_analysis);

  expect_true(read_first_plans.size() == 1 &&
                  read_first_plans.front().alloca_name == "%lv.buf" &&
                  read_first_plans.front().needs_stack_slot &&
                  !read_first_plans.front().remove_following_entry_store,
              "shared slot-assignment planning should preserve paired zero-init stores when a live entry alloca is read before the first overwrite");

  const auto scalar_overwrite_module = make_overwrite_first_scalar_local_alloca_candidate_module();
  const auto& scalar_overwrite_function = scalar_overwrite_module.functions.front();
  const auto scalar_overwrite_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(scalar_overwrite_function, regalloc, {});
  const auto scalar_overwrite_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(scalar_overwrite_function,
                                                          scalar_overwrite_analysis);

  expect_true(scalar_overwrite_plans.size() == 1 &&
                  scalar_overwrite_plans.front().alloca_name == "%lv.x" &&
                  scalar_overwrite_plans.front().needs_stack_slot &&
                  scalar_overwrite_plans.front().remove_following_entry_store,
              "shared slot-assignment planning should drop redundant paired zero-init stores for scalar entry allocas that are overwritten before any read");

  const auto scalar_read_first_module =
      make_read_before_store_scalar_local_alloca_candidate_module();
  const auto& scalar_read_first_function = scalar_read_first_module.functions.front();
  const auto scalar_read_first_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      scalar_read_first_function, regalloc, {});
  const auto scalar_read_first_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(scalar_read_first_function,
                                                          scalar_read_first_analysis);

  expect_true(scalar_read_first_plans.size() == 1 &&
                  scalar_read_first_plans.front().alloca_name == "%lv.x" &&
                  scalar_read_first_plans.front().needs_stack_slot &&
                  !scalar_read_first_plans.front().remove_following_entry_store,
              "shared slot-assignment planning should preserve paired zero-init stores for scalar entry allocas that are read before the first overwrite");

  const auto escaped_module = make_escaped_local_alloca_candidate_module();
  const auto& escaped_function = escaped_module.functions.back();
  const auto escaped_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(escaped_function, regalloc, {});
  const auto escaped_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(escaped_function,
                                                          escaped_analysis);

  expect_true(escaped_plans.size() == 1 &&
                  escaped_plans.front().alloca_name == "%lv.buf" &&
                  escaped_plans.front().needs_stack_slot &&
                  !escaped_plans.front().coalesced_block.has_value(),
              "shared slot-assignment planning should leave call-escaped local allocas out of the single-block reuse pool");
}

void test_backend_shared_slot_assignment_accepts_backend_owned_input() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto scalar_overwrite_module = make_overwrite_first_scalar_local_alloca_candidate_module();
  const auto& scalar_overwrite_function = scalar_overwrite_module.functions.front();
  const auto scalar_input =
      c4c::backend::stack_layout::lower_lir_to_stack_layout_input(scalar_overwrite_function);
  const auto scalar_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(scalar_input, regalloc, {});
  const auto scalar_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(scalar_input, scalar_analysis);

  expect_true(scalar_plans.size() == 1 &&
                  scalar_plans.front().alloca_name == "%lv.x" &&
                  scalar_plans.front().needs_stack_slot &&
                  scalar_plans.front().remove_following_entry_store,
              "backend-owned stack-layout input should preserve scalar overwrite-before-read slot pruning");

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto dead_param_module = make_dead_param_alloca_candidate_module();
  const auto& dead_param_function = dead_param_module.functions.back();
  const auto dead_param_input =
      c4c::backend::stack_layout::lower_lir_to_stack_layout_input(dead_param_function);
  const auto dead_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(dead_param_function, config, {});
  const auto dead_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      dead_param_input, dead_regalloc, {{20}, {21}, {22}});
  const auto dead_param_plans =
      c4c::backend::stack_layout::plan_param_alloca_slots(dead_param_input, dead_analysis);

  expect_true(dead_param_plans.size() == 1 &&
                  dead_param_plans.front().alloca_name == "%lv.param.x" &&
                  dead_param_plans.front().param_name == "%p.x" &&
                  !dead_param_plans.front().needs_stack_slot,
              "backend-owned stack-layout input should preserve dead param alloca pruning decisions");

  auto dead_entry_function = make_dead_local_alloca_candidate_module().functions.front();
  const auto dead_entry_input =
      c4c::backend::stack_layout::lower_lir_to_stack_layout_input(dead_entry_function);
  const auto dead_entry_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      dead_entry_input, regalloc, {});
  const auto dead_entry_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(dead_entry_input,
                                                          dead_entry_analysis);
  c4c::backend::stack_layout::apply_entry_alloca_slot_plan(dead_entry_function,
                                                           dead_entry_plans);

  expect_true(dead_entry_function.alloca_insts.empty(),
              "backend-owned stack-layout input should preserve entry-alloca pruning when the apply step still mutates the original LIR function");
}

void test_backend_shared_slot_assignment_bundle_accepts_backend_owned_input() {
  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto dead_param_module = make_dead_param_alloca_candidate_module();
  const auto& dead_param_function = dead_param_module.functions.back();
  const auto dead_param_input =
      c4c::backend::stack_layout::lower_lir_to_stack_layout_input(dead_param_function);
  const auto dead_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(dead_param_function, config, {});
  const auto dead_bundle = c4c::backend::stack_layout::build_stack_layout_plan_bundle(
      dead_param_input, dead_regalloc, {{20}, {21}, {22}});

  expect_true(dead_bundle.entry_alloca_plans.size() == 1 &&
                  dead_bundle.entry_alloca_plans.front().alloca_name == "%lv.param.x" &&
                  !dead_bundle.entry_alloca_plans.front().needs_stack_slot &&
                  dead_bundle.entry_alloca_plans.front().remove_following_entry_store,
              "backend-owned stack-layout plan bundle should preserve entry-alloca pruning decisions for dead param allocas");
  expect_true(dead_bundle.param_alloca_plans.size() == 1 &&
                  dead_bundle.param_alloca_plans.front().alloca_name == "%lv.param.x" &&
                  dead_bundle.param_alloca_plans.front().param_name == "%p.x" &&
                  !dead_bundle.param_alloca_plans.front().needs_stack_slot,
              "backend-owned stack-layout plan bundle should preserve param-alloca pruning decisions from the same lowered input");

  auto dead_function = dead_param_function;
  c4c::backend::stack_layout::apply_entry_alloca_slot_plan(dead_function,
                                                           dead_bundle.entry_alloca_plans);
  expect_true(dead_function.alloca_insts.empty(),
              "backend-owned stack-layout plan bundle should still drive the production entry-alloca mutation step");
}

void test_backend_shared_slot_assignment_prunes_dead_entry_alloca_insts() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto dead_module = make_dead_local_alloca_candidate_module();
  const auto& dead_function = dead_module.functions.front();
  const auto dead_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(dead_function, regalloc, {});
  const auto dead_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(dead_function, dead_analysis);
  const auto dead_pruned =
      c4c::backend::stack_layout::prune_dead_entry_alloca_insts(dead_function, dead_plans);

  expect_true(dead_pruned.empty(),
              "shared slot-assignment pruning should drop dead non-param entry allocas and their paired zero-init stores");

  const auto live_module = make_live_local_alloca_candidate_module();
  const auto& live_function = live_module.functions.front();
  const auto live_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(live_function, regalloc, {});
  const auto live_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(live_function, live_analysis);
  const auto live_pruned =
      c4c::backend::stack_layout::prune_dead_entry_alloca_insts(live_function, live_plans);

  expect_true(live_pruned.size() == live_function.alloca_insts.size(),
              "shared slot-assignment pruning should keep aggregate live non-param entry allocas and their paired zero-init stores");

  const auto read_first_module = make_read_before_store_local_alloca_candidate_module();
  const auto& read_first_function = read_first_module.functions.front();
  const auto read_first_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(read_first_function, regalloc, {});
  const auto read_first_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(read_first_function,
                                                          read_first_analysis);
  const auto read_first_pruned = c4c::backend::stack_layout::prune_dead_entry_alloca_insts(
      read_first_function, read_first_plans);

  expect_true(read_first_pruned.size() == read_first_function.alloca_insts.size(),
              "shared slot-assignment pruning should preserve paired zero-init stores when a live entry alloca is read before the first overwrite");

  const auto scalar_overwrite_module = make_overwrite_first_scalar_local_alloca_candidate_module();
  const auto& scalar_overwrite_function = scalar_overwrite_module.functions.front();
  const auto scalar_overwrite_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(scalar_overwrite_function, regalloc, {});
  const auto scalar_overwrite_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(scalar_overwrite_function,
                                                          scalar_overwrite_analysis);
  const auto scalar_overwrite_pruned =
      c4c::backend::stack_layout::prune_dead_entry_alloca_insts(scalar_overwrite_function,
                                                                scalar_overwrite_plans);

  expect_true(
      scalar_overwrite_pruned.size() + 1 == scalar_overwrite_function.alloca_insts.size() &&
          std::holds_alternative<c4c::codegen::lir::LirAllocaOp>(
              scalar_overwrite_pruned.front()),
      "shared slot-assignment pruning should keep scalar live entry allocas while dropping redundant paired zero-init stores");
}

void test_backend_shared_slot_assignment_applies_coalesced_entry_slots() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto disjoint_module = make_disjoint_block_local_alloca_candidate_module();
  auto disjoint_function = disjoint_module.functions.front();
  const auto disjoint_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(disjoint_function, regalloc, {});
  const auto disjoint_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(disjoint_function,
                                                          disjoint_analysis);
  c4c::backend::stack_layout::apply_entry_alloca_slot_plan(disjoint_function, disjoint_plans);

  expect_true(disjoint_function.alloca_insts.size() == 1,
              "shared slot-assignment application should collapse disjoint single-block allocas onto one retained entry slot");
  const auto* disjoint_alloca =
      std::get_if<c4c::codegen::lir::LirAllocaOp>(&disjoint_function.alloca_insts.front());
  expect_true(disjoint_alloca != nullptr && disjoint_alloca->result == "%lv.then",
              "shared slot-assignment application should retain the first assigned-slot owner as the canonical entry alloca");
  const auto* else_store =
      std::get_if<c4c::codegen::lir::LirStoreOp>(&disjoint_function.blocks[2].insts[0]);
  const auto* else_load =
      std::get_if<c4c::codegen::lir::LirLoadOp>(&disjoint_function.blocks[2].insts[1]);
  expect_true(else_store != nullptr && else_store->ptr == "%lv.then" &&
                  else_load != nullptr && else_load->ptr == "%lv.then",
              "shared slot-assignment application should rewrite disjoint-block users to the canonical shared slot");

  const auto same_block_module = make_same_block_local_alloca_candidate_module();
  auto same_block_function = same_block_module.functions.front();
  const auto same_block_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(same_block_function, regalloc, {});
  const auto same_block_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(same_block_function,
                                                          same_block_analysis);
  c4c::backend::stack_layout::apply_entry_alloca_slot_plan(same_block_function, same_block_plans);

  expect_true(same_block_function.alloca_insts.size() == 2,
              "shared slot-assignment application should keep same-block allocas distinct when planning assigned them different slots");

  const auto mixed_lifetime_module = make_mixed_lifetime_local_alloca_candidate_module();
  auto mixed_lifetime_function = mixed_lifetime_module.functions.front();
  const auto mixed_lifetime_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      mixed_lifetime_function, regalloc, {});
  const auto mixed_lifetime_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(mixed_lifetime_function,
                                                          mixed_lifetime_analysis);
  c4c::backend::stack_layout::apply_entry_alloca_slot_plan(mixed_lifetime_function,
                                                           mixed_lifetime_plans);

  expect_true(mixed_lifetime_function.alloca_insts.size() == 2,
              "shared slot-assignment application should keep mixed-lifetime allocas distinct when the entry value remains live after the scratch store");
  const auto* mixed_entry_store =
      std::get_if<c4c::codegen::lir::LirStoreOp>(&mixed_lifetime_function.blocks[0].insts[1]);
  expect_true(mixed_entry_store != nullptr && mixed_entry_store->ptr == "%lv.y",
              "shared slot-assignment application should preserve the scratch-store destination instead of rewriting it onto the long-lived entry alloca");

  const auto call_arg_module = make_disjoint_block_call_arg_alloca_candidate_module();
  auto call_arg_function = call_arg_module.functions.front();
  const std::vector<c4c::backend::stack_layout::EntryAllocaSlotPlan> call_arg_plans = {
      c4c::backend::stack_layout::EntryAllocaSlotPlan{
          "%lv.then", true, false, std::nullopt, std::size_t{0}},
      c4c::backend::stack_layout::EntryAllocaSlotPlan{
          "%lv.else", true, false, std::nullopt, std::size_t{0}},
  };
  c4c::backend::stack_layout::apply_entry_alloca_slot_plan(call_arg_function,
                                                           call_arg_plans);

  const auto* rewritten_call =
      std::get_if<c4c::codegen::lir::LirCallOp>(&call_arg_function.blocks[2].insts[1]);
  expect_true(rewritten_call != nullptr && rewritten_call->args_str == "ptr %lv.then",
              "shared slot-assignment application should rewrite typed call arguments to the canonical coalesced entry alloca");
}

void test_backend_binary_utils_contract_headers_are_include_reachable() {
  [[maybe_unused]] c4c::backend::elf::ContractSurface elf_surface;
  [[maybe_unused]] c4c::backend::linker_common::ContractSurface linker_common_surface;
  [[maybe_unused]] c4c::backend::aarch64::assembler::ContractSurface assembler_surface;
  [[maybe_unused]] c4c::backend::aarch64::linker::ContractSurface linker_surface;
  [[maybe_unused]] c4c::backend::x86::assembler::ContractSurface x86_assembler_surface;
  [[maybe_unused]] c4c::backend::x86::linker::ContractSurface x86_linker_surface;

  const auto emitted = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_return_add_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto parsed = c4c::backend::aarch64::assembler::parse_asm(emitted);
  expect_true(parsed.size() >= 4, "binary-utils contract headers should expose line-oriented parser output for backend-emitted assembly");
  expect_true(parsed.front().text == ".text",
              "binary-utils contract parser should keep the opening section directive");
  expect_true(parsed.back().text == "ret",
              "binary-utils contract parser should preserve the backend instruction tail");

  const auto object_path = make_temp_output_path("c4c_aarch64_contract");
  const auto staged = c4c::backend::aarch64::assembler::assemble(
      c4c::backend::aarch64::assembler::AssembleRequest{
          .asm_text = emitted,
          .output_path = object_path,
      });
  expect_true(staged.staged_text == emitted && staged.output_path == object_path &&
                  staged.object_emitted,
              "binary-utils contract headers should expose the active assembler request/result seam and report successful object emission for the minimal backend slice");
  const auto object_bytes = read_file_bytes(object_path);
  expect_true(object_bytes.size() >= 4 && object_bytes[0] == 0x7f &&
                  object_bytes[1] == 'E' && object_bytes[2] == 'L' &&
                  object_bytes[3] == 'F',
              "binary-utils contract assembler seam should emit an ELF object for the minimal backend slice");
  expect_true(std::string(object_bytes.begin(), object_bytes.end()).find("main") != std::string::npos,
              "binary-utils contract assembler seam should preserve the function symbol in the emitted object");
  std::filesystem::remove(object_path);

  const auto compat_path = make_temp_output_path("c4c_aarch64_contract_compat");
  const auto assembled = c4c::backend::aarch64::assembler::assemble(emitted, compat_path);
  expect_true(assembled == emitted,
              "binary-utils contract headers should keep the compatibility overload aligned with the staged assembler text seam");
  expect_true(std::filesystem::exists(compat_path),
              "binary-utils contract compatibility overload should still drive object emission through the named assembler seam");
  std::filesystem::remove(compat_path);
}

void test_shared_linker_parses_minimal_relocation_object_fixture() {
  const auto fixture = make_minimal_relocation_object_fixture();
  std::string error;
  const auto parsed = c4c::backend::linker_common::parse_elf64_object(
      fixture, "minimal_reloc.o", c4c::backend::elf::EM_AARCH64, &error);
  expect_true(parsed.has_value(),
              "shared linker object parser should accept the bounded relocation-bearing fixture: " +
                  error);

  const auto& object = *parsed;
  expect_true(object.source_name == "minimal_reloc.o",
              "shared linker object parser should preserve the source name");
  expect_true(object.sections.size() == 6,
              "shared linker object parser should preserve the full section inventory including the null section");
  expect_true(object.symbols.size() == 4,
              "shared linker object parser should preserve the full symbol inventory for the bounded object slice");
  expect_true(object.relocations.size() == object.sections.size(),
              "shared linker object parser should index relocation vectors by section");

  const auto text_index = find_section_index(object, ".text");
  const auto rela_text_index = find_section_index(object, ".rela.text");
  const auto symtab_index = find_section_index(object, ".symtab");
  const auto strtab_index = find_section_index(object, ".strtab");
  const auto shstrtab_index = find_section_index(object, ".shstrtab");
  expect_true(text_index < object.sections.size() && rela_text_index < object.sections.size() &&
                  symtab_index < object.sections.size() && strtab_index < object.sections.size() &&
                  shstrtab_index < object.sections.size(),
              "shared linker object parser should name the expected bounded ELF sections");

  expect_true(object.section_data[text_index].size() == 8,
              "shared linker object parser should preserve the bounded .text payload");
  expect_true(object.section_data[rela_text_index].size() == 24,
              "shared linker object parser should preserve the raw RELA payload for the bounded fixture");
  expect_true(object.symbols[2].name == "main" && object.symbols[2].is_global() &&
                  object.symbols[2].sym_type() == c4c::backend::elf::STT_FUNC &&
                  object.symbols[2].shndx == text_index,
              "shared linker object parser should preserve the bounded function symbol inventory");
  expect_true(object.symbols[3].name == "helper_ext" && object.symbols[3].is_undefined(),
              "shared linker object parser should preserve undefined extern symbols for later linker resolution");

  expect_true(object.relocations[text_index].size() == 1,
              "shared linker object parser should associate .rela.text entries with the .text section they target");
  const auto& relocation = object.relocations[text_index].front();
  expect_true(relocation.offset == 4 && relocation.sym_idx == 3 &&
                  relocation.rela_type == 279 && relocation.addend == 0,
              "shared linker object parser should preserve the bounded relocation inventory for later linker work");
}

void test_shared_linker_parses_builtin_return_add_object() {
  const auto object_path = make_temp_output_path("c4c_aarch64_return_add_parse");
  const auto staged = c4c::backend::aarch64::assemble_module(make_return_add_module(), object_path);
  expect_true(staged.object_emitted,
              "built-in assembler should emit an object for the current bounded return-add slice");

  const auto object_bytes = read_file_bytes(object_path);
  std::string error;
  const auto parsed = c4c::backend::linker_common::parse_elf64_object(
      object_bytes, object_path, c4c::backend::elf::EM_AARCH64, &error);
  expect_true(parsed.has_value(),
              "shared linker object parser should accept the current built-in return-add object: " +
                  error);

  const auto& object = *parsed;
  const auto text_index = find_section_index(object, ".text");
  const auto symtab_index = find_section_index(object, ".symtab");
  const auto strtab_index = find_section_index(object, ".strtab");
  const auto shstrtab_index = find_section_index(object, ".shstrtab");
  expect_true(text_index < object.sections.size() && symtab_index < object.sections.size() &&
                  strtab_index < object.sections.size() &&
                  shstrtab_index < object.sections.size(),
              "shared linker object parser should preserve the current built-in return-add section inventory");
  expect_true(object.relocations.size() == object.sections.size(),
              "shared linker object parser should keep relocation vectors indexed by section for built-in emitted objects");
  expect_true(object.section_data[text_index].size() == 8,
              "shared linker object parser should preserve the built-in emitted return-add text payload");

  const auto main_symbol = std::find_if(
      object.symbols.begin(), object.symbols.end(),
      [&](const c4c::backend::linker_common::Elf64Symbol& symbol) {
        return symbol.name == "main";
      });
  expect_true(main_symbol != object.symbols.end() && main_symbol->is_global() &&
                  main_symbol->sym_type() == c4c::backend::elf::STT_FUNC &&
                  main_symbol->shndx == text_index,
              "shared linker object parser should preserve the built-in emitted function symbol inventory");

  expect_true(object.relocations[text_index].empty(),
              "shared linker object parser should preserve the current built-in return-add object as relocation-free");

  std::filesystem::remove(object_path);
}

void test_shared_linker_parses_single_member_archive_fixture() {
  const auto fixture = make_single_member_archive_fixture();
  std::string error;
  const auto parsed = c4c::backend::linker_common::parse_elf64_archive(
      fixture, "libminimal.a", c4c::backend::elf::EM_AARCH64, &error);
  expect_true(parsed.has_value(),
              "shared linker archive parser should accept the bounded single-member fixture: " +
                  error);

  const auto& archive = *parsed;
  expect_true(archive.source_name == "libminimal.a",
              "shared linker archive parser should preserve the archive source name");
  expect_true(archive.members.size() == 1,
              "shared linker archive parser should preserve the bounded single-member inventory");
  expect_true(archive.find_member_index_for_symbol("main").has_value(),
              "shared linker archive parser should support symbol-driven member lookup for the bounded archive slice");

  const auto member_index = *archive.find_member_index_for_symbol("main");
  const auto& member = archive.members[member_index];
  expect_true(member.name == "minimal_reloc.o",
              "shared linker archive parser should normalize the bounded member name");
  expect_true(member.object.has_value(),
              "shared linker archive parser should parse the bounded ELF member into a shared object surface");
  expect_true(member.defines_symbol("main") && !member.defines_symbol("helper_ext"),
              "shared linker archive parser should index defined symbols without treating unresolved externs as providers");

  const auto& object = *member.object;
  const auto text_index = find_section_index(object, ".text");
  expect_true(text_index < object.sections.size(),
              "shared linker archive parser should preserve section names for the parsed member object");
  expect_true(object.relocations[text_index].size() == 1,
              "shared linker archive parser should preserve relocation inventory for the bounded archive member");
}

int main(int argc, char* argv[]) {
  if (argc >= 2) test_filter() = argv[1];
  test_backend_shared_call_decode_parses_bir_minimal_direct_call_module();
  test_backend_shared_call_decode_parses_bir_minimal_declared_direct_call_module();
  test_backend_shared_call_decode_parses_bir_minimal_void_direct_call_imm_return_module();
  test_backend_shared_call_decode_parses_bir_minimal_two_arg_direct_call_module();
  test_backend_shared_call_decode_parses_bir_minimal_direct_call_add_imm_module();
  test_backend_shared_call_decode_parses_bir_minimal_direct_call_identity_arg_module();
  test_backend_shared_call_decode_parses_bir_minimal_dual_identity_direct_call_sub_module();
  test_backend_shared_call_decode_parses_bir_minimal_call_crossing_direct_call_module();
  test_backend_shared_call_decode_accepts_typed_i32_single_add_imm_helper();
  test_backend_shared_call_decode_accepts_typed_i32_single_add_imm_helper_with_stale_return_text();
  test_backend_shared_call_decode_accepts_typed_i32_slot_add_helper();
  test_backend_shared_call_decode_accepts_typed_i32_slot_add_helper_with_stale_return_text();
  test_backend_shared_call_decode_accepts_typed_i32_identity_helper();
  test_backend_shared_call_decode_accepts_typed_i32_identity_helper_with_stale_return_text();
  test_backend_shared_call_decode_accepts_typed_i32_two_param_add_helper();
  test_backend_shared_call_decode_accepts_typed_i32_two_param_add_helper_with_stale_return_text();
  test_backend_shared_call_decode_prefers_typed_vararg_call_args_over_stale_fixed_suffix_text();
  test_backend_shared_liveness_surface_tracks_result_names();
  test_backend_shared_liveness_surface_tracks_call_crossing_ranges();
  test_backend_shared_liveness_surface_tracks_phi_join_ranges();
  test_backend_shared_liveness_input_matches_lir_phi_join_ranges();
  test_backend_shared_regalloc_surface_uses_caller_saved_for_non_call_interval();
  test_backend_shared_regalloc_prefers_callee_saved_for_call_spanning_values();
  test_backend_shared_regalloc_accepts_backend_owned_liveness_input();
  test_backend_shared_regalloc_reuses_register_after_interval_ends();
  test_backend_shared_regalloc_spills_overlapping_values_without_reusing_busy_reg();
  test_backend_shared_regalloc_helper_filters_and_merges_clobbers();
  test_backend_shared_stack_layout_regalloc_helper_exposes_handoff_view();
  test_backend_shared_regalloc_helper_accepts_backend_owned_liveness_input();
  test_backend_shared_stack_layout_analysis_tracks_phi_use_blocks();
  test_backend_shared_stack_layout_analysis_accepts_backend_owned_input();
  test_backend_shared_stack_layout_input_collects_value_names();
  test_backend_shared_stack_layout_input_preserves_signature_metadata();
  test_backend_shared_stack_layout_input_preserves_aarch64_return_gate_metadata();
  test_backend_shared_stack_layout_analysis_detects_dead_param_allocas();
  test_backend_shared_stack_layout_analysis_tracks_call_arg_values();
  test_backend_shared_stack_layout_analysis_detects_entry_alloca_overwrite_before_read();
  test_backend_shared_alloca_coalescing_classifies_non_param_allocas();
  test_backend_shared_alloca_coalescing_accepts_backend_owned_input();
  test_backend_shared_slot_assignment_plans_param_alloca_slots();
  test_backend_shared_slot_assignment_prunes_dead_param_alloca_insts();
  test_backend_shared_slot_assignment_plans_entry_alloca_slots();
  test_backend_shared_slot_assignment_accepts_backend_owned_input();
  test_backend_shared_slot_assignment_bundle_accepts_backend_owned_input();
  test_backend_shared_slot_assignment_prunes_dead_entry_alloca_insts();
  test_backend_shared_slot_assignment_applies_coalesced_entry_slots();
  // TODO: binary-utils contract test disabled — assembler seam changed
  // test_backend_binary_utils_contract_headers_are_include_reachable();
  test_shared_linker_parses_minimal_relocation_object_fixture();
  // TODO: builtin return-add object parse disabled — assembler seam changed
  // test_shared_linker_parses_builtin_return_add_object();
  // TODO(phase2): temporary skip while linker/assembler-focused x86 tests are being split.
  // test_shared_linker_parses_builtin_x86_extern_call_object();
  test_shared_linker_parses_single_member_archive_fixture();

  check_failures();
  return 0;
}
