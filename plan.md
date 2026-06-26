# RV64 Object Route Move Bundle Target Shapes Runbook

Status: Active
Source Idea: ideas/open/397_rv64_object_route_move_bundle_target_shapes.md
Activated after: ideas/closed/396_rv64_object_route_terminator_fragment_lowering_refresh.md

## Purpose

Repair the current RV64 prepared-object route bucket where prepared move
bundles, select-publication moves, or missing copy authority block object
emission.

## Goal

Classify and repair reusable move-bundle target and copy-authority shapes so
prepared value publications lower to valid RV64 object code without dropping
copies or fabricating missing prepared facts.

## Core Rule

RV64 object emission may lower explicit prepared move-bundle facts. It must
not invent move authority, silently discard publications, or convert every
copy into a generic memory operation when the prepared contract does not
authorize that move.

## Read First

- `ideas/open/397_rv64_object_route_move_bundle_target_shapes.md`
- `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
- `ideas/closed/395_rv64_object_route_instruction_fragment_lowering.md`
- `ideas/closed/396_rv64_object_route_terminator_fragment_lowering_refresh.md`
- `tests/c/external/gcc_torture/src/20080519-1.c`
- `tests/c/external/gcc_torture/src/20010604-1.c`
- `tests/c/external/gcc_torture/src/930123-1.c`
- Current RV64 gcc_torture backend probe artifacts under `build/agent_state/`
  and `build/rv64_gcc_c_torture_backend/`

## Current Targets

- Primary move-bundle target representative:
  `tests/c/external/gcc_torture/src/20080519-1.c`
- Select-publication move representative:
  `tests/c/external/gcc_torture/src/20010604-1.c`
- Missing move-bundle authority representative:
  `tests/c/external/gcc_torture/src/930123-1.c`
- Nearby same-bucket move cases selected by the supervisor from current RV64
  gcc_torture backend artifacts.

## Non-Goals

- Do not treat every failed copy as a generic memory operation.
- Do not hide missing move-bundle authority by silently dropping copies.
- Do not fabricate BIR/prepared move-bundle or select-publication facts inside
  MIR/RV64 object emission.
- Do not rewrite unrelated instruction, terminator, stack-frame, global-data,
  or runtime-mismatch lowering.
- Do not downgrade torture cases to unsupported, weaken allowlists, or use
  filename-specific branches.

## Working Model

The reopened 354 classification found 92 move-bundle related failures:

- 86 `unsupported_move_bundle_target_shape`
- 4 `prepared select publication move bundle requires unsupported RV64 moves`
- 2 `prepared_consumer_category=missing_move_bundle`

The first packet should refresh the representative set and classify which of
these are valid RV64 target move shapes versus producer-side missing authority.
Implementation should only begin after the first concrete move family is
named from current artifacts.

## Execution Rules

- Start with classification proof before implementation.
- Keep each implementation packet tied to one concrete move-bundle target or
  copy-authority family.
- Inspect prepared dumps and object-route diagnostics before editing target
  emission.
- Preserve value identity, register bank, stack slot, width, publication, and
  select-carrier semantics.
- If a representative lacks required prepared move authority, stop and route
  that producer boundary instead of compensating in the RV64 emitter.
- Use the supervisor-selected proof command and record exact results in
  `todo.md`.
- If a repaired case object-compiles and links, include qemu comparison in the
  proof.
- Treat dropped copies, expectation rewrites, allowlist filtering, and
  named-case green proof as route failures.

## Step 1: Classify Current Move Bundle Rejections

Goal: identify the concrete prepared move-bundle and copy-authority facts
currently blocking RV64 object emission.

Actions:

- Reproduce or inspect the supervisor-selected RV64 gcc_torture backend probe
  for `20080519-1.c`, `20010604-1.c`, `930123-1.c`, and nearby same-bucket
  move cases.
- Dump or inspect prepared BIR for each representative and record source
  placement, destination placement, value bank, width, stack/register target,
  select-publication facts, and move-bundle authority.
- Map each rejection to the RV64 object-route code that emits the current
  move-bundle diagnostic.
- Decide whether the first repair packet is a register move, stack move,
  stack/register crossing, select-publication move, or producer missing-fact
  route.
- Route any missing prepared authority to lifecycle review instead of patching
  RV64 object emission.

Completion check:

- `todo.md` records the concrete move-bundle family for the first executor
  packet, the representative set, and the exact supervisor-delegated proof
  command.
- Any non-397 residual is routed with precise evidence instead of patched in
  RV64 object emission.

## Step 2: Lower The First Valid Move Bundle Family

Goal: add reusable RV64 object lowering for the first classified move-bundle
family when all required prepared facts are explicit.

Actions:

- Update the RV64 object route to consume the prepared move-bundle facts for
  the selected family.
- Preserve correct register, stack slot, width, bank, publication, and
  select-carrier behavior.
- Preserve existing diagnostics for unsupported or incomplete prepared facts.
- Add or update focused backend coverage where the repo has a matching test
  surface for the move family.

Completion check:

- The selected representative no longer fails with the same move-bundle
  diagnostic.
- Nearby same-fragment cases examined by the executor either advance together
  or are explicitly classified as separate owners.
- Existing backend tests for adjacent move, stack, register, and publication
  lowering remain green.

## Step 3: Prove Representatives And Residual Ownership

Goal: prove the move-bundle repair advanced 397 without hiding runtime or
ownership failures.

Actions:

- Run the supervisor-selected narrow RV64 gcc_torture backend probe for the
  repaired representative and same-fragment additions.
- If a case object-compiles and links, inspect qemu comparison rather than
  stopping at compile success.
- Run the supervisor-selected backend CTest subset for the implementation
  slice.
- Classify any remaining failure as the same move-bundle family, a distinct
  target-emission family, or a producer-fact gap that needs a separate source
  idea.

Completion check:

- `todo.md` records exact proof commands, results, and residual ownership.
- No expectation rewrites, unsupported downgrades, allowlist filtering,
  dropped copies, fabricated move authority, or filename-specific fixes are
  used as acceptance evidence.
- The supervisor has enough evidence to continue with another 397 packet,
  request route review, or ask the plan owner for close/deactivation handling.
