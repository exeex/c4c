# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 now admits producer-valid ordinary, internal, and weak nonconst
  definitions of exactly one-level pointers to integer/floating scalar bases.
  Raw and Canonical BIR preserve pointee kind/width/depth, source order,
  identity, linkage, visibility, alignment, byte-exact opaque initializer
  payloads, and ordered initializer link identities.
- Existing coherence authority still rejects empty initializers, constant
  qualifiers on nonconst definitions, internal flag/linkage mismatch, and
  internal or weak const-pointer neighbors. Deeper pointers, aggregate and
  function pointees, and rendered/mirror conflicts remain transactional.

## Suggested Next

- Execute one bounded Step 3 packet admitting producer-valid internal and weak
  const definitions of exactly one-level pointers to integer/floating scalar
  bases. Preserve typed `PointerTypeFacts` together with opaque initializer
  payloads and ordered initializer link identities, enforce the authoritative
  linkage/visibility/qualifier facts, and prove Raw and Canonical publication
  plus transactional rejection and rollback. Keep deeper pointers, aggregate
  pointees, and function pointers closed; do not advance to the accumulated
  module checkpoint until these producer-valid rows are covered.

## Watchouts

- Scalar-pointer global authority comes only from a one-level, non-reference,
  non-array/non-vector/non-function-pointer `TypeSpec` whose cleared base lowers
  to an integer or floating scalar. Pointer definitions require a nonempty
  initializer payload, and the producer emits qualifier `global ` even when
  `is_const` is true; existing ordinary, internal, or weak coherence predicates
  remain authoritative for linkage, visibility, qualifier, and initializer
  shape.
- Producer globals omit `llvm_type_ref` for pointer shapes. A manually supplied
  mirror is accepted only as generic `ptr` corroboration and never supplies
  pointee semantics; rendered `llvm_type` is exact parity evidence only.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records 4/4 backend tests
  passing. The selected proof covers typed external, weak-external, const, and
  nonconst ordinary/internal/weak pointer globals, exact object and initializer
  preservation, `FoundationVerifier` reachability, Canonical publication, and
  Raw/Canonical rollback for unsupported neighboring shapes.
