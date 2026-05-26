# RISC-V Prepared Edge Publication Stack Destination Scratch Policy Runbook

Status: Active
Source Idea: ideas/open/32_riscv_prepared_edge_publication_stack_destination_scratch_policy.md

## Purpose

Define and use a target-local RISC-V scratch-register or scratch-reservation
contract before broadening prepared edge-publication moves into `StackSlot`
destinations from non-register sources.

Goal: make at least one non-register source-to-`StackSlot` publication form
safe through shared `edge_publications`, or record a concrete policy reason
that every candidate must remain fail-closed.

Core Rule: shared prepared `edge_publications` remains the only semantic
authority for supported edge moves; RISC-V owns only target-local scratch,
materialization, address, width, offset-range, load, and store policy.

## Read First

- `ideas/open/32_riscv_prepared_edge_publication_stack_destination_scratch_policy.md`
- RISC-V prepared edge-publication consumer code
- Existing tests for prepared edge-publication stack-destination moves
- Shared prepared edge-publication lookup tests
- Recent closure context from RISC-V ideas 29 and 30 when available in history

## Current Targets

- Prepared edge-publication moves with `StackSlot` destinations.
- Candidate non-register sources:
  - `RematerializableImmediate -> StackSlot`
  - `StackSlot -> StackSlot`
  - `PointerBasePlusOffset -> StackSlot`
- Existing `Register -> StackSlot` support must remain intact.

## Non-Goals

- Do not rediscover edge-copy facts from predecessor or successor block shape.
- Do not create a RISC-V-local edge-copy fact table.
- Do not move scratch, load, store, address, or materialization policy into
  shared prepare, BIR, or target-neutral helpers.
- Do not broaden stack-source register-destination policy.
- Do not broaden pointer-base register-destination policy.
- Do not perform broad frame-layout, allocator, dynamic-stack, or memory-layout
  rewrites outside the scratch contract needed here.
- Do not weaken existing supported contracts or test expectations to claim
  progress.

## Working Model

- Shared prepare publishes the edge move facts.
- The RISC-V prepared edge-publication consumer looks up those facts and lowers
  only the homes it has explicit target-local policy for.
- `StackSlot` destinations from register sources are already supported.
- Non-register sources need a scratch policy before they can safely store into a
  `StackSlot` destination.
- Unsupported source homes, destination homes, widths, offsets, or address
  forms must remain explicit and fail closed.

## Execution Rules

- Keep each code slice semantic: no fixture-label, value-id, stack-slot-id,
  offset, or test-name matching.
- Prefer one candidate form first; do not try to support every non-register
  source before the scratch contract is reviewed by tests.
- Document the scratch ownership, reservation, or clobber contract at the
  consumer path before using it.
- For every newly supported form, add both positive coverage and negative
  coverage proving that shared publication facts are required.
- For code-changing steps, use the validation ladder selected by the
  supervisor: build proof, focused RISC-V prepared edge-publication tests,
  relevant shared prepared lookup tests, then an appropriate backend bucket
  when the blast radius warrants it.

## Ordered Steps

### Step 1: Inspect Current Consumer And Scratch Constraints

Goal: identify the current RISC-V prepared edge-publication lowering path,
existing `Register -> StackSlot` behavior, available temporary-register
conventions, and the exact fail-closed points for candidate non-register
sources.

Primary Target: RISC-V prepared edge-publication consumer and focused tests.

Concrete Actions:

- Inspect the consumer logic that handles prepared edge-publication moves into
  `StackSlot` destinations.
- Trace how existing `Register -> StackSlot` stores select width, address, and
  offset policy.
- Identify nearby RISC-V backend conventions for scratch registers,
  reservations, clobbers, temporary materialization, and large-offset address
  handling.
- Audit the current fail-closed behavior for
  `RematerializableImmediate -> StackSlot`, `StackSlot -> StackSlot`, and
  `PointerBasePlusOffset -> StackSlot`.
- Record the safest first candidate for implementation in `todo.md` if the
  audit finds one.

Completion Check:

- The next executor packet can name the consumer files, current fail-closed
  branch, scratch-contract candidate, and first source-to-stack form to attempt
  without re-auditing the entire route.

### Step 2: Define The RISC-V Scratch Contract

Goal: make the scratch-register or scratch-reservation policy explicit before
  any non-register source-to-stack form uses it.

Primary Target: RISC-V prepared edge-publication consumer path.

Concrete Actions:

- Choose a target-local scratch contract consistent with existing RISC-V
  backend conventions.
- Document the ownership, reservation, clobber, and lifetime assumptions near
  the consumer path.
- Keep the contract local to RISC-V edge-publication lowering.
- Add or adjust focused negative coverage if the chosen policy should reject a
  form lacking a safe scratch path.

Completion Check:

- The consumer path has an explicit scratch contract, and no supported move
  relies on an undocumented hard-coded scratch register.

### Step 3: Implement One Non-Register Source-To-Stack Form

Goal: support the safest selected candidate through shared
`edge_publications` lookup authority using the scratch policy from Step 2.

Primary Target: the selected first candidate among
`RematerializableImmediate -> StackSlot`, `StackSlot -> StackSlot`, or
`PointerBasePlusOffset -> StackSlot`.

Concrete Actions:

- Lower the selected candidate only after a matching shared publication fact is
  consumed.
- Apply explicit target-local policy for materialization, load/store width,
  offset range, address materialization, and scratch use.
- Preserve existing `Register -> StackSlot` support.
- Leave non-selected forms and unsupported widths or address forms fail-closed.
- Avoid broad helper rewrites unless they directly simplify the selected
  semantic lowering.

Completion Check:

- The selected non-register source-to-`StackSlot` form emits correctly through
  shared publication lookup, existing register-source stores still work, and
  unsupported forms remain explicit failures.

### Step 4: Add Focused Positive And Negative Coverage

Goal: prove the new support is semantic and still depends on prepared
edge-publication facts.

Primary Target: focused RISC-V prepared edge-publication tests plus relevant
shared prepared lookup tests.

Concrete Actions:

- Add a positive test for the newly supported source-to-stack form.
- Add a negative test or expectation that fails if shared publication facts are
  absent, ignored, or locally rediscovered.
- Keep existing `Register -> StackSlot` coverage green.
- Keep unsupported forms explicit in tests rather than silently broadening them.

Completion Check:

- Tests distinguish the newly supported form from unsupported nearby forms, and
  they would fail if the implementation bypassed shared `edge_publications`.

### Step 5: Validate And Decide Remaining Policy

Goal: close the activated runbook slice with fresh proof and a clear decision
on remaining source-to-stack forms.

Primary Target: validation logs and `todo.md` execution summary.

Concrete Actions:

- Run the supervisor-selected build and focused test subset.
- Include relevant shared prepared lookup tests and an appropriate backend
  bucket if the implementation touched shared-adjacent behavior or broad
  target-local helpers.
- Record remaining unsupported candidate forms and the reason each remains
  fail-closed.
- Escalate to a follow-up idea only if a remaining candidate needs a separate
  initiative outside this scratch-policy slice.

Completion Check:

- Fresh validation is recorded, the scratch policy and first supported form are
  proven, and the supervisor has enough state to decide whether the source idea
  can close or needs a follow-up plan.
