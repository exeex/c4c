#include "src/backend/backend.hpp"
#include "src/backend/bir/bir_printer.hpp"
#include "src/backend/mir/x86/codegen/x86_codegen.hpp"
#include "src/codegen/lir/ir.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

namespace bir = c4c::backend::bir;
namespace lir = c4c::codegen::lir;
using c4c::backend::BackendModuleInput;
using c4c::backend::BackendOptions;

const c4c::TargetProfile& target_profile_from_module_triple(std::string_view target_triple,
                                                            c4c::TargetProfile& storage) {
  storage = c4c::target_profile_from_triple(
      target_triple.empty() ? c4c::default_host_target_triple() : target_triple);
  return storage;
}

c4c::TargetProfile x86_target_profile() {
  return c4c::default_target_profile(c4c::TargetArch::X86_64);
}

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

std::string expected_minimal_constant_return_asm(const char* function_name, int returned_value) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name +
         ":\n    mov eax, " + std::to_string(returned_value) + "\n    ret\n";
}

std::string expected_minimal_param_passthrough_asm(const char* function_name) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name +
         ":\n    mov eax, edi\n    ret\n";
}

std::string expected_minimal_param_add_immediate_asm(const char* function_name, int immediate) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name +
         ":\n    mov eax, edi\n    add eax, " + std::to_string(immediate) + "\n    ret\n";
}

std::string expected_minimal_param_sub_immediate_asm(const char* function_name, int immediate) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name +
         ":\n    mov eax, edi\n    sub eax, " + std::to_string(immediate) + "\n    ret\n";
}

std::string expected_minimal_param_mul_immediate_asm(const char* function_name, int immediate) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name +
         ":\n    mov eax, edi\n    imul eax, " + std::to_string(immediate) + "\n    ret\n";
}

std::string expected_minimal_param_and_immediate_asm(const char* function_name, int immediate) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name +
         ":\n    mov eax, edi\n    and eax, " + std::to_string(immediate) + "\n    ret\n";
}

std::string expected_minimal_param_or_immediate_asm(const char* function_name, int immediate) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name +
         ":\n    mov eax, edi\n    or eax, " + std::to_string(immediate) + "\n    ret\n";
}

std::string expected_minimal_param_xor_immediate_asm(const char* function_name, int immediate) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name +
         ":\n    mov eax, edi\n    xor eax, " + std::to_string(immediate) + "\n    ret\n";
}

std::string expected_minimal_param_shl_immediate_asm(const char* function_name, int immediate) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name +
         ":\n    mov eax, edi\n    shl eax, " + std::to_string(immediate) + "\n    ret\n";
}

std::string expected_minimal_param_lshr_immediate_asm(const char* function_name, int immediate) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name +
         ":\n    mov eax, edi\n    shr eax, " + std::to_string(immediate) + "\n    ret\n";
}

std::string expected_minimal_param_ashr_immediate_asm(const char* function_name, int immediate) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name +
         ":\n    mov eax, edi\n    sar eax, " + std::to_string(immediate) + "\n    ret\n";
}

std::string expected_minimal_param_eq_zero_branch_asm(const char* function_name,
                                                      const char* false_label,
                                                      int true_returned_value,
                                                      int false_returned_value) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name +
         ":\n    test edi, edi\n    jne .L" + function_name + "_" + false_label +
         "\n    mov eax, " + std::to_string(true_returned_value) + "\n    ret\n.L" +
         function_name + "_" + false_label + ":\n    mov eax, " +
         std::to_string(false_returned_value) + "\n    ret\n";
}

bir::Module make_x86_return_constant_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(7)};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_passthrough_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "id_i32";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "p.x"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_add_immediate_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "add_one";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "sum"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "sum"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_sub_immediate_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "sub_one";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "diff"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "diff"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_mul_immediate_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "mul_three";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Mul,
      .result = bir::Value::named(bir::TypeKind::I32, "product"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(3),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "product"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_and_immediate_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "mask_low_bits";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::And,
      .result = bir::Value::named(bir::TypeKind::I32, "masked"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(15),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "masked"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_xor_immediate_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "flip_masked_bits";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Xor,
      .result = bir::Value::named(bir::TypeKind::I32, "flipped"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(10),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "flipped"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_or_immediate_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "set_low_bits";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Or,
      .result = bir::Value::named(bir::TypeKind::I32, "widened"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(12),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "widened"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_shl_immediate_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "shift_left_three";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Shl,
      .result = bir::Value::named(bir::TypeKind::I32, "shifted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(3),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "shifted"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_lshr_immediate_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "shift_right_two";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::LShr,
      .result = bir::Value::named(bir::TypeKind::I32, "shifted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(2),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "shifted"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_ashr_immediate_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "shift_right_signed";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::AShr,
      .result = bir::Value::named(bir::TypeKind::I32, "shifted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(3),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "shifted"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_eq_zero_branch_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "branch_on_zero";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .true_label = "is_zero",
      .false_label = "is_nonzero",
  };

  bir::Block is_zero;
  is_zero.label = "is_zero";
  is_zero.terminator = bir::ReturnTerminator{
      .value = bir::Value::immediate_i32(7),
  };

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.terminator = bir::ReturnTerminator{
      .value = bir::Value::immediate_i32(11),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(is_zero));
  function.blocks.push_back(std::move(is_nonzero));
  module.functions.push_back(std::move(function));
  return module;
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

int check_route_outputs(const bir::Module& module,
                        const std::string& expected_asm,
                        const std::string& expected_bir_fragment,
                        const char* failure_context) {
  c4c::TargetProfile target_profile;
  const auto prepared =
      c4c::backend::prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto prepared_bir_text = bir::print(prepared.module);

  const auto prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer did not emit the canonical asm")
                    .c_str());
  }

  const auto public_asm = c4c::backend::emit_target_bir_module(module, target_profile);
  if (public_asm != prepared_asm) {
    return fail((std::string(failure_context) +
                 ": public x86 BIR entry no longer routes through the x86 prepared-module consumer")
                    .c_str());
  }

  const auto generic_asm = c4c::backend::emit_module(
      BackendModuleInput{module}, BackendOptions{.target_profile = x86_target_profile()});
  if (generic_asm != public_asm) {
    return fail((std::string(failure_context) +
                 ": generic backend emit path no longer routes x86 BIR input through emit_target_bir_module")
                    .c_str());
  }

  if (prepared_bir_text.find(expected_bir_fragment) == std::string::npos) {
    return fail((std::string(failure_context) +
                 ": test fixture no longer prepares the expected semantic BIR shape before routing into x86")
                    .c_str());
  }

  return 0;
}

int check_lir_route_outputs(const lir::LirModule& module,
                            const std::string& expected_asm,
                            const char* failure_context) {
  const auto public_asm =
      c4c::backend::emit_target_lir_module(module, module.target_profile);
  if (public_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": public x86 LIR entry did not route through the canonical x86 prepared-module consumer")
                    .c_str());
  }

  const auto generic_asm = c4c::backend::emit_module(
      BackendModuleInput{module}, BackendOptions{.target_profile = x86_target_profile()});
  if (generic_asm != public_asm) {
    return fail((std::string(failure_context) +
                 ": generic backend emit path no longer routes x86 LIR input through emit_target_lir_module")
                    .c_str());
  }

  return 0;
}

}  // namespace

int main() {
  if (const auto status =
          check_route_outputs(make_x86_return_constant_module(),
                              expected_minimal_constant_return_asm("main", 7),
                              "bir.func @main() -> i32 {",
                              "minimal immediate return route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_passthrough_module(),
                              expected_minimal_param_passthrough_asm("id_i32"),
                              "bir.func @id_i32(i32 p.x) -> i32 {",
                              "minimal i32 parameter passthrough route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_add_immediate_module(),
                              expected_minimal_param_add_immediate_asm("add_one", 1),
                              "bir.func @add_one(i32 p.x) -> i32 {",
                              "minimal i32 parameter add-immediate route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_sub_immediate_module(),
                              expected_minimal_param_sub_immediate_asm("sub_one", 1),
                              "bir.func @sub_one(i32 p.x) -> i32 {",
                              "minimal i32 parameter sub-immediate route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_mul_immediate_module(),
                              expected_minimal_param_mul_immediate_asm("mul_three", 3),
                              "bir.func @mul_three(i32 p.x) -> i32 {",
                              "minimal i32 parameter mul-immediate route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_and_immediate_module(),
                              expected_minimal_param_and_immediate_asm("mask_low_bits", 15),
                              "bir.func @mask_low_bits(i32 p.x) -> i32 {",
                              "minimal i32 parameter and-immediate route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_or_immediate_module(),
                              expected_minimal_param_or_immediate_asm("set_low_bits", 12),
                              "bir.func @set_low_bits(i32 p.x) -> i32 {",
                              "minimal i32 parameter or-immediate route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_xor_immediate_module(),
                              expected_minimal_param_xor_immediate_asm("flip_masked_bits", 10),
                              "bir.func @flip_masked_bits(i32 p.x) -> i32 {",
                              "minimal i32 parameter xor-immediate route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_shl_immediate_module(),
                              expected_minimal_param_shl_immediate_asm("shift_left_three", 3),
                              "bir.func @shift_left_three(i32 p.x) -> i32 {",
                              "minimal i32 parameter shl-immediate route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_lshr_immediate_module(),
                              expected_minimal_param_lshr_immediate_asm("shift_right_two", 2),
                              "bir.func @shift_right_two(i32 p.x) -> i32 {",
                              "minimal i32 parameter logical-right-shift-immediate route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_ashr_immediate_module(),
                              expected_minimal_param_ashr_immediate_asm("shift_right_signed", 3),
                              "bir.func @shift_right_signed(i32 p.x) -> i32 {",
                              "minimal i32 parameter arithmetic-right-shift-immediate route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_eq_zero_branch_module(),
                              expected_minimal_param_eq_zero_branch_asm("branch_on_zero",
                                                                        "is_nonzero",
                                                                        7,
                                                                        11),
                              "bir.func @branch_on_zero(i32 p.x) -> i32 {",
                              "minimal i32 parameter compare-against-zero branch route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_lir_route_outputs(make_x86_return_constant_lir_module(),
                                  expected_minimal_constant_return_asm("main", 7),
                                  "minimal immediate return LIR route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_lir_route_outputs(make_x86_param_add_immediate_lir_module(),
                                  expected_minimal_param_add_immediate_asm("add_one", 1),
                                  "minimal i32 parameter add-immediate LIR route");
      status != 0) {
    return status;
  }

  return 0;
}
