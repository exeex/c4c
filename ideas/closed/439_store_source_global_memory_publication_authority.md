# Store-Source Global-Memory Publication Authority

Status: Closed
Type: Producer/prepared publication idea
Parent: `ideas/closed/433_rv64_global_select_pointer_memory_residuals.md`
Source Evidence: `build/agent_state/433_step4_residual_disposition/`
Owning Layer: Store-source and global-memory publication before RV64 lowering
Closed Evidence:
- `build/agent_state/439_step4_residual_disposition/classification.md`
- `build/agent_state/439_step4_residual_disposition/evidence_snippets.txt`
- `build/agent_state/439_step3_global_memory_publication_coverage/summary.md`

## Goal

Repair or classify store-source and global-memory publication authority so RV64
global load/store lowering only consumes explicit prepared facts.

## Why This Exists

Idea 433 completed selected global object-data support, but residual rows still
include global memory publication mixed with `source_producer=unknown`, local
store publication, and direct-global chains. Those are not safe to lower by
guessing the store source or global layout in the target backend.

## In Scope

- Audit residual global memory publication rows from
  `build/agent_state/433_step4_residual_disposition/`, including `930930-1`,
  `20000622-1`, and `20041112-1`.
- Separate coherent global load/store target packets from producer gaps such as
  `source_producer=unknown`.
- Define prepared publication facts required for store sources, global symbols,
  offsets, widths, and value homes.
- Add focused producer/prepared coverage and fail-closed diagnostics.

## Out Of Scope

- Reopening selected global object-data support completed by idea 433.
- Inferring store source, global object identity, offsets, layout, or
  addressability from raw BIR or representative filenames.
- Pointer-value memory layout/provenance authority.
- Direct-global return/select-chain authority and terminator/select
  publication.

## Acceptance Criteria

- Store-source/global-memory residuals are bucketed into producer gaps versus
  coherent target work with representative evidence.
- `source_producer=unknown` cases either receive explicit producer facts or
  remain fail-closed with a precise diagnostic.
- Focused tests prove accepted publication facts and missing/incoherent
  boundaries.
- Any later RV64 packet consumes prepared publication authority only.

## Completion Notes

- Step 1/2 classified residual global-memory rows and defined the publication
  authority contract.
- Step 3 added `prepared_global_symbol_memory_has_publication_authority` and
  `prepared_store_global_publication_has_authority` with focused fail-closed
  tests.
- Step 4 confirmed representative rows still fail because producer facts are
  absent, including `layout_authority=unknown`, `source_producer=unknown`, and
  missing explicit immediate-source encoding.
- No RV64 target-lowering packet is justified while those producer facts are
  absent.
- Remaining producer work is split into:
  `ideas/open/446_global_memory_layout_authority_publication.md` and
  `ideas/open/447_immediate_global_store_source_encoding.md`.
- Close gate used canonical `test_before.log` and `test_after.log`; both logs
  report 327 passed, 0 failed, and the non-decreasing regression guard passed.

## Follow-Up Ideas

- `ideas/open/446_global_memory_layout_authority_publication.md`
- `ideas/open/447_immediate_global_store_source_encoding.md`

## Reviewer Reject Signals

- Reject target changes that infer store-source identity, global layout,
  addressability, offset meaning, or value home from raw BIR, testcase names,
  or object labels.
- Reject testcase-shaped fixes keyed to `930930-1`, `20000622-1`,
  `20041112-1`, one global symbol name, or one dump layout.
- Reject unsupported downgrades, allowlists, expectation rewrites, or runtime
  pass/fail changes claimed as publication progress.
- Reject classification-only edits claimed as capability while residual rows
  still report `source_producer=unknown` without a durable follow-up.

