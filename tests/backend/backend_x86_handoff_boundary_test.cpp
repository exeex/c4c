#include "src/backend/backend.hpp"
#include "src/backend/bir/bir_printer.hpp"
#include "src/backend/mir/x86/codegen/x86_codegen.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

namespace bir = c4c::backend::bir;
using c4c::backend::BackendModuleInput;
using c4c::backend::BackendOptions;
using c4c::backend::Target;

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

int check_route_outputs(const bir::Module& module,
                        const std::string& expected_asm,
                        const std::string& expected_bir_fragment,
                        const char* failure_context) {
  const auto prepared =
      c4c::backend::prepare::prepare_semantic_bir_module_with_options(module, Target::X86_64);
  const auto prepared_bir_text = bir::print(prepared.module);

  const auto prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer did not emit the canonical asm")
                    .c_str());
  }

  const auto public_asm = c4c::backend::emit_target_bir_module(module, Target::X86_64);
  if (public_asm != prepared_asm) {
    return fail((std::string(failure_context) +
                 ": public x86 BIR entry no longer routes through the x86 prepared-module consumer")
                    .c_str());
  }

  const auto generic_asm = c4c::backend::emit_module(
      BackendModuleInput{module}, BackendOptions{.target = Target::X86_64});
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

  return 0;
}
