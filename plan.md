# AArch64 Indexed Aggregate Address And Writeback Plan

Status: Active
Source Idea: ideas/open/348_aarch64_indexed_aggregate_address_writeback.md
Activated From: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Repair the AArch64 path for dynamic indexed aggregate and array addressing
where generated code must compute the selected element or member address and
load, store, swap, or write back through that selected address.

Goal: make dynamic indexed aggregate operations preserve and consume the
computed element address instead of writing to a stale base, wrong byte offset,
fixed global snapshot, or stale temporary value.

Core Rule: repair the semantic selected-address/writeback path; do not
special-case c-testsuite filenames, emitted instruction strings, array names,
byte offsets, registers, or individual aggregate shapes.

## Read First

- `ideas/open/348_aarch64_indexed_aggregate_address_writeback.md`
- `ideas/open/295_backend_regex_failure_family_inventory.md`
- Current external representatives from the split: `00130`, `00176`,
  `00181`, `00182`, `00187`, and `00195`.
- Recent adjacent focused owners before editing:
  closed byval aggregate lane publication owner 328,
  `ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md`,
  and other closed variadic/scalar-local/direct-call owners referenced by the
  source idea.

## Current Targets

- AArch64 lowering and printing path from prepared aggregate element selection
  to selected address computation, emitted load/store, and writeback.
- Dynamic indexed loads, stores, swaps, and member accesses for local arrays,
  global arrays, byte buffers, pointer-parameter aggregate paths, and arrays of
  structs.
- Focused backend coverage that exercises representative indexed aggregate
  shapes before relying on external c-testsuite proof.

## Non-Goals

- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, CTest registration, or external test
  contracts.
- Do not fold in variadic aggregate `va_arg`, byval aggregate argument lane
  publication, stdarg cursor progression, HFA/floating aggregate publication,
  fixed-formal entry publication, scalar cast publication, return publication,
  direct-call publication, or local conversion publication without fresh
  first-bad-fact evidence.
- Do not repair static aggregate initializer or relocation materialization for
  `00205.c` or `00216.c` unless localization proves the same dynamic indexed
  address/writeback owner.
- Do not treat boolean/comparison materialization, FP expression lowering,
  semantic `lir_to_bir` admission, timeout, runner, or proof-log behavior as
  part of this owner.

## Working Model

- The selected failures share an address-selection/writeback risk: source code
  selects a dynamic aggregate element or member, but generated AArch64 targets
  the wrong byte offset, loses the computed element address, stores a stale
  temporary, or mutates a fixed global snapshot.
- Representative current evidence includes `00130` storing `arr[1][3]` at the
  wrong local byte offset, `00176` leaving global quicksort data mostly
  unchanged, `00182` losing dynamic buffer writes, `00187` placing a local
  buffer terminator at the wrong byte, `00195` walking `point_array+N` while
  storing stale `d9`, and `00181` crashing through recursive pointer/global
  tower array mutation.
- C-testsuite representatives are proof targets, not implementation selectors.
  Focused backend coverage should identify the repaired owner before broad
  external proof is claimed.

## Execution Rules

- Start by localizing the first bad fact to a concrete prepared address,
  selected address, emitted load/store, or writeback handoff boundary.
- Prefer small focused backend coverage for byte arrays, multidimensional
  locals, global array swaps, pointer-parameter aggregate paths, and
  struct-member array elements.
- Preserve selected addresses across helper temporaries, calls, and recursive
  control flow when those addresses are the source or destination of the
  aggregate operation.
- Treat any change that only fixes one representative or emitted sequence as
  route drift unless nearby same-feature cases are examined.
- Keep validation scoped during implementation, then prove at least two
  differently shaped representatives from the classified bucket advance or
  pass before closure.

## Steps

### Step 1: Localize Indexed Aggregate Address Writeback Boundary

Goal: identify the first boundary where the selected aggregate element address
or writeback target is lost.

Primary target: prepared/BIR-to-AArch64 path and generated AArch64 for the
classified representatives.

Actions:

- Inspect generated AArch64 and prepared artifacts for `00130`, `00176`,
  `00181`, `00182`, `00187`, and `00195`.
- Trace one local byte or multidimensional local case, one global indexed swap
  case, and one struct-member or pointer-parameter case from prepared
  aggregate selection to emitted load/store.
- Record whether the first bad fact is prepared address formation, selected
  address preservation, load/store operand selection, writeback handoff, or
  stale temporary publication.
- Reject any proposed route that depends on a filename, source symbol, fixed
  offset, or single emitted instruction sequence.

Completion check:

- `todo.md` records the concrete first bad boundary, representative evidence,
  and the narrow proof subset for the first implementation packet.

### Step 2: Add Focused Indexed Aggregate Coverage

Goal: create focused backend coverage that fails for the semantic owner before
repair and avoids relying only on c-testsuite representatives.

Actions:

- Add or extend focused backend tests for at least two different indexed
  aggregate shapes from the source idea: byte/multidimensional locals, global
  array swaps, dynamic buffers, pointer-parameter aggregate writes, or
  struct-member array elements.
- Assert selected address/writeback behavior in the smallest local test layer
  that exposes the broken boundary.
- Keep test names and assertions semantic; do not encode c-testsuite filenames
  as implementation contracts.

Completion check:

- The focused tests fail without the repair or document why existing focused
  tests already expose the same first bad boundary.

### Step 3: Repair Selected Address And Writeback Propagation

Goal: make AArch64 lowering preserve and consume the computed selected element
address for dynamic indexed aggregate operations.

Actions:

- Repair the localized boundary from Step 1 in the smallest shared helper or
  lowering path that owns selected aggregate address/writeback semantics.
- Ensure stores, loads, swaps, and member accesses use the selected address
  rather than a stale base, wrong byte offset, stale temporary, or fixed global
  snapshot.
- Preserve existing behavior for variadic/byval aggregate call boundaries,
  scalar casts, scalar local writeback, return publication, direct-call
  publication, and local conversion publication.

Completion check:

- Focused backend coverage from Step 2 passes, and the delegated proof subset
  shows no regression in adjacent guardrails selected by the supervisor.

### Step 4: Prove External Representatives And Reclassify Residuals

Goal: verify that the semantic repair advances the classified c-testsuite
bucket and separate any remaining failures by new first bad fact.

Actions:

- Run the focused external representatives selected by the supervisor from
  `00130`, `00176`, `00181`, `00182`, `00187`, and `00195`.
- Require at least two differently shaped representatives to pass or advance
  past their original first bad fact unless localization proves a narrower
  owner and records the handoff.
- Reclassify any remaining representative failures by current generated-code,
  diagnostic, runtime, timeout, or admission evidence.

Completion check:

- `todo.md` records representative pass/advance results and any residual
  handoff notes with their new first bad facts.

### Step 5: Broader Backend Guard And Closure Decision

Goal: decide whether the source idea is complete or whether remaining indexed
aggregate evidence needs another runbook.

Actions:

- Run the supervisor-chosen broader backend guard after focused proof is
  stable.
- Confirm adjacent focused guardrails remain stable for variadic/byval
  aggregates, scalar casts, scalar local writeback, return publication,
  direct-call publication, and local conversion publication as required by the
  source idea.
- If the idea is complete, request lifecycle close with matching regression
  logs. If not, leave `todo.md` with the remaining indexed aggregate boundary
  and exact blocker.

Completion check:

- The lifecycle state either has closure-ready proof for idea 348 or a clear
  remaining indexed aggregate route that does not broaden beyond the source
  idea.
