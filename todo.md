# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 now admits producer-valid external and weak-external declarations
  of exactly one-level pointers to integer/floating scalar bases. Raw pointer
  facts preserve pointee kind, pointee width, and pointer depth without parsing
  opaque `ptr` spelling; existing admitted const-pointer definitions now retain
  the same typed facts.
- Generic fact-free Raw pointers remain valid for unrelated `LirTypeRef`
  contracts. Malformed staged pointer facts and unsupported neighboring global
  shapes reject transactionally, including deeper pointers, aggregate
  pointees, pointer-to-array/inner-rank and function-pointer forms, parity or
  mirror conflicts, and ordinary nonconst pointer definitions.

## Suggested Next

- Execute one bounded Step 3 packet admitting producer-valid ordinary nonconst
  definitions of exactly one-level pointers to integer/floating scalar bases.
  Preserve typed `PointerTypeFacts` together with the existing authoritative
  linkage and initializer facts, and prove Raw/Canonical publication plus
  transactional rejection and rollback. Keep deeper, aggregate-pointee, and
  function-pointer forms closed; leave intrinsic-requirement parity for the
  later instruction/intrinsic work that supplies its typed authority.

## Watchouts

- Scalar-pointer global authority comes only from a one-level, non-reference,
  non-array/non-vector/non-function-pointer `TypeSpec` whose cleared base lowers
  to an integer or floating scalar. The branch is gated to extern declarations
  and the preexisting const-pointer producer row; ordinary nonconst pointer
  definitions remain closed.
- Producer globals omit `llvm_type_ref` for pointer shapes. A manually supplied
  mirror is accepted only as generic `ptr` corroboration and never supplies
  pointee semantics; rendered `llvm_type` is exact parity evidence only.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records 4/4 backend tests
  passing. The selected proof covers typed external, weak-external, and const
  pointer globals, exact object-fact preservation, `FoundationVerifier`
  reachability, Canonical publication, malformed Raw pointer-fact rejection,
  and Raw/Canonical rollback for unsupported neighboring shapes.
