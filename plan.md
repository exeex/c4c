# String Authority Guard Runbook

Status: Active
Source Idea: ideas/open/383_string_authority_guard_unclassified_symbols.md

## Purpose

Eliminate the live `string_authority_guard` workflow failure by replacing the
remaining unclassified string-authority surfaces with typed or structural
authority.

## Goal

Make `string_authority_guard` pass without changing
`scripts/string_authority_classifications.json` or weakening the guard.

## Core Rule

Repair authority, do not hide scanner findings. A passing guard is acceptable
only when the reported lookups no longer rely on arbitrary string names or
string-keyed containers.

## Read First

- `ideas/open/383_string_authority_guard_unclassified_symbols.md`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/prealloc/regalloc/value_homes.hpp`
- `scripts/string_authority_classifications.json` for context only; do not edit
  it.

## Current Targets

- `HfaReturnLaneMap` at `src/backend/bir/lir_to_bir/lowering.hpp:429`
- `PreparedValueHomeIndexes::homes_by_name` at
  `src/backend/mir/aarch64/module/module.hpp:97`
- `PreparedComputedValueLookup::named_binaries` at
  `src/backend/prealloc/regalloc/value_homes.hpp:18`
- `PreparedComputedValueLookup::named_global_loads` at
  `src/backend/prealloc/regalloc/value_homes.hpp:19`

## Non-Goals

- Do not edit `scripts/string_authority_classifications.json`.
- Do not disable or weaken `string_authority_guard`.
- Do not add allowlist comments, scanner exceptions, or name-only renames to
  make the reports disappear.
- Do not perform broad backend, ABI, or regalloc rewrites outside the four
  reported authority surfaces.

## Working Model

Each target needs a concrete replacement authority:

- HFA lane return mapping should be keyed by the value, lane, or typed ABI
  record that owns the returned lane, not by a string alias.
- Prepared value home lookup should use the prepared value identity or a typed
  symbol/home key, not a by-name member map.
- Prepared computed binary and global-load lookups should use structural
  computed-value keys, instruction identities, or typed symbols instead of
  string-keyed containers.

## Execution Rules

- Keep each edit behavior-preserving unless the old behavior depended on
  string authority.
- Prefer local type changes over broad interface rewrites.
- When an existing caller only has a string name, trace why and move the typed
  authority boundary earlier rather than wrapping the string in a new alias.
- Update focused tests only to cover the new authority behavior, not to relax
  expectations.
- Use `test_after.log` for executor proof output unless the supervisor
  delegates a different proof artifact.

## Steps

### Step 1: Localize Current Authority Usage

Goal: identify every producer and consumer of the four reported symbols.

Actions:

- Inspect the declarations and all direct uses of `HfaReturnLaneMap`,
  `homes_by_name`, `named_binaries`, and `named_global_loads`.
- Record which typed object, enum, instruction id, value id, symbol id, or
  structural key should own each lookup.
- Confirm no proposed fix requires changing the string-authority classifier.

Completion check:

- Each target has a named replacement authority and a bounded edit path.

### Step 2: Replace HFA Return Lane String Authority

Goal: remove the `HfaReturnLaneMap` string-keyed alias from BIR lowering.

Primary target: `src/backend/bir/lir_to_bir/lowering.hpp`

Actions:

- Replace the alias or map key with the typed lane/value authority identified
  in Step 1.
- Update local producers and consumers to pass that typed key.
- Keep HFA lane ordering and return behavior unchanged.

Completion check:

- The old string-keyed `HfaReturnLaneMap` report is gone and focused build
  proof covers the touched lowering code.

### Step 3: Replace Prepared Home By-Name Lookup

Goal: remove `PreparedValueHomeIndexes::homes_by_name` as a by-name authority.

Primary target: `src/backend/mir/aarch64/module/module.hpp`

Actions:

- Replace by-name home lookup with the prepared value or home identity selected
  in Step 1.
- Update all callers to use typed authority at the point where homes are
  produced or consumed.
- Preserve existing prepared-value home publication behavior.

Completion check:

- `homes_by_name` no longer appears as a guard violation and focused proof
  covers the touched AArch64 module path.

### Step 4: Replace Prepared Computed Value String Containers

Goal: remove string-keyed containers for prepared computed binaries and global
loads.

Primary target: `src/backend/prealloc/regalloc/value_homes.hpp`

Actions:

- Replace `named_binaries` with a structural or typed computed-binary key.
- Replace `named_global_loads` with a structural or typed global-load key.
- Update producers and consumers together so arbitrary strings are not the
  authority boundary.

Completion check:

- Both prepared computed value reports are gone and focused proof covers the
  touched prealloc/regalloc path.

### Step 5: Prove The Guard And Adjacent Build Surface

Goal: produce acceptance-grade evidence for the lifecycle slice.

Actions:

- Run the exact focused guard:
  `ctest --test-dir build -j --output-on-failure -R '^string_authority_guard$'`
- Run the narrow compile or CTest subset needed for the touched backend and
  prealloc/regalloc surfaces.
- Escalate to a broader backend subset if the replacement keys cross subsystem
  boundaries.

Completion check:

- `string_authority_guard` passes.
- No implementation change touches
  `scripts/string_authority_classifications.json`.
- The selected compile or CTest proof is recorded in `todo.md`.
