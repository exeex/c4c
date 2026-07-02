# Idea 521 Route8 Extraction Review

Active source idea path: `ideas/open/521_bir_route8_return_chain_body_extraction.md`

Chosen base commit: `746b90841 [plan] Activate BIR route8 return-chain extraction`

Base rationale: this is the lifecycle activation commit for the current idea 521 source idea and active `plan.md`/`todo.md` state. Later commits `fc2911213` and `ad32a44db` are todo-only execution records for this same activation, and `a8afdc328` is the route8 extraction slice under review.

Commit count since base: 3

Reviewed range: `746b90841..HEAD`, plus the current dirty `todo.md` step-title update.

## Findings

1. Low: `todo.md` records proof output at `test_after.log`, but `test_after.log` is not present in the current worktree.

   Evidence: `todo.md` records `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`, but `test_after.log` cannot be opened now. The code slice itself has a plausible proof path and `git diff --check` is clean, but acceptance should either restore the canonical proof log or re-run the same command before final lifecycle closure.

No blocking implementation findings.

## Alignment Review

The route8 extraction matches the source idea. The implementation moves the route8 return-chain bodies and route8-local anonymous helpers into `src/backend/bir/bir_route8.cpp` while preserving public declarations in `bir.hpp`; `bir.hpp` is unchanged in the reviewed range.

The moved route8 definitions still use the existing route1 identity APIs at `src/backend/bir/bir_route8.cpp:242` and `src/backend/bir/bir_route8.cpp:264`, with the same status selection, duplicate-record handling, record ordering, and optional/nullopt behavior visible in the moved body at `src/backend/bir/bir_route8.cpp:119`, `src/backend/bir/bir_route8.cpp:140`, `src/backend/bir/bir_route8.cpp:209`, and `src/backend/bir/bir_route8.cpp:318`.

There is no evidence of testcase-overfit. The reviewed range does not change test expectations, unsupported markers, allowlists, backend route diagnostics, or add printed-shape probes. The only test metadata change is adding `src/backend/bir/bir_route8.cpp` to the direct-source `backend_lir_to_bir_notes_test` executable at `tests/backend/bir/CMakeLists.txt:88`, which is required link/build metadata for the new translation unit.

The private `route_block_matches` boundary is acceptable and should not trigger a plan reset. It exactly preserves the old route7 block-match predicate: compare `BlockLabelId` when either side has a valid id, otherwise compare label text. The helper is private to the BIR implementation family in `src/backend/bir/bir_private.hpp:9`, is consumed by route7 and route8 only, and avoids moving route8 toward a route7 public API or route-index facade dependency.

No route1 semantic change was found. No route6 or facade coupling was introduced. Route7 dependency was narrowed from an anonymous route7 helper dependency into a route-neutral private block matcher, which is aligned with Step 2's recorded boundary decision.

## Judgments

Idea-alignment judgment: `matches source idea`

Runbook-transcription judgment: `plan matches idea`

Route-alignment judgment: `on track`

Technical-debt judgment: `acceptable`

Validation sufficiency: `narrow proof sufficient`, with the caveat that the canonical `test_after.log` artifact is currently absent and should be restored or regenerated before final acceptance.

Reviewer recommendation: `continue current route`
