# AArch64 Variadic Shard Implementation Redistribution Runbook

Status: Active
Source Idea: ideas/open/271_aarch64_variadic_markdown_shard_implementation_redistribution.md

## Purpose

Activate the AArch64 `variadic.md` shard cleanup as executable work. The route
must reconcile the markdown-only variadic ABI surface into compiled ownership
files while preserving accepted variadic call-boundary facts and explicit
deferred function-entry behavior.

## Goal

Create `src/backend/mir/aarch64/codegen/variadic.cpp` and
`src/backend/mir/aarch64/codegen/variadic.hpp` as the current variadic
ownership boundary, wire that boundary into the build if needed, then delete
`src/backend/mir/aarch64/codegen/variadic.md`.

## Core Rule

This is an ownership redistribution, not a new variadic ABI implementation.
Do not implement full AAPCS64 `va_start`, `va_arg`, `va_copy`, `va_list`,
register-save-area, or stack-overflow-area behavior unless structured prepared
facts already exist and the work is behavior-preserving movement.

## Read First

- `ideas/open/271_aarch64_variadic_markdown_shard_implementation_redistribution.md`
- `src/backend/mir/aarch64/codegen/README.md`
- `src/backend/mir/aarch64/codegen/variadic.md`
- `src/backend/mir/aarch64/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`
- `src/backend/mir/aarch64/ALLOCATION_CONTRACT.md`
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- `src/backend/CMakeLists.txt`
- Nearby owners such as `calls`, `dispatch`, `instruction`, `prologue`,
  `memory`, and `module_compile` only as needed to find current variadic
  responsibility.

## Current Targets

- `src/backend/mir/aarch64/codegen/variadic.cpp`
- `src/backend/mir/aarch64/codegen/variadic.hpp`
- `src/backend/mir/aarch64/codegen/variadic.md`
- `src/backend/CMakeLists.txt`, if the new translation unit must be compiled
- Existing AArch64 call, dispatch, instruction, prologue, memory, or module
  owners only where a behavior-preserving ownership hook is required.

## Non-Goals

- Do not implement full AAPCS64 `va_start`, `va_arg`, `va_copy`, `va_list`, or
  register-save-area support for this ownership-cleanup idea.
- Do not invent missing prepared variadic carriers.
- Do not change call ABI, prologue/frame setup, memory, aggregate copy,
  stack-overflow-area, or helper-call semantics.
- Do not weaken tests, unsupported contracts, or diagnostics to claim
  variadic function-entry support.
- Do not redistribute other markdown shards.

## Working Model

The accepted minimum variadic call contract preserves
`PreparedCallPlan::wrapper_kind == DirectExternVariadic` and
`PreparedCallPlan::variadic_fpr_arg_register_count` as call-boundary facts.
Current compiled code may also expose structured deferred paths for variadic
entry helpers, but historical `variadic.md` is not implementation authority.
The compiled variadic owner should expose current supported facts and explicit
deferred handling without reconstructing legacy `va_list` behavior from the
markdown shard.

## Execution Rules

- Keep variadic ownership decisions inside `variadic.{hpp,cpp}` where
  possible.
- Keep `calls`, `dispatch`, `instruction`, `prologue`, `memory`, and
  `module_compile` from becoming hidden variadic owners.
- Prefer narrow fact-preserving APIs or explicit deferred helpers over
  re-deriving ABI policy from legacy markdown.
- Preserve existing ABI-visible behavior, diagnostics, and emitted output.
- Delete `variadic.md` only after the compiled owner records the relevant
  current boundary or explicit deferred handling.
- Run fresh build or focused compile proof for each code-changing packet.
- Use focused AArch64 backend proof before accepting deletion; escalate to
  broader proof if any integration point touches call ABI, prologue, memory,
  aggregate, helper-call, or assembly-output behavior.

## Step 1: Establish The Current Variadic Boundary

Goal: determine whether the current AArch64 route has live variadic behavior
to move, explicit deferred helper behavior to record, or no compiled owner
boundary yet.

Primary target: `src/backend/mir/aarch64/codegen/variadic.md`

Actions:

- Inspect current variadic call-boundary, direct extern wrapper, FPR argument
  count, variadic entry helper, `va_start`, `va_arg`, `va_copy`, dispatch,
  instruction-record, and diagnostic ownership.
- Search for compiled variadic behavior outside a dedicated variadic owner,
  especially in `calls`, `dispatch`, `instruction`, `prologue`, `memory`, and
  `module_compile`.
- Compare discovered behavior against
  `AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`, `ALLOCATION_CONTRACT.md`, and
  `BIR_PREPARED_GAP_LEDGER.md`.
- Decide whether `variadic.{hpp,cpp}` should expose moved behavior, a deferred
  marker, narrow record builders, or a no-op ownership boundary.
- Do not change emitted code, diagnostics, ABI behavior, or tests in this
  step.

Completion check:

- The next implementation step has a concrete API shape for
  `variadic.{hpp,cpp}` and a clear answer for whether integration is
  behavior-preserving movement, explicit deferral, or no-op.

## Step 2: Add The Compiled Variadic Owner

Goal: create the compiled variadic owner and make the build recognize it.

Primary targets:

- `src/backend/mir/aarch64/codegen/variadic.hpp`
- `src/backend/mir/aarch64/codegen/variadic.cpp`
- `src/backend/CMakeLists.txt`

Actions:

- Add `variadic.hpp` and `variadic.cpp` using existing namespace and AArch64
  codegen owner conventions.
- Preserve prepared variadic fact authority in the API; do not accept raw
  legacy `va_list` or register-save layout inputs as current policy.
- If no live behavior can move yet, make the deferred/no-op boundary explicit
  and narrow.
- Add the new translation unit to `src/backend/CMakeLists.txt` if required.
- Avoid broad dependencies on calls, dispatch, instruction, prologue, memory,
  or module compile unless Step 1 proved a narrow integration point is needed.

Completion check:

- The backend builds or the relevant backend target compiles with the new
  files.
- No ABI-visible variadic call, function-entry, helper, memory, aggregate, or
  emitted-output behavior is intentionally changed.

## Step 3: Route Or Record Integration

Goal: ensure the compiled variadic boundary is discoverable without hiding
variadic behavior in broader owners.

Primary targets:

- `src/backend/mir/aarch64/codegen/variadic.*`
- `src/backend/mir/aarch64/codegen/calls.*`
- `src/backend/mir/aarch64/codegen/dispatch.*`
- `src/backend/mir/aarch64/codegen/instruction.*`
- `src/backend/mir/aarch64/codegen/prologue.*`
- `src/backend/mir/aarch64/codegen/memory.*`
- `src/backend/mir/aarch64/codegen/module_compile.*`

Actions:

- If current variadic call-boundary or structured helper behavior already
  exists in broader owners, route it through `variadic.{hpp,cpp}` at the
  narrowest behavior-preserving boundary.
- If current behavior is deferred pending prepared carriers, record that
  deferral in the variadic owner and leave broader owners untouched except for
  required includes or ownership hooks.
- Verify calls remain focused on call sequencing rather than owning variadic
  ABI policy.
- Verify dispatch and instruction records preserve prepared variadic facts
  without reconstructing legacy `va_list` behavior.
- Verify prologue and memory owners remain focused on frame and memory
  operations rather than variadic save-area policy.

Completion check:

- The variadic ownership boundary is reviewable from compiled code.
- Any integration point is behavior-preserving and narrower than the broad
  backend driver.

## Step 4: Delete The Markdown Shard

Goal: complete the redistribution by removing the markdown-only shard.

Primary target: `src/backend/mir/aarch64/codegen/variadic.md`

Actions:

- Delete `variadic.md` after the compiled owner preserves the needed boundary
  or explicit deferred handling.
- Confirm no live documentation or build reference still points at the deleted
  shard as an active artifact.
- Do not delete unrelated markdown shards.

Completion check:

- `variadic.md` is gone.
- `variadic.cpp` and `variadic.hpp` are the discoverable owner boundary.

## Step 5: Focused Proof And Acceptance Check

Goal: prove the redistribution did not alter behavior or drift into testcase
overfit.

Actions:

- Run a fresh build or compile command covering the backend after code
  changes.
- Run focused AArch64 backend proof chosen by the supervisor.
- If any integration touches call ABI, prologue, memory, aggregate,
  helper-call, or assembly-output behavior, run the matching before/after or
  broader regression subset before accepting the slice.
- Check the diff for reviewer reject signals from the source idea.

Completion check:

- Build or compile proof is green.
- Focused backend proof is green or any failure is documented as pre-existing
  by matching before/after logs.
- No ABI behavior, emitted-output contract, tests, unsupported expectation, or
  diagnostics were weakened.
