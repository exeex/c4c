# 155 BIR block-entry publication identity

## Goal

Add BIR-owned block-entry and current-block publication identity for semantic
value availability without importing publication hooks, homes, or storage
encoding.

## Why This Exists

Phase A classified current-block entry publication consumption as a semantic
CFG/value relationship. Prepared currently owns the query used by publication
and call consumers, but it is mixed with target publication mechanics.

Input artifact: `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`.

## In Scope

- Add block-entry/current-block publication relationships keyed by block,
  value name, and before-instruction index.
- Store source producer identity, produced BIR value/name, producer
  instruction/index, and source-producer kind.
- Bridge `PreparedCurrentBlockPublicationConsumption` and current-block entry
  publication reads for semantic source identity.
- Switch scalar publication planning reads only where they ask which semantic
  source is available.

## Out Of Scope

- Hook kind, destination home, storage encoding, stack-source extension policy,
  register view conversion, immediate publication payloads, emitted storage
  availability, or scalar publication emission policy.

## Acceptance Criteria

- BIR current-block publication queries match prepared source/value/producer
  identity for entry and same-block availability cases.
- Old prepared publication queries remain available as comparison oracles
  until the switch is proven.

## Proof Route

Run matching before/after coverage for dispatch publication and current-block
publication consumers. Include query-equivalence assertions for available and
unavailable publication consumption cases.

## Reviewer Reject Signals

- Turns BIR publication identity into a storage hook, destination-home, or
  register-view model.
- Changes publication ordering or emission before semantic query equivalence is
  proven.
- Uses expectation rewrites or classification-only changes as capability
  progress.

## Closure Note

Closed after the active runbook introduced BIR-owned semantic publication
identity queries for current-block and block-entry availability, proved the
prepared/BIR semantic identity boundary, and switched one narrow call-boundary
semantic source consumer while preserving prepared queries as readiness
oracles/fallbacks. Closure proof used matched canonical logs
`test_before.log` and `test_after.log` for
`backend_aarch64_instruction_dispatch` and
`backend_prepare_frame_stack_call_contract`, with monotonic regression guard
PASS.
