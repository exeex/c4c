# Shared-Worktree Direct-Call Authority Isolation Runbook

Status: Active
Source Idea: ideas/open/828_shared_worktree_direct_call_authority_isolation.md
Switched from: ideas/open/827_lir_next_body_parameter_authority_handoff.md at Step 2

## Purpose

Separate the preserved unaccepted Idea 821/822 dirty slice from the shared
worktree so the already selected 827 direct-call authority route has an
independent implementation and proof surface.

## Core Rule

Preserve every dirty implementation/test hunk recoverably before removing it
from the shared route. Isolation is not semantic acceptance.

## Non-Goals

- Any producer/schema/verifier implementation for 827.
- Acceptance, repair, redesign, or deletion of Ideas 821/822.
- Raw-BIR/importer work, expectation changes, or generic cleanup.

## Ordered Steps

### Step 1 - Inventory and preserve the dirty slice

Goal: create one named, recoverable preservation artifact for every dirty
`binary.cpp` and frontend-test hunk owned by the pending 821/822 route.

Actions:

- record the exact source-hunk ownership and current non-accepting proof state;
- create a reversible preservation artifact and verify its restoration command;
- remove only the preserved dirty code/test slice from the shared route.

Completion check: the artifact applies cleanly, the two code/test surfaces are
clean relative to their pre-slice base, and no semantic behavior is accepted.

### Step 2 - Prove the isolated shared route

Goal: establish that the now-clean route can be independently owned by 827.

Actions:

- run the supervisor-selected fresh build and focused frontend proof;
- keep the baseline/after evidence distinct from semantic acceptance; and
- retain the named restoration procedure.

Completion check: evidence and worktree ownership demonstrate a clean 827
route without altering or accepting the preserved slice.

### Step 3 - Return to the direct-call authority handoff

Goal: record a durable return to 827 without changing its selected contract.

Actions:

- record preservation location, restoration command, and proof references;
- close/conclude this isolation prerequisite only after supervisor acceptance;
- reactivate 827 exactly at Step 2.

Completion check: the successor can reconstruct the selected
`FixedDirectCallArgument0` contract and run its stipulated fresh build plus
`^frontend_lir_call_type_ref$` proof without reusing preserved hunks.
