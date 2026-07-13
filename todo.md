# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 now admits producer-valid direct named struct and union extern
  global declarations when `llvm_type_ref` carries an exact, resolvable
  `StructNameId`. External and weak-external rows preserve link-backed or
  fallback identity, source order, visibility, alignment, and no initializer
  through verified Raw BIR and Canonical BIR.
- Named definitions and flexible literal struct definitions retain their
  existing paths; unkeyed literal aggregate externs and malformed authority or
  declaration facts still reject transactionally.

## Suggested Next

- Audit the next producer-valid Plan Step 3 globals/objects family, or make the
  Step 3 checkpoint decision if no additional producer-emitted family remains.

## Watchouts

- Direct aggregate externs require a named structured mirror with a valid,
  resolvable `StructNameId` and exact rendered spelling parity. Do not widen
  this route to literal aggregates, pointer/array aggregate neighbors, or
  initializer-bearing declarations without separate producer evidence.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records all 4/4 backend
  tests passing.
