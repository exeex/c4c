# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 now admits producer-valid positive fixed multidimensional direct
  scalar TypeSpec array globals with no `llvm_type_ref`. Raw `TypeKind::Array`
  carries scalar element kind/width plus exact outer-to-inner dimensions, and
  reconstructs nested spelling inside-out without parsing compatibility text.
- Rank-one definition and extern coverage remains green; a distinct `2 x 3 x
  7` extern proves ordered facts, object metadata, Foundation reachability, and
  Raw/Canonical publication. Rank/capacity mismatch, nonpositive inner and
  outer-size mismatch, unexpected mirrors, nested parity conflicts,
  pointer/aggregate neighbors, and malformed staged Raw facts reject.

## Suggested Next

- Execute one bounded Step 3 producer-valid fixed pointer-element TypeSpec
  array-global receipt packet, first confirming declarator shape and opaque
  pointer spelling invariants while retaining absent `llvm_type_ref` for arrays.

## Watchouts

- Fixed scalar array admission requires rank within the eight-dimension
  producer capacity, every active dimension positive, `array_size ==
  array_dims[0]`, a direct integer/floating element, and absent
  `llvm_type_ref`. Aggregate-element, pointer-element, unsized, vector,
  reference, and function-pointer shapes remain closed.
- The generic `LirTypeRef` array value path preserves its existing typed
  discriminant plus opaque exact spelling contract without decoding element or
  extent semantics. Global TypeSpec array authority is separate and never
  parses `llvm_type`.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records 4/4 backend tests
  passing. The selected proof covers ordered multidimensional scalar array
  receipt, `FoundationVerifier` reachability, Canonical publication, exact
  object-fact preservation, Raw type-fact coherence, and Raw/Canonical
  transactional rejection of neighboring malformed and unsupported shapes.
