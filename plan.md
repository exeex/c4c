# Semantic BIR Local/Pointer Memory Observation Canonicalization Runbook

Status: Active
Source Idea: ideas/open/369_semantic_bir_local_pointer_memory_observation_canonicalization.md
Activated From: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Repair the semantic-BIR observation surface for local and pointer memory so
backend-route tests observe canonical local stores, pointer-parameter accesses,
and dynamic aggregate/member lanes instead of string-literal substitution or
computed-pointer detours.

## Goal

Make the seven classified backend-route semantic-BIR observation residuals
advance or pass through semantic lowering changes, without expectation,
runner, timeout, CTest, unsupported-classification, proof-log, or backend
target-machine policy changes.

## Core Rule

Fix the semantic memory representation rule. Do not special-case route test
names, emitted snippets, temporary names, `@.str0`, one local variable, one
member index, one packed offset, or one observed dump line.

## Read First

- `ideas/open/369_semantic_bir_local_pointer_memory_observation_canonicalization.md`
- `todo.md`
- the seven backend-route failures classified from `test_after.log`
- semantic-BIR route dumps under `build/backend_route/`
- nearby completed or parked pointer-derived string/local memory notes only
  when checking for regression boundaries

## Current Targets

- `backend_codegen_route_x86_64_pointer_param_loaded_char_deref_observe_semantic_bir`
- `backend_codegen_route_x86_64_nested_pointer_param_dynamic_member_array_load_observe_semantic_bir`
- `backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir`
- `backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir`
- `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`
- `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`
- `backend_codegen_route_x86_64_packed_local_member_offsets_observe_semantic_bir`

## Non-Goals

- Do not work on AArch64 runtime mismatches, generated assembly, register
  allocation, machine-printer behavior, ABI call-boundary transport, libc
  calls, initializer lowering, unsigned enum bitfields, switch/fallthrough,
  aggregate writeback, timeout handling, runner policy, CTest registration,
  proof-log policy, unsupported classifications, or external c-testsuite
  expectations.
- Do not reopen parked idea 356 unless fresh evidence shows the exact old
  pointer-derived string-load first bad fact has returned.
- Do not perform broad backend-route expectation cleanup unrelated to the
  seven classified semantic-BIR observation failures.

## Working Model

The selected bucket fails before runtime as `BACKEND_ROUTE_SNIPPET_MISSING`.
The first bad boundary is the semantic-BIR dump/observation shape. The repairs
should keep pointer locals and dynamic local aggregate/member lanes visible as
canonical semantic memory operations when that is the source-level memory
meaning, instead of reducing them to generic computed pointer load/store paths
or substituting string-literal addresses for pointer-local observations.

## Execution Rules

- Start from first-bad evidence in the semantic-BIR dumps, not from filenames
  or pass-count movement.
- Prefer shared semantic-lowering helpers over testcase-shaped snippet edits.
- Keep focused coverage tied to semantic-BIR shape and nearby regression
  boundaries.
- Preserve completed pointer-derived string-load behavior from the idea
  356/365/366 chain unless a fresh first bad fact proves it changed.
- For code-changing steps, run at least build or compile proof plus the
  delegated focused backend-route subset before reporting completion.
- Escalate to broader backend-route or backend-regex proof only after the
  focused semantic surface changes or if touched code has wider blast radius.

## Steps

### Step 1: Localize the Semantic Observation Boundary

Goal: identify the semantic-BIR lowering or canonicalization point that turns
local/pointer memory operations into the observed wrong dump shapes.

Primary target: semantic lowering and backend-route dump generation paths for
local stores, pointer-parameter accesses, and dynamic local aggregate/member
lanes.

Actions:

- Compare expected and actual semantic-BIR dumps for all seven target tests.
- Trace the wrong shapes back to the first semantic representation choice:
  string-literal pointer local materialization, dynamic member-array load/store
  addressing, direct local aggregate lane addressing, or packed-member offset
  addressing.
- Identify whether the same lowering rule owns pointer-parameter and direct
  local cases, or whether the work needs two narrow packets.
- Record the exact first-bad helper or boundary in `todo.md` before changing
  code.

Completion check:

- `todo.md` names the first semantic boundary or helper family for the target
  failures and records whether the repair can be one packet or must split into
  pointer-local and dynamic-lane packets.

### Step 2: Repair Pointer Local String-Literal Materialization

Goal: preserve the expected pointer local store/load observation shape when a
pointer parameter or local interacts with string-literal address materialization.

Primary target: the semantic rule that currently stores `ptr @.str0` or
otherwise substitutes a literal address where the route expects pointer local
state.

Actions:

- Implement the smallest semantic lowering change that keeps pointer-local
  materialization observable without hard-coding `@.str0` or the target test.
- Add or update focused route coverage only if it proves the generalized
  semantic rule.
- Check nearby pointer-derived string-load behavior from the completed
  idea 356/365/366 chain.

Completion check:

- The pointer-param loaded-char dereference route advances or passes for the
  expected semantic-BIR reason, and nearby string-load coverage remains stable
  or is explicitly reclassified with evidence.

### Step 3: Repair Dynamic Local Aggregate/Member Lane Materialization

Goal: represent dynamic indexed local aggregate/member accesses through the
canonical local/member lane shape expected by semantic-BIR observers.

Primary target: dynamic member-array load/store lowering for pointer
parameters, direct locals, and packed local member offsets.

Actions:

- Repair the shared semantic rule for dynamic local aggregate/member lane
  materialization without matching one member index, packed offset, temporary,
  or emitted snippet.
- Cover load and store forms, direct local and pointer-parameter forms, and
  packed-member offsets.
- Keep generic computed-pointer load/store behavior available for cases whose
  source semantics really require it.

Completion check:

- The six dynamic local aggregate/member route targets advance or pass for the
  expected semantic-BIR reason, with no expectation or runner changes.

### Step 4: Focused Proof and Regression Boundary Check

Goal: prove the owner is repaired without drifting into adjacent backend or
runtime buckets.

Primary target: focused backend-route subset plus any nearby semantic pointer
or local-memory regression tests affected by the touched code.

Actions:

- Run build or compile proof for touched code.
- Run the delegated focused backend-route subset covering the seven targets.
- Run nearby semantic pointer/local-memory coverage when the touched code
  overlaps completed pointer-derived string-load or local-memory admission
  routes.
- If the focused semantic proof is green and blast radius justifies it, ask
  the supervisor whether to run broader backend-route or backend-regex proof.

Completion check:

- `todo.md` records the proof commands and results, the seven target outcomes,
  and any remaining residuals with first-bad evidence instead of pass-count
  claims.
