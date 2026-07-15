# Native Vector Authority Carrier Publication

Status: Closed
Type: prerequisite for 754 vector-row authority selection
Predecessor: `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`

## Goal

Publish a reusable, checked native vector authority carrier for the existing
LIR vector producer/use seams: current-function result/use identities, vector
lane-count and element-type facts, index value/type facts, and exact shuffle
mask-lane facts.

## Disposition

**Capability complete.** The bounded carrier prerequisite is accepted and
returns control to 754 Step 9 for a fresh one-row vector audit. It does not
claim any insert-element, extract-element, or shuffle-vector row capability.

## Accepted Contract

- `76f92ad60` (*Publish native vector authority carrier*) publishes checked
  current-function result/use IDs, native vector lane/element facts, checked
  index value/type facts, and ordered shuffle mask-lane facts from the three
  existing vector seams.
- `56203cbf9` (*Cover native vector authority carrier*) adds nearby valid and
  malformed carrier coverage.
- Missing, foreign, malformed, and incoherent carriers fail closed. Display
  strings are compatibility mirrors only; no identity, layout, index, or mask
  fact is recovered from presentation text.
- The carrier is reusable by all three vector rows but neither selects nor
  verifies any row-level operation semantics.

## Accepted Proof

A fresh build plus matching canonical
`ctest --test-dir build -j --output-on-failure -R '^backend_'` captures in
root `test_before.log` and `test_after.log` passed from 5 to 6 tests with zero
failures; the supervisor accepted the monotonic regression guard. The
closure-quality fresh `cmake --build --preset default && ctest --test-dir
build -j --output-on-failure` checkpoint also passed 3038/3038.

## Parent Return Contract

Return to
`ideas/open/754_lir_aggregate_vector_value_identity_convergence.md` at Step
9. First rerun a fresh one-row vector audit against this carrier, then select
exactly one row only if its complete row-local contract is supported. Do not
repeat 754 Steps 1--8, redo carrier publication, parse display text, or claim
row capability from this prerequisite alone.

## Reviewer Reject Signals

- Reject any change that implements or verifies an 754 insert/extract/shuffle
  row while claiming only carrier publication.
- Reject recovered identity/layout/index/mask facts from rendered text,
  instruction order, `%t` spelling, or testcase names.
- Reject a carrier that accepts missing, foreign, or incoherent IDs/facts, or
  silently falls back to raw strings.
- Reject widening into aggregate, CFG/PHI, pointer/object, Raw-BIR, target,
  MIR, emission, or generic non-vector provenance work.
- Reject expectation downgrades, named-case-only behavior, helper renames, or
  classification-only changes claimed as authority publication.
