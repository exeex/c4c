#include "src/backend/lowering/lir_to_bir.hpp"

#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

namespace {

using c4c::backend::BirLoweringNote;
using c4c::backend::BirLoweringOptions;
using c4c::backend::try_lower_to_bir_with_options;
using c4c::codegen::lir::LirBlock;
using c4c::codegen::lir::LirCastKind;
using c4c::codegen::lir::LirCastOp;
using c4c::codegen::lir::LirCallOp;
using c4c::codegen::lir::LirFunction;
using c4c::codegen::lir::LirInlineAsmOp;
using c4c::codegen::lir::LirModule;
using c4c::codegen::lir::LirOperand;
using c4c::codegen::lir::LirRet;
using c4c::codegen::lir::LirAllocaOp;

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

int expect_failure_notes(const LirModule& module,
                         std::string_view module_summary,
                         std::string_view function_failure,
                         std::string_view module_failure,
                         const char* missing_module_summary,
                         const char* missing_function_note,
                         const char* missing_module_note) {
  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail("expected semantic BIR lowering to fail");
  }
  if (!contains_note(result.notes, "module", module_summary)) {
    return fail(missing_module_summary);
  }
  if (!contains_note(result.notes, "function", function_failure)) {
    return fail(missing_function_note);
  }
  if (!contains_note(result.notes, "module", module_failure)) {
    return fail(missing_module_note);
  }
  return 0;
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

LirModule make_bad_direct_call_module() {
  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  LirFunction function;
  function.name = "bad_direct_call";
  function.signature_text = "define void @bad_direct_call()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand(""),
      .return_type = "void",
      .callee = LirOperand("@callee"),
      .callee_type_suffix = "(ptr)",
      .args_str = "ptr @missing_object",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_scalar_cast_module() {
  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  LirFunction function;
  function.name = "bad_scalar_cast";
  function.signature_text = "define void @bad_scalar_cast()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("@not_ssa"),
      .kind = LirCastKind::PtrToInt,
      .from_type = "ptr",
      .operand = LirOperand("@object"),
      .to_type = "i64",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_alloca_module() {
  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  LirFunction function;
  function.name = "bad_alloca";
  function.signature_text = "define void @bad_alloca()";
  function.alloca_insts.push_back(LirAllocaOp{
      .result = LirOperand("@not_ssa"),
      .type_str = "i32",
      .count = LirOperand(""),
      .align = 0,
  });

  LirBlock entry;
  entry.label = "entry";
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
  constexpr std::string_view kModuleSummary =
      "currently admitted capability buckets covering function-signature, scalar-control-flow, scalar/local-memory (including scalar-cast/scalar-binop and alloca/gep/load/store local-memory), and local/global memory semantics, plus semantic call families (direct-call, indirect-call, and call-return) and explicit runtime or intrinsic families such as memcpy, memset, and inline-asm placeholders";
  if (const int inline_asm_status = expect_failure_notes(
          make_unsupported_inline_asm_module(),
          kModuleSummary,
          "failed in runtime/intrinsic family 'inline-asm placeholder family'",
          "latest function failure: semantic lir_to_bir function 'bad_inline_asm' failed in runtime/intrinsic family 'inline-asm placeholder family'",
          "missing module capability-bucket summary note",
          "missing specific runtime family function note",
          "missing module note carrying the runtime family failure");
      inline_asm_status != 0) {
    return inline_asm_status;
  }

  if (const int direct_call_status = expect_failure_notes(
          make_bad_direct_call_module(),
          kModuleSummary,
          "failed in semantic call family 'direct-call semantic family'",
          "latest function failure: semantic lir_to_bir function 'bad_direct_call' failed in semantic call family 'direct-call semantic family'",
          "missing module capability-bucket summary note",
          "missing specific semantic-call function note",
          "missing module note carrying the semantic-call family failure");
      direct_call_status != 0) {
    return direct_call_status;
  }

  if (const int scalar_cast_status = expect_failure_notes(
          make_bad_scalar_cast_module(),
          kModuleSummary,
          "failed in scalar-cast semantic family",
          "latest function failure: semantic lir_to_bir function 'bad_scalar_cast' failed in scalar-cast semantic family",
          "missing module capability-bucket summary note",
          "missing specific scalar-cast function note",
          "missing module note carrying the scalar-cast semantic family failure");
      scalar_cast_status != 0) {
    return scalar_cast_status;
  }

  if (const int alloca_status = expect_failure_notes(
          make_bad_alloca_module(),
          kModuleSummary,
          "failed in alloca local-memory semantic family",
          "latest function failure: semantic lir_to_bir function 'bad_alloca' failed in alloca local-memory semantic family",
          "missing module capability-bucket summary note",
          "missing specific alloca local-memory function note",
          "missing module note carrying the alloca local-memory semantic family failure");
      alloca_status != 0) {
    return alloca_status;
  }
  return 0;
}
