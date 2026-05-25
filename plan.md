# AArch64 Call Move Boundary Consolidation Checkpoint

Status: Active
Source Idea: ideas/open/02_aarch64_calls_emission_consolidation.md

## Purpose

Continue AArch64 calls-emission consolidation after the Step 5 closure review
for the call printing/effect-publication checkpoint rejected source-idea
closure.

## Goal

Decide and execute the next narrow checkpoint for the surviving
`calls_moves.cpp` and call-boundary move surface, leaving target-local calls
code responsible for AArch64 machine-node emission rather than rederiving
prepared call-plan decisions.

## Core Rule

Target-local AArch64 calls code may convert prepared move, preservation, and
call-boundary facts into AArch64 machine instructions. It must not duplicate
call-plan authority, argument placement decisions, preservation eligibility, or
call-boundary classification that is already available from shared prepared
facts.

## Latest Closure Review Finding

The Step 5 closure review after the call printing/effect-publication
checkpoint rejects source-idea closure and keeps the idea active.

The completed checkpoint moved pure call printing into the machine-printer
layer and narrowed the remaining `calls_printing.cpp` declarations to
machine-node construction:

- `make_call_boundary_move_instruction`
- `make_call_boundary_abi_binding_instruction`
- `make_call_instruction`

The checkpoint's broader backend proof was recorded in `todo.md` as:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`,
with 162/162 backend tests passing. The current workspace does not contain
`test_after.log`, so a close-time regression guard could not be accepted even
if source completion were otherwise true.

The source idea remains open because the AArch64 calls family still contains
multiple large target-local translation units and a broad exported surface in
`calls.hpp`. In particular, `calls_moves.cpp`, `calls_preservation.cpp`, and
`calls_dispatch_bridge.cpp` still contain prepared lookup, preservation,
call-boundary, argument-source, and diagnostic reconstruction paths that must
be reviewed against the source idea's acceptance criteria.

## Read First

- `ideas/open/02_aarch64_calls_emission_consolidation.md`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_preservation.cpp`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`
- `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp`
- Focused backend call-boundary, prepared-call, preservation, byval,
  aggregate, and argument-source tests under `tests/backend/mir/`

## Current Targets / Scope

- `calls_moves.cpp`
- `calls_preservation.cpp`
- `calls_argument_sources.cpp`
- `calls_dispatch_bridge.cpp` only where it feeds call-boundary move emission
- `calls_byval_aggregates.cpp` only where Step 1 proves it owns call argument
  move duplication
- move and preservation declarations in `calls.hpp`
- build metadata only if a calls translation unit is retired

## Non-Goals

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not perform broad dispatch cleanup outside AArch64 call move emission.
- Do not invent a new shared call-plan API.
- Do not move AArch64-specific register, memory operand, frame-slot address,
  byval lane, constant-materialization, or assembly spelling details into
  shared planning.
- Do not weaken unsupported or expected-output contracts.
- Do not change behavior solely to reduce line count.
- Do not absorb unrelated ALU, memory, comparison, prologue, returns,
  variadic, inline-asm, or whole-dispatch cleanup into this checkpoint.
- Do not revisit completed aggregate-address, local-frame publication,
  prior stack-preservation lookup, block-entry republication, frame-slot call
  argument narrowing, value-id prior-preservation lookup, call-argument
  prior-preservation lookup, or call printing/effect-publication routes unless
  this checkpoint proves one still owns duplicate prepared-fact authority.

## Working Model

- Treat `calls_moves.cpp` as the next largest mixed boundary: it lowers
  prepared move bundles into AArch64 machine instructions, but may still carry
  target-local decision logic that should be represented by prepared facts.
- Keep AArch64-specific operand assembly, instruction selection, byval lane
  materialization, and diagnostics where emission needs them.
- Prefer deleting or narrowing duplicate decision logic over moving it into a
  different calls file.
- A helper is acceptable when its parameters and callers describe emission from
  prepared facts. It is suspect when it scans prepared structures to rediscover
  call-plan decisions that a shared prepared lookup already owns.
- Any surviving `calls.hpp` declaration should have a clear owner:
  emission-node construction, source-operand assembly, preservation
  publication, or a precise documented blocker.

## Execution Rules

- Start by mapping every exported move, preservation, and argument-source
  declaration in `calls.hpp` to either AArch64 emission, prepared-fact lookup,
  duplicate planning, or unrelated dispatch bridge work.
- Keep each code slice narrow enough for a fresh build plus focused backend
  proof.
- Escalate to `^backend_` after changing prepared move classification,
  preservation publication, argument-source reconstruction, byval aggregate
  move emission, call-boundary machine instructions, or build metadata.
- Reject helper renames, expectation rewrites, and testcase-shaped shortcuts as
  progress.
- If Step 1 proves the next coherent target is outside this source idea, stop
  and record that as a lifecycle blocker instead of expanding this runbook.

## Step 1: Map The Call Move And Preservation Boundary

Goal: identify which surviving calls-family move and preservation
responsibilities are true AArch64 emission and which rederive prepared
call-plan decisions.

Primary targets:

- `calls.hpp`
- `calls_moves.cpp`
- `calls_preservation.cpp`
- `calls_argument_sources.cpp`
- `calls_dispatch_bridge.cpp`
- `calls_byval_aggregates.cpp`

Actions:

- Classify exported helpers for before-call moves, after-call moves,
  before-return moves, value moves, preservation republication, prior
  preserved-value lookup, and argument-source construction.
- Identify helpers that scan prepared call plans, move bundles, value homes, or
  ABI bindings to rediscover decisions instead of consuming a direct prepared
  fact.
- Decide whether each suspect helper should be deleted, narrowed to
  emission-only operands, or left in place behind a precise shared-prepared-fact
  blocker.
- Select a focused proof command that covers AArch64 call moves,
  call-boundary moves, preserved call arguments, byval aggregate arguments, and
  prepared preservation effects.
- Record the ownership map, blockers, and proof command in `todo.md`.

Completion check:

- `todo.md` names the selected next code-changing target, the reason it remains
  in this source idea, any missing prepared-fact blocker, and the focused proof
  scope.

## Step 2: Remove Or Narrow Duplicate Move Decision Logic

Goal: consolidate one coherent duplicate decision path from Step 1 while
preserving AArch64 emission behavior.

Actions:

- Delete or narrow target-local logic that Step 1 proves is already represented
  by prepared call-plan, move-bundle, preservation, or argument-source facts.
- Keep AArch64 operand assembly, instruction record creation, and machine
  instruction details local to the target.
- Preserve diagnostics unless the prepared fact now provides an equivalent or
  stronger contract.
- Update `calls.hpp` declarations only when the exported helper boundary
  changes.
- Run `cmake --build --preset default` plus the focused backend proof selected
  in Step 1.

Completion check:

- One move or preservation boundary no longer duplicates prepared decision
  authority, and `test_after.log` records a passing build plus focused proof.

## Step 3: Consolidate The Remaining Export Surface

Goal: shrink or explicitly justify the remaining calls-family move and
preservation declarations after Step 2.

Actions:

- Remove obsolete declarations and includes made unnecessary by Step 2.
- Fold small emission-only helpers into their direct owner when that reduces
  exported API without hiding behavior.
- Keep helper files only when each has a clear emission-only boundary.
- Update build metadata only if a file is retired.
- Run a fresh build and focused backend proof after each coherent boundary
  change.

Completion check:

- The affected calls-family API is smaller or each surviving exported helper
  has a precise emission-only ownership reason recorded in `todo.md`.

## Step 4: Broader Backend Checkpoint

Goal: prove the call move boundary checkpoint did not regress adjacent call
emission, preservation, byval, aggregate, local-frame, dispatch-bridge, or
printer behavior.

Actions:

- Run the supervisor-selected broader backend validation scope.
- Include focused AArch64 call-boundary, prepared-call, preservation,
  argument-source, byval, aggregate, local-frame/publication, and
  dispatch-bridge tests.
- Include affected shared-boundary and x86 tests if shared prepared facts or
  machine-node effect behavior were touched.
- Record exact proof commands and results in `todo.md`.

Completion check:

- The broader checkpoint passes, or `todo.md` records a precise blocker tied to
  the active source idea.

## Step 5: Closure Review

Goal: decide whether the source idea is complete after this checkpoint or
whether another runbook checkpoint is needed.

Actions:

- Recheck all surviving `calls*.cpp` and `calls.hpp` boundaries against the
  source idea acceptance criteria.
- Confirm target-local calls code no longer rederives decisions already present
  in shared prepared facts.
- Confirm surviving helper files are emission-only, dispatch-owned outside
  this source idea, or identified as the next checkpoint target.
- If durable remaining work exists, keep the idea open and request another
  runbook checkpoint instead of closing.

Completion check:

- Lifecycle state gives the supervisor an unambiguous close/keep-active
  decision.
