#include "prepared_module_emit.hpp"

#include "emit.hpp"
#include "prepared_function_emit.hpp"
#include "prepared_global_memory_emit.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace c4c::backend::riscv::codegen {

std::string emit_prepared_module_text(
    const c4c::backend::prepare::PreparedBirModule& module) {
  namespace prepare = c4c::backend::prepare;

  std::string out;
  if (!append_prepared_global_storage_asm(out, module)) {
    throw std::invalid_argument(
        "riscv prepared module emitter does not support this prepared global storage layout");
  }
  out += "    .text\n";
  for (const auto& function : module.control_flow.functions) {
    const auto lookups = prepare::make_prepared_function_lookups(module, function);
    const auto function_name = prepare::prepared_function_name(module.names,
                                                              function.function_name);
    if (!function_name.empty()) {
      out += "    .globl ";
      out += function_name;
      out += "\n";
      out += function_name;
      out += ":\n";
    }

    const auto function_it = std::find_if(
        module.module.functions.begin(),
        module.module.functions.end(),
        [&](const c4c::backend::bir::Function& candidate) {
          return candidate.name == function_name;
        });
    if (function_it != module.module.functions.end() &&
        append_simple_prepared_bir_function_asm(out, module, &lookups, *function_it)) {
      continue;
    }

    for (const auto& publication : lookups.edge_publications.publications) {
      if (publication.status != prepare::PreparedEdgePublicationLookupStatus::Available) {
        continue;
      }
      (void)append_edge_publication_move_instruction(out,
                                                     &lookups,
                                                     publication.predecessor_label,
                                                     publication.successor_label,
                                                     publication.destination_value_id);
    }
  }
  return out;
}

}  // namespace c4c::backend::riscv::codegen
