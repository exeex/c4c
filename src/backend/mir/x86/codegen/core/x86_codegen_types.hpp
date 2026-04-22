#pragma once

#include <string>

namespace c4c::backend::x86::core {

struct X86ModuleAssemblyText {
  std::string text;
};

struct X86PublicEntryResult {
  std::string target_triple;
  std::string assembly_text;
};

}  // namespace c4c::backend::x86::core
