# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 now normalizes the producer's ordinary no-split
  `TypeSpec.inner_rank == -1` sentinel across every already-supported global
  route while retaining compatibility with legacy/synthetic `0`.
- Representative scalar, pointer, fixed-array, vector, va-list, and direct
  aggregate globals retain identical typed and object facts through Foundation,
  Raw BIR, and Canonical BIR.

## Suggested Next

- Audit the remaining producer-emitted global `TypeSpec` families and make the
  Step 3 checkpoint decision if no additional coherent family remains.

## Watchouts

- Ordinary routes accept only `inner_rank` -1 or 0 and normalize local copies
  before scalar constant-type lowering; values below -1 and positive residue
  remain closed unless a pointer-to-array route owns the split rank.
- Pointer-to-array routes retain their existing positive/all-inner authority;
  the all-inner negative sentinel is now restricted to exactly -1.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records all 4/4 backend
  tests passing.
