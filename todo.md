# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md

## Activation Decision

- 2026-04-14 activation result:
  activate `ideas/open/47_semantic_call_runtime_boundary.md` as the next
  executable backend follow-on
- rationale:
  this is the immediate continuation of the shared semantic-lowering route
  after the closed backend-reboot idea; `ideas/open/48_prepare_pipeline_rebuild.md`
  and `ideas/open/49_prepared_bir_target_ingestion.md` both depend on semantic
  BIR staying honest first, and idea 49 explicitly depends on both earlier
  tracks
- route constraint:
  keep this runbook focused on semantic call/runtime boundary work and do not
  silently pull `prepare` reconstruction or prepared-BIR target-ingestion work
  forward

## Current Active Item

- active route:
  semantic BIR remains the truth surface, `prepare` remains the legality
  owner, and target reconnection stays deferred to later follow-on ideas
- current capability family:
  semantic call ownership and runtime-facing lowering
- current packet focus:
  inspect `src/backend/lowering/lir_to_bir_calling.cpp`,
  `src/backend/lowering/lir_to_bir_memory.cpp`, and
  `src/backend/lowering/lir_to_bir.hpp` to name the first honest semantic miss
  after the accepted non-variadic call baseline
- packet rule:
  the first executor packet must change shared semantic lowering, not just add
  new test observers or rename unsupported cases
- proof expectation:
  choose a proving subset from backend-route or internal backend cases that
  exercises one semantic call/runtime family with a fresh build before broader
  validation

## Suggested Next

- scout the first bounded packet around one real semantic family:
  either broaden callee provenance / call-shape normalization in
  `lir_to_bir_calling.cpp` or add one runtime/intrinsic family that currently
  escapes semantic BIR
- when selecting proof, prefer a narrow backend subset that can demonstrate
  capability growth across more than one nearby shape instead of one named
  testcase
- if exploration shows the missing work is actually `prepare` legality or
  target-ingestion-only behavior, stop and checkpoint rather than expanding
  this runbook
