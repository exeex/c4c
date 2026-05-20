# AArch64 Frame Slot Layout Consistency Runbook

Status: Active
Source Idea: ideas/open/316_aarch64_frame_slot_layout_consistency.md
Split from: ideas/closed/314_aarch64_large_stack_offset_addressing.md

## Purpose

Continue the AArch64 large stack-slot route after idea 314 repaired legal
instruction spelling for large offsets. The remaining representative evidence
points to a frame-layout consistency problem: emitted stack-slot accesses target
large `sp + offset` addresses while the function prologue reserves a much
smaller frame.

## Goal

Repair the AArch64 frame layout contract so selected frame-slot offsets and the
function frame allocation agree for large stack-slot cases.

## Core Rule

Repair a general AArch64 frame-size/frame-slot layout capability. Do not
special-case `00216.c`, `test_correct_filling`, one literal offset, one stack
slot, one function, one prologue size, or one expected-output mismatch.

## Read First

- `ideas/open/316_aarch64_frame_slot_layout_consistency.md`
- `todo.md`
- generated AArch64 artifacts for `00216.c` under
  `build/c_testsuite_aarch64_backend/src/`, if present
- AArch64 frame layout, stack slot, and memory operand lowering code
- existing focused guardrails for ideas 312, 313, and 314, when choosing proof

## Current Targets

- Representative external case:
  - `c_testsuite_aarch64_backend_src_00216_c`
- Current evidence from the source idea:
  - `test_correct_filling` reserves only 48 bytes with `sub sp, sp, #48`
  - generated accesses still target offsets such as `sp + 1600`,
    `sp + 1624`, `sp + 1644`, and `sp + 1648`
- Owner boundary:
  - legal large-offset spelling was repaired by idea 314
  - this plan owns whether the frame allocation and selected slot offsets are
    mutually consistent

## Non-Goals

- Do not reopen idea 314's large-offset instruction spelling owner without new
  evidence that spelling is again the first bad fact.
- Do not repair `00204.c` large frame setup/teardown materialization here.
- Do not fold semantic prepared-handoff, f128 transport, direct-call shuffle,
  vararg admission, runner behavior, timeout policy, expectation changes,
  unsupported classifications, or CTest registration into this route.
- Do not claim progress through filename-only, function-name-only,
  literal-offset-only, diagnostic-string-only, or c-testsuite-number-specific
  fixes.

## Working Model

The representative has advanced from assembler rejection to runtime mismatch.
The suspected failure shape is that the backend's frame plan contains large
slot offsets, but the emitted prologue allocates too small a frame for those
slots. The first packet should verify the prepared frame plan and emitted
assembly agree on the same frame slots before implementation.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Start from generated artifacts and prepared frame/stack dumps before editing
  implementation.
- Preserve frame-slot identity, width, alignment, call-boundary behavior, and
  idea 314's legal large-offset materialization.
- Add focused prepared-layout or backend coverage before relying on the
  external representative alone.
- If `00216.c` advances past this owner, record the next first bad fact in
  `todo.md` and return it for lifecycle classification if it belongs elsewhere.

## Ordered Steps

### Step 1: Localize The Frame Layout Divergence

Goal: identify the first point where AArch64 frame size, frame slots, and
emitted stack offsets diverge for `00216.c`.

Primary targets:

- generated BIR/prepared/MIR/assembly for `00216.c`
- AArch64 prepared frame plan and stack layout records
- frame prologue allocation and frame-slot memory operand lowering

Actions:

- Map the large accessed offsets in `test_correct_filling` back to prepared
  frame slots and their sizes, alignments, and ownership.
- Compare prepared frame size/alignment against the emitted prologue
  allocation.
- Determine whether the first divergence is frame-size computation, slot
  ordering, offset sign/base convention, stack pointer adjustment, frame-slot
  publication, or memory operand lowering.
- Record the frame size, slot offsets, emitted prologue, accessed addresses,
  and owner classification in `todo.md`.

Completion check:

- `todo.md` names the first frame-layout owner with generated-code evidence
  and separates it from large-offset spelling, scalar stack publication,
  f128 transport, semantic admission, and unrelated runtime mismatch owners.

### Step 2: Repair The Classified Layout Owner

Goal: make the classified AArch64 frame layout source authoritative for both
frame allocation and emitted frame-slot accesses.

Primary target: the backend frame layout or AArch64 stack lowering files
identified by Step 1.

Actions:

- Apply the narrow semantic repair for the classified owner.
- Preserve frame-slot identity, width, alignment, call-boundary behavior, and
  legal large-offset materialization.
- Avoid testcase-shaped matching on representative names, offsets, or emitted
  instruction sequences.

Completion check:

- Generated code no longer accesses owned frame slots outside the frame
  allocation through the repaired path.

### Step 3: Add Focused Layout Coverage

Goal: make the repaired frame-size/frame-slot contract observable in local
backend tests.

Primary target: focused AArch64 prepared-layout, frame-plan, or codegen tests
matching the classified owner.

Actions:

- Add or extend coverage that fails without the repair and passes with it.
- Cover at least one large-offset frame-slot case where emitted slot accesses
  must fit inside the allocated frame.
- Keep idea 312, 313, and 314 guardrails in the focused proof scope when cheap.

Completion check:

- Focused coverage proves the repaired layout owner and preserves adjacent
  stack/frame guardrails.

### Step 4: Validate Representative And Classify Residuals

Goal: prove the repair on focused guardrails and the `00216.c`
representative, then hand off any new first bad fact.

Primary target: supervisor-selected focused proof scope including
`c_testsuite_aarch64_backend_src_00216_c`.

Actions:

- Run the delegated focused proof command and record results in `todo.md`.
- Confirm the frame-size/slot-offset mismatch is gone.
- Confirm idea 312, 313, and 314 focused guardrails remain stable.
- If `00216.c` still fails, classify whether the next first bad fact remains
  in this source idea or belongs to another distinct initiative.

Completion check:

- `todo.md` records fresh proof. The localized frame-layout fault is gone,
  adjacent guardrails remain stable, and any remaining blocker is explicitly
  localized for the next lifecycle decision.
