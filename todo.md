# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 now admits producer-valid fixed-size, one-dimensional direct
  scalar TypeSpec array globals with no `llvm_type_ref`. Raw `TypeKind::Array`
  carries coherent typed scalar element kind/width plus positive extent, and
  the importer renders exact `llvm_type` parity from those structured facts
  without parsing compatibility text.
- Nearby proof covers an initialized weak constant definition and an extern
  declaration through Foundation verification and Raw/Canonical publication,
  including exact object/linkage/visibility/alignment facts, opaque initializer
  bytes, and ordered links. Invalid extents/dimensions, rank greater than one,
  pointer/aggregate elements, unexpected mirrors, parity conflicts, and
  malformed staged Raw array facts reject transactionally.

## Suggested Next

- Execute one bounded Step 3 multidimensional scalar TypeSpec array-global
  receipt packet, first confirming the producer's outer-to-inner
  `array_dims`/`array_size` invariants and choosing a non-recursive typed Raw
  shape extension that keeps rendered LLVM spelling parity-only.

## Watchouts

- Fixed scalar array admission is intentionally limited to rank one, matching
  positive `array_size == array_dims[0]`, a direct integer/floating element,
  no vector/pointer/reference/function-pointer/aggregate shape, and absent
  `llvm_type_ref`. Aggregate-element, pointer-element, unsized, and
  multidimensional globals remain closed.
- The generic `LirTypeRef` array value path preserves its existing typed
  discriminant plus opaque exact spelling contract without decoding element or
  extent semantics. Global TypeSpec array authority is separate and never
  parses `llvm_type`.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records 4/4 backend tests
  passing. The selected proof covers typed fixed scalar array receipt,
  `FoundationVerifier` reachability, Canonical publication, exact object-fact
  preservation, Raw type-fact coherence, and Raw/Canonical transactional
  rejection of neighboring unsupported global shapes.
