# AArch64 i128 Ops Shard Redistribution Runbook

Status: Active
Source Idea: ideas/open/262_aarch64_i128_ops_markdown_shard_implementation_redistribution.md

## Purpose

Move the valid AArch64 i128-ops shard behavior out of broad codegen owners and
into ordinary compiled owners named `i128_ops.cpp` and `i128_ops.hpp`.

Goal: reconcile `src/backend/mir/aarch64/codegen/i128_ops.md` into compiled
source/header ownership while preserving existing i128 pair/helper behavior.

Core Rule: this is ownership redistribution, not an i128 semantic expansion,
test expectation rewrite, fixed-register shortcut, or instruction hierarchy
redesign.

## Read First

- `ideas/open/262_aarch64_i128_ops_markdown_shard_implementation_redistribution.md`
- `src/backend/mir/aarch64/codegen/i128_ops.md`
- Existing i128-related code in:
  - `src/backend/mir/aarch64/codegen/instruction.cpp`
  - `src/backend/mir/aarch64/codegen/dispatch.cpp`
  - `src/backend/mir/aarch64/codegen/machine_printer.cpp`
  - any existing shared/prepared i128 authority already used by those owners

## Current Targets

- Add or populate `src/backend/mir/aarch64/codegen/i128_ops.cpp`.
- Add or populate `src/backend/mir/aarch64/codegen/i128_ops.hpp`.
- Remove reconciled durable content from
  `src/backend/mir/aarch64/codegen/i128_ops.md` by deleting that shard before
  close.
- Keep broad owners as neutral core, routing, or caller surfaces only.

## Non-Goals

- Do not redistribute other AArch64 markdown shards.
- Do not redistribute `mod.md` or `records.md`.
- Do not perform broad AArch64 codegen cleanup unrelated to i128 ops.
- Do not redesign `instruction.hpp` or the instruction type hierarchy just to
  split this shard.
- Do not add fixed-register shortcuts.
- Do not rewrite tests or expectations to claim movement is complete.
- Do not expand i128 semantics beyond behavior-preserving relocation.

## Working Model

- `i128_ops.hpp` should expose the narrow i128-ops owner API needed by existing
  AArch64 codegen callers.
- `i128_ops.cpp` should own i128-specific selected-node construction, lowering,
  helper/pair behavior, and spelling helpers that are currently stranded in
  family-neutral owners.
- `instruction.cpp` should keep only family-neutral instruction core.
- `dispatch.cpp` should route into the i128-ops owner instead of containing
  i128-family bodies.
- `machine_printer.cpp` should delegate i128-specific spelling when that
  spelling can live behind i128-ops shard helpers.

## Execution Rules

- Keep each step behavior-preserving unless this plan explicitly says
  otherwise.
- Consume existing prepared/shared i128 authority instead of inventing a new
  authority model.
- Prefer narrow moves with compile proof after each code-changing step.
- Reject changes whose main proof is a renamed helper, expectation update, or
  classification-only diff while i128 bodies remain centralized elsewhere.
- Preserve existing tests and supported-path expectations.
- If a step discovers a separate initiative, record it under `ideas/open/`
  through lifecycle routing instead of expanding this runbook.

## Steps

### Step 1: Inventory the i128 shard and current compiled owners

Goal: establish the exact i128-specific behavior that must move.

Primary target:
- `src/backend/mir/aarch64/codegen/i128_ops.md`

Actions:
- Inspect the markdown shard and identify valid implementation content that
  still lacks compiled ownership.
- Locate matching i128 construction, lowering, helper, pair, and spelling code
  currently living in broad owners.
- Identify the existing prepared/shared i128 authority that relocation must
  consume.
- Record any source surfaces that are neutral routing or core and should remain
  outside the i128-ops owner.

Completion check:
- The next executor packet can name the exact functions/helpers to move into
  `i128_ops.cpp`/`i128_ops.hpp` without guessing.
- No implementation behavior has changed.

### Step 2: Create the compiled i128-ops owner boundary

Goal: introduce the ordinary source/header home for i128-ops behavior.

Primary targets:
- `src/backend/mir/aarch64/codegen/i128_ops.cpp`
- `src/backend/mir/aarch64/codegen/i128_ops.hpp`

Actions:
- Add the narrow public declarations needed by existing callers.
- Add the corresponding compiled implementation file.
- Include existing `instruction.hpp` from the shard owner if needed.
- Wire the new source into the existing build in the smallest repo-native way.

Completion check:
- The project builds with the new files present and no behavior moved yet, or
  with only trivial behavior-preserving scaffolding.

### Step 3: Move i128 construction and lowering bodies

Goal: relocate i128-specific selected-node construction, helper/pair handling,
and lowering bodies into the i128-ops owner.

Primary targets:
- `src/backend/mir/aarch64/codegen/i128_ops.cpp`
- `src/backend/mir/aarch64/codegen/i128_ops.hpp`
- current broad owner files containing those bodies

Actions:
- Move i128-specific construction and lowering code out of broad owners.
- Leave broad owners with neutral core logic or calls into the new i128-ops
  API.
- Preserve existing helper names where they are useful for behavior and review.
- Avoid new fixed-register shortcuts or testcase-shaped handling.

Completion check:
- Fresh compile proof passes.
- Focused backend proof for existing i128 pair/helper behavior still passes.
- Review of broad owners shows the moved bodies now live behind
  `i128_ops.cpp`/`i128_ops.hpp`.

### Step 4: Move i128 spelling and printing helpers

Goal: keep i128-specific spelling out of the family-neutral printer when it can
be owned by the i128-ops shard.

Primary targets:
- `src/backend/mir/aarch64/codegen/i128_ops.cpp`
- `src/backend/mir/aarch64/codegen/i128_ops.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

Actions:
- Move i128-specific spelling helpers into the i128-ops owner.
- Keep `machine_printer.cpp` as a neutral caller/delegator for i128 spelling.
- Preserve output spelling exactly.

Completion check:
- Fresh compile proof passes.
- Focused printer or backend proof shows i128 assembly spelling is unchanged.

### Step 5: Delete the reconciled markdown shard

Goal: remove `i128_ops.md` once its durable valid content has compiled owners.

Primary target:
- `src/backend/mir/aarch64/codegen/i128_ops.md`

Actions:
- Confirm every valid shard item is represented in compiled code or explicitly
  obsolete under the source idea scope.
- Delete `i128_ops.md`.
- Do not delete or alter unrelated markdown shards.

Completion check:
- The markdown shard is gone.
- `i128_ops.cpp` and `i128_ops.hpp` own the valid i128-ops behavior.
- Build proof still passes.

### Step 6: Final focused and broader validation

Goal: prove the redistribution preserved behavior and did not drift into
unrelated codegen work.

Actions:
- Run the focused backend proof chosen by the supervisor.
- Escalate to broader validation if the moved code affects shared instruction,
  dispatch, or printer paths beyond the narrow i128 bucket.
- Inspect the final diff for reject signals from the source idea.

Completion check:
- Focused proof is green.
- Any supervisor-required broader proof is green.
- The diff does not downgrade tests, add named-case shortcuts, reintroduce
  fixed-register shortcuts, or leave i128-family bulk implementation in broad
  owners.
