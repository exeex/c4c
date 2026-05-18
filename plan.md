# AArch64 Function Pointer Indirect Call Values

Status: Active
Source Idea: ideas/open/289_aarch64_function_pointer_indirect_call_values.md
Activated From: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md

## Purpose

Repair the AArch64 backend path for first-class function-pointer values that
eventually feed indirect `blr` calls.

## Goal

Make function symbols, function-pointer stores/loads, function-pointer
returns, and global/static function-pointer initializers produce valid callee
values for indirect AArch64 calls.

## Core Rule

Fix semantic function-pointer value lowering and indirect-call setup. Do not
improve this route by changing tests, expected outputs, runner behavior, CTest
registration, allowlists, unsupported classifications, timeout policy, parser,
sema, or c-testsuite filename/symbol-specific shortcuts.

## Read First

- `ideas/open/289_aarch64_function_pointer_indirect_call_values.md`
- `todo.md`
- `tests/c/external/c-testsuite/src/00087.c`
- `tests/c/external/c-testsuite/src/00089.c`
- `tests/c/external/c-testsuite/src/00124.c`
- `tests/c/external/c-testsuite/src/00210.c`

## Current Targets

- AArch64 function symbol address materialization as pointer values.
- Function-pointer values in local aggregate fields and local slots.
- Function-pointer values in global/static aggregate initializers.
- Function returns whose result is a function pointer.
- Indirect-call callee operand lowering into the register consumed by `blr`.
- Focused proof cases: `src/00087.c`, `src/00089.c`, `src/00124.c`, and
  `src/00210.c`.

## Non-Goals

- Do not revisit stack-frame/SP alignment unless a new unaligned frame is
  proven.
- Do not change runner behavior, allowlists, expectations, unsupported
  classifications, timeout settings, or CTest registration.
- Do not change parser or sema behavior.
- Do not add named-case shortcuts, function-name shortcuts, aggregate-field
  shortcuts, or emitted-instruction-text matching.
- Do not take on broad aggregate ABI, broad struct layout rewrites, libc
  behavior, compare/branch lowering, `_Generic`, or timeout-sensitive cases.

## Working Model

The previous SP/frame-alignment idea removed the misaligned-frame owner from
the former bus-error cluster. The remaining bus-error representatives have
aligned frames and crash before or at indirect `blr`, where the callee value
should be a materialized function pointer carried through locals, globals,
returns, or casts. The first repair should establish the smallest local
function-pointer field case, then expand across global initialization and
returned function-pointer values.

## Execution Rules

- Keep packets small and prove each packet with a fresh build or compile/run
  command chosen by the supervisor.
- Inspect generated AArch64 assembly when debugging, especially function
  symbol address materialization and the register used by `blr`.
- Preserve direct-call behavior and existing frame alignment.
- Document unrelated aggregate, attribute, frontend, or local-state blockers
  in `todo.md` instead of broadening this route.
- Escalate to reviewer scrutiny if the implementation appears to special-case
  one c-testsuite filename, one function name, one field name, or one emitted
  `blr` shape.

## Steps

### Step 1: Establish Minimal Local Function-Pointer Failure

Goal: Prove and localize the smallest remaining bus-error case to a bad
function-pointer value before indirect `blr`.

Primary target: `tests/c/external/c-testsuite/src/00087.c`.

Actions:

- Reproduce the current AArch64 backend behavior for `src/00087.c` with the
  supervisor-selected narrow command.
- Inspect generated function symbol materialization, the local aggregate field
  store/load, and the indirect `blr` callee register.
- Confirm the generated frame remains aligned so this is not an SP-alignment
  regression.
- Record the observed failure shape and owned backend surfaces in `todo.md`.

Completion check: `todo.md` names the current function-pointer value failure,
the backend lowering/emission surface to change, and the proof command/log
used for the baseline.

### Step 2: Repair Local Function-Pointer Value Lowering

Goal: Carry a function symbol through a local function-pointer field into a
valid indirect call target.

Primary target: AArch64 backend function-pointer value materialization,
local storage, reload, and indirect-call callee setup.

Actions:

- Add or repair the semantic path that materializes function symbol addresses
  as first-class pointer values.
- Ensure local stores/loads preserve callable function-pointer values.
- Ensure the indirect-call callee operand is moved into the register consumed
  by `blr`.
- Build and run the supervisor-selected narrow proof for `src/00087.c`.

Completion check: `src/00087.c` no longer bus-errors from a bad indirect-call
callee value.

### Step 3: Expand to Global Initializers and Returned Function Pointers

Goal: Show the repair generalizes beyond the local field case.

Primary target: `src/00089.c` and `src/00124.c`.

Actions:

- Run the supervisor-selected subset covering global/static function-pointer
  initialization and function-pointer return representatives.
- Inspect remaining failures and separate global initializer, return-value,
  aggregate, or call-chain defects from unrelated owners.
- Repair only defects inside this idea's function-pointer value and indirect
  call scope.
- Record proof results and deferred blockers in `todo.md`.

Completion check: `src/00089.c` and `src/00124.c` pass for this owner, or
`todo.md` clearly separates remaining failures that belong to another focused
idea.

### Step 4: Validate Attributed Function-Pointer Cast Overlap

Goal: Decide whether attributed function-pointer casts are covered by the same
backend function-pointer value owner.

Primary target: `tests/c/external/c-testsuite/src/00210.c`.

Actions:

- Run the supervisor-selected proof for `src/00210.c` after the core
  function-pointer cases are green.
- Inspect whether any remaining failure is backend function-pointer value
  loss, frontend attribute parsing/type handling, or unrelated call/stdio
  behavior.
- Repair only backend value/callee lowering defects inside this idea.
- If the failure reduces to a frontend or attribute-specific owner, record a
  separate follow-on instead of broadening this route.

Completion check: `src/00210.c` passes for this owner, or `todo.md` records
the exact non-owner blocker.

### Step 5: Closure Review

Goal: Confirm the route is not overfit and decide whether the source idea is
complete.

Primary target: the focused four-case function-pointer subset.

Actions:

- Re-run the supervisor-selected owner subset covering `00087`, `00089`,
  `00124`, and `00210`.
- Confirm generated frames remain aligned and no pass-count improvement came
  from tests, expectations, runner behavior, allowlists, unsupported
  classifications, or timeout policy.
- If broader former bus-error sampling exposes a separate semantic owner,
  record or split it instead of absorbing it into this route.

Completion check: focused proof supports closure of this source idea, or
`todo.md` records the exact remaining source-idea gap.
