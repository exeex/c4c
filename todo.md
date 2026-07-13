# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 now admits producer-valid VRM-register direct, deep-pointer, and
  fixed multidimensional pointer-element globals through existing typed
  `VrmRegister` width authority.
- Exact VRM width, reconstructed `c4c.vrmN` or opaque-pointer spelling, pointer
  depth, dimensions, and object/initializer facts survive Foundation, Raw BIR,
  and Canonical BIR.

## Suggested Next

- Audit the remaining producer-emitted global `TypeSpec` families and make the
  Step 3 checkpoint decision if no additional coherent family remains.

## Watchouts

- VRM storage semantics come only from `TypeSpec.base == TB_VRM_REGISTER` plus
  valid width 1/2/4/8; no LLVM spelling is parsed. Residual VRM metadata and
  vector, function-pointer, pointer-to-array, reference, aggregate, va-list,
  unexpected-mirror, and malformed-width neighbors remain closed.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records all 4/4 backend
  tests passing.
