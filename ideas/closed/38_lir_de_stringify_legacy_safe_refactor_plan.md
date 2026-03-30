# LIR De-Stringify Legacy-Safe Refactor Plan

## Status

Completed on 2026-03-30 and ready for archive.

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Should precede:

- `ideas/open/35_backend_ready_ir_contract_plan.md`
- `ideas/open/36_lir_to_backend_ready_ir_lowering_plan.md`
- `ideas/open/37_backend_emitter_backend_ir_migration_plan.md`
- `ideas/open/34_backend_x86_c_testsuite_backend_convergence_plan.md`

## Goal

Refactor `src/codegen/lir/ir.hpp` so LIR no longer carries core opcode, type,
and operand semantics primarily through ad hoc `std::string` fields, while
preserving the current legacy LLVM-printing path during the transition.

## Why This Slice Exists

The current LIR surface is split between:

- early typed nodes using `LirValueId`, `TypeSpec`, and enums
- later string-heavy nodes such as `LirLoadOp`, `LirStoreOp`, `LirBinOp`,
  `LirCmpOp`, `LirCallOp`, `LirMemcpyOp`, `LirVa*Op`, and other LLVM-shaped
  payloads

That creates a fake IR boundary where downstream code must decode text-like
protocols instead of consuming structured compiler data. The result is visible
in:

- `src/codegen/lir/lir_printer.cpp`, which mostly reprints pre-encoded LLVM
  details rather than rendering typed IR
- `src/backend/lir_adapter.cpp`, which repeatedly branches on values like
  `opcode == "add"` and `type_str == "i32"`
- backend analysis files that scrape textual operand payloads because operand
  kinds are not modeled directly

Before the backend can have a clean backend-ready IR boundary, LIR itself needs
to stop leaking string-carried semantics into both the legacy LLVM path and the
new backend path.

## Primary Scope

- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/hir_to_lir.hpp`
- `src/codegen/lir/hir_to_lir.cpp`
- `src/codegen/lir/stmt_emitter.hpp`
- `src/codegen/lir/stmt_emitter.cpp`
- `src/codegen/lir/lir_printer.hpp`
- `src/codegen/lir/lir_printer.cpp`
- `src/backend/lir_adapter.hpp`
- `src/backend/lir_adapter.cpp`
- backend analysis helpers that currently consume string-heavy LIR fields

## Proposed Files

Add:

- `src/codegen/lir/operands.hpp`
- `src/codegen/lir/operands.cpp`
- `src/codegen/lir/types.hpp`
- `src/codegen/lir/types.cpp`
- `src/codegen/lir/verify.hpp`
- `src/codegen/lir/verify.cpp`

Update heavily:

- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/lir_printer.cpp`
- `src/codegen/lir/stmt_emitter.cpp`
- `src/backend/lir_adapter.cpp`

## Proposed API Surface

Introduce typed LIR building blocks:

- `struct LirOperand`
- `struct LirTypedOperand`
- `struct LirTypeRef`
- `enum class LirBinaryOpcode`
- `enum class LirCmpPredicate`
- `enum class LirCallKind`

Add validation helpers:

- `bool verify_lir_module(const LirModule& module, std::string* error);`
- `bool verify_lir_function(const LirFunction& function, std::string* error);`

Expected direction for affected instructions:

- `LirMemcpyOp` should use typed pointer/size operands, not raw strings
- `LirVaStartOp`, `LirVaEndOp`, `LirVaCopyOp` should use typed pointer operands
- `LirStackSaveOp` / `LirStackRestoreOp` should use typed value references
- `LirAbsOp` should use typed opcode + operand + result, not `int_type` text
- `LirLoadOp`, `LirStoreOp`, `LirBinOp`, `LirCmpOp`, `LirCallOp`, `LirPhiOp`,
  and related ops should stop encoding core semantics in free-form strings

## Required Migration Strategy

Because this refactor touches the legacy LLVM path, it must be staged:

1. add typed operand/type wrappers alongside existing fields where needed
2. switch construction sites in `stmt_emitter.cpp` and `hir_to_lir.cpp`
3. switch `lir_printer.cpp` to render from typed fields
4. switch backend consumers to typed LIR access
5. delete compatibility string fields only after both legacy printing and
   backend lowering no longer rely on them

The refactor is not allowed to break the current LLVM text output path while the
backend-ready IR work is still in progress.

## Required Behavior

This idea must leave the repo in a state where:

- opcode dispatch no longer depends on raw string compares for core arithmetic
  and comparison ops
- pointer/value/global/immediate operands can be distinguished structurally
- LIR printers and backends do not need to reconstruct operand kind from text
- legacy LLVM printing still works for the supported LIR surface
- any remaining textual fields are limited to genuinely textual payloads such as
  inline assembly source or pre-rendered declarations that have not yet been
  normalized

## Reference Files

Use these files as design references for "typed IR first, printing later":

- `ref/claudes-c-compiler/src/ir/README.md`
- `ref/claudes-c-compiler/src/ir/instruction.rs`
- `ref/claudes-c-compiler/src/ir/module.rs`
- `ref/claudes-c-compiler/src/ir/ops.rs`

Use local files as migration references for the legacy path:

- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/lir_printer.cpp`
- `src/codegen/lir/stmt_emitter.cpp`
- `src/backend/lir_adapter.cpp`

## Non-Goals

- no backend-ready IR contract in this idea
- no mem2reg or phi elimination in this idea
- no target-specific x86/AArch64 feature expansion in this idea
- no broad testsuite convergence work during this refactor

## Validation

- legacy `print_llvm(const LirModule&)` still emits valid LLVM IR for existing
  supported cases
- typed LIR verification catches malformed operand/type combinations early
- backend and analysis code touched by the refactor compiles against typed LIR
  accessors instead of raw string protocols
- any temporary compatibility fields are tracked explicitly in `plan_todo.md`

## Success Condition

This idea is complete when LIR is once again a structured IR instead of a
mixed text protocol, the legacy LLVM path still works, and follow-on backend IR
work can build on typed LIR rather than string-heavy shims.

## Completion Notes

Completed work established a typed-LIR compatibility layer without breaking the
legacy LLVM printer path:

- typed operand/type/opcode/predicate wrappers landed in `src/codegen/lir`
  and now back the main arithmetic/comparison backend consumers
- shared typed-call parse/format/rewrite helpers in
  `src/codegen/lir/call_args.hpp` replaced repeated ad hoc call-string decoding
  across LIR construction, printing, analysis, and backend adapter code
- x86/AArch64 residual direct-call fast paths now use the shared
  `src/backend/lir_adapter.hpp` helper seams rather than emitter-local typed
  call text decoding

Leftover compatibility debt intentionally deferred to ideas `35`, `36`, and
`37`:

- `src/codegen/lir/ir.hpp` still carries
  `LirCallOp::{callee_type_suffix,args_str}` and
  `LirInlineAsmOp::args_str` for legacy-print/render compatibility
- `src/backend/lir_adapter.{hpp,cpp}` remains a transition shim and should be
  retired once backend-ready IR lowering owns production ingestion
- `BackendCallInst::render_callee_type_suffix` still preserves a legacy backend
  render shape and should disappear when emitters stop consuming adapter-owned
  calls
