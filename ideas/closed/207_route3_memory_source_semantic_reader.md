# 207 Route 3 memory/source semantic reader

## Goal

Move exactly one AArch64 memory/source semantic reader to consume selected
Route 3 source identity before prepared lookup fallback, without changing
address formation or target-addressing policy.

## Why This Exists

Phase D2 accepted this as one Step 4 proof-shape row in
`docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`.
Route 3 can provide selected memory/source identity, but prepared memory
lookups and target-addressing policy remain authoritative.

## In Scope

- One named AArch64 memory/source semantic reader.
- Route/prepared agreement for the named reader's source identity.
- Prepared fallback for absent, invalid, or mismatched Route 3 facts.
- Proof that prepared addressing output, x86 wrapper behavior, and expected
  strings remain unchanged.

## Out Of Scope

- Address formation, frame/global/TLS relocation, offsets, address spaces,
  base-plus-offset legality, materialization, final operands, or target policy.
- Public fallback removal, prepared API deletion, aggregate migration, or
  `PreparedFunctionLookups` retirement.
- Baseline refreshes, helper renames, unsupported downgrades, or expected-string
  rewrites.

## Acceptance Criteria

- Positive: route/prepared agreement selects the same source identity for the
  named reader.
- Absent: missing Route 3 fact keeps prepared lookup behavior.
- Invalid: wrong or unusable Route 3 reference fails closed to prepared lookup.
- Mismatch: route/prepared disagreement preserves the prepared result.
- Fallback: target-addressing and address-materialization paths remain
  prepared-owned.
- Printer/debug, wrapper, and expected-string proof shows no output changes.

## Closure Note

Closed after the Route 3 global-load semantic reader slice completed the
runbook. The implemented reader consumes Route 3 source identity only through
route/prepared agreement and keeps prepared memory/addressing authoritative for
target policy, GOT-vs-direct selection, final operands, scratch/target
registers, relocation spelling, and emission.

Close proof used the accepted four-test backend scope:
`backend_aarch64_prepared_memory_operand_records`,
`backend_aarch64_instruction_dispatch`,
`backend_x86_shared_producer_query`, and
`backend_x86_prepared_handoff_label_authority`, with 4/4 tests passing in both
`test_before.log` and `test_after.log`. The lifecycle-only close gate accepted
equal pass counts with no new failures.

## Reviewer Reject Signals

- Reject testcase-shaped matching for a named memory case instead of a semantic
  Route 3 reader rule.
- Reject unsupported downgrades, helper renames, baseline refreshes, or expected
  string changes as evidence of progress.
- Reject any diff that moves target-addressing policy or address-materialization
  ownership into Route 3.
- Reject broad `memory_accesses`, address-materialization, wrapper, or
  `PreparedFunctionLookups` migration.
- Reject retaining the old prepared-only failure mode behind a new route-named
  abstraction.
