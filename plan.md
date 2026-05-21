# AArch64 Address-Valued Memory And Call Argument Publication Runbook

Status: Active
Source Idea: ideas/open/355_aarch64_address_valued_memory_call_argument_publication.md

## Purpose

Repair the AArch64 backend boundary where lowering must distinguish an address
to materialize, a pointer value to reload, and a pointee to load/store.

## Goal

Make address-valued memory and call-argument publication semantic enough to
advance the focused residual family split from the backend-regex inventory.

## Core Rule

Do not fix named c-testsuite files directly. Every code slice must repair a
general address/pointer/pointee publication boundary and prove it with focused
backend coverage plus representative external cases.

## Read First

- `ideas/open/355_aarch64_address_valued_memory_call_argument_publication.md`
- `todo.md`
- `test_after.log` from the Step 1 backend-regex inventory
- generated AArch64 artifacts under `build/c_testsuite_aarch64_backend/`
- selected/prepared backend dumps for `00020`, `00170`, `00189`, and nearby
  pointer cases when needed

## Current Targets

- Minimal indirection representative: `c_testsuite_aarch64_backend_src_00020_c`
- Boundary representatives: `00170` address-exposed local materialization and
  `00189` stack-homed global/pointer call-argument publication
- Adjacent pointer cases: `00005`, `00103`, `00173`, and `00181`

## Non-Goals

- Scalar comparison/select materialization: `00112`, `00123`, `00183`,
  `00200`, likely `00218`
- Floating-point scalar and variadic-call correctness: `00174`
- Composite/byval/HFA/f128 ABI work: `00140`, `00204`
- Dynamic stack/goto timeout work: `00207`
- Complex aggregate initializer/relocation work: `00216`
- Expectation, unsupported, runner, timeout, CTest-registration, or proof-log
  changes

## Working Model

The owner boundary is the value-kind decision at publication time:

- materialize an address when the consumer needs the address of storage;
- reload a pointer value when the consumer needs the pointer stored in storage;
- load or store a pointee when the consumer needs the value behind the pointer.

Repairs should prefer existing prepared-value, selected-home, and publication
helpers over new testcase-shaped branches.

## Execution Rules

- Start from the smallest representative that shows the wrong value-kind
  handoff.
- Add or extend focused backend tests before relying on c-testsuite movement.
- Keep each implementation packet narrow enough to identify the repaired
  boundary.
- If a representative exposes scalar compare/select, ABI, dynamic-stack, or
  initializer semantics instead, stop and route that residual through
  lifecycle instead of widening this owner.
- Use `test_after.log` for delegated proof unless the supervisor chooses a
  different canonical artifact.

## Ordered Steps

### Step 1: Localize The Value-Kind Boundary

Goal: identify the concrete AArch64 handoff where lowering confuses address,
pointer, and pointee values.

Actions:

- Inspect generated AArch64 and backend dumps for `00020`, then one of
  `00170` or `00189`.
- Identify the producer, prepared home, selected home, and consumer that
  disagree about materialize-address versus reload-pointer versus
  load/store-pointee.
- Record nearby cases that share the same boundary and cases that do not.

Completion check:

- `todo.md` names the first repair boundary, the smallest failing
  representative, adjacency candidates, and rejected out-of-scope residuals.

### Step 2: Repair Minimal Indirect Memory Publication

Goal: make the smallest indirect memory representative consume the intended
pointer or pointee value.

Actions:

- Add focused backend coverage for the localized indirect-memory boundary.
- Repair the relevant AArch64 publication path using semantic value-kind
  information already available in prepared or selected lowering state.
- Prove the focused backend test and `00020`.

Completion check:

- The focused backend test passes and `00020` advances or passes without
  expectation, runner, timeout, or filename-specific changes.

### Step 3: Repair Address-Valued Call Argument Publication

Goal: handle stack-homed or symbol/global-derived address/pointer values at
call boundaries without reloading unpublished homes.

Actions:

- Add focused backend coverage for a call argument that needs an address or
  pointer value from a prepared home.
- Repair call-argument publication for the localized boundary.
- Prove one representative from `00170` or `00189`.

Completion check:

- The focused backend test passes and at least one call-boundary
  representative advances or passes.

### Step 4: Check Adjacent Pointer Cases

Goal: determine whether the same repair covers the nearby pointer residuals or
whether a new lifecycle owner is required.

Actions:

- Run the supervisor-selected focused subset including `00005`, `00103`,
  `00173`, and `00181` when appropriate.
- For any remaining failure, capture the first bad fact and decide whether it
  stays in scope.
- Route out-of-scope residuals through lifecycle instead of broadening this
  plan.

Completion check:

- `todo.md` records which adjacent cases advanced, passed, or were split by a
  new first bad fact.

### Step 5: Broader Guard And Closure Decision

Goal: prove the focused owner is stable enough for lifecycle closure or a
clean split.

Actions:

- Run the focused proof selected by the supervisor.
- Run a broader backend guard when the supervisor treats the owner as a
  milestone or closure candidate.
- Ask plan-owner to close this idea only when the source idea acceptance
  criteria are satisfied.

Completion check:

- Regression logs and `todo.md` support either closure, parking with explicit
  residuals, or a lifecycle split.
