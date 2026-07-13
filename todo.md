# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 const-pointer producer-alignment packet now imports actual
  ordinary const-pointer `lower_global` definitions whose optional
  `llvm_type_ref` is absent, using the exact one-level pointer shape in
  `LirGlobal::type` as semantic authority.
- Pointer receipt requires exact rendered `ptr` parity and exact agreement from
  any present structured mirror while preserving typed Pointer identity,
  visibility, opaque initializer payload, and ordered initializer links.

## Suggested Next

- Execute one bounded Step 3 aggregate-global receipt packet for an actual
  producer shape with structured aggregate identity, keeping flexible-member
  literal globals separate.

## Watchouts

- Pointer authority is deliberately limited to exactly one pointer level with
  no reference, array, pointer-to-array, inner-rank, or function-pointer shape.
- Only initialized, non-internal, non-weak const-pointer definitions with the
  producer's `global ` qualifier are admitted; pointer externs, non-const,
  internal, weak, constant-qualified, and multi-level variants remain closed.
- Rendered `llvm_type` is exact parity evidence only and is never parsed into
  semantics; a present `llvm_type_ref` must lower identically.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records 4/4 backend tests
  passing. The selected proof covers producer-shaped mirror-free const-pointer
  receipt and Raw/Canonical rollback for rendered/mirror conflicts plus
  pointer extern, qualifier, non-const, internal/weak, and multi-level shapes.
