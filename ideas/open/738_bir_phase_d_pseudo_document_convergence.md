# BIR Phase D Pseudo Document Convergence

Status: Open
Type: Documentation-only architecture convergence
Phase Owner: D — pseudo formation and pre-allocation legalization
Predecessor: accepted `ideas/open/737_bir_phase_c_preparation_document_convergence.md`
Successor: `ideas/open/739_bir_phase_e_allocation_document_convergence.md`

## Goal

Converge the single ordered route that turns accepted Canonical/preparation
products into directly realizable preallocation pseudo BIR while keeping ABI
call transport, legalization, copy semantics, verification and later
allocation authority separate.

## Why This Exists

Pseudo formation is the last place semantic one-to-many expansion may occur;
unclear owners here would hide target work from ordinary analysis and
allocation or duplicate shared call lowering.

## Scope and Exact Owner Order

1. D1 `passes/pseudo_lowering/README.md` against
   `pseudo/README.md`: closed generic lowering to the admitted schema and first
   exact projected constraints.
2. D2 `passes/call_lowering/README.md`: sole shared ABI-aware call lowering for
   ordinary and helper calls on every supported target.
3. D3 shared verifier `Pseudo` publication gate.
4. D4 `passes/target/README.md`: mandatory target-realizability
   legalization/expansion chain and full Pseudo reverification.
5. D5 `passes/out_of_ssa/README.md`: initial phi-to-`ParallelCopy`/
   `CopyScratch` publication before E1, plus its explicitly subordinate
   post-stable-E3 copy-resolution closure inside E4's private transaction.

Audit shared core, constraints/projection, analysis/invalidation, verifier,
diagnostic and root clauses without reassigning them.

## Uniform Contract and Matrix Method

Apply the umbrella metadata/core-first format and exhaustive per-document
input/output matrices. Each meaningful node/form/product row names exact
producer/consumer, stable IDs, revision/profile/product keys, optional/error
forms, verifier gate, failure, invalidated analyses and checked implementation
truth. Missing real owners become indexed documentation placeholders. Every
mutation reprojects constraints through the sole C9 transaction and failure
publishes no private candidate or product.

## Phase Obligations and Downstream Handoff

- D1 has one disposition for every Canonical instruction and routes all calls
  to `GenericCall`; it performs no ABI placement or allocation.
- D2 alone expands calls into explicit abstract pseudo transport, fixed ABI
  slot requirements, clobbers and preservation requirements. No target wrapper
  may duplicate it or spell concrete registers, frame offsets, or machine
  opcodes.
- D3 is the shared publication gate, D4 is always-on even when its chain is
  empty, and every D4 expansion-introduced value participates in ordinary
  analysis/allocation.
- Initial D5 preserves simultaneous copy semantics and scratch identities for
  E1/E2. Only after stable E3 may D5 resolve copies; that result remains
  private to E4 and is projected together with E4 frame materialization.
- Phase E receives a reverified, directly one-record-realizable preallocation
  pseudo revision with exact projection, except the explicitly admitted
  initial D5 copy family that E owns to allocate before D5 closure.

## Non-Goals

Implementation/tests/build edits; target selection/preparation; home
assignment, spilling, frame placement, MIR/emission; target-specific duplicate
call lowering; or moving D5 copy resolution into E2/E3.

## Acceptance and Closure Criteria

- D1-D5 documents are uniquely owned and ordered, schema/lowering agree, D2
  is the sole shared ABI-aware call owner, and all D3/D4/D5 verifier gates and
  projection/invalidation seams are exact.
- The initial and post-E3 halves of D5 are explicit and non-duplicative.
- Input/output matrices prove the accepted C/D and D/E handoffs, including
  target-realizability, revision/product lineage and transactional failure.
- Closure records all owners/placeholders, matrices, implementation truth,
  adjacency proof, and any intentionally external target rule registry.

## Reviewer Reject Signals

- Code/test/build edits, phase mixing, hidden target call lowering, allocation
  or frame/MIR work in D, or D2 split by target.
- Assertion-only schema/consumer seam, status/heading-only edit, stale
  implementation claim, stale projection/analysis, or partial candidate leak.
- D4 omitted because a sample target needs no expansion, D5 bundles resolved
  before allocation, or scratch/edge identities inferred from positions.
- Testcase-shaped lowering, named-case matcher, rendered-text probe, allowlist,
  unsupported downgrade, expectation weakening, helper rename, or
  classification-only change claimed as progress.
- The old missing lowering/ABI/copy failure hidden behind a renamed owner.
