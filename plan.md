# AArch64 Prepared Consumer Wrapper Contraction Runbook

Status: Active
Source Idea: ideas/open/84_aarch64_prepared_consumer_wrapper_contraction.md

## Purpose

Audit and thin the largest AArch64 prepared-authority consumer wrappers after
ideas 69-72 without re-deriving authority or moving target-local emission into
shared code.

## Goal

Reduce redundant wrapper layers in the AArch64 call, memory, and ALU prepared
consumer owners while preserving direct prepared-fact consumption, diagnostics,
and machine records.

## Core Rule

Do not replace deleted wrappers with renamed reconstruction of prepared call,
memory, or scalar authority. Keep ABI details, concrete instruction spelling,
scratch selection, addressing legality, and machine-record construction
target-local.

## Read First

- `ideas/open/84_aarch64_prepared_consumer_wrapper_contraction.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/alu.hpp`

## Current Targets And Scope

- Prepared call-boundary consumers in `calls.cpp` / `calls.hpp`.
- Prepared memory-access consumers in `memory.cpp` / `memory.hpp`.
- Prepared scalar ALU and scalar/control-flow consumers in `alu.cpp` /
  `alu.hpp`.
- Existing AArch64-local helper owners only when they already own the helper
  shape being folded.

## Non-Goals

- Do not create new BIR/prealloc authority unless a concrete missing fact is
  proven and recorded for supervisor review.
- Do not mechanically merge all large owner files.
- Do not work on dispatch relocation, edge-copy contraction, or
  machine-printer contraction.
- Do not weaken test expectations or downgrade supported paths to unsupported.
- Do not move target-local AArch64 emission details into shared prepared
  authority code.

## Working Model

Classify each prepared consumer helper before changing code:

- `target-local emission`: required because it owns AArch64 ABI, addressing,
  instruction spelling, scratch selection, or machine-record construction.
- `necessary prepared adapter`: required because it adapts prepared facts to a
  local record or emission surface without reconstructing authority.
- `redundant wrapper`: only forwards, renames, or repackages already prepared
  facts and can be removed or merged.
- `existing-local-utility fold`: can move into a narrower AArch64-local helper
  owner that already owns the same shape.

## Execution Rules

- Start with an audit table before implementation.
- Keep each implementation step owner-specific unless the same wrapper pattern
  is proven across owners and the supervisor approves a broader packet.
- Remove or merge redundant wrappers only after showing the prepared facts are
  still consumed directly at the replacement site.
- Preserve equivalent diagnostics and machine-record construction when a
  wrapper is contracted.
- For every code-changing step, run build proof plus focused backend tests for
  the affected owner family.
- Use `c4c-regression-guard` before closure if more than one large owner file
  changes.

## Steps

### Step 1: Build The Prepared Consumer Audit

Goal: identify candidate wrappers and classify them before implementation.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`

Actions:

- Inventory prepared call-boundary, prepared memory-access, prepared scalar
  ALU, and prepared scalar/control-flow consumer helpers.
- For each helper, record owner file, prepared fact inputs, output record or
  emission path, classification, and proof needed before contraction.
- Mark wrappers that must remain because they own AArch64 ABI, addressing,
  instruction spelling, scratch selection, diagnostics, or record construction.
- Mark redundant-wrapper and existing-local-utility-fold candidates with the
  smallest plausible implementation owner.

Completion check:

- `todo.md` records the audit table or its file path, the chosen first
  contraction candidate, and the focused proof command for that owner family.

### Step 2: Contract Call-Boundary Wrappers

Goal: remove or merge redundant prepared call-boundary wrappers while keeping
call ABI behavior target-local.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`

Actions:

- Use the Step 1 audit to select call-boundary wrappers classified as
  redundant or foldable into an existing AArch64-local utility.
- Keep ABI register conversion, call sequence construction, diagnostics, and
  machine records in the AArch64 call owner.
- Verify replacement sites consume the same prepared call facts directly.
- Avoid changing memory or ALU owners unless a local call helper signature
  requires a narrow header adjustment.

Completion check:

- Build proof and focused call/backend tests pass.
- `todo.md` names the contracted helpers, the remaining call wrappers that
  intentionally stayed, and the proof command output location.

### Step 3: Contract Memory-Access Wrappers

Goal: remove or merge redundant prepared memory-access wrappers while keeping
addressing legality and load/store emission target-local.

Primary targets:

- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`

Actions:

- Use the Step 1 audit and any Step 2 lessons without importing call-owner
  assumptions.
- Contract only wrappers whose prepared memory facts remain directly consumed
  after the change.
- Keep addressing legality, scratch selection, concrete load/store spelling,
  diagnostics, and machine records in the AArch64 memory owner or an existing
  AArch64-local helper owner.
- Avoid ALU and call-owner edits unless required by a narrow shared local
  helper signature.

Completion check:

- Build proof and focused memory/backend tests pass.
- `todo.md` names the contracted helpers, the remaining memory wrappers that
  intentionally stayed, and the proof command output location.

### Step 4: Contract ALU And Scalar Control Wrappers

Goal: remove or merge redundant prepared scalar ALU and scalar/control-flow
wrappers without weakening scalar authority consumption.

Primary targets:

- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/alu.hpp`

Actions:

- Use the Step 1 audit to select ALU and scalar/control-flow wrappers
  classified as redundant or foldable.
- Preserve concrete instruction selection, flag behavior, scratch choices,
  diagnostics, and machine-record construction in the AArch64 ALU owner or an
  existing AArch64-local helper owner.
- Verify replacement sites consume the same prepared scalar facts directly.
- Avoid broad helper reshapes that blur prepared authority with target-local
  emission.

Completion check:

- Build proof and focused ALU/backend tests pass.
- `todo.md` names the contracted helpers, the remaining ALU wrappers that
  intentionally stayed, and the proof command output location.

### Step 5: Cross-Owner Consistency And Closure Proof

Goal: confirm the contractions are semantic, not just line-count cleanup, and
collect the proof needed for closure.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls.*`
- `src/backend/mir/aarch64/codegen/memory.*`
- `src/backend/mir/aarch64/codegen/alu.*`
- Existing AArch64-local helper owners touched by earlier steps.

Actions:

- Compare the final helper inventory against the Step 1 audit.
- Verify no target-local emission detail leaked into shared prepared authority.
- Verify no prepared authority is reconstructed locally under a renamed helper.
- If more than one large owner changed, run `c4c-regression-guard` with
  matching before/after commands before closure.
- Record any legitimate leftover large wrappers as target-local emission or
  necessary prepared adapters.

Completion check:

- Focused owner-family proof is green for all changed owners.
- Required regression guard proof is green when applicable.
- `todo.md` contains the closure summary, proof commands, and remaining
  intentionally retained wrappers.
