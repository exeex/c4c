# LIR Nominal Vector Store and Vector-Schema Migration

Status: Open
Type: first-owner vector shape/store convergence
Matrix Rows: M7
Dependencies: 838 when an element is aggregate; preserves 754, 811, 814, and 815

## Goal

Make `LirVectorRef`/store the sole owner of lane count and typed element family
ref, then migrate vector operation schemas and masks.

## In Scope

- Add the vector store/ref and migrate Insert/ExtractElement, shuffle, mask,
  and poison second-shape consumers without row-local shape authority.

## Out Of Scope

- Reopening accepted 754/811/814/815 capability, aggregate identity, or
  universal deletion.

## Acceptance Criteria

- Fresh build plus vector lowering/verifier/printer proof, operation coverage,
  malformed lane/element coherence, and wrong-family rejection.
- Remove `LirNativeVectorShape`, mask text, and operation type-string
  duplicates only after all vector schemas read the store with equivalent
  malformed checks.

## Reviewer Reject Signals

- Reject a renamed row-local shape mirror, text-derived element identity, or
  testcase-only vector repair.
- Reject weakening 814/815 fail-closed behavior or claiming generic type-family
  completion.
