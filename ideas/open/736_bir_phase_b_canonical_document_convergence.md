# BIR Phase B Canonical Document Convergence

Status: Open
Type: Documentation-only architecture convergence
Phase Owner: B — target-independent canonicalization
Predecessor: accepted `ideas/closed/735_bir_phase_a_import_raw_document_convergence.md`
Successor: `ideas/open/737_bir_phase_c_preparation_document_convergence.md`

## Goal

Converge the exact `P01` through `P07` target-independent canonical sequence
and the distinct B8 verification/publication boundary so one accepted phase-A
`RawBir` becomes one verified, unallocated `CanonicalBir`.

## Why This Exists

This prevents pass
framework, pipeline, analysis, pass, and verifier prose from duplicating
authority or accepting a handoff by assertion.

## Scope and Exact Owner Order

Request on-demand analyses immediately before their earliest consumer and
review these normative owners in order:

1. B1/P01 `passes/legalize/README.md`.
2. comparison/select analysis, then B2/P02 `passes/scalar/README.md`.
3. CFG analysis, then B3/P03 `passes/cfg/README.md`; recompute CFG afterward.
4. dominance analysis and publication/value-flow analysis at their earliest
   applicable SSA consumer, then B4/P04 `passes/ssa/README.md`.
5. memory-effects analysis (available from Raw) and provenance once the
   required CFG/dominance/typed-value facts exist, then B5/P05
   `passes/memory/README.md`.
6. B6/P06 `passes/aggregate/README.md`.
7. call-graph analysis at its earliest intrinsic/helper consumer (or earlier
   if a documented pass use proves it), then B7/P07
   `passes/intrinsics/README.md`; reuse/recompute memory effects here according
   to exact invalidation state.
8. B8 support and publication boundary, in this order:
   `passes/README.md` as the pass-framework and orchestration-support owner;
   `pipeline/README.md` as the ordered pipeline and capability boundary; then
   the shared verifier's `Canonical` publication gate.

`analysis/README.md` owns exact revision keys, dependency caching,
preservation/invalidation, and stale-result rejection. The child must place
each analysis at its earliest real consumer without turning it into a linear
stage. Shared root/core/verifier/diagnostic/support text is audited, not
silently reassigned.

## Uniform Contract and Matrix Method

Every owner must use the umbrella metadata spine (`Contract-Status`,
`Implementation-Status`, `Kind`, `Phase-ID`/`Applies-To`, `Upstream`,
`Downstream`, `Owner-Path`, `Last-Reconciled-Commit`) and core-first ownership,
input, output, adjacency ordering. Each gets exhaustive input and output
matrices naming variants, producer/consumer clauses, exact revision key,
stable IDs, validation and optional/error forms, verifier/publication gate,
failure, invalidation, and implementation truth. Missing real owners become
indexed documentation placeholders. Failure publishes no partial revision,
property stamp, cache entry, or capability.

## Non-Goals

Implementation, target/ABI facts, preparation, pseudo lowering, allocation,
MIR/emission, pass renumbering, mixed phase ownership, or weakening any test or
supported behavior.

## Acceptance and Closure Criteria

- P01-P07 identities and order are exact, each Raw-only form has one owner,
  every pass consumes the accepted predecessor revision, and all cumulative
  postconditions are explicit.
- At B8, framework ownership (transactions, occurrence, analysis access,
  invalidation and orchestration support) is distinct from pipeline ownership
  (configured order and capability boundary) and verifier ownership
  (Canonical acceptance/publication); B8 alone mints `CanonicalBir` after the
  full Canonical gate.
- Analyses are exact-revision immutable products requested at the earliest
  consumer and invalidated/recomputed after mutation unless preservation is
  proved.
- The downstream matrix proves phase C receives exactly verified
  target-independent `CanonicalBir`, with no Raw aliases, target facts,
  allocation state, stale analysis, or compatibility identity.
- Closure names all reviewed/created documents, matrices, implementation-truth
  corrections, invalidation results, and exact A/B and B/C adjacency proof.

## Reviewer Reject Signals

- Code/test/build edits, phase mixing, reordered/renumbered P01-P07, or target
  interpretation before C1.
- Pass-framework/pipeline/B8/verifier authority collapsed into one vague owner;
  analysis promoted to a stage; or assertion-only input/output compatibility.
- Heading/status-only edits, stale implementation claims, optional/error forms
  omitted, stale revision/cache reuse, or failed candidates publishing state.
- Named-test shortcuts, rendered-text probes, allowlists, unsupported
  downgrades, expected-output weakening, helper renames, or classification-only
  edits claimed as semantic progress.
- The same old canonicalization failure hidden behind a new abstraction name.
