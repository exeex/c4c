# BIR Layout Dual Path Coverage And Dump Guard Runbook

Status: Active
Source Idea: ideas/open/116_bir_layout_dual_path_coverage_and_dump_guard.md

## Purpose

Strengthen BIR aggregate layout coverage before any legacy type-text removal.
Keep legacy `type_decls` available as fallback and parity evidence while
closing structured-path holes and adding focused `--dump-bir` guards.

## Goal

Make selected BIR lowering paths use structured aggregate layout authority
without depending on legacy body parsing as the only successful route.

## Core Rule

Preserve backend behavior and dump text unless a step explicitly documents and
tests an intentional diagnostic or dump-contract change.

## Read First

- ideas/open/116_bir_layout_dual_path_coverage_and_dump_guard.md
- src/backend/lir_to_bir.cpp
- src/backend/layout.cpp
- src/backend/bir/
- tests that exercise aggregate lowering and `--dump-bir`

## Current Targets

- `BackendStructuredLayoutTable` lookup behavior.
- Aggregate size, alignment, and field offset derivation.
- Nested, packed, empty, array, and pointer-containing aggregate layouts.
- Globals, aggregate initializers, local slots, GEP/addressing, loads, stores,
  `memcpy`, and `memset`.
- Aggregate call ABI cases including sret, byval/byref, HFA-like
  classification, variadic aggregate arguments, and `va_arg`.
- Focused `--dump-bir` tests that observe lowered BIR facts.

## Non-Goals

- Do not remove `LirModule::type_decls`.
- Do not remove legacy parsing helpers.
- Do not rework `src/backend/bir/bir_printer.cpp` to use a render context.
- Do not migrate `src/backend/mir/`.
- Do not downgrade test expectations to claim backend progress.

## Working Model

- Structured layout should become usable for covered BIR paths even when
  legacy parity is not the only authority.
- Legacy layout data remains available as fallback and parity evidence during
  this runbook.
- `--dump-bir` tests guard semantic BIR output produced by lowering, not a
  printer-authority migration.
- Any path that still needs legacy body parsing must be recorded in `todo.md`
  as fallback-only evidence for later ideas.

## Execution Rules

- Prefer semantic lowering or layout lookup improvements over testcase-shaped
  matching.
- Keep changes in small, buildable steps with focused proof after each code
  change.
- Add structured-first behavior only where the fallback behavior is understood
  and behavior-preserving.
- Inventory remaining fallback-only legacy layout users instead of treating
  them as solved.
- Use `test_after.log` for executor proof unless the supervisor delegates a
  different artifact.

## Steps

### Step 1: Inventory BIR Aggregate Layout Authority

Goal: Identify BIR lowering paths that still require legacy layout authority
or parity-gated structured lookup.

Primary target: `src/backend/lir_to_bir.cpp` and backend layout helpers.

Actions:
- Inspect structured and legacy aggregate layout lookup entry points.
- List paths for globals, locals, GEP/addressing, memory ops, calls, variadic
  arguments, and `va_arg`.
- Mark which paths are structured-ready, parity-gated, or fallback-only.
- Record fallback-only findings in `todo.md` for later executor packets.

Completion check:
- `todo.md` names the first implementation target and captures the remaining
  fallback-only surfaces discovered during the inventory.

### Step 2: Make Safe Structured Layout Lookups First-Class

Goal: Let selected BIR lowering paths use structured layout facts without
requiring legacy body parsing as the only successful route.

Primary target: `BackendStructuredLayoutTable` consumers in BIR lowering.

Actions:
- Route covered aggregate size, alignment, and field offset queries through
  structured layout when the structured facts are complete.
- Preserve legacy fallback and parity evidence.
- Keep unsupported or ambiguous paths explicit rather than silently changing
  behavior.
- Add focused tests for nested, packed, empty, array, and pointer-containing
  aggregates as each family is covered.

Completion check:
- Covered layout families pass focused tests through structured authority with
  legacy fallback still present.

### Step 3: Cover Aggregate Storage And Initializer Paths

Goal: Prove structured layout facts survive global and local aggregate
lowering.

Primary target: BIR lowering for globals, aggregate initializers, local slots,
GEP/addressing, loads, stores, `memcpy`, and `memset`.

Actions:
- Add or update tests for globals and aggregate initializers.
- Add or update tests for local aggregate slots and address computation.
- Verify BIR facts for offsets, sizes, and memory operation widths.
- Keep emitted backend behavior stable.

Completion check:
- Focused tests cover global and local-memory aggregate paths without relying
  on legacy-only layout parsing as the sole successful route.

### Step 4: Cover Aggregate Call ABI And Variadic Paths

Goal: Prove aggregate layout facts are stable across call classification and
variadic lowering.

Primary target: BIR lowering for sret, byval/byref, HFA-like aggregate
classification, variadic aggregate arguments, and `va_arg`.

Actions:
- Add focused tests for aggregate call ABI families.
- Add focused tests for variadic aggregate arguments and `va_arg`.
- Preserve existing ABI and output behavior.
- Record any call-family fallback that remains blocked by legacy-only data.

Completion check:
- Aggregate call and variadic tests pass with clear notes for any remaining
  fallback-only surfaces.

### Step 5: Add BIR Dump Guards For Layout-Sensitive Paths

Goal: Guard semantic `--dump-bir` output for layout-sensitive aggregate
lowering before printer-authority work begins.

Primary target: CLI or backend tests that exercise `--dump-bir`.

Actions:
- Add at least one layout-sensitive aggregate dump fixture.
- Cover lowered facts such as aggregate offsets, sizes, memory operations, or
  call ABI facts where the current printer exposes them.
- Preserve existing dump text unless the changed output is intentional and
  tested.
- Do not make `bir_printer.cpp` parse legacy type declarations.

Completion check:
- Focused `--dump-bir` tests prove semantic BIR dump output remains stable for
  the covered layout-sensitive path.

### Step 6: Final Inventory And Regression Check

Goal: Leave a stable proof base for the later BIR printer and legacy-removal
ideas.

Primary target: `todo.md`, focused aggregate tests, and broader backend test
coverage selected by the supervisor.

Actions:
- Summarize remaining fallback-only legacy layout users in `todo.md`.
- Verify legacy `type_decls` remains available as fallback and parity evidence.
- Run the delegated narrow proof, then the supervisor-selected broader
  validation if this runbook has changed multiple aggregate families.

Completion check:
- All acceptance criteria from the source idea are satisfied or explicitly
  blocked with fallback-only inventory ready for a follow-up lifecycle decision.
