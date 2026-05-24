# AArch64 Compatibility Projection Bridge Retirement Runbook

Status: Active
Source Idea: ideas/open/aarch64-codegen-02-compatibility-projection-bridge-retirement.md

## Purpose

Retire or contract one narrow `compatibility_projection.*` bridge path only
after selected target records prove they already own every fact projected by
that path.

## Goal

Move one redundant compatibility bridge responsibility out of the way without
changing AArch64 module lowering behavior, selected target record contracts, or
legacy compatibility test expectations.

## Core Rule

The blocking condition is active: do not remove, contract, or localize any
compatibility projection path until the chosen scope proves selected target
records already carry every needed fact. If the proof fails, leave the bridge
in place and report the exact missing ownership fact.

## Read First

- `ideas/open/aarch64-codegen-02-compatibility-projection-bridge-retirement.md`
- `src/backend/mir/aarch64/codegen/compatibility_projection.hpp`
- `src/backend/mir/aarch64/codegen/compatibility_projection.cpp`
- `src/backend/mir/aarch64/codegen/module_compile.cpp`
- `src/backend/mir/aarch64/module/module.hpp`

## Current Targets

- `src/backend/mir/aarch64/codegen/compatibility_projection.cpp`
- `src/backend/mir/aarch64/codegen/compatibility_projection.hpp`
- `src/backend/mir/aarch64/codegen/module_compile.cpp` only as call-site proof context

## Non-Goals

- Do not redesign module compilation.
- Do not redesign selected target records.
- Do not combine this with contract redesign, phase extraction, broad renaming,
  or generic lowering cleanup.
- Do not move target-specific instruction or register logic into generic layers.
- Do not downgrade expectations, convert tests to unsupported, or weaken test
  contracts.

## Working Model

- Selected target records are the durable owner.
- `compatibility_projection.*` is a bridge from selected target records to
  legacy projection records.
- `module_compile.cpp` currently derives `built_module.functions` from
  `built_module.mir.functions`, then derives `built_module.compatibility` from
  those function records.
- A valid code-changing step must remove, contract, or localize only bridge
  glue that is redundant for the selected narrow scope.

## Execution Rules

- Pick one projected-record family or one narrow bridge entry point.
- Keep each code-changing packet behavior-preserving.
- Prefer semantic ownership proof over file-count reduction.
- Preserve unsupported-node reporting, object record projection, global record
  projection, and module-level behavior unless the selected target records are
  already the source of truth for the chosen scope.
- Treat broad rewrites, helper renames, and expectation-only changes as route
  drift.
- For code-changing steps, prove `build -> focused compatibility tests`; the
  supervisor may require broader regression proof before acceptance.

## Step 1: Ownership Gate

Goal: Select the smallest candidate bridge path and prove whether selected
target records already own its projected facts.

Primary Target:

- `src/backend/mir/aarch64/codegen/compatibility_projection.cpp`

Actions:

- Inspect the two public bridge entry points:
  `derive_compatibility_function_records` and
  `derive_compatibility_projection`.
- Pick one narrow scope, such as function record wrapping or selected-machine
  node filtering.
- Trace every fact projected for that scope back to selected target records and
  `module::MachineModule` / `module::MachineFunction` ownership.
- Check whether any object record, global record, unsupported-node report,
  diagnostic path, or module-level behavior still uses the legacy projection as
  source of truth.
- If any required fact is missing from selected target records, stop without a
  code change and record the missing fact in `todo.md`.

Completion Check:

- A narrow bridge path is named.
- The ownership proof either clears that path for Step 2 or records an explicit
  blocker that leaves the bridge in place.

## Step 2: Contract or Localize the Proven Bridge Path

Goal: Remove, contract, or localize only the bridge glue proven redundant by
Step 1.

Primary Targets:

- `src/backend/mir/aarch64/codegen/compatibility_projection.cpp`
- `src/backend/mir/aarch64/codegen/compatibility_projection.hpp`
- `src/backend/mir/aarch64/codegen/module_compile.cpp` only if the call site
  becomes simpler by using already-owned selected target record behavior

Actions:

- Keep the chosen scope narrow; do not absorb unrelated compatibility
  projection responsibilities.
- Prefer deleting or private-localizing a bridge helper only when all callers
  can use the selected target record owner directly.
- Preserve the observable shape of `module::FunctionRecord` and
  `module::CompatibilityProjection` unless the selected scope proves a field is
  no longer needed.
- Keep `module_compile.cpp` changes limited to the selected bridge path.

Completion Check:

- The selected bridge path is retired, contracted, localized, or proven still
  necessary and left in place.
- No unrelated module compilation or selected target contract behavior changed.

## Step 3: Focused Proof

Goal: Prove the bridge-retirement slice preserves AArch64 compatibility
behavior.

Actions:

- Run the delegated AArch64 backend build proof.
- Run focused module/AArch64 compatibility tests covering selected target
  records, unsupported-node reporting, object record projection, and global
  record projection.
- If the exact focused test names are unclear, first discover the relevant
  tests and record the selected subset in `todo.md`.
- Do not accept expectation rewrites as proof of progress.

Completion Check:

- Build proof is green.
- Focused compatibility proof is green without expectation downgrades.
- Any remaining bridge path is documented as necessary rather than silently
  retained after a failed retirement attempt.
