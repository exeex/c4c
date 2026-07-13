# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 now admits producer-valid ordinary function-pointer globals and
  fixed arrays of function pointers from structured `TypeSpec` authority.
- Return base kind/width, function-pointer identity, exact declarator depth,
  array dimensions, opaque storage, object/linkage/visibility/alignment, and
  semantic initializer function links survive Foundation, Raw BIR, and
  Canonical BIR.

## Suggested Next

- Audit the remaining producer-emitted global `TypeSpec` families and make the
  Step 3 checkpoint decision if no additional coherent family remains.

## Watchouts

- Function-pointer facts intentionally retain only the return base and
  declarator depth available on `LirGlobal`; no absent `FnPtrSig` is invented
  and no rendered signature spelling is parsed.
- Producer probes use `ptr_level` as the effective depth (1 for `T (*)()`, 2
  for `T (**)()`) and `inner_rank == -1` as the ordinary no-split sentinel.
- Aggregate/complex/signature-dependent return identity, bare function-object
  globals, references, split pointer-to-array shapes, vectors, mirrors, and
  negative/unsized/computed bounds remain closed.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records all 4/4 backend
  tests passing.
