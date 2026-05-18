# AArch64 String/Global Address External Call Lowering

Status: Active
Source Idea: ideas/open/287_aarch64_string_global_address_external_call_lowering.md
Activated From: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md

## Purpose

Repair the AArch64 backend path that fails to materialize string/global
addresses and place pointer-valued direct external-call arguments correctly.

## Goal

Make the starter stdio/string family execute through semantic AArch64 backend
lowering instead of passing uninitialized or incorrect address values to
external calls.

## Core Rule

Fix the backend capability. Do not improve this route by changing runner
behavior, CTest registration, allowlists, unsupported classifications,
expected outputs, timeout policy, parser/sema behavior, or c-testsuite
filename/literal-specific shortcuts.

## Read First

- `ideas/open/287_aarch64_string_global_address_external_call_lowering.md`
- `todo.md`
- `tests/c/external/c-testsuite/src/00125.c`
- `tests/c/external/c-testsuite/src/00131.c`
- `tests/c/external/c-testsuite/src/00154.c`
- `tests/c/external/c-testsuite/src/00161.c`
- `tests/c/external/c-testsuite/src/00197.c`
- `tests/c/external/c-testsuite/src/00206.c`
- `tests/c/external/c-testsuite/src/00211.c`

## Current Targets

- AArch64 backend string-literal address materialization.
- AArch64 backend global/static data address materialization where the value is
  used as a pointer.
- Pointer-valued call arguments and formatted scalar arguments for direct
  external calls, especially stdio calls such as `printf`.
- Starter proof cases: `src/00125.c`, `src/00131.c`, `src/00154.c`,
  `src/00161.c`, and `src/00211.c`.
- Post-sampling remaining owner cases: `src/00197.c` and `src/00206.c`.

## Non-Goals

- Do not touch runner behavior, allowlists, expectations, unsupported
  classifications, timeout settings, or CTest registration.
- Do not change parser or sema behavior.
- Do not add named-case shortcuts, literal-spelling shortcuts, or `printf`-only
  special cases.
- Do not take on full aggregate ABI/HFA returns, broad variadic ABI
  completeness, `_Generic`, wide chars, or function-pointer casts.
- Do not use `src/00132.c` as the first proof; it is timeout-sensitive and
  compounded by loop/local-store behavior.

## Working Model

The current failure family is visible in successfully emitted AArch64 programs
that call external stdio functions with bad address arguments. The first
repair should establish a general backend mechanism for materializing
address-valued data and moving those values into ABI argument registers for
direct calls. Once the smallest string-literal case works, expand to repeated
prints and formatted scalar cases to prove the fix is not a named-case patch.

## Execution Rules

- Keep packets small and prove each packet with a fresh build or compile/run
  command chosen by the supervisor.
- Inspect generated AArch64 assembly when debugging, but accept progress only
  through semantic behavior and relevant test proof.
- Prefer lowering/emission changes that generalize across string literals,
  global/static addresses, and pointer-valued call arguments.
- Document unrelated blockers in `todo.md` instead of broadening this route.
- Escalate to reviewer scrutiny if the implementation appears to special-case
  one c-testsuite filename, one literal, or `printf` alone.

## Steps

### Step 1: Establish the Minimal Address Argument Failure

Goal: Prove and localize the smallest non-timeout failure in the selected
family.

Primary target: `tests/c/external/c-testsuite/src/00125.c`.

Actions:

- Reproduce the current AArch64 backend behavior for `src/00125.c` with the
  supervisor-selected narrow command.
- Inspect the generated call path and identify where the string-literal
  address should become the value passed in `x0`.
- Record the observed failure shape and owned backend surfaces in `todo.md`.

Completion check: `todo.md` names the current failure, the backend lowering or
emission surface to change, and the proof command/log used for the baseline.

### Step 2: Implement General Address Materialization for Call Arguments

Goal: Materialize string/global/static addresses and route them into direct
external-call ABI argument registers without case-specific matching.

Primary target: AArch64 backend lowering/emission for address-valued call
arguments.

Actions:

- Add or repair the semantic path that produces AArch64 address materialization
  for string literals and global/static data references.
- Ensure pointer-valued call operands are assigned to the proper argument
  registers before direct external calls.
- Keep the implementation independent of specific c-testsuite filenames,
  literal spellings, or `printf` as a unique callee.
- Build and run the supervisor-selected narrow proof for `src/00125.c`.

Completion check: the minimal proof for `src/00125.c` passes or any remaining
failure is documented as a distinct blocker that is not the original missing
address/call-argument owner.

### Step 3: Expand to Starter Stdio/Data Representatives

Goal: Show the repair generalizes across the selected family.

Primary target: `src/00131.c`, `src/00154.c`, `src/00161.c`, and
`src/00211.c`.

Actions:

- Run the supervisor-selected subset covering the starter representatives.
- Inspect any remaining failures and separate address/call-argument defects
  from unrelated scalar, aggregate, local-memory, or frontend blockers.
- Repair only defects that are inside this idea's address materialization and
  direct external-call argument scope.
- Record proof results and any deferred blockers in `todo.md`.

Completion check: the starter family passes for this owner, or `todo.md`
clearly separates remaining failures that belong to another focused idea.

### Step 4: Sample Related Mismatch Cases and Decide Closure

Goal: Confirm the route is not overfit to the starter cases and decide whether
the source idea is complete.

Primary target: related stdio/data mismatch cases from the inventory.

Actions:

- Ask the supervisor for a broader related-case subset after starter proof is
  green.
- Include nearby stdio/data cases that exercise the same address and direct
  external-call argument owner.
- Keep `src/00132.c` out of first acceptance proof; inspect it only as
  timeout-sensitive overlap evidence after the starter family is repaired.
- If the broader subset exposes a separate semantic owner, record a follow-on
  idea instead of absorbing it into this route.

Completion check: broader related-case proof supports closure of this focused
idea, or `todo.md` records the exact remaining source-idea gap.

### Step 5: Repair Static/Global Data Pointer Materialization

Goal: Fix the remaining sampled global/static data address or value
materialization failure without broadening into unrelated scalar/local-state
owners.

Primary target: `tests/c/external/c-testsuite/src/00197.c`.

Actions:

- Inspect the generated AArch64 for `src/00197.c` and identify whether the
  wrong values come from missing static/global data address materialization,
  incorrect load-from-address emission, or argument-register setup for the
  external print call.
- Repair only the semantic path for global/static data values that flow as
  pointer or print-call operands within this source idea's backend owner.
- Keep loop, branch, recursion, switch, conditional-expression,
  scalar/local-state, aggregate ABI, and internal direct-call issues out of
  this step unless they are proven to be merely downstream of the same
  global/static address materialization defect.
- Build and run the supervisor-selected narrow proof for `src/00197.c`, plus
  the already-green starter owner subset when practical.

Completion check: `src/00197.c` no longer prints address-shaped incorrect
values for the owned static/global data path, or `todo.md` records the exact
remaining non-owner blocker that prevents this step from closing.

### Step 6: Repair `%s` String-Literal External-Call Pointer Arguments

Goal: Fix the remaining sampled string-literal pointer argument segfault for a
direct external call.

Primary target: `tests/c/external/c-testsuite/src/00206.c`.

Actions:

- Inspect the generated AArch64 for `src/00206.c` and identify where the
  `%s` argument should receive a materialized string-literal address before
  the external `printf` call.
- Repair the general string-literal pointer argument path for direct external
  calls; do not add a `printf`, `%s`, literal-spelling, or filename-specific
  shortcut.
- Re-run the supervisor-selected proof for `src/00206.c`, then re-run the
  owner subset including `00125`, `00131`, `00154`, `00197`, `00206`, and
  `00211` when the narrow proof is green.
- If the failure reduces to a different semantic owner, record that in
  `todo.md` and ask for lifecycle review instead of folding broad unrelated
  work into this idea.

Completion check: `src/00206.c` no longer segfaults from the owned
string-literal direct external-call pointer path, and the focused owner subset
supports either source-idea closure review or a clearly documented separate
blocker.
