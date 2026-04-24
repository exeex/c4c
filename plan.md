# LIR HIR-To-LIR Index Tightening Runbook

Status: Active
Source Idea: ideas/open/lir-hir-to-lir-index-tightening.md

## Purpose

Make the `src/codegen/lir` headers act as useful agent indexes after the first
LIR hierarchy cleanup.

Goal: top-level LIR headers read as exported package surfaces, while nested
HIR-to-LIR headers read as private implementation indexes for the semantic area
an agent has entered.

## Core Rule

Use `directory = semantic boundary` and `.hpp = index entry for that boundary`.
Do not introduce a one-header-per-`.cpp` pattern.

## Read First

- `ideas/open/lir-hir-to-lir-index-tightening.md`
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/hir_to_lir.hpp`
- `src/codegen/lir/call_args.hpp`
- `src/codegen/lir/call_args_ops.hpp`
- `src/codegen/lir/operands.hpp`
- `src/codegen/lir/types.hpp`
- `src/codegen/lir/hir_to_lir/lowering.hpp`
- `src/codegen/lir/hir_to_lir/call/call.hpp`
- `src/codegen/lir/hir_to_lir/expr/expr.hpp`

## Current Targets

- `src/codegen/lir/*.hpp` as exported LIR package surfaces or documented model
  subheaders
- `src/codegen/lir/hir_to_lir.hpp` as the public HIR-to-LIR entry header
- `src/codegen/lir/hir_to_lir/lowering.hpp` as the private lowering index
- `src/codegen/lir/hir_to_lir/call/call.hpp` as the private call-lowering index
- `src/codegen/lir/hir_to_lir/expr/expr.hpp` as the private expression-lowering
  index

## Non-Goals

- Do not introduce a traditional separated `include/` tree.
- Do not use headers primarily to enforce C++ interface purity.
- Do not split every `.cpp` into a matching `.hpp`.
- Do not move large implementation families again unless the header-index
  review proves the current semantic directory is wrong.
- Do not change HIR-to-LIR semantics, LIR semantics, printer output, verifier
  behavior, or testcase expectations.
- Do not downgrade tests or claim progress through expectation rewrites.

## Working Model

Top-level headers should be exported surfaces only. Nested headers should be
private implementation indexes. A top-level header that is only an
implementation convenience should move or fold behind the relevant nested
index. A nested header that is only a forwarder should be strengthened or
merged into the nearest useful private index.

## Execution Rules

- Keep each step behavior-preserving unless the source idea explicitly requires
  a structural header move.
- Prefer small compile-proven header slices over broad layout churn.
- After each code-changing structural step, prove `c4c_codegen` builds.
- Run relevant frontend/HIR-to-LIR tests after affected boundaries compile.
- If semantic cleanup is discovered, record it separately instead of absorbing
  it into this structural plan unless it is required to keep the refactor
  compiling.
- Preserve public/private boundary intent in comments or declarations where it
  helps future agents choose the right header.

## Steps

### Step 1: Reclassify Top-Level LIR Headers

Goal: decide and document which top-level LIR headers are exported package
surfaces and which are model subheaders used by the public LIR index.

Primary targets:

- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/call_args.hpp`
- `src/codegen/lir/call_args_ops.hpp`
- `src/codegen/lir/operands.hpp`
- `src/codegen/lir/types.hpp`
- external includes of those headers under `src/`

Actions:

- Inspect include users for each top-level LIR header.
- Keep `ir.hpp` as the public LIR package index for IR, printer, and verifier
  surfaces.
- Confirm whether `call_args.hpp` and `call_args_ops.hpp` are intentionally
  exported for BIR lowering, printer, or other codegen users.
- Decide whether `operands.hpp` and `types.hpp` remain visible top-level model
  headers or should be treated as implementation details of `ir.hpp`.
- Make the smallest structural/header edits needed to make that classification
  obvious to future agents.

Completion check:

- Top-level `src/codegen/lir/*.hpp` files are either clear exported surfaces or
  deliberately documented as model subheaders used by the public LIR index.
- `c4c_codegen` builds.

### Step 2: Tighten The Public HIR-To-LIR Entry Header

Goal: make `src/codegen/lir/hir_to_lir.hpp` sufficient for external HIR-to-LIR
callers without exposing implementation helper internals.

Primary targets:

- `src/codegen/lir/hir_to_lir.hpp`
- external includes of `src/codegen/lir/hir_to_lir/lowering.hpp`
- HIR-to-LIR entry implementation in `src/codegen/lir/hir_to_lir/`

Actions:

- Inspect external callers of HIR-to-LIR lowering.
- Ensure public entry declarations and required public data types are visible
  from `hir_to_lir.hpp`.
- Remove or redirect any external dependency on private lowering headers when
  the dependency is only for entry usage.
- Keep statement-emitter, call-emitter, expression-emitter, and lowering helper
  internals out of the top-level public entry header.

Completion check:

- External HIR-to-LIR users can treat `src/codegen/lir/hir_to_lir.hpp` as the
  only public entry header.
- `c4c_codegen` builds.

### Step 3: Strengthen The Private HIR-To-LIR Lowering Index

Goal: make `src/codegen/lir/hir_to_lir/lowering.hpp` the private implementation
index for shared HIR-to-LIR lowering context and common helper declarations.

Primary target:

- `src/codegen/lir/hir_to_lir/lowering.hpp`

Actions:

- Inspect parent-directory HIR-to-LIR implementation files:
  `hir_to_lir.cpp`, `core.cpp`, `const_init_emitter.cpp`, `lvalue.cpp`,
  `stmt.cpp`, and `types.cpp`.
- Ensure shared lowering context, common helpers, and subdomain entry points are
  discoverable from `lowering.hpp`.
- Keep call-specific and expression-specific details in their own subdomain
  indexes.
- Avoid making `lowering.hpp` a dumping ground for declarations that belong in
  `call/call.hpp` or `expr/expr.hpp`.

Completion check:

- Agents editing core HIR-to-LIR lowering can open `lowering.hpp` to find the
  shared context, common helper declarations, and subdomain entry points.
- `c4c_codegen` builds.

### Step 4: Strengthen The Private Call-Lowering Index

Goal: make `src/codegen/lir/hir_to_lir/call/call.hpp` the single private index
for call-lowering internals.

Primary target:

- `src/codegen/lir/hir_to_lir/call/call.hpp`

Actions:

- Inspect call lowering files: `args.cpp`, `builtin.cpp`, `coordinator.cpp`,
  `target.cpp`, `vaarg.cpp`, `vaarg_amd64.cpp`, and
  `vaarg_amd64_registers.cpp`.
- Ensure helper declarations needed by `call/*.cpp` and the parent lowering
  coordinator are exposed through `call.hpp`.
- Do not create `args.hpp`, `builtin.hpp`, `target.hpp`, or per-ABI headers
  unless a real nested semantic directory is introduced later.

Completion check:

- Call-lowering agents can treat `call/call.hpp` as the single private call
  index.
- `c4c_codegen` builds.

### Step 5: Strengthen The Private Expression-Lowering Index

Goal: make `src/codegen/lir/hir_to_lir/expr/expr.hpp` the single private index
for expression-lowering internals.

Primary target:

- `src/codegen/lir/hir_to_lir/expr/expr.hpp`

Actions:

- Inspect expression lowering files: `coordinator.cpp`, `binary.cpp`, and
  `misc.cpp`.
- Ensure helper declarations needed by `expr/*.cpp` and the parent lowering
  coordinator are exposed through `expr.hpp`.
- Do not create `binary.hpp`, `misc.hpp`, or per-expression headers.

Completion check:

- Expression-lowering agents can treat `expr/expr.hpp` as the single private
  expression index.
- `c4c_codegen` builds.

### Step 6: Run Structural Validation And Close Readiness Review

Goal: prove the header-index tightening preserved behavior and meets the source
idea acceptance criteria.

Actions:

- Confirm no one-header-per-`.cpp` pattern was introduced.
- Confirm top-level and nested header boundaries match the working model.
- Run the supervisor-selected relevant frontend/HIR-to-LIR tests.
- Escalate to broader validation if multiple header boundaries changed or if
  include churn touches shared codegen surfaces.

Completion check:

- `c4c_codegen` builds.
- Relevant frontend/HIR-to-LIR tests pass.
- The acceptance criteria in the source idea are satisfied or remaining work is
  recorded in `todo.md` for the active plan.
