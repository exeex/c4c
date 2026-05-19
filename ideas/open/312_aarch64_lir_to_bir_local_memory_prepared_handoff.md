# AArch64 LIR To BIR Local Memory Prepared Handoff

Status: Open
Created: 2026-05-19
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair the residual semantic `lir_to_bir` local-memory admission and
prepared-module handoff path represented by `00204.c` and `00216.c`, without
reopening broader closed owners or folding in later AArch64 printer/runtime
residuals.

## Why This Exists

Umbrella idea 295 classified the fresh backend-regex evidence after closed
idea 311 as 352 selected tests, 295 passed tests, and 57 failed tests. Step 3
selected the remaining crisp semantic compile-stage owner:

- `c_testsuite_aarch64_backend_src_00204_c` fails before generated assembly
  with `semantic lir_to_bir function 'myprintf' failed in gep local-memory
  semantic family`.
- `c_testsuite_aarch64_backend_src_00216_c` fails before generated assembly
  with `semantic lir_to_bir function 'foo' failed in load local-memory
  semantic family`.

Neither representative has current generated
`build/c_testsuite_aarch64_backend/src/*.s` or `*.bin` output, so this owner is
semantic admission and prepared-handoff work, not AArch64 assembly spelling,
machine printing, linking, runtime behavior, timeout behavior, or runner
behavior.

This owner is separate from closed idea 297's direct local-memory admission
owner and closed idea 298's global/pointer/aggregate projection owner. The
residual evidence appears after those closure boundaries: the failing facts are
now localized to the local-memory GEP/load semantic family as it reaches the
prepared-module handoff.

## In Scope

- Inspect the semantic `lir_to_bir` path that admits local-memory GEP and load
  forms for prepared-module handoff.
- Preserve structured local-memory facts needed by prepared BIR for the
  representative `00204.c` and `00216.c` shapes.
- Repair admission or fact publication so these cases advance past the current
  local-memory semantic-family diagnostics without filename-specific logic.
- Add narrowly scoped semantic or prepared-handoff tests when existing dump
  coverage is insufficient.
- Classify any new focused residual after the old semantic diagnostics are
  removed.

## Out Of Scope

- Reopening closed ideas 297 or 298 from pass counts alone.
- Repairing AArch64 machine-printer, assembler, linker, runtime nonzero,
  runtime mismatch/crash, timeout, output-storm, direct-call shuffle, direct
  vararg, address-of-local, scalar `mul` printable-RHS, or unprepared
  frame-slot source residuals.
- Treating `00164.c`, `00214.c`, runtime buckets, or timeout buckets as part
  of this owner without a separate inventory split.
- Expectation rewrites, unsupported-classification changes, allowlist changes,
  runner edits, timeout-policy changes, proof-log edits, or CTest
  registration changes.
- Filename-only, function-name-only, diagnostic-string-only, or c-testsuite
  number-specific fixes.
- Broad rewrites of unrelated frontend, HIR, LIR, BIR, backend runtime, or
  AArch64 codegen/printer behavior.

## Acceptance Criteria

- `c_testsuite_aarch64_backend_src_00204_c` advances past the current
  `myprintf` GEP local-memory semantic-family admission diagnostic.
- `c_testsuite_aarch64_backend_src_00216_c` advances past the current `foo`
  load local-memory semantic-family admission diagnostic.
- Existing semantic/prepared dump helpers for `00204.c` remain green or are
  updated only to reflect stronger structured semantic facts, not weaker
  contracts.
- Any new focused-scope residual is classified by its new first bad fact
  instead of hidden behind the old semantic diagnostics.
- Fresh build proof is recorded for implementation slices.
- Focused proof uses:

```bash
cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00216_c)$'
```

## Reviewer Reject Signals

Reject the route if it:

- adds named-case checks for `00204.c`, `00216.c`, `myprintf`, `foo`, one
  diagnostic string, or one c-testsuite number instead of repairing semantic
  local-memory admission or prepared-handoff fact publication;
- changes expectations, unsupported classifications, allowlists, timeout
  policy, runner behavior, CTest registration, proof-log contents, or test
  contracts to improve counts;
- claims capability progress while either representative still fails with the
  same GEP/load local-memory semantic-family diagnostic behind a renamed
  helper or alternate wrapper;
- reopens closed ideas 297, 298, or 311 without generated-code, diagnostic, or
  proof evidence contradicting their closure boundaries;
- folds machine-printer, assembler, linker, runtime, timeout, direct-call, or
  unrelated semantic residual buckets into this owner without a separate
  focused source idea;
- proves only one representative while leaving the neighboring `00204.c` /
  `00216.c` local-memory semantic handoff behavior unexamined;
- performs broad frontend, BIR, backend, or AArch64 codegen rewrites outside
  the semantic admission/prepared-handoff path without a lifecycle switch.
