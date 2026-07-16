# LIR Nominal Vector Store and Vector-Schema Migration

Status: Closed
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

## Completion Disposition

Closed as capability-complete for the bounded first-owner vector-store route.
The accepted implementation introduced a module-owned `LirVectorRef` /
vector-store fact carrying lane count and typed element-family identity,
populated it for the selected scalar-to-vector splat producer, and migrated
required `LirInsertElementOp`, direct `LirExtractElementOp`, and required
scalar-splat `LirShuffleVectorOp` consumers to validate lane and element shape
through that store.

The route also migrated the selected shuffle mask and poison second-shape
checks to structured/store-backed facts, demoted the migrated insert, extract,
and scalar-splat shuffle row-local `LirNativeVectorShape` mirrors from
semantic authority, and added nearby malformed coverage for missing,
out-of-range, zero-lane, empty-element, lane-mismatched, element-mismatched,
wrong-family, and aggregate-element coherence cases.

Supervisor acceptance proof:

`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_native_vector_authority|llvm_gcc_c_torture_src_(pr60960_c|scal_to_vec1_c|scal_to_vec2_c))$' ) > test_after.log 2>&1`

Result: 4/4 tests passed, and `git diff --check` passed. Final proof was
recorded in commit `b4b01944a` after implementation commits through
`48d7702bb`.

Compatibility boundaries deliberately remaining for later owners:
`LirNativeVectorShape`, `vec_type`, `elem_type`, `mask_type`, `mask`,
`second_vector_shape`, `mask_lanes`, and related row-local fields still protect
unmigrated or non-required vector paths. This route does not claim universal
vector-schema deletion, generic type-family completion, aggregate identity
ownership beyond consuming accepted 838 aggregate-store facts, or reopening
accepted 754, 811, 814, or 815 capability.
