# LIR De-Stringify Legacy-Safe Refactor Runbook

Status: Active
Source Idea: ideas/open/38_lir_de_stringify_legacy_safe_refactor_plan.md
Activated from: ideas/open/35_backend_ready_ir_contract_plan.md

## Purpose

Refactor LIR so opcode, type, and operand semantics are represented as typed
compiler data rather than ad hoc string protocols, while keeping the current
legacy LLVM text path working during migration.

## Goal

Land typed LIR operand/type infrastructure, migrate the highest-friction
string-heavy instructions toward it, and leave both the legacy LLVM printer and
backend consumers on a clear compatibility path.

## Core Rule

Do not add new string-carried opcode/type/operand semantics to LIR while this
runbook is active. Any new LIR surface must prefer typed fields or explicit
compatibility shims scheduled for deletion.

## Read First

- [ideas/open/38_lir_de_stringify_legacy_safe_refactor_plan.md](/workspaces/c4c/ideas/open/38_lir_de_stringify_legacy_safe_refactor_plan.md)
- [src/codegen/lir/ir.hpp](/workspaces/c4c/src/codegen/lir/ir.hpp)
- [src/codegen/lir/lir_printer.cpp](/workspaces/c4c/src/codegen/lir/lir_printer.cpp)
- [src/codegen/lir/hir_to_lir.cpp](/workspaces/c4c/src/codegen/lir/hir_to_lir.cpp)
- [src/codegen/lir/stmt_emitter.cpp](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp)
- [src/backend/lir_adapter.cpp](/workspaces/c4c/src/backend/lir_adapter.cpp)
- [ref/claudes-c-compiler/src/ir/instruction.rs](/workspaces/c4c/ref/claudes-c-compiler/src/ir/instruction.rs)
- [ref/claudes-c-compiler/src/ir/ops.rs](/workspaces/c4c/ref/claudes-c-compiler/src/ir/ops.rs)

## Current Scope

- typed LIR operands, opcodes, predicates, and type references
- migration strategy for string-heavy LIR nodes
- legacy LLVM printer compatibility during transition
- backend and analysis consumer cleanup where they currently decode LIR strings

## Non-Goals

- defining backend-ready IR in this runbook
- implementing mem2reg, phi elimination, or backend lowering here
- target-specific feature expansion in x86 or AArch64 emitters
- broad testsuite convergence work

## Working Model

1. Audit the current string-heavy LIR surfaces and categorize them by semantic
   payload: operand, type, opcode, or textual-only payload.
2. Define typed LIR building blocks that can represent those semantics without
   relying on free-form strings.
3. Introduce verification and compatibility shims so existing construction and
   printing paths keep working while call sites migrate.
4. Migrate the highest-value instruction families and consumer code to typed
   access.
5. Record any remaining compatibility fields that follow-on work must delete.

## Execution Rules

- Keep the legacy LLVM printer path building at every stage.
- Prefer adding typed fields first, then migrating construction sites, then
  deleting string fields last.
- Distinguish genuinely textual payloads, such as inline asm source text, from
  structural compiler semantics that should be typed.
- If a consumer still needs string compatibility, isolate that compatibility in
  one place instead of letting more raw string protocol spread.
- Record every temporary compatibility field or helper in `plan_todo.md`.

## Ordered Steps

### Step 1: Audit string-heavy LIR semantics

Goal: inventory where LIR currently encodes operands, types, opcodes, and other
core semantics as strings.

Primary targets:

- [src/codegen/lir/ir.hpp](/workspaces/c4c/src/codegen/lir/ir.hpp)
- [src/codegen/lir/stmt_emitter.cpp](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp)
- [src/codegen/lir/lir_printer.cpp](/workspaces/c4c/src/codegen/lir/lir_printer.cpp)
- [src/backend/lir_adapter.cpp](/workspaces/c4c/src/backend/lir_adapter.cpp)

Actions:

- list each string-heavy LIR instruction family
- classify each string field as one of:
  typed operand, typed type, enum-like opcode/predicate, or truly textual data
- identify which consumers depend on each family today

Completion check:

- `plan_todo.md` contains a categorized audit of string-heavy LIR surfaces and
  the affected legacy/backend consumers

### Step 2: Define typed LIR primitives

Goal: add the typed building blocks that later instruction migrations will use.

Primary targets:

- [src/codegen/lir/operands.hpp](/workspaces/c4c/src/codegen/lir/operands.hpp)
- [src/codegen/lir/types.hpp](/workspaces/c4c/src/codegen/lir/types.hpp)
- [src/codegen/lir/ir.hpp](/workspaces/c4c/src/codegen/lir/ir.hpp)

Actions:

- define operand/value/global/immediate representations
- define type references or wrappers suitable for both legacy printing and
  backend consumption
- define enum-backed opcodes and predicates for arithmetic/comparison families

Completion check:

- the new primitives compile and can express the currently problematic LIR
  fields without free-form string parsing

### Step 3: Add verification and legacy-safe compatibility shims

Goal: make the migration safe for the existing LLVM printer path.

Primary targets:

- [src/codegen/lir/verify.hpp](/workspaces/c4c/src/codegen/lir/verify.hpp)
- [src/codegen/lir/verify.cpp](/workspaces/c4c/src/codegen/lir/verify.cpp)
- [src/codegen/lir/lir_printer.cpp](/workspaces/c4c/src/codegen/lir/lir_printer.cpp)

Actions:

- add verification for typed operand/type invariants
- add printer-side rendering from typed fields
- keep any transitional string fallbacks explicit and localized

Completion check:

- valid typed LIR can still print to LLVM text and malformed typed LIR is
  rejected early

### Step 4: Migrate high-friction instruction families and consumers

Goal: move the worst string-heavy surfaces first.

Primary targets:

- [src/codegen/lir/stmt_emitter.cpp](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp)
- [src/backend/lir_adapter.cpp](/workspaces/c4c/src/backend/lir_adapter.cpp)
- backend analysis helpers that read LIR instructions

Actions:

- migrate arithmetic/comparison/load/store/call-adjacent instruction families
- replace raw string compares in backend consumers with typed dispatch
- update any touched analysis code to consume typed operands instead of text

Completion check:

- the worst string-protocol consumers no longer branch primarily on raw opcode
  or type strings

### Step 5: Record the handoff to 35/36/37

Goal: leave the backend-ready IR work with a stable, typed LIR base.

Primary targets:

- [ideas/open/35_backend_ready_ir_contract_plan.md](/workspaces/c4c/ideas/open/35_backend_ready_ir_contract_plan.md)
- [ideas/open/36_lir_to_backend_ready_ir_lowering_plan.md](/workspaces/c4c/ideas/open/36_lir_to_backend_ready_ir_lowering_plan.md)
- [ideas/open/37_backend_emitter_backend_ir_migration_plan.md](/workspaces/c4c/ideas/open/37_backend_emitter_backend_ir_migration_plan.md)

Actions:

- record which typed LIR primitives are now stable
- record which compatibility shims remain and who must delete them
- record any remaining string-heavy corners that still block backend IR work

Completion check:

- `plan_todo.md` contains a clean follow-on list for the backend-ready IR plans
- add brief comments only where the new lowering path is not obvious

Completion check:

- promoted family cases emit native x86 assembly and stop failing due to
  fallback

### Step 5: Prove monotonic backend progress

Goal: show that the slice improves the x86 backend surface without regressions.

Primary targets:

- [plan_todo.md](/workspaces/c4c/plan_todo.md)

Actions:

- rerun the targeted tests for the selected family
- rerun `ctest --test-dir build -L x86_backend --output-on-failure`
- if the patch claims broader backend safety, rerun
  `ctest --test-dir build -L backend --output-on-failure`
- record before/after counts, remaining blockers, and the next bounded family in
  `plan_todo.md`

Completion check:

- targeted selected-family tests pass, no previously passing backend-owned cases
  regress, and the recorded `x86_backend` pass count is monotonic
