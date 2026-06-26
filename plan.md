# RV64 Aggregate va_arg Helper Lowering Runbook

Status: Active
Source Idea: ideas/open/371_rv64_object_route_aggregate_va_arg_helper_lowering.md

## Purpose

Activate the next concrete RV64 object-route child under the prepared module
shape umbrella after the same-module byval aggregate call-argument route
closed.

## Goal

Lower the supported RV64 aggregate `va_arg` helper path from explicit prepared
helper facts, or route unsupported aggregate helper shapes with precise
diagnostics.

## Core Rule

RV64 object emission must consume prepared `va_arg_aggregate` helper facts and
target ABI metadata. It must not reconstruct aggregate layout, `va_list` state,
helper resources, or source intent from testcase names, C syntax, raw BIR
spelling, or diagnostics.

## Read First

- `ideas/open/371_rv64_object_route_aggregate_va_arg_helper_lowering.md`
- Closed vararg contract and helper prerequisite ideas from the 360 through
  367 chain, especially the notes that moved `src/920908-1.c` to the aggregate
  helper boundary
- Existing RV64 variadic helper, prepared-printer, and object-emission tests
- The prepared dump for `src/920908-1.c` around the `va_arg_aggregate` helper

## Current Targets

- Representative gcc torture case: `src/920908-1.c`
- RV64 object-route lowering for prepared aggregate `va_arg` helper records
- Prepared helper facts for source `va_list`, destination aggregate payload,
  aggregate access plan, and ABI movement requirements

## Non-Goals

- Do not reopen shared LIR/BIR vararg semantics from idea 360.
- Do not reopen `va_start` destination-address or overflow-area setup from
  ideas 365 and 366.
- Do not implement byval aggregate parameter homes or non-`va_arg` entry
  parameter-home lowering.
- Do not infer aggregate layout, helper resources, or `va_list` state from C
  source syntax, testcase names, raw BIR text, or target-side assumptions.
- Do not broaden into unrelated RV64 call, CFG, data-section, or EV64 encoding
  work.

## Working Model

The representative currently reaches a target ABI helper boundary:

```text
unsupported_variadic_helper_lowering: RV64 object route does not yet lower va_arg_aggregate helper
```

The active route is to audit the prepared helper contract, identify the first
safe aggregate `va_arg` payload class, prove the contract with focused tests,
and lower only the class whose facts are explicit enough for object emission.

## Execution Rules

- Keep implementation packets small and prove helper-contract behavior before
  accepting representative progress.
- Preserve fail-closed diagnostics for incomplete helper facts, unsupported
  aggregate classes, or ABI paths that remain unsupported.
- Do not weaken unsupported contracts, allowlists, diagnostics, or
  representative expectations to claim progress.
- Record any newly exposed representative boundary in `todo.md` and route it
  to an existing or new idea instead of broadening this plan silently.
- Use canonical `test_after.log` only when delegated by the supervisor for
  executor proof.

## Steps

### Step 1: Audit Prepared Aggregate va_arg Helper Facts

Goal: identify the authoritative prepared facts available at the first
aggregate `va_arg` helper boundary.

Primary target: `src/920908-1.c` at the prepared `va_arg_aggregate` helper.

Actions:

- Inspect the prepared helper record and surrounding prepared BIR.
- List available facts for source `va_list`, destination payload, aggregate
  size/alignment, access plan, ABI placement, frame or stack resources,
  clobbers, and helper result ownership.
- Compare those facts with the RV64 object-route helper-emission requirements.
- Decide whether the first supportable shape can lower directly or needs a
  narrower prepared helper-contract prerequisite.

Completion check:

- `todo.md` names the exact aggregate helper lowering surface and the first
  complete or missing fact set.

### Step 2: Add Focused Helper-Contract Coverage

Goal: encode the selected aggregate `va_arg` helper boundary in focused tests.

Primary target: prepared-printer, variadic helper, or RV64 object-emission
tests that prove helper facts before broad representative proof.

Actions:

- Add coverage for the first supportable aggregate `va_arg` helper shape.
- Add fail-closed coverage for missing helper facts, unsupported aggregate
  payload classes, ambiguous ABI movement, incomplete `va_list` state, dynamic
  layout, or unsupported destination resources.
- Keep tests semantic and independent of `src/920908-1.c` names or source
  spelling.

Completion check:

- Focused tests prove the expected prepared helper facts or the precise
  unsupported boundary before implementation is accepted.

### Step 3: Lower Supported Aggregate va_arg Helper Shape

Goal: implement RV64 object-route lowering for the selected supportable
aggregate `va_arg` helper shape.

Primary target: the RV64 variadic helper object-emission path and any narrow
helper needed to consume prepared aggregate helper facts.

Actions:

- Consume prepared source `va_list`, destination payload, aggregate access
  plan, and ABI movement facts.
- Emit only the supported aggregate movement sequence justified by those facts.
- Preserve precise unsupported diagnostics for incomplete or unsupported
  aggregate helper shapes.
- Avoid target-side reconstruction of aggregate layout, helper resources, or
  `va_list` state.

Completion check:

- Focused RV64 helper tests pass because semantic prepared helper facts are
  consumed, not because the target matched raw BIR spelling.

### Step 4: Rerun Representative And Route Next Boundary

Goal: prove whether `src/920908-1.c` advances beyond the aggregate `va_arg`
helper boundary.

Primary target: the narrow RV64 gcc torture object runner for
`src/920908-1.c`.

Actions:

- Rerun the representative using the supervisor-selected command.
- Document whether the case passes or advances to another distinct owner.
- If a new distinct initiative is exposed, report it instead of broadening this
  plan.

Completion check:

- `todo.md` records the representative outcome, proof command, and next owner.

### Step 5: Broader Guard And Closure Review

Goal: prepare this aggregate helper slice for supervisor acceptance and source
idea closure review.

Primary target: RV64 variadic helper, prepared-printer, object-emission, and
representative coverage.

Actions:

- Run the focused proof required for the implementation packet.
- Run broader backend coverage if shared variadic helper or ABI helper code was
  touched.
- Confirm no expectations, allowlists, unsupported contracts, or diagnostics
  were weakened to hide the aggregate helper boundary.
- Summarize any remaining aggregate `va_arg` helper gaps and route them to
  existing or new ideas.

Completion check:

- The implementation has fresh build/test proof, no testcase-overfit evidence,
  and clear lifecycle notes for any remaining owners.
