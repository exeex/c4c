# AArch64 String Literal Pointer Null Comparison

Status: Open
Created: 2026-05-21
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair the AArch64 lowering residual where a string-literal pointer compared
against null can fail to materialize either the pointer constant or comparison
result, returning a stale register value.

## Why This Exists

The post-365 backend inventory selected `00112` as the leading current
non-timeout residual because it has a small, concrete generated-code first bad
fact. Source `return "abc" == (void *)0;` should compare a non-null
string-literal pointer against null and return false. Generated AArch64 for
`main` is only `mov x0, x13; ret`, with `.str0` emitted afterward, so neither
the `.str0` address nor the pointer/null comparison result is materialized
before returning.

Existing open ideas are only adjacent. Parked idea 356 covers dynamic
pointer-derived byte loads that must remain dynamic in semantic BIR; this
owner is direct string-literal pointer constant comparison/result publication
with no byte load.

## In Scope

- Localize how string-literal pointer constants are represented and published
  for AArch64 scalar pointer comparisons.
- Determine whether the first bad fact is missing string-literal address
  materialization, missing null comparison lowering, missing boolean result
  publication, or return-value publication consuming a stale register.
- Repair the general AArch64 lowering path for pointer constant comparisons
  involving string literals and null.
- Add focused backend coverage for string-literal pointer/null comparison
  result publication independent of `00112`.
- Prove `c_testsuite_aarch64_backend_src_00112_c` advances past the stale
  return register or passes.

## Out Of Scope

- Dynamic pointer-derived string byte loads from idea 356.
- Pointer-derived load/address scaling, materialized pointer storelocal
  writeback, local pointer reassignment to aggregate fields, indexed aggregate
  writeback, scalar FP expression materialization, file API return-count
  publication, global `sizeof` bounds, aggregate initializer layout, enum
  bit-field layout, timeout cases, or other backend inventory buckets.
- Changing expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts.
- Fixing only `00112`, one `.str0` literal, one register, one source line, or
  one emitted `mov x0, x13; ret` sequence.

## Acceptance Criteria

- The first bad fact is localized to a concrete string-literal pointer
  materialization, pointer comparison, boolean result publication, or return
  publication boundary.
- Focused backend coverage proves string-literal pointer/null comparisons
  publish a defined boolean result instead of a stale register.
- The repair is general for pointer constant comparisons involving string
  literals and null, not a named representative shortcut.
- `c_testsuite_aarch64_backend_src_00112_c` no longer fails because `main`
  returns stale `x13` for `"abc" == (void *)0`.
- Any remaining `00112` failure is reclassified by its new first bad fact
  before this idea is closed.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00112`, `"abc"`, `.str0`, `(void *)0`, one source line, one
  register such as `x13`, or the exact `mov x0, x13; ret` text instead of
  repairing pointer constant comparison/result publication generally;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through emitted-text reshuffling while string-literal
  pointer/null comparisons can still return an unpublished or stale register;
- broadens into dynamic pointer-derived byte loads, pointer-address scaling,
  aggregate writeback, scalar FP, file I/O, global `sizeof`, or other backend
  inventory buckets without fresh first-bad-fact evidence and lifecycle
  routing;
- proves only the external representative while leaving focused pointer
  constant comparison behavior unguarded.
