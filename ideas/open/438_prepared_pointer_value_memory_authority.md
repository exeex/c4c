# Prepared Pointer-Value Memory Authority

Status: Open
Type: Producer/prepared authority idea
Parent: `ideas/closed/433_rv64_global_select_pointer_memory_residuals.md`
Source Evidence: `build/agent_state/433_step4_residual_disposition/`
Owning Layer: BIR/prepared memory authority before RV64 pointer-value memory lowering

## Goal

Establish explicit prepared authority for pointer-value memory accesses before
RV64 target lowering consumes them.

## Why This Exists

Idea 433 closed the bounded global residual triage and selected global
object-data packet, but `930930-1` still exposes pointer-value memory accesses
with unknown layout/provenance authority. RV64 must not lower those accesses by
guessing ownership or byte layout from pointer-shaped values.

## In Scope

- Audit pointer-value memory evidence from
  `build/agent_state/433_step4_residual_disposition/`.
- Define the producer/prepared facts needed to prove pointer-value memory
  layout, provenance, and address range authority.
- Add focused producer/prepared coverage for accepted authority and fail-closed
  missing/incoherent authority.
- Route any eventual RV64 lowering packet only after the facts are explicit.

## Out Of Scope

- Reopening bounded `inttoptr`/`ptrtoint` pointer-cast movement from idea 429.
- Inferring pointer-value memory ownership, layout, or bounds in RV64 from raw
  BIR, testcase names, integer values, or object labels.
- Global object-data emission already completed by idea 433.
- Store-source/global-memory publication, direct-global return/select-chain,
  and terminator/select publication work.

## Acceptance Criteria

- Pointer-value memory residual rows are classified by missing or coherent
  prepared authority with representative evidence.
- Required prepared facts are documented and covered by focused tests.
- Missing or incoherent authority remains fail-closed.
- Any target-lowering follow-up consumes explicit prepared facts rather than
  reconstructing authority locally.

## Reviewer Reject Signals

- Reject RV64 changes that infer pointer-value memory layout, provenance,
  range, or object ownership from raw pointer values, integer casts, filenames,
  function names, or object labels.
- Reject testcase-shaped fixes keyed to `930930-1` or one dump layout.
- Reject expectation downgrades, unsupported-marker edits, allowlists, or
  pass/fail accounting changes claimed as producer authority progress.
- Reject helper renames or diagnostic text changes presented as capability when
  the same access still has `layout_authority=unknown` or
  `range_verdict=unknown_compatible`.

