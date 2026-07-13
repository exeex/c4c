# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 now admits producer-valid internal and weak const definitions of
  exactly one-level pointers to integer/floating scalar bases while preserving
  the existing ordinary const-pointer and nonconst rows. Raw, Foundation, and
  Canonical BIR preserve typed `PointerTypeFacts`, source order, link-backed
  identity, decoded linkage and visibility, alignment, byte-exact opaque
  initializer payloads, and ordered initializer link identities.
- Explicit object-coherence rows keep constant qualifiers, missing
  initializers, linkage/flag and extern contradictions, deeper pointers,
  aggregate or function pointees, and rendered/mirror conflicts transactional.

## Suggested Next

- Execute the accumulated Plan Step 3 module-level checkpoint required by
  `plan.md`: audit the Steps 2-3 module families for any remaining generic
  unsupported valid LIR rows, then run the supervisor-selected broader proof.
  Treat any discovered uncovered producer family as a bounded Step 3 packet
  instead of declaring the checkpoint complete.

## Watchouts

- The scalar-pointer type constructor is intentionally broader than object
  publication: it constructs facts for initialized definitions regardless of
  constness, while explicit ordinary/internal/weak coherence rows remain the
  authority for qualifier, linkage, visibility, initializer, and flag shape.
- Internal and weak const pointer producers use qualifier `global `, not
  `constant `. Producer globals omit `llvm_type_ref` for pointer shapes; any
  supplied mirror remains generic `ptr` corroboration only.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records 4/4 backend tests
  passing. The selected proof covers typed external, weak-external, const, and
  nonconst ordinary/internal/weak pointer globals, exact object and initializer
  preservation, `FoundationVerifier` reachability, Canonical publication, and
  Raw/Canonical rollback for unsupported neighboring shapes and coherence
  contradictions.
