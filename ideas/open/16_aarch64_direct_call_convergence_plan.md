# AArch64 Direct-Call Convergence Plan

Status: Open
Last Updated: 2026-03-30

## Relationship To Roadmap

Umbrella source: `ideas/closed/__backend_port_plan.md`

This plan aligns AArch64 backend minimal-slice direct-call behavior with the x86 direct-call testing baseline in:
- `tests/backend/backend_lir_adapter_tests.cpp`
- no workaround-only behavior; prefer parser/IR-driven minimal-slice matching.

## Goal

Converge AArch64 direct-call test coverage and behavior to the x86 baseline where it is already covered, so `ctest -L aarch64_backend` can identify true backend gaps instead of test asymmetry.

## Why this plan

Current work already has a growing set of AArch64 direct-call paths, including two-arg const-folding, but `tests/backend/backend_lir_adapter_tests.cpp` still has asymmetric cases compared with x86. This creates blind spots and slows regression triage.

## Scope

- Compare x86/aarch64 direct-call related test surface in one pass.
- Add AArch64 counterparts for missing x86-equivalent behavior.
- Keep tests minimal, explicit, and non-workaround.
- Keep this within `backend_lir_adapter_tests.cpp` for now; split to a dedicated test file only if file size becomes a maintenance issue.

## Acceptance Criteria

- AArch64 adds at least the direct-call coverage currently present only on x86, unless intentionally deferred.
- No behavioral regression in existing AArch64 direct-call fast path tests.
- `ctest -L aarch64_backend` improves or holds pass/fail trend with no accidental new failures.

## Planned Work

1. Add missing AArch64 direct-call test parity items:
   - aarch64 param-slot direct-call argument materialization (x86 counterpart: `test_x86_backend_renders_param_slot_slice`).
   - verify whether to add the missing spacing/non-spacing rewrite variants for parity where justified.

2. Validate assembler emission expectations for each new test:
   - confirm `bl` call path remains on asm seam for supported slices
   - confirm no LLVM fallback (`target triple =`) for those slices
   - confirm helper/function lowered as expected `.type ...` and ABI move sequence

3. Keep alignment checks focused:
   - keep one-to-one checks with x86 naming where applicable
   - annotate differences only when architecture-required (e.g., ABI register names).

4. Run focused validation and record outcomes:
   - `ctest -j --output-on-failure -L aarch64_backend`

## Risks / Non-goals

- Non-goals for this plan:
  - full backend feature expansion unrelated to direct-call slices
  - broader c-testsuite expansion beyond the direct-call path
  - architecture-wide optimization beyond minimal-slice correctness
