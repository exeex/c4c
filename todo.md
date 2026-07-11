Status: Active
Source Idea Path: ideas/open/691_prepared_mir_view_equivalence_dump_comparator_mvp.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Typed Structural Snapshot And Comparator Skeleton

# Current Packet

## Just Finished

Completed Step 2 by adding a typed `PreparedMirCoreSnapshot` projection and a
`compare_prepared_mir_core_views(...)` comparator skeleton over view-exposed
core facts. The comparator reports blocking category differences without using
`canonical_dump()` string equality or raw `PreparedBirModule` layout.

Extended the focused MIR core view test to prove old-vs-old equality,
diagnostic/prepared-history exclusion, and one typed blocking string-constant
core-fact mutation. Follow-up tightened string constants so both the canonical
dump and typed snapshot include deterministic byte content (`bytes_hex`), and
the blocking mutation preserves string length to prove comparison is not
length-only.

## Suggested Next

Proceed to Step 3 when delegated: add the intended test-routing or caller-side
MVP around the typed comparator while keeping route text, diagnostics, and
target output out of semantic equality.

## Watchouts

- Do not compare or serialize the whole `prepare::PreparedBirModule`.
- Do not use route text, prepare notes, completed phases, proof text, rendered
  debug output, expectations, unsupported markers, allowlists, or target output
  as equality authority.
- The comparator currently reports category-level blocking differences; finer
  per-field diff rendering can be a later refinement if the supervisor asks.
- String constants now compare content via typed snapshot bytes, not only
  names/counts/lengths.
- Keep source/freshness authority work in the separate open idea 692 unless the
  supervisor explicitly switches lifecycle state.

## Proof

Required proof passed and wrote canonical `test_after.log`:
`set -o pipefail; (cmake --build build --target c4c_backend backend_prepared_mir_core_view_test -j && ctest --test-dir build -R '^backend_prepared_mir_' --output-on-failure) > test_after.log 2>&1`

Result: 1/1 selected test passed (`backend_prepared_mir_core_view`).

Diff hygiene passed:
`git diff --check -- src/backend/mir/prepared_view.hpp src/backend/mir/prepared_view.cpp tests/backend/mir/backend_prepared_mir_core_view_test.cpp todo.md`
