# AArch64 Prologue Shard Implementation Redistribution Runbook

Status: Active
Source Idea: ideas/open/270_aarch64_prologue_markdown_shard_implementation_redistribution.md

## Purpose

Activate the AArch64 `prologue.md` shard cleanup as executable work. The route
must reconcile the markdown-only prologue/frame-entry surface into compiled
ownership files while preserving the current prepared frame, call, return, and
deferred-carrier contracts.

## Goal

Create `src/backend/mir/aarch64/codegen/prologue.cpp` and
`src/backend/mir/aarch64/codegen/prologue.hpp` as the current prologue/frame
entry ownership boundary, wire that boundary into the build if needed, then
delete `src/backend/mir/aarch64/codegen/prologue.md`.

## Core Rule

This is an ownership redistribution, not a frame-layout or ABI behavior change.
Do not rebuild historical prologue, epilogue, parameter-home, variadic
save-area, or callee-save machinery unless the current prepared facts already
authorize the behavior being moved.

## Read First

- `ideas/open/270_aarch64_prologue_markdown_shard_implementation_redistribution.md`
- `src/backend/mir/aarch64/codegen/README.md`
- `src/backend/mir/aarch64/codegen/prologue.md`
- `src/backend/mir/aarch64/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`
- `src/backend/mir/aarch64/ALLOCATION_CONTRACT.md`
- `src/backend/CMakeLists.txt`
- Nearby owners such as `calls`, `returns`, `memory`, `module_compile`, and
  existing frame/module records only as needed to find current frame-entry
  responsibility.

## Current Targets

- `src/backend/mir/aarch64/codegen/prologue.cpp`
- `src/backend/mir/aarch64/codegen/prologue.hpp`
- `src/backend/mir/aarch64/codegen/prologue.md`
- `src/backend/CMakeLists.txt`, if the new translation unit must be compiled
- Existing AArch64 frame, call, return, memory, or module owners only where a
  behavior-preserving ownership hook is required.

## Non-Goals

- Do not rebuild full frame layout, register allocation, callee-save
  instruction selection, parameter storage, or variadic function-entry support.
- Do not invent missing prepared frame, outgoing-area, return-boundary,
  parameter-home, or variadic save-area carriers.
- Do not change stack layout, ABI register policy, emitted prologue/epilogue
  sequences, callee-save behavior, or parameter/return semantics.
- Do not move unrelated call, return, memory, variadic, or aggregate behavior
  into the prologue owner.
- Do not redistribute `variadic.md` or other markdown shards.

## Working Model

Current AArch64 frame and call behavior is defined by prepared BIR facts and
target-MIR records, especially `module::FrameRecord`, `module::FrameSlotRecord`,
`module::CalleeSaveRecord`, allocation-result call-preservation facts, and
prepared call/return movement records. `prologue.md` is historical reference
material. The compiled prologue owner should expose the live prologue/frame
entry boundary or an explicit deferred boundary where structured carriers do
not yet exist.

## Execution Rules

- Keep prologue/frame-entry ownership decisions inside `prologue.{hpp,cpp}`
  where possible.
- Keep `calls`, `returns`, `memory`, and `module_compile` from becoming hidden
  prologue owners.
- Prefer explicit deferred helpers or small fact-preserving APIs over
  re-deriving ABI policy from legacy markdown.
- Preserve existing ABI-visible behavior and emitted output.
- Delete `prologue.md` only after the compiled owner records the relevant
  current boundary.
- Run fresh build or focused compile proof for each code-changing packet.
- Use focused AArch64 backend proof before accepting deletion; escalate to
  broader proof if any integration point touches frame, call, return, or
  assembly output behavior.

## Step 1: Establish The Current Prologue Boundary

Goal: determine whether the current AArch64 route has live prologue/frame-entry
behavior to move, explicit deferred frame behavior to record, or no compiled
boundary yet.

Primary target: `src/backend/mir/aarch64/codegen/prologue.md`

Actions:

- Inspect current frame-entry, callee-save, dynamic-stack, parameter-home,
  call, return, and module compile ownership.
- Search for any existing compiled prologue, epilogue, frame-entry, or
  parameter-home helper behavior outside a dedicated prologue owner.
- Compare discovered behavior against
  `AAPCS64_CALL_RETURN_FRAME_CONTRACT.md` and `ALLOCATION_CONTRACT.md`.
- Decide whether `prologue.{hpp,cpp}` should expose moved behavior, a deferred
  marker, a narrow frame-record API, or a no-op ownership boundary.
- Do not change emitted code or ABI behavior in this step.

Completion check:

- The next implementation step has a concrete API shape for
  `prologue.{hpp,cpp}` and a clear answer for whether integration is
  behavior-preserving movement, deferred, or no-op.

## Step 2: Add The Compiled Prologue Owner

Goal: create the compiled prologue owner and make the build recognize it.

Primary targets:

- `src/backend/mir/aarch64/codegen/prologue.hpp`
- `src/backend/mir/aarch64/codegen/prologue.cpp`
- `src/backend/CMakeLists.txt`

Actions:

- Add `prologue.hpp` and `prologue.cpp` using existing namespace and AArch64
  codegen owner conventions.
- Preserve prepared-frame authority in the API; do not accept raw legacy
  frame-layout inputs as if they were current policy.
- If no live behavior can move yet, make the deferred/no-op boundary explicit
  and narrow.
- Add the new translation unit to `src/backend/CMakeLists.txt` if required.
- Avoid broad dependencies on calls, returns, memory, or module compile unless
  Step 1 proved a narrow integration point is needed.

Completion check:

- The backend builds or the relevant backend target compiles with the new
  files.
- No ABI-visible frame, call, return, parameter, or emitted-output behavior is
  intentionally changed.

## Step 3: Route Or Record Integration

Goal: ensure the compiled prologue boundary is discoverable without hiding
frame-entry behavior in broader owners.

Primary targets:

- `src/backend/mir/aarch64/codegen/prologue.*`
- `src/backend/mir/aarch64/codegen/calls.*`
- `src/backend/mir/aarch64/codegen/returns.*`
- `src/backend/mir/aarch64/codegen/memory.*`
- `src/backend/mir/aarch64/codegen/module_compile.*`

Actions:

- If current prologue/frame-entry behavior already exists in broader owners,
  route it through `prologue.{hpp,cpp}` at the narrowest behavior-preserving
  boundary.
- If current behavior is deferred pending prepared carriers, record that
  deferral in the prologue owner and leave broader owners untouched.
- Verify calls and returns remain focused on call/return boundaries rather
  than frame-entry policy.
- Verify memory lowering remains focused on memory operations rather than
  frame setup or teardown.

Completion check:

- The ownership boundary is reviewable from compiled code.
- Any integration point is behavior-preserving and narrower than the broad
  backend driver.

## Step 4: Delete The Markdown Shard

Goal: complete the redistribution by removing the markdown-only shard.

Primary target: `src/backend/mir/aarch64/codegen/prologue.md`

Actions:

- Delete `prologue.md` after the compiled owner preserves the needed boundary
  or explicit deferred handling.
- Confirm no live documentation or build reference still points at the deleted
  shard as an active artifact.
- Do not delete unrelated markdown shards.

Completion check:

- `prologue.md` is gone.
- `prologue.cpp` and `prologue.hpp` are the discoverable owner boundary.

## Step 5: Focused Proof And Acceptance Check

Goal: prove the redistribution did not alter behavior or drift into testcase
overfit.

Actions:

- Run a fresh build or compile command covering the backend after code changes.
- Run focused AArch64 backend proof chosen by the supervisor.
- If any integration touches frame, return, call, or assembly-output behavior,
  run the matching before/after or broader regression subset before accepting
  the slice.
- Check the diff for reviewer reject signals from the source idea.

Completion check:

- Build or compile proof is green.
- Focused backend proof is green or any failure is documented as pre-existing
  by matching before/after logs.
- No ABI behavior, emitted-output contract, tests, or expectations were
  weakened.
