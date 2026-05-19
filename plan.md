# AArch64 LIR To BIR Local Memory Prepared Handoff Runbook

Status: Active
Source Idea: ideas/open/312_aarch64_lir_to_bir_local_memory_prepared_handoff.md
Activated from: ideas/open/295_backend_regex_failure_family_inventory.md Step 4 split

## Purpose

Repair the residual semantic `lir_to_bir` local-memory admission and
prepared-module handoff gap for `00204.c` and `00216.c` without expanding into
parked AArch64 printer, runtime, timeout, or direct-call buckets.

## Goal

Make the representative cases advance past the current GEP/load local-memory
semantic-family diagnostics through structured semantic facts and prepared
handoff behavior, not c-testsuite-specific matching.

## Core Rule

Progress must be a semantic admission or prepared-handoff capability. Do not
change expectations, unsupported classifications, allowlists, runners, timeout
policy, proof-log contents, CTest registration, or test contracts.

## Read First

- `ideas/open/312_aarch64_lir_to_bir_local_memory_prepared_handoff.md`
- `ideas/closed/297_lir_to_bir_local_memory_admission.md`
- `ideas/closed/298_lir_to_bir_global_pointer_aggregate_projection.md`
- `todo.md`
- existing focused evidence in `test_after.log`
- generated artifact absence under `build/c_testsuite_aarch64_backend/src/`
  for `00204.c` and `00216.c`

## Current Targets

- Representative tests:
  - `c_testsuite_aarch64_backend_src_00204_c`
  - `c_testsuite_aarch64_backend_src_00216_c`
- Existing semantic/prepared dump helpers for `00204.c`:
  - `backend_cli_dump_bir_00204_stdarg_semantic_handoff`
  - `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff`
  - `backend_cli_dump_bir_focus_function_filters_00204`
  - `backend_cli_dump_prepared_bir_focus_function_filters_00204`
  - `backend_cli_dump_prepared_bir_focus_block_entry_00204`
- Broad semantic guard:
  - `backend_lir_to_bir_notes`

## Non-Goals

- Do not reopen closed ideas 297, 298, or 311 from counts alone.
- Do not fold in `00164.c`, `00214.c`, direct-call shuffle, direct vararg,
  address-of-local, runtime nonzero, runtime mismatch/crash, timeout, or
  output-storm residuals.
- Do not implement AArch64 printer, assembler, linker, runtime, timeout, or
  runner changes under this owner.
- Do not rely on filename, function-name, diagnostic-string, or
  c-testsuite-number special cases.

## Working Model

The current representatives fail before emitted AArch64 artifacts exist. The
failure source is a semantic `lir_to_bir` local-memory family diagnostic as the
route approaches prepared-module handoff. The first implementation slice should
localize which structured local-memory fact is missing or discarded; later
slices should repair that fact publication and prove both representatives
advance past the current diagnostics.

## Execution Rules

- Keep routine progress in `todo.md`.
- Use the proof command from this runbook unless the supervisor narrows it for
  a packet.
- Preserve fail-closed behavior for unsupported semantic forms.
- If either representative advances to a machine-printer, assembler, linker,
  runtime, or timeout residual, record that first bad fact and return it to
  umbrella classification unless evidence proves this same semantic repair owns
  it.

## Ordered Steps

### Step 1: Localize Missing Local-Memory Handoff Fact

Goal: identify the exact semantic or prepared-handoff fact missing from
`00204.c` and `00216.c`.

Primary target: semantic `lir_to_bir` admission and prepared BIR handoff for
local-memory GEP/load forms.

Actions:

- Reproduce or inspect the current diagnostics for the two representative
  tests.
- Compare the `00204.c` dump helpers against the failing c-testsuite route.
- Inspect the corresponding `00216.c` path enough to decide whether it shares
  the same missing semantic fact.
- Record the localized first bad fact in `todo.md`.

Completion check:

- `todo.md` names the missing semantic/prepared-handoff fact, the owning code
  surface, and whether both representatives share it.

### Step 2: Repair Semantic Admission Or Prepared Handoff

Goal: publish or preserve the structured local-memory facts needed for the
representative GEP/load forms to reach prepared-module handoff.

Primary target: the code surface identified by Step 1.

Actions:

- Implement the narrow semantic repair without c-testsuite filename matching.
- Keep unsupported or incomplete forms fail-closed.
- Avoid unrelated frontend, backend, AArch64 printer, runtime, runner, timeout,
  or CTest changes.

Completion check:

- The focused proof scope advances past the old `00204.c` and `00216.c`
  local-memory semantic-family diagnostics, or `todo.md` records the next
  first bad fact with evidence.

### Step 3: Add Focused Coverage

Goal: make the repaired fact publication observable in local semantic or
prepared-handoff tests.

Primary target: existing `lir_to_bir` notes or CLI dump coverage.

Actions:

- Prefer extending existing semantic/prepared dump or notes coverage over
  broad new test scaffolding.
- Add `00216.c`-specific semantic/prepared coverage only if the current helper
  set cannot prove the shared repair.
- Keep assertions tied to structured semantic facts, not textual coincidences.

Completion check:

- Focused tests prove the repaired local-memory handoff fact without weakening
  existing contracts.

### Step 4: Validate And Classify Residuals

Goal: prove the owner result and classify any new focused residuals.

Primary target: supervisor-selected focused proof scope.

Actions:

- Run:

```bash
cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00216_c)$' > test_after.log 2>&1
```

- Record pass/fail results and first bad facts in `todo.md`.
- Do not claim this owner complete if the old semantic diagnostics remain.

Completion check:

- `todo.md` records fresh proof. The old local-memory semantic-family
  diagnostics are gone for both representatives, or the remaining blocker is
  explicitly localized for the next packet.
