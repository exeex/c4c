# AArch64 Address-Valued Memory And Call Argument Publication

Status: Open
Created: 2026-05-21
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair AArch64 generated-code handling where a value may be an address to
materialize, a pointer value to reload, or a pointee to load/store, especially
when that value crosses indirect memory and call-argument publication
boundaries.

## Why This Exists

The post-closure backend-regex inventory has a dominant residual family where
prepared BIR distinguishes address, pointer, and pointee values, but generated
AArch64 reloads or republishes the wrong storage class.

Representative failures:

- `00020` prepares `pp -> *pp -> **pp`, but generated AArch64 returns the
  intermediate pointer address instead of the final `i32` pointee value.
- `00170` prepares an address-exposed local `%lv.epos` for `deref_uintptr`,
  but generated AArch64 reloads `[sp, #8]` instead of materializing `sp+8`.
- `00189` reloads an unpublished stack home for `stdout` before the second
  indirect call instead of publishing the loaded global pointer there.
- `00173` pointer loops reload fixed string addresses instead of advancing
  through the current `*b` and `*src` values.

This owner is split from the umbrella inventory before coding so the repair is
semantic, not a backend-regex pass-count exercise.

## In Scope

- Localize the first bad handoff where AArch64 lowering chooses between
  materializing an address, reloading a pointer value, or loading/storing a
  pointee.
- Repair indirect memory publication for address-valued stack homes, pointer
  loads, and pointee loads/stores.
- Repair call-argument publication when address-valued or pointer-valued
  sources are stack-homed, symbol/global-derived, or prepared through an
  address-exposed local.
- Add focused backend coverage that proves the ownership boundary without
  depending on c-testsuite filenames.
- Prove at least one minimal representative such as `00020` plus one boundary
  representative from `00170` or `00189`, then check nearby pointer cases for
  adjacency.

## Out Of Scope

- Scalar comparison/select materialization residuals such as `00112`,
  `00123`, `00183`, `00200`, and likely `00218`.
- Floating-point scalar and variadic-call correctness such as `00174`.
- Composite/byval/HFA/f128 ABI work such as `00140` and `00204`.
- Dynamic stack, `stacksave`/`stackrestore`, goto, or timeout-specific work
  such as `00207`.
- Complex aggregate initializer, flexible-array, unnamed-union, range
  initializer, or relocation work such as `00216`.
- Expectation changes, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or filename-specific
  shortcuts.

## Acceptance Criteria

- The first bad fact is localized to a concrete address/pointer/pointee
  publication boundary in generated AArch64 or selected backend state.
- Focused backend coverage fails before the repair and passes after it for the
  repaired boundary.
- At least two representatives advance, including `00020` and one of `00170`
  or `00189`, or a narrower first bad fact is recorded and split before scope
  expands.
- Nearby cases `00005`, `00103`, `00173`, and `00181` are either advanced by
  the same semantic repair or explicitly reclassified by fresh first-bad-fact
  evidence.
- The supervisor-selected focused proof remains stable, and a broader backend
  guard is run before treating the owner as closure-ready.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00020`, `00170`, `00189`, `00173`, one source function, one
  stack offset, one symbol name, or one emitted instruction neighborhood
  instead of repairing general address/pointer/pointee publication semantics;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims capability progress through helper renames, classification notes,
  emitted-text rewrites, or pass-count movement without fixing the generated
  address/pointer/pointee handoff;
- broadens into scalar compare/select, floating variadic scalar, composite ABI,
  dynamic stack/goto, or aggregate initializer/relocation work without fresh
  first-bad-fact evidence and a lifecycle split;
- leaves generated code able to reload an unpublished stack home or return an
  intermediate address where the source semantics require a pointer value or
  pointee value.
