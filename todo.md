# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 now admits producer-valid positive fixed TypeSpec array globals
  whose elements are direct integer/floating scalars or exactly one-level
  pointers to those scalar bases. General Raw array facts preserve scalar
  pointee kind/width, element pointer depth, and exact outer-to-inner
  dimensions while nested LLVM spelling remains parity-only.
- Existing rank-one and multidimensional direct-scalar paths remain green. A
  multidimensional pointer-element extern proves opaque `ptr` nesting, object
  metadata, Foundation reachability, and Raw/Canonical publication; excessive
  pointer depth, aggregate pointees, pointer-to-array/inner-rank neighbors,
  function pointers, unexpected mirrors, parity conflicts, and malformed
  staged array facts reject transactionally.

## Suggested Next

- Execute one bounded Step 3 producer-valid pointer-typed extern-global
  declaration packet, first confirming whether the producer emits an
  `llvm_type_ref` mirror and which linkage/qualifier rows are authoritative.

## Watchouts

- Typed fixed-array admission requires rank within the eight-dimension producer
  capacity, positive active dimensions, `array_size == array_dims[0]`, absent
  `llvm_type_ref`, and an integer/floating scalar base after clearing array and
  pointer declarators. Only element pointer depth zero or one is admitted;
  aggregate/void/complex/va-list bases, references, pointer-to-array,
  function-pointer, vector, unsized, and deeper-pointer shapes remain closed.
- `llvm_type` is reconstructed from facts and compared exactly; it is never
  parsed and never substitutes for scalar pointee semantics. The generic
  `LirTypeRef` array value path remains an independent opaque contract.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records 4/4 backend tests
  passing. The selected proof covers direct-scalar and one-level
  pointer-element fixed arrays, `FoundationVerifier` reachability, Canonical
  publication, exact object-fact preservation, Raw array-fact coherence, and
  Raw/Canonical transactional rejection of neighboring malformed and
  unsupported shapes.
