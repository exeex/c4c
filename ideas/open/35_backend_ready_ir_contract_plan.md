# Backend Ready-IR Contract Plan

## Status

Open as of 2026-03-29.

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/open/38_lir_de_stringify_legacy_safe_refactor_plan.md`

Should precede:

- `ideas/open/36_lir_to_backend_ready_ir_lowering_plan.md`
- `ideas/open/37_backend_emitter_backend_ir_migration_plan.md`
- `ideas/open/34_backend_x86_c_testsuite_backend_convergence_plan.md`

## Goal

Introduce a first-class backend-owned IR contract between frontend LIR and
target emitters so the backend no longer depends on
`src/backend/lir_adapter.cpp` pattern matching as its semantic ingestion layer.

## Why This Slice Exists

The current backend entrypoint still accepts `c4c::codegen::lir::LirModule`
directly, and `src/backend/lir_adapter.cpp` only recognizes a narrow set of
hand-written shapes. That prevents the backend from serving as a stable codegen
consumer for real multi-block, memory-using, external-C inputs.

Before any serious convergence work, the repo needs an explicit
"backend-ready IR" contract that says what the backend may assume about:

- value identity
- basic blocks and terminators
- memory form after LIR normalization
- phi handling policy
- globals / extern declarations / string constants
- call operands and return conventions

This contract work also assumes that LIR stops carrying core opcode/type/operand
semantics primarily through ad hoc string payloads. That prerequisite is owned
by `ideas/open/38_lir_de_stringify_legacy_safe_refactor_plan.md`.

## Parked State

This idea was activated briefly, then parked in favor of the LIR de-stringify
refactor after it became clear that the current `src/codegen/lir/ir.hpp`
surface still leaks too many string-carried semantics into both the legacy LLVM
printer path and backend ingestion. Resume this idea after `38` has established
typed LIR operands/types and a legacy-safe migration path.

## Primary Scope

- `src/backend/backend.hpp`
- `src/backend/backend.cpp`
- `src/backend/lir_adapter.hpp`
- `src/backend/lir_adapter.cpp`
- `src/backend/x86/codegen/emit.hpp`
- `src/backend/aarch64/codegen/emit.hpp`
- new backend IR contract files under `src/backend/`

## Proposed Files

Add:

- `src/backend/ir.hpp`
- `src/backend/ir.cpp`
- `src/backend/ir_validate.hpp`
- `src/backend/ir_validate.cpp`
- `src/backend/ir_printer.hpp`
- `src/backend/ir_printer.cpp`

Keep temporarily but mark as transition-only:

- `src/backend/lir_adapter.hpp`
- `src/backend/lir_adapter.cpp`

## Proposed API Surface

Add a backend-owned IR model in `src/backend/ir.hpp`:

- `struct BackendOperand`
- `struct BackendInst`
- `struct BackendTerminator`
- `struct BackendBlock`
- `struct BackendFunction`
- `struct BackendGlobal`
- `struct BackendExternDecl`
- `struct BackendModule`

Add helper APIs:

- `bool validate_backend_ir(const BackendModule& module, std::string* error);`
- `std::string print_backend_ir(const BackendModule& module);`

Adjust backend dispatch APIs to make the IR boundary explicit:

- keep `std::string emit_module(const BackendModuleInput& input, const BackendOptions& options);`
- add `BackendModule lower_to_backend_ir(const c4c::codegen::lir::LirModule& module);`
- change target emitters to accept `const BackendModule&` instead of
  `const c4c::codegen::lir::LirModule&`

Target emitter signatures expected after this idea chain:

- `std::string c4c::backend::x86::emit_module(const BackendModule& module);`
- `std::string c4c::backend::aarch64::emit_module(const BackendModule& module);`

## Required Contract Decisions

This idea is not complete until the backend IR contract explicitly states:

- whether backend IR is still SSA at block boundaries
- whether `Phi` survives into backend IR or is eliminated before emitter entry
- whether entry `alloca/load/store` survive or are normalized away first
- how globals, pool strings, and extern decls appear in backend IR
- whether calls carry structured callee / args / return info instead of string
  fragments
- which invariants `validate_backend_ir()` enforces

## Reference Files

Use these reference files as the contract model, not as line-for-line rewrite
targets:

- `ref/claudes-c-compiler/src/ir/README.md`
- `ref/claudes-c-compiler/src/ir/module.rs`
- `ref/claudes-c-compiler/src/ir/instruction.rs`
- `ref/claudes-c-compiler/src/backend/README.md`

## Non-Goals

- no attempt to solve mem2reg, phi elimination, or CFG normalization in this
  idea
- no target-specific x86 or AArch64 codegen expansion here
- no testsuite-driven adapter case growth in `src/backend/lir_adapter.cpp`

## Validation

- backend IR types compile without target emitter changes hidden behind casts
  or string shims
- `validate_backend_ir()` can reject malformed IR with targeted diagnostics
- `print_backend_ir()` can dump normalized modules for debugging and tests
- no new backend feature is allowed to depend on extending the legacy minimal
  adapter contract

## Success Condition

This idea is complete when the repo has a stable backend-owned IR data model,
validator, and printer, and later backend work can target that contract instead
of growing `src/backend/lir_adapter.cpp`.
