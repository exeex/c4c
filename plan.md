# RV64 Object Route Terminator Fragment Lowering Refresh Runbook

Status: Active
Source Idea: ideas/open/396_rv64_object_route_terminator_fragment_lowering_refresh.md
Activated after: ideas/closed/395_rv64_object_route_instruction_fragment_lowering.md

## Purpose

Repair the current RV64 prepared-object route bucket where prepared BIR
terminators reach object emission but are rejected as unsupported terminator
fragments.

## Goal

Classify and lower reusable RV64 terminator-fragment families so prepared
control-flow exits emit valid RV64 object code without testcase-specific
shortcuts.

## Core Rule

RV64 object emission may lower explicit prepared terminator facts. It must not
reconstruct missing BIR CFG semantics, invent fallthroughs, drop exits, or
special-case torture filenames.

## Read First

- `ideas/open/396_rv64_object_route_terminator_fragment_lowering_refresh.md`
- `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
- `ideas/closed/395_rv64_object_route_instruction_fragment_lowering.md`
- `tests/c/external/gcc_torture/src/20020206-2.c`
- Current RV64 gcc_torture backend probe artifacts under `build/agent_state/`
  and `build/rv64_gcc_c_torture_backend/`

## Current Targets

- Primary representative: `src/20020206-2.c`, from the 2026-06-26 reopened
  354 classification terminator-fragment bucket.
- Nearby same-bucket terminator cases selected by the supervisor from current
  RV64 gcc_torture backend artifacts.
- Both conditional and unconditional/return terminator shapes when present in
  the refreshed bucket.

## Non-Goals

- Do not reopen broad CFG ownership or add BIR reconstruction inside object
  emission.
- Do not fix semantic `lir_to_bir` scalar-control-flow failures that never
  reach prepared terminators.
- Do not absorb instruction-fragment, move-bundle, stack-frame, global-data, or
  runtime-mismatch work owned by separate open ideas.
- Do not downgrade torture cases to unsupported, weaken allowlists, or use
  filename-specific branches.

## Working Model

Earlier terminator work closed, but the reopened 354 classification still
found a current unowned `unsupported_terminator_fragment` bucket. Because 395
is now closed after its refreshed seed bucket stopped reproducing owned
instruction-fragment failures, the next lifecycle target is this terminator
bucket.

The first packet should refresh the representative set and classify the
prepared terminator forms that still fail. Implementation should only begin
after the current terminator opcode/operand/control-flow shape is named.

## Execution Rules

- Start with classification proof before implementation.
- Keep each implementation packet tied to one concrete prepared terminator
  family.
- Inspect prepared dumps and object-route diagnostics before editing target
  emission.
- Preserve branch, jump, return, fallthrough, label, relocation, and successor
  semantics.
- If a representative lacks required prepared CFG facts, stop and route that
  producer boundary instead of compensating in the RV64 emitter.
- Use the supervisor-selected proof command and record exact results in
  `todo.md`.
- If a repaired case object-compiles and links, include qemu comparison in the
  proof.
- Treat diagnostic-only churn, expectation rewrites, dropped terminators,
  allowlist-only progress, and named-case green proof as route failures.

## Step 1: Classify Current Terminator Fragment Rejections

Goal: identify the concrete prepared terminator facts currently blocking RV64
object emission.

Actions:

- Reproduce or inspect the supervisor-selected RV64 gcc_torture backend probe
  for `src/20020206-2.c` and nearby same-bucket terminator cases.
- Dump or inspect prepared BIR for each representative and record terminator
  opcode, operands, condition values, successor labels, fallthrough facts, and
  branch/return shape.
- Map each rejection to the RV64 object-route code that emits
  `unsupported_terminator_fragment`.
- Decide whether the first repair packet is a conditional branch,
  unconditional jump, return, or mixed control-flow lowering family.
- Route any missing producer CFG fact to lifecycle review instead of patching
  RV64 object emission.

Completion check:

- `todo.md` records the concrete terminator-fragment family for the first
  executor packet, the representative set, and the exact supervisor-delegated
  proof command.
- Any non-396 residual is routed with precise evidence instead of patched in
  RV64 object emission.

## Step 2: Lower The First Valid Terminator Family

Goal: add reusable RV64 object lowering for the first classified prepared
terminator family when all required prepared facts are explicit.

Actions:

- Update the RV64 object route to consume the prepared terminator facts for
  the selected family.
- Preserve correct label, branch, jump, return, fallthrough, and relocation
  behavior.
- Preserve existing diagnostics for unsupported or incomplete prepared facts.
- Add or update focused backend coverage where the repo has a matching test
  surface for the terminator family.

Completion check:

- The selected representative no longer fails with the same
  `unsupported_terminator_fragment` diagnostic.
- Nearby same-fragment cases examined by the executor either advance together
  or are explicitly classified as separate owners.
- Existing backend tests for adjacent control-flow lowering remain green.

## Step 3: Prove Representatives And Bucket Movement

Goal: prove the terminator repair advanced 396 without hiding runtime or
ownership failures.

Actions:

- Run the supervisor-selected narrow RV64 gcc_torture backend probe for the
  repaired representative and same-fragment additions.
- If a case object-compiles and links, inspect qemu comparison rather than
  stopping at compile success.
- Run the supervisor-selected backend CTest subset for the implementation
  slice.
- Classify any remaining failure as the same terminator family, a distinct
  target-emission family, or a producer-fact gap that needs a separate source
  idea.

Completion check:

- `todo.md` records exact proof commands, results, and residual ownership.
- No expectation rewrites, unsupported downgrades, allowlist filtering, dropped
  terminators, or filename-specific fixes are used as acceptance evidence.
- The supervisor has enough evidence to continue with another 396 packet,
  request route review, or ask the plan owner for close/deactivation handling.
