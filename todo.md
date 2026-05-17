Status: Active
Source Idea Path: ideas/open/266_aarch64_atomics_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Delete The Stale Markdown Shard

# Current Packet

## Just Finished

Completed Step 3 by deleting
`src/backend/mir/aarch64/codegen/atomics.md` after Step 2 established
`atomics.cpp`/`atomics.hpp` as the compiled AArch64 atomics owner boundary.

Updated the required non-lifecycle references:

- removed the stale `codegen/atomics.md` row from
  `src/backend/mir/aarch64/CLASSIFICATION_INDEX.md`
- updated `src/backend/mir/aarch64/BACKEND_CASE_BRINGUP_MATRIX.md` so the
  mixed future-feature row no longer requires the deleted markdown shard and
  names `atomics.cpp`/`atomics.hpp` as the current atomics owner

## Suggested Next

Proceed to Step 4 by recording focused backend behavior proof for the atomics
redistribution and confirming supported behavior is preserved while unsupported
behavior remains explicit.

## Watchouts

- Reference search with `rg` found no remaining `atomics.md` references under
  `src/backend/mir/aarch64` after the deletion and owned docs cleanup.
- Remaining `atomics.md` mentions are lifecycle/archive references in
  `plan.md`, `todo.md`, `ideas/open/*`, and `ideas/closed/*`; the packet
  explicitly left those untouched.
- Neighboring AArch64 markdown shards, implementation files, tests, `plan.md`,
  and idea files were not touched.

## Proof

Ran:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: PASS, backend subset `139/139` passed.

Proof log: `test_after.log`
