# AArch64 ALU Legacy Semantic Lowering Follow-Up Runbook

Status: Active
Source Idea: ideas/open/255_aarch64_alu_legacy_semantic_lowering_followup.md

## Purpose

Decide which legacy-only ALU lowering notes from the deleted `alu.md` shard are
still semantically valid for the current structured AArch64 MIR pipeline, then
implement only those routes through typed records, allocation facts, and
ALU-owned helpers.

Goal: convert valid legacy ALU semantic guidance into real structured lowering
behavior without reviving text-emitter conventions or weakening existing ALU
support.

Core Rule: this is semantic ALU lowering work, not another markdown shard
redistribution and not an expectation-rewrite route.

## Read First

- `ideas/open/255_aarch64_alu_legacy_semantic_lowering_followup.md`
- `src/backend/mir/aarch64/codegen/alu.hpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- existing focused AArch64 ALU, scalar cast, instruction dispatch, and printer
  tests

## Current Targets

- Audit old ALU semantic routes against current MIR records and prepared
  allocation facts.
- Classify each route as accepted, obsolete, or separate-idea material before
  implementation.
- Implement accepted routes through ALU-owned lowering, record construction,
  and printer support where needed.
- Prove each accepted route with targeted backend coverage before broadening.

## Non-Goals

- Do not reopen the completed ALU markdown redistribution route.
- Do not recreate the old text-emitter accumulator convention as the semantic
  carrier.
- Do not redistribute other AArch64 markdown shards.
- Do not treat commented legacy mirror behavior as accepted current compiler
  behavior without current-pipeline design and proof.
- Do not weaken tests, mark supported ALU paths unsupported, or rewrite
  expectations to claim progress.
- Do not broaden into unrelated AArch64 cleanup.

## Working Model

- Structured AArch64 behavior must flow through typed MIR records, prepared
  value homes, allocation facts, and ALU-owned helpers.
- Unsigned power-of-two division and remainder reductions are independent from
  signed division and remainder behavior; signed routes require separate design
  and proof.
- Signed 32-bit extension and unsigned zero-extension must be explicit
  post-operation behavior when introduced.
- Scratch handling must respect current allocation facts and right-side
  destination-register conflicts.
- i128 copy behavior must preserve high-half semantics and not collapse into a
  narrow named-case shortcut.

## Execution Rules

- Start with an audit/classification packet before implementation.
- Keep accepted route decisions in `todo.md` until they need a durable plan
  rewrite or separate source idea.
- Implement one semantic family at a time and prove it with a focused backend
  subset chosen by the supervisor.
- Stop and request plan review if a route needs signed division semantics,
  new shared instruction-core abstractions, or cross-family backend changes.
- Treat testcase-shaped shortcuts, final assembly string matching, and
  expectation downgrades as route drift.

## Steps

### Step 1: Audit Legacy ALU Routes Against Current Pipeline

Goal: classify the legacy ALU semantic routes before changing code.

Primary targets:
- `ideas/open/255_aarch64_alu_legacy_semantic_lowering_followup.md`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/alu.hpp`
- focused AArch64 ALU and scalar backend tests

Actions:
- Inventory the named legacy routes: unary integer helpers, CLZ, CTZ,
  byte-swap, popcount, unsigned power-of-two division/remainder reductions,
  accumulator fallback division/remainder/variable-shift behavior,
  register-direct scratch handling, 32-bit extension behavior, and i128 copy.
- Map each route to current MIR records, allocation facts, and existing
  lowering/printer surfaces.
- Classify each route as accepted for this plan, obsolete for the current
  pipeline, or requiring a separate open idea.
- Record the classification and first implementation packet recommendation in
  `todo.md`.

Completion check:
- `todo.md` contains the route classification, explicit first implementation
  packet, and focused proof recommendation.
- No implementation files have changed unless the supervisor separately
  delegates code work.

### Step 2: Implement Accepted Unary And Bit-Operation Routes

Goal: add structured support for accepted unary integer and bit-operation
routes.

Primary targets:
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/alu.hpp`
- existing ALU record/printer tests, with new focused tests only when needed

Actions:
- Implement accepted unary integer helpers, CLZ, CTZ, byte-swap, and popcount
  through ALU-owned structured records and lowering helpers.
- Preserve existing supported ALU paths and diagnostics.
- Keep printer spelling as a consequence of structured records, not direct
  final-text matching.

Completion check:
- Focused backend proof covers each implemented route, including relevant width
  behavior and unsupported-path diagnostics.
- No tests or expectations are weakened.

### Step 3: Implement Accepted Unsigned Power-Of-Two Reductions

Goal: restore only valid unsigned division/remainder power-of-two reductions.

Primary targets:
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/alu.hpp`
- focused ALU lowering and printer tests

Actions:
- Implement unsigned division-by-power-of-two as shift lowering where current
  MIR facts prove the route.
- Implement unsigned remainder-by-power-of-two as mask lowering where current
  MIR facts prove the route.
- Keep signed division and signed remainder out of this step unless a separate
  signed design has been reviewed and approved.

Completion check:
- Focused tests prove unsigned reduction behavior and reject unsupported signed
  equivalence.
- Existing division/remainder behavior is not downgraded.

### Step 4: Implement Accepted Scratch, Fallback, And Extension Routes

Goal: handle accepted register-conflict, fallback, and width-extension
semantics through allocation-aware ALU lowering.

Primary targets:
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/alu.hpp`
- focused dispatch, ALU, and printer tests

Actions:
- Implement accepted register-direct scratch handling for right-side
  destination-register conflicts.
- Implement accepted accumulator fallback division/remainder/variable-shift
  behavior only when it can be represented with current structured facts.
- Model signed 32-bit extension and unsigned zero-extension as explicit
  post-operation behavior for routes that require it.

Completion check:
- Focused tests prove scratch, fallback, and extension behavior for accepted
  routes.
- The implementation does not revive text-emitter accumulator authority.

### Step 5: Implement Accepted i128 Copy Behavior

Goal: preserve valid i128 copy semantics through structured ALU/MIR behavior.

Primary targets:
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/alu.hpp`
- i128-focused backend tests

Actions:
- Audit current i128 record and lowering surfaces for low-half/high-half copy
  representation.
- Implement accepted i128 copy behavior through typed records and allocation
  facts.
- Preserve high-half behavior explicitly.

Completion check:
- Focused tests prove i128 copy behavior, including high-half preservation.
- No narrow testcase-shaped shortcut is introduced.

### Step 6: Acceptance Review And Broader Proof

Goal: decide whether the source idea is complete and prove accepted semantic
routes did not regress existing ALU behavior.

Primary targets:
- `todo.md`
- focused backend ALU, dispatch, printer, and scalar tests
- broader backend or full ctest proof if blast radius requires it

Actions:
- Confirm every in-scope legacy route is classified and every accepted route
  is implemented or explicitly split into a separate idea.
- Run the supervisor-selected focused proof for implemented routes.
- Escalate to backend or full validation when shared instruction, dispatch,
  printer, or allocation behavior changed broadly.
- Send the active plan back for lifecycle review when the source idea itself
  is satisfied or when the remaining scope requires a split.

Completion check:
- `todo.md` records final classification, proof results, and lifecycle
  recommendation.
- Existing supported ALU behavior remains supported.
- No expectation downgrade, final-text shortcut, or unrelated backend cleanup
  is present.
