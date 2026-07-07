Status: Active
Source Idea Path: ideas/open/564_bir_scalar_local_memory_semantic_admission_followup.md
Source Plan Path: plan.md
Current Step ID: 2/3
Current Step Title: Add Focused Semantic Admission Coverage + Narrow Fail-Closed Diagnostic

# Current Packet

## Just Finished

Step 2 - Add Focused Semantic Admission Coverage completed with the smallest
Step 3 diagnostic repair. Added generic focused BIR coverage for an
`unordered_float_uno_compare` LIR shape that lowers a floating `fcmp uno`
producer followed by `zext` and return; this is predicate-family coverage and
is not tied to an IEEE filename.

The coverage now asserts that unsupported `uno` does not silently collapse into
the umbrella `scalar/local-memory semantic family`. Since BIR currently has no
unordered floating compare opcode/fact representation, the coordinator emits a
narrow fail-closed producer-boundary note:

`unordered-float-compare scalar/local-memory semantic family`

Changed files:

- `src/backend/bir/lir_to_bir/memory/coordinator.cpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- `todo.md`
- `test_after.log`

## Suggested Next

Execute Step 4 from `plan.md`: prove the target IEEE representatives now
advance to the narrower unordered-float-compare scalar/local-memory diagnostic
instead of the umbrella scalar/local-memory family, then route the remaining
semantic representation work as the next owner decision.

## Watchouts

- This packet intentionally did not add a new BIR opcode or lower `uno` as an
  existing ordered/relational opcode; doing so would be semantically incorrect.
- No expectations, unsupported markers, allowlists, or IEEE filename shortcuts
  were changed.
- `ord` and `ueq` remain nearby unordered/ordered predicate-family questions;
  do not fold them into this slice without focused coverage and a semantic
  representation decision.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: pass. CTest reported `100% tests passed, 0 tests failed out of 346`.

Proof log: `test_after.log`.
