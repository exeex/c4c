# AArch64 Indexed Aggregate Address And Writeback Plan

Status: Active
Source Idea: ideas/open/348_aarch64_indexed_aggregate_address_writeback.md
Activated After: ideas/open/353_aarch64_local_formal_frame_slot_publication.md

## Purpose

Repair AArch64 generated-code handling for dynamic indexed aggregate and array
addressing when the selected element address, snapshot, or writeback handoff is
lost before loads or stores execute.

Goal: make indexed aggregate stores, loads, swaps, and member accesses write
back through the source-selected element address rather than through stale
snapshots, wrong offsets, fixed globals, or uninitialized temporary slots.

Core Rule: repair general selected-address/writeback semantics; do not
special-case `00176`, `swap`, one global symbol, one stack snapshot slot, one
array index, one register, or one emitted instruction neighborhood.

## Read First

- `ideas/open/348_aarch64_indexed_aggregate_address_writeback.md`
- `ideas/open/353_aarch64_local_formal_frame_slot_publication.md` for the
  split boundary: stale formal/local-slot reads in `partition` were repaired,
  and the remaining `00176` failure is global indexed array snapshot/writeback.
- AArch64 selected-address lowering, aggregate element address computation,
  snapshot temporary handling, emitted load/store selection, and writeback
  handoff code before editing.

## Current Targets

- Current representative: `c_testsuite_aarch64_backend_src_00176_c`.
- First bad fact family: generated `swap` appears to write global indexed array
  elements from uninitialized high stack snapshot slots such as `[sp, #264]`
  and `[sp, #268]`, corrupting the final sorted array after local/formal
  publication has been repaired.
- Regression guardrails: prior indexed aggregate representatives `00130`,
  `00187`, and `00195`; nearby classified representatives `00181` and `00182`
  only after confirming their current first bad facts still reach indexed
  aggregate selected-address/writeback.
- Adjacent focused backend coverage for selected addresses, aggregate element
  stores/loads, global array writes, local array writes, struct-member array
  elements, and frame-slot temporaries selected by the supervisor.

## Non-Goals

- Do not reopen formal-to-local frame-slot publication from idea 353 unless
  fresh evidence shows scalar fixed formals feeding local slots are again the
  first bad fact.
- Do not reopen block label placement from idea 352, recursive call argument
  preservation from idea 349, or unsigned div/rem producer publication from
  idea 350 unless fresh evidence reaches those exact boundaries.
- Do not broaden into variadic aggregate `va_arg`, byval aggregate argument
  lanes, HFA/floating aggregate publication, scalar cast stack-home,
  return-result publication, direct-call publication, static aggregate
  initializer/relocation materialization, semantic admission, runner behavior,
  timeout policy, proof-log behavior, expectation changes, unsupported
  classifications, or CTest registration.
- Do not use filename-only, function-name-only, global-symbol-only,
  stack-offset-only, array-dimension-only, register-only, c-testsuite-number
  specific, or emitted-text-only fixes.

## Working Model

- BIR/prepared lowering should carry the selected aggregate element address
  through address computation, helper temporaries, emitted loads/stores, and
  writeback.
- A snapshot slot may preserve an intermediate value, but it must not replace
  the source-selected destination address or become the source of a store before
  being initialized.
- For a global indexed array swap, each load and store must target the
  dynamically selected element address for the current index, not a stale fixed
  global snapshot or unrelated frame slot.

## Execution Rules

- Start from prepared/generated evidence for the current `00176` `swap`
  boundary before modifying code.
- Prefer a shared selected-address, temporary lifetime, emitted store/load, or
  writeback handoff repair over emitted-instruction pattern matching.
- Preserve prior 348 improvements for `00130`, `00187`, and `00195`.
- Preserve idea 353's formal/local publication repair, idea 352's label path,
  idea 349's recursive call-boundary repair, and idea 350's div/rem owner.
- Treat any fix that recognizes only `00176`, `swap`, one global symbol, one
  stack snapshot slot, one index, one register, or one instruction sequence as
  route drift.
- Reclassify new first bad facts instead of widening this plan by assumption.

## Steps

### Step 1: Localize Global Indexed Array Snapshot/Writeback Boundary

Goal: identify the exact AArch64 owner that loses the selected global array
element address or reads an uninitialized snapshot slot in `swap`.

Primary target: selected-address metadata, aggregate element address lowering,
snapshot temporary creation, emitted indexed load/store selection, and writeback
handoff.

Actions:

- Trace `00176` `swap` from prepared aggregate/index metadata through generated
  AArch64 loads and stores.
- Identify why high stack snapshot slots such as `[sp, #264]` and `[sp, #268]`
  are read before they contain the selected element values.
- Decide whether the owner is selected-address computation, snapshot lifetime,
  temporary-to-store handoff, emitted store addressing, or writeback routing
  with direct evidence.

Completion check:

- `todo.md` records the concrete first bad boundary, representative
  prepared/generated evidence, and the narrow proof subset for the first
  implementation packet.

### Step 2: Add Focused Indexed Aggregate Coverage

Goal: prove dynamic indexed aggregate selected-address/writeback behavior
without depending only on c-testsuite output.

Actions:

- Add or extend focused backend coverage for a global array indexed swap or
  indexed writeback shape that requires dynamic selected addresses.
- Include at least one differently shaped indexed aggregate guardrail, such as
  local byte array, multidimensional local array, or struct-member array
  element access, when the localized owner applies to that shape.
- Keep test contracts independent of `00176`, `swap`, one global symbol, one
  stack slot, one register, or one emitted instruction neighborhood.

Completion check:

- Focused coverage fails without the repair or an existing focused test is
  identified that already exposes stale snapshot or selected-address writeback
  loss.

### Step 3: Repair Selected-Address Snapshot/Writeback Handoff

Goal: make generated AArch64 load from and store to the dynamic indexed
aggregate element selected by the source program.

Actions:

- Repair the localized owner from Step 1 in the smallest shared
  selected-address, temporary lifetime, emitted load/store, or writeback helper.
- Ensure stores do not consume uninitialized snapshot slots and do not collapse
  dynamic element destinations into fixed global or frame-slot snapshots.
- Preserve existing scalar local/formal publication, call argument publication,
  branch/control, return, selected-address, and frame-layout behavior.

Completion check:

- Focused coverage from Step 2 passes, and supervisor-selected adjacent
  indexed aggregate and backend guardrails show no regression.

### Step 4: Prove Representatives And Reclassify Residuals

Goal: verify `00176` advances past the global indexed array snapshot/writeback
failure and confirm the repair is not one representative only.

Actions:

- Run the supervisor-selected external proof for `00176` after focused proof
  is stable.
- Re-run prior 348 representatives `00130`, `00187`, and `00195` or the
  supervisor-selected equivalent guardrails to confirm they remain advanced.
- Use `00181`, `00182`, or another classified representative only when fresh
  evidence shows its current first bad fact reaches the same selected-address
  writeback owner.
- If any representative remains red, reclassify the new first bad fact instead
  of broadening this plan by assumption.

Completion check:

- `todo.md` records whether `00176` passed, advanced past the global indexed
  snapshot/writeback failure, or exposed a new first bad fact; it also records
  at least one differently shaped indexed aggregate proof or a justified
  reclassification.

### Step 5: Broader Guard And Closure Decision

Goal: decide whether the source idea is complete or whether another focused
runbook is needed for remaining in-scope indexed aggregate work.

Actions:

- Run the supervisor-chosen broader backend guard after focused proof is
  stable.
- Confirm adjacent variadic/byval aggregate, scalar cast, scalar local
  writeback, return publication, direct-call publication, local conversion, and
  selected-address guardrails remain stable where selected by the supervisor.
- If the idea is complete, request lifecycle close with matching regression
  logs. If not, leave `todo.md` with the remaining indexed aggregate boundary
  and exact blocker.

Completion check:

- The lifecycle state either has closure-ready proof for idea 348 or a clear
  remaining selected-address/writeback route that does not broaden beyond the
  source idea.
