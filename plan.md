# RV64 Object Route Non-Register Parameter Homes Runbook

Status: Active
Source Idea: ideas/open/374_rv64_object_route_non_register_param_homes.md

## Purpose

Activate the RV64 object-route boundary exposed after aggregate `va_arg` work:
entry parameters whose prepared homes are not supported GPR homes or prepared
FPR register homes.

## Goal

Lower the first supportable RV64 object-route non-register parameter-home form
from explicit prepared ABI facts, or keep unsupported parameter homes routed to
precise fail-closed diagnostics.

## Core Rule

RV64 object emission must consume prepared parameter-home, frame, and ABI facts.
It must not reconstruct entry-parameter placement from source syntax, testcase
names, assumed stack layouts, or target-side ABI guesses.

## Read First

- `ideas/open/374_rv64_object_route_non_register_param_homes.md`
- Parent/sibling boundaries:
  - `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
  - `ideas/open/368_rv64_object_route_frame_slot_base_offset_memory.md`
  - `ideas/open/370_rv64_object_route_byval_aggregate_param_homes.md`
  - `ideas/open/371_rv64_object_route_aggregate_va_arg_helper_lowering.md`
  - `ideas/open/372_rv64_object_route_frame_slot_address_call_args.md`
- Existing RV64 object-route parameter home handling, prepared frame-stack
  call contract coverage, prepared-printer tests, and focused backend fixtures.
- Prepared dumps for `src/va-arg-13.c` around the current
  `unsupported_param_home` boundary.

## Current Targets

- Representative gcc torture case: `src/va-arg-13.c`
- RV64 object-route entry-parameter home classification and lowering
- Prepared parameter-home metadata for non-register homes, including any frame
  or ABI facts needed to consume them safely
- Focused backend tests for supported and rejected non-register parameter-home
  shapes

## Non-Goals

- Do not implement byval aggregate parameter homes; route that to idea 370.
- Do not implement aggregate `va_arg` helper lowering; route that to idea 371.
- Do not implement frame-slot address call-argument materialization; route that
  to idea 372.
- Do not reopen pointer-value local-memory loads/stores from closed idea 368.
- Do not infer parameter layout from source syntax, testcase names, raw BIR
  spelling, or assumed RV64 stack layouts.
- Do not broaden into unrelated RV64 call, frame, data-section, CFG, or
  encoding work.

## Working Model

The representative currently reaches this object-route stop before later
frame-slot-address and aggregate `va_arg` owners can be observed:

```text
unsupported_param_home: RV64 object route requires all parameters in supported GPR or prepared FPR register homes
```

The active route is to audit prepared parameter-home facts, select the first
non-register home whose explicit metadata is sufficient for object emission,
prove both supported and rejected shapes with focused coverage, and then rerun
`src/va-arg-13.c` to identify the next owner.

## Execution Rules

- Keep implementation packets small and tied to one parameter-home contract at
  a time.
- Preserve fail-closed diagnostics for unsupported stack, aggregate,
  missing-home, non-default address-space, dynamic-layout, and ABI-ambiguous
  parameter homes.
- Add semantic backend coverage before claiming representative progress.
- Do not weaken unsupported expectations, allowlists, diagnostics, or broad
  test contracts to move `src/va-arg-13.c`.
- Record any newly exposed representative boundary in `todo.md` and route it
  to an existing or new idea instead of broadening this plan silently.
- Use canonical `test_after.log` only when delegated by the supervisor for
  executor proof.

## Steps

### Step 1: Audit Prepared Non-Register Parameter Homes

Goal: identify the authoritative prepared facts available at the current
non-register parameter-home boundary.

Primary target: `src/va-arg-13.c` at `unsupported_param_home`.

Actions:

- Inspect the prepared parameter-home records and surrounding prepared BIR for
  the representative and focused backend fixtures.
- List available facts for home kind, frame base, frame offset, size,
  alignment, address space, aggregate status, dynamic layout, ABI ownership,
  and any materialization requirements.
- Compare those facts with RV64 object-route entry-parameter requirements.
- Select the first supportable non-register home shape, or identify the exact
  missing fact that keeps the shape unsupported.

Completion check:

- `todo.md` names the exact parameter-home lowering surface and the first
  complete or missing fact set.

### Step 2: Add Focused Parameter-Home Coverage

Goal: encode the selected parameter-home contract in focused backend tests.

Primary target: RV64 object-emission, prepared-printer, or prepared frame-stack
call contract tests that prove prepared home facts before representative proof.

Actions:

- Add coverage for the first supportable non-register parameter-home shape.
- Add fail-closed coverage for unsupported stack, aggregate, missing-home,
  non-default address-space, dynamic-layout, and ABI-ambiguous parameter homes.
- Keep tests semantic and independent of `src/va-arg-13.c` names or source
  spelling.

Completion check:

- Focused tests prove the expected prepared parameter-home facts or the precise
  unsupported boundary before implementation is accepted.

### Step 3: Lower Supported Non-Register Parameter Home Shape

Goal: implement RV64 object-route support for the selected non-register
parameter-home shape.

Primary target: the RV64 object-route entry-parameter home handling path and
any narrow helper needed to consume prepared frame or ABI facts.

Actions:

- Consume only explicit prepared parameter-home, frame, and ABI metadata.
- Materialize the selected home using the prepared base/offset/size/alignment
  facts required by the object route.
- Preserve precise unsupported diagnostics for incomplete or unsupported
  parameter-home shapes.
- Avoid target-side reconstruction of ABI placement or testcase-shaped
  matching.

Completion check:

- Focused RV64 backend tests pass because prepared parameter-home facts are
  consumed semantically, not because the target matched a named representative
  or raw BIR spelling.

### Step 4: Rerun Representative And Route Next Boundary

Goal: prove whether `src/va-arg-13.c` advances beyond the
`unsupported_param_home` boundary.

Primary target: the narrow RV64 gcc torture object runner for
`src/va-arg-13.c`.

Actions:

- Rerun the representative using the supervisor-selected command.
- Document whether the case advances to frame-slot address call arguments,
  aggregate `va_arg`, passes, or exposes another distinct owner.
- If a new distinct initiative is exposed, report it instead of broadening this
  plan.

Completion check:

- `todo.md` records the representative outcome, proof command, and next owner.

### Step 5: Broader Guard And Closure Review

Goal: prepare this parameter-home slice for supervisor acceptance and source
idea closure review.

Primary target: RV64 object-emission, prepared frame-stack call contract,
prepared-printer, and representative coverage.

Actions:

- Run the focused proof required for the implementation packet.
- Run broader backend coverage if shared parameter-home, frame, ABI, or object
  route code was touched.
- Confirm no expectations, allowlists, unsupported contracts, or diagnostics
  were weakened to hide the parameter-home boundary.
- Summarize any remaining non-register parameter-home gaps and route them to
  existing or new ideas.

Completion check:

- The implementation has fresh build/test proof, no testcase-overfit evidence,
  and clear lifecycle notes for any remaining owners.
