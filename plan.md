# Prepared I128 Runtime Helper Authority Runbook

Status: Active
Source Idea: ideas/open/248_prepared_i128_runtime_helper_authority.md
Activated from: ideas/open/248_prepared_i128_runtime_helper_authority.md
Supersedes active runbook: ideas/open/236_aarch64_i128_pair_lowering.md is parked on a prepared-helper-authority blocker.

## Purpose

Provide prepared/shared runtime-helper authority for i128 operations that need
helper calls before AArch64 i128 pair lowering consumes helper boundaries.

## Goal

Map helper-required i128 source operations to structured helper kind, callee,
lane, memory-return, clobber, resource, ABI, and register-bank facts without
moving helper selection or marshaling policy into AArch64 target lowering.

## Core Rule

This runbook prepares helper authority only. It must not implement AArch64
selected helper-call nodes, printer output, local i128 allocation, fixed
register marshaling, or scalar-i64 substitutes.

## Read First

- `ideas/open/248_prepared_i128_runtime_helper_authority.md`
- `ideas/open/236_aarch64_i128_pair_lowering.md`
- prepared call-boundary, value-home, ABI, regalloc, and clobber/resource
  facts for retained calls and wide integer values
- BIR operation forms for i128 div/rem and float/i128 conversions
- focused backend tests under `tests/backend/mir/`

## Current Targets

- Source i128 operation identity for helper-required operations.
- Helper kind and callee symbol authority.
- Low/high argument lane bindings.
- Low/high result lane bindings or memory-return ownership.
- Helper-specific clobber and resource policy.
- ABI and register-bank transition facts needed by later selected records.
- Fail-closed diagnostics for incomplete helper authority.

## Non-Goals

- Do not add selected AArch64 helper-call machine nodes or terminal printer
  output.
- Do not synthesize helper calls in AArch64 dispatch from `BinaryInst` or
  `CastInst` opcodes.
- Do not create a local i128 allocator, fixed `x0`/`x1` marshaler, or
  scalar-i64 lowering path.
- Do not broaden into binary128 soft-float, scalar FP, atomics, intrinsics,
  inline asm, callee-save placement, or preserved-value extent work.
- Do not weaken unsupported expectations to claim helper progress.

## Working Model

Idea 236 already owns selected i128 pair lowering and will resume at Step 6
after this prerequisite lands. This runbook creates or validates the producer
facts that let selected AArch64 helper records consume i128 helper boundaries
without guessing helper families, callee symbols, argument/result lanes,
memory-return ownership, or clobber/resource policy.

## Execution Rules

- Keep routine packet progress and proof in `todo.md`.
- Start by inspecting current i128 operation shapes and generic call facts.
- Add prepared/shared carriers at the producer layer that owns helper
  selection, ABI, lane ownership, and clobber/resource policy.
- Preserve fail-closed behavior whenever helper authority is incomplete.
- Treat AArch64-side helper synthesis, helper-name hard-coding, expectation
  rewrites, scalar-i64 substitutes, and fixed-register matching as route drift.
- For code-changing packets, prove with a build plus the supervisor-chosen
  focused prepared/i128 backend subset. Escalate to broader backend validation
  after shared prepared, ABI, or call-boundary behavior changes.

## Ordered Steps

### Step 1: Inspect I128 Helper Authority Gap

Goal: identify the exact producer surfaces that can own i128 helper-required
operation identity, helper selection, lane ownership, memory-return policy,
and clobber/resource facts.

Primary targets:

- BIR i128 div/rem and float/i128 conversion operation shapes
- prepared value-home and i128 lane carrier facts
- generic retained-call `PreparedCallPlan` facts
- ABI, register-bank, and clobber/resource observations

Actions:

- Trace representative helper-required i128 operations from BIR through
  prepared state.
- Identify the source operation identity, helper kind, callee, low/high
  argument lanes, result lanes, memory-return ownership, clobbers, resources,
  ABI, and register-bank transition facts each helper family needs.
- Compare those requirements against existing generic call and prepared value
  facts.
- Record the first implementation packet target and focused proof subset in
  `todo.md`.

Completion check:

- `todo.md` names the exact producer-owned carrier or diagnostic gap to
  implement first, without assigning helper selection to AArch64 target
  lowering.

### Step 2: Define Source Operation To Helper Mapping

Goal: expose helper kind and callee symbol authority for supported
helper-required i128 source operations.

Actions:

- Define prepared/shared records that map source i128 operation identity to
  helper kind and callee symbol.
- Cover supported div/rem and float/i128 conversion families only where the
  source and ABI facts are complete.
- Preserve explicit unsupported diagnostics for unmapped helper families.
- Add focused coverage proving the mapping is structural, not fixture-shaped.

Completion check:

- Supported i128 helper-required operations expose helper kind and callee
  symbol facts, and unsupported mappings fail closed.

### Step 3: Add Low/High Argument And Result Lane Authority

Goal: preserve helper argument and result ownership for i128 low/high lanes.

Actions:

- Add structured low/high argument lane bindings for each supported helper
  family.
- Add structured low/high result lane bindings where results return directly.
- Preserve lane ordering, source/result value identity, and register-bank facts.
- Diagnose incomplete source or result lane authority explicitly.
- Add focused coverage for lane bindings.

Completion check:

- Helper records expose enough low/high argument and direct-result lane facts
  for idea 236 to consume without fixed-register assumptions.

### Step 4: Add Memory-Return Ownership Where Needed

Goal: represent helper ABI cases where i128 results are returned through
memory rather than direct low/high result lanes.

Actions:

- Identify helper families that require memory-return ownership.
- Add memory-return carrier facts with destination identity, storage extent,
  alignment, and ownership.
- Preserve explicit diagnostics for missing memory-return authority.
- Add focused coverage for direct-result versus memory-return helper shapes.

Completion check:

- Memory-return helpers expose structured destination ownership and extent
  facts, while unsupported memory-return cases remain fail-closed.

### Step 5: Add Helper Clobber, Resource, And ABI Authority

Goal: make helper-specific clobber/resource and ABI/register-bank transition
facts explicit for selected consumers.

Actions:

- Add or connect clobber facts for each supported i128 helper family.
- Add resource facts needed by helper boundary consumers, including call
  scratch, stack, or preserved-state requirements where applicable.
- Preserve ABI and register-bank transition facts needed to marshal helper
  arguments and results.
- Add focused coverage for complete and incomplete helper policy facts.

Completion check:

- Supported i128 helper boundary facts include clobber/resource/ABI policy
  that selected AArch64 records can consume directly.

### Step 6: Validate And Hand Back To I128 Pair Lowering

Goal: prove the prerequisite helper authority and make the lifecycle handoff
back to idea 236 explicit.

Actions:

- Run the supervisor-chosen build and focused prepared/i128 backend subset.
- Escalate to broader backend validation if shared prepared, ABI, or
  call-boundary behavior changed beyond one narrow carrier.
- Summarize available helper families, lane ownership, memory-return, and
  clobber/resource facts in `todo.md`.
- Ask the supervisor to route plan-owner to reactivate idea 236 only if Step 6
  helper-boundary authority is complete.

Completion check:

- The prerequisite helper facts are structurally present, incomplete facts
  still fail closed, and `todo.md` names whether idea 236 can resume Step 6.
