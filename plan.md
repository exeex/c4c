# AArch64 Backend Non-Leaf Call-Frame LR Preservation

Status: Active
Source Idea: ideas/open/285_aarch64_backend_nonleaf_call_frame_lr_preservation.md
Supersedes Active Umbrella: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md

## Purpose

Repair the AArch64 backend rule for non-leaf functions so calls cannot clobber
the return address used by a later `ret`.

## Goal

Functions that emit `bl` preserve and restore `x30` through the existing
AArch64 frame/call/return machinery, then prove the repair on the simple
timeout probes `00100.c`, `00116.c`, and `00121.c`.

## Core Rule

Fix the semantic non-leaf frame rule. Do not special-case c-testsuite files,
function names, exact assembly text, timeout settings, allowlists, unsupported
classifications, or expected outputs.

## Read First

- `ideas/open/285_aarch64_backend_nonleaf_call_frame_lr_preservation.md`
- `src/backend/mir/aarch64/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`
- `src/backend/mir/aarch64/codegen/prologue.cpp`
- `src/backend/mir/aarch64/codegen/returns.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `tests/c/external/c-testsuite/src/00100.c`
- `tests/c/external/c-testsuite/src/00116.c`
- `tests/c/external/c-testsuite/src/00121.c`

## Current Targets

- AArch64 frame/prologue/epilogue lowering for functions containing calls.
- Link-register preservation around non-leaf returns.
- Focused backend tests plus the three simple c-testsuite runtime probes.

## Non-Goals

- Do not repair printf, string-literal, variadic-call, loop predicate,
  short-circuit, aggregate/pointer, static-global, or goto defects in this
  route.
- Do not reclassify the whole 212-case scan in the first execution packet.
- Do not weaken test contracts or runner behavior.

## Working Model

The timeout inventory indicates that generated non-leaf AArch64 functions
execute `bl`, clobber `x30`, and later `ret` through the clobbered link
register. The repair should make the backend know when a function is non-leaf
and make the frame/return path preserve the caller return address under the
chosen ABI-consistent frame strategy.

## Execution Rules

- Start with a minimal backend proof for ordinary user-function calls before
  touching compound timeout cases.
- Prefer existing AArch64 frame and machine-node abstractions over new ad hoc
  emit paths.
- Keep stack alignment explicit when adding any LR spill/restore behavior.
- Record remaining failures as separate owners when the LR timeout shape is no
  longer present.
- Run fresh build or compile proof before accepting any code-changing packet.

## Steps

### Step 1: Confirm Non-Leaf Failure Shape

Goal: Establish the exact backend path that emits calls and returns without
return-address preservation.

Primary target: AArch64 frame, call, and return lowering.

Actions:

- Inspect the current generated assembly for `00100.c`, `00116.c`, and
  `00121.c`.
- Trace where the backend records or can derive that a function contains a
  call.
- Identify the existing frame/prologue/epilogue hooks that should own `x30`
  save/restore.
- Check nearby AArch64 backend tests for an existing non-leaf frame contract.

Completion check: `todo.md` names the owning implementation surfaces and the
proof command the executor should use for the first code slice.

### Step 2: Add Focused Backend Contract Coverage

Goal: Create or update a narrow backend test that would fail while non-leaf
functions can return through a clobbered link register.

Primary target: `tests/backend/mir/` AArch64 backend coverage.

Actions:

- Add focused non-leaf call-frame expectations using existing test patterns.
- Assert semantic frame behavior rather than only checking one emitted text
  sequence when a stronger backend-level assertion is available.
- Keep runtime c-testsuite probes out of the unit-test contract unless the
  existing test harness already supports that shape.

Completion check: the new or updated focused test fails before the repair and
passes after the repair.

### Step 3: Repair LR Preservation For Non-Leaf Functions

Goal: Implement ABI-consistent return-address preservation for functions that
emit calls.

Primary target: AArch64 frame/prologue/epilogue and return lowering.

Actions:

- Derive non-leaf status from backend facts or machine nodes rather than from
  test names.
- Save `x30` in the function frame when a call can execute.
- Restore `x30` before returning, preserving stack alignment and existing
  frame layout invariants.
- Keep leaf function output stable unless the existing frame model requires a
  uniform frame.

Completion check: focused backend tests pass and generated non-leaf assembly
for the three probes no longer has the old `bl` plus bare `ret` failure shape.

### Step 4: Prove Simple Runtime Probes

Goal: Verify the repair removes the timeout owner from the cleanest c-testsuite
cases.

Primary target: `00100.c`, `00116.c`, and `00121.c` under the AArch64 backend
runtime route.

Actions:

- Run the supervisor-delegated narrow proof for the three probes with explicit
  timeout behavior.
- Confirm any remaining failure is not the old link-register clobber timeout.
- Record remaining semantic owners in `todo.md` instead of expanding this plan.

Completion check: the three simple probes no longer time out because of
non-leaf LR clobbering, or `todo.md` records a different blocking owner with
evidence.

### Step 5: Reclassify Timeout Boundary

Goal: Decide what timeout cases remain after the LR preservation owner is
removed.

Primary target: the 23-case timeout bucket from the umbrella inventory.

Actions:

- Rerun or inspect only the timeout bucket with explicit timeouts and
  stale-process cleanup.
- Separate cases fixed by LR preservation from cases blocked by printf,
  string literals, loops, aggregates, globals, or goto behavior.
- Create follow-on focused ideas only for semantic families that remain
  visible after LR preservation.

Completion check: `todo.md` records the updated timeout boundary and any
follow-on idea paths.
