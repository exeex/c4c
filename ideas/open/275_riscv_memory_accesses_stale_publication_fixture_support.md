# 275 RISC-V memory_accesses stale-publication fixture support

## Goal

Add supported fixture construction for a RISC-V prepared
`PreparedFunctionLookups::memory_accesses` stale-publication row so a later
bounded fail-closed proof can exercise the real same-consumer backend path
without synthetic mutation or hand-built stale state.

## Why This Exists

Idea 274 found that the supported RISC-V dynamic stack-source `LoadLocal`
fixture can publish the compatible prepared memory row, but normal construction
ties the public `memory_accesses` row to the same Route 3 / Route 5 memory
identity. The stale-publication proof cannot be built honestly until fixture
support can intentionally produce a stale public memory row through supported
preparation APIs.

## In Scope

- Extend the supported RISC-V prepared-fixture construction so a public
  `PreparedFunctionLookups::memory_accesses` row can intentionally differ from
  the current Route 3 / Route 5 memory-source identity.
- Keep the fixture tied to the real RISC-V same-consumer backend path rather
  than helper-only formatting or classification.
- Preserve the existing compatible `LoadLocal` publication fixture and exact
  `lw a1, 12(s2)` positive output.
- Name the prepared memory row, the current Route 3 / Route 5 authority row,
  and the stale public row relationship clearly enough for a later
  fail-closed proof idea to consume.

## Out of Scope

- Do not add the final stale-publication fail-closed proof in this fixture
  support idea unless a supervisor explicitly folds that proof into the active
  runbook.
- Do not use synthetic post-construction mutation of
  `PreparedFunctionLookups::memory_accesses`, Route 3 / Route 5 oracle rows, or
  lookup indexes as the accepted fixture mechanism.
- Do not broaden this into x86 parity, cross-publication, edge-publication,
  metadata, liveness, aggregate retirement, or draft 155 work.
- Do not weaken fallback, status, route-debug, prepared-printer, wrapper,
  baseline, unsupported, or exact output contracts.

## Acceptance Criteria

- A supported fixture path can construct a stale
  `PreparedFunctionLookups::memory_accesses` publication row without mutating
  prepared state after construction.
- The compatible RISC-V dynamic stack-source `LoadLocal` fixture remains
  byte-stable, including exact `lw a1, 12(s2)` output.
- The new fixture support records enough debug/status evidence to distinguish
  the public stale memory row from the current Route 3 / Route 5 memory-source
  identity.
- A follow-up stale-publication fail-closed proof can use the fixture without
  hand-built stale state or testcase-shaped shortcuts.
- Narrow backend proof covers the compatible path and the new supported stale
  fixture construction path.

## Reviewer Reject Signals

- Reject any route that relies on clearing or directly mutating
  `lookups.memory_accesses.accesses_by_result_value_id`, editing the Route 3 /
  Route 5 memory edge after construction, or otherwise hand-building stale
  state as the accepted fixture mechanism.
- Reject testcase-shaped shortcuts, named-case-only branches, or fixture code
  that only recognizes the old 274 target case.
- Reject helper-only proofs claimed as supported RISC-V backend fixture
  support.
- Reject expectation rewrites, unsupported downgrades, weaker status or
  fallback contracts, route-debug weakening, prepared-printer weakening,
  wrapper weakening, baseline weakening, or exact-output relaxation without
  explicit user approval.
- Reject broad Route 3 / Route 5 rewrites, prepared aggregate retirement, API
  privatization, x86 parity work, metadata/liveness work, or draft 155 work
  under this fixture-support idea.
- Reject changes that retain the exact old blocker where normal fixture
  construction cannot express a stale public `memory_accesses` row, even if the
  limitation is hidden behind a renamed helper or wrapper.

## Source

Split from blocked idea
`ideas/closed/274_phase_f5_riscv_memory_accesses_stale_publication_fail_closed_proof.md`.
