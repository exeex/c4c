# RV64 Scalar Vararg and Variadic Lowering Runbook

Status: Active
Source Idea: ideas/open/362_rv64_scalar_vararg_and_variadic_lowering.md

## Purpose

Continue the RV64 variadic backend work after idea 361 by moving from target
hook materialization into scalar `va_arg`, `va_copy`, and the first object-route
variadic function lowering gate.

## Goal

Consume explicit prepared vararg facts for scalar `va_arg` and `va_copy`, then
replace or narrow the broad RV64 object variadic function admission failure with
the first supportable lowering path or a precise unsupported diagnostic.

## Core Rule

RV64 object emission must consume prepared vararg facts. Do not infer variadic
semantics from source text, testcase names, LIR shape, or BIR shape inside the
target route.

## Read First

- `ideas/open/362_rv64_scalar_vararg_and_variadic_lowering.md`
- `ideas/closed/361_rv64_vararg_abi_hook_materialization.md`
- `ideas/closed/360_lir_bir_vararg_va_arg_contract_completion.md`
- `test_after.log` from the idea 361 close packet, if present
- focused backend tests around prepared vararg facts, RV64 object admission,
  scalar `va_arg`, and `va_copy`
- representative RV64 allowlist logs for `src/20030914-2.c` and
  `src/920908-1.c`

## Current Scope

- scalar `va_arg` prepared facts reaching the RV64 target boundary
- `va_copy` prepared facts reaching the RV64 target boundary
- RV64 homes/access plans for scalar `va_arg` and `va_copy`, or precise target
  ABI diagnostics where support is still intentionally absent
- the first narrow object-route variadic function lowering path after the broad
  admission gate
- representative allowlist proof for `src/20030914-2.c` and `src/920908-1.c`

## Non-Goals

- Do not reopen completed idea 361 target-hook acceptance criteria unless a real
  regression reintroduces the original missing facts.
- Do not improve scan results through unsupported expectation downgrades, skip
  broadening, or allowlist filtering.
- Do not implement testcase-name or source-pattern shortcuts for scalar
  `va_arg`, `va_copy`, `src/20030914-2.c`, or `src/920908-1.c`.
- Do not broaden into globals, vector ABI, FPR ABI, generic instruction
  selection, or unrelated RV64 object-route features.
- Do not require complete support for every C vararg corner before the focused
  scalar and admission-gate behavior is understood.

## Working Model

Idea 361 proved that RV64 now receives the required variadic entry,
`va_list`, `va_start`, and aggregate `va_arg` hook facts for its completed
milestone. Remaining failures are separate lowering work: focused scalar
`va_arg` facts, focused `va_copy` facts, and the broad object-route variadic
function admission gate.

## Execution Rules

- Keep implementation packets narrow enough for build proof plus focused
  backend tests.
- Prefer exact structured diagnostics over guessed ABI homes or access plans.
- Treat representative torture cases as coverage signals, not implementation
  keys.
- Preserve non-variadic RV64 object-route behavior.
- Escalate validation when changes affect shared prepared facts or general RV64
  object admission.

## Steps

### Step 1. Audit Scalar Vararg and `va_copy` Boundary Facts

Goal: identify the exact prepared facts and RV64 target/admission sites for the
remaining scalar `va_arg`, `va_copy`, and variadic function gate failures.

Concrete actions:

- Inspect focused backend tests and logs that still report scalar `va_arg` or
  `va_copy` missing facts.
- Inspect representative logs for `src/20030914-2.c` and `src/920908-1.c`,
  starting from the idea 361 close proof when available.
- Map each remaining diagnostic to the prepared fact producer, prepared fact
  consumer, and RV64 target/admission surface that should materialize or reject
  it.
- Record the first implementation boundary and narrow proof command in
  `todo.md`.

Completion check:

- The next executor can name the exact RV64 target or admission site for the
  first scalar `va_arg`, `va_copy`, or variadic gate packet, with a narrow proof
  command that does not depend on testcase filtering for success.

### Step 2. Consume or Precisely Diagnose Scalar `va_arg`

Goal: make scalar `va_arg` prepared facts actionable at the RV64 target
boundary.

Concrete actions:

- Inspect the prepared scalar `va_arg` access plan, source `va_list` homes, and
  result homes that reach RV64 object emission.
- Materialize the narrow scalar cases whose prepared facts and RV64 ABI homes
  are explicit and unambiguous.
- Replace broad missing-fact diagnostics with precise target ABI diagnostics for
  unsupported scalar classes.
- Add focused backend tests for supported and unsupported scalar `va_arg`
  paths.

Completion check:

- Focused backend tests show scalar `va_arg` facts are consumed by RV64 lowering
  or rejected with precise diagnostics, and the original scalar missing-fact
  failure is not merely renamed.

### Step 3. Consume or Precisely Diagnose `va_copy`

Goal: make `va_copy` prepared facts actionable at the RV64 target boundary.

Concrete actions:

- Inspect the prepared `va_copy` source and destination `va_list` homes.
- Materialize the narrow `va_copy` copy path where RV64 ABI layout and homes are
  explicit.
- Report precise target ABI diagnostics for unsupported `va_copy` source or
  destination classes.
- Add focused backend tests that distinguish materialized `va_copy` homes from
  unsupported diagnostics.

Completion check:

- Focused backend tests show `va_copy` facts are consumed by RV64 lowering or
  rejected with precise diagnostics, and no progress depends on source-shape or
  testcase-name inference.

### Step 4. Narrow the RV64 Variadic Function Admission Gate

Goal: move at least one narrow RV64 object variadic function lowering path past
the current broad admission gate, or replace the gate with diagnostics tied to
exact unsupported lowering classes.

Concrete actions:

- Inspect where RV64 object admission rejects variadic functions with
  `RV64 object variadic function lowering is not implemented`.
- Identify the smallest variadic function path whose prepared facts are already
  explicit enough to lower safely.
- Implement that path only if the target ABI facts, homes, and control-flow
  requirements are clear.
- Otherwise, split the broad gate into precise diagnostics for the exact
  unsupported class encountered.
- Add focused backend tests for the newly supported path or narrowed diagnostic.

Completion check:

- At least one narrow variadic function path moves beyond the broad admission
  gate, or the broad gate is replaced by narrower diagnostics that identify the
  exact unsupported lowering class.

### Step 5. Representative and Baseline Validation

Goal: prove the focused scalar/`va_copy` and admission-gate work is stable
enough to hand back to the broader RV64 object-route backlog.

Concrete actions:

- Run build proof and the focused backend tests touched by this plan.
- Run the representative RV64 allowlist cases:
  - `src/20030914-2.c`
  - `src/920908-1.c`
- Document representative outcomes in `todo.md`.
- Create a separate open idea only if remaining work is outside this focused
  scalar vararg and variadic lowering scope.

Completion check:

- Focused backend tests remain green, representative outcomes are improved or
  more precisely classified, and no accepted progress relies on testcase-name
  matching, expectation downgrades, or scan filtering.
