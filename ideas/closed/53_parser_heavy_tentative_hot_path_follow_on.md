# Parser Heavy Tentative Hot Path Follow-On

Status: Complete
Last Updated: 2026-04-10

## Goal

Reduce the remaining parser-heavy EASTL cost that still sits on the heavy
tentative rollback path after the first-wave lite-guard migrations.

## Why This Exists

The completed tentative-guard tiering slice proved that the new lite path is
correct and active on several common ambiguity probes, but the Release timing
shape on `tests/cpp/eastl/eastl_vector_simple.cpp` still remains:

- `ENABLE_HEAVY_TENTATIVE_SNAPSHOT=OFF`: `--parse-only` about `2.3s` to `2.4s`
- `ENABLE_HEAVY_TENTATIVE_SNAPSHOT=ON`: `--parse-only` about `11.3s` to `11.5s`

Temporary parser-debug instrumentation on the same workload also still showed
heavy tentative traffic dominating:

- heavy tentative totals: `enter=39704`, `commit=17465`, `rollback=22239`
- lite tentative totals: `enter=234`, `commit=129`, `rollback=105`

That means the first-wave migrations were not the dominant source of remaining
EASTL parser cost. The next slice should target the heavy path directly rather
than extending the completed plan opportunistically.

## Candidate Directions

1. Profile and tier the remaining hot heavy probes in template/declarator/type
   parsing where semantic environment mutations are still forcing snapshot use.
2. Replace whole-state heavy rollback with a journaled mutation checkpoint so
   speculative parsing can revert semantic state deltas without copying entire
   typedef/type/variable maps on every heavy probe.

## Constraints

- preserve parser correctness and rollback debuggability
- do not silently weaken semantic-state restoration on risky probes
- validate against EASTL `--parse-only` timing in Release
- keep the next plan scoped to one concrete mechanism at a time

## Completion Notes

- Baseline inspection identified the qualified-type spelling path as the
  hottest remaining heavy tentative region still showing up repeatedly in the
  EASTL vector parse-only workload.
- The completed slice moved
  `consume_qualified_type_spelling_with_typename` and
  `consume_qualified_type_spelling` from `TentativeParseGuard` to
  `TentativeParseGuardLite`. Those probes only consume tokens and assemble
  spelling metadata, so they do not need full semantic-environment snapshots.
- Added focused parser coverage in
  `tests/cpp/internal/parse_only_case/qualified_type_spelling_rollback_parse.cpp`
  to lock in rollback-sensitive behavior for a dependent qualified-name
  statement followed by a `typename` declaration.
- Validation stayed monotonic:
  - `test_before.log`: 3314 passed / 3316 total, 2 existing failures
  - `test_after.log`: 3315 passed / 3317 total, the same 2 existing failures
- Release EASTL vector parse-only timing after the change:
  - heavy snapshot build: `8.766s`
  - lite snapshot build: `2.439s`
- This improves on the pre-slice heavy baseline recorded above
  (`11.3s` to `11.5s`) while preserving parse correctness.

## Leftover Issues

- The heavy path is still materially slower than the lite path on
  `eastl_vector_simple.cpp`, so a follow-on slice could target the next hot
  guarded regions around qualified-base/type probing or pursue a journaled
  rollback design if parser profiling shows whole-state snapshots still
  dominate.
