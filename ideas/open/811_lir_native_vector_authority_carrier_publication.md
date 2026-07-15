# Native Vector Authority Carrier Publication

Status: Open
Type: prerequisite for 754 vector-row authority selection
Predecessor: `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`

## Goal

Publish a reusable, checked native vector authority carrier for the existing
LIR vector producer/use seams: current-function result/use identities, vector
lane-count and element-type facts, index value/type facts, and exact shuffle
mask-lane facts.

## Why This Exists

754 Step 8 established that `LirInsertElementOp`, `LirExtractElementOp`, and
`LirShuffleVectorOp` are constructed from raw result/use strings and rendered
type/mask/index text.  No row can be selected without either forbidden display
recovery or a shared native carrier.  This idea owns only that prerequisite.

## In Scope

- Define a typed, reusable native vector fact carrier with lane count and
  element type, plus current-function `LirValueId` result/use identity.
- Define checked index value/type facts usable by insert/extract element.
- Define checked ordered shuffle mask-lane facts and vector input/result shape
  facts usable by shuffle.
- Publish the carrier from the existing vector lowering seams and verify
  ownership, coherence, and display-mirror compatibility without text recovery.
- Add nearby carrier-focused positive and malformed coverage, then obtain the
  build and regression proof appropriate to the changed shared LIR surface.

## Out Of Scope

- Implementing, enabling, or validating any `LirInsertElementOp`,
  `LirExtractElementOp`, or `LirShuffleVectorOp` row contract for 754.
- Choosing a 754 row, changing 754 tests, Raw-BIR, target lowering, MIR,
  emission, aggregate authority, or generic non-vector provenance.
- Parsing `%t`, LLVM display text, `"i64 0"`, `poison`, `zeroinitializer`,
  rendered vector types, or testcase names as authority.

## Acceptance Criteria

- Existing vector seams can carry checked current-function result/use IDs and
  native vector lane/element facts without relying on presentation text.
- Insert/extract index facts and shuffle mask lanes are structured, exact, and
  fail closed for missing, foreign, malformed, or incoherent carriers.
- The carrier is reusable by all three vector rows but does not itself claim
  any row-level operation capability.
- Fresh build and focused malformed/positive proof pass; shared-code changes
  receive the supervisor-selected regression and baseline proof.

## Return Contract

When accepted, return to 754 at Step 9 and rerun a fresh one-row vector audit
before selecting any implementation row.  Do not repeat 754 Steps 1--7.

## Reviewer Reject Signals

- Reject any patch that implements or verifies an 754 insert/extract/shuffle
  row while claiming only carrier publication.
- Reject recovered identity/layout/index/mask facts from rendered text,
  instruction order, `%t` spelling, or testcase names.
- Reject a carrier that accepts missing, foreign, or incoherent IDs/facts, or
  silently falls back to raw strings.
- Reject widening into aggregate, CFG/PHI, pointer/object, Raw-BIR, target,
  MIR, emission, or generic non-vector provenance work.
- Reject expectation downgrades, named-case-only behavior, helper renames, or
  classification-only changes claimed as authority publication.
