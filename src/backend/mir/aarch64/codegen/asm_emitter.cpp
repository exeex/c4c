#include "asm_emitter.hpp"

#include "machine_printer.hpp"
#include "mir/printer.hpp"

#include <sstream>
#include <stdexcept>

namespace c4c::backend::aarch64::codegen {

std::string print_prepared_machine_nodes(
    const c4c::backend::prepare::PreparedBirModule& prepared) {
  const auto built = compile_prepared_module(prepared);
  if (!built.module.has_value()) {
    std::string message = "AArch64 backend assembly route could not build a prepared module";
    if (built.error.has_value() && !built.error->message.empty()) {
      message += ": ";
      message += built.error->message;
    }
    throw std::invalid_argument(message);
  }

  std::ostringstream assembly;
  assembly << "    .text\n";
  std::size_t machine_node_count = 0;
  for (const auto& function : built.module->functions) {
    if (c4c::backend::mir::empty(function.mir)) {
      continue;
    }
    ++machine_node_count;
    assembly << "    .globl " << function.label << "\n"
             << "    .type " << function.label << ", %function\n"
             << function.label << ":\n";
    const MachineInstructionPrinter target_printer;
    const auto printed = c4c::backend::mir::print_machine_function(function.mir,
                                                                   target_printer);
    if (!printed.ok) {
      throw std::invalid_argument("AArch64 backend assembly route reached the machine-node printer, "
                                  "but printing failed: " +
                                  printed.diagnostic);
    }
    assembly << printed.assembly
             << "    .size " << function.label << ", .-" << function.label << "\n";
  }
  if (machine_node_count == 0) {
    throw std::invalid_argument(
        "AArch64 backend assembly route reached the machine-node printer, but no selected printable machine nodes are available for this source input");
  }
  assembly << "    .section .note.GNU-stack,\"\",@progbits\n";
  return assembly.str();
}

}  // namespace c4c::backend::aarch64::codegen
