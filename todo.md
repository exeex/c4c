Status: Active
Source Idea Path: ideas/open/691_prepared_mir_view_equivalence_dump_comparator_mvp.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Add Canonical Core View Dump

# Current Packet

## Just Finished

Added `PreparedMirCoreView::canonical_dump()` as a deterministic, line-oriented
core projection dump. The dump covers schema/version, target identity, module
data counts, all/defined function traversal, globals, string constants,
per-function control-flow/value-location/stack/addressing presence, lookup
cache counts, block bindings, and instruction cursors from the view surface.

Extended the focused MIR core view test to prove stable repeat output, expected
included core facts, and exclusion of prepare notes/completed phases/route-name
sentinels.

## Suggested Next

Proceed to Step 2 when delegated: build the comparator MVP on top of the dump
format without turning raw prepared-module layout or diagnostic text into
semantic equality authority.

## Watchouts

- Do not compare or serialize the whole `prepare::PreparedBirModule`.
- Do not use route text, prepare notes, completed phases, proof text, rendered
  debug output, expectations, unsupported markers, allowlists, or target output
  as equality authority.
- The new dump is a debug/projection aid only for this packet; structural
  equality/comparison is intentionally deferred.
- Keep source/freshness authority work in the separate open idea 692 unless the
  supervisor explicitly switches lifecycle state.

## Proof

Required proof passed and wrote canonical `test_after.log`:
`set -o pipefail; (cmake --build build --target c4c_backend backend_prepared_mir_core_view_test -j && ctest --test-dir build -R '^backend_prepared_mir_' --output-on-failure) > test_after.log 2>&1`

Result: 1/1 selected test passed (`backend_prepared_mir_core_view`).

Diff hygiene passed:
`git diff --check -- src/backend/mir/prepared_view.hpp src/backend/mir/prepared_view.cpp tests/backend/mir/backend_prepared_mir_core_view_test.cpp todo.md`
