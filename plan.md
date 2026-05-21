# AArch64 Scalar FP Expression Constant Publication Runbook

Status: Active
Source Idea: ideas/open/378_aarch64_scalar_fp_expression_constant_publication.md
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Repair the focused AArch64 scalar floating-point constant/expression
publication residual selected from the backend inventory after idea 377.

## Goal

Make generated scalar FP consumers observe the intended float or double
constant/expression value instead of stale or unmaterialized FP registers.

## Core Rule

Repair a general scalar FP publication capability. Do not special-case
`00174`, do not change expectations or runners, and do not pull aggregate,
timeout, HFA/variadic, or external call-result buckets into this owner without
fresh first-bad-fact evidence.

## Read First

- `ideas/open/378_aarch64_scalar_fp_expression_constant_publication.md`
- `todo.md`
- `test_after.log`
- `tests/c/external/c-testsuite/src/00174.c`
- `build/c_testsuite_aarch64_backend/src/00174.c.s`

## Current Targets

- `c_testsuite_aarch64_backend_src_00174_c`
- Scalar float/double constants and expression values feeding ordinary
  generated AArch64 FP consumers.
- The remaining `00174` unary-plus scalar FP lvalue residual where `+a`
  publishes zero while nearby unary-minus and comparison rows still work.
- Focused backend coverage to prove the repaired scalar FP publication path.

## Non-Goals

- Do not implement aggregate initializer, compound literal, relocation, or
  function-pointer-table behavior for `00216`.
- Do not implement dynamic stack/VLA/goto timeout behavior for `00207`.
- Do not implement shift/type-promotion timeout behavior for `00200`.
- Do not reopen external/libc call-result publication, HFA/variadic floating,
  integer immediate materialization, selected-value publication, or aggregate
  local-address owners without generated-code evidence.
- Do not change expectations, unsupported classifications, allowlists, timeout
  policy, runner behavior, CTest registration, or proof-log policy.

## Working Model

The inventory selected `00174` because it is the strongest current
non-timeout residual. Runtime output shows early scalar FP arithmetic values
printing as zero, while later comparison rows and final `sin(2)`-related
output survive. Generated assembly prints from FP registers such as `d13`
without an obvious preceding materialization of the first expected constants.

Treat that as a starting hypothesis. The executor must localize the first
missing scalar FP value before editing code, then repair the shared FP
constant/expression publication path that feeds the consumer.

## Execution Rules

- Localize the first bad fact from source output to generated AArch64 before
  changing implementation code.
- Prefer focused backend coverage for the repaired FP publication behavior
  before using `00174` as external proof.
- Keep `00216`, `00200`, and `00207` parked unless fresh evidence proves they
  share the scalar FP publication owner.
- Acceptance needs fresh build or focused compile/test proof chosen by the
  supervisor.
- Treat expectation rewrites, filename-shaped fixes, register-only shortcuts,
  and output-count-only claims as route failures.

## Steps

### Step 1: Localize First Scalar FP Bad Fact

Goal: identify the first scalar FP constant or expression value that is missing
or stale before its generated consumer.

Primary target: `00174.c`, `00174.c.s`, and the `00174` excerpt in
`test_after.log`.

Actions:

- Map the earliest incorrect runtime output row back to the source expression.
- Trace the generated AArch64 value setup for that expression through the
  print or comparison consumer.
- Identify whether the missing value is an FP literal, arithmetic expression,
  assignment/unary/coercion result, or consumer handoff problem.
- Record adjacent rows that still work so the owner boundary stays narrow.

Completion check:

- `todo.md` records the first scalar FP bad fact, the consumer, and the
  suspected backend boundary.

### Step 2: Add Focused Backend Coverage

Goal: create focused coverage for the localized scalar FP publication behavior
before relying on the external representative.

Primary target: existing backend tests that cover AArch64 FP constant or
expression publication paths.

Actions:

- Find the nearest backend test location for scalar FP constants or simple
  float/double expression consumers.
- Add the smallest focused case that fails before the repair and describes the
  localized owner.
- Keep the coverage general enough to reject a `00174`-only fix.

Completion check:

- Focused coverage exposes the scalar FP publication failure without needing
  to run the full external c-testsuite route.

### Step 3: Repair Scalar FP Publication

Goal: make the prepared/lowered AArch64 path publish scalar FP constants or
expression results to the value consumed by later FP operations or calls.

Primary target: the backend boundary localized in Step 1.

Actions:

- Repair the shared scalar FP materialization or publication path.
- Preserve existing integer immediate, selected-value, aggregate-address,
  call-result, and HFA/variadic behavior.
- Avoid register-name or filename-specific branches.

Completion check:

- Focused backend coverage from Step 2 passes after the repair.

### Step 4: Prove External Representative And Guardrails

Goal: verify that `00174` advances or passes and that adjacent backend
guardrails stay stable.

Primary target: supervisor-selected focused proof command plus the `00174`
external backend test.

Actions:

- Run the delegated build/proof command exactly as provided by the supervisor.
- Run or report the delegated `00174` representative proof.
- Record whether `00174` passes or advances to a new first bad fact.
- Record any remaining parked buckets separately instead of absorbing them
  into this idea.

Completion check:

- `todo.md` records proof results, residual classification for `00174` if it
  does not pass, and whether the source idea is ready for close review.

### Step 5: Classify Unary-Plus FP Lvalue Publication

Goal: repair the advanced `00174` residual where unary plus on a scalar FP
lvalue loses the value before an ordinary print consumer.

Primary target: the AArch64 lowering/classification path for scalar FP lvalues
used through no-op unary operators.

Actions:

- Map row 13 `printf("%f\n", +a)` back through generated AArch64 and the
  relevant expression classification path.
- Compare the row 13 `+a` path with the adjacent row 14 `-a` path and the rows
  that now publish correctly.
- Identify whether unary plus incorrectly reclassifies an FP lvalue, drops the
  prepared FP storage/register value, or falls through an integer/immediate
  publication route.
- Repair the shared scalar FP expression publication/classification behavior
  without special-casing `00174`, row 13, unary-plus spelling, one register, or
  one emitted instruction sequence.
- Add or extend focused backend coverage if the existing scalar FP publication
  coverage does not exercise the classified unary-plus/lvalue owner.

Completion check:

- Focused coverage and the external `00174` representative prove that unary
  plus on scalar FP lvalues publishes the original FP value, while nearby
  unary-minus and scalar FP rows remain stable.
