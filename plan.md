# RV64 Prepared Value Operand Materialization Runbook

Status: Active
Source Idea: ideas/open/443_rv64_prepared_value_operand_materialization.md
Activated from: open idea inventory after closure of RV64 runtime mismatch triage

## Purpose

Repair RV64 lowering for ordinary-C scalar value consumers that currently reuse
stale or unrelated operand registers even though prepared BIR publishes the
intended producer graph.

## Goal

Make binary, comparison, store, load, and branch/compare consumers materialize
the prepared value producer they actually consume, starting from the
`src/pr81503.c` first-wrong-edge evidence.

## Core Rule

Consume prepared value authority directly. Do not infer producer facts in RV64
from testcase names, source names, function names, raw source shape, block
indexes, register accidents, or final qemu exit codes.

## Read First

- `ideas/open/443_rv64_prepared_value_operand_materialization.md`
- `docs/rv64_gcc_torture_post_contract/runtime_first_wrong_edge.md`
- `docs/rv64_gcc_torture_post_contract/runtime_mismatch_groups.md`
- `docs/rv64_gcc_torture_post_contract/runtime_mismatch_representatives.md`
- `build/agent_state/423_step3_first_wrong_edge/src_pr81503.c.bir.txt`, if present
- `build/agent_state/423_step3_first_wrong_edge/src_pr81503.c.prepared-bir.txt`, if present
- `build/agent_state/423_step3_first_wrong_edge/src_pr81503.c.mir.txt`, if present
- `build/agent_state/423_step3_first_wrong_edge/src_pr81503.c.c4c-bin-objdump.txt`, if present

## Current Targets And Scope

- Starting representative: `src/pr81503.c`.
- Failure owner: RV64 lowering consumption of the prepared BIR value graph.
- Value families: prepared binary producers, comparisons, zext-derived values,
  store sources, loaded scalar consumers, and later branch/compare operands.
- Evidence source: semantic BIR, prepared BIR, focused dumps, emitted RV64, and
  focused ordinary-C validation.

## Non-Goals

- Do not change runtime expectations, output comparisons, allowlists,
  unsupported markers, or pass/fail accounting.
- Do not repair frame-slot call-argument publication; that belongs to
  `ideas/open/444_frame_slot_call_argument_publication.md`.
- Do not repair inline asm tied-output or result materialization in this route.
- Do not claim abort, segfault, or other runtime rows without independent
  first-wrong-edge evidence.
- Do not reconstruct missing producer facts inside RV64 when prepared BIR does
  not publish the needed authority.

## Working Model

- Prepared BIR already records the intended `src/pr81503.c` store source as a
  binary producer and preserves the value graph through the final store.
- The runtime bug appears after prepared BIR, where RV64 emission selects or
  reuses the wrong values for an expression and a later comparison.
- The acceptable fix is a general prepared-value consumption or materialization
  rule with nearby ordinary-C coverage, not a named-case patch.

## Execution Rules

- Start from focused evidence before editing code.
- Keep changes within the value-consumption/materialization path needed by this
  source idea.
- Prefer small semantic repairs over broad RV64 rewrites.
- Preserve fail-closed diagnostics when producer authority is absent or
  incoherent.
- Add focused tests that prove nearby value-producer and consumer behavior, not
  just the starting representative.
- For code-changing steps, require fresh build proof, focused representative
  proof, and the supervisor-selected regression subset before acceptance.

## Step 1: Re-establish Prepared Value Evidence

Goal: confirm the current failure edge and the prepared value authority that
the implementation must consume.

Primary target: `src/pr81503.c` evidence from the runtime triage handoff.

Actions:

- Inspect the stored Step 3 first-wrong-edge artifacts when present.
- Reproduce fresh focused dumps if the artifacts are missing or stale enough to
  block implementation.
- Identify the prepared producer records for the final store source and the
  later value consumers that use stale registers.
- Record any current local evidence needed for implementation in `todo.md`,
  not in the source idea.

Completion check:

- The executor can name the prepared value records and RV64 consumer edges that
  must change before editing implementation code.

## Step 2: Trace RV64 Value Consumer Selection

Goal: find the RV64 lowering path that chooses stale operands instead of the
prepared producer-backed values.

Primary target: RV64 lowering and materialization code that handles prepared
binary, comparison, store-source, load, and branch/compare operands.

Actions:

- Trace how prepared producer identity is represented before RV64 emission.
- Locate where binary/comparison operands are assigned, reused, or reloaded.
- Locate the path that materializes the loaded global `c` value before the
  final compare.
- Separate missing prepared authority from target consumption bugs; stop if
  evidence proves a producer gap instead.

Completion check:

- The implementation site and data-flow rule are identified without relying on
  testcase-specific names or final runtime status.

## Step 3: Repair Prepared Value Consumption

Goal: make RV64 emission use the intended prepared producer-backed values for
the affected ordinary-C scalar consumers.

Primary target: the smallest RV64 value materialization or operand-selection
surface identified in Step 2.

Actions:

- Implement a general consumption rule for prepared binary/comparison value
  producers and their store/load/branch consumers.
- Ensure the rule materializes or selects the correct register/value at each
  consumer point instead of reusing incidental live registers.
- Preserve existing fail-closed behavior when prepared producer information is
  missing or contradictory.
- Avoid broad rewrites of unrelated call, frame-slot, inline-asm, or F128
  lowering paths.

Completion check:

- `src/pr81503.c` no longer reaches the recorded wrong operand/materialization
  edge, and the code path is expressed as a general prepared-value consumption
  repair.

## Step 4: Add Nearby Ordinary-C Coverage

Goal: prove the repair is not limited to `src/pr81503.c`.

Primary target: focused tests or probes around binary, comparison, zext, store,
load, and branch/compare consumers.

Actions:

- Add or update focused ordinary-C validation for nearby producer-consumer
  shapes.
- Include cases that would fail if RV64 reused stale operand registers.
- Keep tests on supported behavior; do not weaken unsupported or runtime
  comparison contracts.
- Do not add text-only testcase-shape matching as proof of capability.

Completion check:

- Focused coverage demonstrates the same prepared-value route across nearby
  ordinary-C scalar value shapes.

## Step 5: Validate And Handoff

Goal: produce acceptance evidence for the implementation slice and leave clear
state for lifecycle close or follow-up execution.

Primary target: build, focused representative proof, focused coverage, and the
supervisor-selected regression subset.

Actions:

- Run a fresh build or compile proof after implementation.
- Run the focused `src/pr81503.c` representative proof.
- Run the focused tests added or updated in Step 4.
- Run the exact regression subset delegated by the supervisor.
- Record proof commands and outcomes in `todo.md`.

Completion check:

- Build, focused representative, nearby coverage, and delegated regression
  proof are available for supervisor acceptance without expectation downgrades
  or testcase-shaped shortcuts.
