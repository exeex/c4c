# String Authority Guard Unclassified Symbols

Status: Closed
Created: 2026-05-22
Closed: 2026-05-22

## Goal

Repair the live `string_authority_guard` workflow failure by eliminating or
properly reworking the remaining string-authority violations in backend and
prealloc data structures.

## Why This Exists

Fresh full-suite baseline evidence showed exactly one failing test:
`string_authority_guard`. The focused command
`ctest --test-dir build -j --output-on-failure -R '^string_authority_guard$'`
reported four unclassified symbols:

- `src/backend/bir/lir_to_bir/lowering.hpp:429: HfaReturnLaneMap: pattern=string-keyed-alias`
- `src/backend/mir/aarch64/module/module.hpp:97: PreparedValueHomeIndexes::homes_by_name: pattern=by-name-member`
- `src/backend/prealloc/regalloc/value_homes.hpp:18: PreparedComputedValueLookup::named_binaries: pattern=string-keyed-container`
- `src/backend/prealloc/regalloc/value_homes.hpp:19: PreparedComputedValueLookup::named_global_loads: pattern=string-keyed-container`

These symbols indicate remaining lookup surfaces keyed by textual names or
aliases where the compiler should prefer typed, structural, or symbol-identity
authority. The failure is a workflow guard violation, not a request to widen
the classifier allowlist.

## In Scope

- Inspect each reported symbol and identify the real authority the lookup
  should use instead of an arbitrary string name.
- Replace string-keyed aliases, containers, or by-name members with typed or
  structural keys where the surrounding backend model already exposes such an
  identity.
- Preserve current lowering, AArch64 module, and prealloc/regalloc behavior
  while removing the guard violations.
- Add or adjust focused code coverage only when needed to prove the new keying
  model preserves behavior.
- Prove `string_authority_guard` passes without modifying
  `scripts/string_authority_classifications.json`.
- Run a narrow build or relevant CTest subset after implementation, escalating
  if the changed authority path has broader backend blast radius.

## Out Of Scope

- Editing `scripts/string_authority_classifications.json`.
- Weakening, disabling, renaming, or reclassifying the
  `string_authority_guard` test.
- Adding temporary allowlist entries or comments that classify these symbols as
  acceptable without changing the underlying authority model.
- Broad rewrites of unrelated lowering, AArch64 module, prealloc, or regalloc
  ownership.
- Reopening parked AArch64 ideas or string literal runtime failures unless a
  fresh first-bad-fact proves this guard repair depends on them.

## Acceptance Criteria

- `ctest --test-dir build -j --output-on-failure -R '^string_authority_guard$'`
  passes on the current tree.
- The four reported symbols no longer appear as unclassified guard violations.
- No change is made to `scripts/string_authority_classifications.json`.
- The replacement authority for each changed lookup is documented in code by
  its type and usage, not by broad explanatory comments or allowlist metadata.
- A focused compile or CTest proof shows the touched backend/prealloc surfaces
  still build and preserve existing behavior.

## Closure Notes

Closed after replacing all four reported string-authority surfaces with typed
or interned value authority:

- `HfaReturnLaneMap` is keyed by lowered HFA return value identity.
- prepared AArch64 value homes resolve through prepared ids instead of
  `homes_by_name`.
- prepared computed binaries and global loads use interned `ValueNameId`
  lookup authority instead of string-keyed containers.

`scripts/string_authority_classifications.json` was not edited. The close-time
regression guard passed against matching six-test logs:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

The supervisor also reported full-suite acceptance after commit `5ef0b4156`:
`ctest --test-dir build -j --output-on-failure` passed 3381/3381.

## Reviewer Reject Signals

Reject the route if it:

- changes `scripts/string_authority_classifications.json`, disables
  `string_authority_guard`, or weakens the test contract instead of repairing
  the reported authority surfaces;
- renames `HfaReturnLaneMap`, `homes_by_name`, `named_binaries`, or
  `named_global_loads` merely to evade the scanner while preserving the same
  string-keyed or by-name authority;
- adds filename-specific, symbol-specific, or scanner-shape shortcuts that make
  only the four known reports disappear without replacing the underlying keying
  model;
- claims progress through comments, expectation rewrites, or classification
  notes while arbitrary strings still choose HFA lanes, prepared value homes,
  computed binaries, or global loads;
- broadens into unrelated AArch64 runtime repairs, semantic string literal
  fixes, ABI behavior changes, or full regalloc rewrites without a lifecycle
  split and fresh first-bad-fact evidence.
