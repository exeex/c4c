# Entry Formal Publication Planning in Prealloc

Status: Active
Source Idea: ideas/open/entry-formal-publication-planning-prealloc.md

## Purpose

Move generic function-entry formal publication planning from AArch64 dispatch
into prealloc value-location and publication planning helpers.

Goal: expose target-neutral formal publication records that describe how
prepared formal homes become machine-value publications, while keeping concrete
ABI source operands and entry-copy instruction emission in each target backend.

## Core Rule

Prealloc may plan target-neutral formal publications from prepared homes and
value-location facts; targets still own ABI register/stack source selection,
entry-copy instruction emission, prologue behavior, and register spelling.

## Read First

- `ideas/open/entry-formal-publication-planning-prealloc.md`
- `src/backend/mir/aarch64/codegen/dispatch_entry_formals.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/calls_common.cpp`
- `src/backend/mir/aarch64/codegen/prologue.cpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/variadic_entry_plans.hpp`
- `src/backend/prealloc/variadic_entry_plans.cpp`
- `src/backend/mir/x86/prepared/prepared.hpp`
- `src/backend/mir/x86/prologue.cpp.md`

## Current Targets

- AArch64 entry-formal dispatch that turns prepared formal homes into machine
  value publications before target instruction emission.
- Existing prealloc value-location, lookup, and entry-plan surfaces that can own
  target-neutral formal-publication records.
- x86 prepared entry-lowering/query surfaces that can consume a shared formal
  publication plan without copying AArch64 entry dispatch.

## Non-Goals

- Do not change the calling convention, parameter store strategy, prologue
  shape, or concrete ABI source operands.
- Do not move ABI register naming, stack argument addressing, or entry-copy
  instruction emission into prealloc.
- Do not combine this with general call-boundary classification, edge-copy
  bookkeeping, or operand decoding migrations.
- Do not downgrade tests around formal publication failures.
- Do not add testcase-shaped handling for one named formal pattern.

## Working Model

- Prealloc owns prepared value-location facts and can derive target-neutral
  publication intent for formal values.
- A formal publication plan should say which formal value is published, which
  prepared home or location authority backs it, and which target-local emission
  step must materialize the copy.
- AArch64 should consume the plan and retain AAPCS64 register/stack source
  details, concrete copies, diagnostics, and prologue interactions locally.
- x86 should be able to query or consume the same plan while preserving x86 ABI
  source operands and operand encoding locally.

## Execution Rules

- Start with inventory before adding helper APIs.
- Keep each implementation step behavior-preserving unless the source idea
  explicitly requires a boundary change.
- Prefer a narrow record/helper extraction over a broad entry-lowering rewrite.
- Keep target-specific errors and copy legality in the target layer.
- Treat expectation downgrades, unsupported reclassification, named-pattern
  shortcuts, or unused helper-only changes as route failure.
- After code-changing steps, run a fresh build or compile proof plus the
  delegated narrow test subset.
- Before final acceptance, validate more than one function-entry shape,
  including different formal locations where currently supported, and preserve
  or improve missing/malformed Prepared formal diagnostics.

## Ordered Steps

### Step 1: Inventory Current Entry-Formal Publication Planning

Goal: identify which AArch64 entry-formal decisions are target-neutral
publication planning and which decisions are AArch64 ABI or concrete emission.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch_entry_formals.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/calls_common.cpp`
- `src/backend/mir/aarch64/codegen/prologue.cpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/variadic_entry_plans.hpp`

Actions:

- Inspect each AArch64 entry-formal path that reads prepared formal homes,
  value locations, byval or variadic entry facts, and publication records.
- Classify each branch as target-neutral formal publication planning,
  AArch64 ABI source selection, concrete AArch64 copy emission, or diagnostics.
- Identify the smallest first helper surface that can move to prealloc without
  changing emitted AArch64 behavior.
- Record in `todo.md` which helper boundaries to extract and which target hooks
  or local mappings must stay in AArch64.

Completion check:

- `todo.md` names the extraction boundaries and explicitly separates
  target-neutral formal publication planning from AArch64 ABI/emission logic.

### Step 2: Add Prealloc Formal Publication Plan API

Goal: introduce prealloc-owned records or helper functions for target-neutral
function-entry formal publication planning.

Primary targets:

- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- New `src/backend/prealloc` helper files only if existing surfaces become too
  crowded.
- Focused backend/prealloc tests under `tests/backend/bir` if a direct harness
  can exercise the helper.

Actions:

- Define formal publication records or query helpers for the target-neutral
  portion identified in Step 1.
- Preserve access to source prepared facts needed by target-specific final
  source selection and copy emission.
- Keep helper names independent of AArch64 terminology.
- Add focused coverage if the helper can be tested without target emission.
- Run a build or compile proof for the new API surface.

Completion check:

- Prealloc exposes reusable formal publication planning without depending on
  AArch64 ABI, register, stack-addressing, or instruction-record types.

### Step 3: Adapt AArch64 To Consume Formal Publication Plans

Goal: make AArch64 entry-formal dispatch consume the prealloc plan while
retaining target ABI source selection and concrete copy emission locally.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch_entry_formals.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/calls_common.cpp`
- Existing AArch64 entry-formal or call-boundary tests under `tests/backend`

Actions:

- Replace target-neutral raw prepared-home publication planning in AArch64 with
  calls to the prealloc formal publication API.
- Keep ABI register/stack source selection, target register names, memory
  operand construction, inline-asm or machine-record emission, and prologue
  behavior in AArch64 code.
- Preserve unsupported-form behavior at least as strictly as before.
- Prove unchanged AArch64 behavior for the affected formal-entry shapes.

Completion check:

- The diff shows ownership movement rather than a thin wrapper around old
  AArch64 helper names, and AArch64 entry-formal tests still pass without
  weakened expectations.

### Step 4: Prove Reuse Path For x86 Entry Lowering

Goal: document or wire a concrete x86 prepared entry-lowering consumer for the
shared formal publication plan without expanding into a full x86 prologue
rewrite.

Primary targets:

- `src/backend/mir/x86/prepared/prepared.hpp`
- `src/backend/mir/x86/prologue.cpp.md`
- `src/backend/mir/x86/lowering/frame.cpp`
- Existing x86 prepared or handoff tests under `tests/backend/bir`

Actions:

- Identify the x86 entry-lowering or prepared-query point that should consume
  the shared formal publication plan.
- Add a narrow query, type use, adapter, or nearby code note showing how x86
  can consume the prealloc plan.
- Keep x86 ABI policy, source operands, register classes, operand spelling, and
  entry-copy emission local to x86.
- Add or update focused tests if the reuse path can be proved without a full
  x86 prologue rewrite.

Completion check:

- There is concrete code or test evidence that x86 can reuse the prealloc
  formal publication plan instead of copying AArch64 entry dispatch.

### Step 5: Validate Behavior And Anti-Overfit Coverage

Goal: prove the boundary move did not change supported behavior or narrow the
entry-formal contract.

Actions:

- Run the supervisor-delegated build or compile proof.
- Run narrow backend/prepared tests covering more than one function-entry
  shape, including different formal locations where currently supported.
- Include missing or malformed Prepared formal fact coverage where current
  tests or direct probes can exercise it.
- Compare before/after output or logs where available to show unchanged AArch64
  entry lowering.
- Escalate to broader backend validation if multiple shared entry/prologue
  surfaces or target entry paths changed.

Completion check:

- `test_after.log` or the delegated proof artifact records passing validation
  over multiple formal-entry shapes.
- No tests are weakened or reclassified to make the route pass.
- `todo.md` includes the x86 reuse note required by the source idea.

## Final Acceptance

- Target-neutral function-entry formal publication planning lives in
  prealloc-owned helper records or query APIs.
- AArch64 still owns ABI source selection, concrete entry-copy emission,
  register spelling, diagnostics, and prologue behavior.
- x86 has a concrete reuse path for the shared formal publication plan.
- Proof covers more than one function-entry shape and does not rely on a single
  named formal pattern.
