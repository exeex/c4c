# RV64 Frame-Slot Call-Argument Publication Runbook

Status: Active
Source Idea: ideas/open/444_frame_slot_call_argument_publication.md
Activated from: open idea inventory after closure of RV64 prepared value operand materialization

## Purpose

Repair ordinary-C pointer call arguments where prepared BIR knows a frame-slot
address payload, but call-argument publication leaves RV64 lowering to consume
stale or incidental register state.

## Goal

Publish and materialize frame-slot address arguments so RV64 call lowering
passes the intended local-object addresses to callees, starting from the
`src/20080506-2.c` first-wrong-edge evidence.

## Core Rule

Prepared call-argument publication must carry the frame-slot address authority
that RV64 consumes. Do not reconstruct frame-slot payloads in RV64 from source
shape, testcase names, function names, local variable names, block indexes, or
final qemu exit modes.

## Read First

- `ideas/open/444_frame_slot_call_argument_publication.md`
- `docs/rv64_gcc_torture_post_contract/runtime_first_wrong_edge.md`
- `docs/rv64_gcc_torture_post_contract/runtime_mismatch_groups.md`
- `docs/rv64_gcc_torture_post_contract/runtime_mismatch_representatives.md`
- `build/agent_state/423_step3_first_wrong_edge/src_20080506-2.c.bir.txt`, if present
- `build/agent_state/423_step3_first_wrong_edge/src_20080506-2.c.prepared-bir.txt`, if present
- `build/agent_state/423_step3_first_wrong_edge/src_20080506-2.c.mir.txt`, if present
- `build/agent_state/423_step3_first_wrong_edge/src_20080506-2.c.c4c-bin-objdump.txt`, if present

## Current Targets And Scope

- Starting representative: `src/20080506-2.c`.
- Failure owner: prepared call-argument publication feeding RV64 call lowering.
- Argument shape: address-of-local or frame-slot address payloads passed as
  ordinary pointer arguments.
- Evidence source: semantic BIR, prepared BIR, focused dumps, emitted RV64, and
  focused ordinary-C validation.
- Nearby probes: additional ordinary-C address-of-local call cases, including
  multiple pointer arguments and argument positions beyond the first.

## Non-Goals

- Do not change runtime expectations, output comparisons, allowlists,
  unsupported markers, or pass/fail accounting.
- Do not assign every segfault runtime row to this owner based only on qemu
  signal `139`.
- Do not repair generic RV64 binary/value operand materialization; that belongs
  to the closed prepared-value route.
- Do not repair inline asm tied-output/result materialization in this route.
- Do not infer frame-slot argument facts in RV64 when prepared BIR does not
  publish the needed authority.

## Working Model

- Semantic BIR passes both local pointer values to `foo` in `src/20080506-2.c`.
- Prepared BIR identifies the second call argument as a frame-slot address,
  records the intended payload, and also reports
  `missing_frame_slot_arg_publication=yes`.
- The linked RV64 binary then passes a wrong or uninitialized register for the
  second pointer argument.
- The acceptable repair is a general producer/publication rule plus direct RV64
  consumption of that published payload, with nearby ordinary-C coverage.

## Execution Rules

- Start from focused evidence before editing code.
- Keep changes within prepared call-argument publication and the RV64
  consumption surface needed for frame-slot address arguments.
- Preserve fail-closed diagnostics when frame-slot payload authority is absent
  or incoherent.
- Stop and report if evidence shows a missing upstream frame-slot fact rather
  than a publication/consumption bug.
- Add focused tests that prove address-of-local pointer arguments generally,
  not only the starting representative.
- For code-changing steps, require fresh build proof, focused representative
  proof, focused coverage, and the supervisor-selected regression subset before
  acceptance.

## Step 1: Re-establish Frame-Slot Argument Evidence

Goal: confirm the current failure edge and the prepared payload authority that
the implementation must publish and consume.

Primary target: `src/20080506-2.c` evidence from the runtime triage handoff.

Actions:

- Inspect the stored Step 3 first-wrong-edge artifacts when present.
- Reproduce fresh focused dumps if the artifacts are missing or stale enough to
  block implementation.
- Identify the semantic BIR call arguments, the prepared BIR
  `missing_frame_slot_arg_publication=yes` record, and the payload record for
  the affected frame-slot address argument.
- Identify the emitted RV64 call setup that passes stale or uninitialized state.
- Record local execution evidence in `todo.md`, not in the source idea.

Completion check:

- The executor can name the prepared call-argument records and emitted RV64 call
  setup edges that must change before editing implementation code.

## Step 2: Trace Prepared Publication And RV64 Call Consumption

Goal: locate where frame-slot address payloads should become call-argument
publication records and where RV64 call lowering consumes them.

Primary target: prepared call-argument publication and RV64 call-argument
materialization code.

Actions:

- Trace how frame-slot address selections are represented before prepared call
  publication.
- Locate the publication path that records the payload but still reports
  missing frame-slot argument publication.
- Locate the RV64 call-lowering path that selects registers or stack homes for
  pointer arguments.
- Separate producer publication bugs from RV64 target-consumption bugs; stop if
  the frame-slot payload itself is not published by the producer layer.

Completion check:

- The implementation site and data-flow rule are identified without relying on
  testcase-specific names, local variable names, or final segfault status.

## Step 3: Repair Frame-Slot Argument Publication And Consumption

Goal: publish coherent frame-slot address argument authority and make RV64 call
lowering use that published payload directly.

Primary target: the smallest prepared publication and RV64 consumption surface
identified in Step 2.

Actions:

- Implement a general publication rule for frame-slot address call arguments
  where the prepared payload is known.
- Ensure RV64 call lowering materializes the published frame-slot address into
  the intended argument location instead of reusing incidental register state.
- Preserve fail-closed behavior when prepared payload authority is missing,
  contradictory, or outside the supported frame-slot address shape.
- Avoid broad rewrites of unrelated binary operand, inline-asm, F128, global
  data, or stack-frame lowering paths.

Completion check:

- `src/20080506-2.c` no longer reaches the recorded missing-publication or
  stale-argument edge, and the code path is expressed as a general frame-slot
  call-argument repair.

## Step 4: Add Nearby Ordinary-C Coverage

Goal: prove the repair is not limited to `src/20080506-2.c`.

Primary target: focused tests or probes around address-of-local pointer
arguments passed through calls.

Actions:

- Add or update focused ordinary-C validation for frame-slot address arguments.
- Include cases with multiple pointer arguments and nonzero argument positions.
- Include a case that would fail if the second pointer argument reused stale
  register state.
- Keep tests on supported behavior; do not weaken unsupported or runtime
  comparison contracts.

Completion check:

- Focused coverage demonstrates the same frame-slot argument route across
  nearby ordinary-C call shapes.

## Step 5: Validate And Handoff

Goal: produce acceptance evidence for the implementation slice and leave clear
state for lifecycle close or follow-up execution.

Primary target: build, focused representative proof, focused coverage, and the
supervisor-selected regression subset.

Actions:

- Run a fresh build or compile proof after implementation.
- Run the focused `src/20080506-2.c` representative proof.
- Run the focused tests added or updated in Step 4.
- Run the exact regression subset delegated by the supervisor.
- Record proof commands and outcomes in `todo.md`.

Completion check:

- Build, focused representative, nearby coverage, and delegated regression
  proof are available for supervisor acceptance without expectation downgrades,
  segfault-bucket overclaiming, or testcase-shaped shortcuts.
