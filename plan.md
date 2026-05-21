# AArch64 Local/Formal Frame-Slot Publication Plan

Status: Active
Source Idea: ideas/open/353_aarch64_local_formal_frame_slot_publication.md
Activated After: ideas/open/352_aarch64_block_label_emission_ordering.md

## Purpose

Repair AArch64 scalar local/formal publication so incoming fixed formal values
are available in the local frame slots that generated local loads consume.

Goal: prevent local slot reads from using stale stack contents when the source
program initializes locals from incoming scalar formals.

Core Rule: repair the shared scalar formal-to-local publication semantics; do
not special-case `00176`, `partition`, one local name, one stack offset, one
formal register, or one emitted instruction neighborhood.

## Read First

- `ideas/open/353_aarch64_local_formal_frame_slot_publication.md`
- `ideas/open/352_aarch64_block_label_emission_ordering.md` for the split
  boundary: the `.LBB90_6`/`.LBB90_7` branch/label path is repaired and should
  remain intact.
- AArch64 formal home assignment, local stack slot mapping, `load_local` and
  `store_local` lowering, call-boundary publication, and frame-slot addressing
  before editing.

## Current Targets

- Current representative: `c_testsuite_aarch64_backend_src_00176_c`.
- First bad fact family: generated `partition` reads stale stack contents from
  local slots before publishing incoming fixed formals from `w0`/`w1` into the
  local frame slots used for `pivotIndex`, `index`, and `i`.
- Focused backend coverage for scalar fixed formals copied into locals, read
  before and after a call, and returned through a local-derived value.
- Adjacent AArch64 local load/store, call-publication, branch/control, return,
  selected-address, and frame-slot guardrails chosen by the supervisor.

## Non-Goals

- Do not reopen AArch64 block label placement, block ordering, fallthrough
  handling, or return/epilogue placement from idea 352.
- Do not reopen recursive call argument preservation or stale argument-register
  repairs from idea 349 unless fresh evidence reaches that exact boundary.
- Do not broaden into variadic, byval, HFA, aggregate argument, indexed
  aggregate address/writeback, scalar cast stack-home, selected-address,
  broad frame-layout, runner behavior, timeout policy, proof-log behavior,
  expectation changes, unsupported-classification changes, or CTest
  registration.
- Do not use filename-only, function-name-only, local-name-only,
  stack-offset-only, register-number-only, c-testsuite-number-specific, or
  emitted-text-only fixes.

## Working Model

- Prepared formal homes describe where incoming scalar arguments are available
  at function entry.
- Prepared local slots describe where later local loads and stores expect local
  values to live.
- When a local is initialized from an incoming fixed formal, generated AArch64
  must publish the formal register value into the local slot before any local
  load consumes that slot.

## Execution Rules

- Start from prepared/generated evidence for the current `00176` boundary
  before modifying code.
- Prefer a shared publication, value-home, or frame-slot handoff repair over
  emitted-instruction pattern matching.
- Preserve existing block label emission, branch lowering, return lowering,
  selected-address handling, and frame layout unless the localized owner proves
  a narrow frame-slot handoff adjustment is required.
- Treat any fix that recognizes only `00176`, `partition`, one local name, one
  stack offset, one formal register, or one instruction sequence as route
  drift.
- Escalate to a separate source idea if fresh evidence reaches variadic/byval,
  aggregate writeback, scalar cast stack-home, broad frame layout, semantic
  admission, or unrelated runtime mismatch work.

## Steps

### Step 1: Localize Formal To Local Slot Publication Boundary

Goal: identify the exact AArch64 owner that should publish incoming scalar
formals into local frame slots before local loads.

Primary target: formal home metadata, local slot metadata, value publication,
`store_local` and `load_local` lowering, frame-slot addressing, and
call-boundary preservation.

Actions:

- Trace `00176` `partition` from prepared formal/local metadata through
  selected AArch64 local slot loads and stores.
- Identify why `w0`/`w1` are not published into the local slots consumed for
  `pivotIndex`, `index`, and `i` before generated loads read those slots.
- Decide whether the owner is formal home publication, local initialization
  lowering, frame-slot mapping, local load/store addressing, or call-boundary
  value publication with direct evidence.

Completion check:

- `todo.md` records the concrete first bad boundary, representative
  prepared/generated evidence, and the narrow proof subset for the first
  implementation packet.

### Step 2: Add Focused Scalar Formal/Local Coverage

Goal: prove fixed scalar formals copied into locals are published before local
loads, without depending only on c-testsuite output.

Actions:

- Add or extend focused backend coverage for a function that initializes locals
  from incoming scalar formals.
- Include local reads before and after a call boundary, then return a
  local-derived value.
- Keep test contracts independent of `00176`, `partition`, one local name, one
  stack offset, one formal register, or one emitted instruction neighborhood.

Completion check:

- Focused coverage fails without the repair or an existing focused test is
  identified that already exposes stale local-slot reads for scalar fixed
  formals.

### Step 3: Repair Formal To Local Slot Publication

Goal: make generated AArch64 publish incoming scalar fixed formals into the
local frame slots that local loads consume.

Actions:

- Repair the localized owner from Step 1 in the smallest shared publication,
  value-home, local-store, local-load, or frame-slot handoff helper.
- Ensure local loads cannot observe stale stack contents when the local was
  initialized from an incoming scalar formal.
- Preserve existing block label, branch, return, call argument, selected
  address, and frame layout behavior.

Completion check:

- Focused coverage from Step 2 passes, and supervisor-selected adjacent
  local/load-store, call/publication, branch/control, return, selected-address,
  and frame-slot guardrails show no regression.

### Step 4: Prove Representative And Reclassify Residuals

Goal: verify `00176` advances past the bogus `partition` local/formal slot
initialization failure and classify any new first bad fact.

Actions:

- Run the supervisor-selected external proof for `00176` after focused proof
  is stable.
- Confirm `00176` advances past local loads reading stale uninitialized slot
  data for values derived from incoming formals.
- If `00176` remains red, reclassify the new first bad fact instead of
  widening this plan by assumption.

Completion check:

- `todo.md` records whether `00176` passed, advanced past the local/formal
  frame-slot publication failure, or exposed a new first bad fact.

### Step 5: Broader Guard And Closure Decision

Goal: decide whether the source idea is complete or whether another focused
runbook is needed for remaining in-scope publication work.

Actions:

- Run the supervisor-chosen broader backend guard after focused proof is
  stable.
- Confirm adjacent local load/store, call-publication, branch/control, return,
  selected-address, and frame-slot guardrails remain stable.
- If the idea is complete, request lifecycle close with matching regression
  logs. If not, leave `todo.md` with the remaining local/formal publication
  boundary and exact blocker.

Completion check:

- The lifecycle state either has closure-ready proof for idea 353 or a clear
  remaining AArch64 local/formal publication route that does not broaden beyond
  the source idea.
