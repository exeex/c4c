# Value Home and Storage Interpretation in Prealloc

Status: Active
Source Idea: ideas/open/value-home-storage-interpretation-prealloc.md

## Purpose

Move target-independent interpretation of Prepared value homes and storage
encodings out of AArch64 operand helpers and into prealloc-owned APIs.

Goal: expose reusable, target-neutral decoded Prepared storage/home forms while
keeping final target operand construction in each backend.

## Core Rule

Prealloc may decode what a Prepared home or storage record means; targets still
own register-bank spelling, operand forms, immediate legality, addressing
constraints, and instruction selection.

## Read First

- `ideas/open/value-home-storage-interpretation-prealloc.md`
- `src/backend/mir/aarch64/codegen/operands.hpp`
- `src/backend/mir/aarch64/codegen/operands.cpp`
- AArch64 prepared-lowering call sites that currently ask operand helpers to
  decode Prepared storage/home records
- `src/backend/prealloc/storage.hpp`
- `src/backend/prealloc/storage_plans.hpp`
- `src/backend/prealloc/regalloc/value_homes.hpp`
- `src/backend/mir/x86/prepared/prepared.hpp`
- `src/backend/mir/x86/prepared/operands.cpp`

## Current Targets

- AArch64 operand helpers that decode generic Prepared value homes, storage
  plans, assignments, frame slots, immediates, symbols, or labels before
  spelling AArch64 operands.
- New or existing `src/backend/prealloc` helper API surface for target-neutral
  decoded Prepared home/storage decisions.
- x86 prepared operand lowering notes or call-site shape showing that x86 can
  reuse the decoded form without re-decoding raw Prepared records.

## Non-Goals

- Do not move AArch64 operand classes, register names, immediate legality,
  addressing modes, or instruction selection into prealloc.
- Do not normalize semantic BIR facts before prealloc.
- Do not change value-home assignment algorithms.
- Do not weaken unsupported-form tests or expectations.
- Do not add testcase-shaped matching keyed to one storage spelling.

## Working Model

- Prealloc produces or finalizes Prepared storage and value-home contracts.
- Prealloc helpers should return target-neutral decoded forms such as
  register-backed, stack/frame-backed, immediate, symbol, or label-like
  decisions where those forms already exist.
- AArch64 maps decoded forms to AArch64 registers, memory operands, immediates,
  symbols, labels, and diagnostics.
- x86 should be able to consume the same decoded forms while keeping x86
  register classes and operand encoding local.

## Execution Rules

- Keep each step behavior-preserving unless the source idea explicitly calls
  for a boundary change.
- Prefer small helper extraction and adapter updates over broad backend
  rewrites.
- Keep target-specific errors and operand legality in the target layer.
- Treat expectation downgrades, unsupported reclassification, or named-case
  shortcuts as route failure.
- After code-changing steps, run a build or compile proof plus the delegated
  narrow test subset. Escalate to broader AArch64 prepared/backend validation
  before accepting the final boundary move.

## Ordered Steps

### Step 1: Inventory Current AArch64 Decoding

Goal: identify the exact policy-free Prepared home/storage interpretation that
currently lives in AArch64 operand code.

Primary targets:

- `src/backend/mir/aarch64/codegen/operands.hpp`
- `src/backend/mir/aarch64/codegen/operands.cpp`
- Nearby AArch64 prepared-lowering call sites that pass Prepared homes,
  assignments, frame slots, immediates, symbols, or labels through operand
  helpers

Actions:

- Inspect each AArch64 helper that receives Prepared storage/home records.
- Classify each branch as target-neutral interpretation or AArch64-specific
  operand spelling.
- Record in `todo.md` which decoded forms need prealloc ownership and which
  target hooks must remain local.

Completion check:

- `todo.md` names the AArch64 helper boundaries to extract and explicitly
  separates target-neutral interpretation from AArch64-specific spelling.

### Step 2: Add Prealloc Decoded Home/Storage API

Goal: introduce prealloc helper APIs for target-neutral Prepared home/storage
decisions.

Primary targets:

- `src/backend/prealloc`
- Existing Prepared storage, value-home, regalloc, frame, and name helper
  headers/source files as appropriate

Actions:

- Define decoded result types or helper functions in prealloc that express the
  target-neutral forms identified in Step 1.
- Preserve access to source Prepared records needed by target-specific final
  mapping.
- Keep helper names and ownership clear enough that x86 does not need to
  reimplement the same raw Prepared-record decoding.
- Add focused unit or integration coverage if an existing test harness covers
  these helpers directly; otherwise rely on backend behavior proof in later
  steps.

Completion check:

- Prealloc exposes a reusable decoded home/storage API without depending on
  AArch64 operand types or legality rules.
- Build or compile proof covers the new API surface.

### Step 3: Adapt AArch64 To Consume Prealloc Interpretation

Goal: make AArch64 operand helpers consume prealloc decoded forms while keeping
  final AArch64 operand construction local.

Primary targets:

- `src/backend/mir/aarch64/codegen/operands.hpp`
- `src/backend/mir/aarch64/codegen/operands.cpp`
- AArch64 prepared-lowering call sites affected by the helper signatures

Actions:

- Replace target-neutral raw Prepared decoding in AArch64 with calls to the
  prealloc helper API.
- Keep AArch64 register-bank selection, immediate validation, memory operand
  formation, symbol/label spelling, and diagnostics in AArch64 code.
- Avoid changing emitted assembly for currently supported forms.
- Keep unsupported-form behavior at least as strict as before.

Completion check:

- AArch64 behavior is unchanged for supported register, frame/stack,
  immediate, symbol, and label-like forms covered by current lowering.
- The diff shows ownership movement rather than a thin wrapper around old
  AArch64 helper names.
- Fresh build or compile proof passes.

### Step 4: Prove Reuse Path For x86 Prepared Operands

Goal: document or wire the x86 reuse path for the decoded prealloc forms without
expanding the scope into a full x86 lowering rewrite.

Primary targets:

- `src/backend/mir/x86/prepared/prepared.hpp`
- `src/backend/mir/x86/prepared/operands.cpp`
- Nearby x86 prepared fast-path docs or call sites if they already describe
  operand lowering

Actions:

- Identify the x86 prepared operand lowering point that should consume decoded
  prealloc forms.
- Add a concrete note, type use, or narrow adapter showing how x86 can reuse
  the target-neutral decoded form while preserving x86 register classes and
  operand encoding.
- Do not move x86-specific operand spelling into prealloc.

Completion check:

- There is concrete evidence in code or nearby documentation that x86 can reuse
  the prealloc decoded form instead of decoding raw Prepared records again.

### Step 5: Validate Behavior And Anti-Overfit Coverage

Goal: prove the boundary move did not change supported behavior or narrow the
contract.

Actions:

- Run the supervisor-delegated build or compile proof.
- Run narrow backend/prepared tests covering at least:
  - one register-backed value
  - one frame or stack-backed value
  - one symbolic or label-like form if currently supported by lowering
- Compare before/after output or logs where available to show unchanged
  AArch64 output for the covered forms.
- Escalate to broader AArch64 prepared/backend validation if multiple operand
  forms or shared helpers changed.

Completion check:

- `test_after.log` or the delegated proof artifact records passing validation
  for multiple storage/home forms.
- No tests are weakened or reclassified to make the route pass.
- `todo.md` includes the x86 reuse note required by the source idea.

## Final Acceptance

- Prepared home/storage interpretation that is policy-free and
  target-independent lives in prealloc-owned helpers.
- AArch64 still owns final operand construction and legality.
- x86 has a concrete reuse path for decoded forms.
- Proof covers multiple forms and does not rely on a single known testcase.
