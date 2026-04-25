# HIR To LIR Layout LegacyDecl Demotion Runbook

Status: Active
Source Idea: ideas/open/115_hir_to_lir_layout_legacy_decl_demotion.md
Activated: 2026-04-25

## Purpose

Demote selected HIR-to-LIR layout consumers from
`StructuredLayoutLookup::legacy_decl` authority to structured
`StructNameId` / `LirStructDecl` authority while retaining legacy fallback for
incomplete coverage.

## Goal

Make selected aggregate layout paths prefer structured layout facts and prove
that emitted LLVM and backend behavior do not drift.

## Core Rule

Prefer structured layout only where `StructNameId` and `LirStructDecl` coverage
is present and parity has been observed; otherwise keep the existing
`legacy_decl` fallback.

## Read First

- [ideas/open/115_hir_to_lir_layout_legacy_decl_demotion.md](/workspaces/c4c/ideas/open/115_hir_to_lir_layout_legacy_decl_demotion.md)
- [src/codegen/lir/hir_to_lir/lowering.hpp](/workspaces/c4c/src/codegen/lir/hir_to_lir/lowering.hpp)
- [src/codegen/lir/hir_to_lir/core.cpp](/workspaces/c4c/src/codegen/lir/hir_to_lir/core.cpp)
- [src/codegen/lir/hir_to_lir/hir_to_lir.cpp](/workspaces/c4c/src/codegen/lir/hir_to_lir/hir_to_lir.cpp)
- [src/codegen/lir/hir_to_lir/const_init_emitter.cpp](/workspaces/c4c/src/codegen/lir/hir_to_lir/const_init_emitter.cpp)
- [src/codegen/lir/hir_to_lir/lvalue.cpp](/workspaces/c4c/src/codegen/lir/hir_to_lir/lvalue.cpp)
- [src/codegen/lir/hir_to_lir/types.cpp](/workspaces/c4c/src/codegen/lir/hir_to_lir/types.cpp)
- [src/codegen/lir/hir_to_lir/call/args.cpp](/workspaces/c4c/src/codegen/lir/hir_to_lir/call/args.cpp)
- [src/codegen/lir/hir_to_lir/call/vaarg.cpp](/workspaces/c4c/src/codegen/lir/hir_to_lir/call/vaarg.cpp)

## Current Targets

- `StructuredLayoutLookup` helpers and observation reporting.
- Const initializer aggregate and field lookup.
- Aggregate GEP and lvalue field-chain resolution.
- Variadic aggregate argument handling.
- `va_arg` aggregate handling.
- Byval/byref aggregate call handling.
- Nested struct field lookup.
- Size and alignment helpers used by HIR-to-LIR lowering.

## Non-Goals

- Do not remove the `TypeSpec::tag` bridge until all consumers no longer need
  it.
- Do not remove legacy fallback for paths without stable structured coverage.
- Do not rewrite MIR layout code under `src/backend/mir/`.
- Do not weaken tests, mark supported paths unsupported, or prove progress by
  expectation-only changes.
- Do not add testcase-shaped shortcuts for named examples.

## Working Model

- `lookup_structured_layout` already reports both legacy and structured facts.
- Existing consumers still read `layout.legacy_decl` for semantic size,
  alignment, or field authority after parity observation.
- This route should add small structured-first accessors or local conversions
  that make call sites choose `structured_decl` when parity permits, then fall
  back to `legacy_decl`.
- Mismatches between structured and legacy layout should be recorded or surfaced
  through existing observation mechanisms; they should not be hidden by
  silently choosing one side.

## Execution Rules

- Keep each code-changing step narrow enough for build proof plus a targeted
  frontend/backend subset.
- Preserve emitted LLVM text and backend output for covered aggregate paths.
- If `src/backend/mir/` compile failures are the only blocker, ask the
  supervisor to exclude the relevant MIR compile target instead of changing MIR
  semantics.
- Prefer reusable helpers when two or more consumers need the same
  structured-first size, alignment, or field lookup rule.
- Update `todo.md` with the current packet, proof command, and result after
  each executor packet.
- Escalate to reviewer scrutiny if a change mainly makes one known case pass
  while nearby aggregate-layout cases remain unexamined.

## Ordered Steps

### Step 1: Inventory HIR-to-LIR Legacy Layout Authority

Goal: identify the exact `legacy_decl` reads that still act as semantic layout
authority and select the first safe demotion packet.

Primary Target:
- `src/codegen/lir/hir_to_lir/`

Actions:
- Inspect all `StructuredLayoutLookup` consumers in HIR-to-LIR.
- Classify each `layout.legacy_decl` read as observation-only, fallback-only,
  or active semantic authority.
- Confirm which candidates already have `structured_decl` and
  `structured_name_id` available at the call site.
- Pick the smallest first demotion target that can be proven without output
  drift, preferably a size/alignment helper or const-initializer lookup path.
- Record any structured-vs-legacy mismatch or missing structured coverage in
  `todo.md`, not in the source idea.

Completion Check:
- `todo.md` names the selected first code-changing target, relevant files,
  proposed proof command, and any blockers.

### Step 2: Add Structured-First Layout Access Helpers

Goal: make structured-first size, alignment, and field access reusable without
removing legacy fallback.

Primary Target:
- `src/codegen/lir/hir_to_lir/lowering.hpp`
- `src/codegen/lir/hir_to_lir/core.cpp`

Actions:
- Introduce the smallest helper surface needed by Step 1's chosen target.
- Prefer `structured_decl` when present and parity-safe.
- Preserve legacy fallback when structured coverage is absent.
- Keep mismatch observation visible through the existing structured layout
  observation path.
- Avoid broad rewrites of `StructuredLayoutLookup` unless multiple consumers
  need the same API.

Completion Check:
- The chosen helper compiles.
- Existing structured layout observation behavior remains intact.
- A narrow test or compile proof covers both structured-present and fallback
  behavior when practical.

### Step 3: Demote Const Initializer And Field Lookup Paths

Goal: make aggregate initializer and nested field lookup prefer structured
layout facts where safe.

Primary Target:
- `src/codegen/lir/hir_to_lir/const_init_emitter.cpp`
- `src/codegen/lir/hir_to_lir/types.cpp`
- `src/codegen/lir/hir_to_lir/lvalue.cpp`

Actions:
- Replace active `legacy_decl` authority in const aggregate lookup with the
  structured-first helper from Step 2.
- Apply the same rule to nested struct field lookup and aggregate GEP/lvalue
  field-chain resolution.
- Keep legacy fallback for unconverted or incomplete structured cases.
- Add or adjust focused tests only when they assert stable semantic behavior,
  not rewritten expectations.

Completion Check:
- Aggregate initializer, nested field lookup, and GEP proof cases pass without
  emitted LLVM drift.
- Fallback paths remain covered or explicitly noted in `todo.md`.

### Step 4: Demote Aggregate Call And Variadic Paths

Goal: make aggregate call lowering, variadic aggregate arguments, and `va_arg`
aggregate handling use structured layout first.

Primary Target:
- `src/codegen/lir/hir_to_lir/call/args.cpp`
- `src/codegen/lir/hir_to_lir/call/vaarg.cpp`
- related HIR-to-LIR call helpers

Actions:
- Convert aggregate payload size and alignment decisions to structured-first
  authority where the lookup carries a valid structured declaration.
- Preserve byval/byref behavior and ABI-facing output.
- Keep legacy fallback for aggregate call paths without stable structured
  coverage.
- Prove fixed aggregate calls, variadic aggregate arguments, and `va_arg`
  aggregate cases together when the blast radius spans shared ABI helpers.

Completion Check:
- Aggregate call, variadic, `va_arg`, and byval/byref proof cases pass without
  output drift.
- No MIR layout cleanup is included in the slice.

### Step 5: Consolidate Remaining HIR-to-LIR Size And Alignment Consumers

Goal: remove remaining selected `legacy_decl` semantic authority in HIR-to-LIR
size/alignment helpers while keeping fallback.

Primary Target:
- `src/codegen/lir/hir_to_lir/core.cpp`
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`
- any remaining HIR-to-LIR helper identified in Step 1

Actions:
- Convert remaining selected size and alignment reads to structured-first
  helpers.
- Deduplicate helper logic introduced by earlier steps only where it reduces
  real call-site duplication.
- Re-scan HIR-to-LIR for `layout.legacy_decl` reads and classify any remaining
  uses as fallback-only, observation-only, or intentionally deferred.

Completion Check:
- Selected HIR-to-LIR consumers no longer treat `legacy_decl` as semantic
  authority.
- Remaining `legacy_decl` uses are documented in `todo.md` as fallback,
  observation, or deferred coverage.

### Step 6: Acceptance Validation And Closure Review

Goal: prove the demotion route preserved behavior and decide whether the source
idea is complete.

Primary Target:
- frontend aggregate lowering tests
- backend aggregate call and variadic proof subsets
- full or broader repo-native validation chosen by the supervisor

Actions:
- Run the supervisor-selected broader validation after the final code slice.
- Compare generated LLVM/backend outputs for representative aggregate
  initializer, GEP, variadic, `va_arg`, and byval/byref cases.
- Confirm no tests were weakened to claim progress.
- Ask plan owner to close only if the source idea acceptance criteria are
  satisfied, not merely because this runbook is exhausted.

Completion Check:
- Fresh proof is recorded in `todo.md`.
- The supervisor has enough evidence to call plan owner for close, deactivate,
  or replan.
