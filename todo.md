# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 now admits producer-valid mixed outer-array / pointer-to-inner-
  array globals from structured `TypeSpec` authority.
- Exact pointer depth, scalar/complex/VRM base facts, rendered outer dimensions,
  hidden pointee dimensions, `inner_rank`, source order, linkage, and initializer
  facts survive Foundation, Raw BIR, and Canonical BIR.

## Suggested Next

- Audit the remaining producer-emitted global `TypeSpec` families and make the
  Step 3 checkpoint decision if no additional coherent family remains.

## Watchouts

- Mixed receipt requires `0 < inner_rank < array_rank`; only the leading outer
  dimensions appear in array storage spelling around opaque `ptr`, while the
  trailing pointee dimensions remain structured facts. Pure pointer-to-array
  and ordinary pointer-element array routes remain separate.
- Negative/unsized or computed bounds, mirrors, aggregate/vector/va-list/
  function-pointer pointees, references, and spelling conflicts remain closed.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records all 4/4 backend
  tests passing.
