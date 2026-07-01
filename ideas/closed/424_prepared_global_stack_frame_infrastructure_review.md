# Prepared Global Data And Stack Frame Infrastructure Review

Status: Closed
Type: Infrastructure triage idea
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Owning Layer: Prepared contract and RV64/MIR infrastructure
Handoff Directory: `docs/rv64_gcc_torture_post_contract/`

## Goal

Review remaining prepared global-data and stack-frame failures and split them
into producer-contract versus RV64 emission follow-up ideas.

## Why This Exists

Global data, relocations, zero-fill, selected object data, stack-frame layout,
save slots, and param homes affect ordinary C coverage. They should outrank
low-priority F128 work.

## In Scope

- Bucket `unsupported_global_data`, `unsupported_stack_frame`, and related
  param-home failures.
- Distinguish missing/incoherent prepared facts from coherent but unlowered
  RV64 forms.
- Propose separate prepared-contract and RV64-emission follow-up ideas.
- Preserve fail-closed diagnostics for incomplete object data or stack facts.

## Out Of Scope

- F128 local memory, F128 parameter homes, or F128 ABI plumbing.
- Reconstructing object data or frame facts in RV64 from raw BIR/source.
- Expectation, allowlist, unsupported-marker, or runtime-comparison changes.

## Acceptance Criteria

- Global-data and stack-frame failures have counts, representative rows, and
  owning-layer classifications.
- Producer gaps are separated from target-emission gaps.
- Follow-up ideas cite the handoff rows they consume.
- Default CTest remains stable.

## Reviewer Reject Signals

- Reject RV64 inference of missing labels, relocations, zero-fill ranges,
  frame slots, offsets, widths, or param homes.
- Reject combining producer repair and RV64 emission in one implementation
  idea.
- Reject F128-specific expansion under this infrastructure review.
- Reject diagnostic-only changes claimed as capability progress.

## Closure Note

Closed after the active runbook produced row-backed infrastructure evidence,
the global/stack-frame handoff, and five focused follow-up ideas:

- `ideas/open/508_rv64_prepared_callee_saved_gpr_save_slot_emission.md`
- `ideas/open/509_rv64_fixed_prepared_stack_frame_emission.md`
- `ideas/open/510_rv64_selected_object_data_emission.md`
- `ideas/open/511_static_local_object_data_contract_publication.md`
- `ideas/open/512_stack_passed_parameter_home_publication.md`

The review route is complete. Implementation remains intentionally delegated
to those follow-up ideas, preserving the RV64 emission versus producer-contract
owner split and keeping F128-specific work out of scope.
