# 153 BIR select-chain direct-global identity

## Goal

Add BIR-owned select-chain and direct-global dependency identity after the
producer/source identity foundation exists.

## Why This Exists

Phase A classified select-chain dependency/materialization as target-neutral
BIR dataflow. Prepared currently owns the query surface used by AArch64 routing
and call materialization; BIR should own the semantic subset before consumers
switch.

Input artifact: `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`.

## In Scope

- Add select-chain analysis keyed by block, root value, and
  before-instruction index.
- Record root producer value/name, root-is-select, root instruction index,
  direct `LoadGlobalInst` dependency, and scalar materialization eligibility.
- Bridge or compare against `find_prepared_select_chain_source_producer`,
  `find_prepared_direct_global_select_chain_dependency`, and
  `find_prepared_scalar_select_chain_materialization`.
- Switch select-materialization or direct-global consumers only after the
  producer/source foundation and query-equivalence proof exist.

## Out Of Scope

- Target materialization cost, hazard decisions, register availability,
  publication routing policy, call ABI behavior, or final AArch64 move/branch
  choices.

## Acceptance Criteria

- BIR select-chain queries match existing prepared answers for direct global
  roots, select roots, scalar materialization, and negative cases.
- Consumer switches remain route-local and keep prepared queries available as
  comparison oracles.

## Proof Route

Use matching before/after backend coverage around select materialization and
direct-global dependency consumers. Include query-equivalence checks for select
root, non-select root, direct global load, and no-dependency paths.

## Reviewer Reject Signals

- Treats select-chain handling as a call-specific shortcut.
- Adds AArch64 routing policy, register availability, or target materialization
  cost to BIR.
- Validates only one named select/global case while nearby select-chain shapes
  remain unexamined.
- Weakens or rewrites expectations instead of proving query equivalence.
