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
using c4c::codegen::lir::LirGepOp;
using c4c::codegen::lir::LirFunction;
using c4c::codegen::lir::LirInlineAsmOp;
using c4c::codegen::lir::LirBinOp;
using c4c::codegen::lir::LirLoadOp;
using c4c::codegen::lir::LirModule;
using c4c::codegen::lir::LirOperand;
using c4c::codegen::lir::LirRet;
using c4c::codegen::lir::LirAllocaOp;
using c4c::codegen::lir::LirAbsOp;
using c4c::codegen::lir::LirStackSaveOp;
using c4c::codegen::lir::LirStoreOp;
using c4c::codegen::lir::LirVaArgOp;

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

LirModule make_bad_indirect_call_module() {
  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  LirFunction function;
  function.name = "bad_indirect_call";
  function.signature_text = "define void @bad_indirect_call()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand(""),
      .return_type = "void",
      .callee = LirOperand("%callee"),
      .callee_type_suffix = "ptr",
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

LirModule make_bad_call_return_module() {
  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  LirFunction function;
  function.name = "bad_call_return";
  function.signature_text = "define void @bad_call_return()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand(""),
      .return_type = "{ i32, i32 }",
      .callee = LirOperand("@callee"),
      .callee_type_suffix = "()",
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

LirModule make_bad_memcpy_runtime_module() {
  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  LirFunction function;
  function.name = "bad_memcpy_runtime";
  function.signature_text = "define void @bad_memcpy_runtime()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand(""),
      .return_type = "void",
      .callee = LirOperand("@memcpy"),
      .callee_type_suffix = "(ptr, ptr, i64)",
      .args_str = "ptr @dst, ptr @src, i64 -1",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_memset_runtime_module() {
  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  LirFunction function;
  function.name = "bad_memset_runtime";
  function.signature_text = "define void @bad_memset_runtime()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand(""),
      .return_type = "void",
      .callee = LirOperand("@memset"),
      .callee_type_suffix = "(ptr, i8, i64)",
      .args_str = "ptr @dst, i8 7, i64 -1",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_variadic_runtime_module() {
  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  LirFunction function;
  function.name = "bad_variadic_runtime";
  function.signature_text = "define void @bad_variadic_runtime()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirVaArgOp{
      .result = LirOperand("@not_ssa"),
      .ap_ptr = LirOperand("%ap"),
      .type_str = "i32",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_stack_state_runtime_module() {
  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  LirFunction function;
  function.name = "bad_stack_state_runtime";
  function.signature_text = "define void @bad_stack_state_runtime()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirStackSaveOp{
      .result = LirOperand("@not_ssa"),
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_abs_runtime_module() {
  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  LirFunction function;
  function.name = "bad_abs_runtime";
  function.signature_text = "define void @bad_abs_runtime()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirAbsOp{
      .result = LirOperand("@not_ssa"),
      .arg = LirOperand("%x"),
      .int_type = "i32",
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

LirModule make_bad_scalar_binop_module() {
  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  LirFunction function;
  function.name = "bad_scalar_binop";
  function.signature_text = "define void @bad_scalar_binop()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{
      .result = LirOperand("@not_ssa"),
      .opcode = "add",
      .type_str = "i32",
      .lhs = LirOperand("%lhs"),
      .rhs = LirOperand("%rhs"),
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_gep_module() {
  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  LirFunction function;
  function.name = "bad_gep";
  function.signature_text = "define void @bad_gep()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("@not_ssa"),
      .element_type = "i32",
      .ptr = LirOperand("%ptr"),
      .indices = {"i32 0"},
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_store_module() {
  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  LirFunction function;
  function.name = "bad_store";
  function.signature_text = "define void @bad_store()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{
      .type_str = "i32",
      .val = LirOperand("%value"),
      .ptr = LirOperand(""),
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_load_module() {
  LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  LirFunction function;
  function.name = "bad_load";
  function.signature_text = "define void @bad_load()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirLoadOp{
      .result = LirOperand("@not_ssa"),
      .type_str = "i32",
      .ptr = LirOperand("%ptr"),
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
  constexpr std::string_view kModuleSummary =
      "currently admitted capability buckets covering function-signature, scalar-control-flow, scalar/local-memory (including scalar-cast/scalar-binop and alloca/gep/load/store local-memory), and local/global memory semantics, plus semantic call families (direct-call, indirect-call, and call-return) and explicit runtime or intrinsic families such as variadic, stack-state, absolute-value, memcpy, memset, and inline-asm placeholders";
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

  if (const int indirect_call_status = expect_failure_notes(
          make_bad_indirect_call_module(),
          kModuleSummary,
          "failed in semantic call family 'indirect-call semantic family'",
          "latest function failure: semantic lir_to_bir function 'bad_indirect_call' failed in semantic call family 'indirect-call semantic family'",
          "missing module capability-bucket summary note",
          "missing specific indirect-call function note",
          "missing module note carrying the indirect-call semantic family failure");
      indirect_call_status != 0) {
    return indirect_call_status;
  }

  if (const int call_return_status = expect_failure_notes(
          make_bad_call_return_module(),
          kModuleSummary,
          "failed in semantic call family 'call-return semantic family'",
          "latest function failure: semantic lir_to_bir function 'bad_call_return' failed in semantic call family 'call-return semantic family'",
          "missing module capability-bucket summary note",
          "missing specific call-return function note",
          "missing module note carrying the call-return semantic family failure");
      call_return_status != 0) {
    return call_return_status;
  }

  if (const int memcpy_status = expect_failure_notes(
          make_bad_memcpy_runtime_module(),
          kModuleSummary,
          "failed in runtime/intrinsic family 'memcpy runtime family'",
          "latest function failure: semantic lir_to_bir function 'bad_memcpy_runtime' failed in runtime/intrinsic family 'memcpy runtime family'",
          "missing module capability-bucket summary note",
          "missing specific memcpy runtime function note",
          "missing module note carrying the memcpy runtime family failure");
      memcpy_status != 0) {
    return memcpy_status;
  }

  if (const int memset_status = expect_failure_notes(
          make_bad_memset_runtime_module(),
          kModuleSummary,
          "failed in runtime/intrinsic family 'memset runtime family'",
          "latest function failure: semantic lir_to_bir function 'bad_memset_runtime' failed in runtime/intrinsic family 'memset runtime family'",
          "missing module capability-bucket summary note",
          "missing specific memset runtime function note",
          "missing module note carrying the memset runtime family failure");
      memset_status != 0) {
    return memset_status;
  }

  if (const int variadic_status = expect_failure_notes(
          make_bad_variadic_runtime_module(),
          kModuleSummary,
          "failed in runtime/intrinsic family 'variadic runtime family'",
          "latest function failure: semantic lir_to_bir function 'bad_variadic_runtime' failed in runtime/intrinsic family 'variadic runtime family'",
          "missing module capability-bucket summary note",
          "missing specific variadic runtime function note",
          "missing module note carrying the variadic runtime family failure");
      variadic_status != 0) {
    return variadic_status;
  }

  if (const int stack_state_status = expect_failure_notes(
          make_bad_stack_state_runtime_module(),
          kModuleSummary,
          "failed in runtime/intrinsic family 'stack-state runtime family'",
          "latest function failure: semantic lir_to_bir function 'bad_stack_state_runtime' failed in runtime/intrinsic family 'stack-state runtime family'",
          "missing module capability-bucket summary note",
          "missing specific stack-state runtime function note",
          "missing module note carrying the stack-state runtime family failure");
      stack_state_status != 0) {
    return stack_state_status;
  }

  if (const int abs_status = expect_failure_notes(
          make_bad_abs_runtime_module(),
          kModuleSummary,
          "failed in runtime/intrinsic family 'absolute-value intrinsic family'",
          "latest function failure: semantic lir_to_bir function 'bad_abs_runtime' failed in runtime/intrinsic family 'absolute-value intrinsic family'",
          "missing module capability-bucket summary note",
          "missing specific absolute-value runtime function note",
          "missing module note carrying the absolute-value runtime family failure");
      abs_status != 0) {
    return abs_status;
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

  if (const int scalar_binop_status = expect_failure_notes(
          make_bad_scalar_binop_module(),
          kModuleSummary,
          "failed in scalar-binop semantic family",
          "latest function failure: semantic lir_to_bir function 'bad_scalar_binop' failed in scalar-binop semantic family",
          "missing module capability-bucket summary note",
          "missing specific scalar-binop function note",
          "missing module note carrying the scalar-binop semantic family failure");
      scalar_binop_status != 0) {
    return scalar_binop_status;
  }

  if (const int gep_status = expect_failure_notes(
          make_bad_gep_module(),
          kModuleSummary,
          "failed in gep local-memory semantic family",
          "latest function failure: semantic lir_to_bir function 'bad_gep' failed in gep local-memory semantic family",
          "missing module capability-bucket summary note",
          "missing specific gep local-memory function note",
          "missing module note carrying the gep local-memory semantic family failure");
      gep_status != 0) {
    return gep_status;
  }

  if (const int store_status = expect_failure_notes(
          make_bad_store_module(),
          kModuleSummary,
          "failed in store local-memory semantic family",
          "latest function failure: semantic lir_to_bir function 'bad_store' failed in store local-memory semantic family",
          "missing module capability-bucket summary note",
          "missing specific store local-memory function note",
          "missing module note carrying the store local-memory semantic family failure");
      store_status != 0) {
    return store_status;
  }

  if (const int load_status = expect_failure_notes(
          make_bad_load_module(),
          kModuleSummary,
          "failed in load local-memory semantic family",
          "latest function failure: semantic lir_to_bir function 'bad_load' failed in load local-memory semantic family",
          "missing module capability-bucket summary note",
          "missing specific load local-memory function note",
          "missing module note carrying the load local-memory semantic family failure");
      load_status != 0) {
    return load_status;
  }
  return 0;
}
