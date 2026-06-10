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

## Closure Notes

Closed after the active runbook completed Step 6 acceptance review.

- BIR select-chain identity records and query APIs were added for source
  producer, direct-global dependency, scalar materialization eligibility, and
  combined identity lookup.
- Prepared/BIR equivalence proof covers select root, direct global root,
  local/no-dependency root, nested non-select direct-global root,
  before-boundary rejection, root type or name mismatch, and missing-root
  fail-closed paths.
- One route-local consumer switched its direct-global identity read to the BIR
  query while leaving prepared select-chain queries available as oracle and
  fallback authority for unswitched materialization, prealloc, and call-routing
  consumers.
- No target materialization cost, register availability, publication routing,
  call ABI behavior, or final AArch64 move/branch policy was added to BIR
  select-chain identity.

Accepted final proof scope:

```bash
set -o pipefail && cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure | tee test_after.log
```

Accepted result: `2/2` tests passed for `backend_prepared_lookup_helper` and
`backend_aarch64_instruction_dispatch`, preserved in `test_before.log` as the
rolled-forward final proof log.
