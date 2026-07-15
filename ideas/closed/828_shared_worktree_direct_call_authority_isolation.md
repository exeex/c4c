# Shared-Worktree Direct-Call Authority Isolation

Status: Closed — capability complete for isolation only
Type: bounded dirty-worktree isolation prerequisite
Predecessor: `ideas/open/827_lir_next_body_parameter_authority_handoff.md`, Step 2
Blocked Return: `ideas/open/827_lir_next_body_parameter_authority_handoff.md`, Step 2

## Goal

Recoverably isolate the preserved, unaccepted Idea 821/822 changes from the
shared frontend test and binary-lowering surfaces so 827 can execute its fixed
direct-call argument-0 authority route without overwriting, co-committing, or
absorbing them.

## Why This Exists

827's selected row requires nearby coverage in
`tests/frontend/frontend_lir_function_signature_type_ref_test.cpp`, but that
file is already dirty with unaccepted Idea 821/822 material. The companion
`src/codegen/lir/hir_to_lir/expr/binary.cpp` change is also preserved and
unaccepted. The clean pre-change baseline was established through a
conflict-free stash/pop and is recorded in `test_before.log`, but that does not
give 827 ownership of the restored dirty surfaces.

## In Scope

- Inventory the exact dirty `binary.cpp` and frontend-test hunks, their Idea
  821/822 ownership boundaries, and their non-accepting proof state.
- Perform one reversible isolation operation that preserves those hunks at a
  named recoverable location while removing them from the shared route needed
  by 827.
- Prove recoverability and the resulting clean-route state using the
  supervisor-selected build/focused-test evidence without claiming semantic
  acceptance for 821, 822, or 827.
- Record the preservation location, restoration command, and exact return to
  827 Step 2.

## Out Of Scope

- Implementing, repairing, accepting, deleting, or semantically changing any
  Idea 821/822 material.
- Publishing 827's `FixedDirectCallArgument0` authority or modifying its
  producer/schema/verifier contracts.
- Raw-BIR/importer/receiver work, test expectation weakening, broad worktree
  cleanup, or changes outside the two identified dirty implementation/test
  surfaces.

## Acceptance Criteria

1. Every preserved dirty hunk is recoverable from a named location with an
   explicit restoration command, and the shared route no longer carries it.
2. The selected preservation operation makes no semantic acceptance claim for
   Ideas 821, 822, or 827.
3. Fresh isolation proof establishes that the clean route is ready for 827's
   independently owned Step 2; the pre-change `test_before.log` remains the
   stated clean baseline and is not replaced by this lifecycle operation.
4. The closure/return record resumes 827 exactly at its selected direct-call
   argument-0 Step 2 contract and proof command.

## Reviewer Reject Signals

- Reject any semantic code change, verifier relaxation, expectation downgrade,
  or testcase-shaped bypass presented as isolation.
- Reject discarding, overwriting, co-committing, or accepting the preserved
  Idea 821/822 material without a recoverable named location and restoration
  action.
- Reject implementation of 827's direct-call authority, an unrelated
  parameter row, Raw-BIR/importer work, or broad cleanup under this idea.
- Reject claiming that a green build/test proves any of the isolated semantic
  routes accepted.

## Closure Record

Status: capability complete — isolation only; closed after supervisor
acceptance.

- Preserved artifact: `review/828_preserved_821_822_frontend_slice.patch`
  committed in `7b0f8671e`.
- Restoration command: `git apply
  review/828_preserved_821_822_frontend_slice.patch` (checked clean after the
  removal).
- Isolation proof: fresh `cmake --build --preset default` and `ctest
  --test-dir build --output-on-failure -R '^frontend_lir_call_type_ref$'`
  passed 1/1; matching `test_before.log` and `test_after.log` record the
  focused regression guard.
- Result: `src/codegen/lir/hir_to_lir/expr/binary.cpp` and
  `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp` are
  clean relative to the preserved slice. This does not accept, repair, or
  alter Ideas 821/822, and it does not publish 827 authority.
- Return: reactivate 827 at Step 2, retaining its exact selected
  `FixedDirectCallArgument0` contract and stipulated fresh build plus focused
  frontend proof.
