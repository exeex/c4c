# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 now admits producer-valid pointers to vectors and ordinary fixed
  arrays of vectors or pointers-to-vectors from structured `TypeSpec` authority.
- Exact scalar component kind/width, vector lanes/storage bytes, pointer depth,
  ordered dimensions, opaque/visible spelling, linkage, visibility, alignment,
  and initializer facts survive Foundation, Raw BIR, and Canonical BIR.

## Suggested Next

- Audit the remaining producer-emitted global `TypeSpec` families and make the
  Step 3 checkpoint decision if no additional coherent family remains.

## Watchouts

- Vector pointee/element facts are nested typed authority; visible vector
  spelling is used only for direct elements, while positive pointer depth uses
  opaque `ptr` without parsing.
- Split pointer-to-array shapes, references, function pointers, mirrors,
  negative/unsized/computed bounds, and enum/complex/VRM/aggregate/va-list
  vector bases remain closed.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records all 4/4 backend
  tests passing.
