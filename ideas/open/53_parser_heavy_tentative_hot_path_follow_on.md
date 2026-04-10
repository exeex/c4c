# Parser Heavy Tentative Hot Path Follow-On

Status: Open
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
