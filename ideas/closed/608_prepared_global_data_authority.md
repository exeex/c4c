# Prepared Global Data Authority

Status: Closed
Type: Implementation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `ideas/open/606_bir_global_initializer_bootstrap.md`
Owning Layer: prepared/global authority
Queue Order: 7
Prerequisites: BIR global initializer bootstrap must publish any required initializer facts; RV64/global emission waits for this authority
Estimated Evidence Breadth: `40` prepared/global authority rows
Proof Surface: selected global object-data, prepared global memory facts, and direct global-symbol base+offset rows

## Goal

Complete prepared/global authority for selected object data, global memory
facts, and direct global-symbol base+offset addressing before RV64 emits or
loads globals.

## Why This Exists

The current global data bucket crosses an owner boundary. `40` rows are
prepared/global authority problems and should be repaired before the `30`
RV64/global consumer rows are activated.

## In Scope

- Selected global object-data contract rows.
- Prepared global memory facts.
- Direct global-symbol base+offset authority.
- Handoff facts that RV64/global consumers may rely on later.

## Out Of Scope

- RV64 global symbol emission or access-width lowering.
- BIR initializer bootstrap, local memory, ABI, runtime/link behavior,
  expectations, unsupported markers, allowlists, timeouts, or accounting.

## Acceptance Criteria

- Multiple prepared/global authority rows progress past the current prepared
  stop.
- RV64/global rows remain target-consumer failures until a separate consumer
  idea is activated.
- The proof covers more than one of object data, global memory facts, and
  direct base+offset where possible.

## Reviewer Reject Signals

- Reject combining prepared/global authority with RV64 emission in one patch.
- Reject matching only representative global cases.
- Reject emitting global symbols without prepared authority facts.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as progress.
- Reject helper-only changes that retain the exact prepared/global authority
  stop.

## Lifecycle Notes

- `608` published accepted selected object-data authority for relocation-only
  pointer object data and moved `src/921110-1.c` from the prepared object-data
  contract stop to the RV64 relocation-record consumer stop.
- `608` published prepared global-memory fact authority for representative
  global rows, including the residual `src/pr36034-1.c` and `src/pr91137.c`
  byte-storage aggregate cases. Their prepared rows now carry global identity,
  offset, width, extent, layout authority, and supported direct base-plus-offset
  addressing evidence.
- Direct global-symbol base-plus-offset authority was verified for
  representatives including `src/pr79737-2.c`, `src/pr82387.c`,
  `src/pr68624.c`, and `src/pr57568.c`.
- Mixed object data with ordinary emitted bytes plus relocation slots is split
  to `ideas/closed/620_prepared_mixed_object_data_slots.md`.
- Remaining direct object-route failures for `src/pr36034-1.c` and
  `src/pr91137.c` are downstream RV64 prepared-global consumer/value-location
  handling, not producer authority, and are split to
  `ideas/open/621_rv64_prepared_global_value_location_consumer.md`.

## Completion Notes

Closed after the residual runbook proved the narrowed producer-authority route.
The close evidence included accepted byte-storage global layout authority,
backend subset proof, and a full-suite baseline at commit `cd470040e`
(`Publish byte-storage global layout authority`) with all tests passing.
