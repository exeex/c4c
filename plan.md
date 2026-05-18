# AArch64 Side-Effect Control-Value Publication Authority Runbook

Status: Active
Source Idea: ideas/open/293_aarch64_side_effect_control_value_publication_authority.md

## Purpose

Repair AArch64 backend publication of scalar values produced by side-effecting
expressions and control-selected expressions so later observable consumers use
the authoritative semantic value.

## Goal

Make the focused starter representatives pass by repairing the publication
authority chain, then prove the repair is not a filename-specific or
closed-owner overlap fix.

## Core Rule

Fix side-effect/control-value publication semantics. Do not broaden into
pointer/aggregate, timeout, printer/admission, floating/conversion/string, or
test harness work.

## Read First

- `ideas/open/293_aarch64_side_effect_control_value_publication_authority.md`
- `todo.md`
- Representative generated assembly under
  `build-aarch64-scan/c_testsuite_aarch64_backend/src/00164.c.s`
  `build-aarch64-scan/c_testsuite_aarch64_backend/src/00183.c.s`
  `build-aarch64-scan/c_testsuite_aarch64_backend/src/00202.c.s`
  `build-aarch64-scan/c_testsuite_aarch64_backend/src/00169.c.s`

## Current Targets

- Starter reps: `src/00164.c`, `src/00183.c`, `src/00202.c`.
- Boundary/support rep: `src/00169.c`.
- Main implementation surfaces are likely in AArch64 MIR codegen and prepared
  value publication paths, but Step 1 must locate the exact ownership boundary
  before edits.

## Non-Goals

- Do not touch expected outputs, runner behavior, allowlists, unsupported
  classifications, timeout policy, CTest registration, or build/test
  infrastructure.
- Do not reopen closed AArch64 owners for LR preservation, scalar call-value
  semantics, string/global external-call lowering, stack-frame alignment,
  function-pointer indirect calls, scalar parameter ALU authority, call-argument
  register authority, or scalar expression/control authority without
  contradictory generated-code proof.
- Do not claim progress by fixing only one c-testsuite filename or one emitted
  instruction sequence.
- Do not fold pointer/aggregate/address-heavy failures, timeouts,
  printer/admission failures, floating/conversion/string-only behavior, or
  broad aggregate ABI work into this route.

## Working Model

The broken cases compute, assign, or select a scalar value, but a later
consumer reads an older or unrelated physical location. The repair should make
the selected semantic value authoritative at publication points such as stores,
call arguments, return values, and print-visible scalar consumers.

## Execution Rules

- Start from generated-code evidence for the starter reps before editing.
- Prefer semantic publication rules over testcase-shaped matching.
- Keep each code slice narrow enough to prove with a focused CTest subset.
- After each runtime subset run, check for stale generated-runtime processes.
- Escalate to broader AArch64 backend sampling before treating the route as
  close-ready.

## Steps

### Step 1: Locate Side-Effect Publication Boundaries

Goal: identify the exact backend/prealloc boundary where side-effecting and
control-selected expression values lose authority.

Primary target: generated assembly and MIR/codegen paths for `src/00164.c`,
`src/00183.c`, and `src/00202.c`.

Actions:

- Build `c4cll` in `build-aarch64-scan`.
- Run the starter subset:
  `ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00183|00202)_c$' -j 4 --timeout 5 --output-on-failure`
- Inspect generated assembly and any useful dumps to separate assignment,
  compound-assignment, logical/comparison, and conditional-expression failure
  shapes.
- Record the first repair primitive and any rejected overlap with closed owners
  in `todo.md`.

Completion check:

- `todo.md` names the first concrete implementation boundary, starter failure
  evidence, proof command, and stale-process result.

### Step 2: Repair Side-Effecting Expression Result Publication

Goal: publish authoritative values for assignment-like and compound-assignment
expression results consumed later.

Primary target: the codegen/prepared-value boundary identified in Step 1.

Actions:

- Repair the smallest semantic primitive that keeps assigned objects and
  expression results aligned.
- Prove `src/00202.c` and the relevant `src/00164.c` expression forms without
  relying on stale stack slots or unrelated registers.
- Keep pointer-through-address and aggregate behavior out of scope unless the
  exact same scalar publication primitive owns it.

Completion check:

- The focused subset shows the repaired side-effecting expression cases passing
  or materially advanced, and generated assembly consumes the authoritative
  value.

### Step 3: Repair Control-Selected Expression Publication

Goal: publish values selected by conditional, logical, and comparison control
flow to later scalar consumers.

Primary target: conditional-expression and logical/comparison result
materialization for `src/00183.c` and remaining `src/00164.c` failures.

Actions:

- Repair control-selected scalar value materialization without reopening broad
  branch predicate or switch-dispatch owners.
- Prove selected results reach later stores, print calls, returns, or scalar
  call arguments.
- Use generated-code evidence to reject closed-owner or pointer/address drift.

Completion check:

- Starter reps `src/00164.c`, `src/00183.c`, and `src/00202.c` pass or are
  blocked only by a documented out-of-scope owner.

### Step 4: Validate Boundary and Broader Sampling

Goal: prove the route is semantic and close-ready rather than starter-only.

Primary target: `src/00169.c` plus nearby non-timeout runtime mismatch samples
chosen by the supervisor.

Actions:

- Run the starter subset plus boundary sample:
  `ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00169|00183|00202)_c$' -j 4 --timeout 5 --output-on-failure`
- Add broader nearby sampling only after pointer/address-heavy and closed-owner
  overlap cases are explicitly separated.
- Record remaining buckets in `todo.md` and request lifecycle close review only
  if the source idea acceptance criteria are satisfied.

Completion check:

- Starter reps pass, boundary sampling is explained, stale-runtime checks are
  clean, and broader validation does not show same-owner regressions.
