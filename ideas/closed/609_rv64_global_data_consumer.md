# RV64 Global Data Consumer

Status: Closed
Type: Implementation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `ideas/open/608_prepared_global_data_authority.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
Owning Layer: RV64/global consumer
Queue Order: 8
Prerequisites: prepared/global authority facts from `608` or equivalent proof that upstream authority already exists
Estimated Evidence Breadth: `30` RV64/global consumer rows
Proof Surface: global symbol emission and global access-width rows after prepared/global authority is present

## Goal

Teach the RV64 object route to consume prepared global symbols and supported
global access widths without inventing missing upstream authority.

## Why This Exists

The scan shows `17` global symbol emission rows and `13` global access-width
rows whose first owner is RV64/global consumption after prepared handoff.

## In Scope

- RV64 emission of prepared global symbols.
- RV64 handling for supported prepared global access widths.
- Diagnostics that keep unsupported or missing-authority globals separate.

## Out Of Scope

- Prepared/global authority production, BIR initializer bootstrap, library or
  string policy, ABI, runtime/link work, expectations, unsupported markers,
  allowlists, timeouts, or accounting.

## Acceptance Criteria

- Multiple RV64/global consumer rows progress when prepared/global authority
  exists.
- Rows missing prepared facts continue to fail with producer or authority
  diagnostics instead of being guessed by RV64.
- Proof covers both symbol emission and access-width behavior when possible.

## Completion Summary

Closed after the active runbook completed all five steps and Step 5 recorded
the consumer-side handoff boundary. The refreshed nine-row allowlist had one
new pass, `src/20030224-2.c`, and the remaining rows were assigned outside
this idea's RV64/global consumer scope:

- Runtime/downstream: `src/pr61517.c`.
- Publication/link: `src/20010924-1.c`, `src/pr57877.c`, `src/pr57860.c`,
  and `src/20020118-1.c`.
- Producer-authority or unsupported-width: `src/20000703-1.c`,
  `src/pr82387.c`, and `src/20020213-1.c`.

The close gate used backend CTest logs with 346 passing tests before and after,
with no new failures.

## Reviewer Reject Signals

- Reject target-local inference of global data not published by prepared
  authority.
- Reject merging this idea with prepared/global production.
- Reject named-case-only emission fixes.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes.
- Reject retaining unsupported global emission behind renamed diagnostics.
