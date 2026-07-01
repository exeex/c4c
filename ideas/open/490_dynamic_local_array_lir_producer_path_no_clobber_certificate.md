# Dynamic Local-Array LIR Producer Path/No-Clobber Certificate

Status: Open
Type: BIR/prepared proof certificate producer idea
Parent: `ideas/closed/489_bir_dynamic_local_array_proof_population_from_lir_coordinates.md`
Source Evidence:
- `build/agent_state/489_step3_proof_population_route/route.md`
- `build/agent_state/489_step4_residual_disposition/disposition.md`
Owning Layer: BIR/prepared path coverage and no-clobber certificate producer keyed to LIR producer coordinates

## Goal

Publish durable path/dominance and dynamic-index same-value/no-clobber
certificates keyed to dynamic local-array `lir_producer_*` coordinates so the
idea 489 proof-population route can feed explicit facts into the idea 486
checker.

## Why This Exists

Idea 489 proved that `lir_producer_*` coordinates are sufficient binding keys,
and prepared branch/compare facts can identify candidate proof sources.
However, no producer currently certifies that a selected proof branch/path
covers the LIR producer site, dominates or guards it, and preserves the dynamic
index value through the interval.

## In Scope

- Audit prepared branch/compare proof sources and dynamic local-array
  `lir_producer_*` sites for certificate inputs.
- Define a certificate keyed by function, LIR producer block/index/role/key,
  proof source, selected edge/outcome, dynamic index identity, and proof bounds.
- Prove path coverage and dominance/guard validity from proof source to LIR
  producer site.
- Prove same-value/no-clobber interval facts for the dynamic index.
- Model fail-closed effects for redefinition/assignment, phi/alias,
  calls/helpers, inline asm, publications, move bundles, and parallel copies.
- Preserve statuses for missing proof source, unsupported predicate,
  operand-role mismatch, missing path, non-covering path, non-dominating proof,
  clobbered/redefined index, unresolved phi/alias, unknown effect,
  raw-shape-only inference, target/final-home-only inference, and
  prepared/BIR coordinate confusion.

## Out Of Scope

- Populating idea 486 checker inputs directly.
- Idea 484 packaging.
- Scalar local-load consumption.
- RV64/MIR lowering.
- Prepared traversal/BIR instruction coordinate conversion.
- Broad generic range analysis beyond dynamic local-array LIR producer
  certificate work.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.

## Acceptance Criteria

- Focused audit identifies whether current prepared/BIR surfaces can produce a
  bounded LIR-producer path/no-clobber certificate.
- The contract names required proof source, path coverage, dominance/guard, and
  no-clobber fields plus fail-closed statuses.
- Positive coverage proves at least one real dynamic local-array proof source
  has a certificate keyed to `lir_producer_*`.
- Negative coverage rejects non-covering paths, non-dominating proof,
  clobbered/redefined index, unresolved alias, unknown effect, raw-shape-only,
  target-only/final-home-only, and prepared/BIR coordinate confusion.
- Fresh residual disposition states whether idea 489 proof population can
  resume or whether another lower-level certificate/source owner remains first.

## Reviewer Reject Signals

Reject patches that:

- infer path coverage or no-clobber from branch proximity, loop shape, value
  names, testcase names, dump order, final homes, or RV64 target behavior;
- mark proof facts available without a durable certificate keyed to
  `lir_producer_*`;
- treat LIR producer instruction index as a prepared traversal/BIR instruction
  index without a separate conversion owner;
- combine certificate production with idea 489 checker input population, idea
  484 packaging, scalar local-load consumption, RV64/MIR lowering, or broad
  range analysis;
- weaken unsupported diagnostics, expectations, allowlists, pass/fail
  accounting, runtime comparison, or baseline logs as proof of progress.
