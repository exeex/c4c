# 253 Phase F3 prepared module structural exit blocker map

## Idea Type

analysis-only.

## Goal

Map the safe exit prerequisites for `PreparedBirModule::module`,
`PreparedBirModule::names`, `PreparedBirModule::control_flow`, and
`PreparedBirModule::store_source_publications` without proposing aggregate
retirement or field deletion.

## Why This Exists

Phase F2 found that these fields mix BIR semantic facts, retained diagnostic or
formatting compatibility, target policy, and public prepared authority. None is
deletion-ready, and each needs a smaller consumer and compatibility map before
implementation can be bounded.

## In Scope

- Split each field into route/BIR semantic facts, retained compatibility
  surfaces, target-policy surfaces, and blocked prepared authority.
- For `module`, distinguish BIR module/function/block structure from prepared
  aggregate handoff and printer/module output.
- For `names`, distinguish semantic name-to-id lookup from debug text,
  route-debug names, printer formatting, AArch64 formatting, fallback
  rendering, and duplicate/conflict name failures.
- For `control_flow`, distinguish CFG/dominance/block-index facts from branch
  lowering, layout, labels, and instruction spelling.
- For `store_source_publications`, distinguish source/publication semantic
  records from source-memory/status compatibility and addressing/storage output.

## Out Of Scope

- Deleting or privatizing any of these fields.
- Opening broad `PreparedBirModule` retirement or draft 155 work.
- Moving target policy into BIR.
- Rewriting printer, debug, route-debug, helper/oracle, fallback, or output
  strings.

## Acceptance Criteria

- The map names one or more future one-field or one-reader packets only where
  a semantic fact, consumer, compatibility surface, and fail-closed proof set
  are all concrete.
- Rows without concrete implementation shape remain blocked.
- The result preserves public prepared aggregate compatibility until all
  consumers migrate or a retained adapter is explicitly specified.

## Reviewer Reject Signals

- Reject named-case shortcuts that special-case one field, function name,
  block label, debug string, or source-publication row while claiming field
  exit readiness.
- Reject unsupported expectation downgrades, weaker printer/debug contracts,
  or route-debug/fallback string rewrites without explicit approval.
- Reject helper renames, output rewrites, or classification-only tables claimed
  as structural exit progress.
- Reject broad `PreparedBirModule`, `PreparedFunctionLookups`, CFG, naming,
  module traversal, or store-source rewrites outside the mapped sub-slice.
- Reject any route that retains the exact old public prepared authority behind
  a new BIR facade or wrapper name.

## Completion Summary

Closed after Step 6, `Normalize The Blocker Map And Proof Needs`.

The completed analysis produced a blocker map for `module`, `names`,
`control_flow`, and `store_source_publications`. It identified bounded future
one-reader/helper-row candidates only where the semantic fact, consumer,
retained compatibility surface, and fail-closed proof set are concrete.

The conclusion does not approve deletion, privatization, wrapping,
implementation, expectation rewrites, broad `PreparedBirModule` retirement, or
compatibility weakening. Rows without concrete implementation shape remain
blocked, and public prepared aggregate compatibility remains authoritative
until migrated consumers or retained adapters are explicitly specified.

The future implementation queue was split into
`ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md`
so this analysis idea can close without turning the closed archive into the
active work queue.
