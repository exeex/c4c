# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 now admits producer-valid direct integer and floating vector
  globals from typed `TypeSpec` authority. Typed BIR preserves exact scalar
  element kind and width, lane count, producer storage bytes, spelling parity,
  and existing object/initializer facts through Foundation, Raw BIR, and
  Canonical BIR.
- Nearby coverage retains the existing scalar/global families and rejects
  nonpositive vector facts, spelling and mirror conflicts, excluded declarator
  shapes and bases, VRM metadata, residual vector facts without `is_vector`,
  and malformed staged `VectorTypeFacts` transactionally.

## Suggested Next

- Audit the next producer-valid Plan Step 3 globals/objects family, or make the
  Step 3 checkpoint decision if no additional producer-emitted family remains.

## Watchouts

- Direct vector authority is typed; LLVM vector spelling is reconstructed only
  for exact parity and is never parsed. Unfactored `LirTypeRef` vectors outside
  globals remain supported, while array, pointer/reference, function-pointer,
  aggregate, and other excluded vector shapes remain closed.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records all 4/4 backend
  tests passing.
