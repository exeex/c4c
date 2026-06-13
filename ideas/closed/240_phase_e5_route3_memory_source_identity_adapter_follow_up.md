# 240 Phase E5 Route 3 memory source identity adapter follow-up

## Goal

Prove and implement one Route 3 memory/source identity adapter that can read a
single semantic source identity fact while retaining prepared lookup delivery,
target policy, diagnostics, fallback, wrapper behavior, expected strings, and
baselines.

This is a narrow successor idea from the E5 gate artifact:

- `docs/bir_prealloc_fusion/phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`

## Why This Exists

The E5 gate found that no whole `PreparedBirModule` field group and no whole
`PreparedFunctionLookups` group is ready for deletion, privatization, or
aggregate replacement. It did identify Route 3 memory/source identity reads
inside `memory_accesses` as a possible one-reader adapter candidate.

This idea exists to test exactly that narrow boundary. It must not reopen
draft 155, claim aggregate retirement readiness, or move address formation,
target materialization, relocation, final operand selection, value homes,
wrappers, diagnostics, or fallback behavior into Route 3 ownership.

## In Scope

- One Route 3 memory/source identity read inside `memory_accesses`.
- One adapter that compares the route-owned source identity fact against the
  existing prepared lookup result before changing any consumer path.
- Prepared fallback for absent, invalid, duplicate/conflict, ambiguous,
  mismatch, unsupported, prepared-only, policy-sensitive, and non-agreement
  cases.
- Unchanged public prepared lookup delivery for diagnostics, helper-oracle
  tests, wrappers, compatibility consumers, and debug/printer paths.
- Nearby same-feature proof showing the adapter is semantic rather than
  fixture-shaped.

## Out Of Scope

- Whole `memory_accesses` deletion, privatization, or aggregate replacement.
- Whole `PreparedFunctionLookups` or `PreparedBirModule` retirement.
- Draft 155 opening, broad rewrite, or broad successor plan.
- Address formation, address materialization, relocation, final memory
  operands, value homes, stack/frame/register policy, wrapper output, or target
  emission policy ownership changes.
- Expected-string rewrites, helper renames, supported-path downgrades,
  unsupported relabeling, wrapper-output relabeling, timeout masking, or
  baseline refreshes as proof.

## Acceptance Criteria

- Exactly one Route 3 memory/source identity read is named and routed through
  an agreement-gated adapter.
- The positive path consumes route/BIR semantic identity only after agreement
  with the current prepared lookup result.
- All non-agreement and policy-sensitive paths continue to use the existing
  prepared fallback and public lookup behavior.
- Prepared printer/debug output, route-debug output, helper-oracle names and
  status labels, wrapper output, expected strings, supported-path contracts,
  and baselines remain byte-stable unless a separately approved source idea
  changes that contract.
- No public prepared aggregate or lookup group is deleted, hidden, or renamed
  as the claimed progress.

## Required Proof Matrix

The implementation is not acceptance-ready until proof covers:

- the selected positive Route 3 memory/source identity read;
- absent, invalid, duplicate/conflict, ambiguous, mismatch, unsupported,
  prepared-only, policy-sensitive, and non-agreement fallback where relevant;
- unchanged prepared lookup delivery for public consumers;
- unchanged diagnostic/oracle strings and helper status labels;
- unchanged printer/debug, route-debug, wrapper output, expected strings, and
  baselines;
- nearby same-feature memory/source cases proving the adapter is semantic and
  not tied to one named fixture;
- target-output no-change proof for adjacent address, frame, register,
  materialization, relocation, value-home, and wrapper surfaces.

## Reviewer Reject Signals

- The change claims or implies whole `memory_accesses`,
  `PreparedFunctionLookups`, `PreparedBirModule`, or draft 155 retirement
  readiness.
- The implementation moves address formation, materialization, relocation,
  final operands, value homes, frame/register policy, wrapper output, or target
  emission policy into Route 3 or target-neutral BIR ownership.
- The adapter is a facade rename or public API hiding exercise that preserves
  the old failure mode.
- The patch rewrites expectations, helper names, supported-path status,
  wrapper output, or baselines instead of preserving behavior.
- The change downgrades supported tests to unsupported or weakens public
  prepared fallback, diagnostic/oracle, debug/printer, wrapper, or helper
  contracts.
- The implementation adds named-case shortcuts, fixture-shaped matching, or
  special handling for one known memory testcase instead of consuming a real
  semantic source identity fact.

## Closure Note

Closed after implementing and proving one narrow Route 3 memory/source identity
adapter for the AArch64 dispatch value-materialization same-block global-load
identity read.

Completed scope:

- selected reader:
  `mir::find_bir_same_block_global_load_access_identity(...)` in the
  `LoadGlobalInst` producer branch of
  `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`;
- adapter boundary:
  `route3_agreed_same_block_global_load_access(...)`, which accepts the Route
  3 semantic identity only when it agrees with the prepared same-block
  global-load access returned by
  `prepare::find_prepared_same_block_global_load_access(...)`;
- fallback behavior: missing or non-agreeing Route 3 identity, unsupported
  producer shape, and policy-sensitive cases remain on the existing prepared
  path;
- target-policy ownership: address formation, materialization, relocation,
  final operands, value homes, wrappers, diagnostics, fallback, and target
  emission policy continue through the prepared path, including
  `emit_prepared_global_load_to_register(...)`;
- public compatibility evidence remains covered by
  `backend_prepared_lookup_helper`,
  `backend_aarch64_instruction_dispatch`, and
  `backend_aarch64_prepared_memory_operand_records`.

Close-time regression guard passed with:

```bash
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

The canonical logs use the same delegated subset:

```bash
cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_prepared_memory_operand_records|backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure > test_after.log
```

Result: before 3/3 passed, after 3/3 passed, no new failing tests.

This closure does not claim whole `memory_accesses`,
`PreparedFunctionLookups`, `PreparedBirModule`, or draft 155 retirement,
deletion, hiding, privatization, replacement, or aggregate readiness. It also
does not claim broad Route 3 reader coverage beyond the selected same-block
global-load adapter.
