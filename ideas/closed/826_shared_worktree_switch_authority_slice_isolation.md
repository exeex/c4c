# Shared-Worktree Switch Authority Slice Isolation

Status: Closed
Type: bounded dirty-worktree isolation prerequisite
Predecessor: `ideas/open/825_lir_next_body_parameter_authority_handoff.md`, Step 2
Blocked Return: `ideas/open/825_lir_next_body_parameter_authority_handoff.md`, Step 2

## Goal

Isolate the preserved, unaccepted Idea 822 materializing-add implementation
and companion test change from the native direct `LirSwitch.selector`
parameter-authority route, so Idea 825 can implement its selected row without
overwriting or blending dirty shared-worktree material.

## Why This Exists

Idea 822's dirty route advances the focused frontend failure to a DirectScalar
binary-LHS authority boundary but remains non-accepting: the matching focused
CTest is 0/1 before and 0/1 after. Idea 825 owns a different direct-selector
authority tuple, yet its Step 2 shares the same dirty `stmt.cpp` and test
surfaces. It cannot safely change those surfaces while the 822 material is
preserved in place.

## In Scope

- Inventory the exact unaccepted 822 materializing-add and companion test
  changes, including their owning source boundaries and proof state.
- Select and perform one reversible isolation operation only: move the dirty
  material to an explicitly preserved patch/worktree location, revert it from
  the shared worktree while preserving a recoverable patch, or reconstruct the
  clean shared route from an identified base.
- Prove only that the chosen isolation preserves recoverability and leaves the
  shared route free for Idea 825; run any required build/test evidence selected
  by the executor and supervisor.
- Record the exact preserved-material location, restoration action, and the
  return action to Idea 825 Step 2.

## Out of Scope

- Accepting, repairing, redesigning, deleting, or semantically changing the
  Idea 822 materializing-add/test route.
- Implementing Idea 825's dedicated `SwitchSelector` authority, or accepting
  its code or proof.
- Modifying Idea 821's manual-fixture material, DirectScalar binary-LHS
  authority semantics, Raw-BIR/importer code, verifier contracts, or test
  expectations.

## Acceptance Criteria

1. The dirty 822 material is recoverably preserved and no longer blocks a
   clean, independently owned 825 Step 2 worktree route.
2. The isolation operation has no semantic acceptance claim for 822 or 825,
   and records the exact recovery/restoration procedure.
3. Fresh isolation proof selected by the supervisor demonstrates the shared
   route was separated without weaker tests, erased material, or broad edits.
4. The durable handoff resumes Idea 825 exactly at Step 2, with its Step 1
   commit `7e6366cc9` and all 822 non-accepting proof evidence unchanged.

## Reviewer Reject Signals

- Reject a semantic code change, verifier relaxation, test expectation change,
  or testcase-specific bypass presented as isolation.
- Reject discarding, silently overwriting, or accepting the dirty 822 material
  without a recoverable preserved location and an explicit restoration action.
- Reject work that implements the direct `SwitchSelector` authority, repairs
  `scalar_lhs_parameter_authority`, or claims either Idea 822 or 825 complete.
- Reject touching Idea 821, Raw-BIR/importer routes, unrelated authority rows,
  or broad worktree cleanup under the isolation label.

## Closure Record

Disposition: capability complete for this bounded isolation-only prerequisite.

- The exact dirty 822 materializing-add/test slice is recoverably preserved at
  `.git/c4c-preservation/822-materializing-add-and-lowered-route.patch`.
  Restoration was verified with `git apply --check --binary`; restore only via
  `git apply --binary .git/c4c-preservation/822-materializing-add-and-lowered-route.patch`.
- Reverse application removed only the owned 822 hunks; the independent Idea
  821 hunk remains in the shared worktree. `git diff --check` passed.
- Fresh proof: `cmake --build --preset default` passed. The exact focused
  `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`
  remains 0/1 at `LirSwitch.selector: must identify a current-function integer
  value definition`. The matching regression guard with
  `--allow-non-decreasing-passed` passed because `test_before.log` and
  `test_after.log` both report 0 passed and 1 failed.
- No semantic authority is accepted for Ideas 821, 822, or 825. Idea 825
  resumes exactly at Step 2 with `7e6366cc9` as its only accepted progress.
