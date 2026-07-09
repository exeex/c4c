# RV64 Call-Argument Frame-Slot Address Materialization Runbook

Status: Active
Source Idea: ideas/open/648_rv64_call_arg_frame_slot_address_materialization.md
Reactivated after: ideas/closed/656_20000722_local_memory_access_object_route.md

## Purpose

Repair the renewed RV64 representative call-argument lowering mismatch for a
prepared call argument whose selected source is a local frame-slot address
materialization.

## Goal

Classify why `src/20000722-1.c` still emits a stale register-home argument copy
after the local-memory object-route blocker was fixed, then repair only the
semantic call-argument owner if the prepared call plan selects a local
frame-slot address.

## Core Rule

RV64 may use the frame-slot address path only when the prepared call plan
explicitly selects `local_frame_address_materialization`. Do not infer this
from source spelling, stack offsets, final assembly, testcase identity, or
diagnostic text.

## Read First

- `ideas/open/648_rv64_call_arg_frame_slot_address_materialization.md`
- `ideas/closed/656_20000722_local_memory_access_object_route.md`
- `ideas/closed/638_rv64_string_label_pointer_runtime_object_correctness.md`
- `ideas/closed/630_string_constant_local_memory_policy.md`
- `ideas/closed/618_runtime_mismatch_ownership_investigation.md`

## Current Scope

- Representative integration surface: `src/20000722-1.c`.
- The earlier object-route `unsupported_local_memory_access` blocker is closed
  under idea 656.
- Fresh post-656 object-route proof emits an object for `src/20000722-1.c`.
- Fresh post-656 asm evidence reaches call-argument setup and still shows
  `mv a0, s1`.
- Existing focused coverage proves a narrow text-route
  `arg.source_selection=local_frame_address_materialization` case emits
  `addi a0, sp, ...` and forbids stale `mv a0, s1`.

## Non-Goals

- Do not reopen string-constant local-memory admission or broaden
  `StringConstantLabelPointer` policy.
- Do not change the idea 656 local-memory route except through a separate
  lifecycle decision.
- Do not change generic runtime support, branch/control-flow lowering, stack
  layout, or ABI destination-register convention.
- Do not special-case `src/20000722-1.c`, `%lv._clit_`, `foo`, `bar`, `s1`,
  `s2`, or `a0`.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting as progress.

## Working Model

The local-memory blocker no longer prevents representative object emission.
The renewed failure is now back at the call-argument boundary, but the focused
text-route proof and the representative row may be exercising different
prepared facts or lowering paths. First refresh the representative prepared
call evidence after idea 656, then choose an implementation packet only if the
semantic source selection is explicit and the stale copy is produced by the
RV64 call-argument consumer.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Start with refreshed post-656 diagnostics for `src/20000722-1.c`; do not
  rely on pre-split `mv a0, s2` evidence.
- Compare the representative prepared call fact against the focused green test
  before editing lowering code.
- Preserve ordinary register-to-register GPR call-argument lowering when there
  is no explicit `local_frame_address_materialization` source selection.
- If the renewed row exposes a distinct owner outside call-argument
  materialization, request lifecycle split or park instead of expanding this
  runbook.

## Steps

### Step 1: Refresh Post-656 Representative Call Evidence

Goal: Confirm the current prepared call fact and emitted RV64 argument setup
for `src/20000722-1.c` after idea 656.

Actions:

- Run focused prepared-BIR, prepared call-plan or prepared move diagnostics,
  RV64 asm, and RV64 object diagnostics for `src/20000722-1.c`.
- Record the call site, selected source value, frame slot if present, ABI
  destination register, and stale emitted copy shape in `todo.md`.
- Compare the representative prepared fact with the focused green
  `riscv64_call_arg_local_frame_address_materialization` test.
- Confirm whether the first current owner is RV64 consumption of
  `arg.source_selection=local_frame_address_materialization` or a different
  upstream/downstream owner.

Completion check:

- `todo.md` names the exact current prepared fact shape, emitted stale copy,
  and the next boundary to inspect, with no implementation packet selected
  from stale pre-656 evidence.

### Step 2: Locate The Representative Call-Argument Boundary

Goal: Find the narrow branch that handles the representative argument setup.

Actions:

- Trace how the representative prepared call source selection reaches
  `src/backend/mir/riscv/codegen/prepared_call_emit.cpp` and
  `src/backend/mir/riscv/codegen/object_emission.cpp`.
- Determine why the focused green route emits `addi a0, sp, ...` while the
  representative row still emits a stale `mv a0, s1`.
- Identify the smallest positive test or existing test update that would fail
  on the representative stale-copy behavior.
- Identify the fail-closed behavior for missing, ambiguous, or non-frame-slot
  argument sources.

Completion check:

- `todo.md` records the owned implementation surface, focused or
  representative test target, and negative behavior that must be preserved.

### Step 3: Implement Or Split The Narrow Call-Argument Owner

Goal: Repair only a proven call-argument materialization owner, or split if the
owner is not this idea.

Actions:

- If the representative row has explicit
  `arg.source_selection=local_frame_address_materialization`, consume that fact
  in the RV64 call-argument path and emit the selected frame-slot address into
  the ABI argument register.
- Preserve existing register-to-register lowering for ordinary register-backed
  arguments.
- Keep missing, ambiguous, or non-frame-slot source selections rejected rather
  than guessed from stack offsets or source syntax.
- If the stale copy comes from a distinct owner outside call-argument
  materialization, request lifecycle split or park instead of fixing it here.

Completion check:

- Focused positive and negative backend proof passes after a fresh build, or
  lifecycle state records the split owner and parks this route.

### Step 4: Prove Representative Integration

Goal: Show that `src/20000722-1.c` advances through the stale call-argument
copy failure mode.

Actions:

- Rerun the focused call-argument assertion and the RV64 GCC C torture backend
  path for `src/20000722-1.c`.
- Capture object/disassembly evidence showing the call receives the selected
  local frame-slot address.
- Record any remaining downstream owner in `todo.md` if the row advances but
  does not fully pass.

Completion check:

- `src/20000722-1.c` no longer fails because the call argument is copied from
  a stale register home, and proof logs identify the exact validation commands.

### Step 5: Run Broader Validation And Close Or Park

Goal: Decide whether the source idea is complete after the focused repair.

Actions:

- Run the supervisor-selected broader validation for the affected backend
  bucket after focused proof is green.
- If the source idea acceptance criteria are satisfied, request plan-owner
  close with regression-guard proof.
- If a distinct downstream owner remains, record it in `todo.md` and request a
  lifecycle split or park decision rather than expanding this runbook.

Completion check:

- Lifecycle state either closes the source idea with passing guard proof or
  records a precise blocked or follow-up owner without broadening this idea.
