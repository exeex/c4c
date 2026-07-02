# Prepared Global-Data And Stack-Frame Infrastructure Review

Status: Open
Type: Infrastructure boundary review
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Owning Layer: Prepared contract and RV64 infrastructure boundary

## Goal

Classify current global-data, stack-frame, and prepared move-bundle
classification failures into prepared-contract work, RV64 infrastructure work,
or evidence gaps.

## Why This Exists

The current bucket map records 85 `unsupported_stack_frame`, 43
`unsupported_global_data`, and 26
`unsupported_prepared_move_bundle_classification` rows. Those rows touch
infrastructure facts that must be coherent before RV64 object lowering can
consume them.

## In Scope

- Review current stack-frame rows for prepared frame facts versus RV64 frame
  lowering gaps.
- Review current global-data rows for prepared symbol/data facts versus RV64
  addressing gaps.
- Review prepared move-bundle classification rows for missing authority or
  expected fail-closed behavior.
- Split resulting work into producer-owned and RV64-owned follow-up ideas.

## Out Of Scope

- Implementing global, frame, or move-bundle lowering in this review idea.
- Folding F128 frame or data needs into ordinary-C infrastructure priority.
- Replaying older helper cleanup ideas as current capability progress.
- Changing default CTest or gcc_torture expectations.

## Acceptance Criteria

- The three explicit current infrastructure buckets are classified by first
  owner and priority.
- Producer defects are separated from RV64 infrastructure lowering work.
- Any F128-primary rows are routed to the existing F128 quarantine lane.
- Follow-up ideas use current row evidence and concrete proof expectations.

## Reviewer Reject Signals

- Reject RV64 global or frame work that fabricates missing prepared symbols,
  stack slots, frame layout, or classification facts.
- Reject treating helper extraction, file ownership cleanup, or diagnostic
  renames as progress on these current rows.
- Reject combining prepared-contract repair and RV64 object lowering in one
  implementation slice.
- Reject expectation rewrites, unsupported downgrades, or allowlist filtering
  as evidence of infrastructure progress.
- Reject F128-driven frame/global changes unless they are isolated in the
  quarantine policy lane.

