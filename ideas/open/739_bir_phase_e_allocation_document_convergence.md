# BIR Phase E Allocation Document Convergence

Status: Open
Type: Documentation-only architecture convergence
Phase Owner: E — shared allocation and MIR-ready publication
Predecessor: accepted `ideas/open/738_bir_phase_d_pseudo_document_convergence.md`
Successor: `ideas/open/740_bir_phase_f_mir_boundary_document_convergence.md`

## Goal

Converge shared allocation from revision-bound liveness through atomic
MIR-ready publication, preserving the split between E1 fact authority, E2
policy authority, E3 graph mutation/retry, D5 copy closure, and E4 final
publication.

## Why This Exists

Allocation correctness depends on exact-current facts and one enclosing
publication transaction; blurred authority would admit stale products, hidden
frame work, or partial readiness capabilities.

## Scope and Exact Owner Order

1. E1 `analysis/liveness/README.md`: sole shared allocation liveness,
   interference and pressure facts.
2. E2 `regalloc/README.md`: sole shared pseudo-physical home-allocation policy
   for RV64, AArch64 and x86.
3. E3 `regalloc/spill_reload/README.md`: explicit spill/reload mutation and
   `E3 -> E1` retry until stable or rejected.
4. D5's already-owned post-stable-E3 copy-resolution sub-transaction, audited
   here only as E4 input; ownership remains phase D.
5. E4 `allocated/README.md`: enclosing `AllocatedPublicationTransaction`,
   private frame-action draft/materialization, final constraint projection,
   E1 recomputation, non-mutating E2/E3 validation, final frame plan and target
   realizability, then atomic `AllocatedBir`/`PreparedBir` ownership and
   borrowing `MirReadyBirView` publication.

Audit shared constraints, verifier/private-candidate profiles, target layout,
analyses, diagnostics and root clauses without duplicating authority.

## Uniform Contract and Matrix Method

Apply the umbrella metadata/core-first format and exhaustive input/output
matrices. Rows name exact revision/target/profile/projection/liveness/
assignment/spill/copy/frame/realizability keys, stable identities,
optional/error states, producer and consumer clauses, verifier gates,
invalidation, failure, and checked implementation truth. Missing real owners
become indexed documentation placeholders. Predecessor/draft products are
lineage only; exact-current products alone can publish.

## Allocation and Publication Obligations

- E1 owns immutable facts, not home policy. E2 owns legal abstract home choice
  and typed eviction requests, never graph mutation. E3 alone inserts admitted
  spill/reload nodes, reprojects, invalidates E1/E2, and retries at E1; scratch
  is non-spillable and shortages fail closed.
- Stable E3 output enters D5 copy resolution. D5 publishes nothing; its
  candidate/fingerprint/lineage stays inside E4.
- E4 is one atomic enclosing publication. It materializes bounded explicit
  frame-action nodes, then installs only final-current projection, E1, E2/E3
  validation, frame-realization and target-realizability products. No hidden
  frame action, unresolved copy, pressure deficit or mixed key may pass.
- Phase F receives only `MirReadyBirView` borrowing the same exact immutable
  revision and final products; it receives no repair authority.

## Non-Goals

Implementation/tests/build edits; target preparation or pseudo legalization;
MIR lowering, assembler/object/link work; target-specific allocators; spilling
scratch; or publishing intermediate assigned candidates.

## Acceptance and Closure Criteria

- E1/E2 authority split, E3 retry/invalidation, D5 seam and E4 atomic ordering
  are unambiguous and consistent across every owner and verifier profile.
- Frame/materialization/final-current product closure is exhaustive and
  failure-atomic; owning and borrowing capability semantics are exact.
- Matrices prove accepted D/E input and the exact `MirReadyBirView` E/F
  handoff, including every key/fingerprint and rejection mode.
- Closure records owners/placeholders, matrices, implementation truth,
  retry/invalidation proof and exact D/E and E/F adjacency.

## Reviewer Reject Signals

- Code/test/build edits, mixed phase ownership, target-specific allocation,
  MIR/frame work outside E4, or E2 graph mutation/E3 policy reassignment.
- E1 facts treated as policy; stale facts retained after E3; spill of scratch;
  D5 output published separately; or predecessor/draft products treated as
  final current.
- Assertion-only handoff, heading/status-only edits, partial publication, or
  implementation claims unsupported by current build/code/verifier truth.
- Named-test shortcuts, text probes, allowlists, unsupported downgrades,
  expectation weakening, helper renames, or classification-only edits claimed
  as capability.
- Hidden frame/copy/allocation work or the old pressure failure behind a new
  abstraction name.
