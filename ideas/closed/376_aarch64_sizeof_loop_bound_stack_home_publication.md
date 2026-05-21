# AArch64 Sizeof Loop-Bound Stack Home Publication

Status: Closed
Created: 2026-05-21
Closed: 2026-05-21
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair AArch64 lowering where compile-time scalar constants, especially
`sizeof`-derived loop bounds, are compared through stack homes that were never
initialized before the loop executes.

## Why This Exists

The post-375 backend inventory classified `00205` as the strongest current
non-timeout residual. Its global `cases:` data is emitted, but the runtime
prints no rows because both loop bounds are loaded from stack slots before any
value is stored there.

Representative generated assembly for `00205.c.s` compares the outer loop
index against values loaded from `[sp, #40]` and `[sp, #32]`, and the inner
loop index against `[sp, #64]` and `[sp, #56]`. The source bounds are
compile-time constants:

```c
sizeof(cases) / sizeof(cases[0])
sizeof(cases->c) / sizeof(cases->c[0])
```

That makes the first bad fact scalar constant or `sizeof` stack-home
publication for loop-bound consumers, not global aggregate data emission or
selected element publication.

## In Scope

- Localize where `sizeof` or constant-binary loop bounds are represented after
  semantic lowering, prepared value assignment, BIR/MIR handoff, and AArch64
  loop compare generation.
- Determine why loop-bound consumers select stack-backed homes for these
  constants without publishing the constant value into those homes or using a
  direct materialized immediate/register value.
- Repair the general scalar constant stack-home publication path so
  compile-time loop-bound values are available before loop compares.
- Add focused backend coverage for constant or `sizeof` loop-bound consumers
  whose selected representation previously depended on an uninitialized stack
  home.
- Prove `c_testsuite_aarch64_backend_src_00205_c` advances past the no-row
  loop-bound failure or passes.

## Out Of Scope

- Global aggregate data emission for `cases`, which is already present by
  current evidence.
- Selected global/static aggregate value publication from closed idea 373.
- Local aggregate address or bit-field layout publication from closed ideas
  374 and 375.
- External/libc call-result publication such as `00187`.
- Scalar FP expression or constant materialization such as `00174`.
- Complex aggregate initializer, compound relocation, or function-pointer
  table behavior such as the later `00216` residual.
- Dynamic stack/VLA fixed-slot timeout work such as `00207`.
- Shift/type-promotion timeout work such as `00200`.
- Fixing only `00205`, one source expression, one stack offset, one loop, one
  register, or one emitted compare sequence.

## Acceptance Criteria

- The first bad fact is localized to a concrete scalar constant,
  `sizeof`/constant-binary, prepared-value, stack-home, or AArch64 loop
  compare publication boundary.
- Focused backend coverage proves compile-time loop bounds do not read
  uninitialized stack homes before loop entry.
- `c_testsuite_aarch64_backend_src_00205_c` no longer fails because its outer
  or inner loop bounds are loaded from unpublished stack slots.
- Any remaining `00205` failure is reclassified by its new first bad fact
  before this idea is closed.
- The supervisor-selected proof scope introduces no new backend-regex
  failures and does not regress recent selected-value, aggregate address,
  aggregate layout, pointer-local, external-call, or scalar-publication
  guardrails.

## Closure Notes

The active runbook completed Step 4. `c_testsuite_aarch64_backend_src_00205_c`
passes, no remaining `00205` first bad fact was exposed in this owner, and the
generated AArch64 loop bounds now compare against folded immediates instead of
unpublished RHS stack-home loads.

Close-gate proof used canonical root logs `test_before.log` and
`test_after.log`, both covering matching `^backend_` runs with 144/144 passed.
The regression guard passed with:

```bash
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00205`, `cases`, `sizeof(cases)`, `sizeof(cases->c)`, one
  stack offset, one loop, one register, one comparison, or one emitted
  instruction neighborhood instead of repairing scalar constant stack-home
  publication generally;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, generated-text reshuffling, or
  classification-only notes while constant loop-bound consumers can still read
  unpublished stack homes;
- reopens closed selected-value, aggregate address, or local aggregate layout
  owners without fresh generated-code evidence that contradicts their closure
  boundaries;
- folds external call-result publication, scalar FP materialization, complex
  aggregate initializer/relocation, dynamic-stack timeout, or shift-promotion
  timeout work into this route without a fresh first-bad-fact handoff;
- proves only the external representative while leaving focused local backend
  coverage for constant or `sizeof` loop-bound stack-home publication absent.
