# Prepared Global Data Authority

Status: Open
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
