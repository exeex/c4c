# BIR Semantic Producer High-Impact Cleanup

Status: Open
Type: Implementation planning idea
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Owning Layer: BIR/semantic producer
Handoff Directory: `docs/rv64_gcc_torture_post_contract/`

## Goal

Prioritize high-frequency BIR semantic producer gaps exposed by RV64
gcc_torture, especially local-memory load/store/GEP, call-argument metadata,
and memcpy/memset families.

## Why This Exists

The exploratory `try_gcc_torture` run showed hundreds of failures still stop
before prepared object handoff with
`backend object route requires semantic lir_to_bir lowering`. These are more
important than F128 and must be repaired in BIR instead of hidden in RV64.

## In Scope

- Bucket semantic producer failures by family and representative rows.
- Focus first on ordinary C coverage: load/store local memory, GEP,
  alloca-derived storage, direct-call argument metadata, memcpy, and memset.
- Identify which rows require a new BIR semantic model, such as byte/object
  representation writes, and route them to separate ideas.
- Produce implementation-ready BIR ideas with fail-closed contracts.

## Out Of Scope

- RV64/MIR recovery from raw BIR shape.
- F128 helper or ABI work.
- Broad BIR rewrites without row ownership.
- Expectation, allowlist, unsupported-marker, or runtime-comparison changes.

## Acceptance Criteria

- Semantic failures are grouped by BIR producer family with counts and examples.
- Each proposed implementation idea has a BIR-owned contract and rejected
  adjacent shapes.
- Any byte/object-representation work is routed through the existing or
  refreshed BIR byte-storage idea rather than patched in RV64.
- Default CTest remains stable.

## Reviewer Reject Signals

- Reject MIR/RV64 changes claimed as semantic producer repair.
- Reject fabricating semantic pointer values from byte patterns.
- Reject broad producer rewrites without focused fail-closed tests.
- Reject F128-first work under this idea.
- Reject classification-only changes claimed as capability progress.
