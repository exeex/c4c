#include "src/backend/lowering/lir_to_bir.hpp"

#include <iostream>
#include <string_view>
#include <vector>

namespace {

using c4c::backend::BirLoweringNote;
using c4c::backend::BirLoweringOptions;
using c4c::backend::try_lower_to_bir_with_options;
using c4c::codegen::lir::LirBlock;
using c4c::codegen::lir::LirFunction;
using c4c::codegen::lir::LirInlineAsmOp;
using c4c::codegen::lir::LirModule;
using c4c::codegen::lir::LirOperand;
using c4c::codegen::lir::LirRet;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

bool contains_note(const std::vector<BirLoweringNote>& notes,
                   std::string_view phase,
                   std::string_view needle) {
  for (const auto& note : notes) {
    if (note.phase == phase && note.message.find(needle) != std::string::npos) {
      return true;
    }
  }
  return false;
}

LirModule make_unsupported_inline_asm_module() {
  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  LirFunction function;
  function.name = "bad_inline_asm";
  function.signature_text = "define void @bad_inline_asm()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirInlineAsmOp{
      .result = LirOperand("%t0"),
      .ret_type = "{ i32, i32 }",
      .asm_text = "",
      .constraints = "=r,=r",
      .side_effects = true,
      .args_str = "",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

}  // namespace

int main() {
  auto result = try_lower_to_bir_with_options(
      make_unsupported_inline_asm_module(), BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail("expected unsupported inline asm lowering to fail");
  }
  if (!contains_note(result.notes,
                     "function",
                     "failed in runtime/intrinsic family 'inline-asm placeholder family'")) {
    return fail("missing specific function-family lowering note");
  }
  if (!contains_note(result.notes,
                     "module",
                     "latest function failure: semantic lir_to_bir function 'bad_inline_asm' failed in runtime/intrinsic family 'inline-asm placeholder family'")) {
    return fail("missing module note carrying the latest semantic family failure");
  }
  return 0;
}
