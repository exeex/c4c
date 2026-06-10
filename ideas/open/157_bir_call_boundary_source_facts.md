# 157 BIR call-boundary source facts

## Goal

Add BIR-owned call-boundary semantic source facts for call arguments/results
while preserving ABI placement and target lowering outside BIR.

## Why This Exists

Phase A classified call-boundary source/dependency identity as a BIR candidate
only for semantic source, dependency, and materializable-producer facts.
Prepared call plans currently mix those facts with ABI registers, stack areas,
aggregate lanes, scratch, and helper policy.

Input artifact: `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`.

## In Scope

- Add call-use source relationships attached to `bir::CallInst` arguments and
  results.
- Store source value/base value/name, target-neutral pointer delta where
  applicable, materializable same-block producer, direct global select-chain
  dependency, memory/access source link, and publication-source route identity.
- Bridge call argument source-producer materialization, direct-global
  dependency, and publication-source routing semantic fields.
- Switch call materialization/routing reads one argument or result class at a
  time after query equivalence.

## Out Of Scope

- ABI register or stack placement, outgoing stack sizing, variadic FPR count,
  preservation/clobber sets, byval aggregate lanes, scratch requirements,
  destination homes, helper/carrier protocols, or aggregate transport lane
  layout.

## Acceptance Criteria

- BIR call queries match prepared semantic call-source answers one
  argument/result class at a time.
- Consumer switches are limited to materialization or routing reads that no
  longer need ABI placement.
- Existing call plans remain the comparison oracle during migration.

## Proof Route

Run matching before/after backend call/codegen coverage with targeted
equivalence for same-block producer, select-chain direct global, memory-source,
publication-source, and negative call-argument cases. Escalate to broader
backend if ABI or aggregate files change.

## Reviewer Reject Signals

- Copies `PreparedCallArgumentPlan`, aggregate transport shapes, ABI placement,
  scratch policy, variadic state, preservation/clobber state, or helper/carrier
  protocols into BIR.
- Changes ABI placement while claiming semantic migration.
- Validates only one named call shape without nearby argument/result coverage.
