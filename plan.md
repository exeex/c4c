# Call ABI Import Boundary Cleanup Runbook

Status: Active
Source Idea: ideas/open/690_call_abi_import_boundary_cleanup.md

## Purpose

Activate the final first-wave LIR-to-BIR adapter boundary cleanup idea after
the memory/address provenance import cleanup closed.

## Goal

Clean up call and return ABI import boundaries without moving prepared call
plans, target ABI placement, wrappers, helper protocols, or emission policy
into the adapter.

## Core Rule

This runbook is behavior-preserving adapter cleanup only. It may narrow or
clarify semantic call import contracts, but it must not change prepared call
plans, target lowering, object output, runtime behavior, tests,
expectations, unsupported markers, allowlists, or harness policy.

## Read First

- `ideas/open/690_call_abi_import_boundary_cleanup.md`
- `docs/lir_bir_adapter_boundary/ordered_followup_plan.md`
- `docs/lir_bir_adapter_boundary/interface_inventory_handoff.md`
- `docs/lir_bir_adapter_boundary/responsibility_classification_handoff.md`
- `docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`
- `docs/lir_bir_adapter_boundary/step2_responsibility_classification.md`

## Current Targets

- `src/backend/bir/lir_to_bir/call_abi.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- adapter-private declarations in `src/backend/bir/lir_to_bir/lowering.hpp`
  when needed to narrow call import helpers
- call and return ABI metadata import paths, inline asm admission, runtime
  call admission, intrinsic call metadata import, and related adapter-private
  helper boundaries

## Non-Goals

- Do not edit prepared call plans, `PreparedBirModule`, physical register
  placement, outgoing stack layout, aggregate lane transport, wrappers,
  helper protocols, MIR consumers, target emission, object output, runtime
  behavior, tests, expectations, unsupported markers, allowlists, or timeout
  policy.
- Do not hide target ABI repair, route schema work, prepared publication
  changes, or named backend testcase repair inside this cleanup.
- Do not claim helper renames or classification-only churn as progress if the
  same call ABI import coupling remains.

## Working Model

`Call ABI import` owns semantic admission of LIR signatures, direct calls, call
returns, byval metadata, varargs, HFA metadata, inline asm, runtime calls, and
call/intrinsic metadata into BIR. Downstream prepared/prealloc and target
layers own ABI placement, call plans, stack layout, wrappers, helper
protocols, carriers, MIR consumption, and final instruction emission.

## Execution Rules

- Start each implementation packet by naming the exact call import coupling it
  will narrow.
- Keep packets small enough that the proof can pair a fresh build with the
  existing call/ABI adapter or backend coverage selected by the supervisor.
- If a packet needs prepared call-plan, target ABI placement, MIR, runtime, or
  test/expectation edits, stop and request a separate source idea instead.
- Treat RV64, AArch64, and x86 backend failures as downstream evidence unless
  the failing row is explicitly a semantic LIR-to-BIR call admission issue.
- Record proof and any split-worthy blockers in `todo.md`; do not rewrite this
  plan for routine executor progress.

## Ordered Steps

### Step 1: Establish Call Import Evidence And First Packet Boundary

Goal: consume the boundary docs and choose one narrow behavior-preserving
call ABI import cleanup packet.

Primary targets:

- `ideas/open/690_call_abi_import_boundary_cleanup.md`
- `docs/lir_bir_adapter_boundary/*.md`
- `src/backend/bir/lir_to_bir/call_abi.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`

Actions:

- Inspect the listed docs and source surfaces.
- Identify which helpers or declarations are true adapter-owned call import
  concerns and which are downstream prepared/target concerns.
- Select one first packet that preserves behavior and does not require edits
  outside the current target surface.
- Record the chosen packet, proof command, and any excluded downstream
  coupling in `todo.md`.

Completion check:

- `todo.md` names the chosen first packet and its exact proof command.
- No source, test, expectation, unsupported-marker, allowlist, runtime, or
  downstream prepared/target files are edited during this evidence step.

### Step 2: Narrow Adapter-Owned Call ABI Helper Boundaries

Goal: make one adapter-owned signature, argument, return, byval, vararg, or
HFA import boundary narrower or clearer without changing emitted behavior.

Primary targets:

- `src/backend/bir/lir_to_bir/call_abi.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`

Actions:

- Extract, hide, or rename only when it reduces actual adapter declaration
  width or clarifies ownership.
- Keep semantic BIR records and diagnostics behavior-compatible.
- Leave ABI placement, stack assignment, aggregate transport lanes, and
  prepared call plans downstream.

Completion check:

- The touched helper boundary is narrower or clearer and still adapter-owned.
- Fresh build plus supervisor-selected call/ABI adapter coverage passes.
- Proof is recorded in `todo.md`.

### Step 3: Narrow Call-Lowering Admission Boundaries

Goal: clarify one direct-call, call-return, inline asm, runtime-call, or
call/intrinsic metadata admission boundary in the adapter.

Primary targets:

- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`

Actions:

- Separate semantic BIR call admission from target helper protocols and
  prepared call-plan facts.
- Preserve existing diagnostics, BIR output, target handoff behavior, object
  output, and runtime behavior.
- Split work immediately if the cleanup needs memory intrinsic ownership,
  prepared call plans, target wrappers, or MIR consumers.

Completion check:

- The cleaned boundary remains import-local and behavior-preserving.
- Fresh build plus focused call/inline-asm/runtime-call coverage passes.
- Proof and any split notes are recorded in `todo.md`.

### Step 4: Audit For Downstream Authority Leaks

Goal: verify the completed cleanup did not move downstream authority into
LIR-to-BIR call import.

Primary targets:

- all files touched by Steps 2 and 3
- call-related references in prepared/prealloc and target docs only as review
  context

Actions:

- Check that prepared call-plan, physical register, outgoing stack, wrapper,
  helper protocol, carrier, and target emission authority stayed downstream.
- Check that no named testcase, expectation rewrite, unsupported downgrade,
  allowlist edit, or weaker proof is being used as evidence.
- Record remaining downstream work as a blocker or follow-up note in
  `todo.md`; do not expand this source idea.

Completion check:

- The diff is confined to behavior-preserving adapter boundary cleanup.
- Reviewer reject signals from the source idea are not triggered.

### Step 5: Final Proof And Handoff

Goal: leave the supervisor with a coherent, acceptance-ready lifecycle slice.

Actions:

- Run `cmake --build --preset default`.
- Run the supervisor-selected existing call/ABI adapter or backend subset.
- Escalate to a broader backend subset if public helper signatures or shared
  call import behavior changed.
- Record commands and results in `todo.md`.

Completion check:

- `todo.md` contains fresh proof results.
- No out-of-scope files or contracts changed.
- Remaining non-adapter work, if any, is documented as follow-up rather than
  absorbed into this plan.
