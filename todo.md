# AArch64 Scalar Call Value Semantics Todo

Status: Active
Source Idea Path: ideas/open/286_aarch64_scalar_call_value_semantics.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm Scalar Call-Value Failure Shape

# Current Packet

## Just Finished

Lifecycle switch completed after closing the AArch64 non-leaf LR preservation
idea. The active plan now starts at Step 1, "Confirm Scalar Call-Value Failure
Shape", for `ideas/open/286_aarch64_scalar_call_value_semantics.md`.

## Suggested Next

Executor should inspect the current AArch64 generated assembly or backend dumps
for `00116.c` and `00159.c`, trace scalar direct-call argument and return-value
lowering, and identify the focused backend proof command for the first code
slice.

## Watchouts

- Keep this route on ordinary scalar direct-call values first.
- Do not absorb variadic `printf`, string-literal addressing, loop predicates,
  short-circuit control flow, aggregate/pointer lowering, static storage, goto,
  or macro/preprocessor-adjacent cases.
- `00116.c` and `00159.c` are probes, not implementation selectors.
- Do not turn remaining failures into expectation changes, allowlist edits,
  timeout changes, CTest runner changes, or c-testsuite-specific shortcuts.

## Proof

No code proof has run for this newly activated plan yet. The LR preservation
close gate passed before activation of this follow-on idea.
