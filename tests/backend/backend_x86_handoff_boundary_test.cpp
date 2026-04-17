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

}  // namespace

int main() {
  const auto module = make_x86_return_constant_module();
  const auto prepared =
      c4c::backend::prepare::prepare_semantic_bir_module_with_options(module, Target::X86_64);
  const auto prepared_bir_text = bir::print(prepared.module);
  const auto expected_asm = expected_minimal_constant_return_asm("main", 7);

  const auto prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail("x86 prepared-module consumer did not emit the canonical minimal return asm");
  }

  const auto public_asm =
      c4c::backend::emit_target_bir_module(module, Target::X86_64);
  if (public_asm != prepared_asm) {
    return fail("public x86 BIR entry no longer routes through the x86 prepared-module consumer");
  }

  const auto generic_asm = c4c::backend::emit_module(
      BackendModuleInput{module}, BackendOptions{.target = Target::X86_64});
  if (generic_asm != public_asm) {
    return fail("generic backend emit path no longer routes x86 BIR input through emit_target_bir_module");
  }

  if (prepared_bir_text.find("bir.func @main() -> i32 {") == std::string::npos) {
    return fail("test fixture no longer prepares semantic BIR before routing into x86");
  }

  return 0;
}
