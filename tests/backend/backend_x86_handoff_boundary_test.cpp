#include "src/backend/backend.hpp"
#include "src/backend/bir/bir_printer.hpp"

#include <cstdlib>
#include <iostream>

namespace {

namespace bir = c4c::backend::bir;
using c4c::backend::BackendModuleInput;
using c4c::backend::BackendOptions;
using c4c::backend::Target;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
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
  const auto prepared = c4c::backend::prepare_bir_module_for_target(module, Target::X86_64);
  const auto prepared_bir_output = bir::print(prepared);

  const auto public_bir_output = c4c::backend::emit_target_bir_module(module, Target::X86_64);
  if (public_bir_output == prepared_bir_output) {
    return fail("public x86 BIR entry still stops at prepared semantic BIR text");
  }

  const auto generic_output =
      c4c::backend::emit_module(BackendModuleInput{module}, BackendOptions{.target = Target::X86_64});
  if (generic_output != public_bir_output) {
    return fail("generic backend emit path no longer routes x86 BIR input through emit_target_bir_module");
  }

  if (public_bir_output.find(".globl main\n") == std::string::npos ||
      public_bir_output.find("main:\n") == std::string::npos) {
    return fail("public x86 BIR entry no longer emits x86 function assembly");
  }

  if (public_bir_output.find("movl $7, %eax\n") == std::string::npos ||
      public_bir_output.find("    ret\n") == std::string::npos) {
    return fail("public x86 BIR entry no longer emits the direct-return x86 slice");
  }

  if (public_bir_output.find("bir.func @main() -> i32 {") != std::string::npos) {
    return fail("public x86 BIR entry still exposes prepared semantic BIR text");
  }

  return 0;
}
