# Call Planning Frame Address Materialization Authority Runbook

Status: Active
Source Idea: ideas/open/110_call_planning_frame_address_materialization_authority.md

## Purpose

Replace remaining ordinary call-planning local frame-address name fallback with
explicit prepared frame-slot address materialization authority.

Goal: supported local aggregate address call arguments should select from
`PreparedAddressMaterializationKind::FrameSlot` facts, not stack-object name
recovery.

Core Rule: prove producer coverage before removing consumer compatibility. Do
not downgrade supported aggregate call paths or replace the fallback with
testcase-shaped matching.

## Read First

- `ideas/open/110_call_planning_frame_address_materialization_authority.md`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/stack_layout/analysis.cpp`
- `src/backend/prealloc/stack_layout/coordinator.cpp`
- `src/backend/prealloc/prepared_lookups.cpp`

## Current Scope

Implementation targets:

- `src/backend/prealloc/call_plans.cpp`
- producer-side prepared frame-address materialization code only if a real
  missing shared fact is proven
- focused backend/prealloc tests for local aggregate address call arguments

Validation targets:

- prepared/address-materialization dump or contract coverage for direct,
  same-block derived, and computed local frame-address call routes
- fail-closed missing or ambiguous materialization behavior

## Non-Goals

- Do not remove stack-layout `LegacyFrameAddressNameCompatibility` as a
  bootstrap producer.
- Do not move frame placement, frame-slot offsets, or target ABI placement into
  BIR.
- Do not rework unrelated call-plan selection, sret handling, publication
  planning, or architecture-specific x86/RISC-V code.
- Do not mark currently supported aggregate call paths unsupported.
- Do not claim progress by changing only expectations or helper names.

## Working Model

- `find_local_frame_address_source()` is still reachable from
  `select_prepared_call_argument_source()`.
- The preferred authority is
  `find_latest_frame_slot_materialization()` returning an explicit
  `PreparedAddressMaterializationKind::FrameSlot` fact.
- Ordinary direct, same-block derived, and immediate computed-address aggregate
  argument routes are expected to have explicit prepared materializations.
- The bounded unresolved risk is whether supported non-binary pointer-carrier
  bases can produce `PointerBasePlusOffset` homes without a preceding or
  same-instruction frame-slot materialization.

## Execution Rules

- Use AST-backed symbol checks before editing shared call-planning code.
- Keep changes semantic and fact-driven; no named testcase shortcuts.
- If producer coverage is missing, repair or record the missing prepared fact
  instead of silently preserving name-derived authority as ordinary behavior.
- Keep any retained fallback explicitly fail-closed, diagnostic, or
  compatibility-only.
- Run `git diff --check` and the supervisor-selected build/test subset before
  accepting any code-changing step.

## Ordered Steps

### Step 1: Prove Current Consumer And Producer Reachability

Goal: map every remaining `find_local_frame_address_source()` consumer branch
to the prepared materialization producer route it should use.

Primary targets:

- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/stack_layout/analysis.cpp`
- `src/backend/prealloc/stack_layout/coordinator.cpp`
- `src/backend/prealloc/prepared_lookups.cpp`

Actions:

- Use AST-backed caller/callee checks for
  `find_local_frame_address_source()`,
  `select_prepared_call_argument_source()`, and
  `find_latest_frame_slot_materialization()`.
- Record each fallback branch: direct `FrameSlotAddress`, register-source
  `LocalFrameAddressMaterialization`, and computed-address
  `LocalFrameAddressMaterialization`.
- Trace how direct pointer call arguments and same-block derived local frame
  addresses receive `PreparedAddressMaterializationKind::FrameSlot` facts.
- Identify the exact tests or dump fixtures that already exercise these routes,
  and note any missing contract coverage.

Completion check:

- `todo.md` records branch-by-branch reachability, producer evidence, and
  missing coverage without implementation edits.

### Step 2: Close The Computed Pointer-Carrier Caveat

Goal: decide whether supported non-binary pointer-carrier authority can reach
  call planning without explicit frame-slot materialization.

Primary targets:

- computed-address home construction and pointer-carrier publication helpers
- `PreparedStorageEncodingKind::ComputedAddress` handling in call planning
- producer code that emits frame-slot address materializations for pointer
  arithmetic, casts, selects, phis, loads, stores, or call arguments

Actions:

- Trace supported computed-address producers for `PointerBasePlusOffset` homes.
- Check whether non-binary pointer-carrier bases are supported, blocked, or
  already materialized before call planning consumes them.
- If a supported gap exists, define the missing prepared producer fact and the
  narrow place to add it.
- If no supported gap exists, record the fail-closed reason and the evidence
  that unsupported shapes cannot rely on name fallback.

Completion check:

- The computed-address caveat has a concrete disposition in `todo.md`: covered
  by existing materialization, repaired by a named producer change, or blocked
  as unsupported/fail-closed.

### Step 3: Replace Or Narrow Name-Derived Call-Planning Authority

Goal: stop using `find_local_frame_address_source()` as ordinary authority for
supported local aggregate call arguments.

Primary target:

- `src/backend/prealloc/call_plans.cpp`

Actions:

- Route direct, derived, and computed local aggregate address call arguments
  through explicit prepared frame-slot materialization lookup.
- Remove the fallback if all supported routes have explicit authority.
- If a fallback must remain, bound it to an unsupported, diagnostic, or
  compatibility path that cannot silently make supported lowering succeed from
  stack-object names alone.
- Preserve sret memory-return slot behavior and unrelated call argument
  selection.

Completion check:

- Supported ordinary call argument selection no longer depends on
  `find_local_frame_address_source()`, or any remaining use is explicitly
  bounded and documented by fail-closed behavior.

### Step 4: Add Contracts And Run Acceptance Proof

Goal: prove the cleanup covers the supported call shapes and does not weaken
contracts.

Actions:

- Add focused coverage for direct frame-slot local aggregate address call
  arguments.
- Add focused coverage for same-block derived local frame-address arguments.
- Add focused coverage for immediate computed-address local aggregate arguments
  or record why existing coverage is sufficient.
- Add or update fail-closed coverage for absent or ambiguous frame-slot
  materialization.
- Run `git diff --check`, a fresh build or compile proof, and the
  supervisor-selected backend/prealloc subset.

Completion check:

- The tests prove explicit prepared frame-slot authority for supported routes,
  fail-closed missing authority, and no expectation downgrade.

