Status: Active
Source Idea Path: ideas/open/691_prepared_mir_view_equivalence_dump_comparator_mvp.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Integrate Focused Tests And Build Routing

# Current Packet

## Just Finished

Completed Step 3 by splitting the dump/comparator assertions into a dedicated
`backend_prepared_mir_core_comparator` CTest target registered under
`tests/backend/mir/CMakeLists.txt`. The original
`backend_prepared_mir_core_view` test remains focused on the core view accessor
contract, while the new target carries deterministic dump coverage, diagnostic
history exclusion, comparator equality, and typed core divergence proof.

## Suggested Next

Proceed to the next delegated packet or supervisor acceptance review for the
MVP proof surface.

## Watchouts

- Do not compare or serialize the whole `prepare::PreparedBirModule`.
- Do not use route text, prepare notes, completed phases, proof text, rendered
  debug output, expectations, unsupported markers, allowlists, or target output
  as equality authority.
- `backend_prepared_mir_core_comparator` is a new CTest target and matches the
  delegated `^backend_prepared_mir_` proof regex.
- Keep source/freshness authority work in the separate open idea 692 unless the
  supervisor explicitly switches lifecycle state.

## Proof

Required proof passed and wrote canonical `test_after.log`:
`set -o pipefail; (cmake --build build --target c4c_backend backend_prepared_mir_core_view_test backend_prepared_mir_core_comparator_test -j && ctest --test-dir build -R '^backend_prepared_mir_' --output-on-failure) > test_after.log 2>&1`

Result: 2/2 selected tests passed (`backend_prepared_mir_core_view`,
`backend_prepared_mir_core_comparator`).

Diff hygiene passed:
`git diff --check -- tests/backend/mir/CMakeLists.txt tests/backend/mir/backend_prepared_mir_core_view_test.cpp tests/backend/mir/backend_prepared_mir_core_comparator_test.cpp todo.md`
