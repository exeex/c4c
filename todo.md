Status: Active
Source Idea Path: ideas/open/418_prepared_target_consumer_boundary_audit.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Make One Low-Risk Cleanup Or Owner Decision

# Current Packet

## Just Finished

Completed Step 3 by recording a concrete docs-only owner decision in
`docs/prepared_fact_contracts/target_consumer_boundary_audit.md` for
`418-AUD-RV64-OBJECT-GLOBAL-RECOVERY-001` and
`418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001`.

The decision assigns object/global data recovery to Idea 417, with a secondary
Idea 415 handoff only for symbol/value materialization fallback in the global
memory helper row. It explicitly rejects RV64 target-side reconstruction of
initializer bytes, object sections, labels, raw value names, and memory-access
provenance from BIR/global fragments. Required replacement facts are prepared
global initializer/storage, publication, memory access, and value
materialization facts, with missing facts classified as `producer_missing`,
contradictory facts as `producer_incoherent`, and complete unsupported data
forms as `target_unsupported_but_coherent`.

File-reference proof is recorded for the rejected RV64 recovery surfaces:
`prepared_global_memory_emit.cpp:75`, `:183`, `:248`, `:290`, `:505`, `:565`,
and `object_emission.cpp:8409`, `:8422`, `:8463`, `:8506`, `:8550`.

## Suggested Next

Execute Step 4 for idea 418. Suggested packet: finalize the audit artifact for
handoff by checking that the Step 3 owner decision, downstream handoff table,
and row classifications give ideas 414-417 enough exact row IDs and reject
signals to consume without repeating the audit.

## Watchouts

- `docs/prepared_fact_contracts/target_consumer_boundary_audit.md` is now the
  canonical row map for this active plan's downstream handoff and includes the
  Step 3 RV64 global/object owner decision.
- Idea 417 owns object/global bytes, zero-fill, sections, relocations,
  publication identity, and storage-layout/initializer contracts for
  `418-AUD-RV64-OBJECT-GLOBAL-RECOVERY-001` and
  `418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001`.
- Idea 415 owns only the symbol/value-materialization fallback portion of
  `418-AUD-RV64-GLOBAL-MEMORY-RECOVERY-001`.
- AArch64 value operands, variadic helpers, and inline asm rows are coherent
  reference rows, not current cleanup targets.
- The AArch64 frame-slot publication and stack-preservation rows should not be
  changed without first deciding which silent skip paths are optional
  non-applicable routes versus required-but-missing producer facts.

## Proof

Docs-only owner-decision packet. No build or test command was required or run.
Proof is the Step 3 owner-decision section and row/file-reference evidence in
`docs/prepared_fact_contracts/target_consumer_boundary_audit.md`; no
`test_after.log` was created or modified.
