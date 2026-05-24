# Call Boundary Move Classification in Prealloc

Status: Active
Source Idea: ideas/open/call-boundary-move-classification-prealloc.md

## Purpose

Extract generic Prepared call-boundary move classification from AArch64 call
dispatch into prealloc-owned call and move helper surfaces.

Goal: expose reusable, target-neutral classifications for call inputs, outputs,
move bundles, clobbers, and preservation records while keeping final target ABI
effects and instruction emission in each backend.

## Core Rule

Prealloc may classify Prepared call-boundary facts; targets still own ABI policy,
register spelling, printed call records, and concrete instruction emission.

## Read First

- `ideas/open/call-boundary-move-classification-prealloc.md`
- `src/backend/mir/aarch64/codegen/dispatch_calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/calls_common.cpp`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_preservation.cpp`
- `src/backend/mir/aarch64/codegen/calls_printing.cpp`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/call_plans.hpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/regalloc/call_moves.hpp`
- `src/backend/prealloc/regalloc/call_moves.cpp`
- `src/backend/mir/x86/prepared/prepared.hpp`
- `src/backend/mir/x86/lowering/call.cpp`

## Current Targets

- Generic AArch64 call dispatch decisions that interpret `PreparedCallPlan`,
  `PreparedCallArgumentPlan`, `PreparedCallResultPlan`,
  `PreparedCallPreservedValue`, and `PreparedClobberedRegister`.
- Existing prealloc call-plan and regalloc call-move surfaces that can own
  target-neutral call-boundary classification.
- x86 prepared call-lowering/query surfaces that can consume the shared
  classification without duplicating AArch64 dispatch logic.

## Non-Goals

- Do not rewrite AArch64 ABI policy, AAPCS64 sret behavior, variadic lane
  handling, call-record printing, register spelling, or final call emission.
- Do not move target-specific operand construction or immediate/addressing
  legality into prealloc.
- Do not solve all call lowering gaps in one broad rewrite.
- Do not combine this with entry-formal publication, edge-copy bookkeeping, or
  operand decoding migrations.
- Do not weaken tests, reclassify supported paths as unsupported, or add
  testcase-shaped matching for one call signature.

## Working Model

- Prealloc already owns prepared call plans and regalloc call-move facts.
- A shared helper should classify prepared call-boundary facts into
  target-neutral records that describe argument movement, result publication,
  clobber information, and preservation routes.
- AArch64 should map those classifications to AAPCS64-specific effects and
  concrete machine records.
- x86 should be able to query or consume the same classification while retaining
  x86-specific register classes, ABI details, and operand spelling.

## Execution Rules

- Start with inventory before adding helper APIs.
- Keep each implementation step behavior-preserving unless the source idea
  explicitly requires a boundary change.
- Add small helper records or query functions before adapting target code.
- Keep target-specific details visibly local to AArch64 or x86.
- Treat expectation downgrades, unsupported reclassification, named-case
  shortcuts, or helper-only renames as route failure.
- After code-changing steps, run a fresh build or compile proof plus the
  delegated narrow test subset.
- Before final acceptance, validate several call shapes including ordinary
  arguments/results, preservation or clobber handling, and a case that still
  exercises AArch64-specific ABI policy locally.

## Ordered Steps

### Step 1: Inventory Current Call-Boundary Classification

Goal: identify which AArch64 call helper decisions are target-neutral Prepared
call classifications and which decisions are AArch64 ABI or emission policy.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch_calls.cpp`
- `src/backend/mir/aarch64/codegen/calls_common.cpp`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_preservation.cpp`
- `src/backend/mir/aarch64/codegen/calls_printing.cpp`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/regalloc/call_moves.hpp`

Actions:

- Inspect each AArch64 call helper branch that reads prepared call plans, move
  records, clobbers, or preservation records.
- Classify each branch as target-neutral classification, AArch64 ABI policy, or
  concrete AArch64 emission/printing.
- Identify the smallest first helper surface that can move to prealloc without
  changing emitted AArch64 behavior.
- Record in `todo.md` which helper boundaries to extract and which target hooks
  or local mappings must stay in AArch64.

Completion check:

- `todo.md` names the extraction boundaries and explicitly separates
  target-neutral prepared call classification from AArch64 ABI/emission logic.

### Step 2: Add Prealloc Call-Boundary Classification API

Goal: introduce prealloc-owned records or helper functions for target-neutral
Prepared call-boundary classification.

Primary targets:

- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/call_plans.hpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/regalloc/call_moves.hpp`
- `src/backend/prealloc/regalloc/call_moves.cpp`
- New `src/backend/prealloc` helper files only if existing surfaces become too
  crowded.

Actions:

- Define classification records or query helpers for the target-neutral portion
  identified in Step 1.
- Preserve access to source prepared facts needed by target-specific final
  mapping.
- Keep helper names independent of AArch64 terminology.
- Add focused direct coverage if a backend/prealloc harness can exercise the
  helper without target emission.
- Run a build or compile proof for the new API surface.

Completion check:

- Prealloc exposes reusable call-boundary classification without depending on
  AArch64 operand, register, ABI, or instruction-record types.

### Step 3: Adapt AArch64 To Consume The Shared Classification

Goal: make AArch64 call dispatch consume the prealloc classification while
retaining target ABI effects and concrete emission locally.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch_calls.cpp`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_preservation.cpp`
- `src/backend/mir/aarch64/codegen/calls_common.cpp`
- Existing AArch64 call-boundary tests under `tests/backend`

Actions:

- Replace target-neutral raw prepared-call interpretation in AArch64 with calls
  to the prealloc classification API.
- Keep AAPCS64 sret, variadic lane behavior, target register names, printable
  call records, and instruction emission in AArch64 code.
- Preserve unsupported-form behavior at least as strictly as before.
- Prove unchanged AArch64 behavior for the affected call shapes.

Completion check:

- The diff shows ownership movement rather than a thin wrapper around old
  AArch64 helper names, and AArch64 call tests still pass without weakened
  expectations.

### Step 4: Prove Reuse Path For x86 Prepared Calls

Goal: document or wire a concrete x86 prepared-call consumer for the shared
classification without expanding into a full x86 call-lowering rewrite.

Primary targets:

- `src/backend/mir/x86/prepared/prepared.hpp`
- `src/backend/mir/x86/lowering/call.cpp`
- Existing x86 prepared or handoff tests under `tests/backend/bir`

Actions:

- Identify the x86 prepared-call lowering/query point that should consume the
  shared classification.
- Add a narrow query, type use, adapter, or nearby code note showing how x86
  can consume the prealloc classification.
- Keep x86 ABI policy, register classes, operand spelling, and call emission
  local to x86.
- Add or update focused tests if the reuse path can be proved without a full
  x86 lowering rewrite.

Completion check:

- There is concrete code or test evidence that x86 can reuse the prealloc
  call-boundary classification instead of re-decoding raw Prepared call records.

### Step 5: Validate Behavior And Anti-Overfit Coverage

Goal: prove the boundary move did not change supported behavior or narrow the
call contract.

Actions:

- Run the supervisor-delegated build or compile proof.
- Run narrow backend/prepared tests covering ordinary arguments/results,
  result publication, preservation or clobber handling where currently
  supported, and a call shape that still exercises AArch64-specific ABI policy
  locally.
- Include nearby call shapes rather than only the originally noticed testcase.
- Compare before/after output or logs where available to show unchanged AArch64
  call lowering.
- Escalate to broader backend validation if multiple shared call surfaces or
  target call paths changed.

Completion check:

- `test_after.log` or the delegated proof artifact records passing validation
  over multiple call shapes.
- No tests are weakened or reclassified to make the route pass.
- `todo.md` includes the x86 reuse note required by the source idea.

## Final Acceptance

- Target-neutral Prepared call-boundary classification lives in prealloc-owned
  helper records or query APIs.
- AArch64 still owns AAPCS64 policy, register spelling, call-record printing,
  and final instruction emission.
- x86 has a concrete reuse path for the shared classification.
- Proof covers several call shapes and does not rely on a single failing
  testcase.
