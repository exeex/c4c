# AArch64 Frame Slot Layout Consistency Plan

Status: Active
Source Idea: ideas/open/316_aarch64_frame_slot_layout_consistency.md
Activated From: ideas/open/350_aarch64_unsigned_div_rem_producer_publication.md

## Purpose

Repair AArch64 frame-slot and frame-size consistency when local aggregate or
stack-slot storage can exceed the allocation reserved by a function prologue.

Goal: ensure emitted AArch64 local aggregate and stack-slot accesses fit inside
the frame allocation or otherwise target proven valid owned storage.

Core Rule: repair the frame layout contract; do not special-case `00182`,
`00216`, `print_led`, `buf`, one offset, one function, or one emitted
instruction neighborhood.

## Read First

- `ideas/open/316_aarch64_frame_slot_layout_consistency.md`
- `ideas/open/350_aarch64_unsigned_div_rem_producer_publication.md` for the
  fresh `00182` reactivation evidence and why the remaining failure is no
  longer unsigned div/rem producer publication.
- The frame layout, frame slot assignment, local aggregate lowering, and
  AArch64 prologue emission paths before editing.

## Current Targets

- Current `00182.c` local array representative: `main` declares
  `char buf[5*MAX_DIGITS]`, generated AArch64 allocates only 48 bytes, passes
  `sp` as the buffer argument, and saves `x30` inside the overwritten region.
- Any still relevant focused `00216.c` frame-size or large-slot evidence, only
  after checking whether the current generated artifacts still reproduce a
  frame-size/slot-offset divergence.
- Focused backend or prepared-layout coverage that proves local aggregate and
  stack-slot storage remain within the function frame allocation.

## Non-Goals

- Do not reopen idea 314's large stack-offset instruction-spelling owner.
- Do not repair `00204.c` large frame setup/teardown materialization.
- Do not widen into scalar stack publication, unsigned div/rem producer
  publication, f128 transport, semantic admission, runner behavior, timeout
  policy, expectation changes, unsupported-classification changes, or CTest
  registration.
- Do not use filename-only, function-name-only, literal-offset-only,
  diagnostic-string-only, or c-testsuite-number-specific fixes.

## Working Model

- After the unsigned div/rem repair, `00182` now reaches the LED output write
  path and segfaults because the caller frame does not reserve enough storage
  for the local buffer passed to `print_led`.
- The current first bad fact is a frame-size/local-array storage divergence:
  the generated function prologue allocation is smaller than the local
  aggregate storage that later code writes through.
- Historical `00216.c` evidence belongs to the same general owner only if
  current artifacts still show frame allocation and stack-slot offsets
  disagreeing.

## Execution Rules

- Start from the current `00182` local array failure and identify where frame
  size, frame slots, and local aggregate storage requirements diverge.
- Add focused layout or backend coverage before relying on external
  c-testsuite proof.
- Preserve frame-slot identity, width, alignment, call-boundary behavior, and
  legal large-offset materialization from idea 314.
- Treat any fix that recognizes only `00182`, `print_led`, `buf`, `00216`, one
  offset, or one emitted instruction sequence as route drift.
- Escalate to a separate source idea if fresh evidence reaches large-frame
  setup/teardown, scalar stack publication, unsigned div/rem publication, f128
  transport, semantic admission, or unrelated runtime mismatch work.

## Steps

### Step 1: Localize Current Frame Layout Boundary

Goal: identify the exact boundary where local aggregate storage requirements
stop contributing to the emitted AArch64 frame allocation or stack-slot layout.

Primary target: frame layout, frame slot assignment, local aggregate storage,
and AArch64 prologue/allocation emission for the current `00182.c` local array
representative.

Actions:

- Trace how `char buf[5*MAX_DIGITS]` is represented before AArch64 emission,
  including required size, alignment, slot identity, and address passed to
  `print_led`.
- Compare required local aggregate storage against the prologue allocation and
  any emitted stack addresses used for the buffer.
- Check whether current `00216.c` artifacts still expose a frame-size or
  stack-slot mismatch; classify stale historical evidence explicitly.
- Record the first boundary as one of: layout sizing, slot assignment, frame
  allocation emission, local aggregate address formation, or call-boundary
  storage handoff.

Completion check:

- `todo.md` records the concrete first bad boundary, representative generated
  or prepared-layout evidence, and the narrow proof subset for the first
  implementation packet.

### Step 2: Add Focused Frame Layout Coverage

Goal: prove the frame allocation and emitted local aggregate or stack-slot
addresses agree before relying on external c-testsuite proof.

Actions:

- Add or extend focused backend/prepared-layout tests that cover local
  aggregate storage larger than the minimal scalar frame.
- Assert semantic layout properties: required aggregate size, frame allocation,
  and emitted stack accesses staying inside owned storage.
- Include large-slot evidence only if current localization proves it remains
  part of the same frame-size/slot-offset contract.
- Keep test contracts independent of `00182`, `print_led`, `buf`, `00216`, or
  literal offsets except as representative expected sizes derived from source
  types.

Completion check:

- Focused coverage fails without the repair or existing focused coverage is
  identified that already exposes the frame-size/slot-offset divergence.

### Step 3: Repair Frame Size And Slot Consistency

Goal: make emitted AArch64 frame allocation and stack/local aggregate accesses
agree for the localized frame-layout owner.

Actions:

- Repair the localized owner from Step 1 in the smallest shared frame layout,
  frame slot, or AArch64 prologue/allocation helper that owns the mismatch.
- Ensure local arrays and aggregates reserve enough frame storage before their
  addresses are passed to callees that write through them.
- Preserve existing legal stack-offset materialization and adjacent frame-slot
  behavior.

Completion check:

- Focused coverage from Step 2 passes, and the delegated proof subset shows no
  regression in supervisor-selected frame, stack-offset, and adjacent backend
  guardrails.

### Step 4: Prove External Representative And Reclassify Residuals

Goal: verify that the frame-layout repair advances `00182` past the local-array
frame-size segfault and classify any remaining first bad fact.

Actions:

- Run the supervisor-selected external proof for `00182` after focused layout
  proof is stable.
- Confirm generated AArch64 no longer lets `print_led` or equivalent callees
  overwrite the caller's saved return address through under-allocated local
  buffer storage.
- Recheck relevant current `00216.c` evidence if Step 1 kept it in scope.
- If `00182` remains red, reclassify it by its new first bad fact rather than
  widening this plan by assumption.

Completion check:

- `todo.md` records whether `00182` passed, advanced past the local-array frame
  allocation failure, or exposed a new first bad fact.

### Step 5: Broader Guard And Closure Decision

Goal: decide whether the source idea is complete or whether another focused
runbook is needed for remaining in-scope frame-layout work.

Actions:

- Run the supervisor-chosen broader backend guard after focused proof is
  stable.
- Confirm relevant frame-slot, large-offset, and adjacent backend guardrails
  remain stable.
- If the idea is complete, request lifecycle close with matching regression
  logs. If not, leave `todo.md` with the remaining frame-layout boundary and
  exact blocker.

Completion check:

- The lifecycle state either has closure-ready proof for idea 316 or a clear
  remaining frame-layout route that does not broaden beyond the source idea.
