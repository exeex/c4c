# BIR Phase B Canonical Document Convergence

Status: Closed
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

## Completion Note

Accepted and closed after the supervisor-approved Step-9 proof in `769f0d012`.
The completed documentation chain is `c7bb43d3e`, `ce3dacf3f`, `878e56a97`,
`5eb4d6f43`, `b6cabf1d2`, `d4c73bdf4`, `c0e3cdc6e`, `938c7b43e`, and
`7b01fd0eb`.

All seven exact-revision analyses, P01-P07 pass owners, pass framework and
pipeline now document the strict earliest-consumer and `P01 -> ... -> P07 ->
B8` route. Framework, pipeline and shared Canonical-verifier authority remain
distinct; B8 alone mints one exact-revision, target-independent, unallocated
`CanonicalBir` accepted by C1. Failure is atomic, invalidation/stale-result
rules are explicit, and no Raw alias or target/ABI/allocation state crosses the
B/C boundary. Implementation truth remains 14 absent analysis/pass owners and
two partial-foundation framework/pipeline owners; documentation acceptance
does not claim runnable canonicalization. No code, test, build, LIR, idea 734,
target-aware work or downstream implementation was changed or activated.

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
